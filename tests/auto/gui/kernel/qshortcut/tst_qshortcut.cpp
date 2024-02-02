// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtGui/qguiapplication.h>
#include <QtGui/qshortcut.h>
#include <QtGui/qwindow.h>
#include <QtTest/qsignalspy.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

class tst_QShortcut : public QObject
{
    Q_OBJECT

private slots:
    void applicationShortcut();
    void windowShortcut();
};

void tst_QShortcut::applicationShortcut()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    auto *shortcut = new QShortcut(Qt::CTRL | Qt::Key_A, this);
    shortcut->setContext(Qt::ApplicationShortcut);
    QSignalSpy activatedSpy(shortcut, &QShortcut::activated);

    // Need a window to send key event to, even if the shortcut is application
    // global. The documentation for Qt::ApplicationShortcut also says that
    // the shortcut "is active when one of the applications windows are active",
    // but this is only honored for Qt Widgets, not for Qt Gui. For now we
    // activate the window just in case.
    QWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QTRY_COMPARE(QGuiApplication::applicationState(), Qt::ApplicationActive);
    QTest::sendKeyEvent(QTest::Shortcut, &window, Qt::Key_A, 'a', Qt::ControlModifier);

    QVERIFY(activatedSpy.size() > 0);
}

void tst_QShortcut::windowShortcut()
{
    QWindow w;
    new QShortcut(Qt::CTRL | Qt::Key_Q, &w, SLOT(close()));
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QEXPECT_FAIL("", "It failed on Wayland, QTBUG-120334", Abort);

    QTRY_VERIFY(QGuiApplication::applicationState() == Qt::ApplicationActive);
    QTest::sendKeyEvent(QTest::Click, &w, Qt::Key_Q, 'q', Qt::ControlModifier);
    QTRY_VERIFY(!w.isVisible());
}

QTEST_MAIN(tst_QShortcut)
#include "tst_qshortcut.moc"
