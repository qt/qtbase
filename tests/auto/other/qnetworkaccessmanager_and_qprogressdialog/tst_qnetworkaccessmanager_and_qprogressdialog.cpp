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
#include <QtGui>
#include <QtWidgets>
#include <QtCore>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <qdebug.h>

#include "../../network-settings.h"


class tst_QNetworkAccessManager_And_QProgressDialog : public QObject
{
    Q_OBJECT
public:
    tst_QNetworkAccessManager_And_QProgressDialog();
private slots:
    void initTestCase();
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
        connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
                this, SLOT(dataReadProgress(qint64,qint64)));
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
}

void tst_QNetworkAccessManager_And_QProgressDialog::initTestCase()
{
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
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
