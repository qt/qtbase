/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QStandardItemModel>
#include <qdebug.h>
#include <qdesktopservices.h>

//TESTED_CLASS=
//TESTED_FILES=

class tst_qdesktopservices : public QObject
{
    Q_OBJECT

public:
    tst_qdesktopservices();
    virtual ~tst_qdesktopservices();

private slots:
    void init();
    void cleanup();
    void openUrl();
    void handlers();
    void storageLocation_data();
    void storageLocation();

    void storageLocationDoesNotEndWithSlash_data();
    void storageLocationDoesNotEndWithSlash();
};

tst_qdesktopservices::tst_qdesktopservices()
{
    QCoreApplication::setOrganizationName("Nokia");
    QCoreApplication::setApplicationName("tst_qdesktopservices");
}

tst_qdesktopservices::~tst_qdesktopservices()
{
}

void tst_qdesktopservices::init()
{
}

void tst_qdesktopservices::cleanup()
{
}

void tst_qdesktopservices::openUrl()
{
    // At the bare minimum check that they return false for invalid url's
    QCOMPARE(QDesktopServices::openUrl(QUrl()), false);
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    // this test is only valid on windows on other systems it might mean open a new document in the application handling .file
    QCOMPARE(QDesktopServices::openUrl(QUrl("file://invalid.file")), false);
#endif
}

class MyUrlHandler : public QObject
{
    Q_OBJECT
public:
    QUrl lastHandledUrl;

public slots:
    inline void handle(const QUrl &url) {
        lastHandledUrl = url;
    }
};

void tst_qdesktopservices::handlers()
{
    MyUrlHandler fooHandler;
    MyUrlHandler barHandler;

    QDesktopServices::setUrlHandler(QString("foo"), &fooHandler, "handle");
    QDesktopServices::setUrlHandler(QString("bar"), &barHandler, "handle");

    QUrl fooUrl("foo://blub/meh");
    QUrl barUrl("bar://hmm/hmmmm");

    QVERIFY(QDesktopServices::openUrl(fooUrl));
    QVERIFY(QDesktopServices::openUrl(barUrl));

    QCOMPARE(fooHandler.lastHandledUrl.toString(), fooUrl.toString());
    QCOMPARE(barHandler.lastHandledUrl.toString(), barUrl.toString());
}

Q_DECLARE_METATYPE(QDesktopServices::StandardLocation)
void tst_qdesktopservices::storageLocation_data()
{
    QTest::addColumn<QDesktopServices::StandardLocation>("location");
    QTest::newRow("DesktopLocation") << QDesktopServices::DesktopLocation;
    QTest::newRow("DocumentsLocation") << QDesktopServices::DocumentsLocation;
    QTest::newRow("FontsLocation") << QDesktopServices::FontsLocation;
    QTest::newRow("ApplicationsLocation") << QDesktopServices::ApplicationsLocation;
    QTest::newRow("MusicLocation") << QDesktopServices::MusicLocation;
    QTest::newRow("MoviesLocation") << QDesktopServices::MoviesLocation;
    QTest::newRow("PicturesLocation") << QDesktopServices::PicturesLocation;
    QTest::newRow("TempLocation") << QDesktopServices::TempLocation;
    QTest::newRow("HomeLocation") << QDesktopServices::HomeLocation;
    QTest::newRow("DataLocation") << QDesktopServices::DataLocation;
}

void tst_qdesktopservices::storageLocation()
{
    QFETCH(QDesktopServices::StandardLocation, location);
    QDesktopServices::storageLocation(location);
    QDesktopServices::displayName(location);
}

void tst_qdesktopservices::storageLocationDoesNotEndWithSlash_data()
{
    storageLocation_data();
}

void tst_qdesktopservices::storageLocationDoesNotEndWithSlash()
{
    // Currently all desktop locations return their storage location
    // with "Unix-style" paths (i.e. they use a slash, not backslash).
    QFETCH(QDesktopServices::StandardLocation, location);
    QString loc = QDesktopServices::storageLocation(location);
    if (loc.size() > 1)  // workaround for unlikely case of locations that return '/'
        QCOMPARE(loc.endsWith(QLatin1Char('/')), false);
}


QTEST_MAIN(tst_qdesktopservices)
#include "tst_qdesktopservices.moc"
