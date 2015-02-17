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

#ifndef QT_PEPPER_INSTANCE_IMPL_H
#define QT_PEPPER_INSTANCE_IMPL_H

#include "qpeppereventtranslator.h"
#include "qpepperhelpers.h"

#include <qpa/qplatformtheme.h>
#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qqueue.h>

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/rect.h>
#include <ppapi/cpp/size.h>
#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/view.h>
#include <ppapi/utility/completion_callback_factory.h>
#include <ppapi/utility/threading/simple_thread.h>

#include <pthread.h>

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_INSTANCE)

// ThreadSafeQueue is a simple thread-safe queue. T must
// be default-constructible and copyable.
template <typename T> class ThreadSafeQueue
{
public:
    ThreadSafeQueue()
        : m_quit(false)
    {
    }

    ~ThreadSafeQueue() { setQuit(); }

    void enqueue(const T &item)
    {
        QMutexLocker lock(&m_lock);
        m_queue.enqueue(item);
        m_nextEvent.wakeAll();
    }

    T dequeue()
    {
        QMutexLocker lock(&m_lock);
        while (!m_quit && m_queue.isEmpty())
            m_nextEvent.wait(&m_lock);
        return m_quit ? T() : m_queue.dequeue();
    }

    void setQuit()
    {
        QMutexLocker lock(&m_lock);
        m_quit = true;
        m_nextEvent.wakeAll();
    }

    bool testQuit()
    {
        QMutexLocker lock(&m_lock);
        return m_quit;
    }

private:
    QWaitCondition m_nextEvent;
    QMutex m_lock;
    QQueue<T> m_queue;
    bool m_quit;
};

class QPepperInstance;
class QPepperIntegration;
class QPepperInstancePrivate
{
public:
    QPepperInstancePrivate(QPepperInstance *instance);
    virtual ~QPepperInstancePrivate();
    static QPepperInstancePrivate *get();
    static QPepperInstance *getInstance();

    bool init(int32_t result, uint32_t argc, const QVector<QByteArray> &vargn,
              const QVector<QByteArray> &vargv);
    void didChangeView(int32_t result, const pp::View &view);
    void didChangeFocus(int32_t result, bool hasFucus);
    bool handleInputEvent(int32_t result, const pp::InputEvent &event);
    bool handleDocumentLoad(int32_t result, const pp::URLLoader &url_loader);
    void handleMessage(int32_t result, const pp::Var &var_message);

    // Instance attribute getters
    QRect geometry();
    QRect deviceGeometry();
    qreal devicePixelRatio();
    qreal cssScale();

    // publics:
    void processCall(pp::CompletionCallback call);
    void scheduleWindowSystemEventsFlush();
    void postMessage(const QByteArray &message);
    void runJavascript(const QByteArray &script);
    void registerMessageHandler(const QByteArray &messageTag, QObject *obj, const char *slot);
    QPlatformTheme::KeyboardSchemes keyboardScheme();

    // privates:
    void startQt();
    void windowSystemEventsFlushCallback(int32_t);
    void handleGetAppVersionMessage(const QByteArray &message);
    void processMessageLoopMessage();
    void qtMessageLoop();

    void flushCompletedCallback(int32_t);

    QPepperInstance *q;
    QPepperIntegration *m_pepperIntegraton;

    bool m_qtStarted;
    bool m_runQtOnThread;
    bool m_useQtMessageLoop;
    pp::SimpleThread m_qtThread;
    pthread_t m_qtThread_pthread; // ##name
    pp::MessageLoop m_qtMessageLoop;
    ThreadSafeQueue<pp::CompletionCallback> m_qtMessageLoop_pthread; // ##name

    pp::Var m_console;
    pp::Rect m_currentGeometry;
    qreal m_currentCssScale;
    qreal m_currentDeviceScale;
    qreal m_currentDevicePixelRatio;

    pp::Graphics2D *m_context2D;
    pp::ImageData *m_imageData2D;
    QImage *m_frameBuffer;
    bool m_inFlush;
    QHash<QByteArray, QPair<QPointer<QObject>, const char *>> m_messageHandlers;

    pp::CompletionCallbackFactory<QPepperInstancePrivate> m_callbackFactory;
};

#endif
