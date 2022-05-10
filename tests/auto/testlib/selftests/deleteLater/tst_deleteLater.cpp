// Copyright (C) 2017 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QTest>

class tst_DeleteLater: public QObject
{
    Q_OBJECT

private slots:
    void qtestLibShouldFlushDeleteLaterBetweenTests_setup();
    void qtestLibShouldFlushDeleteLaterBetweenTests_check();
    void qtestLibShouldFlushDeleteLaterOnExit();
};

class ToBeDeleted : public QObject
{
public:
    ToBeDeleted(bool *staticBool) : staticBool(staticBool) {}
    ~ToBeDeleted() { *staticBool = true; }
private:
    bool *staticBool;
};

static bool deletedBetweenTests = false;

void tst_DeleteLater::qtestLibShouldFlushDeleteLaterBetweenTests_setup()
{
    ToBeDeleted *obj = new ToBeDeleted(&deletedBetweenTests);
    obj->deleteLater();
}

void tst_DeleteLater::qtestLibShouldFlushDeleteLaterBetweenTests_check()
{
    QVERIFY(deletedBetweenTests);
}

static bool deletedOnExit = false;

void tst_DeleteLater::qtestLibShouldFlushDeleteLaterOnExit()
{
    ToBeDeleted *obj = new ToBeDeleted(&deletedOnExit);
    obj->deleteLater();
}

// This global object will check whether the deleteLater was processed
class DeleteChecker
{
public:
    ~DeleteChecker() {
        if (!deletedOnExit) {
            qFatal("QTestLib failed to flush deleteLater on exit");
        }
    }
};
static DeleteChecker s_deleteChecker;

QTEST_MAIN(tst_DeleteLater)

#include "tst_deleteLater.moc"
