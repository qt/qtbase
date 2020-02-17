/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qguiaction.h"
#include "qguiactiongroup.h"

#include "qguiaction_p.h"
#include "qguiapplication.h"
#include "qevent.h"
#include "qlist.h"
#include "qstylehints.h"
#if QT_CONFIG(shortcut)
#  include <private/qshortcutmap_p.h>
#endif
#include <private/qguiapplication_p.h>
#include <private/qdebug_p.h>

#define QAPP_CHECK(functionName) \
    if (Q_UNLIKELY(!QCoreApplication::instance())) { \
        qWarning("QGuiAction: Initialize Q(Gui)Application before calling '" functionName "'."); \
        return; \
    }

QT_BEGIN_NAMESPACE

/*
  internal: guesses a descriptive text from a text suited for a menu entry
 */
static QString qt_strippedText(QString s)
{
    s.remove(QLatin1String("..."));
    for (int i = 0; i < s.size(); ++i) {
        if (s.at(i) == QLatin1Char('&'))
            s.remove(i, 1);
    }
    return s.trimmed();
}

QGuiActionPrivate::QGuiActionPrivate() :
#if QT_CONFIG(shortcut)
    autorepeat(1),
#endif
    enabled(1), forceDisabled(0), visible(1), forceInvisible(0), checkable(0),
    checked(0), separator(0), fontSet(false),
    iconVisibleInMenu(-1), shortcutVisibleInContextMenu(-1)
{
}

#if QT_CONFIG(shortcut)
static bool dummy(QObject *, Qt::ShortcutContext) { return false; } // only for GUI testing.

QShortcutMap::ContextMatcher QGuiActionPrivate::contextMatcher() const
{
    return dummy;
}
#endif // QT_CONFIG(shortcut)

QGuiActionPrivate::~QGuiActionPrivate() = default;

void QGuiActionPrivate::sendDataChanged()
{
    Q_Q(QGuiAction);
    QActionEvent e(QEvent::ActionChanged, q);
    QCoreApplication::sendEvent(q, &e);

    emit q->changed();
}

#if QT_CONFIG(shortcut)
void QGuiActionPrivate::redoGrab(QShortcutMap &map)
{
    Q_Q(QGuiAction);
    if (shortcutId)
        map.removeShortcut(shortcutId, q);
    if (shortcut.isEmpty())
        return;
    shortcutId = map.addShortcut(q, shortcut, shortcutContext, contextMatcher());
    if (!enabled)
        map.setShortcutEnabled(false, shortcutId, q);
    if (!autorepeat)
        map.setShortcutAutoRepeat(false, shortcutId, q);
}

void QGuiActionPrivate::redoGrabAlternate(QShortcutMap &map)
{
    Q_Q(QGuiAction);
    for(int i = 0; i < alternateShortcutIds.count(); ++i) {
        if (const int id = alternateShortcutIds.at(i))
            map.removeShortcut(id, q);
    }
    alternateShortcutIds.clear();
    if (alternateShortcuts.isEmpty())
        return;
    for(int i = 0; i < alternateShortcuts.count(); ++i) {
        const QKeySequence& alternate = alternateShortcuts.at(i);
        if (!alternate.isEmpty())
            alternateShortcutIds.append(map.addShortcut(q, alternate, shortcutContext, contextMatcher()));
        else
            alternateShortcutIds.append(0);
    }
    if (!enabled) {
        for(int i = 0; i < alternateShortcutIds.count(); ++i) {
            const int id = alternateShortcutIds.at(i);
            map.setShortcutEnabled(false, id, q);
        }
    }
    if (!autorepeat) {
        for(int i = 0; i < alternateShortcutIds.count(); ++i) {
            const int id = alternateShortcutIds.at(i);
            map.setShortcutAutoRepeat(false, id, q);
        }
    }
}

void QGuiActionPrivate::setShortcutEnabled(bool enable, QShortcutMap &map)
{
    Q_Q(QGuiAction);
    if (shortcutId)
        map.setShortcutEnabled(enable, shortcutId, q);
    for(int i = 0; i < alternateShortcutIds.count(); ++i) {
        if (const int id = alternateShortcutIds.at(i))
            map.setShortcutEnabled(enable, id, q);
    }
}
#endif // QT_NO_SHORTCUT


