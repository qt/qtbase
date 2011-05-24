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

#include "qopenkodewindow.h"
#include "qopenkodeintegration.h"
#include "../eglconvenience/qeglplatformcontext.h"
#include "../eglconvenience/qeglconvenience.h"

#include <KD/kd.h>
#include <KD/NV_display.h>
#include <KD/kdplatform.h>
#ifdef KD_ATX_keyboard
#include "openkodekeytranslator.h"
#endif

#include <EGL/egl.h>

#include <QtGui/qwidget.h>
#include <QtGui/private/qwidget_p.h>
#include <QtGui/private/qapplication_p.h>

#include <QtCore/qvector.h>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

void kdProcessMouseEvents( const KDEvent *event )
{
    QOpenKODEWindow *window = static_cast<QOpenKODEWindow *>(event->userptr);
    window->processMouseEvents(event);
}

#ifdef KD_ATX_keyboard
void kdProcessKeyEvents( const KDEvent *event )
{
    QOpenKODEWindow *window = static_cast<QOpenKODEWindow *>(event->userptr);
    window->processKeyEvents(event);
}
#endif //KD_ATX_keyboard

QOpenKODEWindow::QOpenKODEWindow(QWidget *tlw)
    : QPlatformWindow(tlw), isFullScreen(false)
{
    if (tlw->platformWindowFormat().windowApi() == QPlatformWindowFormat::OpenVG) {
        m_eglApi = EGL_OPENVG_API;
    } else {
        m_eglContextAttrs.append(EGL_CONTEXT_CLIENT_VERSION);
        m_eglContextAttrs.append(2);

        m_eglApi = EGL_OPENGL_ES_API;
    }
    eglBindAPI(m_eglApi);

    m_eglContextAttrs.append(EGL_NONE);
    m_eglWindowAttrs.append(EGL_NONE);

    QList<QPlatformScreen *> screens = QApplicationPrivate::platformIntegration()->screens();
    //XXXX: jl figure out how to pick the correct screen.
//    Q_ASSERT(screens.size() > tlw->d_func()->screenNumber);
//    QOpenKODEScreen *screen = qobject_cast<QOpenKODEScreen *>(screens.at(tlw->d_func()->screenNumber));
    QOpenKODEScreen *screen = qobject_cast<QOpenKODEScreen *>(screens.at(0));
    if (!screen) {
        qErrnoWarning("Could not make QOpenKODEWindow without a screen");
    }

    QPlatformWindowFormat format = tlw->platformWindowFormat();
    format.setRedBufferSize(5);
    format.setGreenBufferSize(6);
    format.setBlueBufferSize(5);

    m_eglConfig = q_configFromQPlatformWindowFormat(screen->eglDisplay(),format);

    m_kdWindow = kdCreateWindow(screen->eglDisplay(),
                              m_eglConfig,
                              this);
    kdInstallCallback(kdProcessMouseEvents,KD_EVENT_INPUT_POINTER,this);
#ifdef KD_ATX_keyboard
    kdInstallCallback(kdProcessKeyEvents, KD_EVENT_INPUT_KEY_ATX,this);
#endif //KD_ATX_keyboard

    if (!m_kdWindow) {
        qErrnoWarning(kdGetError(), "Error creating native window");
        return;
    }

    KDboolean exclusive(false);
    if (kdSetWindowPropertybv(m_kdWindow,KD_WINDOWPROPERTY_DESKTOP_EXCLUSIVE_NV, &exclusive)) {
        isFullScreen = true;
    }

    if (isFullScreen) {
        tlw->setGeometry(screen->geometry());
        screen->setFullScreen(isFullScreen);
    }else {
        const KDint windowSize[2]  = { tlw->width(), tlw->height() };
        if (kdSetWindowPropertyiv(m_kdWindow, KD_WINDOWPROPERTY_SIZE, windowSize)) {
            qErrnoWarning(kdGetError(), "Could not set native window size");
        }
        KDboolean visibillity(false);
        if (kdSetWindowPropertybv(m_kdWindow, KD_WINDOWPROPERTY_VISIBILITY, &visibillity)) {
            qErrnoWarning(kdGetError(), "Could not set visibillity to false");
        }

        const KDint windowPos[2] = { tlw->x(), tlw->y() };
        if (kdSetWindowPropertyiv(m_kdWindow, KD_WINDOWPROPERTY_DESKTOP_OFFSET_NV, windowPos)) {
            qErrnoWarning(kdGetError(), "Could not set native window position");
            return;
        }
    }


    QOpenKODEIntegration *integration = static_cast<QOpenKODEIntegration *>(QApplicationPrivate::platformIntegration());

    if (!isFullScreen || (isFullScreen && !integration->mainGLContext())) {
        if (kdRealizeWindow(m_kdWindow, &m_eglWindow)) {
            qErrnoWarning(kdGetError(), "Could not realize native window");
            return;
        }

        EGLSurface surface = eglCreateWindowSurface(screen->eglDisplay(),m_eglConfig,m_eglWindow,m_eglWindowAttrs.constData());
        m_platformGlContext = new QEGLPlatformContext(screen->eglDisplay(), m_eglConfig,
                                                      m_eglContextAttrs.data(), surface, m_eglApi);
        integration->setMainGLContext(m_platformGLContext);
    } else {
        m_platformGlContext = integration->mainGLContext();
        kdDestroyWindow(m_kdWindow);
        m_kdWindow = 0;
    }
}


