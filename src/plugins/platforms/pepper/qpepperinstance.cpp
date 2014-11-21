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

#include "qpepperinstance.h"

#ifndef QT_PEPPER_STANDALONE_MODE
#include "qpepperhelpers.h"
#include "qpeppermodule.h"
#include "qpepperintegration.h"
#endif

#include <qpa/qwindowsysteminterface.h>

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/completion_callback.h"

#ifdef Q_OS_NACL_NEWLIB
#include "error_handling/error_handling.h"
#endif

using namespace pp;

QObject *qtScriptableObject;

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_INSTANCE, "qt.platform.pepper.instance")

QPepperInstance::QPepperInstance(PP_Instance instance)
: pp::Instance(instance)
{
    m_qtStarted = false;
    m_currentGeometry = Rect();
    m_callbackFactory.Initialize(this);
}

QPepperInstance::~QPepperInstance()
{
}

static QPepperInstance *g_pepperInstance = 0;
QPepperInstance *QPepperInstance::get()
{
    return g_pepperInstance;
}

#ifdef Q_OS_NACL_NEWLIB
void qtExceptionHandler(const char* json) {
    qDebug() << "CRASH";
    qDebug() << json;
}
#endif

// Message handler for redirecting Qt debug output to the web page.
void qtMessageHandler(QtMsgType, const QMessageLogContext &context, const QString &message)
{
   QByteArray bytes = context.category + message.toLatin1() + "\n";
   QPepperInstance::get()->PostMessage(pp::Var(bytes.constData()));
}

// There is one global app pp::Instance. It corresponds to the
// html div tag that contains the app.
bool QPepperInstance::Init(uint32_t argc, const char* argn[], const char* argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argn);
    Q_UNUSED(argv);

    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "Init argc:" << argc;

#ifdef Q_OS_NACL_NEWLIB
    // Comment in next line to enable the exception/crash handler.
    // EHRequestExceptionsJson(qtExceptionHandler);
#endif

    g_pepperInstance = this;

    if (false) {
        // Optinally redirect debug and logging output to the web page following the conventions
        // used in the nacl_sdk examples: presence of 'ps_stdout="/dev/tty"' attributes
        // signales that the web page can handle debug output. This has a couple of drawbacks:
        // 1) debug utput sent before installing the message handler still goes to Chrome
        //    standard output.
        // 2) It reserves the postMessage functionality for debug output, this may conflict
        //    with other use cases.
        // 3) It's really slow.
        for (unsigned int i = 0; i < argc; ++i) {
            if (qstrcmp(argn[i], "ps_stdout") && qstrcmp(argv[i], "/dev/tty"))
                qInstallMessageHandler(qtMessageHandler);
        }
    }

    // Place argument key/value pairs in the process environment. (NaCl/pepper does
    // not support environment variables, Qt emulates via qputenv and getenv.)
    for (unsigned int i = 0; i < argc; ++i) {
        QByteArray name = QByteArray(argn[i]).toUpper(); // html forces lowercase.
        QByteArray value = argv[i];
        qputenv(name.constData(), value);
        qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "setting environment variable" << name << "=" << value;
    }

    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_WHEEL | PP_INPUTEVENT_CLASS_KEYBOARD);

    return true;
}

// DidchangeView is called on div tag geometry or configuration change, for
// example if the pixel scaling changes. This is also where Qt is started.
void QPepperInstance::DidChangeView(const View &view)
{
    Rect geometry = view.GetRect();
    // Compute the effective devicePixelRatio. DeviceScale is related
    // to the hardware and is often an integer. CSSScale is the is the
    // zoom factor and is often a real number.
    // NOTE: This leads to non-integer devicePixelRatios. We might want
    // add an option to not factor in CSSScale.
    qreal deviceScale = view.GetDeviceScale();
    qreal cssScale = view.GetCSSScale();
    qreal devicePixelRatio = deviceScale * cssScale;

    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "DidChangeView" << toQRect(geometry)
                                         << "DeviceScale" << deviceScale
                                         << "CSSScale" << cssScale
                                         << "devicePixelRatio" << devicePixelRatio;

    if (geometry.size() == m_currentGeometry.size() && devicePixelRatio == m_currentDevicePixelRatio)
        return;

    m_currentGeometry = geometry;
    m_currentDeviceScale = deviceScale;
    m_currentCssScale = cssScale;
    m_currentDevicePixelRatio = devicePixelRatio;

    // Start Qt on the first DidChangeView. This means we have a "screen" while
    // the application creates the root window etc, which makes life easier.
    if (!m_qtStarted) {
        startQt();
        m_qtStarted = true;
    }  else {
        m_pepperIntegraton->resizeScreen(this->geometry().size(), m_currentDevicePixelRatio);
    }

    m_pepperIntegraton->processEvents();
}

void QPepperInstance::DidChangeFocus(bool hasFucus)
{
    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "DidChangeFocus" << hasFucus;

    QWindow *fucusWindow = (hasFucus &&  m_pepperIntegraton && m_pepperIntegraton->m_topLevelWindow)
                         ? m_pepperIntegraton->m_topLevelWindow->window() : 0;
    QWindowSystemInterface::handleWindowActivated(fucusWindow, Qt::ActiveWindowFocusReason);
}

