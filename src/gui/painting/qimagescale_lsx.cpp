// Copyright (C) 2024 Loongson Technology Corporation Limited.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qimagescale_p.h"
#include "qimage.h"
#include <private/qdrawhelper_loongarch64_p.h>
#include <private/qsimd_p.h>

#if QT_CONFIG(qtgui_threadpool)
#include <qsemaphore.h>
#include <private/qguiapplication_p.h>
#include <private/qthreadpool_p.h>
#endif

#if defined(QT_COMPILER_SUPPORTS_LSX)

QT_BEGIN_NAMESPACE

using namespace QImageScale;

template<typename T>
static inline void multithread_pixels_function(QImageScaleInfo *isi, int dh, const T &scaleSection)
{
#if QT_CONFIG(qtgui_threadpool)
    int segments = (qsizetype(isi->sh) * isi->sw) / (1<<16);
    segments = std::min(segments, dh);
    QThreadPool *threadPool = QGuiApplicationPrivate::qtGuiThreadPool();
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
#else
    Q_UNUSED(isi);
#endif
    scaleSection(0, dh);
}

inline static __m128i Q_DECL_VECTORCALL
qt_qimageScaleAARGBA_helper(const unsigned int *pix, int xyap, int Cxy,
                            int step, const __m128i vxyap, const __m128i vCxy)
{
    const __m128i shuffleMask = (__m128i)(v16i8){0, 16, 16, 16, 1, 16, 16, 16,
                                                 2, 16, 16, 16, 3, 16, 16, 16};
    __m128i vpix = __lsx_vshuf_b(__lsx_vldi(0), __lsx_vreplgr2vr_w(*pix), shuffleMask);
    __m128i vx = __lsx_vmul_w(vpix, vxyap);
    int i;
    for (i = (1 << 14) - xyap; i > Cxy; i -= Cxy) {
        pix += step;
        vpix = __lsx_vshuf_b(__lsx_vldi(0), __lsx_vreplgr2vr_w(*pix), shuffleMask);
        vx = __lsx_vadd_w(vx, __lsx_vmul_w(vpix, vCxy));
    }
    pix += step;
    vpix = __lsx_vshuf_b(__lsx_vldi(0), __lsx_vreplgr2vr_w(*pix), shuffleMask);
    vx = __lsx_vadd_w(vx, __lsx_vmul_w(vpix, __lsx_vreplgr2vr_w(i)));
    return vx;
}

template<bool RGB>
void qt_qimageScaleAARGBA_up_x_down_y_lsx(QImageScaleInfo *isi, unsigned int *dest,
                                          int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    const int *xpoints = isi->xpoints;
    const int *xapoints = isi->xapoints;
    const int *yapoints = isi->yapoints;

    const __m128i v256 = __lsx_vreplgr2vr_w(256);

    /* go through every scanline in the output buffer */
    auto scaleSection = [&] (int yStart, int yEnd) {
        for (int y = yStart; y < yEnd; ++y) {
            const int Cy = yapoints[y] >> 16;
            const int yap = yapoints[y] & 0xffff;
            const __m128i vCy = __lsx_vreplgr2vr_w(Cy);
            const __m128i vyap = __lsx_vreplgr2vr_w(yap);

            unsigned int *dptr = dest + (y * dow);
            for (int x = 0; x < dw; x++) {
                const unsigned int *sptr = ypoints[y] + xpoints[x];
                __m128i vx = qt_qimageScaleAARGBA_helper(sptr, yap, Cy, sow, vyap, vCy);

                const int xap = xapoints[x];
                if (xap > 0) {
                    const __m128i vxap = __lsx_vreplgr2vr_w(xap);
                    const __m128i vinvxap = __lsx_vsub_w(v256, vxap);
                    __m128i vr = qt_qimageScaleAARGBA_helper(sptr + 1, yap, Cy, sow, vyap, vCy);

                    vx = __lsx_vmul_w(vx, vinvxap);
                    vr = __lsx_vmul_w(vr, vxap);
                    vx = __lsx_vadd_w(vx, vr);
                    vx = __lsx_vsrli_w(vx, 8);
                }
                vx = __lsx_vsrli_w(vx, 14);
                vx = __lsx_vpickev_h(__lsx_vsat_wu(vx, 15), __lsx_vsat_wu(vx, 15));
                vx = __lsx_vpickev_b(__lsx_vsat_hu(vx, 7), __lsx_vsat_hu(vx, 7));
                *dptr = __lsx_vpickve2gr_w(vx, 0);
                if (RGB)
                    *dptr |= 0xff000000;
                dptr++;
            }
        }
    };
    multithread_pixels_function(isi, dh, scaleSection);
}

