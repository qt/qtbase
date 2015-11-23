/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets/QOpenGLWidget>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtTest/QtTest>
#include <QSignalSpy>

class tst_QOpenGLWidget : public QObject
{
    Q_OBJECT

private slots:
    void create();
    void clearAndGrab();
    void clearAndResizeAndGrab();
    void createNonTopLevel();
    void painter();
    void reparentToAlreadyCreated();
    void reparentToNotYetCreated();
    void asViewport();
    void requestUpdate();
    void fboRedirect();
    void showHide();
    void nativeWindow();
};

void tst_QOpenGLWidget::create()
{
    QScopedPointer<QOpenGLWidget> w(new QOpenGLWidget);
    QVERIFY(!w->isValid());
    QSignalSpy frameSwappedSpy(w.data(), SIGNAL(frameSwapped()));
    w->show();
    QTest::qWaitForWindowExposed(w.data());
    QVERIFY(frameSwappedSpy.count() > 0);

    QVERIFY(w->isValid());
    QVERIFY(w->context());
    QCOMPARE(w->context()->format(), w->format());
    QVERIFY(w->defaultFramebufferObject() != 0);
}

class ClearWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    ClearWidget(QWidget *parent, int expectedWidth, int expectedHeight)
        : QOpenGLWidget(parent),
          m_initCalled(false), m_paintCalled(false), m_resizeCalled(false),
          m_resizeOk(false),
          m_w(expectedWidth), m_h(expectedHeight),
          r(1.0f), g(0.0f), b(0.0f) { }

    void initializeGL() Q_DECL_OVERRIDE {
        m_initCalled = true;
        initializeOpenGLFunctions();
    }
    void paintGL() Q_DECL_OVERRIDE {
        m_paintCalled = true;
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    void resizeGL(int w, int h) Q_DECL_OVERRIDE {
        m_resizeCalled = true;
        m_resizeOk = w == m_w && h == m_h;
    }
    void setClearColor(float r, float g, float b) {
        this->r = r; this->g = g; this->b = b;
    }

    bool m_initCalled;
    bool m_paintCalled;
    bool m_resizeCalled;
    bool m_resizeOk;
    int m_w;
    int m_h;
    float r, g, b;
};

void tst_QOpenGLWidget::clearAndGrab()
{
    QScopedPointer<ClearWidget> w(new ClearWidget(0, 800, 600));
    w->resize(800, 600);
    w->show();
    QTest::qWaitForWindowExposed(w.data());
    QVERIFY(w->m_initCalled);
    QVERIFY(w->m_resizeCalled);
    QVERIFY(w->m_resizeOk);
    QVERIFY(w->m_paintCalled);

    QImage image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(255, 0, 0));
}

void tst_QOpenGLWidget::clearAndResizeAndGrab()
{
    QScopedPointer<QOpenGLWidget> w(new ClearWidget(0, 640, 480));
    w->resize(640, 480);
    w->show();
    QTest::qWaitForWindowExposed(w.data());

    QImage image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    w->resize(800, 600);
    image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 800);
    QCOMPARE(image.height(), 600);
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(255, 0, 0));
}

void tst_QOpenGLWidget::createNonTopLevel()
{
    QWidget w;
    ClearWidget *glw = new ClearWidget(&w, 600, 700);
    QSignalSpy frameSwappedSpy(glw, SIGNAL(frameSwapped()));
    w.resize(400, 400);
    w.show();
    QTest::qWaitForWindowExposed(&w);
    QVERIFY(frameSwappedSpy.count() > 0);

    QVERIFY(glw->m_resizeCalled);
    glw->m_resizeCalled = false;
    QVERIFY(!glw->m_resizeOk);
    glw->resize(600, 700);

    QVERIFY(glw->m_initCalled);
    QVERIFY(glw->m_resizeCalled);
    QVERIFY(glw->m_resizeOk);
    QVERIFY(glw->m_paintCalled);

    QImage image = glw->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), glw->width());
    QCOMPARE(image.height(), glw->height());
    QVERIFY(image.pixel(30, 40) == qRgb(255, 0, 0));

    glw->doneCurrent();
    QVERIFY(!QOpenGLContext::currentContext());
    glw->makeCurrent();
    QVERIFY(QOpenGLContext::currentContext() == glw->context() && glw->context());
}

class PainterWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    PainterWidget(QWidget *parent)
        : QOpenGLWidget(parent), m_clear(false) { }

    void initializeGL() Q_DECL_OVERRIDE {
        initializeOpenGLFunctions();
    }
    void paintGL() Q_DECL_OVERRIDE {
        QPainter p(this);
        QCOMPARE(p.device()->width(), width());
        QCOMPARE(p.device()->height(), height());
        p.fillRect(QRect(QPoint(0, 0), QSize(width(), height())), Qt::blue);
        if (m_clear) {
            p.beginNativePainting();
            glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            p.endNativePainting();
        }
    }
    bool m_clear;
};

void tst_QOpenGLWidget::painter()
{
    QWidget w;
    PainterWidget *glw = new PainterWidget(&w);
    w.resize(640, 480);
    glw->resize(320, 200);
    w.show();
    QTest::qWaitForWindowExposed(&w);

    QImage image = glw->grabFramebuffer();
    QCOMPARE(image.width(), glw->width());
    QCOMPARE(image.height(), glw->height());
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));

    glw->m_clear = true;
    image = glw->grabFramebuffer();
    QVERIFY(image.pixel(20, 10) == qRgb(0, 255, 0));
}

