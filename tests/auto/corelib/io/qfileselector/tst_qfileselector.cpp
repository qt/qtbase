/****************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qplatformdefs.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include <private/qfileselector_p.h>
#include <private/qabstractfileengine_p.h>
#include <private/qfsfileengine_p.h>
#include <private/qfilesystemengine_p.h>

const ushort selectorIndicator = '+';

class tst_QFileSelector : public QObject
{
    Q_OBJECT
public:
    tst_QFileSelector() {}

private slots:
    void basicTest_data();
    void basicTest();

    void urlConvenience_data();
    void urlConvenience();
};

void tst_QFileSelector::basicTest_data()
{
    /* Files existing for this test
     *  platform/test
     *  platform/+<platform>/test for all <platform> in QFileSelectorPrivate::platformSelectors()
     *  extras/test
     *  extras/test2 to test for when selector directories exist, but don't have the files
     *  extras/+custom1/test
     *  extras/+custom1/test3 to test for when base file doesn't exist
     *  extras/+custom2/test
     *  extras/+custom3/test
     *  extras/+custom3/+custom2/test
     *  extras/+custom3/+custom4/test
     *  extras/+custom3/+custom5/test
     *  extras/+custom5/+custom3/test
     */
    QTest::addColumn<QString>("testPath");
    QTest::addColumn<QStringList>("customSelectors");
    QTest::addColumn<QString>("expectedPath");

    QString test("/test");// '/' is here so dir string can also be selector string
    QString test2("/test2");
    QString test3("/test3");
    QString expectedPlatform1File(":/platforms");
    QString expectedPlatform2File(""); //Only the last selector
    QString expectedPlatform3File; // Only the first selector (the family)
#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_BLACKBERRY) && \
    !defined(Q_OS_DARWIN) && !defined(Q_OS_LINUX) && !defined(Q_OS_HAIKU)
    /* We are only aware of specific unixes, and do not have test files for any of the others.
       However those unixes can get a selector added from the result of a uname call, so this will
       lead to a case where we don't have that file so we can't expect the concatenation of platform
       selectors to work. It should just find the +unix/test file.*/
    expectedPlatform1File = QString(":/platforms/") + QLatin1Char(selectorIndicator)
        + QString("unix/test");
    expectedPlatform2File = QString(":/platforms/test2");
#else
    QString distributionName;
#  if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_FREEBSD) || defined(Q_OS_WINRT)
    distributionName = QSysInfo::productType();
#  endif
    foreach (const QString &selector, QFileSelectorPrivate::platformSelectors()) {
        // skip the Linux distribution name (if any) since we don't have files for them
        if (selector == distributionName)
            continue;

        expectedPlatform1File = expectedPlatform1File + QLatin1Char('/') + QLatin1Char(selectorIndicator)
            + selector;
        expectedPlatform2File = selector;
        if (expectedPlatform3File.isNull())
            expectedPlatform3File = selector;
    }
    expectedPlatform1File += test;
    expectedPlatform2File = QLatin1String(":/platforms/") + QLatin1Char(selectorIndicator)
        + expectedPlatform2File + test2;
    expectedPlatform3File = QLatin1String(":/platforms/") + QLatin1Char(selectorIndicator)
        + expectedPlatform3File + test3;
