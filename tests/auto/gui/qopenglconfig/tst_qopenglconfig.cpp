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

#include <QtGui/QOpenGLFunctions>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <private/qopengl_p.h>

#include <QtTest/QtTest>

#include <QtCore/QSysInfo>
#include <QtCore/QLibraryInfo>
#include <QtCore/QScopedPointer>
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QJsonDocument>

#include <algorithm>

#define DUMP_CAPABILITY(str, integration, capability) \
    if (platformIntegration->hasCapability(QPlatformIntegration::capability)) \
        str << ' ' << #capability;

QTextStream &operator<<(QTextStream &str, const QSize &s)
{
    str << s.width() << 'x' << s.height();
    return str;
}

QTextStream &operator<<(QTextStream &str, const QRect &r)
{
    str << r.size() << '+' << r.x() << '+' << r.y();
    return str;
}

QTextStream &operator<<(QTextStream &str, const QSizeF &s)
{
    str << s.width() << 'x' << s.height();
    return str;
}

QTextStream &operator<<(QTextStream &str, const QSurfaceFormat &format)
{
    str << "Version: " << format.majorVersion() << '.'
        << format.minorVersion() << " Profile: " << format.profile()
        << " Swap behavior: " << format.swapBehavior()
        << " Buffer size (RGB";
    if (format.hasAlpha())
        str << 'A';
    str << "): " << format.redBufferSize() << ',' << format.greenBufferSize()
        << ',' << format.blueBufferSize();
    if (format.hasAlpha())
        str << ',' << format.alphaBufferSize();
    if (const int dbs = format.depthBufferSize())
        str << " Depth buffer: " << dbs;
    if (const int sbs = format.stencilBufferSize())
        str << " Stencil buffer: " << sbs;
    const int samples = format.samples();
    if (samples > 0)
        str << " Samples: " << samples;
    return str;
}

/* This test contains code from the qtdiag tool. Its purpose is to output the
 * graphics configuration to the CI log and to verify that Open GL can be
 * initialized for platforms on which the qopengl test is marked as
 * insignificant. */

class tst_QOpenGlConfig : public QObject
{
    Q_OBJECT

private slots:
    void testConfiguration();
    void testGlConfiguration();
    void testBugList();
    void testDefaultWindowsBlacklist();
};

static void dumpConfiguration(QTextStream &str)
{
    const QPlatformIntegration *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    str << "\nBuild        : " << QLibraryInfo::build()
        << "\nPlatform     : " << QGuiApplication::platformName()
        << "\nOS           : " << QSysInfo::prettyProductName() << " ["
        << QSysInfo::kernelType() << " version " << QSysInfo::kernelVersion() << ']'
        << "\nArchitecture : " << QSysInfo::currentCpuArchitecture()
        << "\nCapabilities :";
    DUMP_CAPABILITY(str, platformIntegration, ThreadedPixmaps)
    DUMP_CAPABILITY(str, platformIntegration, OpenGL)
    DUMP_CAPABILITY(str, platformIntegration, ThreadedOpenGL)
    DUMP_CAPABILITY(str, platformIntegration, SharedGraphicsCache)
    DUMP_CAPABILITY(str, platformIntegration, BufferQueueingOpenGL)
    DUMP_CAPABILITY(str, platformIntegration, WindowMasks)
    DUMP_CAPABILITY(str, platformIntegration, RasterGLSurface)
    DUMP_CAPABILITY(str, platformIntegration, AllGLFunctionsQueryable)
    str << '\n';

    const QList<QScreen*> screens = QGuiApplication::screens();
    const int screenCount = screens.size();
    str << "\nScreens: " << screenCount << '\n';
    for (int s = 0; s < screenCount; ++s) {
        const QScreen *screen = screens.at(s);
        str << '#' << ' ' << s << " \"" << screen->name() << '"'
                  << " Depth: " << screen->depth()
                  << " Primary: " <<  (screen == QGuiApplication::primaryScreen() ? "yes" : "no")
            << "\n  Geometry: " << screen->geometry() << " Available: " << screen->availableGeometry();
        if (screen->geometry() != screen->virtualGeometry())
            str << "\n  Virtual geometry: " << screen->virtualGeometry() << " Available: " << screen->availableVirtualGeometry();
        if (screen->virtualSiblings().size() > 1)
            str << "\n  " << screen->virtualSiblings().size() << " virtual siblings";
        str << "\n  Physical size: " << screen->physicalSize() << " mm"
                  << "  Refresh: " << screen->refreshRate() << " Hz"
            << "\n  Physical DPI: " << screen->physicalDotsPerInchX()
            << ',' << screen->physicalDotsPerInchY()
            << " Logical DPI: " << screen->logicalDotsPerInchX()
            << ',' << screen->logicalDotsPerInchY()
            << "\n  DevicePixelRatio: " << screen->devicePixelRatio()
            << " Primary orientation: " << screen->primaryOrientation()
            << "\n  Orientation: " << screen->orientation()
            << " Native orientation: " << screen->nativeOrientation()
            << " OrientationUpdateMask: " << screen->orientationUpdateMask() << '\n';
    }

    // On Windows, this will provide addition GPU info similar to the output of dxdiag.
    const QVariant gpuInfoV = QGuiApplication::platformNativeInterface()->property("gpu");
    if (gpuInfoV.type() == QVariant::Map) {
        const QString description = gpuInfoV.toMap().value(QStringLiteral("printable")).toString();
        if (!description.isEmpty())
            str << "\nGPU:\n" << description << "\n\n";
    }
}

