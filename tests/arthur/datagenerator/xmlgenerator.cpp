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
#include "xmlgenerator.h"

#include "qengines.h"

#include <QtXml>
#include <QDir>

XMLGenerator::XMLGenerator(const QString &baseDir)
{
    QList<QEngine*> qengines = QtEngines::self()->engines();
    foreach(QEngine *engine, qengines) {
        QString engineDir = engine->name();
        QString fileName = engineDir + "/" + "data.xml";

        if (!baseDir.isEmpty()) {
            engineDir = QString("%1/%2").arg(baseDir).arg(engineDir);
            fileName = QString("%1/%2").arg(baseDir).arg(fileName);
        }

        if (!QFile::exists(fileName))
            continue;


        XMLReader handler;
        QXmlSimpleReader reader;
        reader.setContentHandler(&handler);
        reader.setErrorHandler(&handler);

        QFile file(fileName);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qWarning("Cannot open file '%s', because: %s",
                     qPrintable(fileName), qPrintable(file.errorString()));
            continue;
        }

        QXmlInputSource xmlInputSource(&file);
        if (reader.parse(xmlInputSource)) {
            XMLEngine *engine = handler.xmlEngine();
            checkDirs(engine->generationDate, engineDir);
            engines.insert(engine->name, engine);
        }
    }
}


void XMLGenerator::startSuite(const QString &name)
{
    currentSuite = name;
}


void XMLGenerator::startTestcase(const QString &testcase)
{
    currentTestcase = testcase;
}


void XMLGenerator::addImage(const QString &engineName, const QString &image,
                            const XMLData &data, GeneratorFlags flags)
{
    XMLEngine *engine;
    if (engines.contains(engineName))
        engine = engines[engineName];
    else {
        engine = new XMLEngine(engineName, flags & Default);
        engine->defaultEngine = (flags & Default);
        engine->foreignEngine = (flags & Foreign);
        engine->referenceEngine = (flags & Reference);
        engine->generationDate = QDateTime::currentDateTime();
        engines.insert(engineName, engine);
    }

    XMLSuite *suite;
    if (engine->suites.contains(currentSuite))
        suite = engine->suites[currentSuite];
    else {
        suite = new XMLSuite(currentSuite);
        engine->suites.insert(currentSuite, suite);
    }

    XMLFile *file;
    if (suite->files.contains(currentTestcase))
        file = suite->files[currentTestcase];
    else {
        file = new XMLFile(currentTestcase);
        suite->files.insert(currentTestcase, file);
    }

    file->output = image;
    file->data  += data;
}


void XMLGenerator::endTestcase()
{

}


void XMLGenerator::endSuite()
{

}

static void generateDataFile(QTextStream &out, XMLEngine *engine)
{
    QString indent;
    out << "<arthur engine=\""<<engine->name<<"\" default=\"";
    if (engine->defaultEngine) {
        out<<"true\"";
    } else
        out<<"false\"";

    out << " foreign=\""     << (engine->foreignEngine?"true":"false")
        << "\" reference=\"" << (engine->referenceEngine?"true":"false")
        << "\" generationDate=\"" << (engine->generationDate.toString())
        << "\">\n";

    indent += "  ";
    foreach(XMLSuite *suite, engine->suites) {
        out << indent << "<suite dir=\"" << suite->name << "\">\n";
        indent += "  ";

        foreach(XMLFile *file, suite->files) {
            out << indent << "<file name=\""<<file->name<<"\" output=\""<<file->output<<"\">\n";
            indent += "  ";
            foreach(XMLData data, file->data) {
                out << indent
                    << "<data date=\""<<data.date.toString()
                    << "\" time_to_render=\""<<data.timeToRender
                    << "\" iterations=\""<<data.iterations
                    << "\" details=\""<<data.details
                    << "\" maxElapsed=\""<<data.maxElapsed
                    << "\" minElapsed=\""<<data.minElapsed
                    << "\" />\n";
            }
            indent.chop(2);
            out << indent << "</file>\n";
        }

        indent.chop(2);
        out << indent << "</suite>\n";
    }

    out << "</arthur>";
}

void XMLGenerator::generateOutput(const QString &baseDir)
{
    QDir dir;
    if (!baseDir.isEmpty()) {
        dir = QDir(baseDir);
    }
    foreach(XMLEngine *engine, engines) {
        QFile file(QString("%1/%2/data.xml").arg(dir.absolutePath())
                   .arg(engine->name));

        dir.mkpath(QFileInfo(file).absolutePath());

        if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
            fprintf(stderr, "Failed to open output file '%s' for writing\n",
                    qPrintable(QFileInfo(file).absoluteFilePath()));
            return;
        }
        QTextStream out(&file);
        generateDataFile(out, engine);
    }
}

QString XMLGenerator::generateData() const
{
    QString str;
    foreach(XMLEngine *engine, engines) {
        QTextStream out(&str);
        generateDataFile(out, engine);
    }
    return str;
}

void XMLGenerator::checkDirs(const QDateTime &currentDate, const QString &engineDir)
{
    QDateTime yesterday = QDateTime::currentDateTime();
    QDateTime lastWeek  = QDateTime::currentDateTime();
    yesterday = yesterday.addDays(-1);
    lastWeek  = lastWeek.addDays(-7);

    if (currentDate <= yesterday) {
        QString newDir = engineDir + ".yesterday";
        if (QFile::exists(engineDir)) {
            //### handle last week
            QString oldFileName = QString("%1/data.xml").arg(newDir);
            XMLReader handler;
            QXmlSimpleReader reader;
            reader.setContentHandler(&handler);
            reader.setErrorHandler(&handler);
            QFile file(oldFileName);
            if (file.open(QFile::ReadOnly | QFile::Text)) {
                QXmlInputSource xmlInputSource(&file);
                if (reader.parse(xmlInputSource)) {
                    XMLEngine *engine = handler.xmlEngine();
                    if (engine->generationDate <= lastWeek) {
                        QString newDir = engineDir + ".lastweek";
                        qDebug()<<"Backing last weeks's "<< qPrintable(engine->name);
                        QStringList args;
                        args << "-rf";
                        args << engineDir;
                        args << newDir;
                        QProcess::execute("cp", args);
                    }
                }
            }
        }
        qDebug()<<"Backing yesterday's "<< engineDir;
        QStringList args;
        args << "-rf";
        args << engineDir;
        args << newDir;
        QProcess::execute("cp", args);
    }
}


