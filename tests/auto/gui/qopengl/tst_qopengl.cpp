/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QGenericMatrix>
#include <QtGui/QMatrix4x4>
#include <QtGui/private/qopengltextureblitter_p.h>


#include <QtTest/QtTest>

#include <QSignalSpy>

class tst_QOpenGL : public QObject
{
Q_OBJECT

private slots:
    void sharedResourceCleanup_data();
    void sharedResourceCleanup();
    void multiGroupSharedResourceCleanup_data();
    void multiGroupSharedResourceCleanup();
    void multiGroupSharedResourceCleanupCustom_data();
    void multiGroupSharedResourceCleanupCustom();
    void fboSimpleRendering_data();
    void fboSimpleRendering();
    void fboTextureOwnership_data();
    void fboTextureOwnership();
    void fboRendering_data();
    void fboRendering();
    void fboHandleNulledAfterContextDestroyed();
    void openGLPaintDevice_data();
    void openGLPaintDevice();
    void aboutToBeDestroyed();
    void sizeLessWindow();
    void QTBUG15621_triangulatingStrokerDivZero();
    void textureblitterFullSourceRectTransform();
    void textureblitterPartOriginBottomLeftSourceRectTransform();
    void textureblitterPartOriginTopLeftSourceRectTransform();
    void textureblitterFullTargetRectTransform();
    void textureblitterPartTargetRectTransform();
};

struct SharedResourceTracker
{
    SharedResourceTracker()
    {
        reset();
    }

    void reset()
    {
        invalidateResourceCalls = 0;
        freeResourceCalls = 0;
        destructorCalls = 0;
    }

    int invalidateResourceCalls;
    int freeResourceCalls;
    int destructorCalls;
};

struct SharedResource : public QOpenGLSharedResource
{
    SharedResource(SharedResourceTracker *t)
        : QOpenGLSharedResource(QOpenGLContextGroup::currentContextGroup())
        , resource(1)
        , tracker(t)
    {
    }

    SharedResource(QOpenGLContext *ctx)
        : QOpenGLSharedResource(ctx->shareGroup())
        , resource(1)
        , tracker(0)
    {
    }

    ~SharedResource()
    {
        if (tracker)
            tracker->destructorCalls++;
    }

    void invalidateResource()
    {
        resource = 0;
        if (tracker)
            tracker->invalidateResourceCalls++;
    }

    void freeResource(QOpenGLContext *context)
    {
        Q_ASSERT(context == QOpenGLContext::currentContext());
        Q_UNUSED(context)
        resource = 0;
        if (tracker)
            tracker->freeResourceCalls++;
    }

    int resource;
    SharedResourceTracker *tracker;
};

static QSurface *createSurface(int surfaceClass)
{
    if (surfaceClass == int(QSurface::Window)) {
        QWindow *window = new QWindow;
        window->setSurfaceType(QWindow::OpenGLSurface);
        window->setGeometry(0, 0, 10, 10);
        window->create();
        return window;
    } else if (surfaceClass == int(QSurface::Offscreen)) {
        // Create a window and get the format from that.  For example, if an EGL
        // implementation provides 565 and 888 configs for PBUFFER_BIT but only
        // 888 for WINDOW_BIT, we may end up with a pbuffer surface that is
        // incompatible with the context since it could choose the 565 while the
        // window and the context uses a config with 888.
        static QSurfaceFormat format;
        if (format.redBufferSize() == -1) {
            QWindow *window = new QWindow;
            window->setSurfaceType(QWindow::OpenGLSurface);
            window->setGeometry(0, 0, 10, 10);
            window->create();
            format = window->format();
            delete window;
        }
        QOffscreenSurface *offscreenSurface = new QOffscreenSurface;
        offscreenSurface->setFormat(format);
        offscreenSurface->create();
        return offscreenSurface;
    }
    return 0;
}

static void common_data()
{
    QTest::addColumn<int>("surfaceClass");

    QTest::newRow("Using QWindow") << int(QSurface::Window);
    QTest::newRow("Using QOffscreenSurface") << int(QSurface::Offscreen);
}

void tst_QOpenGL::sharedResourceCleanup_data()
{
    common_data();
}

