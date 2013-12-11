// Copyright 2012 mobilinkd <rob@pangalactic.org>
// All rights reserved.

#ifndef GR__MOBILINKD__HDLC_FRAMER_H_
#define GR__MOBILINKD__HDLC_FRAMER_H_

#include "mobilinkd_api.h"

#include <gnuradio/gr_types.h>
#include <gnuradio/gr_sync_block.h>
#include <gnuradio/gr_msg_queue.h>
#include <gnuradio/gr_basic_block.h>

#include <boost/shared_ptr.hpp>

namespace gr { namespace mobilinkd {

class MOBILINKD_API hdlc_framer : public virtual gr_sync_block
{
public:
    typedef boost::shared_ptr<hdlc_framer> sptr;

    static sptr make(bool pass_all);
    static sptr make(bool pass_all, gr_msg_queue_sptr msgq);

    virtual int work(
        int noutput_items,
        gr_vector_const_void_star& input_items,
        gr_vector_void_star& output_items) = 0;

    virtual gr_msg_queue_sptr msgq() const = 0;

    virtual ~hdlc_framer() {}

};

}} // gr::mobilinkd

#endif // GR__MOBILINKD__HDLC_FRAMER_H_