void tst_QOpenGlConfig::testConfiguration()
{
    QString result;
    QTextStream str(&result);
    dumpConfiguration(str);

    qDebug().noquote() << '\n' << result;
}

static void dumpGlConfiguration(QOpenGLContext &context, QTextStream &str)
{
    str << "Type             : ";
#ifdef QT_OPENGL_DYNAMIC
    str << "Dynamic GL ";
#endif
    switch (context.openGLModuleType()) {
    case QOpenGLContext::LibGL:
        str << "LibGL";
        break;
    case QOpenGLContext::LibGLES:
        str << "LibGLES";
        break;
    }
    QOpenGLFunctions functions(&context);

    str << "\nVendor           : " << reinterpret_cast<const char *>(functions.glGetString(GL_VENDOR))
        << "\nRenderer         : " << reinterpret_cast<const char *>(functions.glGetString(GL_RENDERER))
        << "\nVersion          : " << reinterpret_cast<const char *>(functions.glGetString(GL_VERSION))
        << "\nShading language : " << reinterpret_cast<const char *>(functions.glGetString(GL_SHADING_LANGUAGE_VERSION))
        << "\nFormat           : " << context.format();

    QList<QByteArray> extensionList = context.extensions().toList();
    std::sort(extensionList.begin(), extensionList.end());
    const int extensionCount = extensionList.size();
    str << "\n\nFound " << extensionCount << " extensions:\n";
    for (int e = 0; e < extensionCount; ++e)
        str << ((e % 4) ? ' ' : '\n') << extensionList.at(e);
}

void tst_QOpenGlConfig::testGlConfiguration()
{
    QString result;
    QTextStream str(&result);

    QWindow window;
    window.setSurfaceType(QSurface::OpenGLSurface);
    window.create();
    QOpenGLContext context;
    QVERIFY(context.create());
    QVERIFY(context.makeCurrent(&window));
    dumpGlConfiguration(context, str);
    context.doneCurrent();

    qDebug().noquote() << '\n' << result;

    // fromContext either uses the current context or creates a temporary dummy one.
    QOpenGLConfig::Gpu gpu = QOpenGLConfig::Gpu::fromContext();
    qDebug().noquote() << '\n' << "GL_VENDOR queried by QOpenGLConfig::Gpu:" << gpu.glVendor;
    QVERIFY(!gpu.glVendor.isEmpty());
}

