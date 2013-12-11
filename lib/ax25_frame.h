// Copyright 2012 Robert C. Riggs <rob@pangalactic.org>
// All rights reserved.

#ifndef GR__MOBILINKD__AX25_FRAME_H_
#define GR__MOBILINKD__AX25_FRAME_H_

#include <boost/scoped_ptr.hpp>
#include <boost/crc.hpp>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <cctype>

namespace gr { namespace mobilinkd {

struct bad_frame : std::runtime_error
{
    bad_frame(const std::string& msg)
    : std::runtime_error(msg)
    {}
};

template <bool STRICT>
struct basic_ax25_frame
{
    typedef std::vector<std::string> repeaters_type;
    typedef boost::optional<uint8_t> pid_type;
    enum frame_type {UNDEFINED, INFORMATION, SUPERVISORY, UNNUMBERED};

private:

    static const std::string::size_type DEST_ADDRESS_POS = 0;
    static const std::string::size_type SRC_ADDRESS_POS = 7;
    static const std::string::size_type LAST_ADDRESS_POS = 13;
    static const std::string::size_type FIRST_REPEATER_POS = 14;
    static const std::string::size_type ADDRESS_LENGTH = 7;

    std::string destination_;
    std::string source_;
    repeaters_type repeaters_;
    frame_type type_;
    uint8_t raw_type_;
    std::string info_;
    uint16_t fcs_;
    uint16_t crc_;
    pid_type pid_;

    static std::string removeAddressExtensionBit(const std::string& address)
    {
        std::string result = address;
        for (size_t i = 0; i != result.size(); i++)
        {
            result[i] = (uint8_t(result[i]) >> 1);
        }
        return result;
    }

    static int getSSID(const std::string& address)
    {
        assert(address.size() == ADDRESS_LENGTH);

        return (address[6] & 0x0F);
    }

    static std::string cleanBadAddress(const std::string& address)
    {
        std::string result = address;
        // Clean up address field.
        for (size_t i = 0; i != result.size(); i++)
        {
            if (!std::isprint(result[i]))
            {
                result[i] = '?';
            }
        }
        return result;
    }

    static std::string appendSSID(const std::string& address, int ssid)
    {
        std::string result = address;

        if (ssid)
        {
            result += '-';
            result += boost::lexical_cast<std::string>(ssid);
        }
        return result;
    }

    static bool fixup_address(std::string& address)
    {
        assert(address.size() == ADDRESS_LENGTH);

        bool result = (address[ADDRESS_LENGTH - 1] & 1) == 0;

        address = removeAddressExtensionBit(address);

        const int ssid = getSSID(address);

        // Remove trailing spaces and SSID.
        size_t pos = address.find_first_of(' ');
        if (pos == std::string::npos) pos = 6;
        address.erase(pos);

        if (!STRICT)
        {
            address = cleanBadAddress(address);
        }

        address = appendSSID(address, ssid);

        return result;
    }

    static frame_type parse_type(const std::string& frame, size_t pos)
    {
        uint8_t c(frame[pos]);
        switch (c & 0x03)
        {
        case 0:
            return INFORMATION;
        case 1:
            return SUPERVISORY;
        case 2:
            return INFORMATION;
        case 3:
            return UNNUMBERED;
        }
    }

    static std::string parse_info(const std::string& frame, size_t pos)
    {
        std::ostringstream output;
        for (int i = pos; i != frame.size() - 2; i++)
        {
            char c = frame[i];
            if (std::isprint(c))
            {
                output << c;
            }
            else
            {
                output << "0x" << std::setw(2)
                    << std::setbase(16) << int(uint8_t(c)) << ' ';
            }
        }
        return output.str();
    }

    static uint16_t parse_fcs(const std::string& frame)
    {
        size_t checksum_pos = frame.size() - 2;

        uint16_t tmp =
            ((uint8_t(frame[checksum_pos + 1]) << 8) |
                uint8_t(frame[checksum_pos]));

        uint16_t checksum = 0;
        for (size_t i = 1; i != 0x10000; i <<= 1)
        {
            checksum <<= 1;
            checksum |= ((tmp & i) ? 1 : 0);
        }

        return checksum;
    }

