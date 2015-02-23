/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhaikuscreen.h"

#include "qhaikucursor.h"
#include "qhaikuutils.h"

#include <qpa/qwindowsysteminterface.h>

#include <Bitmap.h>
#include <Screen.h>
#include <Window.h>

QHaikuScreen::QHaikuScreen()
    : m_screen(new BScreen(B_MAIN_SCREEN_ID))
    , m_cursor(new QHaikuCursor)
{
    Q_ASSERT(m_screen->IsValid());
}

QHaikuScreen::~QHaikuScreen()
{
    delete m_cursor;
    m_cursor = Q_NULLPTR;

    delete m_screen;
    m_screen = Q_NULLPTR;
}

QPixmap QHaikuScreen::grabWindow(WId winId, int x, int y, int width, int height) const
{
    if (width == 0 || height == 0)
        return QPixmap();

    BScreen screen(Q_NULLPTR);
    BBitmap *bitmap = Q_NULLPTR;
    screen.GetBitmap(&bitmap);

    const BRect frame = (winId ? ((BWindow*)winId)->Frame() : screen.Frame());

    const int absoluteX = frame.left + x;
    const int absoluteY = frame.top + y;

    if (width < 0)
        width = frame.Width() - x;
    if (height < 0)
        height = frame.Height() - y;

    const QImage::Format format = QHaikuUtils::colorSpaceToImageFormat(bitmap->ColorSpace());

    // get image of complete screen
    QImage image((uchar*)bitmap->Bits(), screen.Frame().Width() + 1, screen.Frame().Height() + 1, bitmap->BytesPerRow(), format);

    // extract the area of the requested window
    QRect grabRect(absoluteX, absoluteY, width, height);
    image = image.copy(grabRect);

    delete bitmap;

    return QPixmap::fromImage(image);
}

QRect QHaikuScreen::geometry() const
{
    const BRect frame = m_screen->Frame();
    return QRect(frame.left, frame.top, frame.right - frame.left, frame.bottom - frame.top);
}

int QHaikuScreen::depth() const
{
    switch (format()) {
    case QImage::Format_Invalid:
        return 0;
        break;
    case QImage::Format_MonoLSB:
        return 1;
        break;
    case QImage::Format_Indexed8:
        return 8;
        break;
    case QImage::Format_RGB16:
    case QImage::Format_RGB555:
        return 16;
        break;
    case QImage::Format_RGB888:
        return 24;
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    default:
        return 32;
        break;
    }
}

QImage::Format QHaikuScreen::format() const
{
    return QHaikuUtils::colorSpaceToImageFormat(m_screen->ColorSpace());
}

QPlatformCursor *QHaikuScreen::cursor() const
{
    return m_cursor;
}

QT_END_NAMESPACE
