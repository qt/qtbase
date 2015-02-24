/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtGui>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusReply>

#include "atspi/atspi-constants.h"
#include "bus_interface.h"

#include "dbusconnection_p.h"
#include "struct_marshallers_p.h"

#define COMPARE3(v1, v2, v3) QCOMPARE(v1, v3); QCOMPARE(v2, v3);

class AccessibleTestWindow : public QWidget
{
    Q_OBJECT
public:
    AccessibleTestWindow()
    {
        new QHBoxLayout(this);
    }

    void addWidget(QWidget* widget)
    {
        layout()->addWidget(widget);
        widget->show();
        QTest::qWaitForWindowExposed(widget);
    }

    void clearChildren()
    {
        qDeleteAll(children());
        new QHBoxLayout(this);
    }
};


class tst_QAccessibilityLinux : public QObject
{
    Q_OBJECT

public:
    tst_QAccessibilityLinux() : m_window(0), root(0), rootApplication(0), mainWindow(0)
    {
        qputenv("QT_LINUX_ACCESSIBILITY_ALWAYS_ON", QByteArrayLiteral("1"));
        dbus = new DBusConnection();
    }
    ~tst_QAccessibilityLinux()
    {
        delete dbus;
    }

private slots:
    void initTestCase();

    void testLabel();
    void testLineEdit();
    void testListWidget();
    void testTreeWidget();
    void testTextEdit();
    void testSlider();
    void testFocus();

    void cleanupTestCase();

private:
    void registerDbus();
    static QString getParent(QDBusInterface *interface);
    static QStringList getChildren(QDBusInterface *interface);
    QDBusInterface *getInterface(const QString &path, const QString &interfaceName);

    AccessibleTestWindow *m_window;

    QString address;
    QDBusInterface *root; // the root object on dbus (for the app)
    QDBusInterface *rootApplication;
    QDBusInterface *mainWindow;

    DBusConnection *dbus;
};

// helper to find children of a dbus object
QStringList tst_QAccessibilityLinux::getChildren(QDBusInterface *interface)
{
    QSpiObjectReferenceArray list;
    const QList<QVariant> args = interface->call(QDBus::Block, "GetChildren").arguments();
    Q_ASSERT(args.size() == 1);
    Q_ASSERT(args.first().isValid());
    args.first().value<QDBusArgument>() >> list;

    Q_ASSERT(interface->property("ChildCount").toInt() == list.count());
    QStringList children;
    Q_FOREACH (const QSpiObjectReference &ref, list)
        children << ref.path.path();

    return children;
}

QString tst_QAccessibilityLinux::getParent(QDBusInterface *interface)
{
    if (!interface->isValid())
        return QString();

    QVariant var = interface->property("Parent");
    if (!var.canConvert<QSpiObjectReference>()) {
        qWarning() << "Invalid parent";
        return QString();
    }
    QSpiObjectReference parent = var.value<QSpiObjectReference>();
    return parent.path.path();
}

// helper to get dbus object
QDBusInterface *tst_QAccessibilityLinux::getInterface(const QString &path, const QString &interfaceName)
{
    return new QDBusInterface(address, path, interfaceName, dbus->connection(), this);
}

void tst_QAccessibilityLinux::initTestCase()
{
    // Oxygen style creates many extra items, it's simply unusable here
    qApp->setStyle("fusion");
    qApp->setApplicationName("tst_QAccessibilityLinux app");


    // trigger launching of at-spi if it isn't running already
    QDBusConnection c = QDBusConnection::sessionBus();
    OrgA11yStatusInterface *a11yStatus = new OrgA11yStatusInterface(QStringLiteral("org.a11y.Bus"), QStringLiteral("/org/a11y/bus"), c, this);
    // don't care about the result, calling any function on "org.a11y.Bus" will launch the service
    a11yStatus->isEnabled();
    for (int i = 0; i < 5; ++i) {
        if (!dbus->isEnabled())
            QTest::qWait(100);
    }

    if (!dbus->isEnabled())
        QSKIP("Could not connect to AT-SPI, make sure lib atspi2 is installed.");
    QTRY_VERIFY(dbus->isEnabled());
    QTRY_VERIFY(dbus->connection().isConnected());
    address = dbus->connection().baseService().toLatin1().data();
    QVERIFY(!address.isEmpty());

    m_window = new AccessibleTestWindow();
    m_window->show();

    QTest::qWaitForWindowExposed(m_window);
    registerDbus();
}

