// Copyright 2012 Robert C. Riggs <rob@pangalactic.org>
// All rights reserved.

#include "afsk1200_demod_impl.h"

#include <gnuradio/gr_io_signature.h>
#include <gnuradio/gr_sync_block.h>

#include <gnuradio/digital_binary_slicer_fb.h>
#include <gnuradio/gr_delay.h>
#include <gnuradio/gr_xor_bb.h>
#include <gnuradio/gr_char_to_float.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/gr_dc_blocker_ff.h>
#include <gnuradio/digital_clock_recovery_mm_ff.h>
#include <gnuradio/digital_diff_decoder_bb.h>

namespace gr { namespace mobilinkd {

afsk1200_demod::sptr afsk1200_demod::make(int rate)
{
    return afsk1200_demod_impl::make(rate);
}


namespace detail {

struct invert_bit : public virtual gr_sync_block
{
    typedef boost::shared_ptr<invert_bit> sptr;

    static sptr make()
    {
        return sptr(new invert_bit);
    }

    invert_bit()
    : gr_sync_block("invert_bit",
        gr_make_io_signature(1, 1, 1),
        gr_make_io_signature(1, 1, 1))
    {}

    int work(
        int size,
        gr_vector_const_void_star& input_items,
        gr_vector_void_star& output_items)
    {
        const unsigned char* source =
            reinterpret_cast<const unsigned char*>(input_items[0]);
        unsigned char* dest =
            reinterpret_cast<unsigned char*>(output_items[0]);

        for (int i = 0; i != size; ++i)
        {
            dest[i] = source[i] ? 0 : 1;
        }

        return size;
    }
};

} // detail

afsk1200_demod_impl::afsk1200_demod_impl(int rate)
: gr_hier_block2("afsk1200_demod",
    gr_make_io_signature(1, 1, sizeof(float)),
    gr_make_io_signature(1, 1, sizeof(char)))
, rate_(rate)
{
    digital_binary_slicer_fb_sptr slicer_1 = digital_make_binary_slicer_fb();
    gr_delay_sptr delay = gr_make_delay(1, int(.000448 / (1.0 / rate)));
    gr_xor_bb_sptr gr_xor = gr_make_xor_bb();
    gr_char_to_float_sptr char_to_float = gr_make_char_to_float();
    gr::filter::fir_filter_fff::sptr filter = gr::filter::fir_filter_fff::make(
        1, gr::filter::firdes::low_pass(1, rate_, 1200, 300));
    gr_dc_blocker_ff_sptr dc_blocker = gr_make_dc_blocker_ff(1024, true);
    digital_clock_recovery_mm_ff_sptr clock_recovery =
        digital_make_clock_recovery_mm_ff(rate / 1200, .00005, .240, .01, .00005);
    digital_binary_slicer_fb_sptr slicer_2 = digital_make_binary_slicer_fb();
    detail::invert_bit::sptr inverter = detail::invert_bit::make();


    connect(self(), 0, slicer_1, 0);
    connect(slicer_1, 0, delay, 0);
    connect(slicer_1, 0, gr_xor, 0);
    connect(delay, 0, gr_xor, 1);
    connect(gr_xor, 0, char_to_float, 0);
    connect(char_to_float, 0, filter, 0);
    connect(filter, 0, dc_blocker, 0);
    connect(dc_blocker, 0, clock_recovery, 0);
    connect(clock_recovery, 0, slicer_2, 0);
    connect(slicer_2, 0, inverter, 0);
    connect(inverter, 0, self(), 0);
}


afsk1200_demod_impl::~afsk1200_demod_impl()
{}


}} // gr::mobilinkd