/*!
    \class QGuiAction
    \brief QGuiAction is the base class for actions, an abstract user interface
    action that can be inserted into widgets.
    \since 6.0

    \inmodule QtGui

    In applications many common commands can be invoked via menus,
    toolbar buttons, and keyboard shortcuts. Since the user expects
    each command to be performed in the same way, regardless of the
    user interface used, it is useful to represent each command as
    an \e action.

    Actions can be added to menus and toolbars, and will
    automatically keep them in sync. For example, in a word processor,
    if the user presses a Bold toolbar button, the Bold menu item
    will automatically be checked.

    Actions can be created as independent objects, but they may
    also be created during the construction of menus; the QMenu class
    contains convenience functions for creating actions suitable for
    use as menu items.

    A QGuiAction may contain an icon, menu text, a shortcut, status text,
    "What's This?" text, and a tooltip. Most of these can be set in
    the constructor. They can also be set independently with
    setIcon(), setText(), setIconText(), setShortcut(),
    setStatusTip(), setWhatsThis(), and setToolTip(). For menu items,
    it is possible to set an individual font with setFont().

    We recommend that actions are created as children of the window
    they are used in. In most cases actions will be children of
    the application's main window.

    \sa QMenu, QToolBar, {Application Example}
*/

/*!
    \fn void QGuiAction::trigger()

    This is a convenience slot that calls activate(Trigger).
*/

/*!
    \fn void QGuiAction::hover()

    This is a convenience slot that calls activate(Hover).
*/

/*!
    \enum QGuiAction::MenuRole

    This enum describes how an action should be moved into the application menu on \macos.

    \value NoRole This action should not be put into the application menu
    \value TextHeuristicRole This action should be put in the application menu based on the action's text
           as described in the QMenuBar documentation.
    \value ApplicationSpecificRole This action should be put in the application menu with an application specific role
    \value AboutQtRole This action handles the "About Qt" menu item.
    \value AboutRole This action should be placed where the "About" menu item is in the application menu. The text of
           the menu item will be set to "About <application name>". The application name is fetched from the
           \c{Info.plist} file in the application's bundle (See \l{Qt for macOS - Deployment}).
    \value PreferencesRole This action should be placed where the  "Preferences..." menu item is in the application menu.
    \value QuitRole This action should be placed where the Quit menu item is in the application menu.

    Setting this value only has effect on items that are in the immediate menus
    of the menubar, not the submenus of those menus. For example, if you have
    File menu in your menubar and the File menu has a submenu, setting the
    MenuRole for the actions in that submenu have no effect. They will never be moved.
*/

/*!
    Constructs an action with \a parent. If \a parent is an action
    group the action will be automatically inserted into the group.

    \note The \a parent argument is optional since Qt 5.7.
*/
QGuiAction::QGuiAction(QObject *parent)
    : QGuiAction(*new QGuiActionPrivate, parent)
{
}

/*!
    Constructs an action with some \a text and \a parent. If \a
    parent is an action group the action will be automatically
    inserted into the group.

    The action uses a stripped version of \a text (e.g. "\&Menu
    Option..." becomes "Menu Option") as descriptive text for
    tool buttons. You can override this by setting a specific
    description with setText(). The same text will be used for
    tooltips unless you specify a different text using
    setToolTip().

*/
QGuiAction::QGuiAction(const QString &text, QObject *parent)
    : QGuiAction(parent)
{
    Q_D(QGuiAction);
    d->text = text;
}

/*!
    Constructs an action with an \a icon and some \a text and \a
    parent. If \a parent is an action group the action will be
    automatically inserted into the group.

    The action uses a stripped version of \a text (e.g. "\&Menu
    Option..." becomes "Menu Option") as descriptive text for
    tool buttons. You can override this by setting a specific
    description with setText(). The same text will be used for
    tooltips unless you specify a different text using
    setToolTip().
*/
QGuiAction::QGuiAction(const QIcon &icon, const QString &text, QObject *parent)
    : QGuiAction(text, parent)
{
    Q_D(QGuiAction);
    d->icon = icon;
}

