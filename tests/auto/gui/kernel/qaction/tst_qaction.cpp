// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QSignalSpy>
#include <QFont>

#include <qguiapplication.h>
#include <qevent.h>
#include <qaction.h>
#include <qactiongroup.h>
#include <qpa/qplatformtheme.h>

#include <private/qguiapplication_p.h>

class tst_QAction : public QObject
{
    Q_OBJECT

public:
    tst_QAction();

private slots:
    void cleanup();
    void getSetCheck();
    void setText_data();
    void setText();
    void setIconText_data() { setText_data(); }
    void setIconText();
    void setToolTip_data();
    void setToolTip();
#if QT_CONFIG(shortcut)
    void setStandardKeys();
    void task200823_tooltip();
#endif
    void task229128TriggeredSignalWithoutActiongroup();
    void setData();
    void setEnabledSetVisible();
    void setCheckabledSetChecked();

private:
    const int m_keyboardScheme;
};

tst_QAction::tst_QAction()
    : m_keyboardScheme(QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::KeyboardScheme).toInt())
{
}

void tst_QAction::cleanup()
{
    QVERIFY(QGuiApplication::topLevelWindows().isEmpty());
}

// Testing get/set functions
void tst_QAction::getSetCheck()
{
    QAction obj1(nullptr);
    auto var1 = new QActionGroup(nullptr);
    obj1.setActionGroup(var1);
    QCOMPARE(obj1.actionGroup(), var1);
    obj1.setActionGroup(nullptr);
    QCOMPARE(obj1.actionGroup(), nullptr);
    delete var1;

    QCOMPARE(obj1.isCheckable(), false);
    QCOMPARE(obj1.isChecked(), false);

    obj1.setCheckable(true);
    QCOMPARE(obj1.isCheckable(), true);
    obj1.setChecked(true);
    QCOMPARE(obj1.isChecked(), true);

    obj1.setCheckable(false);
    QCOMPARE(obj1.isCheckable(), false);
    QCOMPARE(obj1.isChecked(), false);

    QCOMPARE(obj1.isEnabled(), true);
    obj1.setEnabled(false);
    QCOMPARE(obj1.isEnabled(), false);

    QCOMPARE(obj1.icon(), QIcon());
    QIcon themeIcon = QIcon::fromTheme("messagebox_info");
    if (!themeIcon.isNull()) {
        obj1.setIcon(themeIcon);
        QCOMPARE(obj1.icon(), themeIcon);
    }

    QString statusTip("StatusTip");
    obj1.setStatusTip(statusTip);
    QCOMPARE(obj1.statusTip(), statusTip);

    QString whatsThis("WhatsThis");
    obj1.setWhatsThis(whatsThis);
    QCOMPARE(obj1.whatsThis(), whatsThis);

    QFont font;
    font.setBold(true);
    obj1.setFont(font);
    QCOMPARE(obj1.font(), font);

#if QT_CONFIG(shortcut)
    QCOMPARE(obj1.shortcut(), QKeySequence());
    QKeySequence quit(Qt::CTRL | Qt::Key_Q);
    obj1.setShortcut(quit);
    QCOMPARE(obj1.shortcut(), quit);

    QCOMPARE(obj1.shortcutContext(), Qt::WindowShortcut);
    obj1.setShortcutContext(Qt::ApplicationShortcut);
    QCOMPARE(obj1.shortcutContext(), Qt::ApplicationShortcut);

    QCOMPARE(obj1.autoRepeat(), true);
    obj1.setAutoRepeat(false);
    QCOMPARE(obj1.autoRepeat(), false);
#endif

    QCOMPARE(obj1.isVisible(), true);
    obj1.setVisible(false);
    QCOMPARE(obj1.isChecked(), false);

    QCOMPARE(obj1.menuRole(), QAction::TextHeuristicRole);
    obj1.setMenuRole(QAction::PreferencesRole);
    QCOMPARE(obj1.menuRole(), QAction::PreferencesRole);

    // default value for those depends on application attributes and/or style
    obj1.setIconVisibleInMenu(true);
    QCOMPARE(obj1.isIconVisibleInMenu(), true);
    obj1.setIconVisibleInMenu(false);
    QCOMPARE(obj1.isIconVisibleInMenu(), false);

    obj1.setShortcutVisibleInContextMenu(true);
    QCOMPARE(obj1.isShortcutVisibleInContextMenu(), true);
    obj1.setShortcutVisibleInContextMenu(false);
    QCOMPARE(obj1.isShortcutVisibleInContextMenu(), false);

    QCOMPARE(obj1.priority(), QAction::NormalPriority);
    obj1.setPriority(QAction::LowPriority);
    QCOMPARE(obj1.priority(), QAction::LowPriority);
}

