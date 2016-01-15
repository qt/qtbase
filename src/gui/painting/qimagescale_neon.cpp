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
#include <private/qsimd_p.h>

#if defined(__ARM_NEON__)

QT_BEGIN_NAMESPACE

using namespace QImageScale;

inline static uint32x4_t qt_qimageScaleAARGBA_helper(const unsigned int *pix, int xyap, int Cxy, int step)
{
    uint32x2_t vpix32 = vmov_n_u32(*pix);
    uint16x4_t vpix16 = vget_low_u16(vmovl_u8(vreinterpret_u8_u32(vpix32)));
    uint32x4_t vx = vmull_n_u16(vpix16, xyap);
    int i;
    for (i = (1 << 14) - xyap; i > Cxy; i -= Cxy) {
        pix += step;
        vpix32 = vmov_n_u32(*pix);
        vpix16 = vget_low_u16(vmovl_u8(vreinterpret_u8_u32(vpix32)));
        vx = vaddq_u32(vx, vmull_n_u16(vpix16, Cxy));
    }
    pix += step;
    vpix32 = vmov_n_u32(*pix);
    vpix16 = vget_low_u16(vmovl_u8(vreinterpret_u8_u32(vpix32)));
    vx = vaddq_u32(vx, vmull_n_u16(vpix16, i));
    return vx;
}

template<bool RGB>
void qt_qimageScaleAARGBA_up_x_down_y_neon(QImageScaleInfo *isi, unsigned int *dest,
                                           int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    /* go through every scanline in the output buffer */
    for (int y = 0; y < dh; y++) {
        int Cy = yapoints[y] >> 16;
        int yap = yapoints[y] & 0xffff;

        unsigned int *dptr = dest + (y * dow);
        for (int x = 0; x < dw; x++) {
            const unsigned int *sptr = ypoints[y] + xpoints[x];
            uint32x4_t vx = qt_qimageScaleAARGBA_helper(sptr, yap, Cy, sow);

            int xap = xapoints[x];
            if (xap > 0) {
                uint32x4_t vr = qt_qimageScaleAARGBA_helper(sptr + 1, yap, Cy, sow);

                vx = vmulq_n_u32(vx, 256 - xap);
                vr = vmulq_n_u32(vr, xap);
                vx = vaddq_u32(vx, vr);
                vx = vshrq_n_u32(vx, 8);
            }
            vx = vshrq_n_u32(vx, 14);
            const uint16x4_t vx16 = vmovn_u32(vx);
            const uint8x8_t vx8 = vmovn_u16(vcombine_u16(vx16, vx16));
            *dptr = vget_lane_u32(vreinterpret_u32_u8(vx8), 0);
            if (RGB)
                *dptr |= 0xff000000;
            dptr++;
        }
    }
}

template<bool RGB>
void qt_qimageScaleAARGBA_down_x_up_y_neon(QImageScaleInfo *isi, unsigned int *dest,
                                           int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    /* go through every scanline in the output buffer */
    for (int y = 0; y < dh; y++) {
        unsigned int *dptr = dest + (y * dow);
        for (int x = 0; x < dw; x++) {
            int Cx = xapoints[x] >> 16;
            int xap = xapoints[x] & 0xffff;

            const unsigned int *sptr = ypoints[y] + xpoints[x];
            uint32x4_t vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1);

            int yap = yapoints[y];
            if (yap > 0) {
                uint32x4_t vr = qt_qimageScaleAARGBA_helper(sptr + sow, xap, Cx, 1);

                vx = vmulq_n_u32(vx, 256 - yap);
                vr = vmulq_n_u32(vr, yap);
                vx = vaddq_u32(vx, vr);
                vx = vshrq_n_u32(vx, 8);
            }
            vx = vshrq_n_u32(vx, 14);
            const uint16x4_t vx16 = vmovn_u32(vx);
            const uint8x8_t vx8 = vmovn_u16(vcombine_u16(vx16, vx16));
            *dptr = vget_lane_u32(vreinterpret_u32_u8(vx8), 0);
            if (RGB)
                *dptr |= 0xff000000;
            dptr++;
        }
    }
}

template<bool RGB>
void qt_qimageScaleAARGBA_down_xy_neon(QImageScaleInfo *isi, unsigned int *dest,
                                       int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    for (int y = 0; y < dh; y++) {
        int Cy = yapoints[y] >> 16;
        int yap = yapoints[y] & 0xffff;

        unsigned int *dptr = dest + (y * dow);
        for (int x = 0; x < dw; x++) {
            const int Cx = xapoints[x] >> 16;
            const int xap = xapoints[x] & 0xffff;

            const unsigned int *sptr = ypoints[y] + xpoints[x];
            uint32x4_t vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1);
            vx = vshrq_n_u32(vx, 4);
            uint32x4_t vr = vmulq_n_u32(vx, yap);

            int j;
            for (j = (1 << 14) - yap; j > Cy; j -= Cy) {
                sptr += sow;
                vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1);
                vx = vshrq_n_u32(vx, 4);
                vx = vmulq_n_u32(vx, Cy);
                vr = vaddq_u32(vr, vx);
            }
            sptr += sow;
            vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1);
            vx = vshrq_n_u32(vx, 4);
            vx = vmulq_n_u32(vx, j);
            vr = vaddq_u32(vr, vx);

            vx = vshrq_n_u32(vr, 24);
            const uint16x4_t vx16 = vmovn_u32(vx);
            const uint8x8_t vx8 = vmovn_u16(vcombine_u16(vx16, vx16));
            *dptr = vget_lane_u32(vreinterpret_u32_u8(vx8), 0);
            if (RGB)
                *dptr |= 0xff000000;
            dptr++;
        }
    }
}

template void qt_qimageScaleAARGBA_up_x_down_y_neon<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                           int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_up_x_down_y_neon<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                          int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_x_up_y_neon<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                           int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_x_up_y_neon<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                          int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_xy_neon<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                       int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_xy_neon<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                      int dw, int dh, int dow, int sow);

QT_END_NAMESPACE

#endif
