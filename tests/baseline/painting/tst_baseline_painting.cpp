// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include "paintcommands.h"
#include <qbaselinetest.h>
#include <QDir>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryFile>
#if QT_CONFIG(process)
#include <QProcess>
#endif

#ifndef QT_NO_OPENGL
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#endif

#include <QFontDatabase>

#include <algorithm>

#ifndef GL_RGB10
#define GL_RGB10                          0x8052
#endif
#ifndef GL_RGB10_A2
#define GL_RGB10_A2                       0x8059
#endif

class tst_Lancelot : public QObject
{
Q_OBJECT

public:
    tst_Lancelot();

private:
    enum GraphicsEngine {
        Raster = 0,
        OpenGL = 1,
        Pdf = 2
    };

    void setupTestSuite(const QStringList& blacklist = QStringList());
    void runTestSuite(GraphicsEngine engine, QImage::Format format,
                      const QSurfaceFormat &contextFormat = QSurfaceFormat::defaultFormat());
    void paint(QPaintDevice *device, GraphicsEngine engine, QImage::Format format, const QStringList &script, const QString &filePath);

    QStringList qpsFiles;
    QHash<QString, QStringList> scripts;
    QHash<QString, quint16> scriptChecksums;
    QString scriptsDir;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void testRasterARGB32PM_data();
    void testRasterARGB32PM();
    void testRasterRGB32_data();
    void testRasterRGB32();
    void testRasterARGB32_data();
    void testRasterARGB32();
    void testRasterRGB16_data();
    void testRasterRGB16();
    void testRasterA2RGB30PM_data();
    void testRasterA2RGB30PM();
    void testRasterBGR30_data();
    void testRasterBGR30();
    void testRasterARGB8565PM_data();
    void testRasterARGB8565PM();
    void testRasterGrayscale8_data();
    void testRasterGrayscale8();
    void testRasterRGBA64PM_data();
    void testRasterRGBA64PM();
    void testRasterRGBA16F_data();
    void testRasterRGBA16F();
    void testRasterRGBA32FPM_data();
    void testRasterRGBA32FPM();

    void testPdf_data();
    void testPdf();

#ifndef QT_NO_OPENGL
    void testOpenGL_data();
    void testOpenGL();
    void testOpenGLBGR30_data();
    void testOpenGLBGR30();
    void testCoreOpenGL_data();
    void testCoreOpenGL();
private:
    void initOpenGL();
    bool checkSystemGLSupport();
    bool checkSystemCoreGLSupport();
#endif
};

tst_Lancelot::tst_Lancelot()
{
}

void tst_Lancelot::initTestCase()
{
    // Check and setup the environment. We treat failures because of test environment
    // (e.g. script files not found) as just warnings, and not QFAILs, to avoid false negatives
    // caused by environment or server instability

    QByteArray msg;
    if (!QBaselineTest::connectToBaselineServer(&msg))
        QSKIP(msg);

    QString baseDir = QFINDTESTDATA("scripts/text.qps");
    scriptsDir = baseDir.left(baseDir.lastIndexOf('/')) + '/';
    QDir qpsDir(scriptsDir);
    qpsFiles = qpsDir.entryList(QStringList() << QLatin1String("*.qps"), QDir::Files | QDir::Readable);
    if (qpsFiles.isEmpty()) {
        qWarning() << "No qps script files found in" << qpsDir.path();
        QSKIP("Aborted due to errors.");
    }

    std::sort(qpsFiles.begin(), qpsFiles.end());
    foreach (const QString& fileName, qpsFiles) {
        QFile file(scriptsDir + fileName);
        QVERIFY(file.open(QFile::ReadOnly));
        QByteArray cont = file.readAll();
        scripts.insert(fileName, QString::fromUtf8(cont).split(QLatin1Char('\n'), Qt::SkipEmptyParts));
        scriptChecksums.insert(fileName, qChecksum(cont));
    }

    QString underlineTestFont1 = QLatin1String(":/fonts/QtUnderlineTest-Regular.ttf");
    int id = QFontDatabase::addApplicationFont(underlineTestFont1);
    QVERIFY(id >= 0);

    QString underlineTestFont2 = QLatin1String(":/fonts/QtUnderlineTest2-Regular.ttf");
    id = QFontDatabase::addApplicationFont(underlineTestFont2);
    QVERIFY(id >= 0);

#ifndef QT_NO_OPENGL
    initOpenGL();
#endif
}

void tst_Lancelot::init()
{
    // This gets called for every row. QSKIP if current item is blacklisted on the baseline server:
    QBASELINE_SKIP_IF_BLACKLISTED;
}

void tst_Lancelot::cleanupTestCase()
{
    QBaselineTest::finalizeAndDisconnect();
}

void tst_Lancelot::testRasterARGB32PM_data()
{
    setupTestSuite();
}