void tst_QAction::setText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("iconText");
    QTest::addColumn<QString>("textFromIconText");

    //next we fill it with data
    QTest::newRow("Normal") << "Action" << "Action" << "Action";
    QTest::newRow("Ampersand") << "Search && Destroy" << "Search & Destroy" << "Search && Destroy";
    QTest::newRow("Mnemonic and ellipsis") << "O&pen File ..." << "Open File" << "Open File";
}

void tst_QAction::setText()
{
    QFETCH(QString, text);

    QAction action(nullptr);
    action.setText(text);

    QCOMPARE(action.text(), text);

    QFETCH(QString, iconText);
    QCOMPARE(action.iconText(), iconText);
}

void tst_QAction::setIconText()
{
    QFETCH(QString, iconText);

    QAction action(nullptr);
    action.setIconText(iconText);
    QCOMPARE(action.iconText(), iconText);

    QFETCH(QString, textFromIconText);
    QCOMPARE(action.text(), textFromIconText);
}

void tst_QAction::setToolTip_data()
{
    QTest::addColumn<QByteArrayList>("properties");
    QTest::addColumn<QStringList>("values");
    QTest::addColumn<QStringList>("expectedToolTips");

    QTest::newRow("Tooltip")
        << QByteArrayList{"toolTip", "toolTip", "toolTip"}
        << QStringList{"ToolTip", "Tool&Tip", "Tool && Tip"}
        << QStringList{"ToolTip", "Tool&Tip", "Tool && Tip"};
    QTest::newRow("Only text")
        << QByteArrayList{"text", "text", "text", "text", "toolTip", "toolTip"}
        << QStringList {"Text", "Te&xt2", "Find && Replace", "O&pen File...", "ToolTip", QString()}
        << QStringList{"Text", "Text2", "Find & Replace", "Open File", "ToolTip", "Open File"};
    QTest::newRow("Only iconText")
        << QByteArrayList{"iconText", "iconText", "iconText", "toolTip", "toolTip"}
        << QStringList{"Icon Text", "Find && Replace", "Icon&Text", "ToolTip", QString()}
        << QStringList{"Icon Text", "Find & Replace", "IconText", "ToolTip", "IconText"};
    QTest::newRow("Text and iconText")
        << QByteArrayList{"iconText", "text", "iconText", "text", "iconText"}
        << QStringList{"Icon", "Text", "Icon2", QString(), "Icon3"}
        << QStringList{"Icon", "Text", "Text", "Icon2", "Icon3"};
    QTest::newRow("All and nothing")
        << QByteArrayList{"text",  "iconText", "toolTip", "text",    "iconText", "toolTip", "text",   "text",     "iconText", "iconText", "text"}
        << QStringList   {"Te&xt", "I&&con",   "ToolTip", "Text",    "Icon",     QString(), "Te&&xt2", QString(), "I&&con2",  QString(), "Text"}
        << QStringList   {"Text",  "Text",     "ToolTip", "ToolTip", "ToolTip",  "Text",    "Te&xt2", "Icon",     "I&con2",   QString(), "Text"};
}

void tst_QAction::setToolTip()
{
    QFETCH(QByteArrayList, properties);
    QFETCH(QStringList, values);
    QFETCH(QStringList, expectedToolTips);

    QCOMPARE(properties.size(), values.size());
    QCOMPARE(properties.size(), expectedToolTips.size());

    QAction action(nullptr);
    for (int i = 0; i < properties.size(); ++i) {
        const auto property = properties.at(i);
        const auto value = values.at(i);
        const auto expectedToolTip = expectedToolTips.at(i);
        action.setProperty(property, value);
        QCOMPARE(action.toolTip(), expectedToolTip);
    }
}

#if QT_CONFIG(shortcut)

