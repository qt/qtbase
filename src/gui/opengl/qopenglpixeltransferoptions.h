/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
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

#ifndef QOPENGLPIXELUPLOADOPTIONS_H
#define QOPENGLPIXELUPLOADOPTIONS_H

#include <QtGui/qtguiglobal.h>

#if !defined(QT_NO_OPENGL)

#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE

class QOpenGLPixelTransferOptionsData;

class Q_GUI_EXPORT QOpenGLPixelTransferOptions
{
public:
    QOpenGLPixelTransferOptions();
    QOpenGLPixelTransferOptions(const QOpenGLPixelTransferOptions &);
    QOpenGLPixelTransferOptions &operator=(QOpenGLPixelTransferOptions &&other) noexcept
    { swap(other); return *this; }
    QOpenGLPixelTransferOptions &operator=(const QOpenGLPixelTransferOptions &);
    ~QOpenGLPixelTransferOptions();

    void swap(QOpenGLPixelTransferOptions &other) noexcept
    { data.swap(other.data); }

    void setAlignment(int alignment);
    int alignment() const;

    void setSkipImages(int skipImages);
    int skipImages() const;

    void setSkipRows(int skipRows);
    int skipRows() const;

    void setSkipPixels(int skipPixels);
    int skipPixels() const;

    void setImageHeight(int imageHeight);
    int imageHeight() const;

    void setRowLength(int rowLength);
    int rowLength() const;

    void setLeastSignificantByteFirst(bool lsbFirst);
    bool isLeastSignificantBitFirst() const;

    void setSwapBytesEnabled(bool swapBytes);
    bool isSwapBytesEnabled() const;

private:
    QSharedDataPointer<QOpenGLPixelTransferOptionsData> data;
};

Q_DECLARE_SHARED(QOpenGLPixelTransferOptions)

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLPIXELUPLOADOPTIONS_H
