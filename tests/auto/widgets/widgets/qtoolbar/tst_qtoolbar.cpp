/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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


#include <qabstractbutton.h>
#include <qaction.h>
#include <qlayout.h>
#include <qmainwindow.h>
#include <qpixmap.h>
#include <qstyle.h>
#include <qtoolbar.h>
#include <qwidgetaction.h>
#include <qtoolbutton.h>
#include <qlineedit.h>
#include <qkeysequence.h>
#include <qmenu.h>
#include <qlabel.h>
#include <private/qtoolbarextension_p.h>

QT_FORWARD_DECLARE_CLASS(QAction)

class tst_QToolBar : public QObject
{
    Q_OBJECT

public:
    tst_QToolBar();

public slots:
    void slot();
    void slot(QAction *action);

private slots:
    void isMovable();
    void allowedAreas();
    void orientation();
    void addAction();
    void addActionConnect();
    void insertAction();
    void addSeparator();
    void insertSeparator();
    void addWidget();
    void insertWidget();
    void actionGeometry();
    void toggleViewAction();
    void iconSize();
    void toolButtonStyle();
    void actionTriggered();
    void visibilityChanged();
    void actionOwnership();
    void widgetAction();
    void accel();

    void task191727_layout();
    void task197996_visibility();

    void extraCpuConsumption(); // QTBUG-54676
};


QAction *triggered = 0;

tst_QToolBar::tst_QToolBar()
{
    qRegisterMetaType<Qt::Orientation>("Qt::Orientation");
    qRegisterMetaType<Qt::ToolBarAreas>("Qt::ToolBarAreas");
    qRegisterMetaType<Qt::ToolButtonStyle>("Qt::ToolButtonStyle");
}

void tst_QToolBar::slot()
{ }

void tst_QToolBar::slot(QAction *action)
{ ::triggered = action; }

void tst_QToolBar::isMovable()
{
#define DO_TEST                                                 \
    do {                                                        \
        QVERIFY(tb.isMovable());                                 \
        tb.setMovable(false);                                   \
        QVERIFY(!tb.isMovable());                                \
        QCOMPARE(spy.count(), 1);                                \
        QCOMPARE(spy.at(0).value(0).toBool(), tb.isMovable());   \
        spy.clear();                                            \
        tb.setMovable(tb.isMovable());                          \
        QCOMPARE(spy.count(), 0);                                \
        spy.clear();                                            \
        tb.setMovable(true);                                    \
        QVERIFY(tb.isMovable());                                 \
        QCOMPARE(spy.count(), 1);                                \
        QCOMPARE(spy.at(0).value(0).toBool(), tb.isMovable());   \
        spy.clear();                                            \
        tb.setMovable(tb.isMovable());                          \
        QCOMPARE(spy.count(), 0);                                \
        spy.clear();                                            \
    } while (false)

    QMainWindow mw;
    QToolBar tb;

    QCOMPARE(tb.isMovable(), (bool)qApp->style()->styleHint(QStyle::SH_ToolBar_Movable));
    if (!tb.isMovable())
        tb.setMovable(true);

    QSignalSpy spy(&tb, SIGNAL(movableChanged(bool)));

    DO_TEST;
    mw.addToolBar(&tb);
    DO_TEST;
    mw.removeToolBar(&tb);
    DO_TEST;
}

