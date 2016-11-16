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

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qgl.h>
#include <qglpixelbuffer.h>
#include <qglframebufferobject.h>
#include <qglcolormap.h>
#include <qpaintengine.h>
#include <qopenglfunctions.h>
#include <qopenglframebufferobject.h>
#include <qopenglpaintdevice.h>

#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QVBoxLayout>

#ifdef QT_BUILD_INTERNAL
#include <qpa/qplatformpixmap.h>
#include <QtOpenGL/private/qgl_p.h>
#include <QtGui/private/qimage_p.h>
#include <QtGui/private/qimagepixmapcleanuphooks_p.h>
#include <QtGui/private/qopenglextensions_p.h>
#endif

class tst_QGL : public QObject
{
Q_OBJECT

public:
    tst_QGL();
    virtual ~tst_QGL();

private slots:
    void initTestCase();
    void getSetCheck();
#ifdef QT_BUILD_INTERNAL
    void qglContextDefaultBindTexture();
    void openGLVersionCheck();
    void shareRegister();
    void textureCleanup();
#endif
    void partialGLWidgetUpdates_data();
    void partialGLWidgetUpdates();
    void glWidgetWithAlpha();
    void glWidgetRendering();
    void glFBOSimpleRendering();
    void glFBORendering();
    void currentFboSync();
    void multipleFBOInterleavedRendering();
    void glFBOUseInGLWidget();
    void glPBufferRendering();
    void glWidgetReparent();
    void glWidgetRenderPixmap();
    void colormap();
    void fboFormat();
    void testDontCrashOnDanglingResources();
    void replaceClipping();
    void clipTest();
    void destroyFBOAfterContext();
    void threadImages();
    void nullRectCrash();
    void graphicsViewClipping();
    void extensions();
};

tst_QGL::tst_QGL()
{
}

tst_QGL::~tst_QGL()
{
}

void tst_QGL::initTestCase()
{
    QGLWidget glWidget;
    if (!glWidget.isValid())
        QSKIP("QGL is not supported on the test system");
}

class MyGLContext : public QGLContext
{
public:
    MyGLContext(const QGLFormat& format) : QGLContext(format) {}
    bool windowCreated() const { return QGLContext::windowCreated(); }
    void setWindowCreated(bool on) { QGLContext::setWindowCreated(on); }
    bool initialized() const { return QGLContext::initialized(); }
    void setInitialized(bool on) { QGLContext::setInitialized(on); }
};

class MyGLWidget : public QGLWidget
{
public:
    MyGLWidget() : QGLWidget() {}
    bool autoBufferSwap() const { return QGLWidget::autoBufferSwap(); }
    void setAutoBufferSwap(bool on) { QGLWidget::setAutoBufferSwap(on); }
};

static int appDefaultDepth()
{
    static int depth = 0;
    if (depth == 0) {
        QPixmap pm(1, 1);
        depth = pm.depth();
    }
    return depth;
}

// Using INT_MIN and INT_MAX will cause failures on systems
// where "int" is 64-bit, so use the explicit values instead.
#define TEST_INT_MIN    (-2147483647 - 1)
#define TEST_INT_MAX    2147483647

