/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qimagescale_p.h"
#include "qimage.h"
#include <private/qdrawhelper_x86_p.h>
#include <private/qsimd_p.h>

#if QT_CONFIG(thread) && !defined(Q_OS_WASM)
#include "qsemaphore.h"
#include "qthreadpool.h"
#endif

#if defined(QT_COMPILER_SUPPORTS_SSE4_1)

QT_BEGIN_NAMESPACE

using namespace QImageScale;

template<typename T>
static inline void multithread_pixels_function(QImageScaleInfo *isi, int dh, const T &scaleSection)
{
#if QT_CONFIG(thread) && !defined(Q_OS_WASM)
    int segments = (qsizetype(isi->sh) * isi->sw) / (1<<16);
    segments = std::min(segments, dh);
    QThreadPool *threadPool = QThreadPool::globalInstance();
    if (segments > 1 && threadPool && !threadPool->contains(QThread::currentThread())) {
        QSemaphore semaphore;
        int y = 0;
        for (int i = 0; i < segments; ++i) {
            int yn = (dh - y) / (segments - i);
            threadPool->start([&, y, yn]() {
                scaleSection(y, y + yn);
                semaphore.release(1);
            });
            y += yn;
        }
        semaphore.acquire(segments);
        return;
    }
#endif
    scaleSection(0, dh);
}

inline static __m128i Q_DECL_VECTORCALL
qt_qimageScaleAARGBA_helper(const unsigned int *pix, int xyap, int Cxy, int step, const __m128i vxyap, const __m128i vCxy)
{
    __m128i vpix = _mm_cvtepu8_epi32(_mm_cvtsi32_si128(*pix));
    __m128i vx = _mm_mullo_epi32(vpix, vxyap);
    int i;
    for (i = (1 << 14) - xyap; i > Cxy; i -= Cxy) {
        pix += step;
        vpix = _mm_cvtepu8_epi32(_mm_cvtsi32_si128(*pix));
        vx = _mm_add_epi32(vx, _mm_mullo_epi32(vpix, vCxy));
    }
    pix += step;
    vpix = _mm_cvtepu8_epi32(_mm_cvtsi32_si128(*pix));
    vx = _mm_add_epi32(vx, _mm_mullo_epi32(vpix, _mm_set1_epi32(i)));
    return vx;
}

template<bool RGB>
void qt_qimageScaleAARGBA_up_x_down_y_sse4(QImageScaleInfo *isi, unsigned int *dest,
                                           int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    const int *xpoints = isi->xpoints;
    const int *xapoints = isi->xapoints;
    const int *yapoints = isi->yapoints;

    const __m128i v256 = _mm_set1_epi32(256);

    /* go through every scanline in the output buffer */
    auto scaleSection = [&] (int yStart, int yEnd) {
        for (int y = yStart; y < yEnd; ++y) {
            const int Cy = yapoints[y] >> 16;
            const int yap = yapoints[y] & 0xffff;
            const __m128i vCy = _mm_set1_epi32(Cy);
            const __m128i vyap = _mm_set1_epi32(yap);

            unsigned int *dptr = dest + (y * dow);
            for (int x = 0; x < dw; x++) {
                const unsigned int *sptr = ypoints[y] + xpoints[x];
                __m128i vx = qt_qimageScaleAARGBA_helper(sptr, yap, Cy, sow, vyap, vCy);

                const int xap = xapoints[x];
                if (xap > 0) {
                    const __m128i vxap = _mm_set1_epi32(xap);
                    const __m128i vinvxap = _mm_sub_epi32(v256, vxap);
                    __m128i vr = qt_qimageScaleAARGBA_helper(sptr + 1, yap, Cy, sow, vyap, vCy);

                    vx = _mm_mullo_epi32(vx, vinvxap);
                    vr = _mm_mullo_epi32(vr, vxap);
                    vx = _mm_add_epi32(vx, vr);
                    vx = _mm_srli_epi32(vx, 8);
                }
                vx = _mm_srli_epi32(vx, 14);
                vx = _mm_packus_epi32(vx, vx);
                vx = _mm_packus_epi16(vx, vx);
                *dptr = _mm_cvtsi128_si32(vx);
                if (RGB)
                    *dptr |= 0xff000000;
                dptr++;
            }
        }
    };
    multithread_pixels_function(isi, dh, scaleSection);
}