void tst_Lancelot::testRasterARGB32PM()
{
    runTestSuite(Raster, QImage::Format_ARGB32_Premultiplied);
}


void tst_Lancelot::testRasterARGB32_data()
{
    setupTestSuite();
}

void tst_Lancelot::testRasterARGB32()
{
    runTestSuite(Raster, QImage::Format_ARGB32);
}


void tst_Lancelot::testRasterRGB32_data()
{
    setupTestSuite();
}


void tst_Lancelot::testRasterRGB32()
{
    runTestSuite(Raster, QImage::Format_RGB32);
}


void tst_Lancelot::testRasterRGB16_data()
{
    setupTestSuite();
}


void tst_Lancelot::testRasterRGB16()
{
    runTestSuite(Raster, QImage::Format_RGB16);
}


void tst_Lancelot::testRasterA2RGB30PM_data()
{
    setupTestSuite();
}


void tst_Lancelot::testRasterA2RGB30PM()
{
    runTestSuite(Raster, QImage::Format_A2RGB30_Premultiplied);
}


void tst_Lancelot::testRasterBGR30_data()
{
    setupTestSuite();
}


void tst_Lancelot::testRasterBGR30()
{
    runTestSuite(Raster, QImage::Format_BGR30);
}


void tst_Lancelot::testRasterARGB8565PM_data()
{
    setupTestSuite();
}

void tst_Lancelot::testRasterARGB8565PM()
{
    runTestSuite(Raster, QImage::Format_ARGB8565_Premultiplied);
}


void tst_Lancelot::testRasterGrayscale8_data()
{
    setupTestSuite();
}

void tst_Lancelot::testRasterGrayscale8()
{
    runTestSuite(Raster, QImage::Format_Grayscale8);
}


void tst_Lancelot::testRasterRGBA64PM_data()
{
    setupTestSuite();
}

void tst_Lancelot::testRasterRGBA64PM()
{
    runTestSuite(Raster, QImage::Format_RGBA64_Premultiplied);
}


void tst_Lancelot::testRasterRGBA16F_data()
{
    setupTestSuite();
}

void tst_Lancelot::testRasterRGBA16F()
{
    runTestSuite(Raster, QImage::Format_RGBA16FPx4);
}

void tst_Lancelot::testRasterRGBA32FPM_data()
{
    setupTestSuite();
}

void tst_Lancelot::testRasterRGBA32FPM()
{
    runTestSuite(Raster, QImage::Format_RGBA32FPx4_Premultiplied);
}


void tst_Lancelot::testPdf_data()
{
#ifdef Q_OS_MACOS
    setupTestSuite();
#else
    QSKIP("Pdf testing only implemented for macOS");
#endif
}

void tst_Lancelot::testPdf()
{
    runTestSuite(Pdf, QImage::Format_RGB32);
}


#ifndef QT_NO_OPENGL
void tst_Lancelot::initOpenGL()
{
    // Stencil buffer is needed for clipping
    QSurfaceFormat glFormat;
    glFormat.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(glFormat);
}

bool tst_Lancelot::checkSystemGLSupport()
{
    QWindow win;
    win.setSurfaceType(QSurface::OpenGLSurface);
    win.create();
    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    fmt.setSamples(4);
    QOpenGLContext ctx;
    if (!ctx.create() || !ctx.makeCurrent(&win))
        return false;
    QOpenGLFramebufferObject fbo(800, 800, fmt);
    if (!fbo.isValid() || !fbo.bind())
        return false;

    return true;
}

bool tst_Lancelot::checkSystemCoreGLSupport()
{
    if (QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL)
        return false;

    QSurfaceFormat coreFormat(QSurfaceFormat::defaultFormat());
    coreFormat.setVersion(3, 2);
    coreFormat.setProfile(QSurfaceFormat::CoreProfile);
    QWindow win;
    win.setSurfaceType(QSurface::OpenGLSurface);
    win.setFormat(coreFormat);
    win.create();
    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    fmt.setSamples(4);
    QOpenGLContext ctx;
    ctx.setFormat(coreFormat);
    if (!ctx.create() || !ctx.makeCurrent(&win))
        return false;
    QOpenGLFramebufferObject fbo(800, 800, fmt);
    if (!fbo.isValid() || !fbo.bind())
        return false;

    return true;
}

void tst_Lancelot::testOpenGL_data()
{
    if (!checkSystemGLSupport())
        QSKIP("System under test does not meet preconditions for GL testing. Skipping.");
    QStringList localBlacklist = QStringList() << QLatin1String("rasterops.qps");
    setupTestSuite(localBlacklist);
}


void tst_Lancelot::testOpenGL()
{
    runTestSuite(OpenGL, QImage::Format_RGB32);
}

void tst_Lancelot::testOpenGLBGR30_data()
{
    tst_Lancelot::testOpenGL_data();
}

void tst_Lancelot::testOpenGLBGR30()
{
    runTestSuite(OpenGL, QImage::Format_BGR30);
}