//basic testing of standard keys
void tst_QAction::setStandardKeys()
{
    QAction act(nullptr);
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

void tst_QAction::task200823_tooltip()
{
    const QScopedPointer<QAction> action(new QAction("foo", nullptr));
    QString shortcut("ctrl+o");
    action->setShortcut(shortcut);

    // we want a non-standard tooltip that shows the shortcut
    action->setToolTip(action->text() + QLatin1String(" (") + action->shortcut().toString() + QLatin1Char(')'));

    QString ref = QLatin1String("foo (") + QKeySequence(shortcut).toString() + QLatin1Char(')');
    QCOMPARE(action->toolTip(), ref);
}

#endif // QT_CONFIG(shortcut)

void tst_QAction::task229128TriggeredSignalWithoutActiongroup()
{
    // test without a group
    const QScopedPointer<QAction> actionWithoutGroup(new QAction("Test", nullptr));
    QSignalSpy spyWithoutGroup(actionWithoutGroup.data(), QOverload<bool>::of(&QAction::triggered));
    QCOMPARE(spyWithoutGroup.size(), 0);
    actionWithoutGroup->trigger();
    // signal should be emitted
    QCOMPARE(spyWithoutGroup.size(), 1);

    // it is now a checkable checked action
    actionWithoutGroup->setCheckable(true);
    actionWithoutGroup->setChecked(true);
    spyWithoutGroup.clear();
    QCOMPARE(spyWithoutGroup.size(), 0);
    actionWithoutGroup->trigger();
    // signal should be emitted
    QCOMPARE(spyWithoutGroup.size(), 1);
}

void tst_QAction::setData() // QTBUG-62006
{
    QAction act(nullptr);
    QSignalSpy spy(&act, &QAction::changed);
    QCOMPARE(act.data(), QVariant());
    QCOMPARE(spy.size(), 0);
    act.setData(QVariant());
    QCOMPARE(spy.size(), 0);

    act.setData(-1);
    QCOMPARE(spy.size(), 1);
    act.setData(-1);
    QCOMPARE(spy.size(), 1);
}

void tst_QAction::setEnabledSetVisible()
{
    QAction action(nullptr);
    QSignalSpy spy(&action, &QAction::enabledChanged);
    QVERIFY(action.isEnabled());
    QVERIFY(action.isVisible());
    QCOMPARE(spy.size(), 0);
    action.setVisible(false);
    QVERIFY(!action.isEnabled());
    QVERIFY(!action.isVisible());
    QCOMPARE(spy.size(), 1);
    action.setEnabled(false);
    QVERIFY(!action.isEnabled());
    QVERIFY(!action.isVisible());
    QCOMPARE(spy.size(), 1);
    action.setVisible(true);
    QVERIFY(!action.isEnabled());
    QVERIFY(action.isVisible());
    QCOMPARE(spy.size(), 1);
    action.resetEnabled();
    QVERIFY(action.isEnabled());
    QCOMPARE(spy.size(), 2);
}

void tst_QAction::setCheckabledSetChecked()
{
    QAction action(nullptr);
    QSignalSpy changedSpy(&action, &QAction::changed);
    QSignalSpy checkedSpy(&action, &QAction::toggled);
    QSignalSpy checkableSpy(&action, &QAction::checkableChanged);
    QVERIFY(!action.isCheckable());
    QVERIFY(!action.isChecked());
    QCOMPARE(changedSpy.size(), 0);
    QCOMPARE(checkedSpy.size(), 0);
    QCOMPARE(checkableSpy.size(), 0);

    action.setCheckable(true);
    QVERIFY(action.isCheckable());
    QVERIFY(!action.isChecked());
    QCOMPARE(changedSpy.size(), 1);
    QCOMPARE(checkedSpy.size(), 0);
    QCOMPARE(checkableSpy.size(), 1);

    action.setChecked(true);
    QVERIFY(action.isCheckable());
    QVERIFY(action.isChecked());
    QCOMPARE(changedSpy.size(), 2);
    QCOMPARE(checkedSpy.size(), 1);
    QCOMPARE(checkableSpy.size(), 1);

    action.setCheckable(false);
    QVERIFY(!action.isCheckable());
    QVERIFY(!action.isChecked());
    QCOMPARE(changedSpy.size(), 3);
    QCOMPARE(checkedSpy.size(), 2);
    QCOMPARE(checkableSpy.size(), 2);

    action.setCheckable(true);
    QVERIFY(action.isCheckable());
    QVERIFY(action.isChecked());
    QCOMPARE(changedSpy.size(), 4);
    QCOMPARE(checkedSpy.size(), 3);
    QCOMPARE(checkableSpy.size(), 3);
}

QTEST_MAIN(tst_QAction)
#include "tst_qaction.moc"
