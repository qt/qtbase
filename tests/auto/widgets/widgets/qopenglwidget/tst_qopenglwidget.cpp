// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QPainter>
#include <QtGui/QBackingStore>
#include <QtGui/QScreen>
#include <QtGui/QStaticText>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QTest>
#include <QSignalSpy>
#include <private/qguiapplication_p.h>
#include <private/qstatictext_p.h>
#include <private/qopengltextureglyphcache_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformintegration.h>
#include <rhi/qrhi.h>

class tst_QOpenGLWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void create();
    void clearAndGrab();
    void clearAndResizeAndGrab();
    void createNonTopLevel();
#if QT_CONFIG(egl)
    void deviceLoss();
#endif
    void painter();
    void reparentToAlreadyCreated();
    void reparentToNotYetCreated();
    void reparentHidden();
    void reparentTopLevel();
    void asViewport();
    void requestUpdate();
    void fboRedirect();
    void showHide();
    void nativeWindow();
    void stackWidgetOpaqueChildIsVisible();
    void offscreen();
    void offscreenThenOnscreen();
    void paintWhileHidden();
    void widgetWindowColorFormat_data();
    void widgetWindowColorFormat();

#ifdef QT_BUILD_INTERNAL
    void staticTextDanglingPointer();
#endif
};

void tst_QOpenGLWidget::initTestCase()
{
    // See QOpenGLWidget constructor
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RasterGLSurface))
        QSKIP("QOpenGLWidget is not supported on this platform.");
}

void tst_QOpenGLWidget::create()
{
    QScopedPointer<QOpenGLWidget> w(new QOpenGLWidget);
    QVERIFY(!w->isValid());
    QVERIFY(w->textureFormat() == 0);
    QSignalSpy frameSwappedSpy(w.data(), SIGNAL(frameSwapped()));
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w.data()));
    QVERIFY(frameSwappedSpy.size() > 0);

    QVERIFY(w->isValid());
    QVERIFY(w->context());
    QCOMPARE(w->context()->format(), w->format());
    QVERIFY(w->defaultFramebufferObject() != 0);
    QVERIFY(w->textureFormat() != 0);
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

    void initializeGL() override {
        m_initCalled = true;
        initializeOpenGLFunctions();
    }
    void paintGL() override {
        m_paintCalled = true;
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    void resizeGL(int w, int h) override {
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
    QVERIFY(QTest::qWaitForWindowExposed(w.data()));
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
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif
    QScopedPointer<QOpenGLWidget> w(new ClearWidget(0, 640, 480));
    w->resize(640, 480);
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w.data()));

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
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QVERIFY(frameSwappedSpy.size() > 0);

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

#if QT_CONFIG(egl)
void tst_QOpenGLWidget::deviceLoss()
{
    QScopedPointer<QOpenGLWidget> w(new ClearWidget(0, 640, 480));

    w->resize(640, 480);
    w->show();

    auto rhi = w->backingStore()->handle()->rhi();
    QNativeInterface::QEGLContext *rhiContext = nullptr;
    if (rhi->backend() == QRhi::OpenGLES2) {
        auto rhiHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
        rhiContext = rhiHandles->context->nativeInterface<QNativeInterface::QEGLContext>();
    }
    if (!rhiContext)
        QSKIP("deviceLoss needs EGL");

    QVERIFY(QTest::qWaitForWindowExposed(w.data()));

    QImage image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(255, 0, 0));

    rhiContext->invalidateContext();

    w->resize(600, 600);
    QSignalSpy frameSwappedSpy(w.get(), &QOpenGLWidget::resized);
    QTRY_VERIFY(frameSwappedSpy.size() > 0);

    image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(255, 0, 0));
}
#endif

class PainterWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
    PainterWidget(QWidget *parent = nullptr)
        : QOpenGLWidget(parent), m_clear(false) { }

    void initializeGL() override {
        initializeOpenGLFunctions();
    }
    void paintGL() override {
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
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QImage image = glw->grabFramebuffer();
    QCOMPARE(image.width(), glw->width());
    QCOMPARE(image.height(), glw->height());
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));

    glw->m_clear = true;
    image = glw->grabFramebuffer();
    QVERIFY(image.pixel(20, 10) == qRgb(0, 255, 0));

    QPixmap pix = glw->grab(); // QTBUG-61036
}

void tst_QOpenGLWidget::reparentToAlreadyCreated()
{
    QWidget w1;
    PainterWidget *glw = new PainterWidget(&w1);
    w1.resize(640, 480);
    glw->resize(320, 200);
    w1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w1));

    QWidget w2;
    w2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w2));

    glw->setParent(&w2);
    glw->show();

    QImage image = glw->grabFramebuffer();
    QCOMPARE(image.width(), 320);
    QCOMPARE(image.height(), 200);
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));
}

void tst_QOpenGLWidget::reparentToNotYetCreated()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif
    QWidget w1;
    PainterWidget *glw = new PainterWidget(&w1);
    w1.resize(640, 480);
    glw->resize(320, 200);
    w1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w1));

    QWidget w2;
    glw->setParent(&w2);
    w2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w2));

    QImage image = glw->grabFramebuffer();
    QCOMPARE(image.width(), 320);
    QCOMPARE(image.height(), 200);
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));
}

void tst_QOpenGLWidget::reparentHidden()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-111235)");
#endif
    // Tests QTBUG-60896
    QWidget topLevel1;

    QWidget *container = new QWidget(&topLevel1);
    PainterWidget *glw = new PainterWidget(container);
    topLevel1.resize(640, 480);
    glw->resize(320, 200);
    topLevel1.show();

    glw->hide(); // Explicitly hidden

    QVERIFY(QTest::qWaitForWindowExposed(&topLevel1));

    QWidget topLevel2;
    topLevel2.resize(640, 480);
    topLevel2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel2));

    QOpenGLContext *originalContext = glw->context();
    QVERIFY(originalContext);

    container->setParent(&topLevel2);
    glw->show(); // Should get a new context now

    QOpenGLContext *newContext = glw->context();
    QVERIFY(originalContext != newContext);
}

void tst_QOpenGLWidget::reparentTopLevel()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif
    // no GL content yet, just an ordinary tab widget, top-level
    QTabWidget tabWidget;
    tabWidget.resize(640, 480);
    tabWidget.addTab(new QLabel("Dummy page"), "Page 1");
    tabWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tabWidget));

    PainterWidget *glw1 = new PainterWidget;
    // add child GL widget as a tab page
    {
        QSignalSpy frameSwappedSpy(glw1, &QOpenGLWidget::frameSwapped);
        tabWidget.setCurrentIndex(tabWidget.addTab(glw1, "OpenGL widget 1"));
        QTRY_VERIFY(frameSwappedSpy.size() > 0);
    }

    PainterWidget *glw2 = new PainterWidget;
    // add child GL widget #2 as a tab page
    {
        QSignalSpy frameSwappedSpy(glw2, &QOpenGLWidget::frameSwapped);
        tabWidget.setCurrentIndex(tabWidget.addTab(glw2, "OpenGL widget 2"));
        QTRY_VERIFY(frameSwappedSpy.size() > 0);
    }

    QImage image = glw2->grabFramebuffer();
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));

    // now delete GL widget #2
    {
        QSignalSpy frameSwappedSpy(glw1, &QOpenGLWidget::frameSwapped);
        delete glw2;
        QTRY_VERIFY(frameSwappedSpy.size() > 0);
    }

    image = glw1->grabFramebuffer();
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));

    // make the GL widget top-level
    {
        QSignalSpy frameSwappedSpy(glw1, &QOpenGLWidget::frameSwapped);
        glw1->setParent(nullptr);
        glw1->show();
        QVERIFY(QTest::qWaitForWindowExposed(glw1));
        QTRY_VERIFY(frameSwappedSpy.size() > 0);
    }

    image = glw1->grabFramebuffer();
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));

    // back to a child widget by readding to the tab widget
    {
        QSignalSpy frameSwappedSpy(glw1, &QOpenGLWidget::frameSwapped);
        tabWidget.setCurrentIndex(tabWidget.addTab(glw1, "Re-added OpenGL widget 1"));
        QTRY_VERIFY(frameSwappedSpy.size() > 0);
    }

    image = glw1->grabFramebuffer();
    QVERIFY(image.pixel(20, 10) == qRgb(0, 0, 255));
}

