/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QXCBSCREEN_H
#define QXCBSCREEN_H

#include <qpa/qplatformscreen.h>
#include <QtCore/QString>

#include <xcb/xcb.h>

#include "qxcbobject.h"

QT_BEGIN_NAMESPACE

class QXcbConnection;
class QXcbCursor;

class QXcbScreen : public QXcbObject, public QPlatformScreen
{
public:
    QXcbScreen(QXcbConnection *connection, xcb_screen_t *screen, int number);
    ~QXcbScreen();

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const;

    QWindow *topLevelAt(const QPoint &point) const;

    QRect geometry() const;
    int depth() const;
    QImage::Format format() const;
    QSizeF physicalSize() const;
    QPlatformCursor *cursor() const;
    qreal refreshRate() const;

    int screenNumber() const;

    xcb_screen_t *screen() const { return m_screen; }
    xcb_window_t root() const { return m_screen->root; }

    xcb_window_t clientLeader() const { return m_clientLeader; }

    QString windowManagerName() const { return m_windowManagerName; }
    bool syncRequestSupported() const { return m_syncRequestSupported; }

    const xcb_visualtype_t *visualForId(xcb_visualid_t) const;

    QString name() const;

    void updateRefreshRate();

private:
    xcb_screen_t *m_screen;
    int m_number;
    QString m_windowManagerName;
    bool m_syncRequestSupported;
    xcb_window_t m_clientLeader;
    QMap<xcb_visualid_t, xcb_visualtype_t> m_visuals;
    QXcbCursor *m_cursor;
    int m_refreshRate;
};

QT_END_NAMESPACE

#endif