bool QPepperInstance::HandleInputEvent(const pp::InputEvent& event)
{
    // this one is spammy (mouse moves)
    // qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "HandleInputEvent";

    bool ret = m_pepperIntegraton->pepperEventTranslator()->processEvent(event);
    m_pepperIntegraton->processEvents();
    return ret;
}

bool QPepperInstance::HandleDocumentLoad(const URLLoader& url_loader)
{
    Q_UNUSED(url_loader);
    return false;
}

void QPepperInstance::HandleMessage(const Var& var_message)
{
    QByteArray message = toQByteArray(var_message);
    if (message.startsWith("qtGetAppVersion")) {
        handleGetAppVersionMessage(message);
    }
}

void QPepperInstance::handleGetAppVersionMessage(const QByteArray  &message)
{
    if (message.contains("OS X"))
        m_keyboardScheme = QPlatformTheme::MacKeyboardScheme;
    else if (message.contains("Win"))
        m_keyboardScheme = QPlatformTheme::WindowsKeyboardScheme;
    else
        m_keyboardScheme = QPlatformTheme::X11KeyboardScheme;
}

QPlatformTheme::KeyboardSchemes QPepperInstance::keyboardScheme()
{
    return m_keyboardScheme;
}

QRect QPepperInstance::geometry()
{
    QRect r = toQRect(m_currentGeometry);
    return QRect(r.topLeft(), r.size() / m_currentCssScale);
}

QRect QPepperInstance::deviceGeometry()
{
    QRect r = toQRect(m_currentGeometry);
    return QRect(r.topLeft(), r.size() * m_currentDeviceScale);
}

qreal QPepperInstance::devicePixelRatio()
{
    return m_currentDevicePixelRatio;
}

qreal QPepperInstance::cssScale()
{
    return m_currentCssScale;
}

// From time to time it's useful to flush and empty the posted
// window system events queue. However, Qt and Qt apps does not
// expect this type of event processing happening at arbitarty times
// (for example in the middle of setGeometry call). This function
// schedules a flushWindowSystemEvents() call for when Qt returns
// to the Pepper event loop.
void QPepperInstance::scheduleWindowSystemEventsFlush()
{
    QtModule::core()->
        CallOnMainThread(0,
                         m_callbackFactory.NewCallback(&QPepperInstance::windowSystemEventsFlushCallback),
                         0);
}

void QPepperInstance::windowSystemEventsFlushCallback(int32_t)
{
    QWindowSystemInterface::flushWindowSystemEvents();
}

void QPepperInstance::postMessage(const QByteArray &message)
{
    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "postMessage" << message;
    PostMessage(toPPVar(message));
}

// Runs the given script on the containing web page, using
// the standard Qt message handler provided by nacldeployqt.
// This function is designed to run "safe" javascript only:
// running javascript witch contains for example user-provided
// strings may open up for javascript injection attacks.
void QPepperInstance::runJavascript(const QByteArray &script)
{
    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "runJavascript" << script;
    postMessage("qtEval:" + script);
}

void QPepperInstance::startQt()
{
    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "startQt";

    // Create Qt applictaion object and platform plugin (the QPepperIntegration instance).
    extern void qGuiStartup();
    qGuiStartup();

    // link the QPepperIntegration instance to this QPepperInstance. This
    // links the Qt and pepper instances.
    m_pepperIntegraton = QPepperIntegration::getPepperIntegration();
    m_pepperIntegraton->setPepperInstance(this);
    m_pepperIntegraton->resizeScreen(toQSize(m_currentGeometry.size()), m_currentDevicePixelRatio);

    // Look at navigator.appVersion to get the host OS.
    const char *getAppVersionsMessageHandler = \
        "this.qtMessageHandlers[\"qtGetAppVersion\"] = function(url) { "
        "    embed.postMessage(\"qtGetAppVersion\"  + navigator.appVersion);"
        "}";
    runJavascript(getAppVersionsMessageHandler);
    postMessage("qtGetAppVersion: ");

    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "qGuiAppInit";
    // Run the applicaiton startup function which will create the root UI Window.
    extern void qGuiAppInit();
    qGuiAppInit();
}

void QPepperInstance::drawTestImage()
{
    Size devicePixelSize = toPPSize(deviceGeometry().size());

    // Create new graphics context and frame buffer.
    Graphics2D * m_context2D = new Graphics2D(this, devicePixelSize, false);
    if (!g_pepperInstance->BindGraphics(*m_context2D)) {
        qWarning("Couldn't bind the device context\n");
    }

    ImageData *m_imageData2D = new ImageData(this, PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                                  devicePixelSize, true);

    QImage *m_frameBuffer = new QImage(reinterpret_cast<uchar *>(m_imageData2D->data()),
                           devicePixelSize.width(),
                           devicePixelSize.height(),
                           m_imageData2D->stride(), QImage::Format_ARGB32_Premultiplied);

    m_frameBuffer->setDevicePixelRatio(m_currentDevicePixelRatio);
    m_frameBuffer->fill(Qt::red);
    m_context2D->PaintImageData(*m_imageData2D, pp::Point(0,0), toPPRect(deviceGeometry()));
    m_context2D->Flush(m_callbackFactory.NewCallback(&QPepperInstance::flushCompletedCallback));
}

void QPepperInstance::flushCompletedCallback(int32_t)
{

}
