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
#ifndef XMLGENERATOR_H
#define XMLGENERATOR_H

#include "xmldata.h"

#include <QDateTime>
#include <QMap>
#include <QList>
#include <QString>


class XMLGenerator
{
public:
    XMLGenerator(const QString &baseDir);

    void startSuite(const QString &name);
    void startTestcase(const QString &testcase);
    void addImage(const QString &engine, const QString &image,
                  const XMLData &data, GeneratorFlags flags);
    void endTestcase();
    void endSuite();

    void checkDirs(const QDateTime &dt, const QString &baseDir);
    void generateOutput(const QString &baseDir);
    QString generateData() const;
private:
    QMap<QString, XMLEngine*> engines;
    QString currentSuite;
    QString currentTestcase;
};

#endif