void tst_Lancelot::testCoreOpenGL_data()
{
    if (!checkSystemCoreGLSupport())
        QSKIP("System under test does not meet preconditions for Core Profile GL testing. Skipping.");
    QStringList localBlacklist = QStringList() << QLatin1String("rasterops.qps");
    setupTestSuite(localBlacklist);
}

void tst_Lancelot::testCoreOpenGL()
{
    QSurfaceFormat coreFormat(QSurfaceFormat::defaultFormat());
    coreFormat.setVersion(3, 2);
    coreFormat.setProfile(QSurfaceFormat::CoreProfile);
    runTestSuite(OpenGL, QImage::Format_RGB32, coreFormat);
}
#endif


void tst_Lancelot::setupTestSuite(const QStringList& blacklist)
{
    QTest::addColumn<QString>("qpsFile");
    foreach (const QString &fileName, qpsFiles) {
        if (blacklist.contains(fileName))
            continue;
        QBaselineTest::newRow(fileName.toLatin1(), scriptChecksums.value(fileName)) << fileName;
    }
}


void tst_Lancelot::runTestSuite(GraphicsEngine engine, QImage::Format format, const QSurfaceFormat &contextFormat)
{
    QFETCH(QString, qpsFile);

    QString filePath = scriptsDir + qpsFile;
    QStringList script = scripts.value(qpsFile);
    QImage rendered;

    if (engine == Raster) {
        QImage img(800, 800, format);
        paint(&img, engine, format, script, QFileInfo(filePath).absoluteFilePath());
        rendered = img;
#ifndef QT_NO_OPENGL
    } else if (engine == OpenGL) {
        QWindow win;
        win.setSurfaceType(QSurface::OpenGLSurface);
        win.setFormat(contextFormat);
        win.create();
        QOpenGLContext ctx;
        ctx.setFormat(contextFormat);
        QVERIFY(ctx.create());
        QVERIFY(ctx.makeCurrent(&win));
        QOpenGLFramebufferObjectFormat fmt;
        fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        fmt.setSamples(4);
        if (format == QImage::Format_BGR30)
            fmt.setInternalTextureFormat(ctx.isOpenGLES() ? GL_RGB10_A2 : GL_RGB10);
        QOpenGLFramebufferObject fbo(800, 800, fmt);
        fbo.bind();
        QOpenGLPaintDevice pdv(800, 800);
        paint(&pdv, engine, format, script, QFileInfo(filePath).absoluteFilePath());
        rendered = fbo.toImage().convertToFormat(format);
#endif
    } else if (engine == Pdf) {
#if QT_CONFIG(process)
        QString tempStem(QDir::tempPath() + QLatin1String("/lancelot_XXXXXX_") + qpsFile.chopped(4));

        QTemporaryFile pdfFile(tempStem + QLatin1String(".pdf"));
        QVERIFY(pdfFile.open());
        QPdfWriter writer(&pdfFile);
        writer.setPdfVersion(QPdfWriter::PdfVersion_1_6);
        QPageSize pageSize(QSize(800, 800), QStringLiteral("LancePage"), QPageSize::ExactMatch);
        writer.setPageSize(pageSize);
        writer.setPageMargins(QMarginsF());
        writer.setResolution(72);
        paint(&writer, engine, format, script, QFileInfo(filePath).absoluteFilePath());
        pdfFile.close();

        // Convert pdf to something we can read into a QImage, using macOS' sips utility
        QTemporaryFile pngFile(tempStem + QLatin1String(".png"));
        QVERIFY(pngFile.open()); // Just create the file name
        pngFile.close();
        QProcess proc;
        const char *rawArgs = "-s format png -o";
        QStringList argList = QString::fromLatin1(rawArgs).split(QLatin1Char(' '));
        proc.start(QLatin1String("sips"), argList << pngFile.fileName() << pdfFile.fileName());
        proc.waitForFinished(2 * 60 * 1000); // May need some time

        rendered = QImage(pngFile.fileName());
#endif
    }

    QBASELINE_TEST(rendered);
}

void tst_Lancelot::paint(QPaintDevice *device, GraphicsEngine engine, QImage::Format format, const QStringList &script, const QString &filePath)
{
    QPainter p(device);
    PaintCommands pcmd(script, 800, 800, format);
    //pcmd.setShouldDrawText(false);
    switch (engine) {
    case OpenGL:
        pcmd.setType(OpenGLBufferType); // version/profile is communicated through the context's format()
        break;
    case Pdf:
        pcmd.setType(PdfType);
        break;
    case Raster:  // fallthrough
    default:
        pcmd.setType(ImageType);
        break;
    }
    pcmd.setPainter(&p);
    pcmd.setFilePath(filePath);
    pcmd.runCommands();
    p.end();
}

QBASELINETEST_MAIN(tst_Lancelot);

#include "tst_baseline_painting.moc"
