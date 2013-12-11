// Copyright 2012 mobilinkd <rob@pangalactic.org>
// All rights reserved.


#ifndef GR__MOBILINKD__MOBILINKD_API_H_
#define GR__MOBILINKD__MOBILINKD_API_H_

#include <gruel/attributes.h>

#ifdef gnuradio_mobilinkd_EXPORTS
#  define MOBILINKD_API __GR_ATTR_EXPORT
#else
#  define MOBILINKD_API __GR_ATTR_IMPORT
#endif

#endif // GR__MOBILINKD__MOBILINKD_API_H_
