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
#include <QtGui>
#include <QtWidgets>
#include <QtCore>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <qdebug.h>

#include "../network-settings.h"


class tst_QNetworkAccessManager_And_QProgressDialog : public QObject
{
    Q_OBJECT
public:
    tst_QNetworkAccessManager_And_QProgressDialog();
private slots:
    void downloadCheck();
    void downloadCheck_data();
};

class DownloadCheckWidget : public QWidget
{
    Q_OBJECT
public:
    DownloadCheckWidget(QWidget *parent = 0) :
        QWidget(parent), lateReadyRead(true), zeroCopy(false),
        progressDlg(this), netmanager(this)
    {
        progressDlg.setRange(1, 100);
        QMetaObject::invokeMethod(this, "go", Qt::QueuedConnection);
    }
    bool lateReadyRead;
    bool zeroCopy;
public slots:
    void go()
    {
        QNetworkRequest request(QUrl("http://" + QtNetworkSettings::serverName() + "/qtest/bigfile"));
        if (zeroCopy)
            request.setAttribute(QNetworkRequest::MaximumDownloadBufferSizeAttribute, 10*1024*1024);
                
        QNetworkReply *reply = netmanager.get(
                QNetworkRequest(
                QUrl("http://" + QtNetworkSettings::serverName() + "/qtest/bigfile")
                ));
        connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
                this, SLOT(dataReadProgress(qint64, qint64)));
        connect(reply, SIGNAL(readyRead()),
                this, SLOT(dataReadyRead()));
        connect(reply, SIGNAL(finished()), this, SLOT(finishedFromReply()));

        progressDlg.exec();
    }
    void dataReadProgress(qint64 done, qint64 total)
    {
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
        Q_UNUSED(reply);
        progressDlg.setMaximum(total);
        progressDlg.setValue(done);
    }
    void dataReadyRead()
    {
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
        Q_UNUSED(reply);
        lateReadyRead = true;
    }
    void finishedFromReply()
    {
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
        lateReadyRead = false;
        reply->deleteLater();
        QTestEventLoop::instance().exitLoop();
    }

private:
    QProgressDialog progressDlg;
    QNetworkAccessManager netmanager;
};


tst_QNetworkAccessManager_And_QProgressDialog::tst_QNetworkAccessManager_And_QProgressDialog()
{
    Q_SET_DEFAULT_IAP
}

void tst_QNetworkAccessManager_And_QProgressDialog::downloadCheck_data()
{
    QTest::addColumn<bool>("useZeroCopy");
    QTest::newRow("with-zeroCopy") << true;
    QTest::newRow("without-zeroCopy") << false;
}

void tst_QNetworkAccessManager_And_QProgressDialog::downloadCheck()
{
    QFETCH(bool, useZeroCopy);

    DownloadCheckWidget widget;
    widget.zeroCopy = useZeroCopy;
    widget.show();
    // run and exit on finished()
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
    // run some more to catch the late readyRead() (or: to not catch it)
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(QTestEventLoop::instance().timeout());
    // the following fails when a readyRead() was received after finished()
    QVERIFY(!widget.lateReadyRead);
}



QTEST_MAIN(tst_QNetworkAccessManager_And_QProgressDialog)
#include "tst_qnetworkaccessmanager_and_qprogressdialog.moc"
