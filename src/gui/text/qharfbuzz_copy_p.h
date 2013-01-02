/*
 * Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies)
 * Copyright (C) 2007  Red Hat, Inc.
 *
 * This code is a modified version of some part of HarfBuzz,
 * an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 */

#ifndef QHARFBUZZ_COPY_P_H
#define QHARFBUZZ_COPY_P_H

/*
  The purpose of this header file is to allow inclusion of the private
  headers for font and text classes without having to pull in the full
  harfbuzz library under QTDIR/src/3rdparty/harfbuzz/src
*/
#if defined(QT_BUILD_GUI_LIB) || defined(QT_COMPILES_IN_HARFBUZZ)
#include <private/qharfbuzz_p.h>
#else

extern "C" {

#ifdef  __xlC__
typedef unsigned hb_bitfield;
#else
typedef QT_PREPEND_NAMESPACE(quint8) hb_bitfield;
#endif

typedef enum {
  /* no error */
  HB_Err_Ok                           = 0x0000,
  HB_Err_Not_Covered                  = 0xFFFF,

  /* _hb_err() is called whenever returning the following errors,
   * and in a couple places for HB_Err_Not_Covered too. */

  /* programmer error */
  HB_Err_Invalid_Argument             = 0x1A66,

  /* font error */
  HB_Err_Invalid_SubTable_Format      = 0x157F,
  HB_Err_Invalid_SubTable             = 0x1570,
  HB_Err_Read_Error                   = 0x6EAD,

  /* system error */
  HB_Err_Out_Of_Memory                = 0xDEAD
} HB_Error;

typedef QT_PREPEND_NAMESPACE(quint32) HB_Glyph;
typedef void * HB_Font;
typedef void * HB_Face;
typedef void * HB_FontRec;
typedef QT_PREPEND_NAMESPACE(quint32) hb_uint32;
typedef QT_PREPEND_NAMESPACE(qint32) HB_Fixed;

typedef struct {
    HB_Fixed x;
    HB_Fixed y;
} HB_FixedPoint;

// The GlyphAttrbutes class is used inline so it needs to be complete.
typedef struct {
    hb_bitfield justification   :4;  /* Justification class */
    hb_bitfield clusterStart    :1;  /* First glyph of representation of cluster */
    hb_bitfield mark            :1;  /* needs to be positioned around base char */
    hb_bitfield zeroWidth       :1;  /* ZWJ, ZWNJ etc, with no width */
    hb_bitfield dontPrint       :1;
    hb_bitfield combiningClass  :8;
} HB_GlyphAttributes;

}

#endif // ifdef QT_BUILD_GUI_LIB

#endif // QHARFBUZZ_COPY_P_H
