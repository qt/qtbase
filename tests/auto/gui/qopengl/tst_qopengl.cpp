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


#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLFunctions_4_2_Core>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QOpenGLTexture>
#include <QtGui/QPainter>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QGenericMatrix>
#include <QtGui/QMatrix4x4>
#include <QtGui/private/qopengltextureblitter_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qopenglextensions_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

#include <QtTest/QtTest>

#include <QSignalSpy>

#ifdef USE_GLX
// Must be included last due to the X11 types
#include <QtPlatformHeaders/QGLXNativeContext>
#endif

#if defined(Q_OS_WIN32) && !defined(QT_OPENGL_ES_2)
#include <QtPlatformHeaders/QWGLNativeContext>
#endif

Q_DECLARE_METATYPE(QImage::Format)

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
    void fboRenderingRGB30_data();
    void fboRenderingRGB30();
    void fboHandleNulledAfterContextDestroyed();
    void fboMRT();
    void fboMRT_differentFormats();
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
    void defaultSurfaceFormat();
    void imageFormatPainting();
    void nullTextureInitializtion();

#ifdef USE_GLX
    void glxContextWrap();
#endif

#if defined(Q_OS_WIN32) && !defined(QT_OPENGL_ES_2)
    void wglContextWrap();
#endif

    void vaoCreate();
    void bufferCreate();
    void bufferMapRange();
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

    ctx.functions()->glClearColor(1.0, 0.0, 0.0, 1.0);
    ctx.functions()->glClear(GL_COLOR_BUFFER_BIT);
    ctx.functions()->glFinish();

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
    QCOMPARE(fbo->texture(), GLuint(0));

    // verify that the next bind() creates a new texture
    fbo->bind();
    QVERIFY(fbo->texture() != 0 && fbo->texture() != texture);

    ctx.functions()->glClearColor(1.0, 0.0, 0.0, 1.0);
    ctx.functions()->glClear(GL_COLOR_BUFFER_BIT);
    ctx.functions()->glFinish();

    QImage fb = fbo->toImage().convertToFormat(QImage::Format_RGB32);
    QImage reference(fb.size(), QImage::Format_RGB32);
    reference.fill(0xffff0000);

    QFUZZY_COMPARE_IMAGES(fb, reference);

    ctx.functions()->glDeleteTextures(1, &texture);
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

    // Uncomplicate things by using POT:
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

void tst_QOpenGL::fboRenderingRGB30_data()
{
    common_data();
}

#ifndef GL_RGB5_A1
#define GL_RGB5_A1                        0x8057
#endif

#ifndef GL_RGBA8
#define GL_RGBA8                          0x8058
#endif

#ifndef GL_RGB10_A2
#define GL_RGB10_A2                       0x8059
#endif

#ifndef GL_FRAMEBUFFER_RENDERABLE
#define GL_FRAMEBUFFER_RENDERABLE         0x8289
#endif

#ifndef GL_FULL_SUPPORT
#define GL_FULL_SUPPORT                   0x82B7
#endif

static bool hasRGB10A2(QOpenGLContext *ctx)
{
    if (ctx->format().majorVersion() < 3)
        return false;
#ifndef QT_OPENGL_ES_2
    if (!ctx->isOpenGLES() && ctx->format().majorVersion() >= 4) {
        GLint value = -1;
        QOpenGLFunctions_4_2_Core* vFuncs = ctx->versionFunctions<QOpenGLFunctions_4_2_Core>();
        if (vFuncs && vFuncs->initializeOpenGLFunctions()) {
            vFuncs->glGetInternalformativ(GL_TEXTURE_2D, GL_RGB10_A2, GL_FRAMEBUFFER_RENDERABLE, 1, &value);
            if (value != GL_FULL_SUPPORT)
                return false;
        }
    }
#endif
    return true;
}

