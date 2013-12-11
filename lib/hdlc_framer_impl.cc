// Copyright 2012 mobilinkd <rob@pangalactic.org>
// All rights reserved.

#include "hdlc_framer_impl.h"
#include "ax25_frame.h"
#include "aprs.h"

#include <gnuradio/gr_io_signature.h>

namespace gr { namespace mobilinkd {


hdlc_framer::sptr hdlc_framer::make(bool pass_all)
{
    return hdlc_framer_impl::make(pass_all);
}

hdlc_framer::sptr hdlc_framer::make(bool pass_all, gr_msg_queue_sptr msgq)
{
    return hdlc_framer_impl::make(pass_all, msgq);
}

hdlc_framer_impl::hdlc_framer_impl(bool pass_all)
: gr_sync_block("hdlc_framer",
    gr_make_io_signature(1, 1, 1),
    gr_make_io_signature(0, 0, 0))
, msgq_(gr_make_msg_queue()), state_(pass_all)
{
    std::clog << "Starting HDLC Framer" << std::endl;
    gr_message_sptr msg =
        gr_make_message_from_string("Starting HDLC Framer\n", 0, 0, 0);

    msgq_->insert_tail(msg);         // send it
}

hdlc_framer_impl::hdlc_framer_impl(bool pass_all, gr_msg_queue_sptr msgq)
: gr_sync_block("hdlc_framer",
    gr_make_io_signature(1, 1, 1),
    gr_make_io_signature(0, 0, 0))
, msgq_(msgq), state_(pass_all)
{
    std::clog << "Starting HDLC Framer" << std::endl;
    gr_message_sptr msg =
        gr_make_message_from_string("Starting HDLC Framer\n", 0, 0, 0);

    msgq_->insert_tail(msg);         // send it
}

int hdlc_framer_impl::work(
    int size,
    gr_vector_const_void_star& input_items,
    gr_vector_void_star& output_items)
{
    const unsigned char* source =
        reinterpret_cast<const unsigned char*>(input_items[0]);

    for (int i = 0; i != size; ++i)
    {
        state_(source[i]);
        if (state_.ready())
        {
            try
            {
                sloppy_ax25_frame frame(state_.frame());
                std::ostringstream output;
                write(output, frame);
                gr_message_sptr msg =
                    gr_make_message_from_string(output.str());

                msgq_->insert_tail(msg);         // send it
            }
            catch (bad_frame&)
            {}
        }
    }

    return size;
}

}} // gr::mobilinkd
