/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaccessiblewidgets.h"
#include "qaccessiblemenu.h"
#include "simplewidgets.h"
#include "rangecontrols.h"
#include "complexwidgets.h"
#include "itemviews.h"

#include <qaccessibleplugin.h>
#include <qplugin.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qtreeview.h>
#include <qvariant.h>
#include <qaccessible.h>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE


class AccessibleFactory : public QAccessiblePlugin
{
public:
    AccessibleFactory();

    QStringList keys() const;
    QAccessibleInterface *create(const QString &classname, QObject *object);
};

AccessibleFactory::AccessibleFactory()
{
}

QStringList AccessibleFactory::keys() const
{
    QStringList list;
#ifndef QT_NO_LINEEDIT
    list << QLatin1String("QLineEdit");
#endif
#ifndef QT_NO_COMBOBOX
    list << QLatin1String("QComboBox");
#endif
#ifndef QT_NO_SPINBOX
    list << QLatin1String("QAbstractSpinBox");
    list << QLatin1String("QSpinBox");
    list << QLatin1String("QDoubleSpinBox");
#endif
#ifndef QT_NO_SCROLLBAR
    list << QLatin1String("QScrollBar");
#endif
#ifndef QT_NO_SLIDER
    list << QLatin1String("QSlider");
#endif
    list << QLatin1String("QAbstractSlider");
#ifndef QT_NO_TOOLBUTTON
    list << QLatin1String("QToolButton");
#endif
    list << QLatin1String("QCheckBox");
    list << QLatin1String("QRadioButton");
    list << QLatin1String("QPushButton");
    list << QLatin1String("QAbstractButton");
    list << QLatin1String("QDialog");
    list << QLatin1String("QMessageBox");
    list << QLatin1String("QMainWindow");
    list << QLatin1String("QLabel");
    list << QLatin1String("QLCDNumber");
    list << QLatin1String("QGroupBox");
    list << QLatin1String("QStatusBar");
    list << QLatin1String("QProgressBar");
    list << QLatin1String("QMenuBar");
    list << QLatin1String("Q3PopupMenu");
    list << QLatin1String("QMenu");
    list << QLatin1String("QHeaderView");
    list << QLatin1String("QTabBar");
    list << QLatin1String("QToolBar");
    list << QLatin1String("QWorkspaceChild");
    list << QLatin1String("QSizeGrip");
    list << QLatin1String("QAbstractItemView");
    list << QLatin1String("QWidget");
#ifndef QT_NO_SPLITTER
    list << QLatin1String("QSplitter");
    list << QLatin1String("QSplitterHandle");
#endif
#ifndef QT_NO_TEXTEDIT
    list << QLatin1String("QTextEdit");
#endif
    list << QLatin1String("QTipLabel");
    list << QLatin1String("QFrame");
    list << QLatin1String("QStackedWidget");
    list << QLatin1String("QToolBox");
    list << QLatin1String("QMdiArea");
    list << QLatin1String("QMdiSubWindow");
    list << QLatin1String("QWorkspace");
    list << QLatin1String("QDialogButtonBox");
#ifndef QT_NO_DIAL
    list << QLatin1String("QDial");
#endif
#ifndef QT_NO_RUBBERBAND
    list << QLatin1String("QRubberBand");
#endif
#ifndef QT_NO_TEXTBROWSER
    list << QLatin1String("QTextBrowser");
#endif
#ifndef QT_NO_SCROLLAREA
    list << QLatin1String("QAbstractScrollArea");
    list << QLatin1String("QScrollArea");
#endif
#ifndef QT_NO_CALENDARWIDGET
    list << QLatin1String("QCalendarWidget");
#endif

#ifndef QT_NO_DOCKWIDGET
    list << QLatin1String("QDockWidget");
#endif
    return list;
}

