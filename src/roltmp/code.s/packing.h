/**
*   Copyright (c) 2017.  Jefferson Lab (JLab). All rights reserved. Permission
*   to use, copy, modify, and distribute  this software and its documentation for
*   educational, research, and not-for-profit purposes, without fee and without a
*   signed licensing agreement.
*
*   Author : G.Gavalian
*   Date   : 02/24/2018
*/

#ifndef __ADC_PACKING__
#define __ADC_PACKING__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

  int pack16(const char *raw, int position, const char *encoded, int epos);
  int pack(const int nsamples, const char *raw, const char *encoded);
#ifdef __cplusplus
}
#endif

#endif