static inline QByteArray msgSetMismatch(const QSet<QString> &expected,
                                        const QSet<QString> &actual)
{
    const QString result = QStringList(expected.toList()).join(QLatin1Char(','))
        + QLatin1String(" != ")
        + QStringList(actual.toList()).join(QLatin1Char(','));
    return result.toLatin1();
}

void tst_QOpenGlConfig::testBugList()
{
    // Check bug list parsing for some arbitrary NVidia card
    // faking Windows OS.
    const QString fileName = QFINDTESTDATA("buglist.json");
    QVERIFY(!fileName.isEmpty());

    QSet<QString> expectedFeatures;
    expectedFeatures << "feature1";

    // adapter info
    QVersionNumber driverVersion(QVector<int>() << 9 << 18 << 13 << 4460);
    QOpenGLConfig::Gpu gpu = QOpenGLConfig::Gpu::fromDevice(0x10DE, 0x0DE9, driverVersion, QByteArrayLiteral("Unknown"));

    QSet<QString> actualFeatures = QOpenGLConfig::gpuFeatures(gpu, QStringLiteral("win"),
                                                              QVersionNumber(6, 3), QStringLiteral("7"), fileName);
    QVERIFY2(expectedFeatures == actualFeatures,
             msgSetMismatch(expectedFeatures, actualFeatures));

    // driver_description
    gpu = QOpenGLConfig::Gpu::fromDevice(0xDEAD, 0xBEEF, driverVersion, QByteArrayLiteral("Very Long And Special Driver Description"));
    actualFeatures = QOpenGLConfig::gpuFeatures(gpu, QStringLiteral("win"),
                                                QVersionNumber(6, 3), QStringLiteral("8"), fileName);
    expectedFeatures = QSet<QString>() << "feature2";
    QVERIFY2(expectedFeatures == actualFeatures,
             msgSetMismatch(expectedFeatures, actualFeatures));

    // os.release
    gpu = QOpenGLConfig::Gpu::fromDevice(0xDEAD, 0xBEEF, driverVersion, QByteArrayLiteral("WinVerTest"));
    actualFeatures = QOpenGLConfig::gpuFeatures(gpu, QStringLiteral("win"),
                                                QVersionNumber(12, 34), QStringLiteral("10"), fileName);
    expectedFeatures = QSet<QString>() << "win10_feature";
    QVERIFY2(expectedFeatures == actualFeatures,
             msgSetMismatch(expectedFeatures, actualFeatures));

    // gl_vendor
    gpu = QOpenGLConfig::Gpu::fromGLVendor(QByteArrayLiteral("Somebody Else"));
    expectedFeatures.clear();
    actualFeatures = QOpenGLConfig::gpuFeatures(gpu, QStringLiteral("linux"),
                                                QVersionNumber(1, 0), QString(), fileName);
    QVERIFY2(expectedFeatures == actualFeatures,
             msgSetMismatch(expectedFeatures, actualFeatures));

    gpu = QOpenGLConfig::Gpu::fromGLVendor(QByteArrayLiteral("The Qt Company"));
    expectedFeatures = QSet<QString>() << "cool_feature";
    actualFeatures = QOpenGLConfig::gpuFeatures(gpu, QStringLiteral("linux"),
                                                QVersionNumber(1, 0), QString(), fileName);
    QVERIFY2(expectedFeatures == actualFeatures,
             msgSetMismatch(expectedFeatures, actualFeatures));
}

void tst_QOpenGlConfig::testDefaultWindowsBlacklist()
{
    if (QGuiApplication::platformName().compare(QLatin1String("windows"), Qt::CaseInsensitive))
        QSKIP("Only applicable to Windows");

    QFile f(QStringLiteral(":/qt-project.org/windows/openglblacklists/default.json"));
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    QVERIFY2(err.error == 0,
             QStringLiteral("Failed to parse built-in Windows GPU blacklist. %1 : %2")
             .arg(err.offset).arg(err.errorString()).toLatin1());
}

QTEST_MAIN(tst_QOpenGlConfig)

#include "tst_qopenglconfig.moc"
