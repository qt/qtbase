/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qapplication.h>
#include <qtoolbar.h>
#include <qcombobox.h>
#include <qwidgetaction.h>
#include <qlabel.h>
#include <qmenu.h>
#include <qmainwindow.h>
#include <qmenubar.h>

static inline void setFrameless(QWidget *w)
{
    Qt::WindowFlags flags = w->windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    w->setWindowFlags(flags);
}

class tst_QWidgetAction : public QObject
{
    Q_OBJECT
private slots:
    void defaultWidget();
    void visibilityUpdate();
    void customWidget();
    void keepOwnership();
    void visibility();
    void setEnabled();
    void popup();
    void releaseWidgetCrash();
};

void tst_QWidgetAction::defaultWidget()
{
    {
        QToolBar tb1;

        QPointer<QComboBox> combo = new QComboBox(&tb1);

        QWidgetAction *action = new QWidgetAction(0);
        action->setDefaultWidget(combo);

        QVERIFY(!combo->isVisible());
        QVERIFY(!combo->parent());
        QVERIFY(action->isVisible());

        delete action;
        QVERIFY(!combo);
    }
    {
        QToolBar tb1;

        QPointer<QComboBox> combo = new QComboBox(&tb1);
        combo->hide();

        QWidgetAction *action = new QWidgetAction(0);
        action->setDefaultWidget(combo);

        // explicitly hidden widgets should also set the action invisible
        QVERIFY(!action->isVisible());

        delete action;
    }
    {
        QPointer<QComboBox> combo = new QComboBox(0);
        setFrameless(combo.data());
        combo->show();

        QWidgetAction *action = new QWidgetAction(0);
        action->setDefaultWidget(combo);

        QVERIFY(action->isVisible());
        QVERIFY(!combo->isVisible());

        delete action;
    }
    {
        QToolBar tb1;
        setFrameless(&tb1);
        tb1.show();
        QToolBar tb2;
        setFrameless(&tb2);
        tb2.show();

        QPointer<QComboBox> combo = new QComboBox(0);

        QWidgetAction *action = new QWidgetAction(0);
        action->setDefaultWidget(combo);

        tb1.addAction(action);
        QVERIFY(combo->parent() == &tb1);
        qApp->processEvents();
        qApp->processEvents();
        QVERIFY(combo->isVisible());

        // not supported, not supposed to work, hence the parent() check
        tb2.addAction(action);
        QVERIFY(combo->parent() == &tb1);

        tb2.removeAction(action);
        tb1.removeAction(action);

        qApp->processEvents(); //the call to hide is delayd by the toolbar layout
        QVERIFY(!combo->isVisible());

        tb2.addAction(action);
        qApp->processEvents(); //the call to hide is delayd by the toolbar layout
        qApp->processEvents();
        QVERIFY(combo->parent() == &tb2);
        QVERIFY(combo->isVisible());

        tb1.addAction(action);
        QVERIFY(combo->parent() == &tb2);

        delete action;
        QVERIFY(!combo);
    }
    {
        QWidgetAction *a = new QWidgetAction(0);
        QVERIFY(!a->defaultWidget());

        QPointer<QComboBox> combo1 = new QComboBox;
        a->setDefaultWidget(combo1);
        QVERIFY(a->defaultWidget() == combo1);
        a->setDefaultWidget(combo1);
        QVERIFY(combo1);
        QVERIFY(a->defaultWidget() == combo1);

        QPointer<QComboBox> combo2 = new QComboBox;
        QVERIFY(combo1 != combo2);

        a->setDefaultWidget(combo2);
        QVERIFY(!combo1);
        QVERIFY(a->defaultWidget() == combo2);

        delete a;
        QVERIFY(!combo2);
    }
}

void tst_QWidgetAction::visibilityUpdate()
{
    // actually keeping the widget's state in sync with the
    // action in terms of visibility is QToolBar's responsibility.
    QToolBar tb;
    setFrameless(&tb);
    tb.show();

    QComboBox *combo = new QComboBox(0);
    QWidgetAction *action = new QWidgetAction(0);
    action->setDefaultWidget(combo);

    tb.addAction(action);
    //the call to show is delayed by the toolbar layout
    QTRY_VERIFY(combo->isVisible());
    QVERIFY(action->isVisible());

    action->setVisible(false);
    //the call to hide is delayed by the toolbar layout
    QTRY_VERIFY(!combo->isVisible());

    delete action;
    // action also deletes combo
}

class ComboAction : public QWidgetAction
{
public:
    inline ComboAction(QObject *parent) : QWidgetAction(parent) {}

    QList<QWidget *> createdWidgets() const { return QWidgetAction::createdWidgets(); }

protected:
    virtual QWidget *createWidget(QWidget *parent);
};

QWidget *ComboAction::createWidget(QWidget *parent)
{
    return new QComboBox(parent);
}

