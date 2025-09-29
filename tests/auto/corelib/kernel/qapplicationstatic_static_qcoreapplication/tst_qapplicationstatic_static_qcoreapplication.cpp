// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include "QtCore/qapplicationstatic.h"

// In a 'realistic' scenario this QCoreApplication would be in a separate
// TU. But for this test it suffices to just force it to be initialized before
// the application static object
static int argc = 1;
static char arg1[] = "./tst_qapplicationstatic_static_qcoreapplication";
static char *argv[] = { arg1, nullptr };
// Note: the order of these two statics matters because it defines the order of
// initialization within this TU!
static QCoreApplication app(argc, argv);
Q_APPLICATION_STATIC(QObject, tstObject)

class tst_qapplicationstatic_static_qcoreapplication : public QObject
{
    Q_OBJECT

private slots:
    void initialize() const;
};

void tst_qapplicationstatic_static_qcoreapplication::initialize() const
{
    tstObject(); // Just initialize the object
    // What we are interested in testing is destruction!
    // Prior to fixing QTBUG-140413 this scenario would result in a crash on
    // exit.
    // What happened is that, because of the static-deinitialization order,
    // we would invoke the dtor of the APPLICATION_STATIC, which deletes the
    // contained object. Next in order is the QCoreApplication, which emits the
    // destroyed() signal, to which APPLICATION_STATIC is connected.
    // It then attempted to destroy the contained object again, leading to a
    // double-free / "heap-use-after-delete".
}

QTEST_APPLESS_MAIN(tst_qapplicationstatic_static_qcoreapplication)
#include "tst_qapplicationstatic_static_qcoreapplication.moc"
