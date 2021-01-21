/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

QT_BEGIN_NAMESPACE

class QWindowsNativeImage
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
