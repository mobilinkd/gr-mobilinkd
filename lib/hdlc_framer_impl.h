// Copyright 2012 mobilinkd <rob@pangalactic.org>
// All rights reserved.

#ifndef GR__MOBILINKD__HDLC_FRAMER_IMPL_H_
#define GR__MOBILINKD__HDLC_FRAMER_IMPL_H_

#include "hdlc_framer.h"
#include "ax25_frame.h"

#include <boost/scoped_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <algorithm>

namespace gr { namespace mobilinkd {

namespace detail {

/**
 * This implements a state machine for HDLC frame parsing.  It uses
 * a 16-bit (2-byte) buffer to scan for flags and data.
 *
 * There are three states: SEARCH, HUNT, FRAME
 *
 * The state machine starts in the SEARCH state.  In this state it is
 * looking for a FLAG byte.  Once it encounters a FLAG, it enters into
 * the HUNT state.
 *
 * In the HUNT state it is searching for a non-FLAG symbol.  It stays
 * in HUNT while it encounters FLAG symbols.  If it encounters a
 * non-FLAG symbol with six consecutive bits set, it transitions back
 * to HUNT.  Otherwise it transitions to FRAME.
 *
 * In the FRAME state, bytes are accumulated into a buffer.  The
 * framing code processes individual bits, removing any stuffed 0s
 * that have been embedded in the bit stream. There are a number
 * of possible transitions from FRAME:
 *
 * - A FLAG is encountered but the frame is too small to be valid.
 *   In this case the frame is aborted and the state transitions
 *   to HUNT.
 * - A FLAG is encountered and the frame is a valid size.  In this
 *   case the frame is emitted and the state transitions to HUNT.
 * - The frame exceeds the valid frame size.  In this case the frame
 *   is aborted and the state transitions to SEARCH.
 * - An invalid bit sequence is encountered.  In this case the frame
 *   is aborted and the state transitions to SEARCH.
 *
 * Bits are pushed
 */
struct hdlc_state_machine
{
    static const uint16_t FLAG = 0x7E00;
    static const uint16_t ABORT = 0x7F;
    static const uint16_t IDLE = 0xFF;

    enum state {SEARCH, HUNT, FRAMING};

    state state_;
    int ones_;
    uint16_t buffer_;
    std::string frame_;
    bool ready_;
    int bits_;
    boost::asio::io_service io_;
    boost::asio::deadline_timer timer_;
    boost::posix_time::time_duration timeout_;
    boost::scoped_ptr<boost::thread> thread_;
    bool passall_;

    hdlc_state_machine(bool pass_all)
    : state_(SEARCH), ones_(0)
    , buffer_(0), frame_(), ready_(false), bits_(0)
    , io_(), timer_(io_), timeout_(boost::posix_time::seconds(2))
    , thread_(), passall_(pass_all)
    {
        thread_.reset(new boost::thread(&hdlc_state_machine::async_thread, this));
    }

    void async_thread()
    {
        std::clog << "thread running..." << std::endl;
        io_.run();
    }

    void timer_handler(const boost::system::error_code& ec)
    {
        if (!ec)
        {
            state_ = SEARCH;
        }
    }

    void start_timer()
    {
        timer_.expires_from_now(timeout_);
        timer_.async_wait(
            boost::bind(&hdlc_state_machine::timer_handler, this, _1));
    }

    void cancel_timer()
    {
        boost::system::error_code ec;
        timer_.cancel(ec);
    }

    void add_bit(char c)
    {
        const uint16_t BIT = 0x8000;

        buffer_ >>= 1;
        buffer_ |= (c ? BIT : 0);
        bits_++;
    }

    char getchar()
    {
        assert(bits_ == 16);

        char result = (buffer_ & 0xFF);

        return result;
    }

    void consume_byte()
    {
        const uint16_t MASK = 0xFF00;

        buffer_ &= MASK;
        bits_ -= 8;
    }

    void consume_bit()
    {
        const uint16_t MASK = 0xFF00;

        uint16_t tmp = (buffer_ & 0x7F);
        tmp <<= 1;
        buffer_ &= MASK;
        buffer_ |= tmp;
        bits_ -= 1;
    }

    void go_search()
    {
        state_ = SEARCH;
        cancel_timer();
    }

    bool have_flag()
    {
        const uint16_t MASK = 0xFF00;

        return (buffer_ & FLAG) == FLAG;
    }

    void go_hunt()
    {
        state_ = HUNT;
        bits_ = 0;
        buffer_ = 0;
        start_timer();
    }

    void search(char c)
    {
        const uint16_t MASK = 0xFF00;

        add_bit(c);

        if (have_flag())
        {
            go_hunt();
        }
    }

    bool have_frame()
    {
        const uint16_t MASK = 0xFF00;

        if  (bits_ != 8) return false;

        const uint16_t test = (buffer_ & MASK);

        switch (test)
        {
        case 0xFF00:
        case 0xFE00:
        case 0xFC00:
        case 0x7F00:
        case 0x7E00:
        case 0x3F00:
            return false;
        default:
            return true;
        }
    }

