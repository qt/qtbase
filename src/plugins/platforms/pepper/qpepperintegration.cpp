/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperintegration.h"
#include "qpepperscreen.h"
#include "qpepperplatformwindow.h"
#include "qpepperfontdatabase.h"
#include "qpepperbackingstore.h"
#include "qpeppereventdispatcher.h"
#include "qpeppertheme.h"
#include "qpepperjavascriptbridge.h"

#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtGui/QSurface>
#include <qdebug.h>

QPlatformIntegration *qt_create_pepper_integration()
{
    return QPepperIntegration::createPepperIntegration();
}

QPepperIntegration * QPepperIntegration::createPepperIntegration()
{
    return new QPepperIntegration();
}

static QPepperIntegration *globalPepperIntegration;
QPepperIntegration *QPepperIntegration::getPepperIntegration()
{
    return globalPepperIntegration;
}

QPepperIntegration::QPepperIntegration()
{
    globalPepperIntegration = this;

    m_screen = new QPepperScreen();
    screenAdded(m_screen);

    m_pepperInstance = 0;
    m_compositor = new QPepperCompositor();
    m_eventTranslator = new PepperEventTranslator();
    QObject::connect(m_eventTranslator, SIGNAL(getWindowAt(QPoint,QWindow**)), this, SLOT(getWindowAt(QPoint,QWindow**)));
    QObject::connect(m_eventTranslator, SIGNAL(getKeyWindow(QWindow**)), this, SLOT(getKeyWindow(QWindow**)));

    m_pepperEventDispatcher = 0;
    m_javascriptBridge = 0;
    m_fontDatabase = 0;
}

QPepperIntegration::~QPepperIntegration()
{
    globalPepperIntegration = 0;
    delete m_compositor;
    delete m_eventTranslator;
    delete m_fontDatabase;
    delete m_pepperEventDispatcher;
    delete m_javascriptBridge;
}

bool QPepperIntegration::hasOpenGL() const
{
    return true;
}

#ifndef Q_OS_NACL
QPlatformOpenGLContext *QPepperIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QPepperGLContext();
}
#endif

QPlatformWindow *QPepperIntegration::createPlatformWindow(QWindow *window) const
{
    QPepperPlatformWindow *platformWindow = new QPepperPlatformWindow(window);
    useOpenglToplevel = (window->surfaceType() == QSurface::OpenGLSurface);
    return platformWindow;
}

QPlatformBackingStore *QPepperIntegration::createPlatformBackingStore(QWindow *window) const
{
    QPepperBackingStore *backingStore = new QPepperBackingStore(window);
    return backingStore;
}

QPlatformOpenGLContext *QPepperIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QPepperGLContext *glContext = new QPepperGLContext();
    return glContext;
}

QAbstractEventDispatcher* QPepperIntegration::createEventDispatcher() const
{
    m_pepperEventDispatcher = new QPepperEventDispatcher();
    return m_pepperEventDispatcher;
}

QPlatformFontDatabase *QPepperIntegration::fontDatabase() const
{
    if (m_fontDatabase == 0)
        m_fontDatabase = new QPepperFontDatabase();

    return m_fontDatabase;
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

// called on QPepperInstance::Init, pepper startup has now completed
void QPepperIntegration::setPepperInstance(QPepperInstance *instance)
{
    m_pepperInstance = instance;

#if 0
    // Set up C++ <-> Javascript messaging.
    m_javascriptBridge = new QPepperJavascriptBridge(m_pepperInstance);
    connect(m_javascriptBridge, SIGNAL(evalFunctionReply(const QByteArray&, const QString&)),
                                SLOT(handleMessage(const QByteArray&, const QString&)));

    // Inject helper javascript into the web page:
    m_javascriptBridge->evalFile(":/qpepperplatformplugin/qpepperhelpers.js");
    m_javascriptBridge->evalFile(":/qpepperplatformplugin/qpepperfileaccess.js");
#endif
}

QPepperInstance *QPepperIntegration::pepperInstance() const
{
    return m_pepperInstance;
}

QPepperCompositor *QPepperIntegration::pepperCompositor() const
{
    return m_compositor;
}

PepperEventTranslator *QPepperIntegration::pepperEventTranslator()
{
    return m_eventTranslator;
}

void QPepperIntegration::processEvents()
{
    m_pepperEventDispatcher->processEvents();
}

bool QPepperIntegration::wantsOpenGLGraphics() const
{
    return useOpenglToplevel;
}

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
    if (m_pepperEventDispatcher)
        m_pepperEventDispatcher->processEvents();

    // End resize and composit.
    if (m_compositor)
        m_compositor->endResize();
}

void QPepperIntegration::getWindowAt(const QPoint & point, QWindow **window)
{
    *window = m_compositor->windowAt(point);
}

void QPepperIntegration::getKeyWindow(QWindow **window)
{
    *window = m_compositor->keyWindow();
}

void QPepperIntegration::handleMessage(const QByteArray &tag, const QString &message)
{
    Q_UNUSED(tag)
    Q_UNUSED(message)
}


