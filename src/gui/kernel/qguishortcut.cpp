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

#include "qguishortcut.h"
#include "qguishortcut_p.h"

#include <qevent.h>
#include <qguiapplication.h>
#include <qwindow.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformmenu.h>

QT_BEGIN_NAMESPACE

#define QAPP_CHECK(functionName) \
    if (Q_UNLIKELY(!qApp)) {                                            \
        qWarning("QGuiShortcut: Initialize QGuiApplication before calling '" functionName "'."); \
        return; \
    }

/*!
    \class QGuiShortcut
    \brief The QGuiShortcut class is a base class for handling keyboard shortcuts.

    \ingroup events
    \inmodule QtGui
    \since 6.0

    The QGuiShortcut class is a base class for classes providing a way of
    connecting keyboard shortcuts to Qt's \l{signals and slots} mechanism,
    so that objects can be informed when a shortcut is executed. The shortcut
    can be set up to contain all the key presses necessary to
    describe a keyboard shortcut, including the states of modifier
    keys such as \uicontrol Shift, \uicontrol Ctrl, and \uicontrol Alt.

    \target mnemonic

    \sa QShortcutEvent, QKeySequence, QAction
*/

/*!
    \fn void QGuiShortcut::activated()

    This signal is emitted when the user types the shortcut's key
    sequence.

    \sa activatedAmbiguously()
*/

/*!
    \fn void QGuiShortcut::activatedAmbiguously()

    When a key sequence is being typed at the keyboard, it is said to
    be ambiguous as long as it matches the start of more than one
    shortcut.

    When a shortcut's key sequence is completed,
    activatedAmbiguously() is emitted if the key sequence is still
    ambiguous (i.e., it is the start of one or more other shortcuts).
    The activated() signal is not emitted in this case.

    \sa activated()
*/

static bool simpleContextMatcher(QObject *object, Qt::ShortcutContext context)
{
    auto guiShortcut = qobject_cast<QGuiShortcut *>(object);
    if (QGuiApplication::applicationState() != Qt::ApplicationActive || guiShortcut == nullptr)
        return false;
    if (context == Qt::ApplicationShortcut)
        return true;
    auto focusWindow = QGuiApplication::focusWindow();
    if (!focusWindow)
        return false;
    auto window = qobject_cast<const QWindow *>(guiShortcut->parent());
    if (!window)
        return false;
    if (focusWindow == window && focusWindow->isTopLevel())
        return context == Qt::WindowShortcut || context == Qt::WidgetWithChildrenShortcut;
    return focusWindow->isAncestorOf(window, QWindow::ExcludeTransients);
}

QShortcutMap::ContextMatcher QGuiShortcutPrivate::contextMatcher() const
{
    return simpleContextMatcher;
}

void QGuiShortcutPrivate::redoGrab(QShortcutMap &map)
{
    Q_Q(QGuiShortcut);
    if (Q_UNLIKELY(!parent)) {
        qWarning("QGuiShortcut: No window parent defined");
        return;
    }

    if (sc_id)
        map.removeShortcut(sc_id, q);
    if (sc_sequence.isEmpty())
        return;
    sc_id = map.addShortcut(q, sc_sequence, sc_context, contextMatcher());
    if (!sc_enabled)
        map.setShortcutEnabled(false, sc_id, q);
    if (!sc_autorepeat)
        map.setShortcutAutoRepeat(false, sc_id, q);
}

/*!
    Constructs a QGuiShortcut object for the \a parent window. Since no
    shortcut key sequence is specified, the shortcut will not emit any
    signals.

    \sa setKey()
*/
QGuiShortcut::QGuiShortcut(QWindow *parent)
    : QGuiShortcut(*new QGuiShortcutPrivate, parent)
{
}

/*!
    Constructs a QGuiShortcut object for the \a parent window. The shortcut
    operates on its parent, listening for \l{QShortcutEvent}s that
    match the \a key sequence. Depending on the ambiguity of the
    event, the shortcut will call the \a member function, or the \a
    ambiguousMember function, if the key press was in the shortcut's
    \a context.
*/
QGuiShortcut::QGuiShortcut(const QKeySequence &key, QWindow *parent,
                           const char *member, const char *ambiguousMember,
                           Qt::ShortcutContext context)
    : QGuiShortcut(*new QGuiShortcutPrivate, key, parent, member, ambiguousMember, context)
{
}

/*!
    \internal
*/
QGuiShortcut::QGuiShortcut(QGuiShortcutPrivate &dd, QObject *parent)
     : QObject(dd, parent)
{
    Q_ASSERT(parent != nullptr);
}

