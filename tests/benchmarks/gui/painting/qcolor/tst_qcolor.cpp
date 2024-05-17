// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QColor>


class tst_QColor : public QObject
{
    Q_OBJECT

private slots:
    void nameRgb();
    void nameArgb();
};

void tst_QColor::nameRgb()
{
    QColor color(128, 255, 10);
    QCOMPARE(color.name(), QStringLiteral("#80ff0a"));
    QBENCHMARK {
        color.name();
    }
}

void tst_QColor::nameArgb()
{
    QColor color(128, 255, 0, 102);
    QCOMPARE(color.name(QColor::HexArgb), QStringLiteral("#6680ff00"));
    QBENCHMARK {
        color.name(QColor::HexArgb);
    }
}

QTEST_MAIN(tst_QColor)

#include "tst_qcolor.moc"
