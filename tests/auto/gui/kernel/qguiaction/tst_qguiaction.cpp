/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtTest/QtTest>

#include <qguiapplication.h>
#include <qevent.h>
#include <qguiaction.h>
#include <qguiactiongroup.h>
#include <qpa/qplatformtheme.h>

#include <private/qguiapplication_p.h>

class tst_QGuiAction : public QObject
{
    Q_OBJECT

public:
    tst_QGuiAction();

private slots:
    void cleanup();
    void getSetCheck();
    void setText_data();
    void setText();
    void setIconText_data() { setText_data(); }
    void setIconText();
#if QT_CONFIG(shortcut)
    void setStandardKeys();
    void task200823_tooltip();
#endif
    void task229128TriggeredSignalWithoutActiongroup();
    void setData();
    void setEnabledSetVisible();

private:
    const int m_keyboardScheme;
};

tst_QGuiAction::tst_QGuiAction()
    : m_keyboardScheme(QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::KeyboardScheme).toInt())
{
}

void tst_QGuiAction::cleanup()
{
    QVERIFY(QGuiApplication::topLevelWindows().isEmpty());
}

// Testing get/set functions
void tst_QGuiAction::getSetCheck()
{
    QGuiAction obj1(nullptr);
    // QActionGroup * QAction::actionGroup()
    // void QAction::setActionGroup(QActionGroup *)
    auto var1 = new QGuiActionGroup(nullptr);
    obj1.setActionGroup(var1);
    QCOMPARE(var1, obj1.guiActionGroup());
    obj1.setActionGroup(nullptr);
    QCOMPARE(obj1.guiActionGroup(), nullptr);
    delete var1;

    QCOMPARE(obj1.priority(), QGuiAction::NormalPriority);
    obj1.setPriority(QGuiAction::LowPriority);
    QCOMPARE(obj1.priority(), QGuiAction::LowPriority);
}

void tst_QGuiAction::setText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("iconText");
    QTest::addColumn<QString>("textFromIconText");

    //next we fill it with data
    QTest::newRow("Normal") << "Action" << "Action" << "Action";
    QTest::newRow("Ampersand") << "Search && Destroy" << "Search & Destroy" << "Search && Destroy";
    QTest::newRow("Mnemonic and ellipsis") << "O&pen File ..." << "Open File" << "Open File";
}

void tst_QGuiAction::setText()
{
    QFETCH(QString, text);

    QGuiAction action(nullptr);
    action.setText(text);

    QCOMPARE(action.text(), text);

    QFETCH(QString, iconText);
    QCOMPARE(action.iconText(), iconText);
}

void tst_QGuiAction::setIconText()
{
    QFETCH(QString, iconText);

    QGuiAction action(nullptr);
    action.setIconText(iconText);
    QCOMPARE(action.iconText(), iconText);

    QFETCH(QString, textFromIconText);
    QCOMPARE(action.text(), textFromIconText);
}

#if QT_CONFIG(shortcut)

//basic testing of standard keys
void tst_QGuiAction::setStandardKeys()
{
    QGuiAction act(nullptr);
    act.setShortcut(QKeySequence("CTRL+L"));
    QList<QKeySequence> list;
    act.setShortcuts(list);
    act.setShortcuts(QKeySequence::Copy);
    QCOMPARE(act.shortcut(), act.shortcuts().constFirst());

    QList<QKeySequence> expected;
    const QKeySequence ctrlC = QKeySequence(QStringLiteral("CTRL+C"));
    const QKeySequence ctrlInsert = QKeySequence(QStringLiteral("CTRL+INSERT"));
    switch (m_keyboardScheme) {
    case QPlatformTheme::MacKeyboardScheme:
        expected  << ctrlC;
        break;
    case QPlatformTheme::WindowsKeyboardScheme:
        expected  << ctrlC << ctrlInsert;
        break;
    default: // X11
        expected  << ctrlC << ctrlInsert << QKeySequence(QStringLiteral("F16"));
        break;
    }

    QCOMPARE(act.shortcuts(), expected);
}

void tst_QGuiAction::task200823_tooltip()
{
    const QScopedPointer<QGuiAction> action(new QGuiAction("foo", nullptr));
    QString shortcut("ctrl+o");
    action->setShortcut(shortcut);

    // we want a non-standard tooltip that shows the shortcut
    action->setToolTip(action->text() + QLatin1String(" (") + action->shortcut().toString() + QLatin1Char(')'));

    QString ref = QLatin1String("foo (") + QKeySequence(shortcut).toString() + QLatin1Char(')');
    QCOMPARE(action->toolTip(), ref);
}

#endif // QT_CONFIG(shortcut)

void tst_QGuiAction::task229128TriggeredSignalWithoutActiongroup()
{
    // test without a group
    const QScopedPointer<QGuiAction> actionWithoutGroup(new QGuiAction("Test", nullptr));
    QSignalSpy spyWithoutGroup(actionWithoutGroup.data(), QOverload<bool>::of(&QGuiAction::triggered));
    QCOMPARE(spyWithoutGroup.count(), 0);
    actionWithoutGroup->trigger();
    // signal should be emitted
    QCOMPARE(spyWithoutGroup.count(), 1);

    // it is now a checkable checked action
    actionWithoutGroup->setCheckable(true);
    actionWithoutGroup->setChecked(true);
    spyWithoutGroup.clear();
    QCOMPARE(spyWithoutGroup.count(), 0);
    actionWithoutGroup->trigger();
    // signal should be emitted
    QCOMPARE(spyWithoutGroup.count(), 1);
}

void tst_QGuiAction::setData() // QTBUG-62006
{
    QGuiAction act(nullptr);
    QSignalSpy spy(&act, &QGuiAction::changed);
    QCOMPARE(act.data(), QVariant());
    QCOMPARE(spy.count(), 0);
    act.setData(QVariant());
    QCOMPARE(spy.count(), 0);

    act.setData(-1);
    QCOMPARE(spy.count(), 1);
    act.setData(-1);
    QCOMPARE(spy.count(), 1);
}

void tst_QGuiAction::setEnabledSetVisible()
{
    QGuiAction action(nullptr);
    QSignalSpy spy(&action, &QGuiAction::enabledChanged);
    QVERIFY(action.isEnabled());
    QVERIFY(action.isVisible());
    QCOMPARE(spy.count(), 0);
    action.setVisible(false);
    QVERIFY(!action.isEnabled());
    QVERIFY(!action.isVisible());
    QCOMPARE(spy.count(), 1);
    action.setEnabled(false);
    QVERIFY(!action.isEnabled());
    QVERIFY(!action.isVisible());
    QCOMPARE(spy.count(), 1);
    action.setVisible(true);
    QVERIFY(!action.isEnabled());
    QVERIFY(action.isVisible());
    QCOMPARE(spy.count(), 1);
    action.resetEnabled();
    QVERIFY(action.isEnabled());
    QCOMPARE(spy.count(), 2);
}

QTEST_MAIN(tst_QGuiAction)
#include "tst_qguiaction.moc"
