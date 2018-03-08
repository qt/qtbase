/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <QtWidgets/QApplication>
#include <QtOpenGL/QtOpenGL>
#include "tst_qglthreads.h"

#ifndef QT_OPENGL_ES_2
#include <QtGui/QOpenGLFunctions_1_0>
#endif

#define RUNNING_TIME 5000

tst_QGLThreads::tst_QGLThreads(QObject *parent)
    : QObject(parent)
{
}

/*

   swapInThread

   The purpose of this testcase is to verify that it is possible to do rendering into
   a GL context from the GUI thread, then swap the contents in from a background thread.

   The usecase for this is to have the background thread do the waiting for vertical
   sync while the GUI thread is idle.

   Currently the locking is handled directly in the paintEvent(). For the actual usecase
   in Qt, the locking is done in the windowsurface before starting any drawing while
   unlocking is done after all drawing has been done.
 */


class SwapThread : public QThread
{
    Q_OBJECT
public:
    SwapThread(QGLWidget *widget)
        : m_context(widget->context())
        , m_swapTriggered(false)
    {
        moveToThread(this);
    }

    void run() {
        QTime time;
        time.start();
        while (time.elapsed() < RUNNING_TIME) {
            lock();
            waitForReadyToSwap();

            m_context->makeCurrent();
            m_context->swapBuffers();
            m_context->doneCurrent();

            m_context->moveToThread(qApp->thread());

            signalSwapDone();
            unlock();
        }

        m_swapTriggered = false;
    }

    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }

    void waitForSwapDone() { if (m_swapTriggered) m_swapDone.wait(&m_mutex); }
    void waitForReadyToSwap() { if (!m_swapTriggered) m_readyToSwap.wait(&m_mutex); }

    void signalReadyToSwap()
    {
        if (!isRunning())
            return;
        m_readyToSwap.wakeAll();
        m_swapTriggered = true;
    }

    void signalSwapDone()
    {
        m_swapTriggered = false;
        m_swapDone.wakeAll();
    }

private:
    QGLContext *m_context;
    QMutex m_mutex;
    QWaitCondition m_readyToSwap;
    QWaitCondition m_swapDone;

    bool m_swapTriggered;
};

class ForegroundWidget : public QGLWidget
{
public:
    ForegroundWidget(const QGLFormat &format)
        : QGLWidget(format), m_thread(0)
    {
        setAutoBufferSwap(false);
    }

    void paintEvent(QPaintEvent *)
    {
        m_thread->lock();
        m_thread->waitForSwapDone();

        makeCurrent();
        QPainter p(this);
        p.fillRect(rect(), QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));
        p.setPen(Qt::red);
        p.setFont(QFont("SansSerif", 24));
        p.drawText(rect(), Qt::AlignCenter, "This is an autotest");
        p.end();
        doneCurrent();

        if (m_thread->isRunning()) {
            context()->moveToThread(m_thread);
            m_thread->signalReadyToSwap();
        }

        m_thread->unlock();

        update();
    }

    void setThread(SwapThread *thread) {
        m_thread = thread;
    }

    SwapThread *m_thread;
};

void tst_QGLThreads::swapInThread()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL))
        QSKIP("No platformsupport for ThreadedOpenGL");
    QGLFormat format;
    format.setSwapInterval(1);
    ForegroundWidget widget(format);
    SwapThread thread(&widget);
    widget.setThread(&thread);
    widget.show();

    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    thread.start();

    while (thread.isRunning()) {
        qApp->processEvents();
    }

    widget.hide();

    QVERIFY(true);
}

/*
   renderInThread

   This test sets up a scene and renders it in a different thread.
   For simplicity, the scene is simply a bunch of rectangles, but
   if that works, we're in good shape..
 */

static inline float qrandom() { return (QRandomGenerator::global()->bounded(100)) / 100.f; }

