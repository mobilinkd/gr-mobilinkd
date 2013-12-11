/* -*- c++ -*- */

#define MOBILINKD_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "mobilinkd_swig_doc.i"

%{
#include "afsk1200_demod.h"
#include "hdlc_framer.h"
%}

%include "afsk1200_demod.h"
GR_SWIG_BLOCK_MAGIC2(mobilinkd, afsk1200_demod);
%include "hdlc_framer.h"
GR_SWIG_BLOCK_MAGIC2(mobilinkd, hdlc_framer);
