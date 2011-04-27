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
#include "htmlgenerator.h"

#include "xmldata.h"

#include <QtXml>

#include <QFile>
#include <QStringList>
#include <QDateTime>
#include <QtDebug>

#include <iostream>

static void generateIndex(QTextStream &out)
{
    out <<"\
<html>\n\
  <head><title>SVG rendering comparison</title></head>\n\
  <body bgcolor=\"white\">\n\
  <h1>QSvg testing framework</h1>\n\
  <table border=\"1\">\n\
  <tr><td>Testing suite</td><td>History</td></tr>\n\
  <tr>\n\
      <td><a href=\"test1.1-full.html\">1.1</a></td>\n\
      <td><a href=\"test1.1-history.html\">1.1 history</a></td>\n\
  </tr>\n\
  <tr>\n\
      <td><a href=\"test1.2-full.html\">1.2 testing suite</a></td>\n\
      <td><a href=\"test1.2-history.html\">1.2 QSvg history</a></td>\n\
  </tr>\n\
  <tr>\n\
      <td><a href=\"testrandom-full.html\">Random testing suite</a></td>\n\
      <td><a href=\"testrandom-history.html\">Random tests QSvg history</a></td>\n\
  </tr>\n\
  </body>\n\
</html>\n";
}


static void usage(const char *progname)
{
    std::cerr << "Couldn't find 'framework.ini' "
              << "file and no output has been specified."<<std::endl;
    std::cerr << "Usage: "<<progname
              << " -framework <framework.ini>"
              << " dirname\n"
              << std::endl;
}


HTMLGenerator::HTMLGenerator()
    : settings(0)
{
    if (QFile::exists("framework.ini")) {
        settings = new QSettings("framework.ini", QSettings::IniFormat);
    }
}

void HTMLGenerator::generateIndex(const QString &)
{

}

void HTMLGenerator::generatePages()
{
    foreach(HTMLSuite *suite, suites) {
        generateSuite(*suite);
    }
}

struct HTMLPage
{
    QString pageName;
    QStringList headings;
    QList<HTMLRow> rows;
};
void HTMLGenerator::generateSuite(const HTMLSuite &suite)
{
    generateReferencePage(suite);
    generateHistoryPages(suite);
    generateQtComparisonPage(suite);
}

void HTMLGenerator::generateReferencePage(const HTMLSuite &suite)
{

    bool generateReference = false;
    QStringList generators;
    foreach(HTMLRow *row, suite.rows) {
        foreach(HTMLImage refs, row->referenceImages) {
            generators += refs.generatorName;
            foreach(HTMLImage img, row->images) {
                if ((img.flags & Default)) {
                    generators += img.generatorName;
                    break;
                }
            }
            foreach(HTMLImage img, row->foreignImages) {
                generators += img.generatorName;
            }
            generateReference = true;
            break;
        }
        if (generateReference)
            break;
    }

    if (!generateReference)
        return;

    QFile file(QString("test-%1-reference.html").arg(suite.name));
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        return;
    }

    QTextStream out(&file);
    generateHeader(out, "Reference Page", generators);

    foreach(HTMLRow *row, suite.rows) {
        bool referenceRow = false;
        QList<HTMLImage> images;
        foreach(HTMLImage refs, row->referenceImages) {
            startGenerateRow(out, row->testcase);
            referenceRow = true;
            images.append(refs);
            break;
        }
        if (referenceRow) {
            foreach(HTMLImage img, row->images) {
                if ((img.flags & Default)) {
                    images.append(img);
                    break;
                }
            }
            images << row->foreignImages;

            generateImages(out, images);
            finishGenerateRow(out, row->testcase);
        }
    }

    generateFooter(out);
}

void HTMLGenerator::generateHistoryPages(const HTMLSuite &suite)
{
    QStringList lst;
    foreach(XMLEngine *engine, engines) {
        generateHistoryForEngine(suite, engine->name);
    }
}

