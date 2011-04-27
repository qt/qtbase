/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "datagenerator.h"

#include "qengines.h"
#include "xmlgenerator.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSvgRenderer>
#include <QImage>
#include <QPainter>
#include <QProcess>
#include <QSettings>
#include <QtDebug>

#include <iostream>

#define W3C_SVG_BASE "http://www.w3.org/Graphics/SVG/Test/20030813/png/"

static QString createW3CReference(const QString &refUrl, const QString &filename)
{
    QString base(refUrl);

    QString pngFile = filename;
    pngFile.replace(".svg", ".png");

    base += "full-";

    base += pngFile;
    return base;
}


static QString createOutFilename(const QString &baseDir, const QString &filename,
                                 QEngine *engine)
{
    QString outFile = filename;
    if (outFile.endsWith(".svgz"))
        outFile.replace(".svgz", ".png");
    else
        outFile.replace(".svg", ".png");

    outFile.replace(".qps", ".qps.png");

    if (!baseDir.isEmpty())
        outFile = QString("%1/%2/%3").arg(baseDir)
                  .arg(engine->name()).arg(outFile);
    else
        outFile = QString("%1/%2").arg(engine->name())
                  .arg(outFile);

    return outFile;
}

static void usage(const char *progname)
{
    std::cerr << "Couldn't find 'framework.ini' "
              << "file and no suite has been specified."<<std::endl;
    std::cerr << "Usage: "<<progname <<  "\n"
              << "\t-framework <framework.ini>\n"
              << "\t-engine <engine name>|onscreen|printing\n"
              << "\t-suite <suite name>\n"
              << "\t-testcase <file.svg>\n"
              << "\t-file </path/to/file.svg>\n"
              << "\t-size <width,height>\n"
              << "\t-fill <color>\n"
              << "\t-output <dirname>\n"
              << std::endl;
}

DataGenerator::DataGenerator()
    : iterations(1)
    , size(480, 360)
    , fillColor(Qt::white)
{
    settings.load(QString("framework.ini"));
    renderer = new QSvgRenderer();
}

DataGenerator::~DataGenerator()
{
}

void DataGenerator::run(int argc, char **argv)
{
    if (!processArguments(argc, argv))
        return;

    if (!fileName.isEmpty()) {
        testGivenFile();
        return;
    }
    if (!settings.isValid() && suiteName.isEmpty()) {
        usage(argv[0]);
        return;
    }

    prepareDirs();
    if (!settings.isValid()) { //only suite specified
        XMLGenerator generator(outputDirName);
        testSuite(generator, suiteName,
                  QString(), QString());
        return;
    }

    XMLGenerator generator(outputDirName);
    QStringList tests = settings.suites();
    qDebug()<<"tests = "<<tests;
    foreach(QString test, tests) {
        if (!suiteName.isEmpty() && test != suiteName)
            continue;

        qDebug()<<"testing "<<test;
        settings.settings()->beginGroup(test);

        QString dirName = settings.settings()->value("dir").toString();
        QString refUrl  = settings.settings()->value("reference").toString();

        QDir dir(dirName);
        if (!dir.isAbsolute() && !dir.exists())
            dir = QDir(QString("%1/%2").arg(baseDataDir).arg(dirName));

        testSuite(generator, test, dir.absolutePath(), refUrl);
        settings.settings()->endGroup();
    }
    generator.generateOutput(outputDirName);
}

void DataGenerator::testEngines(XMLGenerator &generator, const QString &file,
                                const QString &refUrl)
{
    QFileInfo fileInfo(file);

    generator.startTestcase(file);

    if (!refUrl.isEmpty()) {
        QString ref = createW3CReference(refUrl, fileInfo.fileName());
        generator.addImage("Reference", ref, XMLData(), Reference);
    }

    //bool isQpsScript = file.endsWith(".qps");
    bool isSvgFile = file.endsWith(".svg") || file.endsWith(".svgz");

    if (isSvgFile) {
        QDir oldDir = QDir::current();
        if (!baseDataDir.isEmpty()) {
            QDir::setCurrent(baseDataDir);
        }
        renderer->load(fileInfo.absoluteFilePath());
        if (!baseDataDir.isEmpty()) {
            QDir::setCurrent(oldDir.absolutePath());
        }
        if (!renderer->isValid()) {
            qWarning()<<"Error while processing " <<file;
            return;
        }
    }

    QList<QEngine*> engines = QtEngines::self()->engines();
    testGivenEngines(engines, fileInfo, file, generator, Normal);

    engines = QtEngines::self()->foreignEngines();
    testGivenEngines(engines, fileInfo, file, generator, Foreign);

    generator.endTestcase();
}