class CountingGraphicsView : public QGraphicsView
{
public:
    CountingGraphicsView(): m_count(0) { }
    int paintCount() const { return m_count; }
    void resetPaintCount() { m_count = 0; }

protected:
    void drawForeground(QPainter *, const QRectF &) override;
    int m_count;
};

void CountingGraphicsView::drawForeground(QPainter *, const QRectF &)
{
    ++m_count;

    // QTBUG-59318: verify that the context's internal default fbo redirection
    // is active also when using the QOpenGLWidget as a viewport.
    GLint currentFbo = -1;
    QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFbo);
    GLuint defFbo = QOpenGLContext::currentContext()->defaultFramebufferObject();
    QCOMPARE(GLuint(currentFbo), defFbo);
}

void tst_QOpenGLWidget::asViewport()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif
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
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation)) {
        // On some platforms (macOS), the palette will be different depending on if a
        // window is active or not. And because of that, the whole window will be
        // repainted when going from Inactive to Active. So wait for the window to be
        // active before we continue, so the activation doesn't happen at a random
        // time below. And call processEvents to have the paint events delivered right away.
        widget.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&widget));
        qApp->processEvents();
    }

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
    void paintGL() override { ++m_count; }
    int m_count;
};

void tst_QOpenGLWidget::requestUpdate()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif

    PaintCountWidget w;
    w.resize(640, 480);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    w.reset();
    QCOMPARE(w.m_count, 0);

    w.windowHandle()->requestUpdate();
    QTRY_VERIFY(w.m_count > 0);
}

class FboCheckWidget : public QOpenGLWidget
{
public:
    void paintGL() override {
        GLuint reportedDefaultFbo = QOpenGLContext::currentContext()->defaultFramebufferObject();
        GLuint expectedDefaultFbo = defaultFramebufferObject();
        QCOMPARE(reportedDefaultFbo, expectedDefaultFbo);
    }
};

void tst_QOpenGLWidget::fboRedirect()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif
    FboCheckWidget w;
    w.resize(640, 480);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    // Unlike in paintGL(), the default fbo reported by the context is not affected by the widget,
    // so we get the real default fbo: either 0 or (on iOS) the fbo associated with the window.
    w.makeCurrent();
    GLuint reportedDefaultFbo = QOpenGLContext::currentContext()->defaultFramebufferObject();
    GLuint widgetFbo = w.defaultFramebufferObject();
    QVERIFY(reportedDefaultFbo != widgetFbo);
}

void tst_QOpenGLWidget::showHide()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif
    QScopedPointer<ClearWidget> w(new ClearWidget(0, 800, 600));
    w->resize(800, 600);
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w.data()));

    w->hide();

    QImage image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(255, 0, 0));

    w->setClearColor(0, 0, 1);
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w.data()));

    image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(0, 0, 255));
}

QtMessageHandler oldHandler = nullptr;

void nativeWindowMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (oldHandler)
        oldHandler(type, context, msg);

    if (type == QtWarningMsg
        && (msg.contains("QOpenGLContext::makeCurrent() called with non-opengl surface")
            || msg.contains("Failed to make context current")))
    {
        QFAIL("Unexpected warning got printed");
    }
}

