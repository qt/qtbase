// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