void renderAScene(int w, int h)
{
    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();

    if (QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_UNUSED(w);
        Q_UNUSED(h);
        QGLShaderProgram program;
        program.addShaderFromSourceCode(QGLShader::Vertex, "attribute highp vec2 pos; void main() { gl_Position = vec4(pos.xy, 1.0, 1.0); }");
        program.addShaderFromSourceCode(QGLShader::Fragment, "uniform lowp vec4 color; void main() { gl_FragColor = color; }");
        program.bindAttributeLocation("pos", 0);
        program.bind();

        funcs->glEnableVertexAttribArray(0);

        for (int i=0; i<1000; ++i) {
            GLfloat pos[] = {
                (QRandomGenerator::global()->bounded(100)) / 100.f,
                (QRandomGenerator::global()->bounded(100)) / 100.f,
                (QRandomGenerator::global()->bounded(100)) / 100.f,
                (QRandomGenerator::global()->bounded(100)) / 100.f,
                (QRandomGenerator::global()->bounded(100)) / 100.f,
                (QRandomGenerator::global()->bounded(100)) / 100.f
            };

            funcs->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, pos);
            funcs->glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
        }
    } else {
#ifndef QT_OPENGL_ES_2
        QOpenGLFunctions_1_0 *gl1funcs = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_1_0>();
        gl1funcs->initializeOpenGLFunctions();

        gl1funcs->glViewport(0, 0, w, h);

        gl1funcs->glMatrixMode(GL_PROJECTION);
        gl1funcs->glLoadIdentity();
        gl1funcs->glFrustum(0, w, h, 0, 1, 100);
        gl1funcs->glTranslated(0, 0, -1);

        gl1funcs->glMatrixMode(GL_MODELVIEW);
        gl1funcs->glLoadIdentity();

        for (int i=0;i<1000; ++i) {
            gl1funcs->glBegin(GL_TRIANGLES);
            gl1funcs->glColor3f(qrandom(), qrandom(), qrandom());
            gl1funcs->glVertex2f(qrandom() * w, qrandom() * h);
            gl1funcs->glColor3f(qrandom(), qrandom(), qrandom());
            gl1funcs->glVertex2f(qrandom() * w, qrandom() * h);
            gl1funcs->glColor3f(qrandom(), qrandom(), qrandom());
            gl1funcs->glVertex2f(qrandom() * w, qrandom() * h);
            gl1funcs->glEnd();
        }
#endif
    }
}

class ThreadSafeGLWidget : public QGLWidget
{
public:
    ThreadSafeGLWidget(QWidget *parent = 0) : QGLWidget(parent) {}
    void paintEvent(QPaintEvent *)
    {
        // ignored as we're anyway swapping as fast as we can
    };

    void resizeEvent(QResizeEvent *e)
    {
        mutex.lock();
        newSize = e->size();
        mutex.unlock();
    };

    QMutex mutex;
    QSize newSize;
};

class SceneRenderingThread : public QThread
{
    Q_OBJECT
public:
    SceneRenderingThread(ThreadSafeGLWidget *widget)
        : m_widget(widget)
    {
        moveToThread(this);
        m_size = widget->size();
    }

    void run() {
        QTime time;
        time.start();
        failure = false;

        while (time.elapsed() < RUNNING_TIME && !failure) {

            m_widget->makeCurrent();

            m_widget->mutex.lock();
            QSize s = m_widget->newSize;
            m_widget->mutex.unlock();

            QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
            if (s != m_size) {
                funcs->glViewport(0, 0, s.width(), s.height());
            }

            if (QGLContext::currentContext() != m_widget->context()) {
                failure = true;
                break;
            }

            funcs->glClear(GL_COLOR_BUFFER_BIT);

            int w = m_widget->width();
            int h = m_widget->height();

            renderAScene(w, h);

            int color;
            funcs->glReadPixels(w / 2, h / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);

            m_widget->swapBuffers();
        }

        m_widget->doneCurrent();
    }

    bool failure;

private:
    ThreadSafeGLWidget *m_widget;
    QSize m_size;
};

void tst_QGLThreads::renderInThread_data()
{
    QTest::addColumn<bool>("resize");
    QTest::addColumn<bool>("update");

    QTest::newRow("basic") << false << false;
    QTest::newRow("with-resize") << true << false;
    QTest::newRow("with-update") << false << true;
    QTest::newRow("with-resize-and-update") << true << true;
}

void tst_QGLThreads::renderInThread()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL))
        QSKIP("No platformsupport for ThreadedOpenGL");

    QFETCH(bool, resize);
    QFETCH(bool, update);

    ThreadSafeGLWidget widget;
    widget.resize(200, 200);
    SceneRenderingThread thread(&widget);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.doneCurrent();

    widget.context()->moveToThread(&thread);

    thread.start();

    int value = 10;
    while (thread.isRunning()) {
        if (resize)
            widget.resize(200 + value, 200 + value);
        if (update)
            widget.update(100 + value, 100 + value, 20, 20);
        qApp->processEvents();
        value = -value;

        QThread::msleep(100);
    }

    QVERIFY(!thread.failure);
}

