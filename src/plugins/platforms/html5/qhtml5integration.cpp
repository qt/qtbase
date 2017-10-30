/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhtml5integration.h"
#include "qhtml5eventtranslator.h"
#include "qhtml5eventdispatcher.h"
#include "qhtml5compositor.h"

#include "qhtml5window.h"
#ifndef QT_NO_OPENGL
# include "qhtml5backingstore.h"
#endif
#include "qhtml5fontdatabase.h"
#if defined(Q_OS_UNIX)
#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#endif
#include <qpa/qplatformwindow.h>
#include <QtGui/QScreen>
#include <qpa/qwindowsysteminterface.h>

// this is where EGL headers are pulled in, make sure it is last
#include "qhtml5screen.h"

QT_BEGIN_NAMESPACE

static QHTML5Integration *globalHtml5Integration;
QHTML5Integration *QHTML5Integration::get() { return globalHtml5Integration; }

void emscriptenOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    int emOutputFlags = (EM_LOG_CONSOLE | EM_LOG_DEMANGLE);
    QByteArray localMsg = msg.toLocal8Bit();

    switch (type) {
    case QtDebugMsg:
        emscripten_log(emOutputFlags, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        emscripten_log(emOutputFlags, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        emOutputFlags |= EM_LOG_WARN;
        emscripten_log(emOutputFlags, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        emOutputFlags |= EM_LOG_ERROR;
        emscripten_log(emOutputFlags, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        emOutputFlags |= EM_LOG_ERROR;
        emscripten_log(emOutputFlags, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}

QHTML5Integration::QHTML5Integration()
    : mFontDb(0),
      mCompositor(new QHtml5Compositor),
      mScreen(new QHTML5Screen(EGL_DEFAULT_DISPLAY, mCompositor)),
      m_eventDispatcher(0)
{
    qSetMessagePattern(QString("(%{function}:%{line}) - %{message}"));
   // qInstallMessageHandler(emscriptenOutput);

    globalHtml5Integration = this;
    screenAdded(mScreen);
    emscripten_set_resize_callback(0, (void *)this, 1, uiEvent_cb);

    m_eventTranslator = new QHTML5EventTranslator();
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QHTML5Integration\n");
#endif
}

QHTML5Integration::~QHTML5Integration()
{
    delete mCompositor;
    destroyScreen(mScreen);
    delete mFontDb;
    delete m_eventTranslator;
}

bool QHTML5Integration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    case RasterGLSurface: return true;
    case MultipleWindows: return true;
    case WindowManagement: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QHTML5Integration::createPlatformWindow(QWindow *window) const
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QHTML5Integration::createPlatformWindow %p\n",window);
#endif

    QHtml5Window *w = new QHtml5Window(window, mCompositor);
    w->create();

    return w;
}

QPlatformBackingStore *QHTML5Integration::createPlatformBackingStore(QWindow *window) const
{
//#ifdef QEGL_EXTRA_DEBUG
    qWarning("QHTML5Integration::createWindowSurface %p\n", window);
//#endif
#ifndef QT_NO_OPENGL
    return new QHTML5BackingStore(mCompositor, window);
#else
    return nullptr;
#endif
}
#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QHTML5Integration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return static_cast<QHTML5Screen *>(context->screen()->handle())->platformContext();
}
#endif

QPlatformFontDatabase *QHTML5Integration::fontDatabase() const
{
    if (mFontDb == 0)
        mFontDb = new QHtml5FontDatabase();

    return mFontDb;
}

QAbstractEventDispatcher *QHTML5Integration::createEventDispatcher() const
{
    return new QHtml5EventDispatcher();
}

QVariant QHTML5Integration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    if (hint == QPlatformIntegration::ShowIsFullScreen)
        return true;

    return QPlatformIntegration::styleHint(hint);
}

int QHTML5Integration::uiEvent_cb(int eventType, const EmscriptenUiEvent *e, void */*userData*/)
{
    Q_UNUSED(eventType)
    Q_UNUSED(e)

    //QSize windowSize(e->documentBodyClientWidth, e->documentBodyClientHeight);
    //QRect windowRect(QPoint(0, 0), windowSize);

    QHTML5Integration::get()->mScreen->invalidateSize();
    //QHTML5Integration::get()->mScreen->resizeMaximizedWindows();
    QHTML5Integration::get()->mCompositor->requestRedraw();

    return 0;
}


QT_END_NAMESPACE
