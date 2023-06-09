// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaction.h"
#include "qactiongroup.h"

#include "qaction_p.h"
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
        qWarning("QAction: Initialize Q(Gui)Application before calling '" functionName "'."); \
        return; \
    }

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*
  internal: guesses a descriptive text from a text suited for a menu entry
 */
static QString qt_strippedText(QString s)
{
    s.remove("..."_L1);
    for (int i = 0; i < s.size(); ++i) {
        if (s.at(i) == u'&')
            s.remove(i, 1);
    }
    return s.trimmed();
}

QActionPrivate *QGuiApplicationPrivate::createActionPrivate() const
{
    return new QActionPrivate;
}

QActionPrivate::QActionPrivate() :
#if QT_CONFIG(shortcut)
    autorepeat(1),
#endif
    enabled(1), explicitEnabled(0), explicitEnabledValue(1), visible(1), forceInvisible(0), checkable(0),
    checked(0), separator(0), fontSet(false),
    iconVisibleInMenu(-1), shortcutVisibleInContextMenu(-1)
{
}

#if QT_CONFIG(shortcut)
static bool dummy(QObject *, Qt::ShortcutContext) { return false; } // only for GUI testing.

QShortcutMap::ContextMatcher QActionPrivate::contextMatcher() const
{
    return dummy;
};
#endif // QT_CONFIG(shortcut)

QActionPrivate::~QActionPrivate() = default;

void QActionPrivate::destroy()
{
}

void QActionPrivate::sendDataChanged()
{
    Q_Q(QAction);
    QActionEvent e(QEvent::ActionChanged, q);
    QCoreApplication::sendEvent(q, &e);

    emit q->changed();
}

#if QT_CONFIG(shortcut)
void QActionPrivate::redoGrab(QShortcutMap &map)
{
    Q_Q(QAction);
    for (int id : std::as_const(shortcutIds)) {
        if (id)
            map.removeShortcut(id, q);
    }

    shortcutIds.clear();
    for (const QKeySequence &shortcut : std::as_const(shortcuts)) {
        if (!shortcut.isEmpty())
            shortcutIds.append(map.addShortcut(q, shortcut, shortcutContext, contextMatcher()));
        else
            shortcutIds.append(0);
    }
    if (!enabled) {
        for (int id : std::as_const(shortcutIds)) {
            if (id)
                map.setShortcutEnabled(false, id, q);
        }
    }
    if (!autorepeat) {
        for (int id : std::as_const(shortcutIds)) {
            if (id)
                map.setShortcutAutoRepeat(false, id, q);
        }
    }
}

void QActionPrivate::setShortcutEnabled(bool enable, QShortcutMap &map)
{
    Q_Q(QAction);
    for (int id : std::as_const(shortcutIds)) {
        if (id)
            map.setShortcutEnabled(enable, id, q);
    }
}
#endif // QT_NO_SHORTCUT

bool QActionPrivate::showStatusText(QObject *object, const QString &str)
{
    if (QObject *receiver = object ? object : parent) {
        QStatusTipEvent tip(str);
        QCoreApplication::sendEvent(receiver, &tip);
        return true;
    }
    return false;
}

void QActionPrivate::setMenu(QObject *)
{
}

QObject *QActionPrivate::menu() const
{
    return nullptr;
}

/*!
    \class QAction
    \brief The QAction class provides an abstraction for user commands
    that can be added to different user interface components.
    \since 6.0

    \inmodule QtGui

    In applications many common commands can be invoked via menus,
    toolbar buttons, and keyboard shortcuts. Since the user expects
    each command to be performed in the same way, regardless of the
    user interface used, it is useful to represent each command as
    an \e action.

    Actions can be added to user interface elements such as menus and toolbars,
    and will automatically keep the UI in sync. For example, in a word
    processor, if the user presses a Bold toolbar button, the Bold menu item
    will automatically be checked.

    A QAction may contain an icon, descriptive text, icon text, a keyboard
    shortcut, status text, "What's This?" text, and a tooltip. All properties
    can be set independently with setIcon(), setText(), setIconText(),
    setShortcut(), setStatusTip(), setWhatsThis(), and setToolTip(). Icon and
    text, as the two most important properties, can also be set in the
    constructor. It's possible to set an individual font with setFont(), which
    e.g. menus respect when displaying the action as a menu item.

    We recommend that actions are created as children of the window
    they are used in. In most cases actions will be children of
    the application's main window.

    \section1 QAction in widget applications

    Once a QAction has been created, it should be added to the relevant
    menu and toolbar, then connected to the slot which will perform
    the action.

    Actions are added to widgets using QWidget::addAction() or
    QGraphicsWidget::addAction(). Note that an action must be added to a
    widget before it can be used. This is also true when the shortcut should
    be global (i.e., Qt::ApplicationShortcut as Qt::ShortcutContext).

    Actions can be created as independent objects. But they may
    also be created during the construction of menus. The QMenu class
    contains convenience functions for creating actions suitable for
    use as menu items.


    \sa QMenu, QToolBar
*/

