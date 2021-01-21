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

#ifndef QDRAWHELPER_X86_P_H
#define QDRAWHELPER_X86_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <private/qdrawhelper_p.h>

QT_BEGIN_NAMESPACE

#ifdef __SSE2__
void qt_memfill64_sse2(quint64 *dest, quint64 value, qsizetype count);
void qt_memfill32_sse2(quint32 *dest, quint32 value, qsizetype count);
void qt_bitmapblit32_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                          const QRgba64 &color,
                          const uchar *src, int width, int height, int stride);
void qt_bitmapblit8888_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                            const QRgba64 &color,
                            const uchar *src, int width, int height, int stride);
void qt_bitmapblit16_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                          const QRgba64 &color,
                          const uchar *src, int width, int height, int stride);
void qt_blend_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha);
void qt_blend_rgb32_on_rgb32_sse2(uchar *destPixels, int dbpl,
                                 const uchar *srcPixels, int sbpl,
                                 int w, int h,
                                 int const_alpha);

void qt_memfill64_avx2(quint64 *dest, quint64 value, qsizetype count);
void qt_memfill32_avx2(quint32 *dest, quint32 value, qsizetype count);
#endif // __SSE2__

static const int numCompositionFunctions = 38;

QT_END_NAMESPACE

#endif // QDRAWHELPER_X86_P_H
