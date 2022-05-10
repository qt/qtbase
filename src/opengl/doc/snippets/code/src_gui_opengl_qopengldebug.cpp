// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QWidget>
#include <QtOpenGL/QOpenGLDebugLogger>

namespace src_gui_opengl_qopengldebug {
struct LogHandler : public QObject
{
    Q_OBJECT
public slots:
    static bool handleLoggedMessage() { return true; };
};
struct SnippetWrapper : public QObject
{
    Q_OBJECT
    void wrapper1(LogHandler *receiver);
    const QWidget *receiver;
};

void wrapper0() {

//! [0]
GLenum error = GL_NO_ERROR;
do {
    error = glGetError();
    if (error != GL_NO_ERROR) {
        // handle the error
    }
} while (error != GL_NO_ERROR);
//! [0]


//! [1]
QSurfaceFormat format;
// asks for a OpenGL 3.2 debug context using the Core profile
format.setMajorVersion(3);
format.setMinorVersion(2);
format.setProfile(QSurfaceFormat::CoreProfile);
format.setOption(QSurfaceFormat::DebugContext);

QOpenGLContext *context = new QOpenGLContext;
context->setFormat(format);
context->create();
//! [1]

} // wrapper0


void SnippetWrapper::wrapper1(LogHandler *receiver) {
//! [2]
QOpenGLContext *ctx = QOpenGLContext::currentContext();
QOpenGLDebugLogger *logger = new QOpenGLDebugLogger(this);

logger->initialize(); // initializes in the current context, i.e. ctx
//! [2]


//! [3]
ctx->hasExtension(QByteArrayLiteral("GL_KHR_debug"));
//! [3]


//! [4]
const QList<QOpenGLDebugMessage> messages = logger->loggedMessages();
for (const QOpenGLDebugMessage &message : messages)
    qDebug() << message;
//! [4]


//! [5]
connect(logger, &QOpenGLDebugLogger::messageLogged, receiver, &LogHandler::handleLoggedMessage);
logger->startLogging();
//! [5]


//! [6]
QOpenGLDebugMessage message =
    QOpenGLDebugMessage::createApplicationMessage(QStringLiteral("Custom message"));

logger->logMessage(message);
//! [6]

} // SnippetWrapper::wrapper1
} // src_gui_opengl_qopengldebug
