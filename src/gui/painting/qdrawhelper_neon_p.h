/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QDRAWHELPER_NEON_P_H
#define QDRAWHELPER_NEON_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qdrawhelper_p.h>

QT_BEGIN_NAMESPACE

#ifdef __ARM_NEON__

void qt_blend_argb32_on_argb32_neon(uchar *destPixels, int dbpl,
                                            const uchar *srcPixels, int sbpl,
                                            int w, int h,
                                            int const_alpha);

void qt_blend_rgb32_on_rgb32_neon(uchar *destPixels, int dbpl,
                                  const uchar *srcPixels, int sbpl,
                                  int w, int h,
                                  int const_alpha);

void qt_blend_argb32_on_rgb16_neon(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha);

void qt_blend_argb32_on_argb32_scanline_neon(uint *dest,
                                             const uint *src,
                                             int length,
                                             uint const_alpha);

void qt_blend_rgb16_on_argb32_neon(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha);

void qt_blend_rgb16_on_rgb16_neon(uchar *destPixels, int dbpl,
                                  const uchar *srcPixels, int sbpl,
                                  int w, int h,
                                  int const_alpha);

void qt_alphamapblit_quint16_neon(QRasterBuffer *rasterBuffer,
                                  int x, int y, const QRgba64 &color,
                                  const uchar *bitmap,
                                  int mapWidth, int mapHeight, int mapStride,
                                  const QClipData *clip, bool /*useGammaCorrection*/);

void qt_scale_image_argb32_on_rgb16_neon(uchar *destPixels, int dbpl,
                                         const uchar *srcPixels, int sbpl, int srch,
                                         const QRectF &targetRect,
                                         const QRectF &sourceRect,
                                         const QRect &clip,
                                         int const_alpha);

void qt_scale_image_rgb16_on_rgb16_neon(uchar *destPixels, int dbpl,
                                        const uchar *srcPixels, int sbpl, int srch,
                                        const QRectF &targetRect,
                                        const QRectF &sourceRect,
                                        const QRect &clip,
                                        int const_alpha);

void qt_transform_image_argb32_on_rgb16_neon(uchar *destPixels, int dbpl,
                                             const uchar *srcPixels, int sbpl,
                                             const QRectF &targetRect,
                                             const QRectF &sourceRect,
                                             const QRect &clip,
                                             const QTransform &targetRectTransform,
                                             int const_alpha);

void qt_transform_image_rgb16_on_rgb16_neon(uchar *destPixels, int dbpl,
                                            const uchar *srcPixels, int sbpl,
                                            const QRectF &targetRect,
                                            const QRectF &sourceRect,
                                            const QRect &clip,
                                            const QTransform &targetRectTransform,
                                            int const_alpha);

void qt_memfill32_neon(quint32 *dest, quint32 value, qsizetype count);
void qt_memrotate90_16_neon(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl);
void qt_memrotate270_16_neon(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl);

uint * QT_FASTCALL qt_destFetchRGB16_neon(uint *buffer,
                                          QRasterBuffer *rasterBuffer,
                                          int x, int y, int length);

void QT_FASTCALL qt_destStoreRGB16_neon(QRasterBuffer *rasterBuffer,
                                        int x, int y, const uint *buffer, int length);

void QT_FASTCALL comp_func_solid_SourceOver_neon(uint *destPixels, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_Plus_neon(uint *dst, const uint *src, int length, uint const_alpha);

const uint * QT_FASTCALL qt_fetchUntransformed_888_neon(uint *buffer, const Operator *, const QSpanData *data,
                                                       int y, int x, int length);

#endif // __ARM_NEON__

QT_END_NAMESPACE

#endif // QDRAWHELPER_NEON_P_H