void tst_QOpenGL::sharedResourceCleanup()
{
    QFETCH(int, surfaceClass);
    QScopedPointer<QSurface> surface(createSurface(surfaceClass));

    QOpenGLContext *ctx = new QOpenGLContext;
    QVERIFY(ctx->create());
    QVERIFY(ctx->makeCurrent(surface.data()));

    SharedResourceTracker tracker;
    SharedResource *resource = new SharedResource(&tracker);
    resource->free();

    QCOMPARE(tracker.invalidateResourceCalls, 0);
    QCOMPARE(tracker.freeResourceCalls, 1);
    QCOMPARE(tracker.destructorCalls, 1);

    tracker.reset();

    resource = new SharedResource(&tracker);

    QOpenGLContext *ctx2 = new QOpenGLContext;
    ctx2->setShareContext(ctx);
    QVERIFY(ctx2->create());

    delete ctx;

    resource->free();

    // no current context, freeResource() delayed
    QCOMPARE(tracker.invalidateResourceCalls, 0);
    QCOMPARE(tracker.freeResourceCalls, 0);
    QCOMPARE(tracker.destructorCalls, 0);

    ctx2->makeCurrent(surface.data());

    // freeResource() should now have been called
    QCOMPARE(tracker.invalidateResourceCalls, 0);
    QCOMPARE(tracker.freeResourceCalls, 1);
    QCOMPARE(tracker.destructorCalls, 1);

    tracker.reset();

    resource = new SharedResource(&tracker);

    // this should cause invalidateResource() to be called
    delete ctx2;

    QCOMPARE(tracker.invalidateResourceCalls, 1);
    QCOMPARE(tracker.freeResourceCalls, 0);
    QCOMPARE(tracker.destructorCalls, 0);

    // should have no effect other than destroying the resource,
    // as it has already been invalidated
    resource->free();

    QCOMPARE(tracker.invalidateResourceCalls, 1);
    QCOMPARE(tracker.freeResourceCalls, 0);
    QCOMPARE(tracker.destructorCalls, 1);
}

void tst_QOpenGL::multiGroupSharedResourceCleanup_data()
{
    common_data();
}

void tst_QOpenGL::multiGroupSharedResourceCleanup()
{
    QFETCH(int, surfaceClass);
    QScopedPointer<QSurface> surface(createSurface(surfaceClass));

    for (int i = 0; i < 10; ++i) {
        QOpenGLContext *gl = new QOpenGLContext();
        QVERIFY(gl->create());
        gl->makeCurrent(surface.data());
        {
            // Cause QOpenGLMultiGroupSharedResource instantiation.
            QOpenGLFunctions func(gl);
        }
        delete gl;
        // Cause context group's deleteLater() to be processed.
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }
    // Shouldn't crash when application exits.
}

void tst_QOpenGL::multiGroupSharedResourceCleanupCustom_data()
{
    common_data();
}

void tst_QOpenGL::multiGroupSharedResourceCleanupCustom()
{
    QFETCH(int, surfaceClass);
    QScopedPointer<QSurface> surface(createSurface(surfaceClass));

    QOpenGLContext *ctx = new QOpenGLContext();
    QVERIFY(ctx->create());
    QVERIFY(ctx->makeCurrent(surface.data()));

    QOpenGLMultiGroupSharedResource multiGroupSharedResource;
    SharedResource *resource = multiGroupSharedResource.value<SharedResource>(ctx);
    SharedResourceTracker tracker;
    resource->tracker = &tracker;

    delete ctx;

    QCOMPARE(tracker.invalidateResourceCalls, 1);
    QCOMPARE(tracker.freeResourceCalls, 0);
    QCOMPARE(tracker.destructorCalls, 1);
}

