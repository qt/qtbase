/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <QtTest/QtTest>
#include "paintcommands.h"
#include <QPainter>
#include <QLibraryInfo>
#include <baselineprotocol.h>
#include <QHash>

#ifndef QT_NO_OPENGL
#include <QtOpenGL>
#endif

class tst_Lancelot : public QObject
{
Q_OBJECT

public:
    tst_Lancelot();

    static bool simfail;
    static PlatformInfo clientInfo;

private:
    enum GraphicsEngine {
        Raster = 0,
        OpenGL = 1
    };

    bool setupTestSuite(const QStringList& blacklist);
    void runTestSuite(GraphicsEngine engine, QImage::Format format);
    ImageItem render(const ImageItem &item, GraphicsEngine engine, QImage::Format format);
    void paint(QPaintDevice *device, const QStringList &script, const QString &filePath);

    BaselineProtocol proto;
    ImageItemList baseList;
    QHash<QString, QStringList> scripts;
    bool dryRunMode;
    QString scriptsDir;

private slots:
    void initTestCase();
    void cleanupTestCase() {}

    void testRasterARGB32PM_data();
    void testRasterARGB32PM();
    void testRasterRGB32_data();
    void testRasterRGB32();
    void testRasterRGB16_data();
    void testRasterRGB16();

#ifndef QT_NO_OPENGL
    void testOpenGL_data();
    void testOpenGL();
#endif
};

bool tst_Lancelot::simfail = false;
PlatformInfo tst_Lancelot::clientInfo;

tst_Lancelot::tst_Lancelot()
{
}

void tst_Lancelot::initTestCase()
{
    // Check and setup the environment. We treat failures because of test environment
    // (e.g. script files not found) as just warnings, and not QFAILs, to avoid false negatives
    // caused by environment or server instability

    if (!proto.connect(QLatin1String("tst_Lancelot"), &dryRunMode, clientInfo))
        QSKIP(qPrintable(proto.errorMessage()));

    QString baseDir = QFINDTESTDATA("scripts/text.qps");
    scriptsDir = baseDir.left(baseDir.lastIndexOf('/')) + '/';
    QDir qpsDir(scriptsDir);
    QStringList files = qpsDir.entryList(QStringList() << QLatin1String("*.qps"), QDir::Files | QDir::Readable);
    if (files.isEmpty()) {
        QWARN("No qps script files found in " + qpsDir.path().toLatin1());
        QSKIP("Aborted due to errors.");
    }

    baseList.resize(files.count());
    ImageItemList::iterator it = baseList.begin();
    foreach(const QString& fileName, files) {
        QFile file(scriptsDir + fileName);
        file.open(QFile::ReadOnly);
        QByteArray cont = file.readAll();
        scripts.insert(fileName, QString::fromUtf8(cont).split(QLatin1Char('\n'), QString::SkipEmptyParts));
        it->itemName = fileName;
        it->itemChecksum = qChecksum(cont.constData(), cont.size());
        it++;
    }
}


void tst_Lancelot::testRasterARGB32PM_data()
{
    QStringList localBlacklist;
    if (!setupTestSuite(localBlacklist))
        QSKIP("Communication with baseline image server failed.");
}


void tst_Lancelot::testRasterARGB32PM()
{
    runTestSuite(Raster, QImage::Format_ARGB32_Premultiplied);
}


void tst_Lancelot::testRasterRGB32_data()
{
    QStringList localBlacklist;
    if (!setupTestSuite(localBlacklist))
        QSKIP("Communication with baseline image server failed.");
}


void tst_Lancelot::testRasterRGB32()
{
    runTestSuite(Raster, QImage::Format_RGB32);
}


void tst_Lancelot::testRasterRGB16_data()
{
    QStringList localBlacklist;
    if (!setupTestSuite(localBlacklist))
        QSKIP("Communication with baseline image server failed.");
}


void tst_Lancelot::testRasterRGB16()
{
    runTestSuite(Raster, QImage::Format_RGB16);
}


#ifndef QT_NO_OPENGL
void tst_Lancelot::testOpenGL_data()
{
    QStringList localBlacklist = QStringList() << QLatin1String("rasterops.qps");
    if (!setupTestSuite(localBlacklist))
        QSKIP("Communication with baseline image server failed.");
}


void tst_Lancelot::testOpenGL()
{
#ifdef Q_OS_MAC
    QSKIP("QTBUG-22792: This test function crashes on Mac OS X");
#endif
    bool ok = false;
    QGLWidget glWidget;
    if (glWidget.isValid() && glWidget.format().directRendering()
        && ((QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_0)
            || (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_ES_Version_2_0))
        && QGLFramebufferObject::hasOpenGLFramebufferObjects())
    {
        glWidget.makeCurrent();
        if (!QByteArray((const char *)glGetString(GL_VERSION)).contains("Mesa"))
            ok = true;
    }
    if (ok)
        runTestSuite(OpenGL, QImage::Format_RGB32);
    else
        QSKIP("System under test does not meet preconditions for GL testing. Skipping.");
}
#endif


