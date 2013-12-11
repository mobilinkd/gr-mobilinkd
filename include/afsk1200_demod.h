// Copyright 2012 mobilinkd <rob@pangalactic.org>
// All rights reserved.


#ifndef GR__MOBILINKD__AFSK1200_H_
#define GR__MOBILINKD__AFSK1200_H_

#include "mobilinkd_api.h"

#include <gnuradio/gr_types.h>
#include <gnuradio/gr_hier_block2.h>

#include <boost/shared_ptr.hpp>

namespace gr { namespace mobilinkd {

class MOBILINKD_API afsk1200_demod : public virtual gr_hier_block2
{
public:
    typedef boost::shared_ptr<afsk1200_demod> sptr;

    static sptr make(int rate);
};

}} // gr::mobilinkd

#endif // GR__MOBILINKD__AFSK1200_H_
