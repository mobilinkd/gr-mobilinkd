// Copyright 2012 Robert C. Riggs <rob@pangalactic.org>
// All rights reserved.

#ifndef GR__MOBILINKD__AFSK1200_DEMOD_IMPL_H_
#define GR__MOBILINKD__AFSK1200_DEMOD_IMPL_H_

#include "afsk1200_demod.h"

namespace gr { namespace mobilinkd {

class MOBILINKD_API afsk1200_demod_impl : public virtual afsk1200_demod
{
public:
    typedef boost::shared_ptr<afsk1200_demod_impl> sptr;

    static sptr make(int rate)
    {
        return sptr(new afsk1200_demod_impl(rate));
    }

    virtual ~afsk1200_demod_impl();

private:

    int rate_;

    afsk1200_demod_impl(int rate);

};

}} // gr::mobilinkd

#endif // GR__MOBILINKD__AFSK1200_DEMOD_IMPL_H_
