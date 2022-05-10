// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    m_cursor = nullptr;

    delete m_screen;
    m_screen = nullptr;
}

QPixmap QHaikuScreen::grabWindow(WId winId, int x, int y, int width, int height) const
{
    if (width == 0 || height == 0)
        return QPixmap();

    BScreen screen(nullptr);
    BBitmap *bitmap = nullptr;
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