/*!
    \internal
*/
QGuiAction::QGuiAction(QGuiActionPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QGuiAction);
    d->group = qobject_cast<QGuiActionGroup *>(parent);
    if (d->group)
        d->group->addAction(this);
}

#if QT_CONFIG(shortcut)
/*!
    \property QGuiAction::shortcut
    \brief the action's primary shortcut key

    Valid keycodes for this property can be found in \l Qt::Key and
    \l Qt::Modifier. There is no default shortcut key.
*/
void QGuiAction::setShortcut(const QKeySequence &shortcut)
{
    QAPP_CHECK("setShortcut");

    Q_D(QGuiAction);
    if (d->shortcut == shortcut)
        return;

    d->shortcut = shortcut;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
    d->sendDataChanged();
}

/*!
    Sets \a shortcuts as the list of shortcuts that trigger the
    action. The first element of the list is the primary shortcut.

    \sa shortcut
*/
void QGuiAction::setShortcuts(const QList<QKeySequence> &shortcuts)
{
    Q_D(QGuiAction);

    QList <QKeySequence> listCopy = shortcuts;

    QKeySequence primary;
    if (!listCopy.isEmpty())
        primary = listCopy.takeFirst();

    if (d->shortcut == primary && d->alternateShortcuts == listCopy)
        return;

    QAPP_CHECK("setShortcuts");

    d->shortcut = primary;
    d->alternateShortcuts = listCopy;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
    d->redoGrabAlternate(QGuiApplicationPrivate::instance()->shortcutMap);
    d->sendDataChanged();
}

/*!
    Sets a platform dependent list of shortcuts based on the \a key.
    The result of calling this function will depend on the currently running platform.
    Note that more than one shortcut can assigned by this action.
    If only the primary shortcut is required, use setShortcut instead.

    \sa QKeySequence::keyBindings()
*/
void QGuiAction::setShortcuts(QKeySequence::StandardKey key)
{
    QList <QKeySequence> list = QKeySequence::keyBindings(key);
    setShortcuts(list);
}

/*!
    Returns the primary shortcut.

    \sa setShortcuts()
*/
QKeySequence QGuiAction::shortcut() const
{
    Q_D(const QGuiAction);
    return d->shortcut;
}

/*!
    Returns the list of shortcuts, with the primary shortcut as
    the first element of the list.

    \sa setShortcuts()
*/
QList<QKeySequence> QGuiAction::shortcuts() const
{
    Q_D(const QGuiAction);
    QList <QKeySequence> shortcuts;
    if (!d->shortcut.isEmpty())
        shortcuts << d->shortcut;
    if (!d->alternateShortcuts.isEmpty())
        shortcuts << d->alternateShortcuts;
    return shortcuts;
}

/*!
    \property QGuiAction::shortcutContext
    \brief the context for the action's shortcut

    Valid values for this property can be found in \l Qt::ShortcutContext.
    The default value is Qt::WindowShortcut.
*/
void QGuiAction::setShortcutContext(Qt::ShortcutContext context)
{
    Q_D(QGuiAction);
    if (d->shortcutContext == context)
        return;
    QAPP_CHECK("setShortcutContext");
    d->shortcutContext = context;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
    d->redoGrabAlternate(QGuiApplicationPrivate::instance()->shortcutMap);
    d->sendDataChanged();
}

Qt::ShortcutContext QGuiAction::shortcutContext() const
{
    Q_D(const QGuiAction);
    return d->shortcutContext;
}

/*!
    \property QGuiAction::autoRepeat
    \brief whether the action can auto repeat

    If true, the action will auto repeat when the keyboard shortcut
    combination is held down, provided that keyboard auto repeat is
    enabled on the system.
    The default value is true.
*/
void QGuiAction::setAutoRepeat(bool on)
{
    Q_D(QGuiAction);
    if (d->autorepeat == on)
        return;
    QAPP_CHECK("setAutoRepeat");
    d->autorepeat = on;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
    d->redoGrabAlternate(QGuiApplicationPrivate::instance()->shortcutMap);
    d->sendDataChanged();
}

