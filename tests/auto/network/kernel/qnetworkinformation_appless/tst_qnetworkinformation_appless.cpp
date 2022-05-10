// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qcoreapplication.h>
#include <QtNetwork/qnetworkinformation.h>
#include <QtTest/qtest.h>

class tst_QNetworkInformation_appless : public QObject
{
    Q_OBJECT
private slots:
    void reinit();
};

void tst_QNetworkInformation_appless::reinit()
{
    int argc = 1;
    char name[] = "./test";
    char *argv[] = { name, nullptr };

    {
        QCoreApplication app(argc, argv);
        if (QNetworkInformation::availableBackends().isEmpty())
            QSKIP("No backends available!");

        QVERIFY(QNetworkInformation::loadDefaultBackend());
        auto info = QNetworkInformation::instance();
        QVERIFY(info);
    }

    QVERIFY(!QNetworkInformation::instance());

    {
        QCoreApplication app(argc, argv);
        QVERIFY(QNetworkInformation::loadDefaultBackend());
        auto info = QNetworkInformation::instance();
        QVERIFY(info);
    }
}

QTEST_APPLESS_MAIN(tst_QNetworkInformation_appless);
#include "tst_qnetworkinformation_appless.moc"
