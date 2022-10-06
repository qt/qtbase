// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "paintcommands.h"

#include <qtest.h>
#include <QDir>
#include <QPainter>

#ifndef QT_NO_OPENGL
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#endif

#include <algorithm>

class tst_LanceBench : public QObject
{
    Q_OBJECT
public:
    tst_LanceBench();

private:
    enum GraphicsEngine {
        Raster = 0,
        OpenGL = 1
    };

    void setupTestSuite(const QStringList& blacklist = QStringList());
    void runTestSuite(GraphicsEngine engine, QImage::Format format, const QSurfaceFormat &contextFormat = QSurfaceFormat());
    void paint(QPaintDevice *device, GraphicsEngine engine, QImage::Format format, const QStringList &script, const QString &filePath);

    QStringList qpsFiles;
    QHash<QString, QStringList> scripts;
    QString scriptsDir;

private slots:
    void initTestCase();
    void cleanupTestCase() {}

    void testRasterARGB32PM_data();
    void testRasterARGB32PM();
    void testRasterRGB32_data();
    void testRasterRGB32();
    void testRasterARGB32_data();
    void testRasterARGB32();
    void testRasterRGB16_data();
    void testRasterRGB16();
    void testRasterBGR30_data();
    void testRasterBGR30();
    void testRasterARGB8565PM_data();
    void testRasterARGB8565PM();
    void testRasterGrayscale8_data();
    void testRasterGrayscale8();

#ifndef QT_NO_OPENGL
    void testOpenGL_data();
    void testOpenGL();
    void testCoreOpenGL_data();
    void testCoreOpenGL();
private:
    bool checkSystemGLSupport();
    bool checkSystemCoreGLSupport();
#endif
};

tst_LanceBench::tst_LanceBench()
{
}

void tst_LanceBench::initTestCase()
{
    QString baseDir = QFINDTESTDATA("../../../../baseline/painting/scripts/text.qps");
    scriptsDir = baseDir.left(baseDir.lastIndexOf('/')) + '/';
    QDir qpsDir(scriptsDir);
    qpsFiles = qpsDir.entryList(QStringList() << QLatin1String("*.qps"), QDir::Files | QDir::Readable);
    if (qpsFiles.isEmpty()) {
        qWarning() << "No qps script files found in" << qpsDir.path();
        QSKIP("Aborted due to errors.");
    }

    std::sort(qpsFiles.begin(), qpsFiles.end());
    for (const QString& fileName : std::as_const(qpsFiles)) {
        QFile file(scriptsDir + fileName);
        file.open(QFile::ReadOnly);
        QByteArray cont = file.readAll();
        scripts.insert(fileName, QString::fromUtf8(cont).split(QLatin1Char('\n'), Qt::SkipEmptyParts));
    }
}

void tst_LanceBench::testRasterARGB32PM_data()
{
    setupTestSuite();
}

void tst_LanceBench::testRasterARGB32PM()
{
    runTestSuite(Raster, QImage::Format_ARGB32_Premultiplied);
}

void tst_LanceBench::testRasterRGB32_data()
{
    setupTestSuite();
}

void tst_LanceBench::testRasterRGB32()
{
    runTestSuite(Raster, QImage::Format_RGB32);
}

void tst_LanceBench::testRasterARGB32_data()
{
    setupTestSuite();
}

void tst_LanceBench::testRasterARGB32()
{
    runTestSuite(Raster, QImage::Format_ARGB32);
}

void tst_LanceBench::testRasterRGB16_data()
{
    setupTestSuite();
}

void tst_LanceBench::testRasterRGB16()
{
    runTestSuite(Raster, QImage::Format_RGB16);
}

void tst_LanceBench::testRasterBGR30_data()
{
    setupTestSuite();
}

void tst_LanceBench::testRasterBGR30()
{
    runTestSuite(Raster, QImage::Format_BGR30);
}

void tst_LanceBench::testRasterARGB8565PM_data()
{
    setupTestSuite();
}

void tst_LanceBench::testRasterARGB8565PM()
{
    runTestSuite(Raster, QImage::Format_ARGB8565_Premultiplied);
}

void tst_LanceBench::testRasterGrayscale8_data()
{
    setupTestSuite();
}

void tst_LanceBench::testRasterGrayscale8()
{
    runTestSuite(Raster, QImage::Format_Grayscale8);
}

#ifndef QT_NO_OPENGL
bool tst_LanceBench::checkSystemGLSupport()
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

bool tst_LanceBench::checkSystemCoreGLSupport()
{
    if (QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL)
        return false;

    QSurfaceFormat coreFormat;
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

void tst_LanceBench::testOpenGL_data()
{
    if (!checkSystemGLSupport())
        QSKIP("System under test does not meet preconditions for GL testing. Skipping.");
    QStringList localBlacklist = QStringList() << QLatin1String("rasterops.qps");
    setupTestSuite(localBlacklist);
}

void tst_LanceBench::testOpenGL()
{
    runTestSuite(OpenGL, QImage::Format_RGB32);
}

void tst_LanceBench::testCoreOpenGL_data()
{
    if (!checkSystemCoreGLSupport())
        QSKIP("System under test does not meet preconditions for Core Profile GL testing. Skipping.");
    QStringList localBlacklist = QStringList() << QLatin1String("rasterops.qps");
    setupTestSuite(localBlacklist);
}

void tst_LanceBench::testCoreOpenGL()
{
    QSurfaceFormat coreFormat;
    coreFormat.setVersion(3, 2);
    coreFormat.setProfile(QSurfaceFormat::CoreProfile);
    runTestSuite(OpenGL, QImage::Format_RGB32, coreFormat);
}
#endif

void tst_LanceBench::setupTestSuite(const QStringList& blacklist)
{
    QTest::addColumn<QString>("qpsFile");
    for (const QString &fileName : std::as_const(qpsFiles)) {
        if (blacklist.contains(fileName))
            continue;
        QTest::newRow(fileName.toLatin1()) << fileName;
    }
}

void tst_LanceBench::runTestSuite(GraphicsEngine engine, QImage::Format format, const QSurfaceFormat &contextFormat)
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
        QOpenGLFramebufferObjectFormat fmt;
        fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        fmt.setSamples(4);
        QOpenGLContext ctx;
        ctx.setFormat(contextFormat);
        QVERIFY(ctx.create());
        QVERIFY(ctx.makeCurrent(&win));
        QOpenGLFramebufferObject fbo(800, 800, fmt);
        fbo.bind();
        QOpenGLPaintDevice pdv(800, 800);
        paint(&pdv, engine, format, script, QFileInfo(filePath).absoluteFilePath());
        rendered = fbo.toImage().convertToFormat(format);
#endif
    }
}

void tst_LanceBench::paint(QPaintDevice *device, GraphicsEngine engine, QImage::Format format, const QStringList &script, const QString &filePath)
{
    PaintCommands pcmd(script, 800, 800, format);
    switch (engine) {
    case OpenGL:
        pcmd.setType(OpenGLBufferType); // version/profile is communicated through the context's format()
        break;
    case Raster:
        pcmd.setType(ImageType);
        break;
    }
    pcmd.setFilePath(filePath);
    QBENCHMARK {
        QPainter p(device);
        pcmd.setPainter(&p);
        pcmd.runCommands();
        p.end();
    }
}

QTEST_MAIN(tst_LanceBench)

#include "tst_lancebench.moc"
