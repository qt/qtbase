// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QIMAGESCALE_P_H
#define QIMAGESCALE_P_H

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

#include <qimage.h>
#include <private/qtguiglobal_p.h>

#if QT_CONFIG(qtgui_threadpool)
#include <qthreadpool.h>
#include <private/qguiapplication_p.h>
#include <private/qlatch_p.h>
#include <private/qthreadpool_p.h>
#endif

QT_BEGIN_NAMESPACE

/*
  This version accepts only supported formats.
*/
QImage qSmoothScaleImage(const QImage &img, int w, int h);

namespace QImageScale {
    struct QImageScaleInfo {
        int *xpoints{nullptr};
        const unsigned int **ypoints{nullptr};
        int *xapoints{nullptr};
        int *yapoints{nullptr};
        int xup_yup{0};
        int sh = 0;
        int sw = 0;
    };

    template<typename T>
    void multithread_pixels_function(QImageScaleInfo *isi, int dh, const T &scaleSection)
    {
#if QT_CONFIG(qtgui_threadpool)
        int segments = (qsizetype(isi->sh) * isi->sw) / (1<<16);
        segments = std::min(segments, dh);
        QThreadPool *threadPool = QGuiApplicationPrivate::qtGuiThreadPool();
        if (segments > 1 && threadPool && !threadPool->contains(QThread::currentThread())) {
            QLatch latch(segments);
            int y = 0;
            for (int i = 0; i < segments; ++i) {
                int yn = (dh - y) / (segments - i);
                threadPool->start([&, y, yn]() {
                    scaleSection(y, y + yn);
                    latch.countDown();
                });
                y += yn;
            }
            latch.wait();
            return;
        }
#else
        Q_UNUSED(isi);
#endif
        scaleSection(0, dh);
    }
}

QT_END_NAMESPACE

#endif
