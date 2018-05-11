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

#include <QtGui/QOpenGLWindow>
#include <QtTest/QtTest>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLContext>
#include <QtGui/QPainter>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

class tst_QOpenGLWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void create();
    void basic();
    void resize();
    void painter();
    void partial_data();
    void partial();
    void underOver();
};

void tst_QOpenGLWindow::initTestCase()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
        QSKIP("OpenGL is not supported on this platform.");
}

void tst_QOpenGLWindow::create()
{
    QOpenGLWindow w;
    QVERIFY(!w.isValid());

    w.resize(640, 480);
    w.show();

    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QVERIFY(w.isValid());
}

class Window : public QOpenGLWindow
{
public:
    void reset() {
        initCount = resizeCount = paintCount = 0;
    }

    void initializeGL() override {
        ++initCount;
    }

    void resizeGL(int w, int h) override {
        ++resizeCount;
        QCOMPARE(w, size().width());
        QCOMPARE(h, size().height());
    }

    void paintGL() override {
        ++paintCount;

        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        QVERIFY(ctx);
        QCOMPARE(ctx, context());
        QOpenGLFunctions *f = ctx->functions();
        QVERIFY(f);

        f->glClearColor(1, 0, 0, 1);
        f->glClear(GL_COLOR_BUFFER_BIT);

        img = grabFramebuffer();
    }

    int initCount;
    int resizeCount;
    int paintCount;
    QImage img;
};

void tst_QOpenGLWindow::basic()
{
    Window w;
    w.reset();
    w.resize(640, 480);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    // Check that the virtuals are invoked.
    QCOMPARE(w.initCount, 1);
    QVERIFY(w.resizeCount >= 1);
    QVERIFY(w.paintCount >= 1);

    // Check that something has been drawn;
    QCOMPARE(w.img.size(), w.size() * w.devicePixelRatio());
    QVERIFY(w.img.pixel(QPoint(5, 5) * w.devicePixelRatio()) == qRgb(255, 0, 0));

    // Check that the viewport was properly set.
    w.makeCurrent();
    GLint v[4] = { 0, 0, 0, 0 };
    w.context()->functions()->glGetIntegerv(GL_VIEWPORT, v);
    QCOMPARE(v[0], 0);
    QCOMPARE(v[1], 0);
    QCOMPARE(v[2], GLint(w.width() * w.devicePixelRatio()));
    QCOMPARE(v[3], GLint(w.height() * w.devicePixelRatio()));
    w.doneCurrent();
}

static bool isPlatformWayland()
{
    return QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive);
}

void tst_QOpenGLWindow::resize()
{
    if (isPlatformWayland())
        QSKIP("Wayland: Crashes on Intel Mesa due to a driver bug (QTBUG-66848).");

    Window w;
    w.reset();
    w.resize(640, 480);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    // Check that the virtuals are invoked.
    int resCount = w.resizeCount;
    QVERIFY(resCount >= 1);

    // Check that a future resize triggers resizeGL.
    w.resize(800, 600);
    QTRY_VERIFY(w.resizeCount > resCount);
}

class PainterWindow : public QOpenGLWindow
{
public:
    void paintGL() override {
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        QVERIFY(ctx);
        QCOMPARE(ctx, context());
        QOpenGLFunctions *f = ctx->functions();
        QVERIFY(f);

        QPainter p(this);
        p.beginNativePainting();
        f->glClearColor(1, 0, 0, 1);
        f->glClear(GL_COLOR_BUFFER_BIT);
        p.endNativePainting();
        p.fillRect(QRect(0, 0, 100, 100), Qt::blue);
        p.end();

        img = grabFramebuffer();
    }

    QImage img;
};