/*!
    \fn void QAction::trigger()

    This is a convenience slot that calls activate(Trigger).
*/

/*!
    \fn void QAction::hover()

    This is a convenience slot that calls activate(Hover).
*/

/*!
    \enum QAction::MenuRole

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
QAction::QAction(QObject *parent)
    : QAction(*QGuiApplicationPrivate::instance()->createActionPrivate(), parent)
{
}

/*!
    Constructs an action with some \a text and \a parent. If \a
    parent is an action group the action will be automatically
    inserted into the group.

    A stripped version of \a text (for example, "\&Menu Option..." becomes
    "Menu Option") will be used for tooltips and icon text unless you specify a
    different text using setToolTip() or setIconText(), respectively.

    \sa text
*/
QAction::QAction(const QString &text, QObject *parent)
    : QAction(parent)
{
    Q_D(QAction);
    d->text = text;
}

/*!
    Constructs an action with an \a icon and some \a text and \a
    parent. If \a parent is an action group the action will be
    automatically inserted into the group.

    A stripped version of \a text (for example, "\&Menu Option..." becomes
    "Menu Option") will be used for tooltips and icon text unless you specify a
    different text using setToolTip() or setIconText(), respectively.

    \sa text, icon
*/
QAction::QAction(const QIcon &icon, const QString &text, QObject *parent)
    : QAction(text, parent)
{
    Q_D(QAction);
    d->icon = icon;
}

/*!
    \internal
*/
QAction::QAction(QActionPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QAction);
    d->group = qobject_cast<QActionGroup *>(parent);
    if (d->group)
        d->group->addAction(this);
}

#if QT_CONFIG(shortcut)
/*!
    \property QAction::shortcut
    \brief the action's primary shortcut key

    Valid keycodes for this property can be found in \l Qt::Key and
    \l Qt::Modifier. There is no default shortcut key.
*/

/*!
    Sets \a shortcut as the sole shortcut that triggers the action.

    \sa shortcut, setShortcuts()
*/
void QAction::setShortcut(const QKeySequence &shortcut)
{
    if (shortcut.isEmpty())
        setShortcuts({});
    else
        setShortcuts({ shortcut });
}

/*!
    Sets \a shortcuts as the list of shortcuts that trigger the
    action. The first element of the list is the primary shortcut.

    \sa shortcut, setShortcut()
*/
void QAction::setShortcuts(const QList<QKeySequence> &shortcuts)
{
    QAPP_CHECK("setShortcuts");
    Q_D(QAction);

    if (d->shortcuts == shortcuts)
        return;

    d->shortcuts = shortcuts;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
    d->sendDataChanged();
}

/*!
    Sets a platform dependent list of shortcuts based on the \a key.
    The result of calling this function will depend on the currently running platform.
    Note that more than one shortcut can assigned by this action.
    If only the primary shortcut is required, use setShortcut instead.

    \sa QKeySequence::keyBindings()
*/
void QAction::setShortcuts(QKeySequence::StandardKey key)
{
    QList <QKeySequence> list = QKeySequence::keyBindings(key);
    setShortcuts(list);
}

/*!
    Returns the primary shortcut.

    \sa setShortcuts()
*/
QKeySequence QAction::shortcut() const
{
    Q_D(const QAction);
    if (d->shortcuts.isEmpty())
        return QKeySequence();
    return d->shortcuts.first();
}

/*!
    Returns the list of shortcuts, with the primary shortcut as
    the first element of the list.

    \sa setShortcuts()
*/
QList<QKeySequence> QAction::shortcuts() const
{
    Q_D(const QAction);
    return d->shortcuts;
}