static bool fuzzyComparePixels(const QRgb testPixel, const QRgb refPixel, const char* file, int line, int x = -1, int y = -1)
{
    static int maxFuzz = 1;
    static bool maxFuzzSet = false;

    // On 16 bpp systems, we need to allow for more fuzz:
    if (!maxFuzzSet) {
        maxFuzzSet = true;
        if (QGuiApplication::primaryScreen()->depth() < 24)
            maxFuzz = 32;
    }

    int redFuzz = qAbs(qRed(testPixel) - qRed(refPixel));
    int greenFuzz = qAbs(qGreen(testPixel) - qGreen(refPixel));
    int blueFuzz = qAbs(qBlue(testPixel) - qBlue(refPixel));
    int alphaFuzz = qAbs(qAlpha(testPixel) - qAlpha(refPixel));

    if (refPixel != 0 && testPixel == 0) {
        QString msg;
        if (x >= 0) {
            msg = QString("Test pixel [%1, %2] is null (black) when it should be (%3,%4,%5,%6)")
                            .arg(x).arg(y)
                            .arg(qRed(refPixel)).arg(qGreen(refPixel)).arg(qBlue(refPixel)).arg(qAlpha(refPixel));
        } else {
            msg = QString("Test pixel is null (black) when it should be (%2,%3,%4,%5)")
                            .arg(qRed(refPixel)).arg(qGreen(refPixel)).arg(qBlue(refPixel)).arg(qAlpha(refPixel));
        }

        QTest::qFail(msg.toLatin1(), file, line);
        return false;
    }

    if (redFuzz > maxFuzz || greenFuzz > maxFuzz || blueFuzz > maxFuzz || alphaFuzz > maxFuzz) {
        QString msg;

        if (x >= 0)
            msg = QString("Pixel [%1,%2]: ").arg(x).arg(y);
        else
            msg = QString("Pixel ");

        msg += QString("Max fuzz (%1) exceeded: (%2,%3,%4,%5) vs (%6,%7,%8,%9)")
                      .arg(maxFuzz)
                      .arg(qRed(testPixel)).arg(qGreen(testPixel)).arg(qBlue(testPixel)).arg(qAlpha(testPixel))
                      .arg(qRed(refPixel)).arg(qGreen(refPixel)).arg(qBlue(refPixel)).arg(qAlpha(refPixel));
        QTest::qFail(msg.toLatin1(), file, line);
        return false;
    }
    return true;
}

static void fuzzyCompareImages(const QImage &testImage, const QImage &referenceImage, const char* file, int line)
{
    QCOMPARE(testImage.width(), referenceImage.width());
    QCOMPARE(testImage.height(), referenceImage.height());

    for (int y = 0; y < testImage.height(); y++) {
        for (int x = 0; x < testImage.width(); x++) {
            if (!fuzzyComparePixels(testImage.pixel(x, y), referenceImage.pixel(x, y), file, line, x, y)) {
                // Might as well save the images for easier debugging:
                referenceImage.save("referenceImage.png");
                testImage.save("testImage.png");
                return;
            }
        }
    }
}

#define QFUZZY_COMPARE_IMAGES(A,B) \
            fuzzyCompareImages(A, B, __FILE__, __LINE__)

#define QFUZZY_COMPARE_PIXELS(A,B) \
            fuzzyComparePixels(A, B, __FILE__, __LINE__)

void qt_opengl_draw_test_pattern(QPainter* painter, int width, int height)
{
    QPainterPath intersectingPath;
    intersectingPath.moveTo(0, 0);
    intersectingPath.lineTo(100, 0);
    intersectingPath.lineTo(0, 100);
    intersectingPath.lineTo(100, 100);
    intersectingPath.closeSubpath();

    QPainterPath trianglePath;
    trianglePath.moveTo(50, 0);
    trianglePath.lineTo(100, 100);
    trianglePath.lineTo(0, 100);
    trianglePath.closeSubpath();

    painter->setTransform(QTransform()); // reset xform
    painter->fillRect(-1, -1, width+2, height+2, Qt::red); // Background
    painter->translate(14, 14);
    painter->fillPath(intersectingPath, Qt::blue); // Test stencil buffer works
    painter->translate(128, 0);
    painter->setClipPath(trianglePath); // Test depth buffer works
    painter->setTransform(QTransform()); // reset xform ready for fill
    painter->fillRect(-1, -1, width+2, height+2, Qt::green);
}

