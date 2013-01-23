/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
    void listAliases_data();
    void listAliases();
    void icons();
    void mimeTypeForFileWithContent();
    void mimeTypeForUrl();
    void mimeTypeForData_data();
    void mimeTypeForData();
    void mimeTypeForFileAndContent_data();
    void mimeTypeForFileAndContent();
    void allMimeTypes();
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
    QString m_qmlAgainFileName;
    QTemporaryDir m_temporaryDir;
    QString m_testSuite;
};

#endif   // TST_QMIMEDATABASE_H