/*!
    \property QAction::shortcutContext
    \brief the context for the action's shortcut

    Valid values for this property can be found in \l Qt::ShortcutContext.
    The default value is Qt::WindowShortcut.
*/
void QAction::setShortcutContext(Qt::ShortcutContext context)
{
    Q_D(QAction);
    if (d->shortcutContext == context)
        return;
    QAPP_CHECK("setShortcutContext");
    d->shortcutContext = context;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
    d->sendDataChanged();
}

Qt::ShortcutContext QAction::shortcutContext() const
{
    Q_D(const QAction);
    return d->shortcutContext;
}

/*!
    \property QAction::autoRepeat
    \brief whether the action can auto repeat

    If true, the action will auto repeat when the keyboard shortcut
    combination is held down, provided that keyboard auto repeat is
    enabled on the system.
    The default value is true.
*/
void QAction::setAutoRepeat(bool on)
{
    Q_D(QAction);
    if (d->autorepeat == on)
        return;
    QAPP_CHECK("setAutoRepeat");
    d->autorepeat = on;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
    d->sendDataChanged();
}

bool QAction::autoRepeat() const
{
    Q_D(const QAction);
    return d->autorepeat;
}
#endif // QT_CONFIG(shortcut)

/*!
    \property QAction::font
    \brief the action's font

    The font property is used to render the text set on the
    QAction. The font can be considered a hint as it will not be
    consulted in all cases based upon application and style.

    By default, this property contains the application's default font.

    \sa setText()
*/
void QAction::setFont(const QFont &font)
{
    Q_D(QAction);
    if (d->font == font)
        return;

    d->fontSet = true;
    d->font = font;
    d->sendDataChanged();
}

QFont QAction::font() const
{
    Q_D(const QAction);
    return d->font;
}


/*!
    Destroys the object and frees allocated resources.
*/
QAction::~QAction()
{
    Q_D(QAction);

    d->destroy();

    if (d->group)
        d->group->removeAction(this);
#if QT_CONFIG(shortcut)
    if (qApp) {
        for (int id : std::as_const(d->shortcutIds)) {
            if (id)
                QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(id, this);
        }
    }
#endif
}

/*!
  Sets this action group to \a group. The action will be automatically
  added to the group's list of actions.

  Actions within the group will be mutually exclusive.

  \sa QActionGroup, actionGroup()
*/
void QAction::setActionGroup(QActionGroup *group)
{
    Q_D(QAction);
    if (group == d->group)
        return;

    if (d->group)
        d->group->removeAction(this);
    d->group = group;
    if (group)
        group->addAction(this);
    d->sendDataChanged();
}

/*!
  Returns the action group for this action. If no action group manages
  this action, then \nullptr will be returned.

  \sa QActionGroup, setActionGroup()
*/
QActionGroup *QAction::actionGroup() const
{
    Q_D(const QAction);
    return d->group;
}

/*!
  \since 6.0
  Returns a list of objects this action has been added to.

  \sa QWidget::addAction(), QGraphicsWidget::addAction()
*/
QList<QObject*> QAction::associatedObjects() const
{
    Q_D(const QAction);
    return d->associatedObjects;
}

/*!
    \fn QWidget *QAction::parentWidget() const
    \deprecated [6.0] Use parent() with qobject_cast() instead.

    Returns the parent widget.
*/

/*!
    \fn QList<QWidget*> QAction::associatedWidgets() const
    \deprecated [6.0] Use associatedObjects() with qobject_cast() instead.

    Returns a list of widgets this action has been added to.

    \sa QWidget::addAction(), associatedObjects(), associatedGraphicsWidgets()
*/

/*!
    \fn QList<QWidget*> QAction::associatedGraphicsWidgets() const
    \deprecated [6.0] Use associatedObjects() with qobject_cast() instead.

    Returns a list of graphics widgets this action has been added to.

    \sa QGraphicsWidget::addAction(), associatedObjects(), associatedWidgets()
*/

/*!
    \property QAction::icon
    \brief the action's icon

    In toolbars, the icon is used as the tool button icon; in menus,
    it is displayed to the left of the menu text. There is no default
    icon.

    If a null icon (QIcon::isNull()) is passed into this function,
    the icon of the action is cleared.
*/
void QAction::setIcon(const QIcon &icon)
{
    Q_D(QAction);
    d->icon = icon;
    d->sendDataChanged();
}