void qt_opengl_check_test_pattern(const QImage& img)
{
    // As we're doing more than trivial painting, we can't just compare to
    // an image rendered with raster. Instead, we sample at well-defined
    // test-points:
    QVERIFY(!img.isNull());
    QVERIFY2(img.width() > 217, QByteArray::number(img.width()));
    QVERIFY2(img.height() > 90, QByteArray::number(img.height()));

    QFUZZY_COMPARE_PIXELS(img.pixel(39, 64), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(img.pixel(89, 64), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(img.pixel(64, 39), QColor(Qt::blue).rgb());
    QFUZZY_COMPARE_PIXELS(img.pixel(64, 89), QColor(Qt::blue).rgb());

    QFUZZY_COMPARE_PIXELS(img.pixel(167, 39), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(img.pixel(217, 39), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(img.pixel(192, 64), QColor(Qt::green).rgb());
}

void tst_QOpenGL::fboSimpleRendering_data()
{
    common_data();
}

void tst_QOpenGL::fboSimpleRendering()
{
    QFETCH(int, surfaceClass);
    QScopedPointer<QSurface> surface(createSurface(surfaceClass));

    QOpenGLContext ctx;
    QVERIFY(ctx.create());

    QVERIFY(ctx.makeCurrent(surface.data()));

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    // No multisample with combined depth/stencil attachment:
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::NoAttachment);

    const QSize size(200, 100);
    QScopedPointer<QOpenGLFramebufferObject> fbo(new QOpenGLFramebufferObject(size, fboFormat));

    QVERIFY(fbo->bind());

    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    const QImage fb = fbo->toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(fb.size(), size);
    QImage reference(size, QImage::Format_RGB32);
    reference.fill(0xffff0000);

    QFUZZY_COMPARE_IMAGES(fb, reference);
}

void tst_QOpenGL::fboTextureOwnership_data()
{
    common_data();
}

void tst_QOpenGL::fboTextureOwnership()
{
    QFETCH(int, surfaceClass);
    QScopedPointer<QSurface> surface(createSurface(surfaceClass));

    QOpenGLContext ctx;
    QVERIFY(ctx.create());

    ctx.makeCurrent(surface.data());

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::NoAttachment);

    QOpenGLFramebufferObject *fbo = new QOpenGLFramebufferObject(200, 100, fboFormat);
    QVERIFY(fbo->texture() != 0);
    fbo->bind();

    // pull out the texture
    GLuint texture = fbo->takeTexture();
    QVERIFY(texture != 0);
    QVERIFY(fbo->texture() == 0);

    // verify that the next bind() creates a new texture
    fbo->bind();
    QVERIFY(fbo->texture() != 0 && fbo->texture() != texture);

    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    QImage fb = fbo->toImage().convertToFormat(QImage::Format_RGB32);
    QImage reference(fb.size(), QImage::Format_RGB32);
    reference.fill(0xffff0000);

    QFUZZY_COMPARE_IMAGES(fb, reference);

    glDeleteTextures(1, &texture);
    delete fbo;
}

void tst_QOpenGL::fboRendering_data()
{
    common_data();
}

// NOTE: This tests that CombinedDepthStencil attachment works by assuming the
//       GL2 engine is being used and is implemented the same way as it was when
//       this autotest was written. If this is not the case, there may be some
//       false-positives: I.e. The test passes when either the depth or stencil
//       buffer is actually missing. But that's probably ok anyway.
void tst_QOpenGL::fboRendering()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(__x86_64__)
    QSKIP("QTBUG-22617");
#endif

    QFETCH(int, surfaceClass);
    QScopedPointer<QSurface> surface(createSurface(surfaceClass));

    QOpenGLContext ctx;
    QVERIFY(ctx.create());

    QVERIFY(ctx.makeCurrent(surface.data()));

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    // No multisample with combined depth/stencil attachment:
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

    // Uncomplicate things by using NPOT:
    const QSize size(256, 128);
    QOpenGLFramebufferObject fbo(size, fboFormat);

    if (fbo.attachment() != QOpenGLFramebufferObject::CombinedDepthStencil)
        QSKIP("FBOs missing combined depth~stencil support");

    QVERIFY(fbo.bind());

    QPainter fboPainter;
    QOpenGLPaintDevice device(fbo.width(), fbo.height());
    bool painterBegun = fboPainter.begin(&device);
    QVERIFY(painterBegun);

    qt_opengl_draw_test_pattern(&fboPainter, fbo.width(), fbo.height());

    fboPainter.end();

    const QImage fb = fbo.toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(fb.size(), size);

    qt_opengl_check_test_pattern(fb);
}

void tst_QOpenGL::fboHandleNulledAfterContextDestroyed()
{
    QWindow window;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.setGeometry(0, 0, 10, 10);
    window.create();

    QOpenGLFramebufferObject *fbo = 0;

    {
        QOpenGLContext ctx;
        QVERIFY(ctx.create());

        ctx.makeCurrent(&window);

        if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
            QSKIP("QOpenGLFramebufferObject not supported on this platform");

        fbo = new QOpenGLFramebufferObject(128, 128);

        QVERIFY(fbo->handle() != 0);
    }

    QCOMPARE(fbo->handle(), 0U);
}

void tst_QOpenGL::openGLPaintDevice_data()
{
    common_data();
}

void tst_QOpenGL::openGLPaintDevice()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(__x86_64__)
    QSKIP("QTBUG-22617");
#endif

    QFETCH(int, surfaceClass);
    QScopedPointer<QSurface> surface(createSurface(surfaceClass));

    QOpenGLContext ctx;
    QVERIFY(ctx.create());

    QSurfaceFormat format = ctx.format();
    if (format.majorVersion() < 2)
        QSKIP("This test requires at least OpenGL 2.0");
    QVERIFY(ctx.makeCurrent(surface.data()));

    const QSize size(128, 128);

    QImage image(size, QImage::Format_RGB32);
    QPainter p(&image);
    p.fillRect(0, 0, image.width() / 2, image.height() / 2, Qt::red);
    p.fillRect(image.width() / 2, 0, image.width() / 2, image.height() / 2, Qt::green);
    p.fillRect(image.width() / 2, image.height() / 2, image.width() / 2, image.height() / 2, Qt::blue);
    p.fillRect(0, image.height() / 2, image.width() / 2, image.height() / 2, Qt::white);
    p.end();

    QOpenGLFramebufferObject fbo(size);
    QVERIFY(fbo.bind());

    QOpenGLPaintDevice device(size);
    QVERIFY(p.begin(&device));
    p.fillRect(0, 0, image.width() / 2, image.height() / 2, Qt::red);
    p.fillRect(image.width() / 2, 0, image.width() / 2, image.height() / 2, Qt::green);
    p.fillRect(image.width() / 2, image.height() / 2, image.width() / 2, image.height() / 2, Qt::blue);
    p.fillRect(0, image.height() / 2, image.width() / 2, image.height() / 2, Qt::white);
    p.end();

    QImage actual = fbo.toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image.size(), actual.size());
    QCOMPARE(image, actual);

    QVERIFY(p.begin(&device));
    p.fillRect(0, 0, image.width(), image.height(), Qt::black);
    p.drawImage(0, 0, image);
    p.end();

    actual = fbo.toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image.size(), actual.size());
    QCOMPARE(image, actual);

    QVERIFY(p.begin(&device));
    p.fillRect(0, 0, image.width(), image.height(), Qt::black);
    p.fillRect(0, 0, image.width(), image.height(), QBrush(image));
    p.end();

    actual = fbo.toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image.size(), actual.size());
    QCOMPARE(image, actual);
}

