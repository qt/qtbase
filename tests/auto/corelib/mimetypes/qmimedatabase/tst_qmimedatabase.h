/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TST_QMIMEDATABASE_H
#define TST_QMIMEDATABASE_H

#include <QtCore/QObject>
#include <QtCore/QTemporaryDir>
#include <QtCore/QStringList>

class tst_QMimeDatabase : public QObject
{
    Q_OBJECT

public:
    tst_QMimeDatabase();

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

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
    void comment();
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
    void symlinkToFifo();
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
    void initTestCaseInternal(); // test-specific

    QString m_globalXdgDir;
    QString m_localMimeDir;
    QStringList m_additionalMimeFileNames;
    QStringList m_additionalMimeFilePaths;
    QTemporaryDir m_temporaryDir;
    QString m_testSuite;
    bool m_isUsingCacheProvider;
};

#endif   // TST_QMIMEDATABASE_H
