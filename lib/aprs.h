// Copyright 2012 Robert C. Riggs <rob@pangalactic.org>
// All rights reserved.


#ifndef GR__MOBILINKD__APRS_H_
#define GR__MOBILINKD__APRS_H_

#include "ax25_frame.h"

#include <boost/tuple/tuple.hpp>
#include <boost/any.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <string>
#include <list>
#include <stdexcept>

#include <ctime>

namespace gr { namespace mobilinkd { namespace aprs {

struct parse_error : std::runtime_error
{
    parse_error(const std::string& msg)
    : std::runtime_error(msg)
    {}
};

long fromBase91(const std::string& in)
{
    long base = 1;
    long result = 0;
    for (size_t i = 0; i != in.size(); i++) base *= 91;
    for (size_t i = 0; i != in.size(); i++)
    {
        result += (in[i] - 33) * base;
        base /= 91;
    }
    return result;
}

std::string toBase91(long in)
{
    long base = 91;
    std::string result;
    while (in > base * 91) base *= 91;
    while (base != 1)
    {
        long tmp = (in / base) + 33;
        result += char(tmp);
        base /= 91;
    }
    return result;
}

template <typename Derived, typename ValueType, int ID>
struct base_type
{
    typedef Derived type;
    typedef ValueType value_type;

    int type_id() const
    {
        return ID;
    }

    std::string name() const
    {
        return static_cast<Derived*>(this)->getName();
    }

    value_type value() const
    {
        return static_cast<Derived*>(this)->getValue();
    }
};

// Location types are in the range (1, 999)
struct LatLong : public base_type<LatLong, boost::tuple<double, double>, 1>
{
    std::string name_;
    value_type value_;

    LatLong(const std::string& name, const value_type& value)
    : name_(name), value_(value)
    {}

    std::string getName() const { return name_; }
    value_type getValue() const { return value_; }

};


struct Maidenhead : public base_type<Maidenhead, std::string, 2>
{
    std::string name_;
    value_type value_;

    Maidenhead(const std::string& name, const value_type& value)
    : name_(name), value_(value)
    {}

    std::string getName() const { return name_; }
    value_type getValue() const { return value_; }
};

// Date/time parameters are in the range (1001, 1999)

struct LocalTimestamp
    : public base_type<LocalTimestamp, boost::posix_time::time_duration, 2001>
{
    std::string name_;
    value_type value_;

    LocalTimestamp(const std::string& name, const std::string& value)
    : name_(name), value_()
    {
        using boost::posix_time::duration_from_string;
        std::ostringstream os;
        os << value.substr(0, 2) << ":" << value.substr(2, 2) << ":";
        if (value.size() == 6)
            os << value.substr(4, 2);
        else
            os << "00";
        value_ = duration_from_string(os.str());
    }

    LocalTimestamp(const std::string& name, const value_type& value)
    : name_(name), value_(value)
    {}

    std::string getName() const { return name_; }
    value_type getValue() const { return value_; }
};

    struct UtcTimestamp
        : public base_type<UtcTimestamp, boost::posix_time::time_duration, 2001>
    {
        std::string name_;
        value_type value_;

        UtcTimestamp(const std::string& name, const std::string& value)
        : name_(name), value_()
        {
            using boost::posix_time::duration_from_string;
            std::ostringstream os;
            os << value.substr(0, 2) << ":" << value.substr(2, 2) << ":";
            if (value.size() == 6)
                os << value.substr(4, 2);
            else
                os << "00";
            value_ = duration_from_string(os.str());
        }

        UtcTimestamp(const std::string& name, const value_type& value)
        : name_(name), value_(value)
        {}

        std::string getName() const { return name_; }
        value_type getValue() const { return value_; }
    };

// Radio parameters are in the range (2001, 2999)

struct Frequency : public base_type<Frequency, uint64_t, 2001>
{
    std::string name_;
    value_type value_;

    Frequency(const std::string& name, const value_type& value)
    : name_(name), value_(value)
    {}

