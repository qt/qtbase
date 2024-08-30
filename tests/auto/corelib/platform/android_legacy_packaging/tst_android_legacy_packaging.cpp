// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

using namespace Qt::StringLiterals;

class tst_AndroidLegacyPackaging : public QObject
{
Q_OBJECT
private slots:
    void initTestCase();
};

void tst_AndroidLegacyPackaging::initTestCase()
{ }


QTEST_MAIN(tst_AndroidLegacyPackaging)
#include "tst_android_legacy_packaging.moc"
