/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qimagescale_p.h"
#include "qimage.h"
#include <private/qsimd_p.h>

#if defined(QT_COMPILER_SUPPORTS_SSE4_1)

QT_BEGIN_NAMESPACE

using namespace QImageScale;

inline static __m128i qt_qimageScaleAARGBA_helper(const unsigned int *pix, int xyap, int Cxy, int step, const __m128i vxyap, const __m128i vCxy)
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
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    const __m128i v256 = _mm_set1_epi32(256);

    /* go through every scanline in the output buffer */
    for (int y = 0; y < dh; y++) {
        int Cy = yapoints[y] >> 16;
        int yap = yapoints[y] & 0xffff;
        const __m128i vCy = _mm_set1_epi32(Cy);
        const __m128i vyap = _mm_set1_epi32(yap);

        unsigned int *dptr = dest + (y * dow);
        for (int x = 0; x < dw; x++) {
            const unsigned int *sptr = ypoints[y] + xpoints[x];
            __m128i vx = qt_qimageScaleAARGBA_helper(sptr, yap, Cy, sow, vyap, vCy);

            int xap = xapoints[x];
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
            vx = _mm_packus_epi32(vx, _mm_setzero_si128());
            vx = _mm_packus_epi16(vx, _mm_setzero_si128());
            *dptr = _mm_cvtsi128_si32(vx);
            if (RGB)
                *dptr |= 0xff000000;
            dptr++;
        }
    }
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
    for (int y = 0; y < dh; y++) {
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
            vx = _mm_packus_epi32(vx, _mm_setzero_si128());
            vx = _mm_packus_epi16(vx, _mm_setzero_si128());
            *dptr = _mm_cvtsi128_si32(vx);
            if (RGB)
                *dptr |= 0xff000000;
            dptr++;
        }
    }
}

template<bool RGB>
void qt_qimageScaleAARGBA_down_xy_sse4(QImageScaleInfo *isi, unsigned int *dest,
                                       int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    for (int y = 0; y < dh; y++) {
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
