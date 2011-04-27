/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qvolatileimagedata_p.h"
#include <QtGui/qpaintengine.h>

QT_BEGIN_NAMESPACE

QVolatileImageData::QVolatileImageData()
    : pengine(0)
{
}

QVolatileImageData::QVolatileImageData(int w, int h, QImage::Format format)
    : pengine(0)
{
    image = QImage(w, h, format);
}

QVolatileImageData::QVolatileImageData(const QImage &sourceImage)
    : pengine(0)
{
    image = sourceImage;
}

QVolatileImageData::QVolatileImageData(void *, void *)
    : pengine(0)
{
    // Not supported.
}

QVolatileImageData::QVolatileImageData(const QVolatileImageData &other)
    : QSharedData()
{
    image = other.image;
    // The detach is not mandatory here but we do it nonetheless in order to
    // keep the behavior consistent with other platforms.
    image.detach();
    pengine = 0;
}

QVolatileImageData::~QVolatileImageData()
{
    delete pengine;
}

void QVolatileImageData::beginDataAccess() const
{
    // nothing to do here
}

void QVolatileImageData::endDataAccess(bool readOnly) const
{
    Q_UNUSED(readOnly);
    // nothing to do here
}

bool QVolatileImageData::ensureFormat(QImage::Format format)
{
    if (image.format() != format) {
        image = image.convertToFormat(format);
    }
    return true;
}

void *QVolatileImageData::duplicateNativeImage() const
{
    return 0;
}

void QVolatileImageData::ensureImage()
{
    // nothing to do here
}

QT_END_NAMESPACE