void tst_QOpenGLWidget::nativeWindow()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif

    // NB these tests do not fully verify that native child widgets are fully
    // functional since there is no guarantee that the content is composed and
    // presented correctly as we can only do verification with
    // grabFramebuffer() here which only exercises a part of the pipeline.

    // Install a message handler that looks for some typical warnings from
    // QRhi/QOpenGLConext that occur when the RHI-related logic in widgets goes wrong.
    oldHandler = qInstallMessageHandler(nativeWindowMessageHandler);

    {
        QScopedPointer<ClearWidget> w(new ClearWidget(nullptr, 800, 600));
        w->resize(800, 600);
        w->show();
        w->winId();
        QVERIFY(QTest::qWaitForWindowExposed(w.data()));

        QImage image = w->grabFramebuffer();
        QVERIFY(!image.isNull());
        QCOMPARE(image.width(), w->width());
        QCOMPARE(image.height(), w->height());
        QVERIFY(image.pixel(30, 40) == qRgb(255, 0, 0));
        QVERIFY(w->internalWinId());
    }

    // QTBUG-113557: a plain _raster_ QWidget that is a _native_ child in a toplevel
    // combined with a RHI-based (non-native) widget (QOpenGLWidget in this case)
    // in the same toplevel.
    {
        QWidget topLevel;
        topLevel.resize(800, 600);

        ClearWidget *child = new ClearWidget(&topLevel, 800, 600);
        child->setClearColor(1, 0, 0);
        child->resize(400, 400);
        child->move(23, 34);

        QWidget *raster = new QWidget(&topLevel);
        raster->setGeometry(23, 240, 120, 120);
        raster->setStyleSheet("QWidget { background-color: yellow; }");

        raster->winId();

        topLevel.show();
        QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

        // Do not bother checking the output, i.e. if the yellow raster native child
        // shows up as it should, but rather rely on the message handler catching the
        // qWarnings if they occur.
    }

    // Now with the QOpenGLWidget being a native child
    {
        QWidget topLevel;
        topLevel.resize(800, 600);

        ClearWidget *child = new ClearWidget(nullptr, 800, 600);
        child->setParent(&topLevel);

        // make it a native child (native window, but not top-level -> no backingstore)
        child->winId();

        child->setClearColor(0, 1, 0);
        child->resize(400, 400);
        child->move(23, 34);

        topLevel.show();
        QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

        QVERIFY(topLevel.internalWinId());
        QVERIFY(child->internalWinId());

        QImage image = child->grabFramebuffer();
        QVERIFY(!image.isNull());
        QCOMPARE(image.width(), child->width());
        QCOMPARE(image.height(), child->height());
        QVERIFY(image.pixel(30, 40) == qRgb(0, 255, 0));
    }

    // Now the same with WA_NativeWindow instead
    {
        QWidget topLevel;
        topLevel.resize(800, 600);

        ClearWidget *child = new ClearWidget(nullptr, 800, 600);
        child->setParent(&topLevel);

        // make it a native child (native window, but not top-level -> no backingstore)
        child->setAttribute(Qt::WA_NativeWindow);

        child->setClearColor(0, 1, 0);
        child->resize(400, 400);
        child->move(23, 34);

        topLevel.show();
        QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

        QVERIFY(child->internalWinId());

        QImage image = child->grabFramebuffer();
        QCOMPARE(image.width(), child->width());
        QCOMPARE(image.height(), child->height());
        QVERIFY(image.pixel(30, 40) == qRgb(0, 255, 0));
    }

    // Now as a child of a native child
    {
        QWidget topLevel;
        topLevel.resize(800, 600);

        QWidget *container = new QWidget(&topLevel);
        // make it a native child (native window, but not top-level -> no backingstore)
        container->winId();

        ClearWidget *child = new ClearWidget(nullptr, 800, 600);
        // set the parent separately, this is important, see next test case
        child->setParent(container);
        child->setClearColor(0, 0, 1);
        child->resize(400, 400);
        child->move(23, 34);

        topLevel.show();
        QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

        QVERIFY(topLevel.internalWinId());
        QVERIFY(container->internalWinId());
        QVERIFY(!child->internalWinId());

        QImage image = child->grabFramebuffer();
        QCOMPARE(image.width(), child->width());
        QCOMPARE(image.height(), child->height());
        QVERIFY(image.pixel(30, 40) == qRgb(0, 0, 255));
    }

    // Again as a child of a native child, but this time specifying the parent
    // upon construction, not with an explicit setParent() call afterwards.
    {
        QWidget topLevel;
        topLevel.resize(800, 600);
        QWidget *container = new QWidget(&topLevel);
        container->winId();
        // parent it right away
        ClearWidget *child = new ClearWidget(container, 800, 600);
        child->setClearColor(0, 0, 1);
        child->resize(400, 400);
        child->move(23, 34);
        topLevel.show();
        QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
        QVERIFY(topLevel.internalWinId());
        QVERIFY(container->internalWinId());
        QVERIFY(!child->internalWinId());
        QImage image = child->grabFramebuffer();
        QCOMPARE(image.width(), child->width());
        QCOMPARE(image.height(), child->height());
        QVERIFY(image.pixel(30, 40) == qRgb(0, 0, 255));
    }

    if (oldHandler) {
        qInstallMessageHandler(oldHandler);
        oldHandler = nullptr;
    }
}

