// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtGui/qwindow.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

class tst_Keyboard : public QObject
{
    Q_OBJECT

private slots:
    void keyPressAndRelease();
};

class KeyWindow : public QWindow
{
public:
    void keyPressEvent(QKeyEvent *event) override
    {
        QSharedPointer<QKeyEvent> copiedEvent(new QKeyEvent(event->type(), event->key(),
            event->modifiers(), event->text(), event->isAutoRepeat(), event->count()));
        mEventOrder.append(copiedEvent);
    }

    void keyReleaseEvent(QKeyEvent *event) override
    {
        QSharedPointer<QKeyEvent> copiedEvent(new QKeyEvent(event->type(), event->key(),
            event->modifiers(), event->text(), event->isAutoRepeat(), event->count()));
        mEventOrder.append(copiedEvent);
    }

    QList<QSharedPointer<QKeyEvent>> mEventOrder;
};

void tst_Keyboard::keyPressAndRelease()
{
    KeyWindow window;
    window.show();
    window.setGeometry(100, 100, 200, 200);
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation)) {
        QTest::ignoreMessage(QtWarningMsg,
                             "qWaitForWindowActive was called on a platform that doesn't support window "
                             "activation. This means there is an error in the test and it should either "
                             "check for the WindowActivation platform capability before calling "
                             "qWaitForWindowActivate, use qWaitForWindowExposed instead, or skip the test. "
                             "Falling back to qWaitForWindowExposed.");
    }
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QTest::keyPress(&window, Qt::Key_A);
    QTest::keyRelease(&window, Qt::Key_A);
    QCOMPARE(window.mEventOrder.size(), 2);

    const auto pressEvent = window.mEventOrder.at(0);
    QCOMPARE(pressEvent->type(), QEvent::KeyPress);
    QCOMPARE(pressEvent->key(), Qt::Key_A);
    QCOMPARE(pressEvent->modifiers(), Qt::NoModifier);
    QCOMPARE(pressEvent->text(), "a");
    QCOMPARE(pressEvent->isAutoRepeat(), false);

    const auto releaseEvent = window.mEventOrder.at(1);
    QCOMPARE(releaseEvent->type(), QEvent::KeyRelease);
    QCOMPARE(releaseEvent->key(), Qt::Key_A);
    QCOMPARE(releaseEvent->modifiers(), Qt::NoModifier);
    QCOMPARE(releaseEvent->text(), "a");
    QCOMPARE(releaseEvent->isAutoRepeat(), false);
}

QTEST_MAIN(tst_Keyboard)
#include "tst_keyboard.moc"