template<bool RGB>
void qt_qimageScaleAARGBA_down_x_up_y_lsx(QImageScaleInfo *isi, unsigned int *dest,
                                          int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    const __m128i v256 = __lsx_vreplgr2vr_w(256);

    /* go through every scanline in the output buffer */
    auto scaleSection = [&] (int yStart, int yEnd) {
        for (int y = yStart; y < yEnd; ++y) {
            unsigned int *dptr = dest + (y * dow);
            for (int x = 0; x < dw; x++) {
                int Cx = xapoints[x] >> 16;
                int xap = xapoints[x] & 0xffff;
                const __m128i vCx = __lsx_vreplgr2vr_w(Cx);
                const __m128i vxap = __lsx_vreplgr2vr_w(xap);

                const unsigned int *sptr = ypoints[y] + xpoints[x];
                __m128i vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1, vxap, vCx);

                int yap = yapoints[y];
                if (yap > 0) {
                    const __m128i vyap = __lsx_vreplgr2vr_w(yap);
                    const __m128i vinvyap = __lsx_vsub_w(v256, vyap);
                    __m128i vr = qt_qimageScaleAARGBA_helper(sptr + sow, xap, Cx, 1, vxap, vCx);

                    vx = __lsx_vmul_w(vx, vinvyap);
                    vr = __lsx_vmul_w(vr, vyap);
                    vx = __lsx_vadd_w(vx, vr);
                    vx = __lsx_vsrli_w(vx, 8);
                }
                vx = __lsx_vsrli_w(vx, 14);
                vx = __lsx_vpickev_h(__lsx_vsat_wu(vx, 15), __lsx_vsat_wu(vx, 15));
                vx = __lsx_vpickev_b(__lsx_vsat_wu(vx, 7), __lsx_vsat_hu(vx, 7));
                *dptr = __lsx_vpickve2gr_w(vx, 0);
                if (RGB)
                    *dptr |= 0xff000000;
                dptr++;
            }
        }
    };
    multithread_pixels_function(isi, dh, scaleSection);
}

template<bool RGB>
void qt_qimageScaleAARGBA_down_xy_lsx(QImageScaleInfo *isi, unsigned int *dest,
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
            const __m128i vCy = __lsx_vreplgr2vr_w(Cy);
            const __m128i vyap = __lsx_vreplgr2vr_w(yap);

            unsigned int *dptr = dest + (y * dow);
            for (int x = 0; x < dw; x++) {
                const int Cx = xapoints[x] >> 16;
                const int xap = xapoints[x] & 0xffff;
                const __m128i vCx = __lsx_vreplgr2vr_w(Cx);
                const __m128i vxap = __lsx_vreplgr2vr_w(xap);

                const unsigned int *sptr = ypoints[y] + xpoints[x];
                __m128i vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1, vxap, vCx);
                __m128i vr = __lsx_vmul_w(__lsx_vsrli_w(vx, 4), vyap);

                int j;
                for (j = (1 << 14) - yap; j > Cy; j -= Cy) {
                    sptr += sow;
                    vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1, vxap, vCx);
                    vr = __lsx_vadd_w(vr, __lsx_vmul_w(__lsx_vsrli_w(vx, 4), vCy));
                }
                sptr += sow;
                vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1, vxap, vCx);
                vr = __lsx_vadd_w(vr, __lsx_vmul_w(__lsx_vsrli_w(vx, 4), __lsx_vreplgr2vr_w(j)));

                vr = __lsx_vsrli_w(vr, 24);
                vr = __lsx_vpickev_h(__lsx_vldi(0), __lsx_vsat_wu(vr, 15));
                vr = __lsx_vpickev_b(__lsx_vldi(0), __lsx_vsat_hu(vr, 7));
                *dptr = __lsx_vpickve2gr_w(vr, 0);
                if (RGB)
                    *dptr |= 0xff000000;
                dptr++;
            }
        }
    };
    multithread_pixels_function(isi, dh, scaleSection);
}

template void qt_qimageScaleAARGBA_up_x_down_y_lsx<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                          int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_up_x_down_y_lsx<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                         int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_x_up_y_lsx<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                          int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_x_up_y_lsx<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                         int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_xy_lsx<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                      int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_xy_lsx<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                     int dw, int dh, int dow, int sow);

QT_END_NAMESPACE

#endif