static inline QString msgRgbMismatch(unsigned actual, unsigned expected)
{
    return QString::asprintf("Color mismatch, %#010x != %#010x", actual, expected);
}

static QPixmap grabWidgetWithoutRepaint(const QWidget *widget, QRect clipArea)
{
    const QWindow *window = widget->window()->windowHandle();
    Q_ASSERT(window);
    WId windowId = window->winId();

#ifdef Q_OS_WIN
    // OpenGL content is not properly grabbed on Windows when passing a top level widget window,
    // because GDI functions can't grab OpenGL layer content.
    // Instead the whole screen should be captured, with an adjusted clip area, which contains
    // the final composited content.
    windowId = 0;
    clipArea = QRect(widget->mapToGlobal(clipArea.topLeft()),
                     widget->mapToGlobal(clipArea.bottomRight()));
#endif
    QScreen *screen = window->screen();
    Q_ASSERT(screen);

    const QSize size = clipArea.size();
    const QPixmap result = screen->grabWindow(windowId,
                                              clipArea.x(),
                                              clipArea.y(),
                                              size.width(),
                                              size.height());
    return result;
}

#define VERIFY_COLOR(child, region, color) verifyColor(child, region, color, __LINE__)

bool verifyColor(const QWidget *widget, const QRect &clipArea, const QColor &color, int callerLine)
{
    // Create a comparison target image
    QPixmap expectedPixmap(grabWidgetWithoutRepaint(widget, clipArea)); /* ensure equal formats */
    expectedPixmap.detach();
    expectedPixmap.fill(color);
    const QImage expectedImage = expectedPixmap.toImage();

    // test image size
    QPixmap pixmap;
    auto testSize = [&](){
        pixmap = grabWidgetWithoutRepaint(widget, clipArea);
        return pixmap.size() == clipArea.size();
    };

    // test the first pixel's color
    uint firstPixel;
    auto testPixel = [&](){
        const QImage image = grabWidgetWithoutRepaint(widget, clipArea).toImage();
        uint alphaCorrection = image.format() == QImage::Format_RGB32 ? 0xff000000 : 0;
        firstPixel = image.pixel(0,0) | alphaCorrection;
        return firstPixel == QColor(color).rgb();
    };

    // test the rendered image
    QImage image;
    auto testImage = [&](){
        image = grabWidgetWithoutRepaint(widget, clipArea).toImage();
        return image == expectedImage;
    };

    // Perform checks and make test case fail if unsuccessful
    if (!QTest::qWaitFor(testSize))
        return QTest::qCompare(pixmap.size(),
                         clipArea.size(),
                         "pixmap.size()",
                         "rect.size()",
                         __FILE__,
                         callerLine);

    if (!QTest::qWaitFor(testPixel)) {
        return QTest::qVerify(firstPixel == QColor(color).rgb(),
                           "firstPixel == QColor(color).rgb()",
                            qPrintable(msgRgbMismatch(firstPixel, QColor(color).rgb())),
                            __FILE__, callerLine);
    }

    if (!QTest::qWaitFor(testImage)) {
        return QTest::qVerify(image == expectedImage,
                            "image == expectedPixmap.toImage()",
                            "grabbed pixmap differs from expected pixmap",
                            __FILE__, callerLine);
    }

    return true;
}

