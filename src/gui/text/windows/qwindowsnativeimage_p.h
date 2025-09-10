// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSNATIVEIMAGE_H
#define QWINDOWSNATIVEIMAGE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QtGlobal>
#include <QtCore/qt_windows.h>
#include <QtGui/QImage>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QWindowsNativeImage
{
    Q_DISABLE_COPY_MOVE(QWindowsNativeImage)
public:
    QWindowsNativeImage(int width, int height,
                        QImage::Format format);

    ~QWindowsNativeImage();

    inline int width() const  { return m_image.width(); }
    inline int height() const { return m_image.height(); }

    QImage &image() { return m_image; }
    const QImage &image() const { return m_image; }

    HDC hdc() const { return m_hdc; }

    static QImage::Format systemFormat();

private:
    const HDC m_hdc;
    QImage m_image;

    HBITMAP m_bitmap = 0;
    HBITMAP m_null_bitmap = 0;
};

QT_END_NAMESPACE

#endif // QWINDOWSNATIVEIMAGE_H
