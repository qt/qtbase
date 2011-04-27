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
#ifndef HTMLGENERATOR_H
#define HTMLGENERATOR_H

#include "xmldata.h"

#include <QTextStream>
#include <QList>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QStringList)
QT_FORWARD_DECLARE_CLASS(QSettings)

struct HTMLImage
{
public:
    QString        file;
    QString        generatorName;
    GeneratorFlags flags;
    QString        details;
};

struct HTMLRow
{
public:
    QString testcase;
    QList<HTMLImage> referenceImages;
    QList<HTMLImage> foreignImages;
    QList<HTMLImage> images;
};

struct HTMLSuite
{
public:
    QString name;
    QMap<QString, HTMLRow*> rows;
};

class HTMLGenerator
{
public:
    HTMLGenerator();

    void startSuite(const QString &name);
    void startRow(const QString &testcase);
    void addImage(const QString &generator, const QString &image,
                  const QString &details, GeneratorFlags flags);
    void endRow();
    void endSuite();

    void generateIndex(const QString &file);
    void generatePages();

    void run(int argc, char **argv);

private:
    void generateSuite(const HTMLSuite &suite);

    void generateReferencePage(const HTMLSuite &suite);
    void generateHistoryPages(const HTMLSuite &suite);
    void generateHistoryForEngine(const HTMLSuite &suite, const QString &engine);
    void generateQtComparisonPage(const HTMLSuite &suite);

    void generateHeader(QTextStream &out, const QString &name,
                        const QStringList &generators);
    void startGenerateRow(QTextStream &out, const QString &name);
    void generateImages(QTextStream &out,
                        const QList<HTMLImage> &images);
    void generateHistoryImages(QTextStream &out,
                               const QList<HTMLImage> &images);
    void finishGenerateRow(QTextStream &out, const QString &name);
    void generateFooter(QTextStream &out);

    void processArguments(int argc, char **argv);
    void convertToHtml();
    void createPerformance();
private:
    QMap<QString, HTMLSuite*> suites;
    QMap<QString, XMLEngine*> engines;

    QSettings *settings;
    QString outputDirName;
    QString baseDataDir;
    QString htmlOutputDir;
};

#endif