void tst_QOpenGL::fboRenderingRGB30()
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

    if (!hasRGB10A2(&ctx))
        QSKIP("An internal RGB30_A2 format is not guaranteed on this platform");

    // No multisample with combined depth/stencil attachment:
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    fboFormat.setInternalTextureFormat(GL_RGB10_A2);

    // Uncomplicate things by using POT:
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

    QImage fb = fbo.toImage();
    QCOMPARE(fb.format(), QImage::Format_A2BGR30_Premultiplied);
    QCOMPARE(fb.size(), size);

    qt_opengl_check_test_pattern(fb);

    // Check rendering can handle color values below 1/256.
    fboPainter.begin(&device);
    fboPainter.fillRect(QRect(0, 0, 256, 128), QColor::fromRgbF(1.0/512, 1.0/512, 1.0/512));
    fboPainter.end();
    fb = fbo.toImage();
    uint pixel = ((uint*)fb.bits())[0];
    QVERIFY((pixel & 0x3f) > 0);
    QVERIFY(((pixel >> 10) & 0x3f) > 0);
    QVERIFY(((pixel >> 20) & 0x3f) > 0);

    pixel = (3U << 30) | (2U << 20) | (2U << 10) | 2U;
    fb.fill(pixel);

    fboPainter.begin(&device);
    fboPainter.setCompositionMode(QPainter::CompositionMode_Source);
    fboPainter.drawImage(0, 0, fb);
    fboPainter.end();
    fb = fbo.toImage();
    pixel = ((uint*)fb.bits())[0];
    QVERIFY((pixel & 0x3f) > 0);
    QVERIFY(((pixel >> 10) & 0x3f) > 0);
    QVERIFY(((pixel >> 20) & 0x3f) > 0);
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

void tst_QOpenGL::fboMRT()
{
    QWindow window;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.setGeometry(0, 0, 10, 10);
    window.create();

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    ctx.makeCurrent(&window);

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    if (!ctx.functions()->hasOpenGLFeature(QOpenGLFunctions::MultipleRenderTargets))
        QSKIP("Multiple render targets not supported on this platform");

    QOpenGLExtraFunctions *ef = ctx.extraFunctions();

    {
        // 3 color attachments, different sizes, same internal format, no depth/stencil.
        QVector<QSize> sizes;
        sizes << QSize(128, 128) << QSize(192, 128) << QSize(432, 123);
        QOpenGLFramebufferObject fbo(sizes[0]);
        fbo.addColorAttachment(sizes[1]);
        fbo.addColorAttachment(sizes[2]);
        QVERIFY(fbo.bind());
        QCOMPARE(fbo.attachment(), QOpenGLFramebufferObject::NoAttachment);
        QCOMPARE(sizes, fbo.sizes());
        QCOMPARE(sizes[0], fbo.size());
        // Clear the three buffers to red, green and blue.
        GLenum drawBuf = GL_COLOR_ATTACHMENT0;
        ef->glDrawBuffers(1, &drawBuf);
        ef->glClearColor(1, 0, 0, 1);
        ef->glClear(GL_COLOR_BUFFER_BIT);
        drawBuf = GL_COLOR_ATTACHMENT0 + 1;
        ef->glDrawBuffers(1, &drawBuf);
        ef->glClearColor(0, 1, 0, 1);
        ef->glClear(GL_COLOR_BUFFER_BIT);
        drawBuf = GL_COLOR_ATTACHMENT0 + 2;
        ef->glDrawBuffers(1, &drawBuf);
        ef->glClearColor(0, 0, 1, 1);
        ef->glClear(GL_COLOR_BUFFER_BIT);
        // Verify, keeping in mind that only a 128x123 area is touched in the buffers.
        // Some drivers do not get this right, unfortunately, so do not rely on it.
        const char *vendor = (const char *) ef->glGetString(GL_VENDOR);
        bool hasCorrectMRT = false;
        if (vendor && strstr(vendor, "NVIDIA")) // maybe others too
            hasCorrectMRT = true;
        QImage img = fbo.toImage(false, 0);
        QCOMPARE(img.size(), sizes[0]);
        QCOMPARE(img.pixel(0, 0), qRgb(255, 0, 0));
        if (hasCorrectMRT)
            QCOMPARE(img.pixel(127, 122), qRgb(255, 0, 0));
        img = fbo.toImage(false, 1);
        QCOMPARE(img.size(), sizes[1]);
        QCOMPARE(img.pixel(0, 0), qRgb(0, 255, 0));
        if (hasCorrectMRT)
            QCOMPARE(img.pixel(127, 122), qRgb(0, 255, 0));
        img = fbo.toImage(false, 2);
        QCOMPARE(img.size(), sizes[2]);
        QCOMPARE(img.pixel(0, 0), qRgb(0, 0, 255));
        if (hasCorrectMRT)
            QCOMPARE(img.pixel(127, 122), qRgb(0, 0, 255));
        fbo.release();
    }

    {
        // 2 color attachments, same size, same internal format, depth/stencil.
        QVector<QSize> sizes;
        sizes.fill(QSize(128, 128), 2);
        QOpenGLFramebufferObject fbo(sizes[0], QOpenGLFramebufferObject::CombinedDepthStencil);
        fbo.addColorAttachment(sizes[1]);
        QVERIFY(fbo.bind());
        QCOMPARE(fbo.attachment(), QOpenGLFramebufferObject::CombinedDepthStencil);
        QCOMPARE(sizes, fbo.sizes());
        QCOMPARE(sizes[0], fbo.size());
        ef->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ef->glFinish();
        fbo.release();
    }
}

