// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qvariant.h>



class tst_QGuiVariantNoApplication : public QObject
{
    Q_OBJECT

public:
    tst_QGuiVariantNoApplication();

private slots:
    void variantWithoutApplication();
};

tst_QGuiVariantNoApplication::tst_QGuiVariantNoApplication()
{}

void tst_QGuiVariantNoApplication::variantWithoutApplication()
{
    QVariant v = QString("red");

    QCOMPARE(qvariant_cast<QColor>(v), QColor(Qt::red));
}


QTEST_APPLESS_MAIN(tst_QGuiVariantNoApplication)
#include "main.moc"