    std::string getName() const { return name_; }
    value_type getValue() const { return value_; }
};

struct CTCSS : public base_type<CTCSS, double, 2002>
{
    std::string name_;
    value_type value_;

    CTCSS(const std::string& name, const value_type& value)
    : name_(name), value_(value)
    {}

    std::string getName() const { return name_; }
    value_type getValue() const { return value_; }
};

struct DCS : public base_type<DCS, uint16_t, 1003>
{
    std::string name_;
    value_type value_;

    DCS(const std::string& name, const value_type& value)
    : name_(name), value_(value)
    {}

    std::string getName() const { return name_; }
    value_type getValue() const { return value_; }
};


// Text fields are in the range (10001, 19999)

struct Comment : public base_type<Comment, std::string, 10001>
{
    std::string name_;
    value_type value_;

    Comment(const std::string& name, const value_type& value)
    : name_(name), value_(value)
    {}

    std::string getName() const { return name_; }
    value_type getValue() const { return value_; }
};


// Object types are in the range (20001, 29999)

struct Symbol : public base_type<Symbol, std::string, 10001>
{
    std::string name_;
    value_type value_;

    Symbol(const std::string& name, const value_type& value)
    : name_(name), value_(value)
    {}

    std::string getName() const { return name_; }
    value_type getValue() const { return value_; }
};

struct aprs
{
    typedef std::list<boost::any> content_type;

    content_type contents_;
    bool message_capable_;

    static std::string::size_type getCurrentMicE(
        const std::string& info, std::string::size_type pos,
        content_type& result)
    {
        return info.size();
    }

    static std::string::size_type getOldMicE(
        const std::string& info, std::string::size_type pos,
        content_type& result)
    {
        return info.size();
    }

    static std::string::size_type getPositionWithoutTimestamp(
        const std::string& info, std::string::size_type pos,
        content_type& result)
    {
        return info.size();
    }

    static std::string::size_type getObject(
        const std::string& info, std::string::size_type pos,
        content_type& result)
    {
        return info.size();
    }

    static std::string::size_type getPositionWithTimestamp(
        const std::string& info, std::string::size_type pos,
        content_type& result)
    {
        if (info.size() - pos < 24) throw parse_error("bad position");

        if (info[pos + 6] == 'h')
            result.push_back(UtcTimestamp("time", info.substr(pos, 6)));
        else if (info[pos + 6] == 'z')
            result.push_back(UtcTimestamp("time", info.substr(pos + 2, 4)));
        else
            result.push_back(LocalTimestamp("time", info.substr(pos + 2, 4)));

        pos += 7;

        // std::string symbol =

        return info.size();
    }

    static std::string::size_type getRawGPS(
        const std::string& info, std::string::size_type pos,
        content_type& result)
    {
        return info.size();
    }


    static std::string::size_type parse(
        const std::string& info, std::string::size_type pos,
        content_type& result)
    {
        switch(info[pos++])
        {
        case 0x1c:
        case '`':
            return getCurrentMicE(info, pos, result);
        case 0x1d:
        case '\'':
            return getOldMicE(info, pos, result);   // Probably current MicE.
        case '!':
        case '=':
            return getPositionWithoutTimestamp(info, pos, result);
        case ';':
            return getObject(info, pos, result);
        case '@':
        case '/':
            return getPositionWithTimestamp(info, pos, result);
        case '$':
            return getRawGPS(info, pos, result);
        }
    }

    static content_type parse(const ax25_frame& frame)
    {
        content_type result;

        std::string info = frame.info();
        std::string::size_type pos = 0;

        while (pos < info.size() and info[pos] != 0x0D)
        {
            content_type datum;
            pos = parse(info, pos, datum);
            result.push_back(datum);
        }

        return result;
    }

    static bool isMessageCapable(const ax25_frame& frame)
    {
        return false;
    }

    aprs(const ax25_frame& frame)
    : contents_(parse(frame)), message_capable_(isMessageCapable(frame))
    {}

};

}}} /// gr::mobilinkd::aprs

#endif // GR__MOBILINKD__APRS_H_
