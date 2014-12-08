/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperintegration.h"

#include "qpepperbackingstore.h"
#include "qpepperclipboard.h"
#include "qpeppercompositor.h"
#include "qpeppereventdispatcher.h"
#include "qpepperfontdatabase.h"
#include "qpepperglcontext.h"
#include "qpepperinstance_p.h"
#include "qpepperscreen.h"
#include "qpepperservices.h"
#include "qpeppertheme.h"
#include "qpepperwindow.h"

#include <QtCore/qdebug.h>
#include <QtGui/QSurface>
#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>

void *QPepperPlatformNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    if (resource == "Instance")
        return QPepperInstancePrivate::getPPInstance();
    return 0;
}

QPlatformIntegration *qt_create_pepper_integration()
{
    return QPepperIntegration::create();
}

QPepperIntegration *QPepperIntegration::create()
{
    if (QPepperInstancePrivate::get() == 0) {
        qFatal("ERROR: QPepperInstance is not created. Use Q_GUI_MAIN instead of main().");
        return 0;
    }
    return new QPepperIntegration();
}

static QPepperIntegration *globalPepperIntegration;
QPepperIntegration *QPepperIntegration::get() { return globalPepperIntegration; }

QPepperIntegration::QPepperIntegration()
    : m_clipboard(0)
    , m_eventDispatcher(0)
    , m_fontDatabase(0)
    , m_services(0)
    , m_topLevelWindow(0)
    , m_compositor(0)
    , m_eventTranslator(0)
    , m_screen(0)
    , m_platformNativeInterface(0)
{
    globalPepperIntegration = this;

    m_screen = new QPepperScreen();
    screenAdded(m_screen);

    m_eventTranslator = new QPepperEventTranslator();
    QObject::connect(m_eventTranslator, SIGNAL(getWindowAt(QPoint, QWindow **)), this,
                     SLOT(getWindowAt(QPoint, QWindow **)));
    QObject::connect(m_eventTranslator, SIGNAL(getKeyWindow(QWindow **)), this,
                     SLOT(getKeyWindow(QWindow **)));
}

QPepperIntegration::~QPepperIntegration()
{
    globalPepperIntegration = 0;
    delete m_platformNativeInterface;
    delete m_compositor;
    delete m_eventTranslator;
    delete m_fontDatabase;
}

QPlatformWindow *QPepperIntegration::createPlatformWindow(QWindow *window) const
{
    QPepperWindow *platformWindow = new QPepperWindow(window);
    if (m_topLevelWindow == 0)
        m_topLevelWindow = platformWindow;

    return platformWindow;
}

QPlatformBackingStore *QPepperIntegration::createPlatformBackingStore(QWindow *window) const
{
    QPepperBackingStore *backingStore = new QPepperBackingStore(window);
    return backingStore;
}

QPlatformOpenGLContext *
QPepperIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QPepperGLContext *glContext = new QPepperGLContext();
    return glContext;
}

QAbstractEventDispatcher *QPepperIntegration::createEventDispatcher() const
{
    m_eventDispatcher = new QPepperEventDispatcher();
    return m_eventDispatcher;
}

QPlatformFontDatabase *QPepperIntegration::fontDatabase() const
{
    if (m_fontDatabase == 0)
        m_fontDatabase = new QPepperFontDatabase();

    return m_fontDatabase;
}

QPlatformClipboard *QPepperIntegration::clipboard() const
{
    //  WIP: disabled.
    //    if (m_clipboard == 0)
    //        m_clipboard = new QPepperClipboard();
    //    return m_clipboard;
    return QPlatformIntegration::clipboard();
}

QPlatformNativeInterface *QPepperIntegration::nativeInterface() const
{
    if (m_platformNativeInterface == 0)
        m_platformNativeInterface = new QPepperPlatformNativeInterface();

    return m_platformNativeInterface;
}

QPlatformServices *QPepperIntegration::services() const
{
    if (m_services == 0)
        m_services = new QPepperServices();
    return m_services;
}

QVariant QPepperIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case ShowIsFullScreen:
        return true;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

Qt::WindowState QPepperIntegration::defaultWindowState(Qt::WindowFlags) const
{
    return Qt::WindowFullScreen;
}

QStringList QPepperIntegration::themeNames() const
{
    return QStringList() << QStringLiteral("pepper");
}

QPlatformTheme *QPepperIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QStringLiteral("pepper"))
        return new QPepperTheme;

    return 0;
}

QPepperCompositor *QPepperIntegration::pepperCompositor() const { return m_compositor; }

QPepperEventTranslator *QPepperIntegration::pepperEventTranslator() const
{
    return m_eventTranslator;
}

void QPepperIntegration::processEvents() { m_eventDispatcher->processEvents(); }

void QPepperIntegration::resizeScreen(QSize size, qreal devicePixelRatio)
{
    // Set the frame buffer on the compositor
    if (m_compositor)
        m_compositor->beginResize(size, devicePixelRatio);

    // Send the screen geometry change to Qt, resize windows.
    QRect screenRect(QPoint(0, 0), size);
    m_screen->resizeMaximizedWindows();
    QWindowSystemInterface::handleScreenGeometryChange(m_screen->screen(),
                                                       screenRect,  // new geometry
                                                       screenRect); // new available geometry
    QWindowSystemInterface::flushWindowSystemEvents();

    // Let Qt process the resize events;
    if (m_eventDispatcher)
        m_eventDispatcher->processEvents();

    // End resize and composit.
    if (m_compositor)
        m_compositor->endResize();
}

QPepperWindow *QPepperIntegration::topLevelWindow() const { return m_topLevelWindow; }

void QPepperIntegration::getWindowAt(const QPoint &point, QWindow **window)
{
    if (m_compositor)
        *window = m_compositor->windowAt(point);
    else if (m_topLevelWindow)
        *window = m_topLevelWindow->window();
    else
        *window = 0;
}

void QPepperIntegration::getKeyWindow(QWindow **window)
{
    if (m_compositor)
        *window = m_compositor->keyWindow();
    else if (m_topLevelWindow)
        *window = m_topLevelWindow->window();
    else
        *window = 0;
}