void DataGenerator::prepareDirs()
{
    QList<QEngine*> engines = QtEngines::self()->engines();
    QDir outDirs;
    foreach(QEngine *engine, engines) {
        if (!wantedEngine(engine->name()))
            continue;

        QString dirName = engine->name();
        if (!outputDirName.isEmpty())
            dirName = QString("%1/%2").arg(outputDirName).arg(dirName);
        outDirs.mkpath(dirName);
    }

    engines = QtEngines::self()->foreignEngines();
    foreach(QEngine *engine, engines) {
        if (!wantedEngine(engine->name()))
            continue;

        QString dirName = engine->name();
        if (!outputDirName.isEmpty())
            dirName = QString("%1/%2").arg(outputDirName).arg(dirName);
        outDirs.mkpath(dirName);
    }
}

bool DataGenerator::processArguments(int argc, char **argv)
{
    QString frameworkFile;
    for (int i=1; i < argc; ++i) {
        QString opt(argv[i]);
        if (opt == "-framework") {
            frameworkFile = QString(argv[i+1]);
        } else if (opt == "-engine") {
            engineName = QString(argv[i+1]);
        } else if (opt == "-suite") {
            suiteName = QString(argv[i+1]);
        } else if (opt == "-testcase") {
            testcase = QString(argv[i+1]);
        } else if (opt == "-file") {
            fileName = QString(argv[i+1]);
        } else if (opt == "-output") {
            outputDirName = QString(argv[i+1]);
        } else if (opt == "-iterations") {
            iterations = QString(argv[i+1]).toInt();
        } else if (opt == "-size") {
            QStringList args = QString(argv[i+1]).split(",", QString::SkipEmptyParts);
            size = QSize(args[0].toInt(), args.size() > 1 ? args[1].toInt() : args[0].toInt());
        } else if (opt == "-fill") {
            fillColor = QColor(QString(argv[i+1]));
        } else if (opt.startsWith('-')) {
            qDebug()<<"Unknown option "<<opt;
        }
    }
    if (!frameworkFile.isEmpty() && QFile::exists(frameworkFile)) {
        baseDataDir = QFileInfo(frameworkFile).absoluteDir().absolutePath();
        settings.load(frameworkFile);
    }

    if (outputDirName.isEmpty() && settings.isValid()) {
        outputDirName = settings.outputDir();
    }

    if (!outputDirName.isEmpty()) {
        QDir dir;
        dir.mkpath(outputDirName);
    }

    
    if (!engineName.isEmpty()) {
        QList<QEngine *> engines = QtEngines::self()->engines();
        bool found = false;
        if (engineName == QLatin1String("onscreen")||
            engineName == QLatin1String("printing"))
            found = true;
        else {
            for (int i=0; i<engines.size(); ++i)
                found |= (engines.at(i)->name() == engineName);
        }
        if (!found) {
            qDebug("No such engine: '%s'\nAvailable engines are:", qPrintable(engineName));
            for (int i=0; i<engines.size(); ++i)
                qDebug("  %s", qPrintable(engines.at(i)->name()));
            return false;
        }
    }

    if (!fileName.isEmpty()) {
        baseDataDir = QFileInfo(fileName).absoluteDir().absolutePath();
    }

    return true;
}

void DataGenerator::testGivenFile()
{
    prepareDirs();

    XMLGenerator generator(baseDataDir);
    generator.startSuite("Single");

    QFileInfo fileInfo(fileName);

    bool qpsScript = fileName.endsWith("qps");
    if (!qpsScript) {
        QDir oldDir = QDir::current();
        if (!baseDataDir.isEmpty()) {
            QDir::setCurrent(baseDataDir);
        }
        renderer->load(fileInfo.absoluteFilePath());

        if (!baseDataDir.isEmpty()) {
            QDir::setCurrent(oldDir.absolutePath());
        }

        if (!renderer->isValid()) {
            qWarning()<<"Error while processing " <<fileInfo.absolutePath();
            return;
        }
    }

    QList<QEngine*> engines = QtEngines::self()->engines();
    testGivenEngines(engines, fileInfo, fileInfo.fileName(), generator,
                     Normal);
    generator.endSuite();

    std::cout<< qPrintable(generator.generateData());
}

