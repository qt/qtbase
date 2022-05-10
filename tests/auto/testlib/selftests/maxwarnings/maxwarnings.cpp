// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QTest>

class MaxWarnings: public QObject
{
    Q_OBJECT
private slots:
    void warn();
};

void MaxWarnings::warn()
{
    for (int i = 0; i < 10000; ++i)
        qWarning("%d", i);
}

QTEST_MAIN(MaxWarnings)
#include "maxwarnings.moc"