class Device
{
public:
    virtual ~Device() {}
    virtual QPaintDevice *realPaintDevice() = 0;
    virtual void prepareDevice() {}
    virtual void moveToThread(QThread *) {}
};

class GLWidgetWrapper : public Device
{
public:
    GLWidgetWrapper() {
        widget.resize(150, 150);
        widget.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        widget.doneCurrent();
    }
    QPaintDevice *realPaintDevice() { return &widget; }
    void moveToThread(QThread *thread) { widget.context()->moveToThread(thread); }

    ThreadSafeGLWidget widget;
};

class PixmapWrapper : public Device
{
public:
    PixmapWrapper() { pixmap = new QPixmap(512, 512); }
    ~PixmapWrapper() { delete pixmap; }
    QPaintDevice *realPaintDevice() { return pixmap; }

    QPixmap *pixmap;
};

class PixelBufferWrapper : public Device
{
public:
    PixelBufferWrapper() { pbuffer = new QGLPixelBuffer(512, 512); }
    ~PixelBufferWrapper() { delete pbuffer; }
    QPaintDevice *realPaintDevice() { return pbuffer; }
    void moveToThread(QThread *thread) { pbuffer->context()->moveToThread(thread); }

    QGLPixelBuffer *pbuffer;
};


class FrameBufferObjectWrapper : public Device
{
public:
    FrameBufferObjectWrapper() {
        widget.makeCurrent();
        fbo = new QGLFramebufferObject(512, 512);
        widget.doneCurrent();
    }
    ~FrameBufferObjectWrapper() { delete fbo; }
    QPaintDevice *realPaintDevice() { return fbo; }
    void prepareDevice() { widget.makeCurrent(); }
    void moveToThread(QThread *thread) { widget.context()->moveToThread(thread); }

    ThreadSafeGLWidget widget;
    QGLFramebufferObject *fbo;
};


class ThreadPainter : public QObject
{
    Q_OBJECT
public:
    ThreadPainter(Device *pd) : device(pd), fail(true) {
        pixmap = QPixmap(40, 40);
        pixmap.fill(Qt::green);
        QPainter p(&pixmap);
        p.drawLine(0, 0, 40, 40);
        p.drawLine(0, 40, 40, 0);
    }

public slots:
    void draw() {
        bool beginFailed = false;
        QTime time;
        time.start();
        int rotAngle = 10;
        device->prepareDevice();
        QPaintDevice *paintDevice = device->realPaintDevice();
        QSize s(paintDevice->width(), paintDevice->height());
        while (time.elapsed() < RUNNING_TIME) {
            QPainter p;
            if (!p.begin(paintDevice)) {
                beginFailed = true;
                break;
            }
            p.translate(s.width()/2, s.height()/2);
            p.rotate(rotAngle);
            p.translate(-s.width()/2, -s.height()/2);
            p.fillRect(0, 0, s.width(), s.height(), Qt::red);
            QRect rect(QPoint(0, 0), s);
            p.drawPixmap(10, 10, pixmap);
            p.drawTiledPixmap(50, 50, 100, 100, pixmap);
            p.drawText(rect.center(), "This is a piece of text");
            p.end();
            rotAngle += 2;
            QThread::msleep(20);
        }

        device->moveToThread(qApp->thread());

        fail = beginFailed;
        QThread::currentThread()->quit();
    }

    bool failed() { return fail; }

private:
    QPixmap pixmap;
    Device *device;
    bool fail;
};

template <class T>
class PaintThreadManager
{
public:
    PaintThreadManager(int count) : numThreads(count)
    {
        for (int i=0; i<numThreads; ++i)
            devices.append(new T);
        // Wait until resize events are processed on the internal
        // QGLWidgets of the buffers to suppress errors
        // about makeCurrent() from the wrong thread.
        QCoreApplication::processEvents();
        for (int i=0; i<numThreads; ++i) {
            devices.append(new T);
            threads.append(new QThread);
            painters.append(new ThreadPainter(devices.at(i)));
            painters.at(i)->moveToThread(threads.at(i));
            painters.at(i)->connect(threads.at(i), SIGNAL(started()), painters.at(i), SLOT(draw()));
            devices.at(i)->moveToThread(threads.at(i));
        }
    }

    ~PaintThreadManager() {
        qDeleteAll(threads);
        qDeleteAll(painters);
        qDeleteAll(devices);
    }


    void start() {
        for (int i=0; i<numThreads; ++i)
            threads.at(i)->start();
    }

    bool areRunning() {
        bool running = false;
        for (int i=0; i<numThreads; ++i){
            if (threads.at(i)->isRunning())
                running = true;
        }

        return running;
    }