bool QGuiAction::autoRepeat() const
{
    Q_D(const QGuiAction);
    return d->autorepeat;
}
#endif // QT_CONFIG(shortcut)

/*!
    \property QGuiAction::font
    \brief the action's font

    The font property is used to render the text set on the
    QGuiAction. The font will can be considered a hint as it will not be
    consulted in all cases based upon application and style.

    By default, this property contains the application's default font.

    \sa setText()
*/
void QGuiAction::setFont(const QFont &font)
{
    Q_D(QGuiAction);
    if (d->font == font)
        return;

    d->fontSet = true;
    d->font = font;
    d->sendDataChanged();
}

QFont QGuiAction::font() const
{
    Q_D(const QGuiAction);
    return d->font;
}


/*!
    Destroys the object and frees allocated resources.
*/
QGuiAction::~QGuiAction()
{
    Q_D(QGuiAction);
    if (d->group)
        d->group->removeAction(this);
#if QT_CONFIG(shortcut)
    if (d->shortcutId && qApp) {
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(d->shortcutId, this);
        for (int id : qAsConst(d->alternateShortcutIds))
            QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(id, this);
    }
#endif
}

/*!
  Sets this action group to \a group. The action will be automatically
  added to the group's list of actions.

  Actions within the group will be mutually exclusive.

  \sa QGuiActionGroup, guiActionGroup()
*/
void QGuiAction::setActionGroup(QGuiActionGroup *group)
{
    Q_D(QGuiAction);
    if(group == d->group)
        return;

    if(d->group)
        d->group->removeAction(this);
    d->group = group;
    if(group)
        group->addAction(this);
    d->sendDataChanged();
}

/*!
  Returns the action group for this action. If no action group manages
  this action, then \nullptr will be returned.

  \sa QGuiActionGroup, setActionGroup()
*/
QGuiActionGroup *QGuiAction::guiActionGroup() const
{
    Q_D(const QGuiAction);
    return d->group;
}


/*!
    \property QGuiAction::icon
    \brief the action's icon

    In toolbars, the icon is used as the tool button icon; in menus,
    it is displayed to the left of the menu text. There is no default
    icon.

    If a null icon (QIcon::isNull()) is passed into this function,
    the icon of the action is cleared.
*/
void QGuiAction::setIcon(const QIcon &icon)
{
    Q_D(QGuiAction);
    d->icon = icon;
    d->sendDataChanged();
}

QIcon QGuiAction::icon() const
{
    Q_D(const QGuiAction);
    return d->icon;
}

/*!
  If \a b is true then this action will be considered a separator.

  How a separator is represented depends on the widget it is inserted
  into. Under most circumstances the text, submenu, and icon will be
  ignored for separator actions.

  \sa isSeparator()
*/
void QGuiAction::setSeparator(bool b)
{
    Q_D(QGuiAction);
    if (d->separator == b)
        return;

    d->separator = b;
    d->sendDataChanged();
}

/*!
  Returns \c true if this action is a separator action; otherwise it
  returns \c false.

  \sa setSeparator()
*/
bool QGuiAction::isSeparator() const
{
    Q_D(const QGuiAction);
    return d->separator;
}

/*!
    \property QGuiAction::text
    \brief the action's descriptive text

    If the action is added to a menu, the menu option will consist of
    the icon (if there is one), the text, and the shortcut (if there
    is one). If the text is not explicitly set in the constructor, or
    by using setText(), the action's description icon text will be
    used as text. There is no default text.

    \sa iconText
*/
void QGuiAction::setText(const QString &text)
{
    Q_D(QGuiAction);
    if (d->text == text)
        return;

    d->text = text;
    d->sendDataChanged();
}

QString QGuiAction::text() const
{
    Q_D(const QGuiAction);
    QString s = d->text;
    if(s.isEmpty()) {
        s = d->iconText;
        s.replace(QLatin1Char('&'), QLatin1String("&&"));
    }
    return s;
}