void tst_QAccessibilityLinux::cleanupTestCase()
{
    delete mainWindow;
    delete rootApplication;
    delete root;
    delete m_window;
}

void tst_QAccessibilityLinux::registerDbus()
{
    QVERIFY(dbus->connection().isConnected());

    root = getInterface("/org/a11y/atspi/accessible/root",
                        "org.a11y.atspi.Accessible");

    rootApplication = getInterface("/org/a11y/atspi/accessible/root",
                                   "org.a11y.atspi.Application");
    QVERIFY(root->isValid());
    QVERIFY(rootApplication->isValid());

    QStringList appChildren = getChildren(root);
    QString window = appChildren.at(0);
    mainWindow = getInterface(window, "org.a11y.atspi.Accessible");
}

#define ROOTPATH "/org/a11y/atspi/accessible"

void tst_QAccessibilityLinux::testLabel()
{
    QLabel *l = new QLabel(m_window);
    l->setText("Hello A11y");
    m_window->addWidget(l);

    // Application
    QCOMPARE(getParent(mainWindow), QLatin1String(ATSPI_DBUS_PATH_ROOT));
    QStringList children = getChildren(mainWindow);

    QDBusInterface *labelInterface = getInterface(children.at(0), "org.a11y.atspi.Accessible");
    QVERIFY(labelInterface->isValid());
    QCOMPARE(labelInterface->property("Name").toString(), QLatin1String("Hello A11y"));
    QCOMPARE(getChildren(labelInterface).count(), 0);
    QCOMPARE(labelInterface->call(QDBus::Block, "GetRoleName").arguments().first().toString(), QLatin1String("label"));
    QCOMPARE(labelInterface->call(QDBus::Block, "GetRole").arguments().first().toUInt(), 29u);
    QCOMPARE(getParent(labelInterface), mainWindow->path());

    l->setText("New text");
    QCOMPARE(labelInterface->property("Name").toString(), l->text());

    m_window->clearChildren();
    delete labelInterface;
}