void tst_QOpenGL::fboMRT_differentFormats()
{
    QWindow window;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.setGeometry(0, 0, 10, 10);
    window.create();

    QOpenGLContext ctx;
    QVERIFY(ctx.create());
    ctx.makeCurrent(&window);

    QOpenGLFunctions *f = ctx.functions();
    const char * vendor = (const char *) f->glGetString(GL_VENDOR);
    if (vendor && strstr(vendor, "VMware, Inc."))
        QSKIP("The tested formats may not be supported on this platform");

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    if (!f->hasOpenGLFeature(QOpenGLFunctions::MultipleRenderTargets))
        QSKIP("Multiple render targets not supported on this platform");

    if (!hasRGB10A2(&ctx))
        QSKIP("RGB10_A2 not supported on this platform");

    // 3 color attachments, same size, different internal format, depth/stencil.
    QVector<QSize> sizes;
    sizes.fill(QSize(128, 128), 3);
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    QVector<GLenum> internalFormats;
    internalFormats << GL_RGBA8 << GL_RGB10_A2 << GL_RGB5_A1;
    format.setInternalTextureFormat(internalFormats[0]);
    QOpenGLFramebufferObject fbo(sizes[0], format);
    fbo.addColorAttachment(sizes[1], internalFormats[1]);
    fbo.addColorAttachment(sizes[2], internalFormats[2]);

    QVERIFY(fbo.bind());
    QCOMPARE(fbo.attachment(), QOpenGLFramebufferObject::CombinedDepthStencil);
    QCOMPARE(sizes, fbo.sizes());
    QCOMPARE(sizes[0], fbo.size());

    QOpenGLExtraFunctions *ef = ctx.extraFunctions();
    QVERIFY(ef->glGetError() == 0);
    ef->glClearColor(1, 0, 0, 1);
    GLenum drawBuf[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1, GL_COLOR_ATTACHMENT0 + 2 };
    ef->glDrawBuffers(3, drawBuf);
    ef->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QVERIFY(ef->glGetError() == 0);

    QImage img = fbo.toImage(true, 0);
    QCOMPARE(img.size(), sizes[0]);
    QCOMPARE(img.pixel(0, 0), qRgb(255, 0, 0));
    img = fbo.toImage(true, 1);
    QCOMPARE(img.size(), sizes[1]);
    QCOMPARE(img.format(), QImage::Format_A2BGR30_Premultiplied);
    QCOMPARE(img.pixel(0, 0), qRgb(255, 0, 0));

    fbo.release();
}