QAccessibleInterface *AccessibleFactory::create(const QString &classname, QObject *object)
{
    QAccessibleInterface *iface = 0;
    if (!object || !object->isWidgetType())
        return iface;
    QWidget *widget = static_cast<QWidget*>(object);

    if (false) {
#ifndef QT_NO_LINEEDIT
    } else if (classname == QLatin1String("QLineEdit")) {
        iface = new QAccessibleLineEdit(widget);
#endif
#ifndef QT_NO_COMBOBOX
    } else if (classname == QLatin1String("QComboBox")) {
        iface = new QAccessibleComboBox(widget);
#endif
#ifndef QT_NO_SPINBOX
    } else if (classname == QLatin1String("QAbstractSpinBox")) {
        iface = new QAccessibleAbstractSpinBox(widget);
    } else if (classname == QLatin1String("QSpinBox")) {
        iface = new QAccessibleSpinBox(widget);
    } else if (classname == QLatin1String("QDoubleSpinBox")) {
        iface = new QAccessibleDoubleSpinBox(widget);
#endif
#ifndef QT_NO_SCROLLBAR
    } else if (classname == QLatin1String("QScrollBar")) {
        iface = new QAccessibleScrollBar(widget);
#endif
    } else if (classname == QLatin1String("QAbstractSlider")) {
        iface = new QAccessibleAbstractSlider(widget);
#ifndef QT_NO_SLIDER
    } else if (classname == QLatin1String("QSlider")) {
        iface = new QAccessibleSlider(widget);
#endif
#ifndef QT_NO_TOOLBUTTON
    } else if (classname == QLatin1String("QToolButton")) {
        Role role = NoRole;
#ifndef QT_NO_MENU
        QToolButton *tb = qobject_cast<QToolButton*>(widget);
        if (!tb->menu())
            role = tb->isCheckable() ? CheckBox : PushButton;
        else if (!tb->popupMode() != QToolButton::DelayedPopup)
            role = ButtonDropDown;
        else
#endif
            role = ButtonMenu;
        iface = new QAccessibleToolButton(widget, role);
#endif // QT_NO_TOOLBUTTON
    } else if (classname == QLatin1String("QCheckBox")) {
        iface = new QAccessibleButton(widget, CheckBox);
    } else if (classname == QLatin1String("QRadioButton")) {
        iface = new QAccessibleButton(widget, RadioButton);
    } else if (classname == QLatin1String("QPushButton")) {
        Role role = NoRole;
        QPushButton *pb = qobject_cast<QPushButton*>(widget);
#ifndef QT_NO_MENU
        if (pb->menu())
            role = ButtonMenu;
        else
#endif
        if (pb->isCheckable())
            role = CheckBox;
        else
            role = PushButton;
        iface = new QAccessibleButton(widget, role);
    } else if (classname == QLatin1String("QAbstractButton")) {
        iface = new QAccessibleButton(widget, PushButton);
    } else if (classname == QLatin1String("QDialog")) {
        iface = new QAccessibleWidgetEx(widget, Dialog);
    } else if (classname == QLatin1String("QMessageBox")) {
        iface = new QAccessibleWidgetEx(widget, AlertMessage);
#ifndef QT_NO_MAINWINDOW
    } else if (classname == QLatin1String("QMainWindow")) {
        iface = new QAccessibleMainWindow(widget);
#endif
    } else if (classname == QLatin1String("QLabel") || classname == QLatin1String("QLCDNumber")) {
        iface = new QAccessibleDisplay(widget);
    } else if (classname == QLatin1String("QGroupBox")) {
        iface = new QAccessibleDisplay(widget, Grouping);
    } else if (classname == QLatin1String("QStatusBar")) {
        iface = new QAccessibleWidgetEx(widget, StatusBar);
#ifndef QT_NO_PROGRESSBAR
    } else if (classname == QLatin1String("QProgressBar")) {
        iface = new QAccessibleProgressBar(widget);
#endif
    } else if (classname == QLatin1String("QToolBar")) {
        iface = new QAccessibleWidgetEx(widget, ToolBar, widget->windowTitle());
#ifndef QT_NO_MENUBAR
    } else if (classname == QLatin1String("QMenuBar")) {
        iface = new QAccessibleMenuBar(widget);
#endif
#ifndef QT_NO_MENU
    } else if (classname == QLatin1String("QMenu")) {
        iface = new QAccessibleMenu(widget);
    } else if (classname == QLatin1String("Q3PopupMenu")) {
        iface = new QAccessibleMenu(widget);
#endif
#ifndef QT_NO_ITEMVIEWS
#ifdef Q_WS_X11
    } else if (classname == QLatin1String("QAbstractItemView")) {
        if (qobject_cast<const QTreeView*>(widget)) {
            iface = new QAccessibleTree(widget);
        } else {
            iface = new QAccessibleTable2(widget);
        }
    } else if (classname == QLatin1String("QWidget")
               && widget->objectName() == QLatin1String("qt_scrollarea_viewport")
               && qobject_cast<QAbstractItemView*>(widget->parentWidget())) {
        if (qobject_cast<const QTreeView*>(widget->parentWidget())) {
            iface = new QAccessibleTree(widget->parentWidget());
        } else {
            iface = new QAccessibleTable2(widget->parentWidget());
        }
#else
    } else if (classname == QLatin1String("QHeaderView")) {
        iface = new QAccessibleHeader(widget);
    } else if (classname == QLatin1String("QAbstractItemView")) {
        iface = new QAccessibleItemView(widget);
    } else if (classname == QLatin1String("QWidget")
               && widget->objectName() == QLatin1String("qt_scrollarea_viewport")
               && qobject_cast<QAbstractItemView*>(widget->parentWidget())) {
        iface = new QAccessibleItemView(widget);
#endif // Q_WS_X11
#endif // QT_NO_ITEMVIEWS
#ifndef QT_NO_TABBAR
    } else if (classname == QLatin1String("QTabBar")) {
        iface = new QAccessibleTabBar(widget);
#endif
    } else if (classname == QLatin1String("QWorkspaceChild")) {
        iface = new QAccessibleWidgetEx(widget, Window);
    } else if (classname == QLatin1String("QSizeGrip")) {
        iface = new QAccessibleWidgetEx(widget, Grip);
#ifndef QT_NO_SPLITTER
    } else if (classname == QLatin1String("QSplitter")) {
        iface = new QAccessibleWidgetEx(widget, Splitter);
    } else if (classname == QLatin1String("QSplitterHandle")) {
        iface = new QAccessibleWidgetEx(widget, Grip);
#endif
#ifndef QT_NO_TEXTEDIT
    } else if (classname == QLatin1String("QTextEdit")) {
        iface = new QAccessibleTextEdit(widget);
#endif
    } else if (classname == QLatin1String("QTipLabel")) {
        iface = new QAccessibleDisplay(widget, ToolTip);
    } else if (classname == QLatin1String("QFrame")) {
        iface = new QAccessibleWidget(widget, Border);
#ifndef QT_NO_STACKEDWIDGET
    } else if (classname == QLatin1String("QStackedWidget")) {
        iface = new QAccessibleStackedWidget(widget);
#endif
#ifndef QT_NO_TOOLBOX
    } else if (classname == QLatin1String("QToolBox")) {
        iface = new QAccessibleToolBox(widget);
#endif
#ifndef QT_NO_MDIAREA
    } else if (classname == QLatin1String("QMdiArea")) {
        iface = new QAccessibleMdiArea(widget);
    } else if (classname == QLatin1String("QMdiSubWindow")) {
        iface = new QAccessibleMdiSubWindow(widget);
#endif
#ifndef QT_NO_WORKSPACE
    } else if (classname == QLatin1String("QWorkspace")) {
        iface = new QAccessibleWorkspace(widget);
#endif
    } else if (classname == QLatin1String("QDialogButtonBox")) {
        iface = new QAccessibleDialogButtonBox(widget);
#ifndef QT_NO_DIAL
    } else if (classname == QLatin1String("QDial")) {
        iface = new QAccessibleDial(widget);
#endif
#ifndef QT_NO_RUBBERBAND
    } else if (classname == QLatin1String("QRubberBand")) {
        iface = new QAccessibleWidgetEx(widget, QAccessible::Border);
#endif
#ifndef QT_NO_TEXTBROWSER
    } else if (classname == QLatin1String("QTextBrowser")) {
        iface = new QAccessibleTextBrowser(widget);
#endif
#ifndef QT_NO_SCROLLAREA
    } else if (classname == QLatin1String("QAbstractScrollArea")) {
        iface = new QAccessibleAbstractScrollArea(widget);
    } else if (classname == QLatin1String("QScrollArea")) {
        iface = new QAccessibleScrollArea(widget);
#endif
#ifndef QT_NO_CALENDARWIDGET
    } else if (classname == QLatin1String("QCalendarWidget")) {
        iface = new QAccessibleCalendarWidget(widget);
#endif
#ifndef QT_NO_DOCKWIDGET
    } else if (classname == QLatin1String("QDockWidget")) {
        iface = new QAccessibleDockWidget(widget);
#endif
    }

    return iface;
}

Q_EXPORT_STATIC_PLUGIN(AccessibleFactory)
Q_EXPORT_PLUGIN2(qtaccessiblewidgets, AccessibleFactory)

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