void tst_QToolBar::allowedAreas()
{
    QToolBar tb;

    QSignalSpy spy(&tb, SIGNAL(allowedAreasChanged(Qt::ToolBarAreas)));

    // default
    QCOMPARE((int)tb.allowedAreas(), (int)Qt::AllToolBarAreas);
    QVERIFY(tb.isAreaAllowed(Qt::LeftToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::RightToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::TopToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::BottomToolBarArea));

    // a single dock window area
    tb.setAllowedAreas(Qt::LeftToolBarArea);
    QCOMPARE((int)tb.allowedAreas(), (int)Qt::LeftToolBarArea);
    QVERIFY(tb.isAreaAllowed(Qt::LeftToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::RightToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::TopToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::BottomToolBarArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::ToolBarAreas *>(spy.at(0).value(0).constData()),
            tb.allowedAreas());
    spy.clear();
    tb.setAllowedAreas(tb.allowedAreas());
    QCOMPARE(spy.count(), 0);

    tb.setAllowedAreas(Qt::RightToolBarArea);
    QCOMPARE((int)tb.allowedAreas(), (int)Qt::RightToolBarArea);
    QVERIFY(!tb.isAreaAllowed(Qt::LeftToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::RightToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::TopToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::BottomToolBarArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::ToolBarAreas *>(spy.at(0).value(0).constData()),
            tb.allowedAreas());
    spy.clear();
    tb.setAllowedAreas(tb.allowedAreas());
    QCOMPARE(spy.count(), 0);

    tb.setAllowedAreas(Qt::TopToolBarArea);
    QCOMPARE((int)tb.allowedAreas(), (int)Qt::TopToolBarArea);
    QVERIFY(!tb.isAreaAllowed(Qt::LeftToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::RightToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::TopToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::BottomToolBarArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::ToolBarAreas *>(spy.at(0).value(0).constData()),
            tb.allowedAreas());
    spy.clear();
    tb.setAllowedAreas(tb.allowedAreas());
    QCOMPARE(spy.count(), 0);

    tb.setAllowedAreas(Qt::BottomToolBarArea);
    QCOMPARE((int)tb.allowedAreas(), (int)Qt::BottomToolBarArea);
    QVERIFY(!tb.isAreaAllowed(Qt::LeftToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::RightToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::TopToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::BottomToolBarArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::ToolBarAreas *>(spy.at(0).value(0).constData()),
            tb.allowedAreas());
    spy.clear();
    tb.setAllowedAreas(tb.allowedAreas());
    QCOMPARE(spy.count(), 0);

    // multiple dock window areas
    tb.setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    QCOMPARE(tb.allowedAreas(), Qt::TopToolBarArea | Qt::BottomToolBarArea);
    QVERIFY(!tb.isAreaAllowed(Qt::LeftToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::RightToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::TopToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::BottomToolBarArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::ToolBarAreas *>(spy.at(0).value(0).constData()),
            tb.allowedAreas());
    spy.clear();
    tb.setAllowedAreas(tb.allowedAreas());
    QCOMPARE(spy.count(), 0);

    tb.setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
    QCOMPARE(tb.allowedAreas(), Qt::LeftToolBarArea | Qt::RightToolBarArea);
    QVERIFY(tb.isAreaAllowed(Qt::LeftToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::RightToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::TopToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::BottomToolBarArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::ToolBarAreas *>(spy.at(0).value(0).constData()),
            tb.allowedAreas());
    spy.clear();
    tb.setAllowedAreas(tb.allowedAreas());
    QCOMPARE(spy.count(), 0);

    tb.setAllowedAreas(Qt::TopToolBarArea | Qt::LeftToolBarArea);
    QCOMPARE(tb.allowedAreas(), Qt::TopToolBarArea | Qt::LeftToolBarArea);
    QVERIFY(tb.isAreaAllowed(Qt::LeftToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::RightToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::TopToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::BottomToolBarArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::ToolBarAreas *>(spy.at(0).value(0).constData()),
            tb.allowedAreas());
    spy.clear();
    tb.setAllowedAreas(tb.allowedAreas());
    QCOMPARE(spy.count(), 0);

    tb.setAllowedAreas(Qt::BottomToolBarArea | Qt::RightToolBarArea);
    QCOMPARE(tb.allowedAreas(), Qt::BottomToolBarArea | Qt::RightToolBarArea);
    QVERIFY(!tb.isAreaAllowed(Qt::LeftToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::RightToolBarArea));
    QVERIFY(!tb.isAreaAllowed(Qt::TopToolBarArea));
    QVERIFY(tb.isAreaAllowed(Qt::BottomToolBarArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::ToolBarAreas *>(spy.at(0).value(0).constData()),
            tb.allowedAreas());
    spy.clear();
    tb.setAllowedAreas(tb.allowedAreas());
    QCOMPARE(spy.count(), 0);
}

void tst_QToolBar::orientation()
{
    QToolBar tb;
    QCOMPARE(tb.orientation(), Qt::Horizontal);

    QSignalSpy spy(&tb, SIGNAL(orientationChanged(Qt::Orientation)));

    tb.setOrientation(Qt::Vertical);
    QCOMPARE(tb.orientation(), Qt::Vertical);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::Orientation *>(spy.at(0).value(0).constData()),
            tb.orientation());
    spy.clear();
    tb.setOrientation(tb.orientation());
    QCOMPARE(spy.count(), 0);

    tb.setOrientation(Qt::Horizontal);
    QCOMPARE(tb.orientation(), Qt::Horizontal);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::Orientation *>(spy.at(0).value(0).constData()),
            tb.orientation());
    spy.clear();
    tb.setOrientation(tb.orientation());
    QCOMPARE(spy.count(), 0);

    tb.setOrientation(Qt::Vertical);
    QCOMPARE(tb.orientation(), Qt::Vertical);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::Orientation *>(spy.at(0).value(0).constData()),
            tb.orientation());
    spy.clear();
    tb.setOrientation(tb.orientation());
    QCOMPARE(spy.count(), 0);

    tb.setOrientation(Qt::Horizontal);
    QCOMPARE(tb.orientation(), Qt::Horizontal);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::Orientation *>(spy.at(0).value(0).constData()),
            tb.orientation());
    spy.clear();
    tb.setOrientation(tb.orientation());
    QCOMPARE(spy.count(), 0);

    tb.setOrientation(Qt::Vertical);
    QCOMPARE(tb.orientation(), Qt::Vertical);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::Orientation *>(spy.at(0).value(0).constData()),
            tb.orientation());
    spy.clear();
    tb.setOrientation(tb.orientation());
    QCOMPARE(spy.count(), 0);
}

void tst_QToolBar::addAction()
{
    QToolBar tb;

    {
        QAction action(0);

        QCOMPARE(tb.actions().count(), 0);
        tb.addAction(&action);
        QCOMPARE(tb.actions().count(), 1);
        QCOMPARE(tb.actions()[0], &action);

        tb.clear();
        QCOMPARE(tb.actions().count(), 0);
    }

    {
        QString text = "text";
        QPixmap pm(32, 32);
        pm.fill(Qt::blue);
        QIcon icon = pm;

        QAction *action1 = tb.addAction(text);
        QCOMPARE(text, action1->text());

        QAction *action2 = tb.addAction(icon, text);
        QCOMPARE(icon, action2->icon());
        QCOMPARE(text, action2->text());

        QAction *action3 = tb.addAction(text, this, SLOT(slot()));
        QCOMPARE(text, action3->text());

        QAction *action4 = tb.addAction(icon, text, this, SLOT(slot()));
        QCOMPARE(icon, action4->icon());
        QCOMPARE(text, action4->text());

        QCOMPARE(tb.actions().count(), 4);
        QCOMPARE(tb.actions()[0], action1);
        QCOMPARE(tb.actions()[1], action2);
        QCOMPARE(tb.actions()[2], action3);
        QCOMPARE(tb.actions()[3], action4);

        tb.clear();
        QCOMPARE(tb.actions().count(), 0);
    }
}

static void testFunction() { }

void tst_QToolBar::addActionConnect()
{
    QToolBar tb;
    const QString text = QLatin1String("bla");
    const QIcon icon;
    tb.addAction(text, &tb, SLOT(deleteLater()));
    tb.addAction(text, &tb, &QMenu::deleteLater);
    tb.addAction(text, testFunction);
    tb.addAction(text, &tb, testFunction);
    tb.addAction(icon, text, &tb, SLOT(deleteLater()));
    tb.addAction(icon, text, &tb, &QMenu::deleteLater);
    tb.addAction(icon, text, testFunction);
    tb.addAction(icon, text, &tb, testFunction);
}

void tst_QToolBar::insertAction()
{
    QToolBar tb;
    QAction action1(0);
    QAction action2(0);
    QAction action3(0);
    QAction action4(0);

    QCOMPARE(tb.actions().count(), 0);
    tb.insertAction(0, &action1);
    tb.insertAction(&action1, &action2);
    tb.insertAction(&action2, &action3);
    tb.insertAction(&action3, &action4);
    QCOMPARE(tb.actions().count(), 4);
    QCOMPARE(tb.actions()[0], &action4);
    QCOMPARE(tb.actions()[1], &action3);
    QCOMPARE(tb.actions()[2], &action2);
    QCOMPARE(tb.actions()[3], &action1);

    tb.clear();
    QCOMPARE(tb.actions().count(), 0);
}

void tst_QToolBar::addSeparator()
{
    QToolBar tb;

    QAction action1(0);
    QAction action2(0);

    tb.addAction(&action1);
    QAction *sep = tb.addSeparator();
    tb.addAction(&action2);

    QCOMPARE(tb.actions().count(), 3);
    QCOMPARE(tb.actions()[0], &action1);
    QCOMPARE(tb.actions()[1], sep);
    QCOMPARE(tb.actions()[2], &action2);

    tb.clear();
    QCOMPARE(tb.actions().count(), 0);
}

void tst_QToolBar::insertSeparator()
{
    QToolBar tb;

    QAction action1(0);
    QAction action2(0);

    tb.addAction(&action1);
    tb.addAction(&action2);
    QAction *sep = tb.insertSeparator(&action2);

    QCOMPARE(tb.actions().count(), 3);
    QCOMPARE(tb.actions()[0], &action1);
    QCOMPARE(tb.actions()[1], sep);
    QCOMPARE(tb.actions()[2], &action2);

    tb.clear();
    QCOMPARE(tb.actions().count(), 0);
}

void tst_QToolBar::addWidget()
{
    QToolBar tb;
    QWidget w(&tb);

    QAction action1(0);
    QAction action2(0);

    tb.addAction(&action1);
    QAction *widget = tb.addWidget(&w);
    tb.addAction(&action2);

    QCOMPARE(tb.actions().count(), 3);
    QCOMPARE(tb.actions()[0], &action1);
    QCOMPARE(tb.actions()[1], widget);
    QCOMPARE(tb.actions()[2], &action2);

    // it should be possible to reuse the action returned by
    // addWidget() to place the widget somewhere else in the toolbar
    tb.removeAction(widget);
    QCOMPARE(tb.actions().count(), 2);
    QCOMPARE(tb.actions()[0], &action1);
    QCOMPARE(tb.actions()[1], &action2);

    tb.addAction(widget);
    QCOMPARE(tb.actions().count(), 3);
    QCOMPARE(tb.actions()[0], &action1);
    QCOMPARE(tb.actions()[1], &action2);
    QCOMPARE(tb.actions()[2], widget);

    tb.clear();
    QCOMPARE(tb.actions().count(), 0);
}

void tst_QToolBar::insertWidget()
{
    QToolBar tb;
    QWidget w(&tb);

    QAction action1(0);
    QAction action2(0);

    tb.addAction(&action1);
    tb.addAction(&action2);
    QAction *widget = tb.insertWidget(&action2, &w);

    QCOMPARE(tb.actions().count(), 3);
    QCOMPARE(tb.actions()[0], &action1);
    QCOMPARE(tb.actions()[1], widget);
    QCOMPARE(tb.actions()[2], &action2);

    // it should be possible to reuse the action returned by
    // addWidget() to place the widget somewhere else in the toolbar
    tb.removeAction(widget);
    QCOMPARE(tb.actions().count(), 2);
    QCOMPARE(tb.actions()[0], &action1);
    QCOMPARE(tb.actions()[1], &action2);

    tb.insertAction(&action1, widget);
    QCOMPARE(tb.actions().count(), 3);
    QCOMPARE(tb.actions()[0], widget);
    QCOMPARE(tb.actions()[1], &action1);
    QCOMPARE(tb.actions()[2], &action2);

    tb.clear();
    QCOMPARE(tb.actions().count(), 0);

    {
        QToolBar tb;
        QPointer<QWidget> widget = new QWidget;
        QAction *action = tb.addWidget(widget);
        QCOMPARE(action->parent(), &tb);

        QToolBar tb2;
        tb.removeAction(action);
        tb2.addAction(action);
        QVERIFY(widget && widget->parent() == &tb2);
        QCOMPARE(action->parent(), &tb2);
    }
}

void tst_QToolBar::actionGeometry()
{
    QToolBar tb;

    QAction action1(0);
    QAction action2(0);
    QAction action3(0);
    QAction action4(0);

    tb.addAction(&action1);
    tb.addAction(&action2);
    tb.addAction(&action3);
    tb.addAction(&action4);

    tb.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tb));

    QList<QToolBarExtension *> extensions = tb.findChildren<QToolBarExtension *>();

    QRect rect01;
    QRect rect02;
    QRect rect03;
    QRect rect04;
    QMenu *popupMenu = 0;

    if (extensions.size() != 0)
    {
        QToolBarExtension *extension = extensions.at(0);
        if (extension->isVisible()) {
            QRect rect0 = extension->geometry();
            QTest::mouseClick( extension, Qt::LeftButton, {}, rect0.center(), -1 );
            QApplication::processEvents();
            popupMenu = qobject_cast<QMenu *>(extension->menu());
            rect01 = popupMenu->actionGeometry(&action1);
            rect02 = popupMenu->actionGeometry(&action2);
            rect03 = popupMenu->actionGeometry(&action3);
            rect04 = popupMenu->actionGeometry(&action4);
        }
    }

    QRect rect1 = tb.actionGeometry(&action1);
    QRect rect2 = tb.actionGeometry(&action2);
    QRect rect3 = tb.actionGeometry(&action3);
    QRect rect4 = tb.actionGeometry(&action4);

    QVERIFY(rect1.isValid());
    QVERIFY(!rect1.isNull());
    QVERIFY(!rect1.isEmpty());

    QVERIFY(rect2.isValid());
    QVERIFY(!rect2.isNull());
    QVERIFY(!rect2.isEmpty());

    QVERIFY(rect3.isValid());
    QVERIFY(!rect3.isNull());
    QVERIFY(!rect3.isEmpty());

    QVERIFY(rect4.isValid());
    QVERIFY(!rect4.isNull());
    QVERIFY(!rect4.isEmpty());

    if (rect01.isValid())
        QCOMPARE(popupMenu->actionAt(rect01.center()), &action1);
    else
        QCOMPARE(tb.actionAt(rect1.center()), &action1);

    if (rect02.isValid())
        QCOMPARE(popupMenu->actionAt(rect02.center()), &action2);
    else
        QCOMPARE(tb.actionAt(rect2.center()), &action2);

    if (rect03.isValid())
        QCOMPARE(popupMenu->actionAt(rect03.center()), &action3);
    else
        QCOMPARE(tb.actionAt(rect3.center()), &action3);

    if (rect04.isValid())
        QCOMPARE(popupMenu->actionAt(rect04.center()), &action4);
    else
        QCOMPARE(tb.actionAt(rect4.center()), &action4);
}

void tst_QToolBar::toggleViewAction()
{
    {
        QToolBar tb;
        QAction *toggleViewAction = tb.toggleViewAction();
        QVERIFY(tb.isHidden());
        toggleViewAction->trigger();
        QVERIFY(!tb.isHidden());
        toggleViewAction->trigger();
        QVERIFY(tb.isHidden());
    }

    {
        QMainWindow mw;
        QToolBar tb(&mw);
        mw.addToolBar(&tb);
        mw.show();
        QAction *toggleViewAction = tb.toggleViewAction();
        QVERIFY(!tb.isHidden());
        toggleViewAction->trigger();
        QVERIFY(tb.isHidden());
        toggleViewAction->trigger();
        QVERIFY(!tb.isHidden());
        toggleViewAction->trigger();
        QVERIFY(tb.isHidden());
    }
}

void tst_QToolBar::iconSize()
{
    {
        QToolBar tb;

        QSignalSpy spy(&tb, SIGNAL(iconSizeChanged(QSize)));

        // the default is determined by the style
        const int metric = tb.style()->pixelMetric(QStyle::PM_ToolBarIconSize);
        const QSize defaultIconSize = QSize(metric, metric);
        const QSize smallIconSize = QSize(metric / 2, metric / 2);
        const QSize largeIconSize = QSize(metric * 2, metric * 2);

        QCOMPARE(tb.iconSize(), defaultIconSize);
        tb.setIconSize(defaultIconSize);
        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(spy.count(), 0);

        spy.clear();
        tb.setIconSize(largeIconSize);
        QCOMPARE(tb.iconSize(), largeIconSize);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toSize(), largeIconSize);

        // no-op
        spy.clear();
        tb.setIconSize(largeIconSize);
        QCOMPARE(tb.iconSize(), largeIconSize);
        QCOMPARE(spy.count(), 0);

        spy.clear();
        tb.setIconSize(defaultIconSize);
        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toSize(), defaultIconSize);

        // no-op
        spy.clear();
        tb.setIconSize(defaultIconSize);
        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(spy.count(), 0);

        spy.clear();
        tb.setIconSize(smallIconSize);
        QCOMPARE(tb.iconSize(), smallIconSize);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toSize(), smallIconSize);

        // no-op
        spy.clear();
        tb.setIconSize(smallIconSize);
        QCOMPARE(tb.iconSize(), smallIconSize);
        QCOMPARE(spy.count(), 0);

        // setting the icon size to an invalid QSize will reset the
        // iconSize property to the default
        tb.setIconSize(QSize());
        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.first().first().toSize(), defaultIconSize);
        spy.clear();
    }

    {
        QMainWindow mw;
        QToolBar tb;
        QSignalSpy mwSpy(&mw, SIGNAL(iconSizeChanged(QSize)));
        QSignalSpy tbSpy(&tb, SIGNAL(iconSizeChanged(QSize)));

        // the default is determined by the style
        const int metric = tb.style()->pixelMetric(QStyle::PM_ToolBarIconSize);
        const QSize defaultIconSize = QSize(metric, metric);
        const QSize smallIconSize = QSize(metric / 2, metric / 2);
        const QSize largeIconSize = QSize(metric * 2, metric * 2);

        mw.setIconSize(smallIconSize);

        // explicitly set it to the default
        tb.setIconSize(defaultIconSize);
        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(tbSpy.count(), 0);

        mw.addToolBar(&tb);

        // tb icon size should not change since it has been explicitly set
        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(tbSpy.count(), 0);

        mw.setIconSize(largeIconSize);

        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(tbSpy.count(), 0);

        mw.setIconSize(defaultIconSize);

        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(tbSpy.count(), 0);

        mw.setIconSize(smallIconSize);

        QCOMPARE(tb.iconSize(), defaultIconSize);
        QCOMPARE(tbSpy.count(), 0);

        // resetting to the default should cause the toolbar to take
        // on the mainwindow's icon size
        tb.setIconSize(QSize());
        QCOMPARE(tb.iconSize(), smallIconSize);
        QCOMPARE(tbSpy.size(), 1);
        QCOMPARE(tbSpy.first().first().toSize(), smallIconSize);
        tbSpy.clear();
    }
}

