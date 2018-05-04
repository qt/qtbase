/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/qtest.h>
#include <QtGui/qwindow.h>

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

    QVector<QSharedPointer<QKeyEvent>> mEventOrder;
};

void tst_Keyboard::keyPressAndRelease()
{
    KeyWindow window;
    window.show();
    window.setGeometry(100, 100, 200, 200);
    QVERIFY(QTest::qWaitForWindowExposed(&window));
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