QIcon QAction::icon() const
{
    Q_D(const QAction);
    return d->icon;
}

/*!
  If \a b is true then this action will be considered a separator.

  How a separator is represented depends on the widget it is inserted
  into. Under most circumstances the text, submenu, and icon will be
  ignored for separator actions.

  \sa isSeparator()
*/
void QAction::setSeparator(bool b)
{
    Q_D(QAction);
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
bool QAction::isSeparator() const
{
    Q_D(const QAction);
    return d->separator;
}

/*!
    \property QAction::text
    \brief the action's descriptive text

    If the action is added to a menu, the menu option will consist of
    the icon (if there is one), the text, and the shortcut (if there
    is one). If the text is not explicitly set in the constructor, or
    by using setText(), the action's description icon text will be
    used as text. There is no default text.

    Certain UI elements, such as menus or buttons, can use '&' in front of a
    character to automatically create a mnemonic (a shortcut) for that
    character. For example, "&File" for a menu will create the shortcut
    \uicontrol Alt+F, which will open the File menu. "E&xit" will create the
    shortcut \uicontrol Alt+X for a button, or in a menu allow navigating to
    the menu item by pressing "x". (use '&&' to display an actual ampersand).
    The widget might consume and perform an action on a given shortcut.

    \sa iconText
*/
void QAction::setText(const QString &text)
{
    Q_D(QAction);
    if (d->text == text)
        return;

    d->text = text;
    d->sendDataChanged();
}

QString QAction::text() const
{
    Q_D(const QAction);
    QString s = d->text;
    if (s.isEmpty()) {
        s = d->iconText;
        s.replace(u'&', "&&"_L1);
    }
    return s;
}

/*!
    \property QAction::iconText
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
void QAction::setIconText(const QString &text)
{
    Q_D(QAction);
    if (d->iconText == text)
        return;

    d->iconText = text;
    d->sendDataChanged();
}

QString QAction::iconText() const
{
    Q_D(const QAction);
    if (d->iconText.isEmpty())
        return qt_strippedText(d->text);
    return d->iconText;
}

/*!
    \property QAction::toolTip
    \brief the action's tooltip

    This text is used for the tooltip. If no tooltip is specified,
    the action's text is used.

    By default, this property contains the action's text.

    \sa setStatusTip(), setShortcut()
*/
void QAction::setToolTip(const QString &tooltip)
{
    Q_D(QAction);
    if (d->tooltip == tooltip)
        return;

    d->tooltip = tooltip;
    d->sendDataChanged();
}

QString QAction::toolTip() const
{
    Q_D(const QAction);
    if (d->tooltip.isEmpty()) {
        if (!d->text.isEmpty())
            return qt_strippedText(d->text);
        return qt_strippedText(d->iconText);
    }
    return d->tooltip;
}

/*!
    \property QAction::statusTip
    \brief the action's status tip

    The status tip is displayed on all status bars provided by the
    action's top-level parent widget.

    By default, this property contains an empty string.

    \sa setToolTip(), showStatusText()
*/
void QAction::setStatusTip(const QString &statustip)
{
    Q_D(QAction);
    if (d->statustip == statustip)
        return;

    d->statustip = statustip;
    d->sendDataChanged();
}

QString QAction::statusTip() const
{
    Q_D(const QAction);
    return d->statustip;
}

/*!
  Updates the relevant status bar for the UI represented by \a object by sending a
  QStatusTipEvent. Returns \c true if an event was sent, otherwise returns \c false.

  If a null widget is specified, the event is sent to the action's parent.

  \sa statusTip
*/
bool QAction::showStatusText(QObject *object)
{
    Q_D(QAction);
    return d->showStatusText(object, statusTip());
}

/*!
    \property QAction::whatsThis
    \brief the action's "What's This?" help text

    The "What's This?" text is used to provide a brief description of
    the action. The text may contain rich text. There is no default
    "What's This?" text.

    \sa QWhatsThis
*/
void QAction::setWhatsThis(const QString &whatsthis)
{
    Q_D(QAction);
    if (d->whatsthis == whatsthis)
        return;

    d->whatsthis = whatsthis;
    d->sendDataChanged();
}

QString QAction::whatsThis() const
{
    Q_D(const QAction);
    return d->whatsthis;
}

/*!
    \enum QAction::Priority

    This enum defines priorities for actions in user interface.

    \value LowPriority The action should not be prioritized in
    the user interface.

    \value NormalPriority

    \value HighPriority The action should be prioritized in
    the user interface.

    \sa priority
*/


/*!
    \property QAction::priority

    \brief the actions's priority in the user interface.

    This property can be set to indicate how the action should be prioritized
    in the user interface.

    For instance, when toolbars have the Qt::ToolButtonTextBesideIcon
    mode set, then actions with LowPriority will not show the text
    labels.
*/
void QAction::setPriority(Priority priority)
{
    Q_D(QAction);
    if (d->priority == priority)
        return;

    d->priority = priority;
    d->sendDataChanged();
}

QAction::Priority QAction::priority() const
{
    Q_D(const QAction);
    return d->priority;
}

/*!
    \property QAction::checkable
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
    QActionGroup with the QActionGroup::exclusive property set to
    true.

    \sa setChecked()
*/
void QAction::setCheckable(bool b)
{
    Q_D(QAction);
    if (d->checkable == b)
        return;

    d->checkable = b;
    QPointer<QAction> guard(this);
    d->sendDataChanged();
    if (guard)
        emit checkableChanged(b);
    if (guard && d->checked)
        emit toggled(b);
}

bool QAction::isCheckable() const
{
    Q_D(const QAction);
    return d->checkable;
}

/*!
    \fn void QAction::toggle()

    This is a convenience function for the \l checked property.
    Connect to it to change the checked state to its opposite state.
*/
void QAction::toggle()
{
    Q_D(QAction);
    setChecked(!d->checked);
}

/*!
    \property QAction::checked
    \brief whether the action is checked.

    Only checkable actions can be checked.  By default, this is false
    (the action is unchecked).

    \note The notifier signal for this property is toggled(). As toggling
    a QAction changes its state, it will also emit a changed() signal.

    \sa checkable, toggled()
*/
void QAction::setChecked(bool b)
{
    Q_D(QAction);
    if (d->checked == b)
        return;

    d->checked = b;
    if (!d->checkable)
        return;
    QPointer<QAction> guard(this);
    d->sendDataChanged();
    if (guard)
        emit toggled(b);
}

bool QAction::isChecked() const
{
    Q_D(const QAction);
    return d->checked && d->checkable;
}

/*!
    \fn void QAction::setDisabled(bool b)

    This is a convenience function for the \l enabled property, that
    is useful for signals--slots connections. If \a b is true the
    action is disabled; otherwise it is enabled.
*/

/*!
    \property QAction::enabled
    \brief whether the action is enabled

    Disabled actions cannot be chosen by the user. They do not
    disappear from menus or toolbars, but they are displayed in a way
    which indicates that they are unavailable. For example, they might
    be displayed using only shades of gray.

    \uicontrol{What's This?} help on disabled actions is still available, provided
    that the QAction::whatsThis property is set.

    An action will be disabled when all widgets to which it is added
    (with QWidget::addAction()) are disabled or not visible. When an
    action is disabled, it is not possible to trigger it through its
    shortcut.

    By default, this property is \c true (actions are enabled).

    \sa text
*/
void QAction::setEnabled(bool b)
{
    Q_D(QAction);
    if (d->explicitEnabledValue == b && d->explicitEnabled)
        return;
    d->explicitEnabledValue = b;
    d->explicitEnabled = true;
    QAPP_CHECK("setEnabled");
    d->setEnabled(b, false);
}

bool QActionPrivate::setEnabled(bool b, bool byGroup)
{
    Q_Q(QAction);
    if (b && !visible)
        b = false;
    if (b && !byGroup && (group && !group->isEnabled()))
        b = false;
    if (b && byGroup && explicitEnabled)
        b = explicitEnabledValue;

    if (b == enabled)
        return false;

    enabled = b;
#if QT_CONFIG(shortcut)
    setShortcutEnabled(b, QGuiApplicationPrivate::instance()->shortcutMap);
#endif
    QPointer guard(q);
    sendDataChanged();
    if (guard)
        emit q->enabledChanged(b);
    return true;
}

void QAction::resetEnabled()
{
    Q_D(QAction);
    if (!d->explicitEnabled)
        return;
    d->explicitEnabled = false;
    d->setEnabled(true, false);
}

bool QAction::isEnabled() const
{
    Q_D(const QAction);
    return d->enabled;
}

/*!
    \property QAction::visible
    \brief whether the action can be seen (e.g. in menus and toolbars)

    If \e visible is true the action can be seen (e.g. in menus and
    toolbars) and chosen by the user; if \e visible is false the
    action cannot be seen or chosen by the user.

    Actions which are not visible are \e not grayed out; they do not
    appear at all.

    By default, this property is \c true (actions are visible).
*/
void QAction::setVisible(bool b)
{
    Q_D(QAction);
    if (b != d->forceInvisible)
        return;
    d->forceInvisible = !b;
    if (b && d->group && !d->group->isVisible())
        return;
    d->setVisible(b);
}

void QActionPrivate::setVisible(bool b)
{
    Q_Q(QAction);
    if (b == visible)
        return;
    QAPP_CHECK("setVisible");
    visible = b;
    bool enable = visible;
    if (enable && explicitEnabled)
        enable = explicitEnabledValue;
    QPointer guard(q);
    if (!setEnabled(enable, false))
        sendDataChanged();
    if (guard)
        emit q->visibleChanged();
}

bool QAction::isVisible() const
{
    Q_D(const QAction);
    return d->visible;
}

/*!
  \reimp
*/
bool QAction::event(QEvent *e)
{
    Q_D(QAction);
    if (e->type() == QEvent::ActionChanged) {
        for (auto object : std::as_const(d->associatedObjects))
            QCoreApplication::sendEvent(object, e);
    }

#if QT_CONFIG(shortcut)
    if (e->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        Q_ASSERT_X(d_func()->shortcutIds.contains(se->shortcutId()),
                   "QAction::event",
                   "Received shortcut event from incorrect shortcut");
        if (se->isAmbiguous())
            qWarning("QAction::event: Ambiguous shortcut overload: %s", se->key().toString(QKeySequence::NativeText).toLatin1().constData());
        else
            activate(Trigger);
        return true;
    }
#endif // QT_CONFIG(shortcut)
    return QObject::event(e);
}

/*!
  Returns the user data as set in QAction::setData.

  \sa setData()
*/
QVariant QAction::data() const
{
    Q_D(const QAction);
    return d->userData;
}

/*!
  Sets the action's internal data to the given \a data.

  \sa data()
*/
void QAction::setData(const QVariant &data)
{
    Q_D(QAction);
    if (d->userData == data)
        return;
    d->userData = data;
    d->sendDataChanged();
}

/*!
  Sends the relevant signals for ActionEvent \a event.

  Action-based widgets use this API to cause the QAction
  to emit signals as well as emitting their own.
*/
void QAction::activate(ActionEvent event)
{
    Q_D(QAction);
    if (event == Trigger) {
        // Ignore even explicit triggers when explicitly disabled
        if ((d->explicitEnabled && !d->explicitEnabledValue) || (d->group && !d->group->isEnabled()))
            return;
        QPointer<QObject> guard = this;
        if (d->checkable) {
            // the checked action of an exclusive group may not be unchecked
            if (d->checked && (d->group
                               && d->group->exclusionPolicy() == QActionGroup::ExclusionPolicy::Exclusive
                               && d->group->checkedAction() == this)) {
                if (!guard.isNull())
                    emit triggered(true);
                return;
            }
            setChecked(!d->checked);
        }
        if (!guard.isNull())
            emit triggered(d->checked);
    } else if (event == Hover) {
        emit hovered();
    }
}

/*!
    \fn void QAction::triggered(bool checked)

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
    \fn void QAction::toggled(bool checked)

    This signal is emitted whenever a checkable action changes its
    isChecked() status. This can be the result of a user interaction,
    or because setChecked() was called. As setChecked() changes the
    QAction, it emits changed() in addition to toggled().

    \a checked is true if the action is checked, or false if the
    action is unchecked.

    \sa activate(), triggered(), checked
*/

/*!
    \fn void QAction::hovered()

    This signal is emitted when an action is highlighted by the user;
    for example, when the user pauses with the cursor over a menu option,
    toolbar button, or presses an action's shortcut key combination.

    \sa activate()
*/

/*!
    \fn void QAction::changed()

    This signal is emitted when an action has changed. If you
    are only interested in actions in a given widget, you can
    watch for QWidget::actionEvent() sent with an
    QEvent::ActionChanged.

    \sa QWidget::actionEvent()
*/

/*!
    \enum QAction::ActionEvent

    This enum type is used when calling QAction::activate()

    \value Trigger this will cause the QAction::triggered() signal to be emitted.

    \value Hover this will cause the QAction::hovered() signal to be emitted.
*/

/*!
    \property QAction::menuRole
    \brief the action's menu role

    This indicates what role the action serves in the application menu on
    \macos. By default all actions have the TextHeuristicRole, which means that
    the action is added based on its text (see QMenuBar for more information).

    The menu role can only be changed before the actions are put into the menu
    bar in \macos (usually just before the first application window is
    shown).
*/
void QAction::setMenuRole(MenuRole menuRole)
{
    Q_D(QAction);
    if (d->menuRole == menuRole)
        return;

    d->menuRole = menuRole;
    d->sendDataChanged();
}

QAction::MenuRole QAction::menuRole() const
{
    Q_D(const QAction);
    return d->menuRole;
}

/*!
    \fn QMenu *QAction::menu() const

    Returns the menu contained by this action.

    In widget applications, actions that contain menus can be used to create menu
    items with submenus, or inserted into toolbars to create buttons with popup menus.

    \sa QMenu::addAction(), QMenu::menuInAction()
*/
QObject* QAction::menuObject() const
{
    Q_D(const QAction);
    return d->menu();
}

/*!
    \fn void QAction::setMenu(QMenu *menu)

    Sets the menu contained by this action to the specified \a menu.
*/
void QAction::setMenuObject(QObject *object)
{
    Q_D(QAction);
    d->setMenu(object);
}

/*!
    \property QAction::iconVisibleInMenu
    \brief Whether or not an action should show an icon in a menu

    In some applications, it may make sense to have actions with icons in the
    toolbar, but not in menus. If true, the icon (if valid) is shown in the menu, when it
    is false, it is not shown.

    The default is to follow whether the Qt::AA_DontShowIconsInMenus attribute
    is set for the application. Explicitly settings this property overrides
    the presence (or absence) of the attribute.

    For example:
    \snippet code/src_gui_kernel_qaction.cpp 0

    \sa icon, QCoreApplication::setAttribute()
*/
void QAction::setIconVisibleInMenu(bool visible)
{
    Q_D(QAction);
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

bool QAction::isIconVisibleInMenu() const
{
    Q_D(const QAction);
    if (d->iconVisibleInMenu == -1) {
        return !QCoreApplication::testAttribute(Qt::AA_DontShowIconsInMenus);
    }
    return d->iconVisibleInMenu;
}

/*!
    \property QAction::shortcutVisibleInContextMenu
    \brief Whether or not an action should show a shortcut in a context menu

    In some applications, it may make sense to have actions with shortcuts in
    context menus. If true, the shortcut (if valid) is shown when the action is
    shown via a context menu, when it is false, it is not shown.

    The default is to follow whether the Qt::AA_DontShowShortcutsInContextMenus attribute
    is set for the application. Explicitly setting this property overrides the attribute.

    \sa shortcut, QCoreApplication::setAttribute()
*/
void QAction::setShortcutVisibleInContextMenu(bool visible)
{
    Q_D(QAction);
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

bool QAction::isShortcutVisibleInContextMenu() const
{
    Q_D(const QAction);
    if (d->shortcutVisibleInContextMenu == -1)
        return !QCoreApplication::testAttribute(Qt::AA_DontShowShortcutsInContextMenus);
    return d->shortcutVisibleInContextMenu;
}

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug d, const QAction *action)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "QAction(" << static_cast<const void *>(action);
    if (action) {
        d << " text=" << action->text();
        if (!action->toolTip().isEmpty())
            d << " toolTip=" << action->toolTip();
        if (action->isCheckable())
            d << " checked=" << action->isChecked();
#if QT_CONFIG(shortcut)
        if (!action->shortcuts().isEmpty())
            d << " shortcuts=" << action->shortcuts();
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

#include "moc_qaction.cpp"
