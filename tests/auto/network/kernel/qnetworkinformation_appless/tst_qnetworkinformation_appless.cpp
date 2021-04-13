/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

        QVERIFY(QNetworkInformation::load(QNetworkInformation::Feature::Reachability));
        auto info = QNetworkInformation::instance();
        QVERIFY(info);
    }

    QVERIFY(!QNetworkInformation::instance());

    {
        QCoreApplication app(argc, argv);
        QVERIFY(QNetworkInformation::load(QNetworkInformation::Feature::Reachability));
        auto info = QNetworkInformation::instance();
        QVERIFY(info);
    }
}

QTEST_APPLESS_MAIN(tst_QNetworkInformation_appless);
#include "tst_qnetworkinformation_appless.moc"
