// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QPointer>
#include "QtCore/qapplicationstatic.h"

Q_APPLICATION_STATIC(QObject, tstObject)

class tst_qapplicationstatic : public QObject
{
    Q_OBJECT

private slots:
    void testCreateMultipleApplications() const;
};

void tst_qapplicationstatic::testCreateMultipleApplications() const
{
    for (int i = 0; i < 5; i++) {
        int argc = 1;
        char *argv[] = { (char *)"tst_qapplicationstatic" };
        auto app = new QCoreApplication(argc, argv);

        QVERIFY(tstObject);

        QPointer<QObject> tstObjectPointer(tstObject);
        QVERIFY(tstObjectPointer.get());

        QVERIFY2(tstObject->objectName().isEmpty(), "Got QObject from previous iteration, not correctly recreated");
        tstObject->setObjectName(QStringLiteral("tstObject"));
        QVERIFY(!tstObject->objectName().isEmpty());

        delete app;
        QVERIFY2(!tstObjectPointer.get(), "QObject wasn't destroyed on QCoreApplication destruction");
    }
}

QTEST_APPLESS_MAIN(tst_qapplicationstatic)
#include "tst_qapplicationstatic.moc"
