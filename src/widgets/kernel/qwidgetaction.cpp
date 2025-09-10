// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwidgetaction.h"
#include "qwidget.h"
#include "qdebug.h"

#include <QtWidgets/private/qwidget_p.h>

#include "qwidgetaction_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QWidgetAction
    \since 4.2
    \brief The QWidgetAction class extends QAction by an interface
    for inserting custom widgets into action based containers, such
    as toolbars.

    \ingroup mainwindow-classes
    \inmodule QtWidgets

    Most actions in an application are represented as items in menus or
    buttons in toolbars. However sometimes more complex widgets are
    necessary. For example a zoom action in a word processor may be
    realized using a QComboBox in a QToolBar, presenting a range
    of different zoom levels. QToolBar provides QToolBar::insertWidget()
    as convenience function for inserting a single widget.
    However if you want to implement an action that uses custom
    widgets for visualization in multiple containers then you have to
    subclass QWidgetAction.

    If a QWidgetAction is added for example to a QToolBar then
    QWidgetAction::createWidget() is called. Reimplementations of that
    function should create a new custom widget with the specified parent.

    If the action is removed from a container widget then
    QWidgetAction::deleteWidget() is called with the previously created custom
    widget as argument. The default implementation hides the widget and deletes
    it using QObject::deleteLater().

    If you have only one single custom widget then you can set it as default
    widget using setDefaultWidget(). That widget will then be used if the
    action is added to a QToolBar, or in general to an action container that
    supports QWidgetAction. If a QWidgetAction with only a default widget is
    added to two toolbars at the same time then the default widget is shown
    only in the first toolbar the action was added to. QWidgetAction takes
    over ownership of the default widget.

    Note that it is up to the widget to activate the action, for example by
    reimplementing mouse event handlers and calling QAction::trigger().

    \b {\macos}: If you add a widget to a menu in the application's menu
    bar on \macos, the widget will be added and it will function but with some
    limitations:
    \list 1
        \li The widget is reparented away from the QMenu to the native menu
            view. If you show the menu in some other place (e.g. as a popup menu),
            the widget will not be there.
        \li Focus/Keyboard handling of the widget is not possible.
        \li Due to Apple's design, mouse tracking on the widget currently does
            not work.
        \li Connecting the triggered() signal to a slot that opens a modal
            dialog will cause a crash in \macos 10.4 (known bug acknowledged
            by Apple), a workaround is to use a QueuedConnection instead of a
            DirectConnection.
    \endlist

    \sa QAction, QActionGroup, QWidget
*/

/*!
    Constructs an action with \a parent.
*/
QWidgetAction::QWidgetAction(QObject *parent)
    : QAction(*(new QWidgetActionPrivate), parent)
{
}

/*!
    Destroys the object and frees allocated resources.
*/
QWidgetAction::~QWidgetAction()
{
    Q_D(QWidgetAction);
    for (QWidget *w : std::as_const(d->createdWidgets))
        QObjectPrivate::disconnect(w, &QWidget::destroyed,
                                   d, &QWidgetActionPrivate::widgetDestroyed);
    QList<QWidget *> widgetsToDelete = d->createdWidgets;
    d->createdWidgets.clear();
    qDeleteAll(widgetsToDelete);
    delete d->defaultWidget;
}

/*!
    Sets \a widget to be the default widget. The ownership is
    transferred to QWidgetAction. Unless createWidget() is
    reimplemented by a subclass to return a new widget the default
    widget is used when a container widget requests a widget through
    requestWidget().
*/
void QWidgetAction::setDefaultWidget(QWidget *widget)
{
    Q_D(QWidgetAction);
    if (widget == d->defaultWidget || d->defaultWidgetInUse)
        return;
    delete d->defaultWidget;
    d->defaultWidget = widget;
    if (!widget)
        return;

    setVisible(!QWidgetPrivate::get(widget)->isExplicitlyHidden());
    d->defaultWidget->hide();
    d->defaultWidget->setParent(nullptr);
    d->defaultWidgetInUse = false;
    if (!isEnabled())
        d->defaultWidget->setEnabled(false);
}

/*!
    Returns the default widget.
*/
QWidget *QWidgetAction::defaultWidget() const
{
    Q_D(const QWidgetAction);
    return d->defaultWidget;
}

/*!
    Returns a widget that represents the action, with the given \a
    parent.

    Container widgets that support actions can call this function to
    request a widget as visual representation of the action.

    \sa releaseWidget(), createWidget(), defaultWidget()
*/
QWidget *QWidgetAction::requestWidget(QWidget *parent)
{
    Q_D(QWidgetAction);

    QWidget *w = createWidget(parent);
    if (!w) {
        if (d->defaultWidgetInUse || !d->defaultWidget)
            return nullptr;
        d->defaultWidget->setParent(parent);
        d->defaultWidgetInUse = true;
        return d->defaultWidget;
    }

    QObjectPrivate::connect(w, &QWidget::destroyed,
                            d, &QWidgetActionPrivate::widgetDestroyed);
    d->createdWidgets.append(w);
    return w;
}

/*!
    Releases the specified \a widget.

    Container widgets that support actions call this function when a widget
    action is removed.

    \sa requestWidget(), deleteWidget(), defaultWidget()
*/
void QWidgetAction::releaseWidget(QWidget *widget)
{
    Q_D(QWidgetAction);

    if (widget == d->defaultWidget) {
        d->defaultWidget->hide();
        d->defaultWidget->setParent(nullptr);
        d->defaultWidgetInUse = false;
        return;
    }

    if (!d->createdWidgets.contains(widget))
        return;

    QObjectPrivate::disconnect(widget, &QWidget::destroyed,
                               d, &QWidgetActionPrivate::widgetDestroyed);
    d->createdWidgets.removeAll(widget);
    deleteWidget(widget);
}

/*!
    \reimp
*/
bool QWidgetAction::event(QEvent *event)
{
    Q_D(QWidgetAction);
    if (event->type() == QEvent::ActionChanged) {
        if (d->defaultWidget)
            d->defaultWidget->setEnabled(isEnabled());
        for (int i = 0; i < d->createdWidgets.size(); ++i)
            d->createdWidgets.at(i)->setEnabled(isEnabled());
    }
    return QAction::event(event);
}

/*!
    \reimp
 */
bool QWidgetAction::eventFilter(QObject *obj, QEvent *event)
{
    return QAction::eventFilter(obj,event);
}

/*!
    This function is called whenever the action is added to a container widget
    that supports custom widgets. If you don't want a custom widget to be
    used as representation of the action in the specified \a parent widget then
    0 should be returned.

    \sa deleteWidget()
*/
QWidget *QWidgetAction::createWidget(QWidget *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

/*!
    This function is called whenever the action is removed from a
    container widget that displays the action using a custom \a
    widget previously created using createWidget(). The default
    implementation hides the \a widget and schedules it for deletion
    using QObject::deleteLater().

    \sa createWidget()
*/
void QWidgetAction::deleteWidget(QWidget *widget)
{
    widget->hide();
    widget->deleteLater();
}

/*!
    Returns the list of widgets that have been using createWidget() and
    are currently in use by widgets the action has been added to.
*/
QList<QWidget *> QWidgetAction::createdWidgets() const
{
    Q_D(const QWidgetAction);
    return d->createdWidgets;
}

QT_END_NAMESPACE

#include "moc_qwidgetaction.cpp"
