// Copyright (C) 2017 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QCoreApplication>
#include <QTest>

class tst_DeleteLater_noApp: public QObject
{
    Q_OBJECT

private slots:
    void qtestLibShouldNotFlushDeleteLaterBetweenTests_setup();
    void qtestLibShouldNotFlushDeleteLaterBetweenTests_check();
    void qtestLibShouldNotFlushDeleteLaterOnExit();
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

void tst_DeleteLater_noApp::qtestLibShouldNotFlushDeleteLaterBetweenTests_setup()
{
    ToBeDeleted *obj = new ToBeDeleted(&deletedBetweenTests);
    obj->deleteLater();
}

void tst_DeleteLater_noApp::qtestLibShouldNotFlushDeleteLaterBetweenTests_check()
{
    // There's no qApp, we can't flush the events
    QVERIFY(!deletedBetweenTests);
}

static bool deletedOnExit = false;

void tst_DeleteLater_noApp::qtestLibShouldNotFlushDeleteLaterOnExit()
{
    ToBeDeleted *obj = new ToBeDeleted(&deletedOnExit);
    obj->deleteLater();
}

// This global object will check whether the deleteLater was processed
class DeleteChecker
{
public:
    ~DeleteChecker() {
        if (deletedOnExit) {
            qFatal("QTestLib somehow flushed deleteLater on exit, without a qApp?");
        }
    }
};
static DeleteChecker s_deleteChecker;

QTEST_APPLESS_MAIN(tst_DeleteLater_noApp)

#include "tst_deleteLater_noApp.moc"
