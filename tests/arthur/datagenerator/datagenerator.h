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
#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include "xmlgenerator.h"
#include "framework.h"

#include <QTextStream>
#include <QSettings>
#include <QSize>
#include <QColor>

QT_FORWARD_DECLARE_CLASS(QSvgRenderer)
QT_FORWARD_DECLARE_CLASS(QEngine)
QT_FORWARD_DECLARE_CLASS(QFileInfo)

class DataGenerator
{
public:
    DataGenerator();
    ~DataGenerator();

    void run(int argc, char **argv);
private:
    bool processArguments(int argc, char **argv);
    void testEngines(XMLGenerator &generator, const QString &file,
                     const QString &refUrl);
    void testDirectory(const QString &dirname, const QString &refUrl);
    void testFile(const QString &file, const QString &refUrl,
                  QTextStream &out, QTextStream &hout);
    void testGivenFile();
    void testSuite(XMLGenerator &generator, const QString &suite,
                   const QString &dirName, const QString &refUrl);
    void prepareDirs();

    void testGivenEngines(const QList<QEngine*> engines,
                          const QFileInfo &fileInfo,
                          const QString &file,
                          XMLGenerator &generator,
                          GeneratorFlags flags);
    void testGivenEngines(const QList<QEngine*> engines,
                          const QFileInfo &fileInfo,
                          const QString &file,
                          XMLGenerator &generator,
                          int iterations,
                          GeneratorFlags flags);

    bool wantedEngine(const QString &engine) const;
private:
    QSvgRenderer *renderer;
    Framework settings;

    QString engineName;
    QString suiteName;
    QString testcase;
    QString fileName;
    QString outputDirName;
    QString baseDataDir;
    int     iterations;
    QSize   size;
    QColor  fillColor;
};

#endif