void tst_QAccessibilityLinux::testLineEdit()
{
    QLineEdit *lineEdit = new QLineEdit(m_window);
    lineEdit->setText("a11y test QLineEdit");
    m_window->addWidget(lineEdit);

    QStringList children = getChildren(mainWindow);

    QDBusInterface *accessibleInterface = getInterface(children.at(0), "org.a11y.atspi.Accessible");
    QDBusInterface *editableTextInterface = getInterface(children.at(0), "org.a11y.atspi.EditableText");
    QDBusInterface *textInterface = getInterface(children.at(0), "org.a11y.atspi.Text");
    QVERIFY(accessibleInterface->isValid());
    QVERIFY(editableTextInterface->isValid());
    QVERIFY(textInterface->isValid());

    QCOMPARE(accessibleInterface->call(QDBus::Block, "GetRoleName").arguments().first().toString(), QLatin1String("text"));
    QCOMPARE(textInterface->call(QDBus::Block,"GetText", 5, -1).arguments().first().toString(), QLatin1String("test QLineEdit"));
    QString newText = "Text has changed!";
    editableTextInterface->call(QDBus::Block, "SetTextContents", newText);
    COMPARE3(lineEdit->text(), textInterface->call(QDBus::Block, "GetText", 0, -1).arguments().first().toString(), newText);
    QCOMPARE(textInterface->call(QDBus::Block, "GetText", 0, 4).arguments().first().toString(), QLatin1String("Text"));
    editableTextInterface->call(QDBus::Block, "DeleteText", 4, 8);
    COMPARE3(lineEdit->text(), "Te" + textInterface->call(QDBus::Block, "GetText", 2, 10).arguments().first().toString() + "ed!", QLatin1String("Text changed!"));
    editableTextInterface->call(QDBus::Block, "InsertText", 12, " again ", 6);
    QCOMPARE(lineEdit->text(), QLatin1String("Text changed again!"));
    COMPARE3(lineEdit->text().length(), textInterface->property("CharacterCount").toInt(), 19);

    textInterface->call(QDBus::Block, "SetCaretOffset", 4);
    COMPARE3(lineEdit->cursorPosition(), textInterface->property("CaretOffset").toInt(), 4);

    textInterface->call(QDBus::Block, "AddSelection", 1, 4);
    QList<QVariant> data = textInterface->call(QDBus::Block, "GetSelection", 0).arguments();
    COMPARE3(data.at(0).toInt(), lineEdit->selectionStart(), 1);
    QCOMPARE(data.at(1).toInt(), 4);
    QCOMPARE(lineEdit->selectedText().length(), 3);
    QCOMPARE(textInterface->call(QDBus::Block, "GetNSelections").arguments().first().toInt(), 1);
    textInterface->call(QDBus::Block, "SetSelection", 0, 0, 5);
    data = textInterface->call(QDBus::Block, "GetSelection", 0).arguments();
    COMPARE3(data.at(0).toInt(), lineEdit->selectionStart(), 0);
    COMPARE3(data.at(1).toInt(), lineEdit->selectedText().length(), 5);
    textInterface->call(QDBus::Block, "RemoveSelection", 0);
    QCOMPARE(lineEdit->selectionStart(), -1);
    QCOMPARE(textInterface->call(QDBus::Block, "GetNSelections").arguments().first().toInt(), 0);

    m_window->clearChildren();
    delete accessibleInterface;
    delete textInterface;
    delete editableTextInterface;
}

void tst_QAccessibilityLinux::testListWidget()
{
    QListWidget *lw = new QListWidget;
    lw->addItem("Hello");
    lw->addItem("Good morning");
    lw->addItem("Good bye");
    m_window->addWidget(lw);

    QStringList children = getChildren(mainWindow);
    QDBusInterface *listIface = getInterface(children.at(0), "org.a11y.atspi.Accessible");
    QCOMPARE(listIface->call(QDBus::Block, "GetRoleName").arguments().first().toString(), QLatin1String("list"));
    QStringList tableChildren = getChildren(listIface);
    QCOMPARE(tableChildren.size(), 3);

    QDBusInterface *cell1 = getInterface(tableChildren.at(0), "org.a11y.atspi.Accessible");
    QCOMPARE(cell1->call(QDBus::Block, "GetRoleName").arguments().first().toString(), QLatin1String("list item"));
    QCOMPARE(cell1->property("Name").toString(), QLatin1String("Hello"));

    QDBusInterface *cell2 = getInterface(tableChildren.at(1), "org.a11y.atspi.Accessible");
    QCOMPARE(cell2->call(QDBus::Block, "GetRoleName").arguments().first().toString(), QLatin1String("list item"));
    QCOMPARE(cell2->property("Name").toString(), QLatin1String("Good morning"));

    QDBusInterface *cell3 = getInterface(tableChildren.at(2), "org.a11y.atspi.Accessible");
    QCOMPARE(cell3->call(QDBus::Block, "GetRoleName").arguments().first().toString(), QLatin1String("list item"));
    QCOMPARE(cell3->property("Name").toString(), QLatin1String("Good bye"));

    delete cell1; delete cell2; delete cell3;
    m_window->clearChildren();
    delete listIface;
}

