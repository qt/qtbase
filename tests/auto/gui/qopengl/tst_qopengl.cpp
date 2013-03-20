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
    void fboRendering_data();
    void fboRendering();
    void fboHandleNulledAfterContextDestroyed();
    void openGLPaintDevice_data();
    void openGLPaintDevice();
    void aboutToBeDestroyed();
    void QTBUG15621_triangulatingStrokerDivZero();
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
        QOffscreenSurface *offscreenSurface = new QOffscreenSurface;
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
    ctx->create();
    ctx->makeCurrent(surface.data());

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
    ctx2->create();

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
        gl->create();
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
    ctx->create();
    ctx->makeCurrent(surface.data());

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
    ctx.create();

    ctx.makeCurrent(surface.data());

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    // No multisample with combined depth/stencil attachment:
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::NoAttachment);

    QOpenGLFramebufferObject *fbo = new QOpenGLFramebufferObject(200, 100, fboFormat);

    fbo->bind();

    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    QImage fb = fbo->toImage().convertToFormat(QImage::Format_RGB32);
    QImage reference(fb.size(), QImage::Format_RGB32);
    reference.fill(0xffff0000);

    QFUZZY_COMPARE_IMAGES(fb, reference);

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
    ctx.create();

    ctx.makeCurrent(surface.data());

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    // No multisample with combined depth/stencil attachment:
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

    // Uncomplicate things by using NPOT:
    QOpenGLFramebufferObject fbo(256, 128, fboFormat);

    if (fbo.attachment() != QOpenGLFramebufferObject::CombinedDepthStencil)
        QSKIP("FBOs missing combined depth~stencil support");

    fbo.bind();

    QPainter fboPainter;
    QOpenGLPaintDevice device(fbo.width(), fbo.height());
    bool painterBegun = fboPainter.begin(&device);
    QVERIFY(painterBegun);

    qt_opengl_draw_test_pattern(&fboPainter, fbo.width(), fbo.height());

    fboPainter.end();

    QImage fb = fbo.toImage().convertToFormat(QImage::Format_RGB32);

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
        ctx.create();

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
    ctx.create();

    QSurfaceFormat format = ctx.format();
    if (format.majorVersion() < 2)
        QSKIP("This test requires at least OpenGL 2.0");
    ctx.makeCurrent(surface.data());

    QImage image(128, 128, QImage::Format_RGB32);
    QPainter p(&image);
    p.fillRect(0, 0, image.width() / 2, image.height() / 2, Qt::red);
    p.fillRect(image.width() / 2, 0, image.width() / 2, image.height() / 2, Qt::green);
    p.fillRect(image.width() / 2, image.height() / 2, image.width() / 2, image.height() / 2, Qt::blue);
    p.fillRect(0, image.height() / 2, image.width() / 2, image.height() / 2, Qt::white);
    p.end();

    QOpenGLFramebufferObject fbo(128, 128);
    fbo.bind();

    QOpenGLPaintDevice device(128, 128);
    p.begin(&device);
    p.fillRect(0, 0, image.width() / 2, image.height() / 2, Qt::red);
    p.fillRect(image.width() / 2, 0, image.width() / 2, image.height() / 2, Qt::green);
    p.fillRect(image.width() / 2, image.height() / 2, image.width() / 2, image.height() / 2, Qt::blue);
    p.fillRect(0, image.height() / 2, image.width() / 2, image.height() / 2, Qt::white);
    p.end();

    QCOMPARE(image, fbo.toImage().convertToFormat(QImage::Format_RGB32));

    p.begin(&device);
    p.fillRect(0, 0, image.width(), image.height(), Qt::black);
    p.drawImage(0, 0, image);
    p.end();

    QCOMPARE(image, fbo.toImage().convertToFormat(QImage::Format_RGB32));

    p.begin(&device);
    p.fillRect(0, 0, image.width(), image.height(), Qt::black);
    p.fillRect(0, 0, image.width(), image.height(), QBrush(image));
    p.end();

    QCOMPARE(image, fbo.toImage().convertToFormat(QImage::Format_RGB32));
}

void tst_QOpenGL::aboutToBeDestroyed()
{
    QWindow window;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.setGeometry(0, 0, 128, 128);
    window.create();

    QOpenGLContext *context = new QOpenGLContext;
    QSignalSpy spy(context, SIGNAL(aboutToBeDestroyed()));

    context->create();
    context->makeCurrent(&window);

    QCOMPARE(spy.size(), 0);

    delete context;

    QCOMPARE(spy.size(), 1);
}

void tst_QOpenGL::QTBUG15621_triangulatingStrokerDivZero()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(__x86_64__)
    QSKIP("QTBUG-22617");
#endif

    QWindow window;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.setGeometry(0, 0, 128, 128);
    window.create();

    QOpenGLContext ctx;
    ctx.create();
    ctx.makeCurrent(&window);

    if (!QOpenGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QOpenGLFramebufferObject not supported on this platform");

    QOpenGLFramebufferObject fbo(128, 128);
    fbo.bind();

    QOpenGLPaintDevice device(128, 128);

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
    p.fillRect(QRect(0, 0, 128, 128), Qt::blue);
    p.strokePath(path, pen);
    p.end();
    QImage image = fbo.toImage().convertToFormat(QImage::Format_RGB32);

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


QTEST_MAIN(tst_QOpenGL)

#include "tst_qopengl.moc"
