// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QTimer>
#include <QSignalSpy>

#include <qguiapplication.h>
#include <qdebug.h>
#include <qsystemtrayicon.h>
#include <qmenu.h>

class tst_QSystemTrayIcon: public QObject
{
Q_OBJECT

public:
    tst_QSystemTrayIcon();
    virtual ~tst_QSystemTrayIcon();

private slots:
    void getSetCheck();
    void showHide();
    void showMessage();
    void supportsMessages();
    void lastWindowClosed();
};

tst_QSystemTrayIcon::tst_QSystemTrayIcon()
{
}

tst_QSystemTrayIcon::~tst_QSystemTrayIcon()
{
}

// Testing get/set functions
void tst_QSystemTrayIcon::showHide()
{
    QSystemTrayIcon icon;
    icon.setIcon(QIcon(":/icons/icon.png"));
    icon.show();
    icon.setIcon(QIcon(":/icons/icon.png"));
    icon.hide();
}

// Testing get/set functions
void tst_QSystemTrayIcon::showMessage()
{
    QSystemTrayIcon icon;
    icon.setIcon(QIcon(":/icons/icon.png"));

    icon.showMessage("Title", "Messagecontents");
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::NoIcon);
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::Warning);
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::Critical);

    icon.show();
    icon.showMessage("Title", "Messagecontents");
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::NoIcon);
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::Warning);
    icon.showMessage("Title", "Messagecontents", QSystemTrayIcon::Critical);
}

// Testing get/set functions
void tst_QSystemTrayIcon::getSetCheck()
{
    QSystemTrayIcon icon;
    QCOMPARE(true, icon.toolTip().isEmpty());
    icon.setToolTip("testToolTip");
    QCOMPARE(true, "testToolTip" == icon.toolTip());

    QCOMPARE(true, icon.icon().isNull());
    icon.setIcon(QIcon(":/icons/icon.png"));
    QCOMPARE(false, icon.icon().isNull());

    QMenu menu;
    QCOMPARE(true, icon.contextMenu() == 0);
    icon.setContextMenu(&menu);
    QCOMPARE(false, icon.contextMenu() == 0);
}

void tst_QSystemTrayIcon::supportsMessages()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    if (QGuiApplication::platformName() == u"offscreen")
        QSKIP("Offscreen rendering. Might or might not have system tray icon.");

    // ### fixme: Check platforms.
    const QString platform = QGuiApplication::platformName();
    if (platform.compare(QStringLiteral("xcb"), Qt::CaseInsensitive)
        && platform.compare(QStringLiteral("windows"), Qt::CaseInsensitive)
        && platform.compare(QStringLiteral("cocoa"), Qt::CaseInsensitive)) {
            QEXPECT_FAIL("", "QTBUG-20978 QSystemTrayIcon is unimplemented for this platform", Abort);
    }
    QCOMPARE(QSystemTrayIcon::supportsMessages(), true);
}

void tst_QSystemTrayIcon::lastWindowClosed()
{
    QSignalSpy spy(qApp, &QGuiApplication::lastWindowClosed);
    QWidget window;
    QSystemTrayIcon icon;
    icon.setIcon(QIcon(":/icons/icon.png"));
    icon.show();
    window.show();
    QTimer::singleShot(2500, &window, SLOT(close()));
    QTimer::singleShot(20000, qApp, SLOT(quit())); // in case the test fails
    qApp->exec();
    QCOMPARE(spy.size(), 1);
}

QTEST_MAIN(tst_QSystemTrayIcon)
#include "tst_qsystemtrayicon.moc"
