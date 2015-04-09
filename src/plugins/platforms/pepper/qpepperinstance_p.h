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

#ifndef QTPEPPERINSTANCE_P_H
#define QTPEPPERINSTANCE_P_H

#include "qpeppereventtranslator.h"

#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QWaitCondition>
#include <qpa/qplatformtheme.h>

#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/rect.h>
#include <ppapi/cpp/size.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/view.h>
#include <ppapi/utility/completion_callback_factory.h>
#include <ppapi/utility/threading/simple_thread.h>

#include <pthread.h>

QT_BEGIN_NAMESPACE

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
    ~QPepperInstancePrivate();

    static QPepperInstancePrivate *get();
    static pp::Instance *getPPInstance();

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
    QHash<QByteArray, QPair<QPointer<QObject>, const char *> > m_messageHandlers;

    pp::CompletionCallbackFactory<QPepperInstancePrivate> m_callbackFactory;
};

QT_END_NAMESPACE

#endif