void tst_QAccessibilityLinux::testTreeWidget()
{
    QTreeWidget *tree = new QTreeWidget;
    tree->setColumnCount(2);
    tree->setHeaderLabels(QStringList() << "Header 1" << "Header 2");

    QTreeWidgetItem *top1 = new QTreeWidgetItem(QStringList() << "0.0" << "0.1");
    tree->addTopLevelItem(top1);

    QTreeWidgetItem *top2 = new QTreeWidgetItem(QStringList() << "1.0" << "1.1");
    tree->addTopLevelItem(top2);

    QTreeWidgetItem *child1 = new QTreeWidgetItem(QStringList() << "1.0 0.0" << "1.0 0.1");
    top2->addChild(child1);

    m_window->addWidget(tree);

    QStringList children = getChildren(mainWindow);
    QDBusInterface *treeIface = getInterface(children.at(0), "org.a11y.atspi.Accessible");
    QCOMPARE(treeIface->call(QDBus::Block, "GetRoleName").arguments().first().toString(), QLatin1String("tree"));
    QStringList tableChildren = getChildren(treeIface);

    QCOMPARE(tableChildren.size(), 6);

    QDBusInterface *cell1 = getInterface(tableChildren.at(0), "org.a11y.atspi.Accessible");
    QCOMPARE(cell1->call(QDBus::Block, "GetRoleName").arguments().first().toString(), QLatin1String("column header"));
    QCOMPARE(cell1->property("Name").toString(), QLatin1String("Header 1"));

    QDBusInterface *cell2 = getInterface(tableChildren.at(1), "org.a11y.atspi.Accessible");
    QCOMPARE(cell2->call(QDBus::Block, "GetRoleName").arguments().first().toString(), QLatin1String("column header"));
    QCOMPARE(cell2->property("Name").toString(), QLatin1String("Header 2"));

    QDBusInterface *cell3 = getInterface(tableChildren.at(2), "org.a11y.atspi.Accessible");
    QCOMPARE(cell3->property("Name").toString(), QLatin1String("0.0"));

    QDBusInterface *cell4 = getInterface(tableChildren.at(3), "org.a11y.atspi.Accessible");
    QCOMPARE(cell4->property("Name").toString(), QLatin1String("0.1"));

    tree->expandItem(top2);
    tableChildren = getChildren(treeIface);
    QCOMPARE(tableChildren.size(), 8);

    QDBusInterface *cell5 = getInterface(tableChildren.at(6), "org.a11y.atspi.Accessible");
    QCOMPARE(cell5->property("Name").toString(), QLatin1String("1.0 0.0"));

    QDBusInterface *cell6 = getInterface(tableChildren.at(7), "org.a11y.atspi.Accessible");
    QCOMPARE(cell6->property("Name").toString(), QLatin1String("1.0 0.1"));


    QDBusInterface *treeTableIface = getInterface(children.at(0), "org.a11y.atspi.Table");

    QCOMPARE(treeTableIface->call(QDBus::Block, "GetRowAtIndex", 0).arguments().first().toInt(), -1);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetRowAtIndex", 1).arguments().first().toInt(), -1);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetRowAtIndex", 2).arguments().first().toInt(), 0);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetRowAtIndex", 3).arguments().first().toInt(), 0);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetRowAtIndex", 4).arguments().first().toInt(), 1);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetRowAtIndex", 5).arguments().first().toInt(), 1);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetRowAtIndex", 6).arguments().first().toInt(), 2);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetRowAtIndex", 7).arguments().first().toInt(), 2);

    QCOMPARE(treeTableIface->call(QDBus::Block, "GetColumnAtIndex", 0).arguments().first().toInt(), 0);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetColumnAtIndex", 1).arguments().first().toInt(), 1);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetColumnAtIndex", 2).arguments().first().toInt(), 0);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetColumnAtIndex", 3).arguments().first().toInt(), 1);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetColumnAtIndex", 4).arguments().first().toInt(), 0);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetColumnAtIndex", 5).arguments().first().toInt(), 1);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetColumnAtIndex", 6).arguments().first().toInt(), 0);
    QCOMPARE(treeTableIface->call(QDBus::Block, "GetColumnAtIndex", 7).arguments().first().toInt(), 1);

    delete treeTableIface;
    delete cell1; delete cell2; delete cell3; delete cell4; delete cell5; delete cell6;
    m_window->clearChildren();
    delete treeIface;
}