void HTMLGenerator::generateHistoryForEngine(const HTMLSuite &suite, const QString &engine)
{
    QFile file(QString("test-%1-%2-history.html").arg(suite.name).arg(engine));
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        return;
    }

    QTextStream out(&file);

    QStringList generators;
    foreach(HTMLRow *row, suite.rows) {
        foreach(HTMLImage refs, row->referenceImages) {
            generators += refs.generatorName;
            generators += "Today";
            generators += "Yesterday";
            generators += "Last Week";
            break;
        }
        if (!generators.isEmpty())
            break;
    }
    if (generators.isEmpty()) {
        generators += "Today";
        generators += "Yesterday";
        generators += "Last Week";
    }
    generateHeader(out, QString("History for %1 engine").arg(engine), generators);

    foreach(HTMLRow *row, suite.rows) {
        QList<HTMLImage> images;
        QStringList generators;
        foreach(HTMLImage refs, row->referenceImages) {
            generators += refs.generatorName;
            images.append(refs);
            break;
        }

        startGenerateRow(out, row->testcase);
        foreach(HTMLImage img, row->images) {
            if (img.generatorName == engine) {
                images << img;
            }
        }

        generateHistoryImages(out, images);
        finishGenerateRow(out, row->testcase);
    }

    generateFooter(out);
}


void HTMLGenerator::generateQtComparisonPage(const HTMLSuite &suite)
{
    QFile file(QString("test-%1-comparison.html").arg(suite.name));
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        return;
    }

    QTextStream out(&file);

    QStringList lst;
    foreach(XMLEngine *engine, engines) {
        if (!engine->foreignEngine && !engine->referenceEngine)
            lst += engine->name;
    }

    generateHeader(out, QString("Qt Engine Comparison"), lst);
    foreach(HTMLRow *row, suite.rows) {
        QList<HTMLImage> images;

        startGenerateRow(out, row->testcase);
        foreach(HTMLImage img, row->images) {
            images.append(img);
        }
        generateImages(out, images);
        finishGenerateRow(out, row->testcase);
    }

    generateFooter(out);
}


void HTMLGenerator::generateHeader(QTextStream &out, const QString &name,
                                   const QStringList &generators)
{
    out << "<html>\n"
        << "<head><title>"<<name<<"</title></head>\n"
        << "<body bgcolor=\"white\">\n"
        << "<a href=\"index.html\">Click here to go back to main page</a>\n"
        << "<p><center><h2> Generated: "<<QDateTime::currentDateTime().toString()
        <<"</h2></center></p>\n"
        << "<table border=\"1\">\n";

    out << "<tr>";
    foreach(QString generator, generators) {
        out <<"<td><b>"<<generator<<"</b></td>\n";
    }
    out<<"</tr>\n";

}

void HTMLGenerator::startGenerateRow(QTextStream &out, const QString &name)
{
    Q_UNUSED(name);
    out <<"  <tr>\n";
}

void HTMLGenerator::generateImages(QTextStream &out,
                                   const QList<HTMLImage> &images)
{
    out <<"  <tr>\n";
    foreach(HTMLImage image, images) {
        out <<"     <td valign=top><img src=\""<< image.file <<"\"></td>\n";
    }
    out <<"  </tr>\n";
    out <<"  <tr>\n";
    foreach(HTMLImage image, images) {
        out <<"     <td><center>"
            << image.generatorName << ": "
            << image.details <<" ms </center></td>\n";
    }
}

void HTMLGenerator::generateHistoryImages(QTextStream &out,
                                          const QList<HTMLImage> &images)
{
    foreach(HTMLImage image, images) {
        if ((image.flags & Reference)) {
            out <<"     <td><img src=\""<< image.file
                <<"\"  width=480 height=360></td>\n";
        } else {
            QString genName = image.generatorName;
            QString file = image.file.replace(image.generatorName, "");
            out <<"     <td><img src=\""<<image.generatorName
                << file <<"\"></td>\n"
                <<"     <td><img src=\""<<image.generatorName
                <<".yesterday"<< file <<"\"></td>\n"
                <<"     <td><img src=\""<<image.generatorName
                <<".lastweek"<< file <<"\"></td>\n";
        }
    }
}


void HTMLGenerator::finishGenerateRow(QTextStream &out, const QString &name)
{
    out <<"  </tr>\n"
        <<"  <tr><td colspan=5 bgcolor=yellow><center>"
        <<name<<"</center></td>\n";
}

void HTMLGenerator::generateFooter(QTextStream &out)
{
    out << "</table>\n"
        << "</body>\n"
        << "</html>\n";
}

