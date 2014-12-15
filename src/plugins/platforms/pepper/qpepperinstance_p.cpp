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

#include "qpepperinstance_p.h"
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

QPepperInstancePrivate *qtCreatePepperInstancePrivate(QPepperInstance *instance)
{
    return new QPepperInstancePrivate(instance);
}

static QPepperInstancePrivate *g_pepperInstancePrivate = 0;

QPepperInstancePrivate::QPepperInstancePrivate(QPepperInstance *instance)
{
    q = instance;
    g_pepperInstancePrivate = this;
    m_qtStarted = false;
    m_currentGeometry = Rect();
    m_callbackFactory.Initialize(this);
}

QPepperInstancePrivate::~QPepperInstancePrivate()
{
}

QPepperInstancePrivate *QPepperInstancePrivate::get()
{
    return g_pepperInstancePrivate;
}

QPepperInstance *QPepperInstancePrivate::getInstance()
{
    return g_pepperInstancePrivate->q;
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
   QPepperInstancePrivate::getInstance()->PostMessage(pp::Var(bytes.constData()));
}

// There is one global app pp::Instance. It corresponds to the
// html div tag that contains the app.
bool QPepperInstancePrivate::init(uint32_t argc, const char* argn[], const char* argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argn);
    Q_UNUSED(argv);

    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "Init argc:" << argc;

#ifdef Q_OS_NACL_NEWLIB
    // Comment in next line to enable the exception/crash handler.
    // EHRequestExceptionsJson(qtExceptionHandler);
#endif

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

    // Enable input event types Qt is interested in. Filtering input events
    // are special: Qt can reject those and the event will bubble to the rest
    // of the page.
    q->RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_WHEEL);
    q->RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);

    return true;
}

// DidchangeView is called on div tag geometry or configuration change, for
// example if the pixel scaling changes. This is also where Qt is started.
void QPepperInstancePrivate::didChangeView(const View &view)
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

void QPepperInstancePrivate::didChangeFocus(bool hasFucus)
{
    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "DidChangeFocus" << hasFucus;

    QWindow *fucusWindow = (hasFucus &&  m_pepperIntegraton && m_pepperIntegraton->m_topLevelWindow)
                         ? m_pepperIntegraton->m_topLevelWindow->window() : 0;
    QWindowSystemInterface::handleWindowActivated(fucusWindow, Qt::ActiveWindowFocusReason);
}

bool QPepperInstancePrivate::handleInputEvent(const pp::InputEvent& event)
{
    // this one is spammy (mouse moves)
    // qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "HandleInputEvent";

    bool ret = m_pepperIntegraton->pepperEventTranslator()->processEvent(event);
    m_pepperIntegraton->processEvents();
    return ret;
}

bool QPepperInstancePrivate::handleDocumentLoad(const URLLoader& url_loader)
{
    Q_UNUSED(url_loader);
    return false;
}

void QPepperInstancePrivate::handleMessage(const Var& var_message)
{
    // Expect messages formatted like "tag:message". Dispatch the
    // message to the handler registered for "tag".
    QByteArray q_message = toQByteArray(var_message);
    int split = q_message.indexOf(':');
    if (split == -1)
        return;
    QByteArray tag = q_message.mid(0, split);
    QByteArray message = q_message.mid(split + 1);
    if (m_messageHandlers.contains(tag)) {
        QPair<QPointer<QObject>, const char *> handler = m_messageHandlers[tag];
        if (!handler.first.isNull())
            QMetaObject::invokeMethod(handler.first.data(), handler.second, Q_ARG(QByteArray, message));
    }
}

void QPepperInstancePrivate::registerMessageHandler(const QByteArray &messageTag, QObject *obj, const char *slot)
{
    m_messageHandlers[messageTag] = qMakePair(QPointer<QObject>(obj), slot);
}

QRect QPepperInstancePrivate::geometry()
{
    QRect r = toQRect(m_currentGeometry);
    return QRect(r.topLeft(), r.size() / m_currentCssScale);
}

QRect QPepperInstancePrivate::deviceGeometry()
{
    QRect r = toQRect(m_currentGeometry);
    return QRect(r.topLeft(), r.size() * m_currentDeviceScale);
}

qreal QPepperInstancePrivate::devicePixelRatio()
{
    return m_currentDevicePixelRatio;
}

qreal QPepperInstancePrivate::cssScale()
{
    return m_currentCssScale;
}

// From time to time it's useful to flush and empty the posted
// window system events queue. However, Qt and Qt apps does not
// expect this type of event processing happening at arbitarty times
// (for example in the middle of setGeometry call). This function
// schedules a flushWindowSystemEvents() call for when Qt returns
// to the Pepper event loop.
void QPepperInstancePrivate::scheduleWindowSystemEventsFlush()
{
    QtModule::core()->
        CallOnMainThread(0,
                         m_callbackFactory.NewCallback(&QPepperInstancePrivate::windowSystemEventsFlushCallback),
                         0);
}

void QPepperInstancePrivate::windowSystemEventsFlushCallback(int32_t)
{
    QWindowSystemInterface::flushWindowSystemEvents();
}

void QPepperInstancePrivate::postMessage(const QByteArray &message)
{
    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "postMessage" << message;
    q->PostMessage(toPPVar(message));
}

// Runs the given script on the containing web page, using
// the standard Qt message handler provided by nacldeployqt.
// This function is designed to run "safe" javascript only:
// running javascript witch contains for example user-provided
// strings may open up for javascript injection attacks.
void QPepperInstancePrivate::runJavascript(const QByteArray &script)
{
    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "runJavascript" << script;
    postMessage("qtEval:" + script);
}

void QPepperInstancePrivate::startQt()
{
    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "startQt";

    // Create Qt applictaion object and platform plugin (the QPepperIntegration instance).
    extern void qGuiStartup();
    qGuiStartup();

    // link the QPepperIntegration instance to this QPepperInstancePrivate. This
    // links the Qt and pepper instances.
    m_pepperIntegraton = QPepperIntegration::getPepperIntegration();
    m_pepperIntegraton->setPepperInstance(q);
    m_pepperIntegraton->resizeScreen(toQSize(m_currentGeometry.size()), m_currentDevicePixelRatio);

    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "qGuiAppInit";
    q->applicationInit();
}

void QPepperInstancePrivate::flushCompletedCallback(int32_t)
{

}