void tst_QAccessibilityLinux::testTextEdit()
{
    QTextEdit *textEdit = new QTextEdit(m_window);
    textEdit->setText("<html><head></head><body>This is a <b>sample</b> text.<br />"
                      "How are you today</body></html>");
    textEdit->show();
    m_window->addWidget(textEdit);

    QStringList children = getChildren(mainWindow);
    QDBusInterface *accessibleInterface = getInterface(children.at(0), "org.a11y.atspi.Accessible");
    QDBusInterface *editableTextInterface = getInterface(children.at(0), "org.a11y.atspi.EditableText");
    QDBusInterface *textInterface = getInterface(children.at(0), "org.a11y.atspi.Text");
    QVERIFY(accessibleInterface->isValid());
    QVERIFY(editableTextInterface->isValid());
    QVERIFY(textInterface->isValid());

    QList<QVariant> callResult;

    QDBusMessage msg = textInterface->call(QDBus::Block, "GetText", 0, 5);
    callResult = msg.arguments();
    QCOMPARE(callResult.at(0).toString(), QLatin1String("This "));

    msg = textInterface->call(QDBus::Block, "GetTextAtOffset", 12, (uint) ATSPI_TEXT_BOUNDARY_WORD_START);
    callResult = msg.arguments();

    QEXPECT_FAIL("", "Word should contain space at end according to atspi.", Continue);
    QCOMPARE(callResult.at(0).toString(), QLatin1String("sample "));
    QCOMPARE(callResult.at(1).toInt(), 10);
    QEXPECT_FAIL("", "Due to missing space the count is off by one.", Continue);
    QCOMPARE(callResult.at(2).toInt(), 17);

    // Check if at least CharacterExtents and RangeExtents give a consistent result

    QDBusMessage replyRect20 = textInterface->call(QDBus::Block, "GetCharacterExtents", 20, ATSPI_COORD_TYPE_SCREEN);
    QCOMPARE(replyRect20.type(), QDBusMessage::ReplyMessage);
    QCOMPARE(replyRect20.signature(), QStringLiteral("iiii"));
    callResult = replyRect20.arguments();
    QRect r1 = QRect(callResult.at(0).toInt(), callResult.at(1).toInt(), callResult.at(2).toInt(), callResult.at(3).toInt());
    QDBusMessage replyRect21  = textInterface->call(QDBus::Block, "GetCharacterExtents", 21, ATSPI_COORD_TYPE_SCREEN);
    QCOMPARE(replyRect21.type(), QDBusMessage::ReplyMessage);
    QCOMPARE(replyRect21.signature(), QStringLiteral("iiii"));
    callResult = replyRect21.arguments();
    QRect r2 = QRect(callResult.at(0).toInt(), callResult.at(1).toInt(), callResult.at(2).toInt(), callResult.at(3).toInt());

    QDBusMessage replyRange = textInterface->call(QDBus::Block, "GetRangeExtents", 20, 21, ATSPI_COORD_TYPE_SCREEN);
    callResult = replyRange.arguments();
    QRect rectRangeExtents = QRect(callResult.at(0).toInt(), callResult.at(1).toInt(), callResult.at(2).toInt(), callResult.at(3).toInt());
    QCOMPARE(rectRangeExtents, r1|r2);

    m_window->clearChildren();
    delete textInterface;
}

