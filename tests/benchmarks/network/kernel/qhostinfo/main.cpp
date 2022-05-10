// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QDebug>
#include <QHostInfo>
#include <QStringList>
#include <QString>

#include <qtest.h>
#include <qtesteventloop.h>

#include "private/qhostinfo_p.h"

class tst_qhostinfo : public QObject
{
    Q_OBJECT
public slots:
    void init();
private slots:
    void lookupSpeed_data();
    void lookupSpeed();
};

class SignalReceiver : public QObject
{
    Q_OBJECT
public:
    SignalReceiver(int nrc) : receiveCount(0), neededReceiveCount(nrc) {};
    int receiveCount;
    int neededReceiveCount;
public slots:
    void resultsReady(const QHostInfo) {
        receiveCount++;
        if (receiveCount == neededReceiveCount)
            QTestEventLoop::instance().exitLoop();
    }
};

void tst_qhostinfo::init()
{
    // delete the cache so inidividual testcase results are independent from each other
    qt_qhostinfo_clear_cache();
}

void tst_qhostinfo::lookupSpeed_data()
{
    QTest::addColumn<bool>("cache");
    QTest::newRow("WithCache") << true;
    QTest::newRow("WithoutCache") << false;
}

void tst_qhostinfo::lookupSpeed()
{
    QFETCH(bool, cache);
    qt_qhostinfo_enable_cache(cache);

    QStringList hostnameList;
    hostnameList << "www.ovi.com" << "www.nokia.com" << "qt-project.org" << "www.trolltech.com" << "troll.no"
            << "www.qtcentre.org" << "forum.nokia.com" << "www.forum.nokia.com" << "wiki.forum.nokia.com"
            << "www.nokia.no" << "nokia.de" << "127.0.0.1" << "----";
    // also add some duplicates:
    hostnameList << "www.nokia.com" << "127.0.0.1" << "www.trolltech.com";
    // and some more
    hostnameList << hostnameList;

    const int COUNT = hostnameList.size();

    SignalReceiver receiver(COUNT);

    QBENCHMARK {
        for (int i = 0; i < hostnameList.size(); i++)
            QHostInfo::lookupHost(hostnameList.at(i), &receiver, SLOT(resultsReady(QHostInfo)));
        QTestEventLoop::instance().enterLoop(20);
        QVERIFY(!QTestEventLoop::instance().timeout());
    }
}


QTEST_MAIN(tst_qhostinfo)

#include "main.moc"