void HTMLGenerator::run(int argc, char **argv)
{
    processArguments(argc, argv);

    QDir dir;
    dir.setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);

        QString dataFile = QString("%1/data.xml")
                           .arg(fileInfo.absoluteFilePath());
        if (QFile::exists(dataFile)) {
            XMLReader handler;
            QXmlSimpleReader reader;
            reader.setContentHandler(&handler);
            reader.setErrorHandler(&handler);

            QFile file(dataFile);
            if (!file.open(QFile::ReadOnly | QFile::Text)) {
                qWarning("Cannot open file '%s', because: %s",
                         qPrintable(dataFile), qPrintable(file.errorString()));
                continue;
            }

            QXmlInputSource xmlInputSource(&file);
            if (reader.parse(xmlInputSource)) {
                XMLEngine *engine = handler.xmlEngine();
                engines.insert(engine->name, engine);
            }
        }
    }

    if (engines.isEmpty()) {
        usage(argv[0]);
        return;
    }

    convertToHtml();
    generatePages();
}

void HTMLGenerator::processArguments(int argc, char **argv)
{
    QString frameworkFile;
    for (int i=1; i < argc; ++i) {
        QString opt(argv[i]);
        if (opt == "-framework") {
            frameworkFile = QString(argv[++i]);
        } else {
            outputDirName = opt;
        }
    }

    if (!frameworkFile.isEmpty() && QFile::exists(frameworkFile)) {
        delete settings;
        baseDataDir = QFileInfo(frameworkFile).absoluteDir().absolutePath();
        settings = new QSettings(frameworkFile, QSettings::IniFormat);
    }

    if (!outputDirName.isEmpty()) {
        QDir::setCurrent(outputDirName);
    }
    htmlOutputDir = QString("html");
    QDir dir;
    dir.mkpath(htmlOutputDir);
}

void HTMLGenerator::convertToHtml()
{
    foreach(XMLEngine *engine, engines) {
        foreach(XMLSuite *suite, engine->suites) {
            QString refUrl;
            QString refPrefix;
            if (settings) {
                settings->beginGroup(suite->name);
                refUrl = settings->value("reference").toString();
                refPrefix = settings->value("referencePrefix").toString();
                if (refUrl.endsWith('/'))
                    refUrl.chop(1);
                settings->endGroup();
            }

            foreach(XMLFile *file, suite->files) {
                HTMLImage image;
                image.file = file->output;
                image.generatorName = engine->name;

                image.details = file->data.last().iterations == 0
                                ? QString::number(-1)
                                : QString::number(file->data.last().timeToRender
                                                  / file->data.last().iterations);
                image.flags = Normal;

                if (file->data.last().timeToRender == 0)
                    image.details = file->data.last().details;

                if (engine->defaultEngine)
                    image.flags |= Default;
                if (engine->foreignEngine) {
                    image.flags ^= Normal;
                    image.flags |= Foreign;
                }
                if (engine->referenceEngine) {
                    image.flags ^= Normal;
                    image.flags |= Reference;
                }

                if (!outputDirName.isEmpty() && image.file.startsWith(outputDirName))
                    image.file.remove(0, outputDirName.length() + 1); // + '/'
                HTMLSuite *htmlSuite = suites[suite->name];
                if (!htmlSuite) {
                    htmlSuite = new HTMLSuite;
                    htmlSuite->name = suite->name;
                    suites.insert(suite->name, htmlSuite);
                }
                HTMLRow *htmlRow = htmlSuite->rows[file->name];
                if (!htmlRow) {
                    htmlRow = new HTMLRow;
                    htmlRow->testcase = file->name;
                    htmlSuite->rows.insert(file->name, htmlRow);
                }

                if ((image.flags & Foreign))
                    htmlRow->foreignImages.append(image);
                else if ((image.flags & Reference))
                    htmlRow->referenceImages.append(image);
                else {
                    htmlRow->images.append(image);
                }
                if (!refUrl.isEmpty()) {
                    QFileInfo fi(file->output);
                    HTMLImage image;
                    image.file = QString("%1/%2%3")
                                 .arg(refUrl)
                                 .arg(refPrefix)
                                 .arg(fi.fileName());
                    image.generatorName = QString("Reference");
                    image.details = QString("Reference");
                    image.flags = Reference;
                    if (htmlRow) {
                        htmlRow->referenceImages.append(image);
                    }
                }
            }
        }
    }
}

void HTMLGenerator::createPerformance()
{
#if 0
    QFile file(QString("test-performance.html"));
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        return;
    }

    QTextStream out(&file);
    foreach(XMLEngine *engine, engines) {
        QImage img = createHistoryImage(engine);
        QImage ;

    }
#endif
}