void DataGenerator::testSuite(XMLGenerator &generator, const QString &test,
                              const QString &dirName, const QString &refUrl)
{
    generator.startSuite(test);

    QDir dir(dirName);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    //dir.setNameFilter()

    foreach (QFileInfo fileInfo, dir.entryInfoList()) {
        if (!testcase.isEmpty() && fileInfo.fileName() != testcase)
            continue;
        QString suffix = fileInfo.suffix().toLower();
        if (suffix != "qps" && suffix != "svg" && suffix != "svgz")
            continue;
        qDebug()<<"Testing: "<<fileInfo.absoluteFilePath();
        testEngines(generator, fileInfo.absoluteFilePath(), refUrl);
    }

    generator.endSuite();
}

static QString loadFile(const QString &name)
{
    QFile file(name);
    if (!file.open(QFile::ReadOnly)) {
        qDebug("Can't open file '%s'", qPrintable(name));
        return QString();
    }
    QTextStream str(&file);
    return str.readAll();
}

void DataGenerator::testGivenEngines(const QList<QEngine*> engines,
                                     const QFileInfo &fileInfo,
                                     const QString &file,
                                     XMLGenerator &generator,
                                     GeneratorFlags eflags)
{
    QString fileName = fileInfo.absoluteFilePath();
    bool qpsScript = fileName.endsWith(".qps");
    QStringList qpsContents;
    if (qpsScript) {
        QString script = loadFile(fileName);
        qpsContents = script.split("\n", QString::SkipEmptyParts);
    }

    //foreign one don't generate qpsScripts
    if ((eflags & Foreign) && qpsScript)
        return;

    foreach (QEngine *engine, engines) {
        if (!wantedEngine(engine->name()))
            continue;
        if (settings.isTestBlacklisted(engine->name(), fileInfo.fileName())) {
            XMLData data;
            data.details    = QString("Test blacklisted");
            data.iterations = 1;
            generator.addImage(engine->name(), QString(""), data, eflags);
            continue;
        }

        QString outFilename = createOutFilename(outputDirName,
                                                fileInfo.fileName(), engine);
        engine->prepare(qpsScript ? QSize(800, 800) : size, fillColor);
        int elapsed = -1;
        int maxElapsed = 0;
        int minElapsed = 0;
        if ((eflags & Foreign)) {
            engine->render(renderer, file);
            engine->save(outFilename);
        } else {
            bool saved = false;
            //only measure Qt engines
            QTime time;
            int currentElapsed = 0;
            for (int i = 0; i < iterations; ++i) {
                if (qpsScript) {
                    QDir oldDir = QDir::current();
                    if (!baseDataDir.isEmpty()) {
                        QDir::setCurrent(baseDataDir+"/images");
                    }
                    time.start();
                    engine->render(qpsContents, fileName);
                    currentElapsed = time.elapsed();
                    if (!baseDataDir.isEmpty()) {
                        QDir::setCurrent(oldDir.absolutePath());
                    }
                } else {
                    time.start();
                    engine->render(renderer, file);
                    currentElapsed = time.elapsed();
                }
                if (currentElapsed > maxElapsed)
                    maxElapsed = currentElapsed;
                if (!minElapsed ||
                    currentElapsed < minElapsed)
                    minElapsed = currentElapsed;
                elapsed += currentElapsed;
                if (!saved) {
                    //qDebug()<<"saving "<<i<<engine->name();
                    engine->save(outFilename);
                    engine->cleanup();
                    engine->prepare(size, fillColor);
                    saved = true;
                }
            }
            engine->cleanup();
        }
        GeneratorFlags flags = Normal;
        if (QtEngines::self()->defaultEngine() == engine)
            flags |= Default;
        flags |= eflags;
        if ((eflags & Foreign))
            flags ^= Normal;
        XMLData data;
        data.date = QDateTime::currentDateTime();
        data.timeToRender = elapsed;
        data.iterations = iterations;
        data.maxElapsed = maxElapsed;
        data.minElapsed = minElapsed;
        generator.addImage(engine->name(), outFilename,
                           data, flags);
    }
}

bool DataGenerator::wantedEngine(const QString &engine) const
{
    if (!engineName.isEmpty() &&
        engine != engineName) {
        if (engineName == "onscreen") {
            if (engine.startsWith("Native") ||
                engine == QLatin1String("Raster") ||
                engine == QLatin1String("OpenGL"))
                return true;
        } else if (engineName == QLatin1String("printing")) {
            if (engine == QLatin1String("PS") ||
                engine == QLatin1String("PDF"))
                return true;
        }
        return false;
    }
    return true;
}
