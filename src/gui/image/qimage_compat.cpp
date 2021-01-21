/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifdef QIMAGE_H
#  error "This file cannot be used with precompiled headers"
#endif
#define QT_COMPILING_QIMAGE_COMPAT_CPP

#include "qimage.h"

QT_BEGIN_NAMESPACE

// These implementations must be the same as the inline versions in qimage.h

QImage QImage::convertToFormat(Format f, Qt::ImageConversionFlags flags) const
{
    return convertToFormat_helper(f, flags);
}

QImage QImage::mirrored(bool horizontally, bool vertically) const
{
    return mirrored_helper(horizontally, vertically);
}

QImage QImage::rgbSwapped() const
{
    return rgbSwapped_helper();
}

QT_END_NAMESPACE
