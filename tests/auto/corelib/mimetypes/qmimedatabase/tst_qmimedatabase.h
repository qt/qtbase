// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    void mimeTypesForFileName_glob_deleteall();
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
    void mimeTypeForFileNameAndData_data();
    void mimeTypeForFileNameAndData();
#ifdef Q_OS_UNIX
    void mimeTypeForUnixSpecials_data();
    void mimeTypeForUnixSpecials();
#endif
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