// Testing get/set functions
void tst_QGL::getSetCheck()
{
    QGLFormat obj1;
    // int QGLFormat::depthBufferSize()
    // void QGLFormat::setDepthBufferSize(int)
    QCOMPARE(-1, obj1.depthBufferSize());
    obj1.setDepthBufferSize(0);
    QCOMPARE(0, obj1.depthBufferSize());
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setDepthBufferSize: Cannot set negative depth buffer size -2147483648");
    obj1.setDepthBufferSize(TEST_INT_MIN);
    QCOMPARE(0, obj1.depthBufferSize()); // Makes no sense with a negative buffer size
    obj1.setDepthBufferSize(3);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setDepthBufferSize: Cannot set negative depth buffer size -1");
    obj1.setDepthBufferSize(-1);
    QCOMPARE(3, obj1.depthBufferSize());
    obj1.setDepthBufferSize(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.depthBufferSize());

    // int QGLFormat::accumBufferSize()
    // void QGLFormat::setAccumBufferSize(int)
    QCOMPARE(-1, obj1.accumBufferSize());
    obj1.setAccumBufferSize(0);
    QCOMPARE(0, obj1.accumBufferSize());
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setAccumBufferSize: Cannot set negative accumulate buffer size -2147483648");
    obj1.setAccumBufferSize(TEST_INT_MIN);
    QCOMPARE(0, obj1.accumBufferSize()); // Makes no sense with a negative buffer size
    obj1.setAccumBufferSize(3);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setAccumBufferSize: Cannot set negative accumulate buffer size -1");
    obj1.setAccumBufferSize(-1);
    QCOMPARE(3, obj1.accumBufferSize());
    obj1.setAccumBufferSize(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.accumBufferSize());

    // int QGLFormat::redBufferSize()
    // void QGLFormat::setRedBufferSize(int)
    QCOMPARE(-1, obj1.redBufferSize());
    obj1.setRedBufferSize(0);
    QCOMPARE(0, obj1.redBufferSize());
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setRedBufferSize: Cannot set negative red buffer size -2147483648");
    obj1.setRedBufferSize(TEST_INT_MIN);
    QCOMPARE(0, obj1.redBufferSize()); // Makes no sense with a negative buffer size
    obj1.setRedBufferSize(3);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setRedBufferSize: Cannot set negative red buffer size -1");
    obj1.setRedBufferSize(-1);
    QCOMPARE(3, obj1.redBufferSize());
    obj1.setRedBufferSize(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.redBufferSize());

    // int QGLFormat::greenBufferSize()
    // void QGLFormat::setGreenBufferSize(int)
    QCOMPARE(-1, obj1.greenBufferSize());
    obj1.setGreenBufferSize(0);
    QCOMPARE(0, obj1.greenBufferSize());
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setGreenBufferSize: Cannot set negative green buffer size -2147483648");
    obj1.setGreenBufferSize(TEST_INT_MIN);
    QCOMPARE(0, obj1.greenBufferSize()); // Makes no sense with a negative buffer size
    obj1.setGreenBufferSize(3);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setGreenBufferSize: Cannot set negative green buffer size -1");
    obj1.setGreenBufferSize(-1);
    QCOMPARE(3, obj1.greenBufferSize());
    obj1.setGreenBufferSize(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.greenBufferSize());

    // int QGLFormat::blueBufferSize()
    // void QGLFormat::setBlueBufferSize(int)
    QCOMPARE(-1, obj1.blueBufferSize());
    obj1.setBlueBufferSize(0);
    QCOMPARE(0, obj1.blueBufferSize());
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setBlueBufferSize: Cannot set negative blue buffer size -2147483648");
    obj1.setBlueBufferSize(TEST_INT_MIN);
    QCOMPARE(0, obj1.blueBufferSize()); // Makes no sense with a negative buffer size
    obj1.setBlueBufferSize(3);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setBlueBufferSize: Cannot set negative blue buffer size -1");
    obj1.setBlueBufferSize(-1);
    QCOMPARE(3, obj1.blueBufferSize());
    obj1.setBlueBufferSize(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.blueBufferSize());

    // int QGLFormat::alphaBufferSize()
    // void QGLFormat::setAlphaBufferSize(int)
    QCOMPARE(-1, obj1.alphaBufferSize());
    QCOMPARE(false, obj1.alpha());
    QVERIFY(!obj1.testOption(QGL::AlphaChannel));
    QVERIFY(obj1.testOption(QGL::NoAlphaChannel));
    obj1.setAlphaBufferSize(1);
    QCOMPARE(true, obj1.alpha());   // setAlphaBufferSize() enables alpha.
    QCOMPARE(1, obj1.alphaBufferSize());
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setAlphaBufferSize: Cannot set negative alpha buffer size -2147483648");
    obj1.setAlphaBufferSize(TEST_INT_MIN);
    QCOMPARE(1, obj1.alphaBufferSize()); // Makes no sense with a negative buffer size
    obj1.setAlphaBufferSize(3);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setAlphaBufferSize: Cannot set negative alpha buffer size -1");
    obj1.setAlphaBufferSize(-1);
    QCOMPARE(3, obj1.alphaBufferSize());
    obj1.setAlphaBufferSize(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.alphaBufferSize());

    // int QGLFormat::stencilBufferSize()
    // void QGLFormat::setStencilBufferSize(int)
    QCOMPARE(-1, obj1.stencilBufferSize());
    obj1.setStencilBufferSize(1);
    QCOMPARE(1, obj1.stencilBufferSize());
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setStencilBufferSize: Cannot set negative stencil buffer size -2147483648");
    obj1.setStencilBufferSize(TEST_INT_MIN);
    QCOMPARE(1, obj1.stencilBufferSize()); // Makes no sense with a negative buffer size
    obj1.setStencilBufferSize(3);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setStencilBufferSize: Cannot set negative stencil buffer size -1");
    obj1.setStencilBufferSize(-1);
    QCOMPARE(3, obj1.stencilBufferSize());
    obj1.setStencilBufferSize(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.stencilBufferSize());

    // bool QGLFormat::sampleBuffers()
    // void QGLFormat::setSampleBuffers(bool)
    QCOMPARE(false, obj1.sampleBuffers());
    QVERIFY(!obj1.testOption(QGL::SampleBuffers));
    QVERIFY(obj1.testOption(QGL::NoSampleBuffers));

    obj1.setSampleBuffers(false);
    QCOMPARE(false, obj1.sampleBuffers());
    QVERIFY(obj1.testOption(QGL::NoSampleBuffers));
    obj1.setSampleBuffers(true);
    QCOMPARE(true, obj1.sampleBuffers());
    QVERIFY(obj1.testOption(QGL::SampleBuffers));

    // int QGLFormat::samples()
    // void QGLFormat::setSamples(int)
    QCOMPARE(-1, obj1.samples());
    obj1.setSamples(0);
    QCOMPARE(0, obj1.samples());
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setSamples: Cannot have negative number of samples per pixel -2147483648");
    obj1.setSamples(TEST_INT_MIN);
    QCOMPARE(0, obj1.samples());  // Makes no sense with a negative sample size
    obj1.setSamples(3);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setSamples: Cannot have negative number of samples per pixel -1");
    obj1.setSamples(-1);
    QCOMPARE(3, obj1.samples());
    obj1.setSamples(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.samples());

    // int QGLFormat::swapInterval()
    // void QGLFormat::setSwapInterval(int)
    QCOMPARE(-1, obj1.swapInterval());
    obj1.setSwapInterval(0);
    QCOMPARE(0, obj1.swapInterval());
    obj1.setSwapInterval(TEST_INT_MIN);
    QCOMPARE(TEST_INT_MIN, obj1.swapInterval());
    obj1.setSwapInterval(-1);
    QCOMPARE(-1, obj1.swapInterval());
    obj1.setSwapInterval(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.swapInterval());

    // bool QGLFormat::doubleBuffer()
    // void QGLFormat::setDoubleBuffer(bool)
    QCOMPARE(true, obj1.doubleBuffer());
    QVERIFY(obj1.testOption(QGL::DoubleBuffer));
    QVERIFY(!obj1.testOption(QGL::SingleBuffer));
    obj1.setDoubleBuffer(false);
    QCOMPARE(false, obj1.doubleBuffer());
    QVERIFY(!obj1.testOption(QGL::DoubleBuffer));
    QVERIFY(obj1.testOption(QGL::SingleBuffer));
    obj1.setDoubleBuffer(true);
    QCOMPARE(true, obj1.doubleBuffer());
    QVERIFY(obj1.testOption(QGL::DoubleBuffer));
    QVERIFY(!obj1.testOption(QGL::SingleBuffer));

    // bool QGLFormat::depth()
    // void QGLFormat::setDepth(bool)
    QCOMPARE(true, obj1.depth());
    QVERIFY(obj1.testOption(QGL::DepthBuffer));
    QVERIFY(!obj1.testOption(QGL::NoDepthBuffer));
    obj1.setDepth(false);
    QCOMPARE(false, obj1.depth());
    QVERIFY(!obj1.testOption(QGL::DepthBuffer));
    QVERIFY(obj1.testOption(QGL::NoDepthBuffer));
    obj1.setDepth(true);
    QCOMPARE(true, obj1.depth());
    QVERIFY(obj1.testOption(QGL::DepthBuffer));
    QVERIFY(!obj1.testOption(QGL::NoDepthBuffer));

    // bool QGLFormat::rgba()
    // void QGLFormat::setRgba(bool)
    QCOMPARE(true, obj1.rgba());
    QVERIFY(obj1.testOption(QGL::Rgba));
    QVERIFY(!obj1.testOption(QGL::ColorIndex));
    obj1.setRgba(false);
    QCOMPARE(false, obj1.rgba());
    QVERIFY(!obj1.testOption(QGL::Rgba));
    QVERIFY(obj1.testOption(QGL::ColorIndex));
    obj1.setRgba(true);
    QCOMPARE(true, obj1.rgba());
    QVERIFY(obj1.testOption(QGL::Rgba));
    QVERIFY(!obj1.testOption(QGL::ColorIndex));

    // bool QGLFormat::alpha()
    // void QGLFormat::setAlpha(bool)
    QVERIFY(obj1.testOption(QGL::AlphaChannel));
    QVERIFY(!obj1.testOption(QGL::NoAlphaChannel));
    obj1.setAlpha(false);
    QCOMPARE(false, obj1.alpha());
    QVERIFY(!obj1.testOption(QGL::AlphaChannel));
    QVERIFY(obj1.testOption(QGL::NoAlphaChannel));
    obj1.setAlpha(true);
    QCOMPARE(true, obj1.alpha());
    QVERIFY(obj1.testOption(QGL::AlphaChannel));
    QVERIFY(!obj1.testOption(QGL::NoAlphaChannel));

    // bool QGLFormat::accum()
    // void QGLFormat::setAccum(bool)
    obj1.setAccumBufferSize(0);
    QCOMPARE(false, obj1.accum());
    QVERIFY(!obj1.testOption(QGL::AccumBuffer));
    QVERIFY(obj1.testOption(QGL::NoAccumBuffer));
    obj1.setAccum(false);
    QCOMPARE(false, obj1.accum());
    QVERIFY(!obj1.testOption(QGL::AccumBuffer));
    QVERIFY(obj1.testOption(QGL::NoAccumBuffer));
    obj1.setAccum(true);
    QCOMPARE(true, obj1.accum());
    QVERIFY(obj1.testOption(QGL::AccumBuffer));
    QVERIFY(!obj1.testOption(QGL::NoAccumBuffer));

    // bool QGLFormat::stencil()
    // void QGLFormat::setStencil(bool)
    QCOMPARE(true, obj1.stencil());
    QVERIFY(obj1.testOption(QGL::StencilBuffer));
    QVERIFY(!obj1.testOption(QGL::NoStencilBuffer));
    obj1.setStencil(false);
    QCOMPARE(false, obj1.stencil());
    QVERIFY(!obj1.testOption(QGL::StencilBuffer));
    QVERIFY(obj1.testOption(QGL::NoStencilBuffer));
    obj1.setStencil(true);
    QCOMPARE(true, obj1.stencil());
    QVERIFY(obj1.testOption(QGL::StencilBuffer));
    QVERIFY(!obj1.testOption(QGL::NoStencilBuffer));

    // bool QGLFormat::stereo()
    // void QGLFormat::setStereo(bool)
    QCOMPARE(false, obj1.stereo());
    QVERIFY(!obj1.testOption(QGL::StereoBuffers));
    QVERIFY(obj1.testOption(QGL::NoStereoBuffers));
    obj1.setStereo(false);
    QCOMPARE(false, obj1.stereo());
    QVERIFY(!obj1.testOption(QGL::StereoBuffers));
    QVERIFY(obj1.testOption(QGL::NoStereoBuffers));
    obj1.setStereo(true);
    QCOMPARE(true, obj1.stereo());
    QVERIFY(obj1.testOption(QGL::StereoBuffers));
    QVERIFY(!obj1.testOption(QGL::NoStereoBuffers));

    // bool QGLFormat::directRendering()
    // void QGLFormat::setDirectRendering(bool)
    QCOMPARE(true, obj1.directRendering());
    QVERIFY(obj1.testOption(QGL::DirectRendering));
    QVERIFY(!obj1.testOption(QGL::IndirectRendering));
    obj1.setDirectRendering(false);
    QCOMPARE(false, obj1.directRendering());
    QVERIFY(!obj1.testOption(QGL::DirectRendering));
    QVERIFY(obj1.testOption(QGL::IndirectRendering));
    obj1.setDirectRendering(true);
    QCOMPARE(true, obj1.directRendering());
    QVERIFY(obj1.testOption(QGL::DirectRendering));
    QVERIFY(!obj1.testOption(QGL::IndirectRendering));

    // bool QGLFormat::overlay()
    // void QGLFormat::setOverlay(bool)
    QCOMPARE(false, obj1.hasOverlay());
    QVERIFY(!obj1.testOption(QGL::HasOverlay));
    QVERIFY(obj1.testOption(QGL::NoOverlay));
    obj1.setOverlay(false);
    QCOMPARE(false, obj1.hasOverlay());
    QVERIFY(!obj1.testOption(QGL::HasOverlay));
    QVERIFY(obj1.testOption(QGL::NoOverlay));
    obj1.setOverlay(true);
    QCOMPARE(true, obj1.hasOverlay());
    QVERIFY(obj1.testOption(QGL::HasOverlay));
    QVERIFY(!obj1.testOption(QGL::NoOverlay));

    // int QGLFormat::plane()
    // void QGLFormat::setPlane(int)
    QCOMPARE(0, obj1.plane());
    obj1.setPlane(0);
    QCOMPARE(0, obj1.plane());
    obj1.setPlane(TEST_INT_MIN);
    QCOMPARE(TEST_INT_MIN, obj1.plane());
    obj1.setPlane(TEST_INT_MAX);
    QCOMPARE(TEST_INT_MAX, obj1.plane());

    // int QGLFormat::major/minorVersion()
    // void QGLFormat::setVersion(int, int)
    QCOMPARE(obj1.majorVersion(), 2);
    QCOMPARE(obj1.minorVersion(), 0);
    obj1.setVersion(3, 2);
    QCOMPARE(obj1.majorVersion(), 3);
    QCOMPARE(obj1.minorVersion(), 2);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setVersion: Cannot set zero or negative version number 0.1");
    obj1.setVersion(0, 1);
    QCOMPARE(obj1.majorVersion(), 3);
    QCOMPARE(obj1.minorVersion(), 2);
    QTest::ignoreMessage(QtWarningMsg, "QGLFormat::setVersion: Cannot set zero or negative version number 3.-1");
    obj1.setVersion(3, -1);
    QCOMPARE(obj1.majorVersion(), 3);
    QCOMPARE(obj1.minorVersion(), 2);
    obj1.setVersion(TEST_INT_MAX, TEST_INT_MAX - 1);
    QCOMPARE(obj1.majorVersion(), TEST_INT_MAX);
    QCOMPARE(obj1.minorVersion(), TEST_INT_MAX - 1);


    // operator== and operator!= for QGLFormat
    QGLFormat format1;
    QGLFormat format2;

    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));
    format1.setDoubleBuffer(false);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setDoubleBuffer(false);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setDepthBufferSize(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setDepthBufferSize(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setAccumBufferSize(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setAccumBufferSize(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setRedBufferSize(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setRedBufferSize(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setGreenBufferSize(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setGreenBufferSize(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setBlueBufferSize(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setBlueBufferSize(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setAlphaBufferSize(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setAlphaBufferSize(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setStencilBufferSize(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setStencilBufferSize(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setSamples(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setSamples(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setSwapInterval(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setSwapInterval(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setPlane(8);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setPlane(8);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setVersion(3, 2);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setVersion(3, 2);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setProfile(QGLFormat::CoreProfile);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setProfile(QGLFormat::CoreProfile);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    format1.setOption(QGL::NoDeprecatedFunctions);
    QVERIFY(!(format1 == format2));
    QVERIFY(format1 != format2);
    format2.setOption(QGL::NoDeprecatedFunctions);
    QCOMPARE(format1, format2);
    QVERIFY(!(format1 != format2));

    // Copy constructor and assignment for QGLFormat.
    QGLFormat format3(format1);
    QGLFormat format4;
    QCOMPARE(format1, format3);
    QVERIFY(format1 != format4);
    format4 = format1;
    QCOMPARE(format1, format4);

    // Check that modifying a copy doesn't affect the original.
    format3.setRedBufferSize(16);
    format4.setPlane(16);
    QCOMPARE(format1.redBufferSize(), 8);
    QCOMPARE(format1.plane(), 8);

    // Check the QGLFormat constructor that takes an option list.
    QGLFormat format5
        (QGL::DepthBuffer | QGL::StereoBuffers | QGL::ColorIndex, 3);
    QVERIFY(format5.depth());
    QVERIFY(format5.stereo());
    QVERIFY(format5.doubleBuffer());        // From defaultFormat()
    QVERIFY(!format5.hasOverlay());         // From defaultFormat()
    QVERIFY(!format5.rgba());
    QCOMPARE(format5.plane(), 3);

    // The default format should be the same as QGLFormat().
    QCOMPARE(QGLFormat::defaultFormat(), QGLFormat());

    // Modify the default format and check that it was changed.
    QGLFormat::setDefaultFormat(format1);
    QCOMPARE(QGLFormat::defaultFormat(), format1);

    // Restore the default format.
    QGLFormat::setDefaultFormat(QGLFormat());
    QCOMPARE(QGLFormat::defaultFormat(), QGLFormat());

    // Check the default overlay format's expected values.
    QGLFormat overlay(QGLFormat::defaultOverlayFormat());
    QCOMPARE(overlay.depthBufferSize(), -1);
    QCOMPARE(overlay.accumBufferSize(), -1);
    QCOMPARE(overlay.redBufferSize(), -1);
    QCOMPARE(overlay.greenBufferSize(), -1);
    QCOMPARE(overlay.blueBufferSize(), -1);
    QCOMPARE(overlay.alphaBufferSize(), -1);
    QCOMPARE(overlay.samples(), -1);
    QCOMPARE(overlay.swapInterval(), -1);
    QCOMPARE(overlay.plane(), 1);
    QVERIFY(!overlay.sampleBuffers());
    QVERIFY(!overlay.doubleBuffer());
    QVERIFY(!overlay.depth());
    QVERIFY(!overlay.rgba());
    QVERIFY(!overlay.alpha());
    QVERIFY(!overlay.accum());
    QVERIFY(!overlay.stencil());
    QVERIFY(!overlay.stereo());
    QVERIFY(overlay.directRendering()); // Only option that should be on.
    QVERIFY(!overlay.hasOverlay());     // Overlay doesn't need an overlay!

    // Modify the default overlay format and check that it was changed.
    QGLFormat::setDefaultOverlayFormat(format1);
    QCOMPARE(QGLFormat::defaultOverlayFormat(), format1);

    // Restore the default overlay format.
    QGLFormat::setDefaultOverlayFormat(overlay);
    QCOMPARE(QGLFormat::defaultOverlayFormat(), overlay);

    MyGLContext obj2(obj1);
    // bool QGLContext::windowCreated()
    // void QGLContext::setWindowCreated(bool)
    obj2.setWindowCreated(false);
    QCOMPARE(false, obj2.windowCreated());
    obj2.setWindowCreated(true);
    QCOMPARE(true, obj2.windowCreated());

    // bool QGLContext::initialized()
    // void QGLContext::setInitialized(bool)
    obj2.setInitialized(false);
    QCOMPARE(false, obj2.initialized());
    obj2.setInitialized(true);
    QCOMPARE(true, obj2.initialized());

    MyGLWidget obj3;
    // bool QGLWidget::autoBufferSwap()
    // void QGLWidget::setAutoBufferSwap(bool)
    obj3.setAutoBufferSwap(false);
    QCOMPARE(false, obj3.autoBufferSwap());
    obj3.setAutoBufferSwap(true);
    QCOMPARE(true, obj3.autoBufferSwap());
}

#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
extern QGLFormat::OpenGLVersionFlags qOpenGLVersionFlagsFromString(const QString &versionString);
QT_END_NAMESPACE
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QGL::openGLVersionCheck()
{
    QString versionString;
    QGLFormat::OpenGLVersionFlags expectedFlag;
    QGLFormat::OpenGLVersionFlags versionFlag;

    versionString = "1.1 Irix 6.5";
    expectedFlag = QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "1.2 Microsoft";
    expectedFlag = QGLFormat::OpenGL_Version_1_2 | QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "1.2.1";
    expectedFlag = QGLFormat::OpenGL_Version_1_2 | QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "1.3 NVIDIA";
    expectedFlag = QGLFormat::OpenGL_Version_1_3 | QGLFormat::OpenGL_Version_1_2 | QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "1.4";
    expectedFlag = QGLFormat::OpenGL_Version_1_4 | QGLFormat::OpenGL_Version_1_3 | QGLFormat::OpenGL_Version_1_2 | QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "1.5 NVIDIA";
    expectedFlag = QGLFormat::OpenGL_Version_1_5 | QGLFormat::OpenGL_Version_1_4 | QGLFormat::OpenGL_Version_1_3 | QGLFormat::OpenGL_Version_1_2 | QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "2.0.2 NVIDIA 87.62";
    expectedFlag = QGLFormat::OpenGL_Version_2_0 | QGLFormat::OpenGL_Version_1_5 | QGLFormat::OpenGL_Version_1_4 | QGLFormat::OpenGL_Version_1_3 | QGLFormat::OpenGL_Version_1_2 | QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "2.1 NVIDIA";
    expectedFlag = QGLFormat::OpenGL_Version_2_1 | QGLFormat::OpenGL_Version_2_0 | QGLFormat::OpenGL_Version_1_5 | QGLFormat::OpenGL_Version_1_4 | QGLFormat::OpenGL_Version_1_3 | QGLFormat::OpenGL_Version_1_2 | QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "2.1";
    expectedFlag = QGLFormat::OpenGL_Version_2_1 | QGLFormat::OpenGL_Version_2_0 | QGLFormat::OpenGL_Version_1_5 | QGLFormat::OpenGL_Version_1_4 | QGLFormat::OpenGL_Version_1_3 | QGLFormat::OpenGL_Version_1_2 | QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "OpenGL ES-CM 1.0 ATI";
    expectedFlag = QGLFormat::OpenGL_ES_Common_Version_1_0 | QGLFormat::OpenGL_ES_CommonLite_Version_1_0;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "OpenGL ES-CL 1.0 ATI";
    expectedFlag = QGLFormat::OpenGL_ES_CommonLite_Version_1_0;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "OpenGL ES-CM 1.1 ATI";
    expectedFlag = QGLFormat::OpenGL_ES_Common_Version_1_1 | QGLFormat::OpenGL_ES_CommonLite_Version_1_1 | QGLFormat::OpenGL_ES_Common_Version_1_0 | QGLFormat::OpenGL_ES_CommonLite_Version_1_0;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "OpenGL ES-CL 1.1 ATI";
    expectedFlag = QGLFormat::OpenGL_ES_CommonLite_Version_1_1 | QGLFormat::OpenGL_ES_CommonLite_Version_1_0;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "OpenGL ES 2.0 ATI";
    expectedFlag = QGLFormat::OpenGL_ES_Version_2_0;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    versionString = "3.0";
    expectedFlag = QGLFormat::OpenGL_Version_3_0 | QGLFormat::OpenGL_Version_2_1 | QGLFormat::OpenGL_Version_2_0 | QGLFormat::OpenGL_Version_1_5 | QGLFormat::OpenGL_Version_1_4 | QGLFormat::OpenGL_Version_1_3 | QGLFormat::OpenGL_Version_1_2 | QGLFormat::OpenGL_Version_1_1;
    versionFlag = qOpenGLVersionFlagsFromString(versionString);
    QCOMPARE(versionFlag, expectedFlag);

    QGLWidget glWidget;
    glWidget.show();
    glWidget.makeCurrent();

    // This is unfortunately the only test we can make on the actual openGLVersionFlags()
    // However, the complicated parts are in openGLVersionFlags(const QString &versionString)
    // tested above

#if defined(QT_OPENGL_ES_2)
    QVERIFY(QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_ES_Version_2_0);
#else
    if (QOpenGLContext::currentContext()->isOpenGLES())
        QVERIFY(QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_ES_Version_2_0);
    else
        QVERIFY(QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_1_1);
#endif //defined(QT_OPENGL_ES_2)
}
#endif //QT_BUILD_INTERNAL

static bool fuzzyComparePixels(const QRgb testPixel, const QRgb refPixel, const char* file, int line, int x = -1, int y = -1)
{
    static int maxFuzz = 1;
    static bool maxFuzzSet = false;

    // On 16 bpp systems, we need to allow for more fuzz:
    if (!maxFuzzSet) {
        maxFuzzSet = true;
        if (appDefaultDepth() < 24)
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

class UnclippedWidget : public QWidget
{
public:
    bool painted;

    UnclippedWidget()
        : painted(false)
    {
    }

    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.fillRect(rect().adjusted(-1000, -1000, 1000, 1000), Qt::black);

        painted = true;
    }
};

void tst_QGL::graphicsViewClipping()
{
    const int size = 64;
    UnclippedWidget *widget = new UnclippedWidget;
    widget->setFixedSize(size, size);

    QGraphicsScene scene;

    scene.addWidget(widget)->setPos(0, 0);

    QGraphicsView view(&scene);
    // Use Qt::Tool as fully decorated windows have a minimum width of 160 on Windows.
    view.setWindowFlags(view.windowFlags() | Qt::Tool);
    view.setBackgroundBrush(Qt::white);
    view.resize(2*size, 2*size);

    QGLWidget *viewport = new QGLWidget;
    view.setViewport(viewport);
    view.show();
    qApp->setActiveWindow(&view);

    if (!viewport->isValid())
        return;

    scene.setSceneRect(view.viewport()->rect());

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    #ifdef Q_OS_MAC
        // The black rectangle jumps from the center to the upper left for some reason.
        QTest::qWait(100);
    #endif

    QTRY_VERIFY(widget->painted);

    QImage image = viewport->grabFrameBuffer();
    QImage expected = image;

    QPainter p(&expected);
    p.fillRect(expected.rect(), Qt::white);
    p.fillRect(QRect(0, 0, size, size), Qt::black);
    p.end();

    QFUZZY_COMPARE_IMAGES(image, expected);
}

void tst_QGL::partialGLWidgetUpdates_data()
{
    QTest::addColumn<bool>("doubleBufferedContext");
    QTest::addColumn<bool>("autoFillBackground");
    QTest::addColumn<bool>("supportsPartialUpdates");

    QTest::newRow("Double buffered context") << true << true << false;
    QTest::newRow("Double buffered context without auto-fill background") << true << false << false;
    QTest::newRow("Single buffered context") << false << true << false;
    QTest::newRow("Single buffered context without auto-fill background") << false << false << true;
}

void tst_QGL::partialGLWidgetUpdates()
{
    QFETCH(bool, doubleBufferedContext);
    QFETCH(bool, autoFillBackground);
    QFETCH(bool, supportsPartialUpdates);

    class MyGLWidget : public QGLWidget
    {
        public:
            QRegion paintEventRegion;
            void paintEvent(QPaintEvent *e)
            {
                paintEventRegion = e->region();
            }
    };

    QGLFormat format = QGLFormat::defaultFormat();
    format.setDoubleBuffer(doubleBufferedContext);
    QGLFormat::setDefaultFormat(format);

    MyGLWidget widget;
    widget.setFixedSize(150, 150);
    widget.setAutoFillBackground(autoFillBackground);
    widget.show();

    QTest::qWait(200);

    if (widget.format().doubleBuffer() != doubleBufferedContext)
        QSKIP("Platform does not support requested format");

    widget.paintEventRegion = QRegion();
    widget.repaint(50, 50, 50, 50);

    if (supportsPartialUpdates)
        QCOMPARE(widget.paintEventRegion, QRegion(50, 50, 50, 50));
    else
        QCOMPARE(widget.paintEventRegion, QRegion(widget.rect()));
}


// This tests that rendering to a QGLPBuffer using QPainter works.
void tst_QGL::glPBufferRendering()
{
    if (!QGLPixelBuffer::hasOpenGLPbuffers())
        QSKIP("QGLPixelBuffer not supported on this platform");

    QGLPixelBuffer* pbuf = new QGLPixelBuffer(128, 128);

    QPainter p;
    bool begun = p.begin(pbuf);
    QVERIFY(begun);

    QPaintEngine::Type engineType = p.paintEngine()->type();
    QVERIFY(engineType == QPaintEngine::OpenGL || engineType == QPaintEngine::OpenGL2);

    p.fillRect(0, 0, 128, 128, Qt::red);
    p.fillRect(32, 32, 64, 64, Qt::blue);
    p.end();

    QImage fb = pbuf->toImage();
    delete pbuf;

    QImage reference(128, 128, fb.format());
    p.begin(&reference);
    p.fillRect(0, 0, 128, 128, Qt::red);
    p.fillRect(32, 32, 64, 64, Qt::blue);
    p.end();

    QFUZZY_COMPARE_IMAGES(fb, reference);
}

void tst_QGL::glWidgetWithAlpha()
{
    QGLWidget* w = new QGLWidget(QGLFormat(QGL::AlphaChannel));
    w->show();
    QVERIFY(QTest::qWaitForWindowExposed(w));

    delete w;
}


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

class GLWidget : public QGLWidget
{
public:
    GLWidget(QWidget* p = 0)
            : QGLWidget(p), beginOk(false), engineType(QPaintEngine::MaxUser) {}
    bool beginOk;
    QPaintEngine::Type engineType;
    void paintGL()
    {
        QPainter p;
        beginOk = p.begin(this);
        QPaintEngine* pe = p.paintEngine();
        engineType = pe->type();

        qt_opengl_draw_test_pattern(&p, width(), height());

        // No p.end() or swap buffers, should be done automatically
    }

};

void tst_QGL::glWidgetRendering()
{
    GLWidget w;
    w.resize(256, 128);
    w.show();

    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QVERIFY(w.beginOk);
    QVERIFY(w.engineType == QPaintEngine::OpenGL || w.engineType == QPaintEngine::OpenGL2);

#if defined(Q_OS_QNX)
    // glReadPixels reads from the back buffer. On QNX the buffer is not preserved
    // after a buffer swap. This is why we have to swap the buffer explicitly before calling
    // grabFrameBuffer to retrieve the content of the front buffer.
    w.swapBuffers();
#endif
    QImage fb = w.grabFrameBuffer(false);
    qt_opengl_check_test_pattern(fb);
}

void tst_QGL::glFBOSimpleRendering()
{
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QGLFramebufferObject not supported on this platform");

    QGLWidget glw;
    glw.makeCurrent();

    // No multisample with combined depth/stencil attachment:
    QGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QGLFramebufferObject::NoAttachment);

    QGLFramebufferObject *fbo = new QGLFramebufferObject(200, 100, fboFormat);

    fbo->bind();

    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    funcs->glClearColor(1.0, 0.0, 0.0, 1.0);
    funcs->glClear(GL_COLOR_BUFFER_BIT);
    funcs->glFinish();

    QImage fb = fbo->toImage().convertToFormat(QImage::Format_RGB32);
    QImage reference(fb.size(), QImage::Format_RGB32);
    reference.fill(0xffff0000);

    QFUZZY_COMPARE_IMAGES(fb, reference);

    delete fbo;
}

// NOTE: This tests that CombinedDepthStencil attachment works by assuming the
//       GL2 engine is being used and is implemented the same way as it was when
//       this autotest was written. If this is not the case, there may be some
//       false-positives: I.e. The test passes when either the depth or stencil
//       buffer is actually missing. But that's probably ok anyway.
void tst_QGL::glFBORendering()
{
#if defined(Q_OS_QNX)
    QSKIP("Reading the QGLFramebufferObject is unsupported on this platform");
#endif
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QGLFramebufferObject not supported on this platform");

    QGLWidget glw;
    glw.makeCurrent();

    // No multisample with combined depth/stencil attachment:
    QGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QGLFramebufferObject::CombinedDepthStencil);

    // Don't complicate things by using NPOT:
    QGLFramebufferObject *fbo = new QGLFramebufferObject(256, 128, fboFormat);

    if (fbo->attachment() != QGLFramebufferObject::CombinedDepthStencil) {
        delete fbo;
        QSKIP("FBOs missing combined depth~stencil support");
    }

    QPainter fboPainter;
    bool painterBegun = fboPainter.begin(fbo);
    QVERIFY(painterBegun);

    qt_opengl_draw_test_pattern(&fboPainter, fbo->width(), fbo->height());

    fboPainter.end();

    QImage fb = fbo->toImage().convertToFormat(QImage::Format_RGB32);
    delete fbo;

    qt_opengl_check_test_pattern(fb);
}

class QOpenGLFramebufferObjectPaintDevice : public QOpenGLPaintDevice
{
public:
    QOpenGLFramebufferObjectPaintDevice(int width, int height)
        : QOpenGLPaintDevice(width, height)
        , m_fbo(width, height, QOpenGLFramebufferObject::CombinedDepthStencil)
    {
    }

    void ensureActiveTarget()
    {
        m_fbo.bind();
    }

    QImage toImage() const
    {
        return m_fbo.toImage();
    }

private:
    QOpenGLFramebufferObject m_fbo;
};

void tst_QGL::currentFboSync()
{
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QGLFramebufferObject not supported on this platform");

#if defined(Q_OS_QNX)
    QSKIP("Reading the QGLFramebufferObject is unsupported on this platform");
#endif

    QGLWidget glw;
    glw.makeCurrent();

    {
        QGLFramebufferObject fbo1(256, 256, QGLFramebufferObject::CombinedDepthStencil);

        QOpenGLFramebufferObjectPaintDevice fbo2(256, 256);

        QImage sourceImage(256, 256, QImage::Format_ARGB32_Premultiplied);
        QPainter sourcePainter(&sourceImage);
        qt_opengl_draw_test_pattern(&sourcePainter, 256, 256);

        QPainter fbo1Painter(&fbo1);

        QPainter fbo2Painter(&fbo2);
        fbo2Painter.drawImage(0, 0, sourceImage);
        fbo2Painter.end();

        QImage fbo2Image = fbo2.toImage();

        fbo1Painter.drawImage(0, 0, sourceImage);
        fbo1Painter.end();

        QGLFramebufferObject::bindDefault();

        QCOMPARE(fbo1.toImage(), fbo2Image);
    }

    {
        QGLFramebufferObject fbo1(512, 512, QGLFramebufferObject::CombinedDepthStencil);

        QOpenGLFramebufferObjectPaintDevice fbo2(256, 256);

        QImage sourceImage(256, 256, QImage::Format_ARGB32_Premultiplied);
        QPainter sourcePainter(&sourceImage);
        qt_opengl_draw_test_pattern(&sourcePainter, 256, 256);

        QPainter fbo2Painter(&fbo2);
        fbo2Painter.drawImage(0, 0, sourceImage);
        QImage fbo2Image1 = fbo2.toImage();
        fbo2Painter.fillRect(0, 0, 256, 256, Qt::white);

        QPainter fbo1Painter(&fbo1);
        fbo1Painter.drawImage(0, 0, sourceImage);
        fbo1Painter.end();

        // check that the OpenGL paint engine now knows it needs to sync
        fbo2Painter.drawImage(0, 0, sourceImage);
        QImage fbo2Image2 = fbo2.toImage();

        fbo2Painter.end();

        QCOMPARE(fbo2Image1, fbo2Image2);
    }
}

// Tests multiple QPainters active on different FBOs at the same time, with
// interleaving painting. Performance-wise, this is sub-optimal, but it still
// has to work flawlessly
void tst_QGL::multipleFBOInterleavedRendering()
{
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QGLFramebufferObject not supported on this platform");

    QGLWidget glw;
    glw.makeCurrent();

    // No multisample with combined depth/stencil attachment:
    QGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QGLFramebufferObject::CombinedDepthStencil);

    QGLFramebufferObject *fbo1 = new QGLFramebufferObject(256, 128, fboFormat);
    QGLFramebufferObject *fbo2 = new QGLFramebufferObject(256, 128, fboFormat);
    QGLFramebufferObject *fbo3 = new QGLFramebufferObject(256, 128, fboFormat);

    if ( (fbo1->attachment() != QGLFramebufferObject::CombinedDepthStencil) ||
         (fbo2->attachment() != QGLFramebufferObject::CombinedDepthStencil) ||
         (fbo3->attachment() != QGLFramebufferObject::CombinedDepthStencil)    )
    {
        delete fbo1;
        delete fbo2;
        delete fbo3;
        QSKIP("FBOs missing combined depth~stencil support");
    }

    QPainter fbo1Painter;
    QPainter fbo2Painter;
    QPainter fbo3Painter;

    QVERIFY(fbo1Painter.begin(fbo1));
    QVERIFY(fbo2Painter.begin(fbo2));
    QVERIFY(fbo3Painter.begin(fbo3));

    // Confirm we're using the GL2 engine, as interleaved rendering isn't supported
    // on the GL1 engine:
    if (fbo1Painter.paintEngine()->type() != QPaintEngine::OpenGL2)
        QSKIP("Interleaved GL rendering requires OpenGL 2.0 or higher");

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

    fbo1Painter.fillRect(0, 0, fbo1->width(), fbo1->height(), Qt::red); // Background
    fbo2Painter.fillRect(0, 0, fbo2->width(), fbo2->height(), Qt::green); // Background
    fbo3Painter.fillRect(0, 0, fbo3->width(), fbo3->height(), Qt::blue); // Background

    fbo1Painter.translate(14, 14);
    fbo2Painter.translate(14, 14);
    fbo3Painter.translate(14, 14);

    fbo1Painter.fillPath(intersectingPath, Qt::blue); // Test stencil buffer works
    fbo2Painter.fillPath(intersectingPath, Qt::red); // Test stencil buffer works
    fbo3Painter.fillPath(intersectingPath, Qt::green); // Test stencil buffer works

    fbo1Painter.translate(128, 0);
    fbo2Painter.translate(128, 0);
    fbo3Painter.translate(128, 0);

    fbo1Painter.setClipPath(trianglePath);
    fbo2Painter.setClipPath(trianglePath);
    fbo3Painter.setClipPath(trianglePath);

    fbo1Painter.setTransform(QTransform()); // reset xform
    fbo2Painter.setTransform(QTransform()); // reset xform
    fbo3Painter.setTransform(QTransform()); // reset xform

    fbo1Painter.fillRect(0, 0, fbo1->width(), fbo1->height(), Qt::green);
    fbo2Painter.fillRect(0, 0, fbo2->width(), fbo2->height(), Qt::blue);
    fbo3Painter.fillRect(0, 0, fbo3->width(), fbo3->height(), Qt::red);

    fbo1Painter.end();
    fbo2Painter.end();
    fbo3Painter.end();

    QImage fb1 = fbo1->toImage().convertToFormat(QImage::Format_RGB32);
    QImage fb2 = fbo2->toImage().convertToFormat(QImage::Format_RGB32);
    QImage fb3 = fbo3->toImage().convertToFormat(QImage::Format_RGB32);
    delete fbo1;
    delete fbo2;
    delete fbo3;

    // As we're doing more than trivial painting, we can't just compare to
    // an image rendered with raster. Instead, we sample at well-defined
    // test-points:
    QFUZZY_COMPARE_PIXELS(fb1.pixel(39, 64), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(fb1.pixel(89, 64), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(fb1.pixel(64, 39), QColor(Qt::blue).rgb());
    QFUZZY_COMPARE_PIXELS(fb1.pixel(64, 89), QColor(Qt::blue).rgb());
    QFUZZY_COMPARE_PIXELS(fb1.pixel(167, 39), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(fb1.pixel(217, 39), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(fb1.pixel(192, 64), QColor(Qt::green).rgb());

    QFUZZY_COMPARE_PIXELS(fb2.pixel(39, 64), QColor(Qt::green).rgb());
    QFUZZY_COMPARE_PIXELS(fb2.pixel(89, 64), QColor(Qt::green).rgb());
    QFUZZY_COMPARE_PIXELS(fb2.pixel(64, 39), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(fb2.pixel(64, 89), QColor(Qt::red).rgb());
    QFUZZY_COMPARE_PIXELS(fb2.pixel(167, 39), QColor(Qt::green).rgb());
    QFUZZY_COMPARE_PIXELS(fb2.pixel(217, 39), QColor(Qt::green).rgb());
    QFUZZY_COMPARE_PIXELS(fb2.pixel(192, 64), QColor(Qt::blue).rgb());

    QFUZZY_COMPARE_PIXELS(fb3.pixel(39, 64), QColor(Qt::blue).rgb());
    QFUZZY_COMPARE_PIXELS(fb3.pixel(89, 64), QColor(Qt::blue).rgb());
    QFUZZY_COMPARE_PIXELS(fb3.pixel(64, 39), QColor(Qt::green).rgb());
    QFUZZY_COMPARE_PIXELS(fb3.pixel(64, 89), QColor(Qt::green).rgb());
    QFUZZY_COMPARE_PIXELS(fb3.pixel(167, 39), QColor(Qt::blue).rgb());
    QFUZZY_COMPARE_PIXELS(fb3.pixel(217, 39), QColor(Qt::blue).rgb());
    QFUZZY_COMPARE_PIXELS(fb3.pixel(192, 64), QColor(Qt::red).rgb());
}

class FBOUseInGLWidget : public QGLWidget
{
public:
    bool widgetPainterBeginOk;
    bool fboPainterBeginOk;
    QImage fboImage;
protected:
    void paintEvent(QPaintEvent*)
    {
        QPainter widgetPainter;
        widgetPainterBeginOk = widgetPainter.begin(this);
        QGLFramebufferObjectFormat fboFormat;
        fboFormat.setAttachment(QGLFramebufferObject::NoAttachment);
        QGLFramebufferObject *fbo = new QGLFramebufferObject(100, 100, fboFormat);

        QPainter fboPainter;
        fboPainterBeginOk = fboPainter.begin(fbo);
        fboPainter.fillRect(-1, -1, 130, 130, Qt::red);
        fboPainter.end();
        fboImage = fbo->toImage();

        widgetPainter.fillRect(-1, -1, width()+2, height()+2, Qt::blue);

        delete fbo;
    }

};

void tst_QGL::glFBOUseInGLWidget()
{
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QGLFramebufferObject not supported on this platform");

    FBOUseInGLWidget w;
    w.resize(100, 100);
    w.showNormal();

    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QVERIFY(w.widgetPainterBeginOk);
    QVERIFY(w.fboPainterBeginOk);

#if defined(Q_OS_QNX)
    // glReadPixels reads from the back buffer. On QNX the buffer is not preserved
    // after a buffer swap. This is why we have to swap the buffer explicitly before calling
    // grabFrameBuffer to retrieve the content of the front buffer
    w.swapBuffers();
#endif

    QImage widgetFB = w.grabFrameBuffer(false);
    QImage widgetReference(widgetFB.size(), widgetFB.format());
    widgetReference.fill(0xff0000ff);
    QFUZZY_COMPARE_IMAGES(widgetFB, widgetReference);

    QImage fboReference(w.fboImage.size(), w.fboImage.format());
    fboReference.fill(0xffff0000);
    QFUZZY_COMPARE_IMAGES(w.fboImage, fboReference);
}

void tst_QGL::glWidgetReparent()
{
    // Try it as a top-level first:
    GLWidget *widget = new GLWidget;
    widget->setObjectName(QStringLiteral("glWidget1"));
    widget->setGeometry(0, 0, 200, 30);
    widget->show();

    QWidget grandParentWidget;
    grandParentWidget.setObjectName(QStringLiteral("grandParentWidget"));
    grandParentWidget.setPalette(Qt::blue);
    QVBoxLayout grandParentLayout(&grandParentWidget);

    QWidget parentWidget(&grandParentWidget);
    parentWidget.setObjectName(QStringLiteral("parentWidget"));
    grandParentLayout.addWidget(&parentWidget);
    parentWidget.setPalette(Qt::green);
    parentWidget.setAutoFillBackground(true);
    QVBoxLayout parentLayout(&parentWidget);

    grandParentWidget.setGeometry(0, 100, 200, 200);
    grandParentWidget.show();

    QVERIFY(QTest::qWaitForWindowExposed(widget));
    QVERIFY(QTest::qWaitForWindowExposed(&grandParentWidget));

    QVERIFY(parentWidget.children().count() == 1); // The layout

    // Now both widgets should be created & shown, time to re-parent:
    parentLayout.addWidget(widget);

    QVERIFY(QTest::qWaitForWindowExposed(&grandParentWidget));

    QVERIFY(parentWidget.children().count() == 2); // Layout & glwidget
    QVERIFY(parentWidget.children().contains(widget));
    QTRY_VERIFY(widget->height() > 30);

    delete widget;

    QVERIFY(QTest::qWaitForWindowExposed(&grandParentWidget));

    QVERIFY(parentWidget.children().count() == 1); // The layout

    // Now do pretty much the same thing, but don't show the
    // widget first:
    widget = new GLWidget;
    widget->setObjectName(QStringLiteral("glWidget2"));
    parentLayout.addWidget(widget);

    QVERIFY(QTest::qWaitForWindowExposed(&grandParentWidget));

    QVERIFY(parentWidget.children().count() == 2); // Layout & glwidget
    QVERIFY(parentWidget.children().contains(widget));
    QVERIFY(widget->height() > 30);

    delete widget;
}

class RenderPixmapWidget : public QGLWidget
{
protected:
    void initializeGL() {
        // Set some gl state:
        QOpenGLContext::currentContext()->functions()->glClearColor(1.0, 0.0, 0.0, 1.0);
    }

    void paintGL() {
        QOpenGLContext::currentContext()->functions()->glClear(GL_COLOR_BUFFER_BIT);
    }
};

void tst_QGL::glWidgetRenderPixmap()
{
    RenderPixmapWidget *w = new RenderPixmapWidget;

    QSize pmSize = QSize(100, 100);
    QPixmap pm = w->renderPixmap(pmSize.width(), pmSize.height(), false);

    delete w;

    QImage fb = pm.toImage().convertToFormat(QImage::Format_RGB32);
    QImage reference(pmSize, QImage::Format_RGB32);
    reference.fill(0xffff0000);

    QFUZZY_COMPARE_IMAGES(fb, reference);
}

class ColormapExtended : public QGLColormap
{
public:
    ColormapExtended() {}

    Qt::HANDLE handle() { return QGLColormap::handle(); }
    void setHandle(Qt::HANDLE handle) { QGLColormap::setHandle(handle); }
};

void tst_QGL::colormap()
{
    // Check the properties of the default empty colormap.
    QGLColormap cmap1;
    QVERIFY(cmap1.isEmpty());
    QCOMPARE(cmap1.size(), 0);
    QCOMPARE(cmap1.entryRgb(0), QRgb(0));
    QCOMPARE(cmap1.entryRgb(-1), QRgb(0));
    QCOMPARE(cmap1.entryRgb(100), QRgb(0));
    QVERIFY(!cmap1.entryColor(0).isValid());
    QVERIFY(!cmap1.entryColor(-1).isValid());
    QVERIFY(!cmap1.entryColor(100).isValid());
    QCOMPARE(cmap1.find(qRgb(255, 0, 0)), -1);
    QCOMPARE(cmap1.findNearest(qRgb(255, 0, 0)), -1);

    // Set an entry and re-test.
    cmap1.setEntry(56, qRgb(255, 0, 0));
    // The colormap is still considered "empty" even though it
    // has entries in it now.  The isEmpty() method is used to
    // detect when the colormap is in use by a GL widget,
    // not to detect when it is empty!
    QVERIFY(cmap1.isEmpty());
    QCOMPARE(cmap1.size(), 256);
    QCOMPARE(cmap1.entryRgb(0), QRgb(0));
    QVERIFY(cmap1.entryColor(0) == QColor(0, 0, 0, 255));
    QVERIFY(cmap1.entryRgb(56) == qRgb(255, 0, 0));
    QVERIFY(cmap1.entryColor(56) == QColor(255, 0, 0, 255));
    QCOMPARE(cmap1.find(qRgb(255, 0, 0)), 56);
    QCOMPARE(cmap1.findNearest(qRgb(255, 0, 0)), 56);

    // Set some more entries.
    static QRgb const colors[] = {
        qRgb(255, 0, 0),
        qRgb(0, 255, 0),
        qRgb(255, 255, 255),
        qRgb(0, 0, 255),
        qRgb(0, 0, 0)
    };
    cmap1.setEntry(57, QColor(0, 255, 0));
    cmap1.setEntries(3, colors + 2, 58);
    cmap1.setEntries(5, colors, 251);
    int idx;
    for (idx = 0; idx < 5; ++idx) {
        QVERIFY(cmap1.entryRgb(56 + idx) == colors[idx]);
        QVERIFY(cmap1.entryColor(56 + idx) == QColor(colors[idx]));
        QVERIFY(cmap1.entryRgb(251 + idx) == colors[idx]);
        QVERIFY(cmap1.entryColor(251 + idx) == QColor(colors[idx]));
    }
    QCOMPARE(cmap1.size(), 256);

    // Perform color lookups.
    QCOMPARE(cmap1.find(qRgb(255, 0, 0)), 56);
    QCOMPARE(cmap1.find(qRgb(0, 0, 0)), 60); // Actually finds 0, 0, 0, 255.
    QCOMPARE(cmap1.find(qRgba(0, 0, 0, 0)), 0);
    QCOMPARE(cmap1.find(qRgb(0, 255, 0)), 57);
    QCOMPARE(cmap1.find(qRgb(255, 255, 255)), 58);
    QCOMPARE(cmap1.find(qRgb(0, 0, 255)), 59);
    QCOMPARE(cmap1.find(qRgb(140, 0, 0)), -1);
    QCOMPARE(cmap1.find(qRgb(0, 140, 0)), -1);
    QCOMPARE(cmap1.find(qRgb(0, 0, 140)), -1);
    QCOMPARE(cmap1.find(qRgb(64, 0, 0)), -1);
    QCOMPARE(cmap1.find(qRgb(0, 64, 0)), -1);
    QCOMPARE(cmap1.find(qRgb(0, 0, 64)), -1);
    QCOMPARE(cmap1.findNearest(qRgb(255, 0, 0)), 56);
    QCOMPARE(cmap1.findNearest(qRgb(0, 0, 0)), 60);
    QCOMPARE(cmap1.findNearest(qRgba(0, 0, 0, 0)), 0);
    QCOMPARE(cmap1.findNearest(qRgb(0, 255, 0)), 57);
    QCOMPARE(cmap1.findNearest(qRgb(255, 255, 255)), 58);
    QCOMPARE(cmap1.findNearest(qRgb(0, 0, 255)), 59);
    QCOMPARE(cmap1.findNearest(qRgb(140, 0, 0)), 56);
    QCOMPARE(cmap1.findNearest(qRgb(0, 140, 0)), 57);
    QCOMPARE(cmap1.findNearest(qRgb(0, 0, 140)), 59);
    QCOMPARE(cmap1.findNearest(qRgb(64, 0, 0)), 0);
    QCOMPARE(cmap1.findNearest(qRgb(0, 64, 0)), 0);
    QCOMPARE(cmap1.findNearest(qRgb(0, 0, 64)), 0);

    // Make some copies of the colormap and check that they are the same.
    QGLColormap cmap2(cmap1);
    QGLColormap cmap3;
    cmap3 = cmap1;
    QVERIFY(cmap2.isEmpty());
    QVERIFY(cmap3.isEmpty());
    QCOMPARE(cmap2.size(), 256);
    QCOMPARE(cmap3.size(), 256);
    for (idx = 0; idx < 256; ++idx) {
        QCOMPARE(cmap1.entryRgb(idx), cmap2.entryRgb(idx));
        QCOMPARE(cmap1.entryRgb(idx), cmap3.entryRgb(idx));
    }

    // Modify an entry in one of the copies and recheck the original.
    cmap2.setEntry(45, qRgb(255, 0, 0));
    for (idx = 0; idx < 256; ++idx) {
        if (idx != 45)
            QCOMPARE(cmap1.entryRgb(idx), cmap2.entryRgb(idx));
        else
            QCOMPARE(cmap2.entryRgb(45), qRgb(255, 0, 0));
        QCOMPARE(cmap1.entryRgb(idx), cmap3.entryRgb(idx));
    }

    // Check that setting the handle will cause isEmpty() to work right.
    ColormapExtended cmap4;
    cmap4.setEntry(56, qRgb(255, 0, 0));
    QVERIFY(cmap4.isEmpty());
    QCOMPARE(cmap4.size(), 256);
    cmap4.setHandle(Qt::HANDLE(42));
    QCOMPARE(cmap4.handle(), Qt::HANDLE(42));
    QVERIFY(!cmap4.isEmpty());
    QCOMPARE(cmap4.size(), 256);
}

#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D 0x806F
#endif

#ifndef GL_RGB16
#define GL_RGB16 0x8054
#endif

void tst_QGL::fboFormat()
{
    // Check the initial conditions.
    QGLFramebufferObjectFormat format1;
    QCOMPARE(format1.samples(), 0);
    QCOMPARE(format1.attachment(), QGLFramebufferObject::NoAttachment);
    QCOMPARE(int(format1.textureTarget()), int(GL_TEXTURE_2D));
    int expectedFormat =
#ifdef QT_OPENGL_ES_2
        GL_RGBA;
#else
        QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL ? GL_RGBA : GL_RGBA8;
#endif
    QCOMPARE(int(format1.internalTextureFormat()), expectedFormat);

    // Modify the values and re-check.
    format1.setSamples(8);
    format1.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
    format1.setTextureTarget(GL_TEXTURE_3D);
    format1.setInternalTextureFormat(GL_RGB16);
    QCOMPARE(format1.samples(), 8);
    QCOMPARE(format1.attachment(), QGLFramebufferObject::CombinedDepthStencil);
    QCOMPARE(int(format1.textureTarget()), int(GL_TEXTURE_3D));
    QCOMPARE(int(format1.internalTextureFormat()), int(GL_RGB16));

    // Make copies and check that they are the same.
    QGLFramebufferObjectFormat format2(format1);
    QGLFramebufferObjectFormat format3;
    QCOMPARE(format2.samples(), 8);
    QCOMPARE(format2.attachment(), QGLFramebufferObject::CombinedDepthStencil);
    QCOMPARE(int(format2.textureTarget()), int(GL_TEXTURE_3D));
    QCOMPARE(int(format2.internalTextureFormat()), int(GL_RGB16));
    format3 = format1;
    QCOMPARE(format3.samples(), 8);
    QCOMPARE(format3.attachment(), QGLFramebufferObject::CombinedDepthStencil);
    QCOMPARE(int(format3.textureTarget()), int(GL_TEXTURE_3D));
    QCOMPARE(int(format3.internalTextureFormat()), int(GL_RGB16));

    // Modify the copies and check that the original is unchanged.
    format2.setSamples(9);
    format3.setTextureTarget(GL_TEXTURE_2D);
    QCOMPARE(format1.samples(), 8);
    QCOMPARE(format1.attachment(), QGLFramebufferObject::CombinedDepthStencil);
    QCOMPARE(int(format1.textureTarget()), int(GL_TEXTURE_3D));
    QCOMPARE(int(format1.internalTextureFormat()), int(GL_RGB16));

    // operator== and operator!= for QGLFramebufferObjectFormat.
    QGLFramebufferObjectFormat format1c;
    QGLFramebufferObjectFormat format2c;

    QCOMPARE(format1c, format2c);
    QVERIFY(!(format1c != format2c));
    format1c.setSamples(8);
    QVERIFY(!(format1c == format2c));
    QVERIFY(format1c != format2c);
    format2c.setSamples(8);
    QCOMPARE(format1c, format2c);
    QVERIFY(!(format1c != format2c));

    format1c.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
    QVERIFY(!(format1c == format2c));
    QVERIFY(format1c != format2c);
    format2c.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
    QCOMPARE(format1c, format2c);
    QVERIFY(!(format1c != format2c));

    format1c.setTextureTarget(GL_TEXTURE_3D);
    QVERIFY(!(format1c == format2c));
    QVERIFY(format1c != format2c);
    format2c.setTextureTarget(GL_TEXTURE_3D);
    QCOMPARE(format1c, format2c);
    QVERIFY(!(format1c != format2c));

    format1c.setInternalTextureFormat(GL_RGB16);
    QVERIFY(!(format1c == format2c));
    QVERIFY(format1c != format2c);
    format2c.setInternalTextureFormat(GL_RGB16);
    QCOMPARE(format1c, format2c);
    QVERIFY(!(format1c != format2c));

    QGLFramebufferObjectFormat format3c(format1c);
    QGLFramebufferObjectFormat format4c;
    QCOMPARE(format1c, format3c);
    QVERIFY(!(format1c != format3c));
    format3c.setInternalTextureFormat(
#ifdef QT_OPENGL_ES_2
        GL_RGBA
#else
        QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL ? GL_RGBA : GL_RGBA8
#endif
        );
    QVERIFY(!(format1c == format3c));
    QVERIFY(format1c != format3c);

    format4c = format1c;
    QCOMPARE(format1c, format4c);
    QVERIFY(!(format1c != format4c));
    format4c.setInternalTextureFormat(
#ifdef QT_OPENGL_ES_2
        GL_RGBA
#else
        QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL ? GL_RGBA : GL_RGBA8
#endif
        );
    QVERIFY(!(format1c == format4c));
    QVERIFY(format1c != format4c);
}

void tst_QGL::testDontCrashOnDanglingResources()
{
    // We have a number of Q_GLOBAL_STATICS inside the Qt OpenGL
    // module. This test is verify that we don't crash as a result of
    // them calling into libgl on application shutdown.
    QWidget *widget = new UnclippedWidget();
    widget->show();
    qApp->processEvents();
    widget->hide();
}

class ReplaceClippingGLWidget : public QGLWidget
{
public:
    void paint(QPainter *painter)
    {
        painter->fillRect(rect(), Qt::white);

        QPainterPath path;
        path.addRect(0, 0, 100, 100);
        path.addRect(50, 50, 100, 100);

        painter->setClipRect(0, 0, 150, 150);
        painter->fillPath(path, Qt::red);

        painter->translate(150, 150);
        painter->setClipRect(0, 0, 150, 150);
        painter->fillPath(path, Qt::red);
    }

protected:
    void paintEvent(QPaintEvent*)
    {
        // clear the stencil with junk
        QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
        funcs->glStencilMask(0xFFFF);
        funcs->glClearStencil(0xFFFF);
        funcs->glDisable(GL_STENCIL_TEST);
        funcs->glDisable(GL_SCISSOR_TEST);
        funcs->glClear(GL_STENCIL_BUFFER_BIT);

        QPainter painter(this);
        paint(&painter);
    }
};

void tst_QGL::replaceClipping()
{
    ReplaceClippingGLWidget glw;
    glw.resize(300, 300);
    glw.show();

    QVERIFY(QTest::qWaitForWindowExposed(&glw));

    QImage reference(300, 300, QImage::Format_RGB32);
    QPainter referencePainter(&reference);
    glw.paint(&referencePainter);
    referencePainter.end();

#if defined(Q_OS_QNX)
    // glReadPixels reads from the back buffer. On QNX the buffer is not preserved
    // after a buffer swap. This is why we have to swap the buffer explicitly before calling
    // grabFrameBuffer to retrieve the content of the front buffer
    glw.swapBuffers();
#endif
    const QImage widgetFB = glw.grabFrameBuffer(false).convertToFormat(QImage::Format_RGB32);

    // Sample pixels in a grid pattern which avoids false failures due to
    // off-by-one pixel errors on some buggy GL implementations
    for (int x = 25; x < reference.width(); x += 50) {
        for (int y = 25; y < reference.width(); y += 50) {
            QFUZZY_COMPARE_PIXELS(widgetFB.pixel(x, y), reference.pixel(x, y));
        }
    }
}

class ClipTestGLWidget : public QGLWidget
{
public:
    void paint(QPainter *painter)
    {
        painter->fillRect(-1, -1, width()+2, height()+2, Qt::white);
        painter->setClipRect(10, 10, width()-20, height()-20);
        painter->fillRect(rect(), Qt::cyan);

        painter->save();
        painter->setClipRect(10, 10, 100, 100, Qt::IntersectClip);

        painter->fillRect(rect(), Qt::blue);

        painter->save();
        painter->setClipRect(10, 10, 50, 50, Qt::IntersectClip);
        painter->fillRect(rect(), Qt::red);
        painter->restore();
        painter->fillRect(0, 0, 40, 40, Qt::white);
        painter->save();

        painter->setClipRect(0, 0, 35, 35, Qt::IntersectClip);
        painter->fillRect(rect(), Qt::black);
        painter->restore();

        painter->fillRect(0, 0, 30, 30, Qt::magenta);

        painter->save();
        painter->setClipRect(60, 10, 50, 50, Qt::ReplaceClip);
        painter->fillRect(rect(), Qt::green);
        painter->restore();

        painter->restore();

        painter->translate(100, 100);

        {
            QPainterPath path;
            path.addRect(10, 10, 100, 100);
            path.addRect(10, 10, 10, 10);
            painter->setClipPath(path, Qt::IntersectClip);
        }

        painter->fillRect(rect(), Qt::blue);

        painter->save();
        {
            QPainterPath path;
            path.addRect(10, 10, 50, 50);
            path.addRect(10, 10, 10, 10);
            painter->setClipPath(path, Qt::IntersectClip);
        }
        painter->fillRect(rect(), Qt::red);
        painter->restore();
        painter->fillRect(0, 0, 40, 40, Qt::white);
        painter->save();

        {
            QPainterPath path;
            path.addRect(0, 0, 35, 35);
            path.addRect(10, 10, 10, 10);
            painter->setClipPath(path, Qt::IntersectClip);
        }
        painter->fillRect(rect(), Qt::black);
        painter->restore();

        painter->fillRect(0, 0, 30, 30, Qt::magenta);

        painter->save();
        {
            QPainterPath path;
            path.addRect(60, 10, 50, 50);
            path.addRect(10, 10, 10, 10);
            painter->setClipPath(path, Qt::ReplaceClip);
        }
        painter->fillRect(rect(), Qt::green);
        painter->restore();
    }

protected:
    void paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        paint(&painter);
    }
};

void tst_QGL::clipTest()
{
    ClipTestGLWidget glw;
    glw.resize(220, 220);
    glw.showNormal();

    QVERIFY(QTest::qWaitForWindowExposed(&glw));

    QImage reference(glw.size(), QImage::Format_RGB32);
    QPainter referencePainter(&reference);
    glw.paint(&referencePainter);
    referencePainter.end();

#if defined(Q_OS_QNX)
    // glReadPixels reads from the back buffer. On QNX the buffer is not preserved
    // after a buffer swap. This is why we have to swap the buffer explicitly before calling
    // grabFrameBuffer to retrieve the content of the front buffer
    glw.swapBuffers();
#endif
    const QImage widgetFB = glw.grabFrameBuffer(false).convertToFormat(QImage::Format_RGB32);

    // Sample pixels in a grid pattern which avoids false failures due to
    // off-by-one pixel errors on some buggy GL implementations
    for (int x = 2; x < reference.width(); x += 5) {
        for (int y = 2; y < reference.height(); y += 5) {
            QFUZZY_COMPARE_PIXELS(widgetFB.pixel(x, y), reference.pixel(x, y));
        }
    }
}

void tst_QGL::destroyFBOAfterContext()
{
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QGLFramebufferObject not supported on this platform");

    QGLWidget *glw = new QGLWidget();
    glw->makeCurrent();

    // No multisample with combined depth/stencil attachment:
    QGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QGLFramebufferObject::CombinedDepthStencil);

    // Don't complicate things by using NPOT:
    QGLFramebufferObject *fbo = new QGLFramebufferObject(256, 128, fboFormat);

    // The handle should be valid until the context is destroyed.
    QVERIFY(fbo->handle() != 0);
    QVERIFY(fbo->isValid());

    delete glw;

    // The handle should now be zero.
    QVERIFY(!fbo->handle());
    QVERIFY(!fbo->isValid());

    delete fbo;
}

#ifdef QT_BUILD_INTERNAL

class tst_QGLResource
{
public:
    tst_QGLResource(const QGLContext * = 0) {}
    ~tst_QGLResource() { ++deletions; }

    static int deletions;
};

int tst_QGLResource::deletions = 0;

#ifdef TODO
Q_GLOBAL_STATIC(QOpenGLContextGroupResource<tst_QGLResource>, qt_shared_test)
#endif //TODO
#endif // QT_BUILD_INTERNAL

#ifdef QT_BUILD_INTERNAL
void tst_QGL::shareRegister()
{
#ifdef TODO
    // Create a context.
    QGLWidget *glw1 = new QGLWidget();
    glw1->makeCurrent();

    // Nothing should be sharing with glw1's context yet.
    QVERIFY(!glw1->isSharing());

    // Create a guard for the first context.
    QOpenGLSharedResourceGuard guard(glw1->context()->contextHandle());
    QCOMPARE(guard.id(), 0);
    guard.setId(3);
    QCOMPARE(guard.id(), 3);

    // Request a tst_QGLResource object for the first context.
    tst_QGLResource *res1 = qt_shared_test()->value(glw1->context()->contextHandle());
    QVERIFY(res1);
    QCOMPARE(qt_shared_test()->value(glw1->context()->contextHandle()), res1);

    // Create another context that shares with the first.
    QVERIFY(!glw1->isSharing());
    QGLWidget *glw2 = new QGLWidget(0, glw1);
    if (!glw2->isSharing()) {
        delete glw2;
        delete glw1;
        QSKIP("Context sharing is not supported");
    }
    QVERIFY(glw1->isSharing());
    QVERIFY(glw1->context() != glw2->context());

    // Check that the first context's resource is also on the second.
    QCOMPARE(qt_shared_test()->value(glw1->context()), res1);
    QCOMPARE(qt_shared_test()->value(glw2->context()), res1);

    // Guard should still be the same.
    QCOMPARE(guard.context(), glw1->context());
    QCOMPARE(guard.id(), 3);

    // Check the sharing relationships.
    QVERIFY(QGLContext::areSharing(glw1->context(), glw1->context()));
    QVERIFY(QGLContext::areSharing(glw2->context(), glw2->context()));
    QVERIFY(QGLContext::areSharing(glw1->context(), glw2->context()));
    QVERIFY(QGLContext::areSharing(glw2->context(), glw1->context()));
    QVERIFY(!QGLContext::areSharing(0, glw2->context()));
    QVERIFY(!QGLContext::areSharing(glw1->context(), 0));
    QVERIFY(!QGLContext::areSharing(0, 0));

    // Create a third context, not sharing with the others.
    QGLWidget *glw3 = new QGLWidget();
    QVERIFY(!glw3->isSharing());

    // Create a guard on the standalone context.
    QGLSharedResourceGuard guard3(glw3->context());
    guard3.setId(5);

    // Request a resource to the third context.
    tst_QGLResource *res3 = qt_shared_test()->value(glw3->context());
    QVERIFY(res3);
    QCOMPARE(qt_shared_test()->value(glw1->context()), res1);
    QCOMPARE(qt_shared_test()->value(glw2->context()), res1);
    QCOMPARE(qt_shared_test()->value(glw3->context()), res3);

    // Check the sharing relationships again.
    QVERIFY(QGLContext::areSharing(glw1->context(), glw1->context()));
    QVERIFY(QGLContext::areSharing(glw2->context(), glw2->context()));
    QVERIFY(QGLContext::areSharing(glw1->context(), glw2->context()));
    QVERIFY(QGLContext::areSharing(glw2->context(), glw1->context()));
    QVERIFY(!QGLContext::areSharing(glw1->context(), glw3->context()));
    QVERIFY(!QGLContext::areSharing(glw2->context(), glw3->context()));
    QVERIFY(!QGLContext::areSharing(glw3->context(), glw1->context()));
    QVERIFY(!QGLContext::areSharing(glw3->context(), glw2->context()));
    QVERIFY(QGLContext::areSharing(glw3->context(), glw3->context()));
    QVERIFY(!QGLContext::areSharing(0, glw2->context()));
    QVERIFY(!QGLContext::areSharing(glw1->context(), 0));
    QVERIFY(!QGLContext::areSharing(0, glw3->context()));
    QVERIFY(!QGLContext::areSharing(glw3->context(), 0));
    QVERIFY(!QGLContext::areSharing(0, 0));

    // Shared guard should still be the same.
    QCOMPARE(guard.context(), glw1->context());
    QCOMPARE(guard.id(), 3);

    // Delete the first context.
    delete glw1;

    // The second context should no longer register as sharing.
    QVERIFY(!glw2->isSharing());

    // The first context's resource should transfer to the second context.
    QCOMPARE(tst_QGLResource::deletions, 0);
    QCOMPARE(qt_shared_test()->value(glw2->context()), res1);
    QCOMPARE(qt_shared_test()->value(glw3->context()), res3);

    // Shared guard should now be the second context, with the id the same.
    QCOMPARE(guard.context(), glw2->context());
    QCOMPARE(guard.id(), 3);
    QCOMPARE(guard3.context(), glw3->context());
    QCOMPARE(guard3.id(), 5);

    // Clean up and check that the resources are properly deleted.
    delete glw2;
    QCOMPARE(tst_QGLResource::deletions, 1);
    delete glw3;
    QCOMPARE(tst_QGLResource::deletions, 2);

    // Guards should now be null and the id zero.
    QVERIFY(guard.context() == 0);
    QVERIFY(guard.id() == 0);
    QVERIFY(guard3.context() == 0);
    QVERIFY(guard3.id() == 0);
#endif //TODO
}
#endif

// Tests QGLContext::bindTexture with default options
#ifdef QT_BUILD_INTERNAL
void tst_QGL::qglContextDefaultBindTexture()
{
    QGLWidget w;
    w.makeCurrent();
    QGLContext *ctx = const_cast<QGLContext*>(w.context());

    QImage *boundImage = new QImage(256, 256, QImage::Format_RGB32);
    boundImage->fill(0xFFFFFFFF);
    QPixmap *boundPixmap = new QPixmap(256, 256);
    boundPixmap->fill(Qt::red);

    int startCacheItemCount = QGLTextureCache::instance()->size();

    GLuint boundImageTextureId = ctx->bindTexture(*boundImage);
    GLuint boundPixmapTextureId = ctx->bindTexture(*boundPixmap);

    // Make sure the image & pixmap have been added to the cache:
    QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);

    // Make sure the image & pixmap have the is_cached flag set:
    QVERIFY(QImagePixmapCleanupHooks::isImageCached(*boundImage));
    QVERIFY(QImagePixmapCleanupHooks::isPixmapCached(*boundPixmap));

    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    // Make sure the texture IDs returned are valid:
    QCOMPARE(funcs->glIsTexture(boundImageTextureId), GLboolean(GL_TRUE));
    QCOMPARE(funcs->glIsTexture(boundPixmapTextureId), GLboolean(GL_TRUE));

    // Make sure the textures are still valid after we delete the image/pixmap:
    // Also check that although the textures are left intact, the cache entries are removed:
    delete boundImage;
    boundImage = 0;
    QCOMPARE(funcs->glIsTexture(boundImageTextureId), GLboolean(GL_TRUE));
    QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);
    delete boundPixmap;
    boundPixmap = 0;
    QCOMPARE(funcs->glIsTexture(boundPixmapTextureId), GLboolean(GL_TRUE));
    QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);

    // Finally, make sure QGLContext::deleteTexture deletes the texture IDs:
    ctx->deleteTexture(boundImageTextureId);
    ctx->deleteTexture(boundPixmapTextureId);
    QCOMPARE(funcs->glIsTexture(boundImageTextureId), GLboolean(GL_FALSE));
    QCOMPARE(funcs->glIsTexture(boundPixmapTextureId), GLboolean(GL_FALSE));
}
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QGL::textureCleanup()
{
    QGLWidget w;
    w.resize(200,200);
    w.show();
    QTest::qWaitForWindowExposed(&w);
    w.makeCurrent();

    // Test pixmaps which have been loaded via QPixmapCache are removed from the texture cache
    // when the pixmap cache is cleared
    {
        int startCacheItemCount = QGLTextureCache::instance()->size();
        QPainter p(&w);

        QPixmap boundPixmap(":designer.png");

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);

        p.drawPixmap(0, 0, boundPixmap);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        // Need to call end for the GL2 paint engine to release references to pixmap if using tfp
        p.end();

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        // Check that the texture doesn't get removed from the cache when the pixmap is cleared
        // as it should still be in the cache:
        boundPixmap = QPixmap();
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        QPixmapCache::clear();
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);
    }

    // Test pixmaps which have been loaded via QPixmapCache are removed from the texture cache
    // when they are explicitly removed from the pixmap cache
    {
        int startCacheItemCount = QGLTextureCache::instance()->size();
        QPainter p(&w);

        QPixmap boundPixmap(128, 128);
        QString cacheKey = QString::fromLatin1("myPixmap");
        QPixmapCache::insert(cacheKey, boundPixmap);

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);

        p.drawPixmap(0, 0, boundPixmap);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        // Need to call end for the GL2 paint engine to release references to pixmap if using tfp
        p.end();

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        // Check that the texture doesn't get removed from the cache when the pixmap is cleared
        // as it should still be in the cache:
        boundPixmap = QPixmap();
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        // Finally, we check that the texture cache entry is removed when we remove the
        // pixmap cache entry, which should hold the last reference:
        QPixmapCache::remove(cacheKey);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);
    }

    // Check images & pixmaps are removed from the cache when they are deleted
    {
        int startCacheItemCount = QGLTextureCache::instance()->size();
        QPainter p(&w);

        QImage *boundImage = new QImage(256, 256, QImage::Format_RGB32);
        boundImage->fill(0xFFFFFFFF);
        QPixmap *boundPixmap = new QPixmap(256, 256);
        boundPixmap->fill(Qt::red);

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);

        p.drawImage(0, 0, *boundImage);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        p.drawPixmap(0, 0, *boundPixmap);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);

        // Need to call end for the GL2 paint engine to release references to pixmap if using tfp
        p.end();

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);

        delete boundImage;
        boundImage = 0;
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        delete boundPixmap;
        boundPixmap = 0;
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);
    }

    // Check images & pixmaps are removed from the cache when they are assigned to
    {
        int startCacheItemCount = QGLTextureCache::instance()->size();
        QPainter p(&w);

        QImage boundImage(256, 256, QImage::Format_RGB32);
        boundImage.fill(0xFFFFFFFF);
        QPixmap boundPixmap(256, 256);
        boundPixmap.fill(Qt::red);

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);

        p.drawImage(0, 0, boundImage);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        p.drawPixmap(0, 0, boundPixmap);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);

        // Need to call end for the GL2 paint engine to release references to pixmap if using tfp
        p.end();

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);

        boundImage = QImage(64, 64, QImage::Format_RGB32);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        boundPixmap = QPixmap(64, 64);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);
    }

    // Check images & pixmaps are removed from the cache when they are modified (detached)
    {
        int startCacheItemCount = QGLTextureCache::instance()->size();
        QPainter p(&w);

        QImage boundImage(256, 256, QImage::Format_RGB32);
        boundImage.fill(0xFFFFFFFF);
        QPixmap boundPixmap(256, 256);
        boundPixmap.fill(Qt::red);

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);

        p.drawImage(0, 0, boundImage);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        p.drawPixmap(0, 0, boundPixmap);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);

        // Need to call end for the GL2 paint engine to release references to pixmap if using tfp
        p.end();

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);

        boundImage.fill(0x00000000);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        boundPixmap.fill(Qt::blue);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);
    }

    // Check that images/pixmaps aren't removed from the cache if a shallow copy has been made
    QImage copyOfImage;
    QPixmap copyOfPixmap;
    int startCacheItemCount = QGLTextureCache::instance()->size();
    {
        QPainter p(&w);

        QImage boundImage(256, 256, QImage::Format_RGB32);
        boundImage.fill(0xFFFFFFFF);
        QPixmap boundPixmap(256, 256);
        boundPixmap.fill(Qt::red);

        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);

        p.drawImage(0, 0, boundImage);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

        p.drawPixmap(0, 0, boundPixmap);
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);

        // Need to call end for the GL2 paint engine to release references to pixmap if using tfp
        p.end();

        copyOfImage = boundImage;
        copyOfPixmap = boundPixmap;
        QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);
    } // boundImage & boundPixmap would have been deleted when they went out of scope
    QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+2);

    copyOfImage = QImage();
    QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount+1);

    copyOfPixmap = QPixmap();
    QCOMPARE(QGLTextureCache::instance()->size(), startCacheItemCount);
}
#endif