/*!
    \property QGuiAction::iconText
    \brief the action's descriptive icon text

    If QToolBar::toolButtonStyle is set to a value that permits text to
    be displayed, the text defined held in this property appears as a
    label in the relevant tool button.

    It also serves as the default text in menus and tooltips if the action
    has not been defined with setText() or setToolTip(), and will
    also be used in toolbar buttons if no icon has been defined using setIcon().

    If the icon text is not explicitly set, the action's normal text will be
    used for the icon text.

    By default, this property contains an empty string.

    \sa setToolTip(), setStatusTip()
*/
void QGuiAction::setIconText(const QString &text)
{
    Q_D(QGuiAction);
    if (d->iconText == text)
        return;

    d->iconText = text;
    d->sendDataChanged();
}

QString QGuiAction::iconText() const
{
    Q_D(const QGuiAction);
    if (d->iconText.isEmpty())
        return qt_strippedText(d->text);
    return d->iconText;
}

/*!
    \property QGuiAction::toolTip
    \brief the action's tooltip

    This text is used for the tooltip. If no tooltip is specified,
    the action's text is used.

    By default, this property contains the action's text.

    \sa setStatusTip(), setShortcut()
*/
void QGuiAction::setToolTip(const QString &tooltip)
{
    Q_D(QGuiAction);
    if (d->tooltip == tooltip)
        return;

    d->tooltip = tooltip;
    d->sendDataChanged();
}

QString QGuiAction::toolTip() const
{
    Q_D(const QGuiAction);
    if (d->tooltip.isEmpty()) {
        if (!d->text.isEmpty())
            return qt_strippedText(d->text);
        return qt_strippedText(d->iconText);
    }
    return d->tooltip;
}

/*!
    \property QGuiAction::statusTip
    \brief the action's status tip

    The status tip is displayed on all status bars provided by the
    action's top-level parent widget.

    By default, this property contains an empty string.

    \sa setToolTip(), showStatusText()
*/
void QGuiAction::setStatusTip(const QString &statustip)
{
    Q_D(QGuiAction);
    if (d->statustip == statustip)
        return;

    d->statustip = statustip;
    d->sendDataChanged();
}

QString QGuiAction::statusTip() const
{
    Q_D(const QGuiAction);
    return d->statustip;
}

/*!
    \property QGuiAction::whatsThis
    \brief the action's "What's This?" help text

    The "What's This?" text is used to provide a brief description of
    the action. The text may contain rich text. There is no default
    "What's This?" text.

    \sa QWhatsThis
*/
void QGuiAction::setWhatsThis(const QString &whatsthis)
{
    Q_D(QGuiAction);
    if (d->whatsthis == whatsthis)
        return;

    d->whatsthis = whatsthis;
    d->sendDataChanged();
}

QString QGuiAction::whatsThis() const
{
    Q_D(const QGuiAction);
    return d->whatsthis;
}

/*!
    \enum QGuiAction::Priority

    This enum defines priorities for actions in user interface.

    \value LowPriority The action should not be prioritized in
    the user interface.

    \value NormalPriority

    \value HighPriority The action should be prioritized in
    the user interface.

    \sa priority
*/


/*!
    \property QGuiAction::priority

    \brief the actions's priority in the user interface.

    This property can be set to indicate how the action should be prioritized
    in the user interface.

    For instance, when toolbars have the Qt::ToolButtonTextBesideIcon
    mode set, then actions with LowPriority will not show the text
    labels.
*/
void QGuiAction::setPriority(Priority priority)
{
    Q_D(QGuiAction);
    if (d->priority == priority)
        return;

    d->priority = priority;
    d->sendDataChanged();
}

QGuiAction::Priority QGuiAction::priority() const
{
    Q_D(const QGuiAction);
    return d->priority;
}

