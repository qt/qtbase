/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtGui>

class EventTarget : public QObject
{
    Q_OBJECT
protected:
    bool event(QEvent *evt)
    {
        if (evt->type() == QEvent::User) {
            qDebug() << "User event on thread" << QThread::currentThread();
            return true;
        }
        return QObject::event(evt);
    }
};

class EventPosterWindow: public QRasterWindow
{
public:
    EventPosterWindow(EventTarget *target)
        :m_target(target)
        { }

    void paintEvent(QPaintEvent *ev) override {
        QPainter p(this);
        p.fillRect(ev->rect(), QColorConstants::Svg::deepskyblue);
        p.drawText(50, 100, "Application has started. Click to post events.\n See the developer tools console for debug output");
    }

    void mousePressEvent(QMouseEvent *) override {
        qDebug() << "Posting events from thread" << QThread::currentThread();
        QGuiApplication::postEvent(m_target, new QEvent(QEvent::User));
        QTimer::singleShot(500, m_target, []() {
            qDebug() << "Timer event on secondary thread" << QThread::currentThread();
        });
    }

public:
    EventTarget *m_target;
};

class SecondaryThread : public QThread
{
public:
    void run() override {
        qDebug() << "exec on secondary thread" << QThread::currentThread();
        exec();
    }
};

// This example demonstrates how to start a secondary thread event loop
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    EventTarget eventTarget;

    EventPosterWindow window(&eventTarget);
    window.show();

    SecondaryThread thread;
    eventTarget.moveToThread(&thread);

#if QT_CONFIG(thread)
    thread.start();
#else
    qDebug() << "Warning: This test requires a multithreaded build of Qt for WebAssembly";
#endif

    return app.exec();
}

#include "main.moc"