namespace ThreadImages {

class Producer : public QObject
{
    Q_OBJECT
public:
    Producer()
    {
        startTimer(20);

        QThread *thread = new QThread;
        thread->start();

        connect(this, SIGNAL(destroyed()), thread, SLOT(quit()));

        moveToThread(thread);
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    }

signals:
    void imageReady(const QImage &image);

protected:
    void timerEvent(QTimerEvent *)
    {
        QImage image(256, 256, QImage::Format_RGB32);
        QLinearGradient g(0, 0, 0, 256);
        g.setColorAt(0, QColor(255, 180, 180));
        g.setColorAt(1, Qt::white);
        g.setSpread(QGradient::ReflectSpread);

        QBrush brush(g);
        brush.setTransform(QTransform::fromTranslate(0, delta));
        delta += 10;

        QPainter p(&image);
        p.fillRect(image.rect(), brush);

        if (images.size() > 10)
            images.removeFirst();

        images.append(image);

        emit imageReady(image);
    }

private:
    QList<QImage> images;
    int delta;
};


class DisplayWidget : public QGLWidget
{
    Q_OBJECT
public:
    DisplayWidget(QWidget *parent) : QGLWidget(parent) {}
    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.drawImage(rect(), m_image);
    }

public slots:
    void setImage(const QImage &image)
    {
        m_image = image;
        update();
    }

private:
    QImage m_image;
};