    bool failed() {
        for (int i=0; i<numThreads; ++i) {
            if (painters.at(i)->failed())
                return true;
        }

        return false;
    }

private:
    QList<QThread *> threads;
    QList<Device *> devices;
    QList<ThreadPainter *> painters;
    int numThreads;
};

/*
   This test uses QPainter to draw onto different QGLWidgets in
   different threads at the same time. The ThreadSafeGLWidget is
   necessary to handle paint and resize events that might come from
   the main thread at any time while the test is running. The resize
   and paint events would cause makeCurrent() calls to be issued from
   within the QGLWidget while the widget's context was current in
   another thread, which would cause errors.
*/
void tst_QGLThreads::painterOnGLWidgetInThread()
{
    //QTBUG-46446 tst_qglthreads is unstable on windows 7
    if (QGuiApplication::platformName().compare("windows 7", Qt::CaseInsensitive))
        QSKIP("Doesn't work on this platform. QTBUG-46446");
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL))
        QSKIP("No platformsupport for ThreadedOpenGL");
    if (!((QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_0) ||
          (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_ES_Version_2_0))) {
        QSKIP("The OpenGL based threaded QPainter tests requires OpenGL/ES 2.0.");
    }

    PaintThreadManager<GLWidgetWrapper> painterThreads(5);
    painterThreads.start();

    while (painterThreads.areRunning()) {
        qApp->processEvents();
        QThread::msleep(100);
    }
    QVERIFY(!painterThreads.failed());
}

/*
   This test uses QPainter to draw onto different QPixmaps in
   different threads at the same time.
*/
void tst_QGLThreads::painterOnPixmapInThread()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL)
        || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedPixmaps))
        QSKIP("No platformsupport for ThreadedOpenGL or ThreadedPixmaps");
#if 0 // Used to be included in Qt4 for Q_WS_X11
    QSKIP("Drawing text in threads onto X11 drawables currently crashes on some X11 servers.");
#endif
    PaintThreadManager<PixmapWrapper> painterThreads(5);
    painterThreads.start();

    while (painterThreads.areRunning()) {
        qApp->processEvents();
        QThread::msleep(100);
    }
    QVERIFY(!painterThreads.failed());
}

/* This test uses QPainter to draw onto different QGLPixelBuffer
   objects in different threads at the same time.
*/
void tst_QGLThreads::painterOnPboInThread()
{
    //QTBUG-46446 tst_qglthreads is unstable on windows 7
    if (QGuiApplication::platformName().compare("windows 7", Qt::CaseInsensitive))
        QSKIP("Doesn't work on this platform. QTBUG-46446");
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL))
        QSKIP("No platformsupport for ThreadedOpenGL");
    if (!((QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_0) ||
          (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_ES_Version_2_0))) {
        QSKIP("The OpenGL based threaded QPainter tests requires OpenGL/ES 2.0.");
    }

    if (!QGLPixelBuffer::hasOpenGLPbuffers()) {
        QSKIP("This system doesn't support pbuffers.");
    }

    PaintThreadManager<PixelBufferWrapper> painterThreads(5);
    painterThreads.start();

    while (painterThreads.areRunning()) {
        qApp->processEvents();
        QThread::msleep(100);
    }
    QVERIFY(!painterThreads.failed());
}

/* This test uses QPainter to draw onto different
   QGLFramebufferObjects (bound in a QGLWidget's context) in different
   threads at the same time.
*/
void tst_QGLThreads::painterOnFboInThread()
{
    //QTBUG-46446 tst_qglthreads is unstable on windows 7
    if (QGuiApplication::platformName().compare("windows 7", Qt::CaseInsensitive))
        QSKIP("Doesn't work on this platform. QTBUG-46446");
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL))
        QSKIP("No platformsupport for ThreadedOpenGL");
    if (!((QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_0) ||
          (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_ES_Version_2_0))) {
        QSKIP("The OpenGL based threaded QPainter tests requires OpenGL/ES 2.0.");
    }

    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects()) {
        QSKIP("This system doesn't support framebuffer objects.");
    }

    PaintThreadManager<FrameBufferObjectWrapper> painterThreads(5);
    painterThreads.start();

    while (painterThreads.areRunning()) {
        qApp->processEvents();
        QThread::msleep(100);
    }
    QVERIFY(!painterThreads.failed());
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTEST_DISABLE_KEYPAD_NAVIGATION \

    tst_QGLThreads tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_qglthreads.moc"