/*!
    \property QGuiAction::checkable
    \brief whether the action is a checkable action

    A checkable action is one which has an on/off state. For example,
    in a word processor, a Bold toolbar button may be either on or
    off. An action which is not a toggle action is a command action;
    a command action is simply executed, e.g. file save.
    By default, this property is \c false.

    In some situations, the state of one toggle action should depend
    on the state of others. For example, "Left Align", "Center" and
    "Right Align" toggle actions are mutually exclusive. To achieve
    exclusive toggling, add the relevant toggle actions to a
    QGuiActionGroup with the QGuiActionGroup::exclusive property set to
    true.

    \sa setChecked()
*/
void QGuiAction::setCheckable(bool b)
{
    Q_D(QGuiAction);
    if (d->checkable == b)
        return;

    d->checkable = b;
    d->checked = false;
    d->sendDataChanged();
}

bool QGuiAction::isCheckable() const
{
    Q_D(const QGuiAction);
    return d->checkable;
}

/*!
    \fn void QGuiAction::toggle()

    This is a convenience function for the \l checked property.
    Connect to it to change the checked state to its opposite state.
*/
void QGuiAction::toggle()
{
    Q_D(QGuiAction);
    setChecked(!d->checked);
}

/*!
    \property QGuiAction::checked
    \brief whether the action is checked.

    Only checkable actions can be checked.  By default, this is false
    (the action is unchecked).

    \note The notifier signal for this property is toggled(). As toggling
    a QGuiAction changes its state, it will also emit a changed() signal.

    \sa checkable, toggled()
*/
void QGuiAction::setChecked(bool b)
{
    Q_D(QGuiAction);
    if (!d->checkable || d->checked == b)
        return;

    QPointer<QGuiAction> guard(this);
    d->checked = b;
    d->sendDataChanged();
    if (guard)
        emit toggled(b);
}

bool QGuiAction::isChecked() const
{
    Q_D(const QGuiAction);
    return d->checked;
}

/*!
    \fn void QGuiAction::setDisabled(bool b)

    This is a convenience function for the \l enabled property, that
    is useful for signals--slots connections. If \a b is true the
    action is disabled; otherwise it is enabled.
*/

/*!
    \property QGuiAction::enabled
    \brief whether the action is enabled

    Disabled actions cannot be chosen by the user. They do not
    disappear from menus or toolbars, but they are displayed in a way
    which indicates that they are unavailable. For example, they might
    be displayed using only shades of gray.

    \uicontrol{What's This?} help on disabled actions is still available, provided
    that the QGuiAction::whatsThis property is set.

    An action will be disabled when all widgets to which it is added
    (with QWidget::addAction()) are disabled or not visible. When an
    action is disabled, it is not possible to trigger it through its
    shortcut.

    By default, this property is \c true (actions are enabled).

    \sa text
*/
void QGuiAction::setEnabled(bool b)
{
    Q_D(QGuiAction);
    if (b == d->enabled && b != d->forceDisabled)
        return;
    d->forceDisabled = !b;
    if (b && (!d->visible || (d->group && !d->group->isEnabled())))
        return;
    QAPP_CHECK("setEnabled");
    d->enabled = b;
#if QT_CONFIG(shortcut)
    d->setShortcutEnabled(b, QGuiApplicationPrivate::instance()->shortcutMap);
#endif
    d->sendDataChanged();
}

bool QGuiAction::isEnabled() const
{
    Q_D(const QGuiAction);
    return d->enabled;
}

/*!
    \property QGuiAction::visible
    \brief whether the action can be seen (e.g. in menus and toolbars)

    If \e visible is true the action can be seen (e.g. in menus and
    toolbars) and chosen by the user; if \e visible is false the
    action cannot be seen or chosen by the user.

    Actions which are not visible are \e not grayed out; they do not
    appear at all.

    By default, this property is \c true (actions are visible).
*/
void QGuiAction::setVisible(bool b)
{
    Q_D(QGuiAction);
    if (b == d->visible && b != d->forceInvisible)
        return;
    QAPP_CHECK("setVisible");
    d->forceInvisible = !b;
    d->visible = b;
    d->enabled = b && !d->forceDisabled && (!d->group || d->group->isEnabled()) ;
#if QT_CONFIG(shortcut)
    d->setShortcutEnabled(d->enabled, QGuiApplicationPrivate::instance()->shortcutMap);
#endif
    d->sendDataChanged();
}


bool QGuiAction::isVisible() const
{
    Q_D(const QGuiAction);
    return d->visible;
}