void tst_QToolBar::toolButtonStyle()
{
    {
        QToolBar tb;

        QSignalSpy spy(&tb, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)));

        // no-op
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        tb.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(spy.count(), 0);

        tb.setToolButtonStyle(Qt::ToolButtonTextOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextOnly);
        QCOMPARE(spy.count(), 1);
        spy.clear();

        // no-op
        tb.setToolButtonStyle(Qt::ToolButtonTextOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextOnly);
        QCOMPARE(spy.count(), 0);

        tb.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(spy.count(), 1);
        spy.clear();

        // no-op
        tb.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(spy.count(), 0);

        tb.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(spy.count(), 1);
        spy.clear();

        // no-op
        tb.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
        QCOMPARE(spy.count(), 0);

        tb.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(spy.count(), 1);
        spy.clear();

        // no-op
        tb.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(spy.count(), 0);

        tb.setToolButtonStyle(Qt::ToolButtonFollowStyle);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonFollowStyle);
        QCOMPARE(spy.count(), 1);
    }

    {
        QMainWindow mw;
        QToolBar tb;
        QSignalSpy mwSpy(&mw, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)));
        QSignalSpy tbSpy(&tb, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)));

        mw.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        // explicitly set the tb to the default
        tb.setToolButtonStyle(Qt::ToolButtonIconOnly);
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(tbSpy.count(), 0);

        mw.addToolBar(&tb);

        // tb icon size should not change since it has been explicitly set
        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(tbSpy.count(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonIconOnly);

        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(tbSpy.count(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonTextOnly);

        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(tbSpy.count(), 0);

        mw.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

        QCOMPARE(tb.toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(tbSpy.count(), 0);

        // note: there is no way to clear the explicitly set tool
        // button style... once you explicitly set it, the toolbar
        // will never follow the mainwindow again
    }
}

void tst_QToolBar::actionTriggered()
{
    QToolBar tb;
    connect(&tb, SIGNAL(actionTriggered(QAction*)), SLOT(slot(QAction*)));

    QAction action1(0);
    QAction action2(0);
    QAction action3(0);
    QAction action4(0);

    tb.addAction(&action1);
    tb.addAction(&action2);
    tb.addAction(&action3);
    tb.addAction(&action4);

    tb.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tb));

    QList<QToolBarExtension *> extensions = tb.findChildren<QToolBarExtension *>();

    QRect rect01;
    QRect rect02;
    QRect rect03;
    QRect rect04;
    QMenu *popupMenu = 0;

    if (extensions.size() != 0)
    {
        QToolBarExtension *extension = extensions.at(0);
        if (extension->isVisible()) {
            QRect rect0 = extension->geometry();
            QTest::mouseClick( extension, Qt::LeftButton, {}, rect0.center(), -1 );
            QApplication::processEvents();
            popupMenu = qobject_cast<QMenu *>(extension->menu());
            rect01 = popupMenu->actionGeometry(&action1);
            rect02 = popupMenu->actionGeometry(&action2);
            rect03 = popupMenu->actionGeometry(&action3);
            rect04 = popupMenu->actionGeometry(&action4);
        }
    }

    QRect rect1 = tb.actionGeometry(&action1);
    QRect rect2 = tb.actionGeometry(&action2);
    QRect rect3 = tb.actionGeometry(&action3);
    QRect rect4 = tb.actionGeometry(&action4);

    QAbstractButton *button1 = 0;
    QAbstractButton *button2 = 0;
    QAbstractButton *button3 = 0;
    QAbstractButton *button4 = 0;

    if (!rect01.isValid()) {
        button1 = qobject_cast<QAbstractButton *>(tb.childAt(rect1.center()));
        QVERIFY(button1 != 0);
    }
    if (!rect02.isValid()) {
        button2 = qobject_cast<QAbstractButton *>(tb.childAt(rect2.center()));
        QVERIFY(button2 != 0);
    }
    if (!rect03.isValid()) {
        button3 = qobject_cast<QAbstractButton *>(tb.childAt(rect3.center()));
        QVERIFY(button3 != 0);
    }
    if (!rect04.isValid()) {
        button4 = qobject_cast<QAbstractButton *>(tb.childAt(rect4.center()));
        QVERIFY(button4 != 0);
    }

    ::triggered = 0;
    if (!rect01.isValid())
        QTest::mouseClick(button1, Qt::LeftButton);
    else
        QTest::mouseClick(popupMenu, Qt::LeftButton, {}, rect01.center(), -1 );
    QCOMPARE(::triggered, &action1);

    ::triggered = 0;
    if (!rect02.isValid())
        QTest::mouseClick(button2, Qt::LeftButton);
    else
        QTest::mouseClick(popupMenu, Qt::LeftButton, {}, rect02.center(), -1 );
    QCOMPARE(::triggered, &action2);

    ::triggered = 0;
    if (!rect03.isValid())
        QTest::mouseClick(button3, Qt::LeftButton);
    else
        QTest::mouseClick(popupMenu, Qt::LeftButton, {}, rect03.center(), -1 );
    QCOMPARE(::triggered, &action3);

    ::triggered = 0;
    if (!rect04.isValid())
        QTest::mouseClick(button4, Qt::LeftButton);
    else
        QTest::mouseClick(popupMenu, Qt::LeftButton, {}, rect04.center(), -1 );
    QCOMPARE(::triggered, &action4);
}