void tst_QOpenGL::aboutToBeDestroyed()
{
    QWindow window;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.setGeometry(0, 0, 128, 128);
    window.create();

    QOpenGLContext *context = new QOpenGLContext;
    QSignalSpy spy(context, SIGNAL(aboutToBeDestroyed()));

    QVERIFY(context->create());
    QVERIFY(context->makeCurrent(&window));

    QCOMPARE(spy.size(), 0);

    delete context;

    QCOMPARE(spy.size(), 1);
}

// Verify that QOpenGLContext works with QWindows that do
// not have an explicit size set.
void tst_QOpenGL::sizeLessWindow()
{
    // top-level window
    {
        QWindow window;
        window.setSurfaceType(QWindow::OpenGLSurface);

        QOpenGLContext context;
        QVERIFY(context.create());

        window.show();
        QVERIFY(context.makeCurrent(&window));
        QVERIFY(QOpenGLContext::currentContext());
    }

    QVERIFY(!QOpenGLContext::currentContext());

    // child window
    {
        QWindow parent;
        QWindow window(&parent);
        window.setSurfaceType(QWindow::OpenGLSurface);

        QOpenGLContext context;
        QVERIFY(context.create());

        parent.show();
        window.show();
        QVERIFY(context.makeCurrent(&window));
        QVERIFY(QOpenGLContext::currentContext());
    }

    QVERIFY(!QOpenGLContext::currentContext());
}