/*!
  \reimp
*/
bool QGuiAction::event(QEvent *e)
{
#if QT_CONFIG(shortcut)
    if (e->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        Q_ASSERT_X(se->key() == d_func()->shortcut || d_func()->alternateShortcuts.contains(se->key()),
                   "QGuiAction::event",
                   "Received shortcut event from incorrect shortcut");
        if (se->isAmbiguous())
            qWarning("QGuiAction::event: Ambiguous shortcut overload: %s", se->key().toString(QKeySequence::NativeText).toLatin1().constData());
        else
            activate(Trigger);
        return true;
    }
#endif // QT_CONFIG(shortcut)
    return QObject::event(e);
}

/*!
  Returns the user data as set in QGuiAction::setData.

  \sa setData()
*/
QVariant QGuiAction::data() const
{
    Q_D(const QGuiAction);
    return d->userData;
}

/*!
  Sets the action's internal data to the given \a userData.

  \sa data()
*/
void QGuiAction::setData(const QVariant &data)
{
    Q_D(QGuiAction);
    if (d->userData == data)
        return;
    d->userData = data;
    d->sendDataChanged();
}

/*!
  Sends the relevant signals for ActionEvent \a event.

  Action based widgets use this API to cause the QGuiAction
  to emit signals as well as emitting their own.
*/
void QGuiAction::activate(ActionEvent event)
{
    Q_D(QGuiAction);
    if(event == Trigger) {
        QPointer<QObject> guard = this;
        if(d->checkable) {
            // the checked action of an exclusive group may not be unchecked
            if (d->checked && (d->group
                               && d->group->exclusionPolicy() == QGuiActionGroup::ExclusionPolicy::Exclusive
                               && d->group->checkedGuiAction() == this)) {
                if (!guard.isNull())
                    emit triggered(true);
                return;
            }
            setChecked(!d->checked);
        }
        if (!guard.isNull())
            emit triggered(d->checked);
    } else if(event == Hover) {
        emit hovered();
    }
}

/*!
    \fn void QGuiAction::triggered(bool checked)

    This signal is emitted when an action is activated by the user;
    for example, when the user clicks a menu option, toolbar button,
    or presses an action's shortcut key combination, or when trigger()
    was called. Notably, it is \e not emitted when setChecked() or
    toggle() is called.

    If the action is checkable, \a checked is true if the action is
    checked, or false if the action is unchecked.

    \sa activate(), toggled(), checked
*/

/*!
    \fn void QGuiAction::toggled(bool checked)

    This signal is emitted whenever a checkable action changes its
    isChecked() status. This can be the result of a user interaction,
    or because setChecked() was called. As setChecked() changes the
    QGuiAction, it emits changed() in addition to toggled().

    \a checked is true if the action is checked, or false if the
    action is unchecked.

    \sa activate(), triggered(), checked
*/

/*!
    \fn void QGuiAction::hovered()

    This signal is emitted when an action is highlighted by the user;
    for example, when the user pauses with the cursor over a menu option,
    toolbar button, or presses an action's shortcut key combination.

    \sa activate()
*/

/*!
    \fn void QGuiAction::changed()

    This signal is emitted when an action has changed. If you
    are only interested in actions in a given widget, you can
    watch for QWidget::actionEvent() sent with an
    QEvent::ActionChanged.

    \sa QWidget::actionEvent()
*/

/*!
    \enum QGuiAction::ActionEvent

    This enum type is used when calling QGuiAction::activate()

    \value Trigger this will cause the QGuiAction::triggered() signal to be emitted.

    \value Hover this will cause the QGuiAction::hovered() signal to be emitted.
*/

/*!
    \property QGuiAction::menuRole
    \brief the action's menu role

    This indicates what role the action serves in the application menu on
    \macos. By default all actions have the TextHeuristicRole, which means that
    the action is added based on its text (see QMenuBar for more information).

    The menu role can only be changed before the actions are put into the menu
    bar in \macos (usually just before the first application window is
    shown).
*/
void QGuiAction::setMenuRole(MenuRole menuRole)
{
    Q_D(QGuiAction);
    if (d->menuRole == menuRole)
        return;

    d->menuRole = menuRole;
    d->sendDataChanged();
}