void tst_QWidgetAction::customWidget()
{
    QToolBar tb1;
    setFrameless(&tb1);
    tb1.show();
    QToolBar tb2;
    setFrameless(&tb2);
    tb2.show();

    ComboAction *action = new ComboAction(0);

    tb1.addAction(action);

    QList<QWidget *> combos = action->createdWidgets();
    QCOMPARE(combos.count(), 1);

    QPointer<QComboBox> combo1 = qobject_cast<QComboBox *>(combos.at(0));
    QVERIFY(combo1);

    tb2.addAction(action);

    combos = action->createdWidgets();
    QCOMPARE(combos.count(), 2);

    QVERIFY(combos.at(0) == combo1);
    QPointer<QComboBox> combo2 = qobject_cast<QComboBox *>(combos.at(1));
    QVERIFY(combo2);

    tb2.removeAction(action);
    QVERIFY(combo2);
    // widget is deleted using deleteLater(), so process that posted event
    QCoreApplication::sendPostedEvents(combo2, QEvent::DeferredDelete);
    QVERIFY(!combo2);

    delete action;
    QVERIFY(!combo1);
    QVERIFY(!combo2);
}

void tst_QWidgetAction::keepOwnership()
{
    QPointer<QComboBox> combo = new QComboBox;
    QWidgetAction *action = new QWidgetAction(0);
    action->setDefaultWidget(combo);

    {
        QToolBar *tb = new QToolBar;
        tb->addAction(action);
        QVERIFY(combo->parent() == tb);
        delete tb;
    }

    QVERIFY(combo);
    delete action;
    QVERIFY(!combo);
}

void tst_QWidgetAction::visibility()
{
    {
        QWidgetAction *a = new QWidgetAction(0);
        QComboBox *combo = new QComboBox;
        a->setDefaultWidget(combo);

        QToolBar *tb = new QToolBar;
        setFrameless(tb);
        tb->addAction(a);
        QVERIFY(!combo->isVisible());
        tb->show();
        QVERIFY(combo->isVisible());

        delete tb;

        delete a;
    }
    {
        QWidgetAction *a = new QWidgetAction(0);
        QComboBox *combo = new QComboBox;
        a->setDefaultWidget(combo);

        QToolBar *tb = new QToolBar;
        tb->addAction(a);
        QVERIFY(!combo->isVisible());

        QToolBar *tb2 = new QToolBar;
        setFrameless(tb2);
        tb->removeAction(a);
        tb2->addAction(a);
        QVERIFY(!combo->isVisible());
        tb2->show();
        QVERIFY(combo->isVisible());

        delete tb;
        delete tb2;

        delete a;
    }
}

void tst_QWidgetAction::setEnabled()
{
    QToolBar toolbar;
    setFrameless(&toolbar);
    QComboBox *combobox = new QComboBox;
    QAction *action = toolbar.addWidget(combobox);
    toolbar.show();

    QVERIFY(action->isEnabled());
    QVERIFY(combobox->isEnabled());

    action->setEnabled(false);
    QVERIFY(!action->isEnabled());
    QVERIFY(!combobox->isEnabled());

    action->setEnabled(true);
    QVERIFY(action->isEnabled());
    QVERIFY(combobox->isEnabled());

    combobox->setEnabled(false);
    QVERIFY(!combobox->isEnabled());

    combobox->setEnabled(true);
    QVERIFY(action->isEnabled());
    QVERIFY(combobox->isEnabled());


    QWidgetAction aw(0);
    aw.setEnabled(false);
    QVERIFY(!aw.isEnabled());

    combobox = new QComboBox;
    aw.setDefaultWidget(combobox);
    QVERIFY(!aw.isEnabled());
    QVERIFY(!combobox->isEnabled());

    // Make sure we don't change the default widget's Qt::WA_ForceDisabled attribute
    // during a normal disable/enable operation (task 207433).
    {
        QToolBar toolBar;
        QWidget widget;
        toolBar.addWidget(&widget); // creates a QWidgetAction and sets 'widget' as the default widget.
        QVERIFY(!widget.testAttribute(Qt::WA_ForceDisabled));

        toolBar.setEnabled(false);
        QVERIFY(toolBar.testAttribute(Qt::WA_ForceDisabled));
        QVERIFY(!widget.isEnabled());
        QVERIFY(!widget.testAttribute(Qt::WA_ForceDisabled));

        toolBar.setEnabled(true);
        QVERIFY(widget.isEnabled());
        QVERIFY(!widget.testAttribute(Qt::WA_ForceDisabled));
    }
}

void tst_QWidgetAction::popup()
{
    QPointer<QLabel> l = new QLabel("test");
    QWidgetAction action(0);
    action.setDefaultWidget(l);

    {
        QMenu menu;
        menu.addAction(&action);
        QTimer::singleShot(100, &menu, SLOT(close()));
        menu.exec();
    }

    QVERIFY(!l.isNull());
    delete l;
}

class CrashedAction : public QWidgetAction
{
public:
    inline CrashedAction(QObject *parent) : QWidgetAction(parent) { }

    virtual QWidget *createWidget(QWidget *parent) {
        return new QWidget(parent);
    }
};

void tst_QWidgetAction::releaseWidgetCrash()
{
    // this should not crash!
    QMainWindow *w = new QMainWindow;
    QAction *a = new CrashedAction(w);
    QMenu *menu = w->menuBar()->addMenu("Test");
    menu->addAction("foo");
    menu->addAction(a);
    menu->addAction("bar");
    delete w;
}

QTEST_MAIN(tst_QWidgetAction)
#include "tst_qwidgetaction.moc"
