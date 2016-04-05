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


#include <QtTest/QtTest>
#include <QtWidgets/private/qsidebar_p.h>
#include <QtWidgets/private/qfilesystemmodel_p.h>

class tst_QSidebar : public QObject {
  Q_OBJECT

private slots:
    void setUrls();
    void selectUrls();
    void addUrls();

    void goToUrl();
};

void tst_QSidebar::setUrls()
{
    QList<QUrl> urls;
    QFileSystemModel fsmodel;
    QSidebar qsidebar;
    qsidebar.setModelAndUrls(&fsmodel, urls);
    QAbstractItemModel *model = qsidebar.model();

    urls << QUrl::fromLocalFile(QDir::rootPath())
         << QUrl::fromLocalFile(QDir::temp().absolutePath());

    QCOMPARE(model->rowCount(), 0);
    qsidebar.setUrls(urls);
    QCOMPARE(qsidebar.urls(), urls);
    QCOMPARE(model->rowCount(), urls.count());
    qsidebar.setUrls(urls);
    QCOMPARE(model->rowCount(), urls.count());
}

void tst_QSidebar::selectUrls()
{
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(QDir::rootPath())
         << QUrl::fromLocalFile(QDir::temp().absolutePath());
    QFileSystemModel fsmodel;
    QSidebar qsidebar;
    qsidebar.setModelAndUrls(&fsmodel, urls);

    QSignalSpy spy(&qsidebar, SIGNAL(goToUrl(QUrl)));
    qsidebar.selectUrl(urls.at(0));
    QCOMPARE(spy.count(), 0);
}

void tst_QSidebar::addUrls()
{
    QList<QUrl> emptyUrls;
    QFileSystemModel fsmodel;
    QSidebar qsidebar;
    qsidebar.setModelAndUrls(&fsmodel, emptyUrls);
    QAbstractItemModel *model = qsidebar.model();
    QDir testDir = QDir::home();

#if defined(Q_OS_ANDROID)
    // temp and home is the same directory on Android
    testDir.mkdir(QStringLiteral("test"));
    QVERIFY(testDir.cd(QStringLiteral("test")));
#endif

    // default
    QCOMPARE(model->rowCount(), 0);

    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(QDir::rootPath())
         << QUrl::fromLocalFile(QDir::temp().absolutePath());

    // test < 0
    qsidebar.addUrls(urls, -1);
    QCOMPARE(model->rowCount(), 2);

    // test = 0
    qsidebar.setUrls(emptyUrls);
    qsidebar.addUrls(urls, 0);
    QCOMPARE(model->rowCount(), 2);

    // test > 0
    qsidebar.setUrls(emptyUrls);
    qsidebar.addUrls(urls, 100);
    QCOMPARE(model->rowCount(), 2);

    // test inserting with already existing rows
    QList<QUrl> moreUrls;
    moreUrls << QUrl::fromLocalFile(testDir.absolutePath());
    qsidebar.addUrls(moreUrls, -1);
    QCOMPARE(model->rowCount(), 3);

    // make sure invalid urls are still added
    QList<QUrl> badUrls;
    badUrls << QUrl::fromLocalFile(testDir.absolutePath() + "/I used to exist");
    qsidebar.addUrls(badUrls, 0);
    QCOMPARE(model->rowCount(), 4);

    // check that every item has text and an icon including the above invalid one
    for (int i = 0; i < model->rowCount(); ++i) {
        QVERIFY(!model->index(i, 0).data().toString().isEmpty());
        QIcon icon = qvariant_cast<QIcon>(model->index(i, 0).data(Qt::DecorationRole));
        QVERIFY(!icon.isNull());
    }

    // test moving up the list
    qsidebar.setUrls(emptyUrls);
    qsidebar.addUrls(urls, 100);
    qsidebar.addUrls(moreUrls, 100);
    QCOMPARE(model->rowCount(), 3);
    qsidebar.addUrls(moreUrls, 1);
    QCOMPARE(qsidebar.urls()[1], moreUrls[0]);

    // test appending with -1
    qsidebar.setUrls(emptyUrls);
    qsidebar.addUrls(urls, -1);
    qsidebar.addUrls(moreUrls, -1);
    QCOMPARE(qsidebar.urls()[0], urls[0]);

    QList<QUrl> doubleUrls;
    //tow exact same paths, we have only one entry
    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath());
    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath());
    qsidebar.setUrls(emptyUrls);
    qsidebar.addUrls(doubleUrls, 1);
    QCOMPARE(qsidebar.urls().size(), 1);

    // Two paths that are effectively pointing to the same location
    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath());
    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath() + "/.");
    qsidebar.setUrls(emptyUrls);
    qsidebar.addUrls(doubleUrls, 1);
    QCOMPARE(qsidebar.urls().size(), 1);

    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath());
    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath().toUpper());
    qsidebar.setUrls(emptyUrls);
    qsidebar.addUrls(doubleUrls, 1);

#ifdef Q_OS_WIN
    //Windows is case insensitive so no duplicate entries in that case
    QCOMPARE(qsidebar.urls().size(), 1);
#else
    //Two different paths we should have two entries
    QCOMPARE(qsidebar.urls().size(), 2);
#endif
}

void tst_QSidebar::goToUrl()
{
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(QDir::rootPath())
         << QUrl::fromLocalFile(QDir::temp().absolutePath());
    QFileSystemModel fsmodel;
    QSidebar qsidebar;
    qsidebar.setModelAndUrls(&fsmodel, urls);
    qsidebar.show();

    QSignalSpy spy(&qsidebar, SIGNAL(goToUrl(QUrl)));
    QTest::mousePress(qsidebar.viewport(), Qt::LeftButton, 0, qsidebar.visualRect(qsidebar.model()->index(0, 0)).center());
    QCOMPARE(spy.count(), 1);
    QCOMPARE((spy.value(0)).at(0).toUrl(), urls.first());
}

QTEST_MAIN(tst_QSidebar)
#include "tst_qsidebar.moc"

