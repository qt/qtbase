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


#include <QtTest/QtTest>
#include <qdebug.h>
#include <qdesktopservices.h>

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
    void testDataLocation();
};

tst_qdesktopservices::tst_qdesktopservices()
{
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

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#define Q_XDG_PLATFORM
#endif

void tst_qdesktopservices::testDataLocation()
{
    // This is the one point where QDesktopServices and QStandardPaths differ.
    // QDesktopServices on unix returns "data"/orgname/appname for DataLocation, for Qt4 compat.
    // And the appname in qt4 defaulted to empty, not to argv[0].
    {
        const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        const QString app = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#ifdef Q_XDG_PLATFORM
        QCOMPARE(app, base + "/data//"); // as ugly as in Qt4
#else
        QCOMPARE(app, base);
#endif
    }
    QCoreApplication::instance()->setOrganizationName("Qt");
    QCoreApplication::instance()->setApplicationName("QtTest");
    {
        const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        const QString app = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#ifdef Q_XDG_PLATFORM
        QCOMPARE(app, base + "/data/Qt/QtTest");
#else
        QCOMPARE(app, base + "/Qt/QtTest");
#endif
    }
}

QTEST_MAIN(tst_qdesktopservices)

#include "tst_qdesktopservices.moc"
