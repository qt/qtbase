/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "performancediff.h"

#include "xmldata.h"

#include <QtXml>

#include <QFile>
#include <QStringList>
#include <QDateTime>
#include <QtDebug>

#include <iostream>
#include <iomanip>


static const int MIN_TEST_VAL = 20;
static const int TEST_EPSILON = 5; //ms

static void usage(const char *progname)
{
    std::cerr << "Couldn't find 'framework.ini' "
              << "file and no output has been specified."<<std::endl;
    std::cerr << "Usage: "<<progname
              << " oldDataDir"
              << " newDataDir\n"
              << std::endl;
}


PerformanceDiff::PerformanceDiff()
    : settings(0)
{
    if (QFile::exists("framework.ini")) {
        settings = new QSettings("framework.ini", QSettings::IniFormat);
    }
}

static void loadEngines(const QString &dirName,
                        QMap<QString, XMLEngine*> &engines)
{
    QDir dir(dirName);
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

}
void PerformanceDiff::run(int argc, char **argv)
{
    processArguments(argc, argv);

    loadEngines(inputDirName, inputEngines);
    loadEngines(diffDirName, diffEngines);

    if (inputEngines.isEmpty() || diffEngines.isEmpty()) {
        usage(argv[0]);
        return;
    }

    generateDiff();
    //generateOutput();
}

void PerformanceDiff::processArguments(int argc, char **argv)
{
    if (argc != 3)
        return;
    inputDirName = QString(argv[1]);
    diffDirName  = QString(argv[2]);
}

void PerformanceDiff::generateDiff()
{
    qreal totalIn   = 0;
    qreal totalDiff = 0;

    std::cout<<std::setiosflags(std::ios::left)<<std::setw(30)<<"Testcase"
             <<std::setiosflags(std::ios::right)
             <<std::setw(10) <<"Before"
             <<std::setw(15) <<"After"
             <<std::setw(20) <<"Difference"<<std::endl;
    std::cout << std::resetiosflags(std::ios::right);
    std::cout << std::resetiosflags(std::ios::left);
    std::cout<<std::setfill('-')<<std::setw(75)<<'-'<<std::endl;
    foreach(XMLEngine *diffEngine, diffEngines) {
        XMLEngine *inEngine = inputEngines[diffEngine->name];
        if (!inEngine)
            continue;
        foreach(XMLSuite *diffSuite, diffEngine->suites) {
            XMLSuite *inSuite = inEngine->suites[diffSuite->name];
            if (!inSuite)
                continue;

            foreach(XMLFile *diffFile, diffSuite->files) {
                XMLFile *inFile = inSuite->files[diffFile->name];
                if (!inFile)
                    continue;

                qreal inAvg   = 0;
                qreal diffAvg = 0;
                qreal inMin   = 0;
                qreal inMax   = 0;
                foreach(XMLData data, inFile->data) {
                    inAvg = (double(data.timeToRender)/data.iterations);
                    if (!inMin)
                        inMin = data.minElapsed;
                    else if (data.minElapsed < inMin)
                        inMin = data.minElapsed;
                    if (!inMax)
                        inMax = data.maxElapsed;
                    else if (inMax < data.maxElapsed)
                        inMax = data.maxElapsed;
                }
                //skipping really small tests
                if (inAvg < MIN_TEST_VAL) {
                    continue;
                }

                totalIn += inAvg;
                foreach(XMLData data, diffFile->data) {
                    diffAvg = (double(data.timeToRender)/data.iterations);
                }
                totalDiff += diffAvg;

                QFileInfo fi(diffFile->name);
                std::cout.width(80);
                std::cout.setf(std::ios::fixed, std::ios::floatfield);
                std::cout.setf(std::ios::showpoint);
                std::cout << std::resetiosflags(std::ios::right);
                std::cout << std::resetiosflags(std::ios::left);
                std::cout<<std::setw(30)<<std::setfill('.')<<std::setiosflags(std::ios::left)
                         <<qPrintable(fi.fileName())<<"\t";
                std::cout<<std::setfill(' ')<<std::setprecision(2)
                         <<std::setiosflags(std::ios::right)
                         <<std::setw(8)<<std::setiosflags(std::ios::right)<<inAvg<<"\t"
                         <<std::setw(8)<<diffAvg<<"\t"
                         <<std::setw(7)<< ((1.0-(diffAvg/inAvg))*100.0) <<"%";
                if (diffAvg < inMin &&
                    (qAbs(inMin - diffAvg) > TEST_EPSILON)) {
                    std::cout<<" + ("<<inMin<<")";
                }
                if (diffAvg > inMax &&
                    (qAbs(diffAvg - inMax) > TEST_EPSILON)) {
                    std::cout<<" - ("<<inMax<<")";
                }

                std::cout<<std::endl;
            }
        }
    }
    std::cout << std::resetiosflags(std::ios::right);
    std::cout << std::resetiosflags(std::ios::left);
    std::cout<<std::setfill('-')<<std::setw(75)<<'-'<<std::endl;
}