void tst_QAccessibilityLinux::testSlider()
{
    QSlider *slider = new QSlider(m_window);
    slider->setMinimum(2);
    slider->setMaximum(5);
    slider->setValue(3);
    m_window->addWidget(slider);

    QStringList children = getChildren(mainWindow);

    QDBusInterface *accessibleInterface = getInterface(children.at(0), "org.a11y.atspi.Accessible");
    QDBusInterface *valueInterface = getInterface(children.at(0), "org.a11y.atspi.Value");
    QVERIFY(accessibleInterface->isValid());
    QVERIFY(valueInterface->isValid());

    QCOMPARE(valueInterface->property("CurrentValue").toInt(), 3);
    QCOMPARE(valueInterface->property("MinimumValue").toInt(), 2);
    QCOMPARE(valueInterface->property("MaximumValue").toInt(), 5);

    valueInterface->setProperty("CurrentValue", 4);
    QCOMPARE(valueInterface->property("CurrentValue").toInt(), 4);
    m_window->clearChildren();
}

quint64 getAtspiState(QDBusInterface *interface)
{
    QDBusMessage msg = interface->call(QDBus::Block, "GetState");
    const QDBusArgument arg = msg.arguments().at(0).value<QDBusArgument>();
    quint32 state1 = 0;
    quint64 state2 = 0;
    arg.beginArray();
    arg >> state1;
    arg >> state2;
    arg.endArray();

    state2 = state2 << 32;
    return state2 | state1;
}

void tst_QAccessibilityLinux::testFocus()
{
    QLineEdit *lineEdit1 = new QLineEdit(m_window);
    lineEdit1->setText("lineEdit 1");
    QLineEdit *lineEdit2 = new QLineEdit(m_window);
    lineEdit2->setText("lineEdit 2");

    m_window->addWidget(lineEdit1);
    m_window->addWidget(lineEdit2);
    lineEdit1->setFocus();

    QStringList children = getChildren(mainWindow);
    QCOMPARE(children.length(), 2);
    QDBusInterface *accessibleInterfaceLineEdit1 = getInterface(children.at(0), "org.a11y.atspi.Accessible");
    QVERIFY(accessibleInterfaceLineEdit1->isValid());
    QDBusInterface *accessibleInterfaceLineEdit2 = getInterface(children.at(1), "org.a11y.atspi.Accessible");
    QVERIFY(accessibleInterfaceLineEdit2->isValid());
    QDBusInterface *componentInterfaceLineEdit1 = getInterface(children.at(0), "org.a11y.atspi.Component");
    QVERIFY(componentInterfaceLineEdit1->isValid());
    QDBusInterface *componentInterfaceLineEdit2 = getInterface(children.at(1), "org.a11y.atspi.Component");
    QVERIFY(componentInterfaceLineEdit2->isValid());

    quint64 focusedState = quint64(1) << ATSPI_STATE_FOCUSED;
    QVERIFY(getAtspiState(accessibleInterfaceLineEdit1) & focusedState);
    QVERIFY(!(getAtspiState(accessibleInterfaceLineEdit2) & focusedState));

    QDBusMessage focusReply = componentInterfaceLineEdit2->call(QDBus::Block, "GrabFocus");
    QVERIFY(focusReply.arguments().at(0).toBool());
    QVERIFY(lineEdit2->hasFocus());
    QVERIFY(!(getAtspiState(accessibleInterfaceLineEdit1) & focusedState));
    QVERIFY(getAtspiState(accessibleInterfaceLineEdit2) & focusedState);
    m_window->clearChildren();
    delete accessibleInterfaceLineEdit1;
    delete accessibleInterfaceLineEdit2;
    delete componentInterfaceLineEdit1;
    delete componentInterfaceLineEdit2;
}

QTEST_MAIN(tst_QAccessibilityLinux)
#include "tst_qaccessibilitylinux.moc"