QOpenKODEWindow::~QOpenKODEWindow()
{
    if (m_platformGlContext != static_cast<QOpenKODEIntegration *>(QApplicationPrivate::platformIntegration())) {
        delete m_platformGlContext;
    }
    if (m_kdWindow)
        kdDestroyWindow(m_kdWindow);
}
void QOpenKODEWindow::setGeometry(const QRect &rect)
{
    if (isFullScreen) {
        QList<QPlatformScreen *> screens = QApplicationPrivate::platformIntegration()->screens();
        QOpenKODEScreen *screen = qobject_cast<QOpenKODEScreen *>(screens.at(0));
        widget()->setGeometry(screen->geometry());
        return;
    }
    bool needToDeleteContext = false;
    if (!isFullScreen) {
        const QRect geo = geometry();
        if (geo.size() != rect.size()) {
            const KDint windowSize[2]  = { rect.width(), rect.height() };
            if (kdSetWindowPropertyiv(m_kdWindow, KD_WINDOWPROPERTY_SIZE, windowSize)) {
                qErrnoWarning(kdGetError(), "Could not set native window size");
                //return;
            } else {
                needToDeleteContext = true;
            }
        }

        if (geo.topLeft() != rect.topLeft()) {
            const KDint windowPos[2] = { rect.x(), rect.y() };
            if (kdSetWindowPropertyiv(m_kdWindow, KD_WINDOWPROPERTY_DESKTOP_OFFSET_NV, windowPos)) {
                qErrnoWarning(kdGetError(), "Could not set native window position");
                //return;
            } else {
                needToDeleteContext = true;
            }
        }
    }

    //need to recreate context
    if (needToDeleteContext) {
        delete m_platformGlContext;

        QList<QPlatformScreen *> screens = QApplicationPrivate::platformIntegration()->screens();
        QOpenKODEScreen *screen = qobject_cast<QOpenKODEScreen *>(screens.at(0));
        EGLSurface surface = eglCreateWindowSurface(screen->eglDisplay(),m_eglConfig,m_eglWindow,m_eglWindowAttrs.constData());
        m_platformGlContext = new QEGLPlatformContext(screen->eglDisplay(),m_eglConfig,
                                                      m_eglContextAttrs.data(),surface,m_eglApi);
    }
}

void QOpenKODEWindow::setVisible(bool visible)
{
    if (!m_kdWindow)
        return;
    KDboolean visibillity(visible);
    if (kdSetWindowPropertybv(m_kdWindow, KD_WINDOWPROPERTY_VISIBILITY, &visibillity)) {
        qErrnoWarning(kdGetError(), "Could not set visibillity property");
    }
}

WId QOpenKODEWindow::winId() const
{
    static int i = 0;
    return i++;
}

QPlatformGLContext *QOpenKODEWindow::glContext() const
{
    return m_platformGlContext;
}

void QOpenKODEWindow::raise()
{
    if (!m_kdWindow)
        return;
    KDboolean focus(true);
    if (kdSetWindowPropertybv(m_kdWindow, KD_WINDOWPROPERTY_FOCUS, &focus)) {
        qErrnoWarning(kdGetError(), "Could not set focus");
    }
}

void QOpenKODEWindow::lower()
{
    if (!m_kdWindow)
        return;
    KDboolean focus(false);
    if (kdSetWindowPropertybv(m_kdWindow, KD_WINDOWPROPERTY_FOCUS, &focus)) {
        qErrnoWarning(kdGetError(), "Could not set focus");
    }
}

void QOpenKODEWindow::processMouseEvents(const KDEvent *event)
{
    int x = event->data.inputpointer.x;
    int y = event->data.inputpointer.y;
    Qt::MouseButtons buttons;
    switch(event->data.inputpointer.select) {
    case 1:
        buttons = Qt::LeftButton;
        break;
    default:
        buttons = Qt::NoButton;
    }
    QPoint pos(x,y);
    QWindowSystemInterface::handleMouseEvent(0,event->timestamp,pos,pos,buttons);
}

void QOpenKODEWindow::processKeyEvents(const KDEvent *event)
{
#ifdef KD_ATX_keyboard
    //KD_KEY_PRESS_ATX 1
    QEvent::Type keyPressed = QEvent::KeyRelease;
    if (event->data.keyboardInputKey.flags)
        keyPressed = QEvent::KeyPress;
//KD_KEY_LOCATION_LEFT_ATX // dont care for now
//KD_KEY_LOCATION_RIGHT_ATX
//KD_KEY_LOCATION_NUMPAD_ATX
    Qt::KeyboardModifiers mod = Qt::NoModifier;
    int openkodeMods = event->data.keyboardInputKey.flags;
    if (openkodeMods & KD_KEY_MODIFIER_SHIFT_ATX)
        mod |= Qt::ShiftModifier;
    if (openkodeMods & KD_KEY_MODIFIER_CTRL_ATX)
        mod |= Qt::ControlModifier;
    if (openkodeMods & KD_KEY_MODIFIER_ALT_ATX)
        mod |= Qt::AltModifier;
    if (openkodeMods & KD_KEY_MODIFIER_META_ATX)
        mod |= Qt::MetaModifier;

    Qt::Key qtKey;
    QChar keyText;
    int key = event->data.keyboardInputKey.keycode;
    if (key >= 0x20 && key <= 0x0ff){ // 8 bit printable Latin1
        qtKey = Qt::Key(key);
        keyText = QChar(event->data.keyboardInputKeyChar.character);
        if (!(mod & Qt::ShiftModifier))
            keyText = keyText.toLower();
    } else {
        qtKey = keyTranslator(key);
    }
    QWindowSystemInterface::handleKeyEvent(0,event->timestamp,keyPressed,qtKey,mod,keyText);
#endif
}

QT_END_NAMESPACE