    static uint16_t compute_crc(const std::string& frame)
    {
        assert(frame.size() > 2);

        const size_t payload_len = frame.size() - 2;

        // Not exactly CRC16-CCITT because of final complement.
        boost::crc_optimal<16, 0x1021, 0xFFFF, 0xFFFF, true, false> crc;

        crc.process_bytes(frame.data(), payload_len);

        return crc.checksum();
    }

    static std::string parse_destination(const std::string& frame)
    {
        assert(frame.size() > DEST_ADDRESS_POS + ADDRESS_LENGTH);
        return frame.substr(DEST_ADDRESS_POS, ADDRESS_LENGTH);
    }

    static std::string parse_source(const std::string& frame)
    {
        assert(frame.size() > SRC_ADDRESS_POS + ADDRESS_LENGTH);
        return frame.substr(SRC_ADDRESS_POS, ADDRESS_LENGTH);
    }

    static repeaters_type parse_repeaters(const std::string& frame)
    {
        assert(frame[LAST_ADDRESS_POS] & 1);

        repeaters_type result;
        std::string::size_type index = FIRST_REPEATER_POS;
        bool more = (index + ADDRESS_LENGTH) < frame.length();

        while (more)
        {
            std::string repeater = frame.substr(index, ADDRESS_LENGTH);
            index += ADDRESS_LENGTH;
            more = fixup_address(repeater)
                and (index + ADDRESS_LENGTH) < frame.length();
            result.push_back(repeater);
        }

        return result;
    }

    void parse(const std::string& frame)
    {
        if (frame.length() < 17) return;

        fcs_ = parse_fcs(frame);
        crc_ = compute_crc(frame);

        if (STRICT and (fcs_ != crc_)) throw bad_frame("crc mismatch");

        destination_ = parse_destination(frame);
        fixup_address(destination_);

        source_ = parse_source(frame);
        bool have_repeaters = fixup_address(source_);

        if (have_repeaters)
        {
            repeaters_ = parse_repeaters(frame);
        }

        size_t index = ADDRESS_LENGTH * (repeaters_.size() + 2);

        if (frame.length() < index + 5) return;

        type_ = parse_type(frame, index);
        raw_type_ = uint8_t(frame[index++]);

        if (type_ == UNNUMBERED) pid_ = uint8_t(frame[index++]);

        info_.assign(frame.begin() + index, frame.end() - 2);
    }

public:

    basic_ax25_frame(const std::string& frame)
    : destination_()
    , source_()
    , repeaters_()
    , type_(UNDEFINED)
    , info_()
    , fcs_(-1), crc_(0)
    , pid_()
    {
        parse(frame);
    }

    std::string destination() const { return destination_; }

    std::string source() const { return source_; }

    repeaters_type repeaters() const { return repeaters_; }

    frame_type type() const { return type_; }

    std::string info() const { return info_; }

    uint16_t fcs() const { return fcs_; }

    uint16_t crc() const { return crc_; }

    pid_type pid() const { return pid_; }
};

typedef basic_ax25_frame<true> strict_ax25_frame;
typedef basic_ax25_frame<false> sloppy_ax25_frame;
typedef strict_ax25_frame ax25_frame;

template <bool STRICT>
void write(std::ostream& os, const basic_ax25_frame<STRICT>& frame)
{
    typedef typename basic_ax25_frame<STRICT>::repeaters_type repeaters_type;

    os << "Dest: " << frame.destination() << std::endl
        << "Source: " << frame.source() << std::endl;

    repeaters_type repeaters = frame.repeaters();
    if (!repeaters.empty())
    {
        os << "Via: ";
        std::copy(
            repeaters.begin(), repeaters.end(),
            std::ostream_iterator<std::string>(os, " "));
        os << std::endl;
    }

    os << "PID: " << std::setbase(16) << frame.pid() << std::endl;
    os << "Info: " << std::endl << frame.info() << std::endl;
    os << "FCS: " << frame.fcs() << std::endl;
    os << "CRC: " << frame.crc() << std::endl;
}

}} // gr::mobilinkd

#endif // GR__MOBILINKD__AX25_FRAME_H_
