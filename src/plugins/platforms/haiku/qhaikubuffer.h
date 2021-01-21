/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
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

#ifndef QHAIKUBUFFER_H
#define QHAIKUBUFFER_H

#include <QtGui/QImage>

class BBitmap;

QT_BEGIN_NAMESPACE

class QHaikuBuffer
{
public:
    QHaikuBuffer();
    QHaikuBuffer(BBitmap *buffer);

    BBitmap* nativeBuffer() const;
    const QImage *image() const;
    QImage *image();

    QRect rect() const;

private:
    BBitmap *m_buffer;
    QImage m_image;
};

QT_END_NAMESPACE

#endif