void tst_QToolBar::visibilityChanged()
{
    QMainWindow mw;
    QToolBar tb;
    QSignalSpy spy(&tb, SIGNAL(visibilityChanged(bool)));

    mw.addToolBar(&tb);
    mw.show();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    tb.hide();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    spy.clear();

    tb.hide();
    QCOMPARE(spy.count(), 0);

    tb.show();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    tb.show();
    QCOMPARE(spy.count(), 0);
}

void tst_QToolBar::actionOwnership()
{
    {
        QToolBar *tb1 = new QToolBar;
        QToolBar *tb2 = new QToolBar;

        QPointer<QAction> action = tb1->addAction("test");
        QCOMPARE(action->parent(), tb1);

        tb2->addAction(action);
        QCOMPARE(action->parent(), tb1);

        delete tb1;
        QVERIFY(!action);
        delete tb2;
    }
    {
        QToolBar *tb1 = new QToolBar;
        QToolBar *tb2 = new QToolBar;

        QPointer<QAction> action = tb1->addAction("test");
        QCOMPARE(action->parent(), tb1);

        tb1->removeAction(action);
        QCOMPARE(action->parent(), tb1);

        tb2->addAction(action);
        QCOMPARE(action->parent(), tb1);

        delete tb1;
        QVERIFY(!action);
        delete tb2;
    }
}