void tst_QOpenGL::imageFormatPainting()
{
    QScopedPointer<QSurface> surface(createSurface(QSurface::Window));

    QOpenGLContext ctx;
    QVERIFY(ctx.create());

    QVERIFY(ctx.makeCurrent(surface.data()));

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

    const QSize size(128, 128);
    QOpenGLFramebufferObject fbo(size, fboFormat);

    if (fbo.attachment() != QOpenGLFramebufferObject::CombinedDepthStencil)
        QSKIP("FBOs missing combined depth~stencil support");

    QVERIFY(fbo.bind());

    QImage alpha(128, 128, QImage::Format_Alpha8);
    alpha.fill(127);

    QPainter fboPainter;
    QOpenGLPaintDevice device(fbo.width(), fbo.height());

    QVERIFY(fboPainter.begin(&device));
    fboPainter.fillRect(0, 0, 128, 128, qRgb(255, 0, 255));
    fboPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    fboPainter.drawImage(0, 0, alpha);
    fboPainter.end();

    QImage fb = fbo.toImage();
    QCOMPARE(fb.pixel(0, 0), qRgba(127, 0, 127, 127));

    QImage grayscale(128, 128, QImage::Format_Grayscale8);
    grayscale.fill(128);

    QVERIFY(fboPainter.begin(&device));
    fboPainter.setCompositionMode(QPainter::CompositionMode_Plus);
    fboPainter.drawImage(0, 0, grayscale);
    fboPainter.end();

    fb = fbo.toImage();
    QCOMPARE(fb.pixel(0, 0), qRgb(255, 128, 255));

    QImage argb(128, 128, QImage::Format_ARGB32);
    argb.fill(qRgba(255, 255, 255, 128));

    QVERIFY(fboPainter.begin(&device));
    fboPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    fboPainter.drawImage(0, 0, argb);
    fboPainter.end();

    fb = fbo.toImage();
    QCOMPARE(fb.pixel(0, 0), qRgb(255, 192, 255));

}

void tst_QOpenGL::openGLPaintDevice_data()
{
    QTest::addColumn<int>("surfaceClass");
    QTest::addColumn<QImage::Format>("imageFormat");

    QTest::newRow("Using QWindow - RGB32") << int(QSurface::Window) << QImage::Format_RGB32;
    QTest::newRow("Using QOffscreenSurface - RGB32") << int(QSurface::Offscreen) << QImage::Format_RGB32;
    QTest::newRow("Using QOffscreenSurface - RGBx8888") << int(QSurface::Offscreen) << QImage::Format_RGBX8888;
    QTest::newRow("Using QOffscreenSurface - RGB888") << int(QSurface::Offscreen) << QImage::Format_RGB888;
    QTest::newRow("Using QOffscreenSurface - RGB16") << int(QSurface::Offscreen) << QImage::Format_RGB16;
}

void tst_QOpenGL::openGLPaintDevice()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(__x86_64__)
    QSKIP("QTBUG-22617");
#endif

    QFETCH(int, surfaceClass);
    QFETCH(QImage::Format, imageFormat);
    QScopedPointer<QSurface> surface(createSurface(surfaceClass));

    QOpenGLContext ctx;
    QVERIFY(ctx.create());

    QSurfaceFormat format = ctx.format();
    if (format.majorVersion() < 2)
        QSKIP("This test requires at least OpenGL 2.0");
    QVERIFY(ctx.makeCurrent(surface.data()));

    const QSize size(128, 128);

    QImage image(size, imageFormat);
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

    QImage actual = fbo.toImage().convertToFormat(imageFormat);
    QCOMPARE(image.size(), actual.size());
    QCOMPARE(image, actual);

    QVERIFY(p.begin(&device));
    p.fillRect(0, 0, image.width(), image.height(), Qt::black);
    p.drawImage(0, 0, image);
    p.end();

    actual = fbo.toImage().convertToFormat(imageFormat);
    QCOMPARE(image.size(), actual.size());
    QCOMPARE(image, actual);

    QVERIFY(p.begin(&device));
    p.fillRect(0, 0, image.width(), image.height(), Qt::black);
    p.fillRect(0, 0, image.width(), image.height(), QBrush(image));
    p.end();

    actual = fbo.toImage().convertToFormat(imageFormat);
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