void tst_QOpenGLWidget::reparentToAlreadyCreated()
{
    QWidget w1;
    PainterWidget *glw = new PainterWidget(&w1);
    w1.resize(640, 480);
    glw->resize(320, 200);
    w1.show();
    QTest::qWaitForWindowExposed(&w1);

    QWidget w2;
    w2.show();
    QTest::qWaitForWindowExposed(&w2);

    glw->setParent(&w2);
    glw->show();

    QImage image = glw->grabFramebuffer();
    QCOMPARE(image.width(), 320);
    QCOMPARE(image.height(), 200);
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));
}

void tst_QOpenGLWidget::reparentToNotYetCreated()
{
    QWidget w1;
    PainterWidget *glw = new PainterWidget(&w1);
    w1.resize(640, 480);
    glw->resize(320, 200);
    w1.show();
    QTest::qWaitForWindowExposed(&w1);

    QWidget w2;
    glw->setParent(&w2);
    w2.show();
    QTest::qWaitForWindowExposed(&w2);

    QImage image = glw->grabFramebuffer();
    QCOMPARE(image.width(), 320);
    QCOMPARE(image.height(), 200);
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));
}

class CountingGraphicsView : public QGraphicsView
{
public:
    CountingGraphicsView(): m_count(0) { }
    int paintCount() const { return m_count; }
    void resetPaintCount() { m_count = 0; }

protected:
    void drawForeground(QPainter *, const QRectF &) Q_DECL_OVERRIDE;
    int m_count;
};

void CountingGraphicsView::drawForeground(QPainter *, const QRectF &)
{
    ++m_count;
}

void tst_QOpenGLWidget::asViewport()
{
    // Have a QGraphicsView with a QOpenGLWidget as its viewport.
    QGraphicsScene scene;
    scene.addItem(new QGraphicsRectItem(10, 10, 100, 100));
    CountingGraphicsView *view = new CountingGraphicsView;
    view->setScene(&scene);
    view->setViewport(new QOpenGLWidget);
    QWidget widget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(view);
    QPushButton *btn = new QPushButton("Test");
    layout->addWidget(btn);
    widget.setLayout(layout);
    widget.show();
    QTest::qWaitForWindowExposed(&widget);

    QVERIFY(view->paintCount() > 0);
    view->resetPaintCount();

    // And now trigger a repaint on the push button. We must not
    // receive paint events for the graphics view. If we do, that's a
    // side effect of QOpenGLWidget's special behavior and handling in
    // the widget stack.
    btn->update();
    qApp->processEvents();
    QCOMPARE(view->paintCount(), 0);
}

class PaintCountWidget : public QOpenGLWidget
{
public:
    PaintCountWidget() : m_count(0) { }
    void reset() { m_count = 0; }
    void paintGL() Q_DECL_OVERRIDE { ++m_count; }
    int m_count;
};

void tst_QOpenGLWidget::requestUpdate()
{
    PaintCountWidget w;
    w.resize(640, 480);
    w.show();
    QTest::qWaitForWindowExposed(&w);

    w.reset();
    QCOMPARE(w.m_count, 0);

    w.windowHandle()->requestUpdate();
    QTRY_VERIFY(w.m_count > 0);
}

class FboCheckWidget : public QOpenGLWidget
{
public:
    void paintGL() Q_DECL_OVERRIDE {
        GLuint reportedDefaultFbo = QOpenGLContext::currentContext()->defaultFramebufferObject();
        GLuint expectedDefaultFbo = defaultFramebufferObject();
        QCOMPARE(reportedDefaultFbo, expectedDefaultFbo);
    }
};

void tst_QOpenGLWidget::fboRedirect()
{
    FboCheckWidget w;
    w.resize(640, 480);
    w.show();
    QTest::qWaitForWindowExposed(&w);

    // Unlike in paintGL(), the default fbo reported by the context is not affected by the widget,
    // so we get the real default fbo: either 0 or (on iOS) the fbo associated with the window.
    w.makeCurrent();
    GLuint reportedDefaultFbo = QOpenGLContext::currentContext()->defaultFramebufferObject();
    GLuint widgetFbo = w.defaultFramebufferObject();
    QVERIFY(reportedDefaultFbo != widgetFbo);
}

void tst_QOpenGLWidget::showHide()
{
    QScopedPointer<ClearWidget> w(new ClearWidget(0, 800, 600));
    w->resize(800, 600);
    w->show();
    QTest::qWaitForWindowExposed(w.data());

    w->hide();

    QImage image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(255, 0, 0));

    w->setClearColor(0, 0, 1);
    w->show();
    QTest::qWaitForWindowExposed(w.data());

    image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(0, 0, 255));
}

void tst_QOpenGLWidget::nativeWindow()
{
    QScopedPointer<ClearWidget> w(new ClearWidget(0, 800, 600));
    w->resize(800, 600);
    w->show();
    w->winId();
    QTest::qWaitForWindowExposed(w.data());

    QImage image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(255, 0, 0));
    QVERIFY(w->internalWinId());

    // Now as a native child.
    QWidget nativeParent;
    nativeParent.resize(800, 600);
    nativeParent.setAttribute(Qt::WA_NativeWindow);
    ClearWidget *child = new ClearWidget(0, 800, 600);
    child->setClearColor(0, 1, 0);
    child->setParent(&nativeParent);
    child->resize(400, 400);
    child->move(23, 34);
    nativeParent.show();
    QTest::qWaitForWindowExposed(&nativeParent);

    QVERIFY(nativeParent.internalWinId());
    QVERIFY(!child->internalWinId());

    image = child->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), child->width());
    QCOMPARE(image.height(), child->height());
    QVERIFY(image.pixel(30, 40) == qRgb(0, 255, 0));
}

QTEST_MAIN(tst_QOpenGLWidget)

#include "tst_qopenglwidget.moc"