void tst_QOpenGL::QTBUG15621_triangulatingStrokerDivZero()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(__x86_64__)
    QSKIP("QTBUG-22617");
#endif

    const QSize size(128, 128);

    QWindow window;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.setGeometry(QRect(QPoint(0, 0), size));
    window.create();

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    QVERIFY(ctx.makeCurrent(&window));

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    QOpenGLFramebufferObject fbo(size);
    QVERIFY(fbo.bind());

    QOpenGLPaintDevice device(size);

    // QTBUG-15621 is only a problem when qreal is double, but do the test anyway.
    qreal delta = sizeof(qreal) == sizeof(float) ? 1e-4 : 1e-8;
    QVERIFY(128 != 128 + delta);

    QPainterPath path;
    path.moveTo(16 + delta, 16);
    path.moveTo(16, 16);

    path.lineTo(16 + delta, 16); // Short lines to check for division by zero.
    path.lineTo(112 - delta, 16);
    path.lineTo(112, 16);

    path.quadTo(112, 16, 112, 16 + delta);
    path.quadTo(112, 64, 112, 112 - delta);
    path.quadTo(112, 112, 112, 112);

    path.cubicTo(112, 112, 112, 112, 112 - delta, 112);
    path.cubicTo(80, 112, 48, 112, 16 + delta, 112);
    path.cubicTo(16 + delta, 112, 16 + delta, 112, 16, 112);

    path.closeSubpath();

    QPen pen(Qt::red, 28, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

    QPainter p(&device);
    p.fillRect(QRect(QPoint(0, 0), size), Qt::blue);
    p.strokePath(path, pen);
    p.end();
    const QImage image = fbo.toImage().convertToFormat(QImage::Format_RGB32);
    QCOMPARE(image.size(), size);

    const QRgb red = 0xffff0000;
    const QRgb blue = 0xff0000ff;

    QCOMPARE(image.pixel(8, 8), red);
    QCOMPARE(image.pixel(119, 8), red);
    QCOMPARE(image.pixel(8, 119), red);
    QCOMPARE(image.pixel(119, 119), red);

    QCOMPARE(image.pixel(0, 0), blue);
    QCOMPARE(image.pixel(127, 0), blue);
    QCOMPARE(image.pixel(0, 127), blue);
    QCOMPARE(image.pixel(127, 127), blue);

    QCOMPARE(image.pixel(32, 32), blue);
    QCOMPARE(image.pixel(95, 32), blue);
    QCOMPARE(image.pixel(32, 95), blue);
    QCOMPARE(image.pixel(95, 95), blue);
}

typedef QGenericMatrix<1, 3, float> TestVertex3D;
static const float uv_top_left[] = {0.f, 1.f, 1.f};
static const float uv_bottom_left[] = {0.f, 0.f, 1.f};
static const float uv_top_right[] = {1.f, 1.f, 1.f};
static const float uv_bottom_right[] = {1.f, 0.f, 1.f};

bool q_fuzzy_compare(const TestVertex3D &left, const TestVertex3D &right) {
    return qFuzzyCompare(left(0,0), right(0,0)) &&
           qFuzzyCompare(left(1,0), right(1,0)) &&
           qFuzzyCompare(left(2,0), right(2,0));
}

void tst_QOpenGL::textureblitterFullSourceRectTransform()
{
    TestVertex3D topLeft(uv_top_left);
    TestVertex3D bottomLeft(uv_bottom_left);
    TestVertex3D topRight(uv_top_right);
    TestVertex3D bottomRight(uv_bottom_right);

    QRectF rect(0,0,1,1);
    QMatrix3x3 flippedMatrix = QOpenGLTextureBlitter::sourceTransform(rect, rect.size().toSize(), QOpenGLTextureBlitter::OriginTopLeft);

    TestVertex3D flippedTopLeft = flippedMatrix * topLeft;
    QCOMPARE(flippedTopLeft, bottomLeft);

    TestVertex3D flippedBottomLeft = flippedMatrix * bottomLeft;
    QCOMPARE(flippedBottomLeft, topLeft);

    TestVertex3D flippedTopRight = flippedMatrix * topRight;
    QCOMPARE(flippedTopRight, bottomRight);

    TestVertex3D flippedBottomRight = flippedMatrix * bottomRight;
    QCOMPARE(flippedBottomRight, topRight);

    QMatrix3x3 identityMatrix = QOpenGLTextureBlitter::sourceTransform(rect, rect.size().toSize(), QOpenGLTextureBlitter::OriginBottomLeft);

    TestVertex3D notFlippedTopLeft = identityMatrix * topLeft;
    QCOMPARE(notFlippedTopLeft, topLeft);

    TestVertex3D notFlippedBottomLeft = identityMatrix * bottomLeft;
    QCOMPARE(notFlippedBottomLeft, bottomLeft);

    TestVertex3D notFlippedTopRight = identityMatrix * topRight;
    QCOMPARE(notFlippedTopRight, topRight);

    TestVertex3D notFlippedBottomRight = identityMatrix * bottomRight;
    QCOMPARE(notFlippedBottomRight, bottomRight);
}

