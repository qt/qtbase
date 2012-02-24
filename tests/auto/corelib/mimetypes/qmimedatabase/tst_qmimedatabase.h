/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TST_QMIMEDATABASE_H
#define TST_QMIMEDATABASE_H

#include <QtCore/QObject>
#include <QtCore/QTemporaryDir>

class tst_QMimeDatabase : public QObject
{
    Q_OBJECT

public:
    tst_QMimeDatabase();

private slots:
    void initTestCase();

    void mimeTypeForName();
    void mimeTypeForFileName_data();
    void mimeTypeForFileName();
    void mimeTypesForFileName_data();
    void mimeTypesForFileName();
    void inheritance();
    void aliases();
    void icons();
    void mimeTypeForFileWithContent();
    void mimeTypeForUrl();
    void mimeTypeForData_data();
    void mimeTypeForData();
    void mimeTypeForFileAndContent_data();
    void mimeTypeForFileAndContent();
    void allMimeTypes();
    void inheritsPerformance();
    void suffixes_data();
    void suffixes();
    void knownSuffix();
    void fromThreads();

    // shared-mime-info test suite

    void findByFileName_data();
    void findByFileName();

    void findByData_data();
    void findByData();

    void findByFile_data();
    void findByFile();

    //

    void installNewGlobalMimeType();
    void installNewLocalMimeType();

private:
    void init(); // test-specific

    QString m_globalXdgDir;
    QString m_localXdgDir;
    QString m_yastMimeTypes;
    QTemporaryDir m_temporaryDir;
    QString m_testSuite;
};

#endif   // TST_QMIMEDATABASE_H