void tst_QOpenGL::defaultSurfaceFormat()
{
    QSurfaceFormat fmt;
    QCOMPARE(QSurfaceFormat::defaultFormat(), fmt);

    fmt.setDepthBufferSize(16);
    QSurfaceFormat::setDefaultFormat(fmt);
    QCOMPARE(QSurfaceFormat::defaultFormat(), fmt);
    QCOMPARE(QSurfaceFormat::defaultFormat().depthBufferSize(), 16);

    QScopedPointer<QWindow> window(new QWindow);
    QCOMPARE(window->requestedFormat(), fmt);

    QScopedPointer<QOpenGLContext> context(new QOpenGLContext);
    QCOMPARE(context->format(), fmt);
}

#ifdef USE_GLX
void tst_QOpenGL::glxContextWrap()
{
    QWindow *window = new QWindow;
    window->setSurfaceType(QWindow::OpenGLSurface);
    window->setGeometry(0, 0, 10, 10);
    window->show();
    QTest::qWaitForWindowExposed(window);

    QPlatformNativeInterface *nativeIf = QGuiApplicationPrivate::instance()->platformIntegration()->nativeInterface();
    QVERIFY(nativeIf);

    // Fetch a GLXContext.
    QOpenGLContext *ctx0 = new QOpenGLContext;
    ctx0->setFormat(window->format());
    QVERIFY(ctx0->create());
    QVariant v = ctx0->nativeHandle();
    QVERIFY(!v.isNull());
    QVERIFY(v.canConvert<QGLXNativeContext>());
    GLXContext context = v.value<QGLXNativeContext>().context();
    QVERIFY(context);

    // Then create another QOpenGLContext wrapping it.
    QOpenGLContext *ctx = new QOpenGLContext;
    ctx->setNativeHandle(QVariant::fromValue<QGLXNativeContext>(QGLXNativeContext(context)));
    QVERIFY(ctx->create());
    QCOMPARE(ctx->nativeHandle().value<QGLXNativeContext>().context(), context);
    QVERIFY(nativeIf->nativeResourceForContext(QByteArrayLiteral("glxcontext"), ctx) == (void *) context);

    QVERIFY(ctx->makeCurrent(window));
    ctx->doneCurrent();

    delete ctx;
    delete ctx0;

    delete window;
}
#endif // USE_GLX

#if defined(Q_OS_WIN32) && !defined(QT_OPENGL_ES_2)
void tst_QOpenGL::wglContextWrap()
{
    QScopedPointer<QOpenGLContext> ctx(new QOpenGLContext);
    QVERIFY(ctx->create());
    if (ctx->isOpenGLES())
        QSKIP("Not applicable to EGL");

    QScopedPointer<QWindow> window(new QWindow);
    window->setSurfaceType(QWindow::OpenGLSurface);
    window->setGeometry(0, 0, 256, 256);
    window->show();
    QTest::qWaitForWindowExposed(window.data());

    QVariant v = ctx->nativeHandle();
    QVERIFY(!v.isNull());
    QVERIFY(v.canConvert<QWGLNativeContext>());
    QWGLNativeContext nativeContext = v.value<QWGLNativeContext>();
    QVERIFY(nativeContext.context());

    // Now do a makeCurrent() do make sure the pixel format on the native
    // window (the HWND we are going to retrieve below) is set.
    QVERIFY(ctx->makeCurrent(window.data()));
    ctx->doneCurrent();

    HWND wnd = (HWND) qGuiApp->platformNativeInterface()->nativeResourceForWindow(QByteArrayLiteral("handle"), window.data());
    QVERIFY(wnd);

    QScopedPointer<QOpenGLContext> adopted(new QOpenGLContext);
    adopted->setNativeHandle(QVariant::fromValue<QWGLNativeContext>(QWGLNativeContext(nativeContext.context(), wnd)));
    QVERIFY(adopted->create());

    // This tests two things: that a regular, non-adopted QOpenGLContext is
    // able to return a QSurfaceFormat containing the real values after
    // create(), and that the adopted context got the correct pixel format from
    // window and was able to update its format accordingly.
    QCOMPARE(adopted->format().version(), ctx->format().version());
    QCOMPARE(adopted->format().profile(), ctx->format().profile());
    QVERIFY(ctx->format().redBufferSize() > 0);
    QCOMPARE(adopted->format().redBufferSize(), ctx->format().redBufferSize());
    QVERIFY(ctx->format().greenBufferSize() > 0);
    QCOMPARE(adopted->format().greenBufferSize(), ctx->format().greenBufferSize());
    QVERIFY(ctx->format().blueBufferSize() > 0);
    QCOMPARE(adopted->format().blueBufferSize(), ctx->format().blueBufferSize());
    QVERIFY(ctx->format().depthBufferSize() > 0);
    QCOMPARE(adopted->format().depthBufferSize(), ctx->format().depthBufferSize());
    QVERIFY(ctx->format().stencilBufferSize() > 0);
    QCOMPARE(adopted->format().stencilBufferSize(), ctx->format().stencilBufferSize());

    // This must work since we are using the exact same window.
    QVERIFY(adopted->makeCurrent(window.data()));
    adopted->doneCurrent();
}
#endif // Q_OS_WIN32 && !QT_OPENGL_ES_2