void tst_QOpenGL::textureblitterPartOriginBottomLeftSourceRectTransform()
{
    TestVertex3D topLeft(uv_top_left);
    TestVertex3D bottomLeft(uv_bottom_left);
    TestVertex3D topRight(uv_top_right);
    TestVertex3D bottomRight(uv_bottom_right);

    QRectF sourceRect(50,200,200,200);
    QSize textureSize(400,400);

    QMatrix3x3 sourceMatrix = QOpenGLTextureBlitter::sourceTransform(sourceRect, textureSize, QOpenGLTextureBlitter::OriginBottomLeft);

    const float x_point_ratio = sourceRect.topLeft().x() / textureSize.width();
    const float y_point_ratio = sourceRect.topLeft().y() / textureSize.height();
    const float width_ratio = sourceRect.width() / textureSize.width();
    const float height_ratio = sourceRect.height() / textureSize.height();

    TestVertex3D uvTopLeft = sourceMatrix * topLeft;
    const float expected_top_left[] = { x_point_ratio, y_point_ratio + height_ratio, 1 };
    TestVertex3D expectedTopLeft(expected_top_left);
    QCOMPARE(uvTopLeft, expectedTopLeft);

    TestVertex3D uvBottomLeft = sourceMatrix * bottomLeft;
    const float expected_bottom_left[] = { x_point_ratio, y_point_ratio, 1 };
    TestVertex3D expectedBottomLeft(expected_bottom_left);
    QCOMPARE(uvBottomLeft, expectedBottomLeft);

    TestVertex3D uvTopRight = sourceMatrix * topRight;
    const float expected_top_right[] = { x_point_ratio + width_ratio, y_point_ratio + height_ratio, 1 };
    TestVertex3D expectedTopRight(expected_top_right);
    QCOMPARE(uvTopRight, expectedTopRight);

    TestVertex3D uvBottomRight = sourceMatrix * bottomRight;
    const float expected_bottom_right[] = { x_point_ratio + width_ratio, y_point_ratio, 1 };
    TestVertex3D expectedBottomRight(expected_bottom_right);
    QCOMPARE(uvBottomRight, expectedBottomRight);
}

void tst_QOpenGL::textureblitterPartOriginTopLeftSourceRectTransform()
{
    TestVertex3D topLeft(uv_top_left);
    TestVertex3D bottomLeft(uv_bottom_left);
    TestVertex3D topRight(uv_top_right);
    TestVertex3D bottomRight(uv_bottom_right);

    QRectF sourceRect(50,190,170,170);
    QSize textureSize(400,400);

    QMatrix3x3 sourceMatrix = QOpenGLTextureBlitter::sourceTransform(sourceRect, textureSize, QOpenGLTextureBlitter::OriginTopLeft);

    const float x_point_ratio = sourceRect.topLeft().x() / textureSize.width();
    const float y_point_ratio = sourceRect.topLeft().y() / textureSize.height();
    const float width_ratio = sourceRect.width() / textureSize.width();
    const float height_ratio = sourceRect.height() / textureSize.height();

    TestVertex3D uvTopLeft = sourceMatrix * topLeft;
    const float expected_top_left[] = { x_point_ratio, 1 - y_point_ratio - height_ratio, 1 };
    TestVertex3D expectedTopLeft(expected_top_left);
    QVERIFY(q_fuzzy_compare(uvTopLeft, expectedTopLeft));

    TestVertex3D uvBottomLeft = sourceMatrix * bottomLeft;
    const float expected_bottom_left[] = { x_point_ratio, 1 - y_point_ratio, 1 };
    TestVertex3D expectedBottomLeft(expected_bottom_left);
    QVERIFY(q_fuzzy_compare(uvBottomLeft, expectedBottomLeft));

    TestVertex3D uvTopRight = sourceMatrix * topRight;
    const float expected_top_right[] = { x_point_ratio + width_ratio, 1 - y_point_ratio - height_ratio, 1 };
    TestVertex3D expectedTopRight(expected_top_right);
    QVERIFY(q_fuzzy_compare(uvTopRight, expectedTopRight));

    TestVertex3D uvBottomRight = sourceMatrix * bottomRight;
    const float expected_bottom_right[] = { x_point_ratio + width_ratio, 1 - y_point_ratio, 1 };
    TestVertex3D expectedBottomRight(expected_bottom_right);
    QVERIFY(q_fuzzy_compare(uvBottomRight, expectedBottomRight));
}