bool tst_Lancelot::setupTestSuite(const QStringList& blacklist)
{
    QTest::addColumn<ImageItem>("baseline");

    ImageItemList itemList(baseList);
    if (!proto.requestBaselineChecksums(QTest::currentTestFunction(), &itemList)) {
        QWARN(qPrintable(proto.errorMessage()));
        return false;
    }

    foreach(const ImageItem& item, itemList) {
        if (!blacklist.contains(item.itemName))
            QTest::newRow(item.itemName.toLatin1()) << item;
    }
    return true;
}


void tst_Lancelot::runTestSuite(GraphicsEngine engine, QImage::Format format)
{
    QFETCH(ImageItem, baseline);

    if (baseline.status == ImageItem::IgnoreItem)
        QSKIP("Blacklisted by baseline server.");

    ImageItem rendered = render(baseline, engine, format);
    static int consecutiveErrs = 0;
    if (rendered.image.isNull()) {    // Assume an error in the test environment, not Qt
        QWARN("Error: Failed to render image.");
        if (++consecutiveErrs < 3) {
            QSKIP("Aborted due to errors.");
        } else {
            consecutiveErrs = 0;
            QSKIP("Too many errors, skipping rest of testfunction.");
        }
    } else {
        consecutiveErrs = 0;
    }


    if (baseline.status == ImageItem::BaselineNotFound) {
        if (!proto.submitNewBaseline(rendered, 0))
            QWARN("Failed to submit new baseline: " + proto.errorMessage().toLatin1());
        QSKIP("Baseline not found; new baseline created.");
    }

    if (!baseline.imageChecksums.contains(rendered.imageChecksums.at(0))) {
            QByteArray serverMsg;
            if (!proto.submitMismatch(rendered, &serverMsg))
                serverMsg = "Failed to submit mismatching image to server.";
            if (dryRunMode)
                qDebug() << "Dryrun mode, ignoring detected mismatch." << serverMsg;
            else
                QFAIL("Rendered image differs from baseline. Report:\n   " + serverMsg);
    }
}


ImageItem tst_Lancelot::render(const ImageItem &item, GraphicsEngine engine, QImage::Format format)
{
    ImageItem res = item;
    res.imageChecksums.clear();
    res.image = QImage();
    QString filePath = scriptsDir + item.itemName;
    QStringList script = scripts.value(item.itemName);

    if (engine == Raster) {
        QImage img(800, 800, format);
        paint(&img, script, QFileInfo(filePath).absoluteFilePath()); // eh yuck (filePath stuff)
        res.image = img;
        res.imageChecksums.append(ImageItem::computeChecksum(img));
#ifndef QT_NO_OPENGL
    } else if (engine == OpenGL) {
        QGLWidget glWidget;
        if (glWidget.isValid()) {
            glWidget.makeCurrent();
            QGLFramebufferObjectFormat fboFormat;
            fboFormat.setSamples(16);
            fboFormat.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
            QGLFramebufferObject fbo(800, 800, fboFormat);
            paint(&fbo, script, QFileInfo(filePath).absoluteFilePath()); // eh yuck (filePath stuff)
            res.image = fbo.toImage().convertToFormat(format);
            res.imageChecksums.append(ImageItem::computeChecksum(res.image));
        }
#endif
    }

    return res;
}

void tst_Lancelot::paint(QPaintDevice *device, const QStringList &script, const QString &filePath)
{
    QPainter p(device);
    PaintCommands pcmd(script, 800, 800);
    //pcmd.setShouldDrawText(false);
    pcmd.setType(ImageType);
    pcmd.setPainter(&p);
    pcmd.setFilePath(filePath);
    pcmd.runCommands();
    p.end();

    if (simfail) {
        QPainter p2(device);
        p2.setPen(QPen(QBrush(Qt::cyan), 3, Qt::DashLine));
        p2.drawLine(200, 200, 600, 600);
        p2.drawLine(600, 200, 200, 600);
        simfail = false;
    }
}

#define main rmain
QTEST_MAIN(tst_Lancelot)
#undef main

int main(int argc, char *argv[])
{
    tst_Lancelot::clientInfo = PlatformInfo::localHostInfo();

    char *fargv[20];
    int fargc = 0;
    for (int i = 0; i < qMin(argc, 19); i++) {
        if (!qstrcmp(argv[i], "-simfail")) {
            tst_Lancelot::simfail = true;
        } else if (!qstrcmp(argv[i], "-compareto") && i < argc-1) {
            QString arg = QString::fromLocal8Bit(argv[++i]);
            int split = arg.indexOf(QLC('='));
            if (split < 0)
                continue;
            QString key = arg.left(split).trimmed();
            QString value = arg.mid(split+1).trimmed();
            if (key.isEmpty() || value.isEmpty())
                continue;
            tst_Lancelot::clientInfo.addOverride(key, value);
        } else {
            fargv[fargc++] = argv[i];
        }
    }
    fargv[fargc] = 0;
    return rmain(fargc, fargv);
}

#include "tst_lancelot.moc"