QGuiAction::MenuRole QGuiAction::menuRole() const
{
    Q_D(const QGuiAction);
    return d->menuRole;
}

/*!
    \property QGuiAction::iconVisibleInMenu
    \brief Whether or not an action should show an icon in a menu

    In some applications, it may make sense to have actions with icons in the
    toolbar, but not in menus. If true, the icon (if valid) is shown in the menu, when it
    is false, it is not shown.

    The default is to follow whether the Qt::AA_DontShowIconsInMenus attribute
    is set for the application. Explicitly settings this property overrides
    the presence (or abscence) of the attribute.

    For example:
    \snippet code/src_gui_kernel_qaction.cpp 0

    \sa icon, QCoreApplication::setAttribute()
*/
void QGuiAction::setIconVisibleInMenu(bool visible)
{
    Q_D(QGuiAction);
    if (d->iconVisibleInMenu == -1 || visible != bool(d->iconVisibleInMenu)) {
        int oldValue = d->iconVisibleInMenu;
        d->iconVisibleInMenu = visible;
        // Only send data changed if we really need to.
        if (oldValue != -1
            || visible == !QCoreApplication::testAttribute(Qt::AA_DontShowIconsInMenus)) {
            d->sendDataChanged();
        }
    }
}

bool QGuiAction::isIconVisibleInMenu() const
{
    Q_D(const QGuiAction);
    if (d->iconVisibleInMenu == -1) {
        return !QCoreApplication::testAttribute(Qt::AA_DontShowIconsInMenus);
    }
    return d->iconVisibleInMenu;
}

/*!
    \property QGuiAction::shortcutVisibleInContextMenu
    \brief Whether or not an action should show a shortcut in a context menu

    In some applications, it may make sense to have actions with shortcuts in
    context menus. If true, the shortcut (if valid) is shown when the action is
    shown via a context menu, when it is false, it is not shown.

    The default is to follow whether the Qt::AA_DontShowShortcutsInContextMenus attribute
    is set for the application, falling back to the widget style hint.
    Explicitly setting this property overrides the presence (or abscence) of the attribute.

    \sa shortcut, QCoreApplication::setAttribute()
*/
void QGuiAction::setShortcutVisibleInContextMenu(bool visible)
{
    Q_D(QGuiAction);
    if (d->shortcutVisibleInContextMenu == -1 || visible != bool(d->shortcutVisibleInContextMenu)) {
        int oldValue = d->shortcutVisibleInContextMenu;
        d->shortcutVisibleInContextMenu = visible;
        // Only send data changed if we really need to.
        if (oldValue != -1
            || visible == !QCoreApplication::testAttribute(Qt::AA_DontShowShortcutsInContextMenus)) {
            d->sendDataChanged();
        }
    }
}

bool QGuiAction::isShortcutVisibleInContextMenu() const
{
    Q_D(const QGuiAction);
    if (d->shortcutVisibleInContextMenu == -1) {
        return !QCoreApplication::testAttribute(Qt::AA_DontShowShortcutsInContextMenus)
            && QGuiApplication::styleHints()->showShortcutsInContextMenus();
    }
    return d->shortcutVisibleInContextMenu;
}

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug d, const QGuiAction *action)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "QGuiAction(" << static_cast<const void *>(action);
    if (action) {
        d << " text=" << action->text();
        if (!action->toolTip().isEmpty())
            d << " toolTip=" << action->toolTip();
        if (action->isCheckable())
            d << " checked=" << action->isChecked();
#if QT_CONFIG(shortcut)
        if (!action->shortcut().isEmpty())
            d << " shortcut=" << action->shortcut();
#endif
        d << " menuRole=";
        QtDebugUtils::formatQEnum(d, action->menuRole());
        d << " enabled=" << action->isEnabled();
        d << " visible=" << action->isVisible();
    } else {
        d << '0';
    }
    d << ')';
    return d;
}
#endif // QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#include "moc_qguiaction.cpp"