void tst_QOpenGL::vaoCreate()
{
    QScopedPointer<QSurface> surface(createSurface(QSurface::Window));
    QOpenGLContext *ctx = new QOpenGLContext;
    ctx->create();
    ctx->makeCurrent(surface.data());

    QOpenGLVertexArrayObject vao;
    bool success = vao.create();
    if (ctx->isOpenGLES()) {
        if (ctx->format().majorVersion() >= 3 || ctx->hasExtension(QByteArrayLiteral("GL_OES_vertex_array_object")))
            QVERIFY(success);
    } else {
        if (ctx->format().majorVersion() >= 3 || ctx->hasExtension(QByteArrayLiteral("GL_ARB_vertex_array_object")))
            QVERIFY(success);
    }

    vao.destroy();
    ctx->doneCurrent();
}

void tst_QOpenGL::bufferCreate()
{
    QScopedPointer<QSurface> surface(createSurface(QSurface::Window));
    QOpenGLContext *ctx = new QOpenGLContext;
    ctx->create();
    ctx->makeCurrent(surface.data());

    QOpenGLBuffer buf;

    QVERIFY(!buf.isCreated());

    QVERIFY(buf.create());
    QVERIFY(buf.isCreated());

    QCOMPARE(buf.type(), QOpenGLBuffer::VertexBuffer);

    buf.bind();
    buf.allocate(128);
    QCOMPARE(buf.size(), 128);

    buf.release();

    buf.destroy();
    QVERIFY(!buf.isCreated());

    ctx->doneCurrent();
}

void tst_QOpenGL::bufferMapRange()
{
    QScopedPointer<QSurface> surface(createSurface(QSurface::Window));
    QOpenGLContext *ctx = new QOpenGLContext;
    ctx->create();
    ctx->makeCurrent(surface.data());

    QOpenGLExtensions funcs(ctx);
    if (!funcs.hasOpenGLExtension(QOpenGLExtensions::MapBufferRange))
        QSKIP("glMapBufferRange not supported");

    QOpenGLBuffer buf;
    QVERIFY(buf.create());
    buf.bind();
    const char data[] = "some data";
    buf.allocate(data, sizeof(data));

    char *p = (char *) buf.mapRange(0, sizeof(data), QOpenGLBuffer::RangeRead | QOpenGLBuffer::RangeWrite);
    QVERIFY(p);
    QVERIFY(!strcmp(p, data));
    p[1] = 'O';
    buf.unmap();

    p = (char *) buf.mapRange(1, 2, QOpenGLBuffer::RangeWrite);
    QVERIFY(p);
    p[1] = 'M';
    buf.unmap();

    p = (char *) buf.mapRange(0, sizeof(data), QOpenGLBuffer::RangeRead);
    QVERIFY(!strcmp(p, "sOMe data"));
    buf.unmap();

    buf.destroy();
    ctx->doneCurrent();
}

void tst_QOpenGL::nullTextureInitializtion()
{
    QScopedPointer<QSurface> surface(createSurface(QSurface::Window));
    QOpenGLContext ctx;
    ctx.create();
    ctx.makeCurrent(surface.data());

    QImage i;
    QOpenGLTexture t(i);
    QVERIFY(!t.isCreated());
}

QTEST_MAIN(tst_QOpenGL)

#include "tst_qopengl.moc"