/*!
    \internal
*/
QGuiShortcut::QGuiShortcut(QGuiShortcutPrivate &dd,
                           const QKeySequence &key, QObject *parent,
                           const char *member, const char *ambiguousMember,
                           Qt::ShortcutContext context)
    : QGuiShortcut(dd, parent)
{
    QAPP_CHECK("QGuiShortcut");

    Q_D(QGuiShortcut);
    d->sc_context = context;
    d->sc_sequence = key;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
    if (member)
        connect(this, SIGNAL(activated()), parent, member);
    if (ambiguousMember)
        connect(this, SIGNAL(activatedAmbiguously()), parent, ambiguousMember);
}

/*!
    Destroys the shortcut.
*/
QGuiShortcut::~QGuiShortcut()
{
    Q_D(QGuiShortcut);
    if (qApp)
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(d->sc_id, this);
}

/*!
    \property QGuiShortcut::key
    \brief the shortcut's key sequence

    This is a key sequence with an optional combination of Shift, Ctrl,
    and Alt. The key sequence may be supplied in a number of ways:

    \snippet code/src_gui_kernel_qshortcut.cpp 1

    By default, this property contains an empty key sequence.
*/
void QGuiShortcut::setKey(const QKeySequence &key)
{
    Q_D(QGuiShortcut);
    if (d->sc_sequence == key)
        return;
    QAPP_CHECK("setKey");
    d->sc_sequence = key;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
}

QKeySequence QGuiShortcut::key() const
{
    Q_D(const QGuiShortcut);
    return d->sc_sequence;
}

/*!
    \property QGuiShortcut::enabled
    \brief whether the shortcut is enabled

    An enabled shortcut emits the activated() or activatedAmbiguously()
    signal when a QShortcutEvent occurs that matches the shortcut's
    key() sequence.

    If the application is in \c WhatsThis mode the shortcut will not emit
    the signals, but will show the "What's This?" text instead.

    By default, this property is \c true.

    \sa whatsThis
*/
void QGuiShortcut::setEnabled(bool enable)
{
    Q_D(QGuiShortcut);
    if (d->sc_enabled == enable)
        return;
    QAPP_CHECK("setEnabled");
    d->sc_enabled = enable;
    QGuiApplicationPrivate::instance()->shortcutMap.setShortcutEnabled(enable, d->sc_id, this);
}

bool QGuiShortcut::isEnabled() const
{
    Q_D(const QGuiShortcut);
    return d->sc_enabled;
}

/*!
    \property QGuiShortcut::context
    \brief the context in which the shortcut is valid

    A shortcut's context decides in which circumstances a shortcut is
    allowed to be triggered. The normal context is Qt::WindowShortcut,
    which allows the shortcut to trigger if the parent (the widget
    containing the shortcut) is a subwidget of the active top-level
    window.

    By default, this property is set to Qt::WindowShortcut.
*/
void QGuiShortcut::setContext(Qt::ShortcutContext context)
{
    Q_D(QGuiShortcut);
    if (d->sc_context == context)
        return;
    QAPP_CHECK("setContext");
    d->sc_context = context;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
}

Qt::ShortcutContext QGuiShortcut::context() const
{
    Q_D(const QGuiShortcut);
    return d->sc_context;
}

/*!
    \property QGuiShortcut::autoRepeat
    \brief whether the shortcut can auto repeat

    If true, the shortcut will auto repeat when the keyboard shortcut
    combination is held down, provided that keyboard auto repeat is
    enabled on the system.
    The default value is true.
*/
void QGuiShortcut::setAutoRepeat(bool on)
{
    Q_D(QGuiShortcut);
    if (d->sc_autorepeat == on)
        return;
    QAPP_CHECK("setAutoRepeat");
    d->sc_autorepeat = on;
    QGuiApplicationPrivate::instance()->shortcutMap.setShortcutAutoRepeat(on, d->sc_id, this);
}

bool QGuiShortcut::autoRepeat() const
{
    Q_D(const QGuiShortcut);
    return d->sc_autorepeat;
}

/*!
    Returns the shortcut's ID.

    \sa QShortcutEvent::shortcutId()
*/
int QGuiShortcut::id() const
{
    Q_D(const QGuiShortcut);
    return d->sc_id;
}

/*!
    \internal
*/
bool QGuiShortcut::event(QEvent *e)
{
    Q_D(QGuiShortcut);
    if (d->sc_enabled && e->type() == QEvent::Shortcut) {
        auto se = static_cast<QShortcutEvent *>(e);
        if (se->shortcutId() == d->sc_id && se->key() == d->sc_sequence
            && !d->handleWhatsThis()) {
            if (se->isAmbiguous())
                emit activatedAmbiguously();
            else
                emit activated();
            return true;
        }
    }
    return QObject::event(e);
}

QT_END_NAMESPACE

#include "moc_qguishortcut.cpp"
