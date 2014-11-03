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

#include "qpepperinstance.h"

#ifndef QT_PEPPER_STANDALONE_MODE
#include "qpepperhelpers.h"
#include "qpeppermodule.h"
#include "qpepperintegration.h"
#include "qpepperjavascriptbridge.h"
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

    // Log the arguemnt key/value pairs.
    for (unsigned int i = 0; i < argc; ++i) {
        qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "arg" << argn[i] << argv[i];
    }

    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_WHEEL | PP_INPUTEVENT_CLASS_KEYBOARD);

    return true;
}

// DidchangeView is called on div tag geometry or configuration change, for
// example if the pixel scalng changes. This is also where we start Qt.
void QPepperInstance::DidChangeView(const View &view)
{

    qCDebug(QT_PLATFORM_PEPPER_INSTANCE) << "DidChangeView" << view.GetRect().width() << view.GetRect().height()
                                         << view.GetDeviceScale();

    Rect geometry = view.GetRect();
    if (geometry.size() == m_currentGeometry.size())
        return;

    m_currentGeometry = geometry;
    m_currentDevicePixelRatio = view.GetDeviceScale();

    // Start Qt on the first DidChangeView. This means we have a "screen" while
    // the application creates the root window etc, which makes life easier.
    if (!m_qtStarted) {
        startQt();
        m_qtStarted = true;
    }  else {
        m_pepperIntegraton->resizeScreen(toQSize(m_currentGeometry.size()), m_currentDevicePixelRatio);
    }

    m_pepperIntegraton->processEvents();
}

void QPepperInstance::DidChangeFocus(bool has_focus)
{
    Q_UNUSED(has_focus);
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

// HandleMessage expects a message of the format "funcitonName:argument", or
// "functionName" and will call the corresponding slot on qtScriptableObject.
void QPepperInstance::HandleMessage(const Var& var_message)
{
#if 0
    if (var_message.is_string()) {
        QString message = QString::fromUtf8(var_message.AsString().data());
        QStringList parts = message.split(':');
        if (parts.count() == 2) {
            emit m_pepperIntegraton->m_javascriptBridge->evalFunctionReply(parts.at(0).toLocal8Bit(), parts.at(1));
        }
     }
#endif
}

QRect QPepperInstance::geometry()
{
    return toQRect(m_currentGeometry);
}

QRect QPepperInstance::deviceGeometry()
{
    QRect geometry = toQRect(m_currentGeometry);
    geometry.setSize(geometry.size() * m_currentDevicePixelRatio);
    return geometry;
}

qreal QPepperInstance::devicePixelRatio()
{
    return m_currentDevicePixelRatio;
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
