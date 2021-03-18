/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qhaikuutils.h"

color_space QHaikuUtils::imageFormatToColorSpace(QImage::Format format)
{
    color_space colorSpace = B_NO_COLOR_SPACE;
    switch (format) {
    case QImage::Format_Invalid:
        colorSpace = B_NO_COLOR_SPACE;
        break;
    case QImage::Format_MonoLSB:
        colorSpace = B_GRAY1;
        break;
    case QImage::Format_Indexed8:
        colorSpace = B_CMAP8;
        break;
    case QImage::Format_RGB32:
        colorSpace = B_RGB32;
        break;
    case QImage::Format_ARGB32:
        colorSpace = B_RGBA32;
        break;
    case QImage::Format_RGB16:
        colorSpace = B_RGB16;
        break;
    case QImage::Format_RGB555:
        colorSpace = B_RGB15;
        break;
    case QImage::Format_RGB888:
        colorSpace = B_RGB24;
        break;
    default:
        qWarning("Cannot convert image format %d to color space", format);
        Q_ASSERT(false);
        break;
    }

    return colorSpace;
}

QImage::Format QHaikuUtils::colorSpaceToImageFormat(color_space colorSpace)
{
    QImage::Format format = QImage::Format_Invalid;
    switch (colorSpace) {
    case B_NO_COLOR_SPACE:
        format = QImage::Format_Invalid;
        break;
    case B_GRAY1:
        format = QImage::Format_MonoLSB;
        break;
    case B_CMAP8:
        format = QImage::Format_Indexed8;
        break;
    case B_RGB32:
        format = QImage::Format_RGB32;
        break;
    case B_RGBA32:
        format = QImage::Format_ARGB32;
        break;
    case B_RGB16:
        format = QImage::Format_RGB16;
        break;
    case B_RGB15:
        format = QImage::Format_RGB555;
        break;
    case B_RGB24:
        format = QImage::Format_RGB888;
        break;
    default:
        qWarning("Cannot convert color space %d to image format", colorSpace);
        Q_ASSERT(false);
        break;
    }

    return format;
}

QT_END_NAMESPACE
