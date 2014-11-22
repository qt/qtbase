/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpepperintegration.h"
#include "qpepperscreen.h"
#include "qpepperplatformwindow.h"
#include "qpepperfontdatabase.h"
#include "qpepperclipboard.h"
#include "qpepperbackingstore.h"
#include "qpeppereventdispatcher.h"
#include "qpeppertheme.h"
#include "qpepperservices.h"

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
    m_compositor = 0;
    m_eventTranslator = new PepperEventTranslator();
    QObject::connect(m_eventTranslator, SIGNAL(getWindowAt(QPoint,QWindow**)), this, SLOT(getWindowAt(QPoint,QWindow**)));
    QObject::connect(m_eventTranslator, SIGNAL(getKeyWindow(QWindow**)), this, SLOT(getKeyWindow(QWindow**)));

    m_pepperEventDispatcher = 0;
    m_topLevelWindow = 0;
    m_fontDatabase = 0;
    m_clipboard = 0;
    m_services = 0;
}

QPepperIntegration::~QPepperIntegration()
{
    globalPepperIntegration = 0;
    delete m_compositor;
    delete m_eventTranslator;
    delete m_fontDatabase;
    delete m_pepperEventDispatcher;
}

bool QPepperIntegration::hasOpenGL() const
{
    return true;
}

QPlatformWindow *QPepperIntegration::createPlatformWindow(QWindow *window) const
{
    QPepperPlatformWindow *platformWindow = new QPepperPlatformWindow(window);
    if (m_topLevelWindow == 0)
        m_topLevelWindow = platformWindow;

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

QPlatformClipboard *QPepperIntegration::clipboard() const
{
//  WIP: disabled.
//    if (m_clipboard == 0)
//        m_clipboard = new QPepperClipboard();
//    return m_clipboard;
    return QPlatformIntegration::clipboard();
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

// called on QPepperInstance::Init, pepper startup has now completed
void QPepperIntegration::setPepperInstance(QPepperInstance *instance)
{
    m_pepperInstance = instance;
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
    return (m_topLevelWindow->window()->surfaceType() == QSurface::OpenGLSurface);
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

void QPepperIntegration::handleMessage(const QByteArray &tag, const QString &message)
{
    Q_UNUSED(tag)
    Q_UNUSED(message)
}