void tst_QOpenGLWidget::stackWidgetOpaqueChildIsVisible()
{
#ifdef Q_OS_MACOS
    QSKIP("QScreen::grabWindow() doesn't work properly on OSX HighDPI screen: QTBUG-46803");
    return;
#endif
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Platform does not support window activation");
    if (QGuiApplication::platformName().startsWith(QLatin1String("offscreen"), Qt::CaseInsensitive))
        QSKIP("Offscreen: This fails.");

    QStackedWidget stack;

    QWidget* emptyWidget = new QWidget(&stack);
    stack.addWidget(emptyWidget);

    // Create an opaque red QOpenGLWidget.
    const int dimensionSize = 400;
    ClearWidget* clearWidget = new ClearWidget(&stack, dimensionSize, dimensionSize);
    clearWidget->setAttribute(Qt::WA_OpaquePaintEvent);
    stack.addWidget(clearWidget);

    // Show initial QWidget.
    stack.setCurrentIndex(0);
    stack.resize(dimensionSize, dimensionSize);
    stack.show();
    QVERIFY(QTest::qWaitForWindowExposed(&stack));
    stack.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&stack));

    // Switch to the QOpenGLWidget.
    stack.setCurrentIndex(1);
    QTRY_VERIFY(clearWidget->m_paintCalled);

    // Resize the tested region to be half size in the middle, because some OSes make the widget
    // have rounded corners (e.g. OSX), and the grabbed window pixmap will not coincide perfectly
    // with what was actually painted.
    QRect clipArea = stack.rect();
    clipArea.setSize(clipArea.size() / 2);
    const int translationOffsetToMiddle = dimensionSize / 4;
    clipArea.translate(translationOffsetToMiddle, translationOffsetToMiddle);

    // Verify that the QOpenGLWidget was actually painted AND displayed.
    const QColor red(255, 0, 0, 255);
    VERIFY_COLOR(&stack, clipArea, red);
    #undef VERIFY_COLOR
}

void tst_QOpenGLWidget::offscreen()
{
    {
        QScopedPointer<ClearWidget> w(new ClearWidget(0, 800, 600));
        w->resize(800, 600);

        w->setClearColor(0, 0, 1);
        QImage image = w->grabFramebuffer();

        QVERIFY(!image.isNull());
        QCOMPARE(image.width(), w->width());
        QCOMPARE(image.height(), w->height());
        QVERIFY(image.pixel(30, 40) == qRgb(0, 0, 255));
    }

    // QWidget::grab() should eventually end up in grabFramebuffer() as well
    {
        QScopedPointer<ClearWidget> w(new ClearWidget(0, 800, 600));
        w->resize(800, 600);

        w->setClearColor(0, 0, 1);
        QPixmap pm = w->grab();
        QImage image = pm.toImage();

        QVERIFY(!image.isNull());
        QCOMPARE(image.width(), w->width());
        QCOMPARE(image.height(), w->height());
        QVERIFY(image.pixel(30, 40) == qRgb(0, 0, 255));
    }

    // ditto for QWidget::render()
    {
        QScopedPointer<ClearWidget> w(new ClearWidget(0, 800, 600));
        w->resize(800, 600);

        w->setClearColor(0, 0, 1);
        QImage image(800, 600, QImage::Format_ARGB32);
        w->render(&image);

        QVERIFY(image.pixel(30, 40) == qRgb(0, 0, 255));
    }
}