void tst_QToolBar::widgetAction()
{
    // ensure that a QWidgetAction without widget behaves like a normal action
    QToolBar tb;
    QWidgetAction *a = new QWidgetAction(0);
    a->setIconText("Blah");

    tb.addAction(a);
    QWidget *w = tb.widgetForAction(a);
    QVERIFY(w);
    QToolButton *button = qobject_cast<QToolButton *>(w);
    QVERIFY(button);
    QCOMPARE(a->iconText(), button->text());

    delete a;
}

#ifdef Q_OS_MAC
QT_BEGIN_NAMESPACE
extern void qt_set_sequence_auto_mnemonic(bool b);
QT_END_NAMESPACE
#endif

void tst_QToolBar::accel()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

#ifdef Q_OS_MAC
    qt_set_sequence_auto_mnemonic(true);
#endif
    QMainWindow mw;
    QToolBar *toolBar = mw.addToolBar("test");
    QAction *action = toolBar->addAction("&test");
    action->setIconText(action->text()); // we really want that mnemonic in the button!

    QSignalSpy spy(action, SIGNAL(triggered(bool)));

    mw.show();
    QApplication::setActiveWindow(&mw);
    QVERIFY(QTest::qWaitForWindowActive(&mw));

    QTest::keyClick(&mw, Qt::Key_T, Qt::AltModifier);

    QTRY_COMPARE(spy.count(), 1);
