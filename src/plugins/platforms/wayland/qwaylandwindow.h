/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the config.tests of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWAYLANDWINDOW_H
#define QWAYLANDWINDOW_H

#include <QtGui/QPlatformWindow>
#include <QtCore/QWaitCondition>

#include "qwaylanddisplay.h"

class QWaylandDisplay;
class QWaylandBuffer;
struct wl_egl_window;

class QWaylandWindow : public QPlatformWindow
{
public:
    enum WindowType {
        Shm,
        Egl
    };

    QWaylandWindow(QWidget *window);
    ~QWaylandWindow();

    virtual WindowType windowType() const = 0;
    WId winId() const;
    void setVisible(bool visible);
    void setParent(const QPlatformWindow *parent);

    void configure(uint32_t time, uint32_t edges,
                   int32_t x, int32_t y, int32_t width, int32_t height);

    void attach(QWaylandBuffer *buffer);
    void damage(const QRect &rect);

    void waitForFrameSync();

    struct wl_surface *wl_surface() const { return mSurface; }

protected:
    struct wl_surface *mSurface;
    virtual void newSurfaceCreated();
    QWaylandDisplay *mDisplay;
    QWaylandBuffer *mBuffer;
    WId mWindowId;
    bool mWaitingForFrameSync;
    QWaitCondition mFrameSyncWait;

private:
    static void frameCallback(struct wl_surface *surface, void *data, uint32_t time);


};


#endif // QWAYLANDWINDOW_H