void tst_QOpenGLWindow::painter()
{
    PainterWindow w;
    w.resize(400, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QCOMPARE(w.img.size(), w.size() * w.devicePixelRatio());
    QVERIFY(w.img.pixel(QPoint(5, 5) * w.devicePixelRatio()) == qRgb(0, 0, 255));
    QVERIFY(w.img.pixel(QPoint(200, 5) * w.devicePixelRatio()) == qRgb(255, 0, 0));
}

class PartialPainterWindow : public QOpenGLWindow
{
public:
    PartialPainterWindow(QOpenGLWindow::UpdateBehavior u)
        : QOpenGLWindow(u), x(0) { }

    void paintGL() override {
        ++paintCount;

        QPainter p(this);
        if (!x)
            p.fillRect(QRect(0, 0, width(), height()), Qt::green);

        p.fillRect(QRect(x, 0, 10, 10), Qt::blue);
        x += 20;
    }

    int paintCount;
    int x;
};

void tst_QOpenGLWindow::partial_data()
{
    QTest::addColumn<int>("behavior");
    QTest::newRow("blit") << int(QOpenGLWindow::PartialUpdateBlit);
    QTest::newRow("blend") << int(QOpenGLWindow::PartialUpdateBlend);
}

void tst_QOpenGLWindow::partial()
{
    QFETCH(int, behavior);
    QOpenGLWindow::UpdateBehavior u = QOpenGLWindow::UpdateBehavior(behavior);
    PartialPainterWindow w(u);
    w.resize(800, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    // Add a couple of small blue rects.
    for (int i = 0; i < 10; ++i) {
        w.paintCount = 0;
        w.update();
        QTRY_VERIFY(w.paintCount > 0);
    }

    // Now since the painting went to an extra framebuffer, all the rects should
    // be present since everything is preserved between the frames.
    QImage img = w.grabFramebuffer();
    QCOMPARE(img.size(), w.size() * w.devicePixelRatio());
    QCOMPARE(img.pixel(QPoint(5, 5) * w.devicePixelRatio()), qRgb(0, 0, 255));
    QCOMPARE(img.pixel(QPoint(15, 5) * w.devicePixelRatio()), qRgb(0, 255, 0));
    QCOMPARE(img.pixel(QPoint(25, 5) * w.devicePixelRatio()), qRgb(0, 0, 255));
}

class PaintUnderOverWindow : public QOpenGLWindow
{
public:
    PaintUnderOverWindow() : QOpenGLWindow(PartialUpdateBlend), m_state(None) { }
    enum State {
        None,
        PaintUnder,
        Paint,
        PaintOver,
        Error
    } m_state;

    void paintUnderGL() override {
        if (m_state == None || m_state == PaintOver)
            m_state = PaintUnder;
        else
            m_state = Error;

        GLuint fbo = 0xFFFF;
        QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &fbo);
        QCOMPARE(fbo, QOpenGLContext::currentContext()->defaultFramebufferObject());
    }

    void paintGL() override {
        if (m_state == PaintUnder)
            m_state = Paint;
        else
            m_state = Error;

        // Using PartialUpdateBlend so paintGL() targets a user fbo, not the default.
        GLuint fbo = 0xFFFF;
        QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &fbo);
        QVERIFY(fbo != QOpenGLContext::currentContext()->defaultFramebufferObject());
        QCOMPARE(fbo, defaultFramebufferObject());
    }

    void paintOverGL() override {
        if (m_state == Paint)
            m_state = PaintOver;
        else
            m_state = Error;

        GLuint fbo = 0xFFFF;
        QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &fbo);
        QCOMPARE(fbo, QOpenGLContext::currentContext()->defaultFramebufferObject());
    }
};

void tst_QOpenGLWindow::underOver()
{
    PaintUnderOverWindow w;
    w.resize(400, 400);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    // under -> paint -> over -> under -> paint -> ... is the only acceptable sequence
    QCOMPARE(w.m_state, PaintUnderOverWindow::PaintOver);
}

#include <tst_qopenglwindow.moc>

QTEST_MAIN(tst_QOpenGLWindow)