#ifdef Q_OS_MAC
    qt_set_sequence_auto_mnemonic(false);
#endif
}

void tst_QToolBar::task191727_layout()
{
    QMainWindow mw;
    QToolBar *toolBar = mw.addToolBar("test");
    toolBar->addAction("one");
    QAction *action = toolBar->addAction("two");

    QLineEdit *lineedit = new QLineEdit;
    lineedit->setMaximumWidth(50);
    toolBar->addWidget(lineedit);

    mw.resize(400, 400);
    mw.show();

    QWidget *actionwidget = toolBar->widgetForAction(action);
    QVERIFY(qAbs(lineedit->pos().x() - (actionwidget->geometry().right() + 1 + toolBar->layout()->spacing())) < 2);
}

void tst_QToolBar::task197996_visibility()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow mw;
    QToolBar *toolBar = new QToolBar(&mw);

    mw.addToolBar(toolBar);
    toolBar->addAction(new QAction("Foo", &mw));
    QAction *pAction = new QAction("Test", &mw);
    toolBar->addAction(pAction);

    pAction->setVisible(false);
    toolBar->setVisible(false);

    toolBar->setVisible(true);
    pAction->setVisible(true);

    mw.show();
    QVERIFY(QTest::qWaitForWindowActive(&mw));

    QVERIFY(toolBar->widgetForAction(pAction)->isVisible());

    toolBar->setVisible(false);
    pAction->setVisible(false);

    QVERIFY(!toolBar->widgetForAction(pAction)->isVisible());

    toolBar->setVisible(true);
    pAction->setVisible(true);

    QTRY_VERIFY(toolBar->widgetForAction(pAction)->isVisible());
}

