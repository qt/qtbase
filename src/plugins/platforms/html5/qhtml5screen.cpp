/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhtml5screen.h"
#include "qhtml5window.h"
#include "qhtml5compositor.h"

#include <QtEglSupport/private/qeglconvenience_p.h>
#ifndef QT_NO_OPENGL
# include <QtEglSupport/private/qeglplatformcontext_p.h>
#endif
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <private/qhighdpiscaling_p.h>

#ifdef Q_OPENKODE
#include <KD/kd.h>
#include <KD/NV_initialize.h>
#endif //Q_OPENKODE

QT_BEGIN_NAMESPACE

QHTML5Screen::QHTML5Screen(QHtml5Compositor *compositor)
    : mCompositor(compositor)
    , m_depth(32)
    , m_format(QImage::Format_RGB32)
{
    mCompositor->setScreen(this);
}

QHTML5Screen::~QHTML5Screen()
{

}

QRect QHTML5Screen::geometry() const
{
    return m_geometry;
}

int QHTML5Screen::depth() const
{
    return m_depth;
}

QImage::Format QHTML5Screen::format() const
{
    return m_format;
}

QPlatformCursor *QHTML5Screen::cursor() const
{
    return const_cast<QHtml5Cursor *>(&m_cursor);
}

void QHTML5Screen::resizeMaximizedWindows()
{
    QList<QWindow*> windows = QGuiApplication::allWindows();
    // 'screen()' still has the old geometry info while 'this' has the new geometry info
    const QRect oldGeometry = screen()->geometry();
    const QRect oldAvailableGeometry = screen()->availableGeometry();

    const QRect newGeometry = deviceIndependentGeometry();
    const QRect newAvailableGeometry = QHighDpi::fromNative(availableGeometry(), QHighDpiScaling::factor(this), newGeometry.topLeft());

    // make sure maximized and fullscreen windows are updated
    for (int i = 0; i < windows.size(); ++i) {
        QWindow *w = windows.at(i);

        // Skip non-platform windows, e.g., offscreen windows.
        if (!w->handle())
            continue;

        if (platformScreenForWindow(w) != this)
            continue;

        if (w->windowState() & Qt::WindowMaximized || w->geometry() == oldAvailableGeometry)
            w->setGeometry(newAvailableGeometry);

        else if (w->windowState() & Qt::WindowFullScreen || w->geometry() == oldGeometry)
            w->setGeometry(newGeometry);
    }
}

QWindow *QHTML5Screen::topWindow() const
{
    return mCompositor->keyWindow();
}

QWindow *QHTML5Screen::topLevelAt(const QPoint & p) const
{
    return mCompositor->windowAt(p);
}

void QHTML5Screen::invalidateSize()
{
    m_geometry = QRect();
}

void QHTML5Screen::setGeometry(const QRect &rect)
{
    m_geometry = rect;
    QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry(), availableGeometry());
    resizeMaximizedWindows();
}

QT_END_NAMESPACE