class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget()
        : iterations(0)
        , display(0)
        , producer(new Producer)
    {
        startTimer(400);
        connect(this, SIGNAL(destroyed()), producer, SLOT(deleteLater()));
    }

    int iterations;

protected:
    void timerEvent(QTimerEvent *)
    {
        ++iterations;

        delete display;
        display = new DisplayWidget(this);
        connect(producer, SIGNAL(imageReady(QImage)), display, SLOT(setImage(QImage)));

        display->setGeometry(rect());
        display->show();
    }

private:
    DisplayWidget *display;
    Producer *producer;
};

}

void tst_QGL::threadImages()
{
    ThreadImages::Widget *widget = new ThreadImages::Widget;
    widget->show();

    while (widget->iterations <= 5) {
        qApp->processEvents();
    }

    delete widget;
}

void tst_QGL::nullRectCrash()
{
    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects())
        QSKIP("QGLFramebufferObject not supported on this platform");

    QGLWidget glw;
    glw.makeCurrent();

    QGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QGLFramebufferObject::CombinedDepthStencil);

    QGLFramebufferObject *fbo = new QGLFramebufferObject(128, 128, fboFormat);

    QPainter fboPainter(fbo);

    fboPainter.setPen(QPen(QColor(255, 127, 127, 127), 2));
    fboPainter.setBrush(QColor(127, 255, 127, 127));
    fboPainter.drawRect(QRectF());

    fboPainter.end();
}