template<bool RGB>
void qt_qimageScaleAARGBA_down_x_up_y_sse4(QImageScaleInfo *isi, unsigned int *dest,
                                           int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    const __m128i v256 = _mm_set1_epi32(256);

    /* go through every scanline in the output buffer */
    auto scaleSection = [&] (int yStart, int yEnd) {
        for (int y = yStart; y < yEnd; ++y) {
            unsigned int *dptr = dest + (y * dow);
            for (int x = 0; x < dw; x++) {
                int Cx = xapoints[x] >> 16;
                int xap = xapoints[x] & 0xffff;
                const __m128i vCx = _mm_set1_epi32(Cx);
                const __m128i vxap = _mm_set1_epi32(xap);

                const unsigned int *sptr = ypoints[y] + xpoints[x];
                __m128i vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1, vxap, vCx);

                int yap = yapoints[y];
                if (yap > 0) {
                    const __m128i vyap = _mm_set1_epi32(yap);
                    const __m128i vinvyap = _mm_sub_epi32(v256, vyap);
                    __m128i vr = qt_qimageScaleAARGBA_helper(sptr + sow, xap, Cx, 1, vxap, vCx);

                    vx = _mm_mullo_epi32(vx, vinvyap);
                    vr = _mm_mullo_epi32(vr, vyap);
                    vx = _mm_add_epi32(vx, vr);
                    vx = _mm_srli_epi32(vx, 8);
                }
                vx = _mm_srli_epi32(vx, 14);
                vx = _mm_packus_epi32(vx, vx);
                vx = _mm_packus_epi16(vx, vx);
                *dptr = _mm_cvtsi128_si32(vx);
                if (RGB)
                    *dptr |= 0xff000000;
                dptr++;
            }
        }
    };
    multithread_pixels_function(isi, dh, scaleSection);
}

template<bool RGB>
void qt_qimageScaleAARGBA_down_xy_sse4(QImageScaleInfo *isi, unsigned int *dest,
                                       int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    auto scaleSection = [&] (int yStart, int yEnd) {
        for (int y = yStart; y < yEnd; ++y) {
            int Cy = yapoints[y] >> 16;
            int yap = yapoints[y] & 0xffff;
            const __m128i vCy = _mm_set1_epi32(Cy);
            const __m128i vyap = _mm_set1_epi32(yap);

            unsigned int *dptr = dest + (y * dow);
            for (int x = 0; x < dw; x++) {
                const int Cx = xapoints[x] >> 16;
                const int xap = xapoints[x] & 0xffff;
                const __m128i vCx = _mm_set1_epi32(Cx);
                const __m128i vxap = _mm_set1_epi32(xap);

                const unsigned int *sptr = ypoints[y] + xpoints[x];
                __m128i vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1, vxap, vCx);
                __m128i vr = _mm_mullo_epi32(_mm_srli_epi32(vx, 4), vyap);

                int j;
                for (j = (1 << 14) - yap; j > Cy; j -= Cy) {
                    sptr += sow;
                    vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1, vxap, vCx);
                    vr = _mm_add_epi32(vr, _mm_mullo_epi32(_mm_srli_epi32(vx, 4), vCy));
                }
                sptr += sow;
                vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1, vxap, vCx);
                vr = _mm_add_epi32(vr, _mm_mullo_epi32(_mm_srli_epi32(vx, 4), _mm_set1_epi32(j)));

                vr = _mm_srli_epi32(vr, 24);
                vr = _mm_packus_epi32(vr, _mm_setzero_si128());
                vr = _mm_packus_epi16(vr, _mm_setzero_si128());
                *dptr = _mm_cvtsi128_si32(vr);
                if (RGB)
                    *dptr |= 0xff000000;
                dptr++;
            }
        }
    };
    multithread_pixels_function(isi, dh, scaleSection);
}

template void qt_qimageScaleAARGBA_up_x_down_y_sse4<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                           int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_up_x_down_y_sse4<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                          int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_x_up_y_sse4<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                           int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_x_up_y_sse4<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                          int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_xy_sse4<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                       int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_xy_sse4<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                      int dw, int dh, int dow, int sow);

QT_END_NAMESPACE

#endif