    bool have_bogon()
    {
        const uint16_t MASK = 0xFF00;

        if  (bits_ != 8) return false;

        const uint16_t test = (buffer_ & MASK);

        switch (test)
        {
        case 0xFF00:
        case 0xFE00:
        case 0x7F00:
            return true;
        default:
            return false;
        }
    }

    void go_frame()
    {
        state_ = FRAMING;
        frame_.clear();
        ones_ = 0;
        buffer_ &= 0xFF00;
        start_timer();
    }

    void hunt(char c)
    {
        const uint16_t MASK = 0xFF00;

        add_bit(c);
        buffer_ &= MASK;

        if (bits_ != 8) return;

        if (have_flag())
        {
            go_hunt();
        }
        if (have_bogon())
        {
            go_search();
        }
        else if (have_frame())
        {
            go_frame();
        }
        else
        {
            go_search();
        }
    }

    void frame(char c)
    {
        const uint16_t MASK = 0xFF00;
        const uint16_t CHECK = 0x00F8;

        add_bit(c);

        if (ones_ < 5)
        {
            ones_ = (buffer_ & 0x80) ? ones_ + 1: 0;

            if (bits_ == 16)
            {
                frame_.push_back(getchar());

                consume_byte();
                if (have_flag())
                {
                    if (frame_.size() > 17)
                    {
                        output_frame();
                    }
                    go_hunt();
                }
                else if (frame_.size() > 330)
                {
                    go_search();
                }
            }
        }
        else
        {
            // 5 ones in a row means the next one should be 0 and be skipped.

            if ((buffer_ & 0x80) == 0)
            {
                ones_ = 0;
                consume_bit();
                return;
            }
            else if (frame_end())
            {
                std::clog << "how did we get here?" << std::endl;

                output_frame();
                go_hunt();
            }
            else
            {
                // Framing error.  Drop the frame.  If there is a FLAG
                // in the buffer, go into HUNT otherwise SEARCH.

                if ((buffer_ >> (16 - bits_) & 0xFF) == 0x7E)
                {
                    // Cannot call go_hunt() here because we need
                    // to preserve buffer state.
                    bits_ -= 8;
                    state_ = HUNT;
                }
                else
                {
                    go_search();
                }
            }
        }
    }

    bool frame_end()
    {
        uint16_t tmp = (buffer_ >> (16 - bits_));
        return (tmp & 0xFF) == FLAG;
    }

    void output_frame()
    {
        try
        {
            ax25_frame frame(frame_);

            std::clog << boost::posix_time::to_simple_string(
                boost::posix_time::second_clock().local_time()) << std::endl;

            std::cout << "\07\07\07";

            write(std::cout, frame);
            ready_ = true;
        }
        catch (bad_frame& ex)
        {
            if (passall_)
            {
                std::clog << boost::posix_time::to_simple_string(
                        boost::posix_time::second_clock().local_time())
                    << std::endl;

                std::cout << "\07\07\07";
                report_frame_error();
                ready_ = true;
            }
            else
            {
                frame_.clear();
            }
        }
    }

    bool frame_abort()
    {
        uint16_t tmp = (buffer_ >> (16 - bits_));
        return (tmp & 0x7FFF) == 0x7FFF;
    }

    void abort_frame()
    {
        bits_ = 8;
        buffer_ &= 0xFF00;
        frame_.clear();
    }

    void report_frame_error()
    {
        if (frame_.size() > 17)
        {
            write(std::clog, sloppy_ax25_frame(frame_));
        }
    }

    bool ready() const
    {
        return ready_;
    }

    std::string frame()
    {
        assert(ready_);
        std::string result = frame_;
        frame_.clear();
        ready_ = false;
        return result;
    }

    bool operator()(char c)
    {
        c &= 1; // One bit only

        switch (state_)
        {
        case SEARCH:
            search(c);
            break;
        case HUNT:
            hunt(c);
            break;
        case FRAMING:
            frame(c);
            break;
        default:
            abort();
        }

        return ready();
    }
};

} // detail


class MOBILINKD_API hdlc_framer_impl : public virtual hdlc_framer
{
public:
    typedef boost::shared_ptr<hdlc_framer_impl> sptr;

    static sptr make(bool pass_all)
    {
        return sptr(new hdlc_framer_impl(pass_all));
    }

    static sptr make(bool pass_all, gr_msg_queue_sptr msgq)
    {
        return sptr(new hdlc_framer_impl(pass_all, msgq));
    }

    virtual int work(
        int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items);

    virtual gr_msg_queue_sptr msgq() const { return msgq_; }

    virtual ~hdlc_framer_impl() {}

private:

    hdlc_framer_impl(bool pass_all);
    hdlc_framer_impl(bool pass_all, gr_msg_queue_sptr msgq);

    gr_msg_queue_sptr msgq_;
    detail::hdlc_state_machine state_;
};

}} // gr::mobilinkd

#endif // GR__MOBILINKD__HDLC_FRAMER_IMPL_H_
