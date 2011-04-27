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


#include <QtTest/QtTest>
#include "../../../src/gui/dialogs/qsidebar_p.h"
#include "../../../src/gui/dialogs/qfilesystemmodel_p.h"

//TESTED_CLASS=
//TESTED_FILES=

class tst_QSidebar : public QObject {
  Q_OBJECT

public:
    tst_QSidebar();
    virtual ~tst_QSidebar();

public Q_SLOTS:
    void init();
    void cleanup();

private slots:
    void setUrls();
    void selectUrls();
    void addUrls();

    void goToUrl();
};

tst_QSidebar::tst_QSidebar()
{
}

tst_QSidebar::~tst_QSidebar()
{
}

void tst_QSidebar::init()
{
}

void tst_QSidebar::cleanup()
{
}

void tst_QSidebar::setUrls()
{
    QList<QUrl> urls;
    QFileSystemModel fsmodel;
    QSidebar qsidebar;
    qsidebar.init(&fsmodel, urls);
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
    qsidebar.init(&fsmodel, urls);

    QSignalSpy spy(&qsidebar, SIGNAL(goToUrl(const QUrl &)));
    qsidebar.selectUrl(urls.at(0));
    QCOMPARE(spy.count(), 0);
}

void tst_QSidebar::addUrls()
{
    QList<QUrl> emptyUrls;
    QFileSystemModel fsmodel;
    QSidebar qsidebar;
    qsidebar.init(&fsmodel, emptyUrls);
    QAbstractItemModel *model = qsidebar.model();
#if defined(Q_OS_SYMBIAN)
    // On Symbian, QDir::rootPath() and QDir::home() are same.
    QDir testDir = QDir::currentPath();
#else
    QDir testDir = QDir::home();
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

#if defined(Q_OS_WIN)
    //Windows is case insensitive so no duplicate entries in that case
    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath());
    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath().toUpper());
    qsidebar.setUrls(emptyUrls);
    qsidebar.addUrls(doubleUrls, 1);
    QCOMPARE(qsidebar.urls().size(), 1);
#else
    //Two different paths we should have two entries
    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath());
    doubleUrls << QUrl::fromLocalFile(testDir.absolutePath().toUpper());
    qsidebar.setUrls(emptyUrls);
    qsidebar.addUrls(doubleUrls, 1);
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
    qsidebar.init(&fsmodel, urls);
    qsidebar.show();

    QSignalSpy spy(&qsidebar, SIGNAL(goToUrl(const QUrl &)));
    QTest::mousePress(qsidebar.viewport(), Qt::LeftButton, 0, qsidebar.visualRect(qsidebar.model()->index(0, 0)).center());
    QCOMPARE(spy.count(), 1);
    QCOMPARE((spy.value(0)).at(0).toUrl(), urls.first());
}

QTEST_MAIN(tst_QSidebar)
#include "tst_qsidebar.moc"

