// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QHAIKUSCREEN_H
#define QHAIKUSCREEN_H

#include <qpa/qplatformscreen.h>

class BScreen;
class QHaikuCursor;

QT_BEGIN_NAMESPACE

class QHaikuScreen : public QPlatformScreen
{
public:
    QHaikuScreen();
    ~QHaikuScreen();

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const override;

    QRect geometry() const override;
    int depth() const override;
    QImage::Format format() const override;

    QPlatformCursor *cursor() const override;

private:
    BScreen *m_screen;

    QHaikuCursor *m_cursor;
};

QT_END_NAMESPACE

#endif