void tst_QOpenGLWidget::offscreenThenOnscreen()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif
    QScopedPointer<ClearWidget> w(new ClearWidget(0, 800, 600));
    w->resize(800, 600);

    w->setClearColor(0, 0, 1);
    QImage image = w->grabFramebuffer();

    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(0, 0, 255));

    // now let's make things more challenging: show. Internally this needs
    // recreating the context.
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w.data()));

    image = w->grabFramebuffer();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), w->width());
    QCOMPARE(image.height(), w->height());
    QVERIFY(image.pixel(30, 40) == qRgb(0, 0, 255));
}

void tst_QOpenGLWidget::paintWhileHidden()
{
#ifdef Q_OS_ANDROID
    QSKIP("Crashes on Android, figure out why (QTBUG-102043)");
#endif
    QScopedPointer<QWidget> tlw(new QWidget);
    tlw->resize(640, 480);

    ClearWidget *w = new ClearWidget(0, 640, 480);
    w->setParent(tlw.data());
    w->setClearColor(0, 0, 1);

    tlw->show();
    QVERIFY(QTest::qWaitForWindowExposed(tlw.data()));

    // QTBUG-101620: Now make visible=false and call update and see if we get to
    // paintEvent/paintGL eventually, to ensure the updating of the texture is
    // not optimized permanently away even though there is no composition
    // on-screen at the point when update() is called.

    w->setVisible(false);
    w->m_paintCalled = false;
    w->update();
    w->setVisible(true);
    QTRY_VERIFY(w->m_paintCalled);
}

void tst_QOpenGLWidget::widgetWindowColorFormat_data()
{
    QTest::addColumn<bool>("translucent");
    QTest::newRow("Translucent background disabled") << false;
    QTest::newRow("Translucent background enabled") << true;
}

void tst_QOpenGLWidget::widgetWindowColorFormat()
{
    QFETCH(bool, translucent);

    QOpenGLWidget w;
    w.setAttribute(Qt::WA_TranslucentBackground, translucent);
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setFixedSize(16, 16);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QCOMPARE(w.format().redBufferSize(), ctx->format().redBufferSize());
    QCOMPARE(w.format().greenBufferSize(), ctx->format().greenBufferSize());
    QCOMPARE(w.format().blueBufferSize(), ctx->format().blueBufferSize());
}

class StaticTextPainterWidget : public QOpenGLWidget
{
public:
    StaticTextPainterWidget(QWidget *parent = nullptr)
        : QOpenGLWidget(parent)
    {
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        text.setText(QStringLiteral("test"));
        p.drawStaticText(0, 0, text);

        ctx = QOpenGLContext::currentContext();
    }

    QStaticText text;
    QOpenGLContext *ctx;
};

#ifdef QT_BUILD_INTERNAL
void tst_QOpenGLWidget::staticTextDanglingPointer()
{
    QWidget w;
    StaticTextPainterWidget *glw = new StaticTextPainterWidget(&w);
    w.resize(640, 480);
    glw->resize(320, 200);
    w.show();

    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QStaticTextPrivate *d = QStaticTextPrivate::get(&glw->text);

    QCOMPARE(d->itemCount, 1);
    QFontEngine *fe = d->items->fontEngine();

    for (int i = QFontEngine::Format_None; i <= QFontEngine::Format_ARGB; ++i) {
        QOpenGLTextureGlyphCache *cache =
                (QOpenGLTextureGlyphCache *) fe->glyphCache(glw->ctx,
                                                            QFontEngine::GlyphFormat(i),
                                                            QTransform());
        if (cache != nullptr)
            QCOMPARE(cache->paintEnginePrivate(), nullptr);
    }
}
#endif

QTEST_MAIN(tst_QOpenGLWidget)

#include "tst_qopenglwidget.moc"