void tst_QOpenGL::textureblitterFullTargetRectTransform()
{
    QVector4D topLeft(-1.f, 1.f, 0.f, 1.f);
    QVector4D bottomLeft(-1.f, -1.f, 0.f, 1.f);
    QVector4D topRight(1.f, 1.f, 0.f, 1.f);
    QVector4D bottomRight(1.f, -1.f, 0.f, 1.f);

    QRectF rect(0,0,200,200);
    QMatrix4x4 targetMatrix = QOpenGLTextureBlitter::targetTransform(rect,rect.toRect());

    QVector4D translatedTopLeft = targetMatrix * topLeft;
    QCOMPARE(translatedTopLeft, topLeft);

    QVector4D translatedBottomLeft = targetMatrix * bottomLeft;
    QCOMPARE(translatedBottomLeft, bottomLeft);

    QVector4D translatedTopRight = targetMatrix * topRight;
    QCOMPARE(translatedTopRight, topRight);

    QVector4D translatedBottomRight = targetMatrix * bottomRight;
    QCOMPARE(translatedBottomRight, bottomRight);
}

void tst_QOpenGL::textureblitterPartTargetRectTransform()
{
    QVector4D topLeft(-1.f, 1.f, 0.f, 1.f);
    QVector4D bottomLeft(-1.f, -1.f, 0.f, 1.f);
    QVector4D topRight(1.f, 1.f, 0.f, 1.f);
    QVector4D bottomRight(1.f, -1.f, 0.f, 1.f);

    QRectF targetRect(50,50,200,200);
    QRect viewport(0,0,400,400);

    //multiply by 2 since coordinate system goes from -1 -> 1;
    qreal x_point_ratio = (50. / 400.) * 2;
    qreal y_point_ratio = (50. / 400.) * 2;
    qreal width_ratio = (200. / 400.) * 2;
    qreal height_ratio = (200. / 400.) * 2;

    QMatrix4x4 targetMatrix = QOpenGLTextureBlitter::targetTransform(targetRect, viewport);

    QVector4D targetTopLeft = targetMatrix * topLeft;
    QVector4D expectedTopLeft(-1 + x_point_ratio, 1 - y_point_ratio, .0, 1.0);
    QCOMPARE(targetTopLeft, expectedTopLeft);

    QVector4D targetBottomLeft = targetMatrix * bottomLeft;
    QVector4D expectedBottomLeft(-1 + x_point_ratio, 1 - y_point_ratio  - height_ratio, 0.0, 1.0);
    QCOMPARE(targetBottomLeft, expectedBottomLeft);

    QVector4D targetTopRight = targetMatrix * topRight;
    QVector4D expectedTopRight(-1 + x_point_ratio + width_ratio, 1 - y_point_ratio, 0.0, 1.0);
    QCOMPARE(targetTopRight, expectedTopRight);

    QVector4D targetBottomRight = targetMatrix * bottomRight;
    QVector4D expectedBottomRight(-1 + x_point_ratio + width_ratio, 1 - y_point_ratio - height_ratio, 0.0, 1.0);
    QCOMPARE(targetBottomRight, expectedBottomRight);
}

QTEST_MAIN(tst_QOpenGL)

#include "tst_qopengl.moc"