void tst_QGL::extensions()
{
    QGLWidget glw;
    glw.makeCurrent();

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QVERIFY(ctx);
    QOpenGLFunctions *funcs = ctx->functions();
    QVERIFY(funcs);
    QSurfaceFormat format = ctx->format();

#ifdef QT_BUILD_INTERNAL
    QOpenGLExtensions *exts = static_cast<QOpenGLExtensions *>(funcs);
    QOpenGLExtensions::OpenGLExtensions allExts = exts->openGLExtensions();
    // Mipmapping is always available in GL2/GLES2+. Verify this.
    if (format.majorVersion() >= 2)
        QVERIFY(allExts.testFlag(QOpenGLExtensions::GenerateMipmap));
#endif

    // Now look for some features should always be available in a given version.
    QOpenGLFunctions::OpenGLFeatures allFeatures = funcs->openGLFeatures();
    QVERIFY(allFeatures.testFlag(QOpenGLFunctions::Multitexture));
    if (format.majorVersion() >= 2) {
        QVERIFY(allFeatures.testFlag(QOpenGLFunctions::Shaders));
        QVERIFY(allFeatures.testFlag(QOpenGLFunctions::Buffers));
        QVERIFY(allFeatures.testFlag(QOpenGLFunctions::Multisample));
        QVERIFY(!ctx->isOpenGLES() || allFeatures.testFlag(QOpenGLFunctions::Framebuffers));
        QVERIFY(allFeatures.testFlag(QOpenGLFunctions::NPOTTextures)
                && allFeatures.testFlag(QOpenGLFunctions::NPOTTextureRepeat));
        if (ctx->isOpenGLES()) {
            QVERIFY(!allFeatures.testFlag(QOpenGLFunctions::FixedFunctionPipeline));
            QVERIFY(allFeatures.testFlag(QOpenGLFunctions::Framebuffers));
        }
    }
    if (format.majorVersion() >= 3)
        QVERIFY(allFeatures.testFlag(QOpenGLFunctions::Framebuffers));
}

QTEST_MAIN(tst_QGL)
#include "tst_qgl.moc"