#endif

    QTest::newRow("platform1") <<  QString(":/platforms/test") << QStringList()
        << expectedPlatform1File;

    QTest::newRow("platform2") <<  QString(":/platforms/test2") << QStringList()
        << expectedPlatform2File;

    QTest::newRow("platform3") << QString(":/platforms/test3") << QStringList()
                               << expectedPlatform3File;

    QString resourceTestPath(":/extras/test");
    QString custom1("custom1");
    QTest::newRow("custom1-noselector") << resourceTestPath << QStringList()
        << QString(":/extras") + test;

    QTest::newRow("custom1-withselector") << resourceTestPath << (QStringList() << custom1)
        << QString(":/extras/")  + QLatin1Char(selectorIndicator) + custom1 + test;

    QTest::newRow("customX-withselector-nofile") << QString(":/extras/test2") << (QStringList() << custom1)
        << QString(":/extras/test2");

    QTest::newRow("custom1-withselector-nobasefile") << QString(":/extras/test3") << (QStringList() << custom1)
        << QString(":/extras/test3");

    QString custom2("custom2");
    QString custom3("custom3");
    QString custom4("custom4");
    QString custom5("custom5");
    QString slash("/");
    QTest::newRow("custom12") << resourceTestPath << (QStringList() << custom1 << custom2)
        << QString(":/extras/")  + QLatin1Char(selectorIndicator) + custom1 + test;

    QTest::newRow("custom21") << resourceTestPath << (QStringList() << custom2 << custom1)
        << QString(":/extras/")  + QLatin1Char(selectorIndicator) + custom2 + test;

    QTest::newRow("custom213") << resourceTestPath << (QStringList() << custom2 << custom1 << custom3)
        << QString(":/extras/")  + QLatin1Char(selectorIndicator) + custom2 + test;

    QTest::newRow("custom23") << resourceTestPath << (QStringList() << custom2 << custom3)
        << QString(":/extras/")  + QLatin1Char(selectorIndicator) + custom2 + test;

    QTest::newRow("custom34nested") << resourceTestPath << (QStringList() << custom3 << custom4)
        << QString(":/extras/")  + QLatin1Char(selectorIndicator) + custom3 + slash
           + QLatin1Char(selectorIndicator) + custom4 + test;

    QTest::newRow("custom43nested") << resourceTestPath << (QStringList() << custom4 << custom3)
        << QString(":/extras/")  + QLatin1Char(selectorIndicator) + custom3 + slash
           + QLatin1Char(selectorIndicator) + custom4 + test;

    QTest::newRow("custom35conflict") << resourceTestPath << (QStringList() << custom3 << custom5)
        << QString(":/extras/")  + QLatin1Char(selectorIndicator) + custom3 + slash
           + QLatin1Char(selectorIndicator) + custom5 + test;

    QTest::newRow("relativePaths") << QFINDTESTDATA("extras/test") << (QStringList() << custom1)
        << QFINDTESTDATA(QString("extras/") + QLatin1Char(selectorIndicator) + custom1
           + QString("/test"));
}

void tst_QFileSelector::basicTest()
{
    QFETCH(QString, testPath);
    QFETCH(QStringList, customSelectors);
    QFETCH(QString, expectedPath);

    QFileSelector fs;
    fs.setExtraSelectors(customSelectors);
    QCOMPARE(fs.select(testPath), expectedPath);
}

void tst_QFileSelector::urlConvenience_data()
{
    /* Files existing for this test
     *  extras/test
     *  extras/+custom1/test
     */
    QTest::addColumn<QUrl>("testUrl");
    QTest::addColumn<QStringList>("customSelectors");
    QTest::addColumn<QUrl>("expectedUrl");

    QString test("/test");// '/' is here so dir string can also be selector string
    QString custom1("custom1");

    QTest::newRow("qrc") << QUrl("qrc:///extras/test") << (QStringList() << custom1)
        << QUrl(QString("qrc:///extras/") + QLatin1Char(selectorIndicator) + custom1 + test);

    QString fileBasePath = QFINDTESTDATA("extras/test");
    QString fileSelectedPath = QFINDTESTDATA(QString("extras/") + QLatin1Char(selectorIndicator)
            + custom1 + QString("/test"));
    QTest::newRow("file") << QUrl::fromLocalFile(fileBasePath) << (QStringList() << custom1)
        << QUrl::fromLocalFile(fileSelectedPath);

    // http://qt-project.org/images/qtdn/sprites-combined-latest.png is chosen as a representative real world URL
    // But note that this test is checking that http urls are NOT selected so it shouldn't be checked
    QUrl testHttpUrl("http://qt-project.org/images/sprites-combined-latest.png");
    QTest::newRow("http") << testHttpUrl << (QStringList() << QString("qtdn")) << testHttpUrl;
}

void tst_QFileSelector::urlConvenience()
{
    QFETCH(QUrl, testUrl);
    QFETCH(QStringList, customSelectors);
    QFETCH(QUrl, expectedUrl);

    QFileSelector fs;
    //All rows of this test use only custom selectors, so should not select before the setExtra call
    QCOMPARE(fs.select(testUrl), testUrl);
    fs.setExtraSelectors(customSelectors);
    QCOMPARE(fs.select(testUrl), expectedUrl);
}

QTEST_MAIN(tst_QFileSelector)
#include "tst_qfileselector.moc"