class ShowHideEventCounter : public QObject
{
public:
    using QObject::QObject;

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (qobject_cast<QLineEdit*>(watched) && !event->spontaneous()) {
            if (event->type() == QEvent::Show)
                ++m_showEventsCount;

            if (event->type() == QEvent::Hide)
                ++m_hideEventsCount;
        }

        return QObject::eventFilter(watched, event);
    }

    uint showEventsCount() const { return m_showEventsCount; }
    uint hideEventsCount() const { return m_hideEventsCount; }

private:
    uint m_showEventsCount = 0;
    uint m_hideEventsCount = 0;
};

void tst_QToolBar::extraCpuConsumption()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QMainWindow mainWindow;

    auto tb = new QToolBar(&mainWindow);
    tb->setMovable(false);

    auto extensions = tb->findChildren<QToolBarExtension *>();
    QVERIFY(!extensions.isEmpty());

    auto extensionButton = extensions.at(0);
    QVERIFY(extensionButton);

    tb->addWidget(new QLabel("Lorem ipsum dolor sit amet"));

    auto le = new QLineEdit;
    le->setClearButtonEnabled(true);
    le->setText("Lorem ipsum");
    tb->addWidget(le);

    mainWindow.addToolBar(tb);
    mainWindow.show();
    QVERIFY(QTest::qWaitForWindowActive(&mainWindow));

    auto eventCounter = new ShowHideEventCounter(&mainWindow);
    le->installEventFilter(eventCounter);

    auto defaultSize = mainWindow.size();

    // Line edit should be hidden now and extension button should be displayed
    for (double p = 0.7; extensionButton->isHidden() || qFuzzyCompare(p, 0.); p -= 0.01) {
        mainWindow.resize(int(defaultSize.width() * p), defaultSize.height());
    }
    QVERIFY(!extensionButton->isHidden());

    // Line edit should be visible, but smaller
    for (double p = 0.75; !extensionButton->isHidden() || qFuzzyCompare(p, 1.); p += 0.01) {
        mainWindow.resize(int(defaultSize.width() * p), defaultSize.height());
    }
    QVERIFY(extensionButton->isHidden());

    // Dispatch all pending events
    qApp->sendPostedEvents();
    qApp->processEvents();

    QCOMPARE(eventCounter->showEventsCount(), eventCounter->hideEventsCount());
    QCOMPARE(eventCounter->showEventsCount(), uint(1));
    QCOMPARE(eventCounter->hideEventsCount(), uint(1));
}

QTEST_MAIN(tst_QToolBar)
#include "tst_qtoolbar.moc"
