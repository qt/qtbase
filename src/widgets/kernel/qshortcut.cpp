/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qshortcut.h"
#include "private/qwidget_p.h"

#ifndef QT_NO_SHORTCUT
#include <qevent.h>
#if QT_CONFIG(whatsthis)
#include <qwhatsthis.h>
#endif
#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#if QT_CONFIG(menubar)
#include <qmenubar.h>
#endif
#include <qapplication.h>
#include <private/qapplication_p.h>
#include <private/qshortcutmap_p.h>
#include <private/qaction_p.h>
#include <private/qwidgetwindow_p.h>
#include <qpa/qplatformmenu.h>

QT_BEGIN_NAMESPACE

#define QAPP_CHECK(functionName) \
    if (Q_UNLIKELY(!qApp)) {                                            \
        qWarning("QShortcut: Initialize QApplication before calling '" functionName "'."); \
        return; \
    }


static bool correctWidgetContext(Qt::ShortcutContext context, QWidget *w, QWidget *active_window);
#if QT_CONFIG(graphicsview)
static bool correctGraphicsWidgetContext(Qt::ShortcutContext context, QGraphicsWidget *w, QWidget *active_window);
#endif
#ifndef QT_NO_ACTION
static bool correctActionContext(Qt::ShortcutContext context, QAction *a, QWidget *active_window);
#endif


/*! \internal
    Returns \c true if the widget \a w is a logical sub window of the current
    top-level widget.
*/
bool qWidgetShortcutContextMatcher(QObject *object, Qt::ShortcutContext context)
{
    Q_ASSERT_X(object, "QShortcutMap", "Shortcut has no owner. Illegal map state!");

    QWidget *active_window = QApplication::activeWindow();

    // popups do not become the active window,
    // so we fake it here to get the correct context
    // for the shortcut system.
    if (QApplication::activePopupWidget())
        active_window = QApplication::activePopupWidget();

    if (!active_window) {
        QWindow *qwindow = QGuiApplication::focusWindow();
        if (qwindow && qwindow->isActive()) {
            while (qwindow) {
                if (auto widgetWindow = qobject_cast<QWidgetWindow *>(qwindow)) {
                    active_window = widgetWindow->widget();
                    break;
                }
                qwindow = qwindow->parent();
            }
        }
    }

    if (!active_window)
        return false;

#ifndef QT_NO_ACTION
    if (auto a = qobject_cast<QAction *>(object))
        return correctActionContext(context, a, active_window);
#endif

#if QT_CONFIG(graphicsview)
    if (auto gw = qobject_cast<QGraphicsWidget *>(object))
        return correctGraphicsWidgetContext(context, gw, active_window);
#endif

    auto w = qobject_cast<QWidget *>(object);
    if (!w) {
        if (auto s = qobject_cast<QShortcut *>(object))
            w = s->parentWidget();
    }

    if (!w) {
        auto qwindow = qobject_cast<QWindow *>(object);
        while (qwindow) {
            if (auto widget_window = qobject_cast<QWidgetWindow *>(qwindow)) {
                w = widget_window->widget();
                break;
            }
            qwindow = qwindow->parent();
        }
    }

    if (!w)
        return false;

    return correctWidgetContext(context, w, active_window);
}

static bool correctWidgetContext(Qt::ShortcutContext context, QWidget *w, QWidget *active_window)
{
    bool visible = w->isVisible();
#if QT_CONFIG(menubar)
    if (auto menuBar = qobject_cast<QMenuBar *>(w)) {
        if (auto *pmb = menuBar->platformMenuBar()) {
            if (menuBar->parentWidget()) {
                visible = true;
            } else {
                if (auto *ww = qobject_cast<QWidgetWindow *>(pmb->parentWindow()))
                    w = ww->widget(); // Good enough since we only care about the window
                else
                    return false; // This is not a QWidget window. We won't deliver
            }
        }
    }
#endif

    if (!visible || !w->isEnabled())
        return false;

    if (context == Qt::ApplicationShortcut)
        return QApplicationPrivate::tryModalHelper(w, nullptr); // true, unless w is shadowed by a modal dialog

    if (context == Qt::WidgetShortcut)
        return w == QApplication::focusWidget();

    if (context == Qt::WidgetWithChildrenShortcut) {
        const QWidget *tw = QApplication::focusWidget();
        while (tw && tw != w && (tw->windowType() == Qt::Widget || tw->windowType() == Qt::Popup || tw->windowType() == Qt::SubWindow))
            tw = tw->parentWidget();
        return tw == w;
    }

    // Below is Qt::WindowShortcut context
    QWidget *tlw = w->window();
#if QT_CONFIG(graphicsview)
    if (auto topData = static_cast<QWidgetPrivate *>(QObjectPrivate::get(tlw))->extra.get()) {
        if (topData->proxyWidget) {
            bool res = correctGraphicsWidgetContext(context, topData->proxyWidget, active_window);
            return res;
        }
    }
#endif

    if (active_window && active_window != tlw) {
        /* if a floating tool window is active, keep shortcuts on the parent working.
         * and if a popup window is active (f.ex a completer), keep shortcuts on the
         * focus proxy working */
        if (active_window->windowType() == Qt::Tool && active_window->parentWidget()) {
            active_window = active_window->parentWidget()->window();
        } else if (active_window->windowType() == Qt::Popup && active_window->focusProxy()) {
            active_window = active_window->focusProxy()->window();
        }
    }

    if (active_window != tlw) {
#if QT_CONFIG(menubar)
        // If the tlw is a QMenuBar then we allow it to proceed as this indicates that
        // the QMenuBar is a parentless one and is therefore used for multiple top level
        // windows in the application. This is common on macOS platforms for example.
        if (!qobject_cast<QMenuBar *>(tlw))
#endif
        return false;
    }

    /* if we live in a MDI subwindow, ignore the event if we are
       not the active document window */
    const QWidget* sw = w;
    while (sw && !(sw->windowType() == Qt::SubWindow) && !sw->isWindow())
        sw = sw->parentWidget();
    if (sw && (sw->windowType() == Qt::SubWindow)) {
        QWidget *focus_widget = QApplication::focusWidget();
        while (focus_widget && focus_widget != sw)
            focus_widget = focus_widget->parentWidget();
        return sw == focus_widget;
    }

#if defined(DEBUG_QSHORTCUTMAP)
    qDebug().nospace() << "..true [Pass-through]";
#endif
    return QApplicationPrivate::tryModalHelper(w, nullptr);
}

#if QT_CONFIG(graphicsview)
static bool correctGraphicsWidgetContext(Qt::ShortcutContext context, QGraphicsWidget *w, QWidget *active_window)
{
    bool visible = w->isVisible();
#if defined(Q_OS_DARWIN) && QT_CONFIG(menubar)
    if (!QCoreApplication::testAttribute(Qt::AA_DontUseNativeMenuBar) && qobject_cast<QMenuBar *>(w))
        visible = true;
#endif

    if (!visible || !w->isEnabled() || !w->scene())
        return false;

    if (context == Qt::ApplicationShortcut) {
        // Applicationwide shortcuts are always reachable unless their owner
        // is shadowed by modality. In QGV there's no modality concept, but we
        // must still check if all views are shadowed.
        const auto &views = w->scene()->views();
        for (auto view : views) {
            if (QApplicationPrivate::tryModalHelper(view, nullptr))
                return true;
        }
        return false;
    }

    if (context == Qt::WidgetShortcut)
        return static_cast<QGraphicsItem *>(w) == w->scene()->focusItem();

    if (context == Qt::WidgetWithChildrenShortcut) {
        const QGraphicsItem *ti = w->scene()->focusItem();
        if (ti && ti->isWidget()) {
            const auto *tw = static_cast<const QGraphicsWidget *>(ti);
            while (tw && tw != w && (tw->windowType() == Qt::Widget || tw->windowType() == Qt::Popup))
                tw = tw->parentWidget();
            return tw == w;
        }
        return false;
    }

    // Below is Qt::WindowShortcut context

    // Find the active view (if any).
    const auto &views = w->scene()->views();
    QGraphicsView *activeView = nullptr;
    for (auto view : views) {
        if (view->window() == active_window) {
            activeView = view;
            break;
        }
    }
    if (!activeView)
        return false;

    // The shortcut is reachable if owned by a windowless widget, or if the
    // widget's window is the same as the focus item's window.
    QGraphicsWidget *a = w->scene()->activeWindow();
    return !w->window() || a == w->window();
}
#endif

#ifndef QT_NO_ACTION
static bool correctActionContext(Qt::ShortcutContext context, QAction *a, QWidget *active_window)
{
    const QWidgetList &widgets = static_cast<QActionPrivate *>(QObjectPrivate::get(a))->widgets;
#if defined(DEBUG_QSHORTCUTMAP)
    if (widgets.isEmpty())
        qDebug() << a << "not connected to any widgets; won't trigger";
#endif
    for (auto w : widgets) {
#if QT_CONFIG(menu)
        if (auto menu = qobject_cast<QMenu *>(w)) {
#ifdef Q_OS_DARWIN
            // On Mac, menu item shortcuts are processed before reaching any window.
            // That means that if a menu action shortcut has not been already processed
            // (and reaches this point), then the menu item itself has been disabled.
            // This occurs at the QPA level on Mac, where we disable all the Cocoa menus
            // when showing a modal window. (Notice that only the QPA menu is disabled,
            // not the QMenu.) Since we can also reach this code by climbing the menu
            // hierarchy (see below), or when the shortcut is not a key-equivalent, we
            // need to check whether the QPA menu is actually disabled.
            // When there is no QPA menu, there will be no QCocoaMenuDelegate checking
            // for the actual shortcuts. We can then fallback to our own logic.
            QPlatformMenu *pm = menu->platformMenu();
            if (pm && !pm->isEnabled())
                continue;
#endif
            QAction *a = menu->menuAction();
            if (correctActionContext(context, a, active_window))
                return true;
        } else
#endif
            if (correctWidgetContext(context, w, active_window))
                return true;
    }

#if QT_CONFIG(graphicsview)
    const auto &graphicsWidgets = static_cast<QActionPrivate *>(QObjectPrivate::get(a))->graphicsWidgets;
#if defined(DEBUG_QSHORTCUTMAP)
    if (graphicsWidgets.isEmpty())
        qDebug() << a << "not connected to any widgets; won't trigger";
#endif
    for (auto graphicsWidget : graphicsWidgets) {
        if (correctGraphicsWidgetContext(context, graphicsWidget, active_window))
            return true;
    }
#endif
    return false;
}
#endif // QT_NO_ACTION


/*!
    \class QShortcut
    \brief The QShortcut class is used to create keyboard shortcuts.

    \ingroup events
    \inmodule QtWidgets

    The QShortcut class provides a way of connecting keyboard
    shortcuts to Qt's \l{signals and slots} mechanism, so that
    objects can be informed when a shortcut is executed. The shortcut
    can be set up to contain all the key presses necessary to
    describe a keyboard shortcut, including the states of modifier
    keys such as \uicontrol Shift, \uicontrol Ctrl, and \uicontrol Alt.

    \target mnemonic

    On certain widgets, using '&' in front of a character will
    automatically create a mnemonic (a shortcut) for that character,
    e.g. "E&xit" will create the shortcut \uicontrol Alt+X (use '&&' to
    display an actual ampersand). The widget might consume and perform
    an action on a given shortcut. On X11 the ampersand will not be
    shown and the character will be underlined. On Windows, shortcuts
    are normally not displayed until the user presses the \uicontrol Alt
    key, but this is a setting the user can change. On Mac, shortcuts
    are disabled by default. Call \l qt_set_sequence_auto_mnemonic() to
    enable them. However, because mnemonic shortcuts do not fit in
    with Aqua's guidelines, Qt will not show the shortcut character
    underlined.

    For applications that use menus, it may be more convenient to
    use the convenience functions provided in the QMenu class to
    assign keyboard shortcuts to menu items as they are created.
    Alternatively, shortcuts may be associated with other types of
    actions in the QAction class.

    The simplest way to create a shortcut for a particular widget is
    to construct the shortcut with a key sequence. For example:

    \snippet code/src_gui_kernel_qshortcut.cpp 0

    When the user types the \l{QKeySequence}{key sequence}
    for a given shortcut, the shortcut's activated() signal is
    emitted. (In the case of ambiguity, the activatedAmbiguously()
    signal is emitted.) A shortcut is "listened for" by Qt's event
    loop when the shortcut's parent widget is receiving events.

    A shortcut's key sequence can be set with setKey() and retrieved
    with key(). A shortcut can be enabled or disabled with
    setEnabled(), and can have "What's This?" help text set with
    setWhatsThis().

    \sa QShortcutEvent, QKeySequence, QAction
*/

/*!
    \fn QWidget *QShortcut::parentWidget() const

    Returns the shortcut's parent widget.
*/

/*!
    \fn void QShortcut::activated()

    This signal is emitted when the user types the shortcut's key
    sequence.

    \sa activatedAmbiguously()
*/

/*!
    \fn void QShortcut::activatedAmbiguously()

    When a key sequence is being typed at the keyboard, it is said to
    be ambiguous as long as it matches the start of more than one
    shortcut.

    When a shortcut's key sequence is completed,
    activatedAmbiguously() is emitted if the key sequence is still
    ambiguous (i.e., it is the start of one or more other shortcuts).
    The activated() signal is not emitted in this case.

    \sa activated()
*/

/*!
    \fn template<typename Functor>
        QShortcut(const QKeySequence &key, QWidget *parent,
                  Functor functor,
                  Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    \since 5.15
    \overload

    This is a QShortcut convenience constructor which connects the shortcut's
    \l{QShortcut::activated()}{activated()} signal to the \a functor.
*/
/*!
    \fn template<typename Functor>
        QShortcut(const QKeySequence &key, QWidget *parent,
                  const QObject *context, Functor functor,
                  Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    \since 5.15
    \overload

    This is a QShortcut convenience constructor which connects the shortcut's
    \l{QShortcut::activated()}{activated()} signal to the \a functor.

    The \a functor can be a pointer to a member function of the \a context object.

    If the \a context object is destroyed, the \a functor will not be called.
*/
/*!
    \fn template<typename Functor, typename FunctorAmbiguous>
        QShortcut(const QKeySequence &key, QWidget *parent,
                  const QObject *context1, Functor functor,
                  FunctorAmbiguous functorAmbiguous,
                  Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    \since 5.15
    \overload

    This is a QShortcut convenience constructor which connects the shortcut's
    \l{QShortcut::activated()}{activated()} signal to the \a functor and
    \l{QShortcut::activatedAmbiguously()}{activatedAmbiguously()}
    signal to the \a FunctorAmbiguous.

    The \a functor and \a FunctorAmbiguous can be a pointer to a member
    function of the \a context object.

    If the \a context object is destroyed, the \a functor and
    \a FunctorAmbiguous will not be called.
*/
/*!
    \fn template<typename Functor, typename FunctorAmbiguous>
        QShortcut(const QKeySequence &key, QWidget *parent,
                  const QObject *context1, Functor functor,
                  const QObject *context2, FunctorAmbiguous functorAmbiguous,
                  Qt::ShortcutContext shortcutContext = Qt::WindowShortcut);
    \since 5.15
    \overload

    This is a QShortcut convenience constructor which connects the shortcut's
    \l{QShortcut::activated()}{activated()} signal to the \a functor and
    \l{QShortcut::activatedAmbiguously()}{activatedAmbiguously()}
    signal to the \a FunctorAmbiguous.

    The \a functor can be a pointer to a member function of the
    \a context1 object.
    The \a FunctorAmbiguous can be a pointer to a member function of the
    \a context2 object.

    If the \a context1 object is destroyed, the \a functor will not be called.
    If the \a context2 object is destroyed, the \a FunctorAmbiguous
    will not be called.
*/

/*
    \internal
    Private data accessed through d-pointer.
*/
class QShortcutPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QShortcut)
public:
    QShortcutPrivate() = default;
    QKeySequence sc_sequence;
    Qt::ShortcutContext sc_context = Qt::WindowShortcut;
    bool sc_enabled = true;
    bool sc_autorepeat = true;
    int sc_id = 0;
    QString sc_whatsthis;
    void redoGrab(QShortcutMap &map);
};

void QShortcutPrivate::redoGrab(QShortcutMap &map)
{
    Q_Q(QShortcut);
    if (Q_UNLIKELY(!parent)) {
        qWarning("QShortcut: No widget parent defined");
        return;
    }

    if (sc_id)
        map.removeShortcut(sc_id, q);
    if (sc_sequence.isEmpty())
        return;
    sc_id = map.addShortcut(q, sc_sequence, sc_context, qWidgetShortcutContextMatcher);
    if (!sc_enabled)
        map.setShortcutEnabled(false, sc_id, q);
    if (!sc_autorepeat)
        map.setShortcutAutoRepeat(false, sc_id, q);
}

/*!
    Constructs a QShortcut object for the \a parent widget. Since no
    shortcut key sequence is specified, the shortcut will not emit any
    signals.

    \sa setKey()
*/
QShortcut::QShortcut(QWidget *parent)
    : QObject(*new QShortcutPrivate, parent)
{
    Q_ASSERT(parent != nullptr);
}

/*!
    Constructs a QShortcut object for the \a parent widget. The shortcut
    operates on its parent, listening for \l{QShortcutEvent}s that
    match the \a key sequence. Depending on the ambiguity of the
    event, the shortcut will call the \a member function, or the \a
    ambiguousMember function, if the key press was in the shortcut's
    \a shortcutContext.
*/
QShortcut::QShortcut(const QKeySequence &key, QWidget *parent,
                     const char *member, const char *ambiguousMember,
                     Qt::ShortcutContext shortcutContext)
    : QShortcut(parent)
{
    QAPP_CHECK("QShortcut");

    Q_D(QShortcut);
    d->sc_context = shortcutContext;
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
QShortcut::~QShortcut()
{
    Q_D(QShortcut);
    if (qApp)
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(d->sc_id, this);
}

/*!
    \property QShortcut::key
    \brief the shortcut's key sequence

    This is a key sequence with an optional combination of Shift, Ctrl,
    and Alt. The key sequence may be supplied in a number of ways:

    \snippet code/src_gui_kernel_qshortcut.cpp 1

    By default, this property contains an empty key sequence.
*/
void QShortcut::setKey(const QKeySequence &key)
{
    Q_D(QShortcut);
    if (d->sc_sequence == key)
        return;
    QAPP_CHECK("setKey");
    d->sc_sequence = key;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
}

QKeySequence QShortcut::key() const
{
    Q_D(const QShortcut);
    return d->sc_sequence;
}

/*!
    \property QShortcut::enabled
    \brief whether the shortcut is enabled

    An enabled shortcut emits the activated() or activatedAmbiguously()
    signal when a QShortcutEvent occurs that matches the shortcut's
    key() sequence.

    If the application is in \c WhatsThis mode the shortcut will not emit
    the signals, but will show the "What's This?" text instead.

    By default, this property is \c true.

    \sa whatsThis
*/
void QShortcut::setEnabled(bool enable)
{
    Q_D(QShortcut);
    if (d->sc_enabled == enable)
        return;
    QAPP_CHECK("setEnabled");
    d->sc_enabled = enable;
    QGuiApplicationPrivate::instance()->shortcutMap.setShortcutEnabled(enable, d->sc_id, this);
}

bool QShortcut::isEnabled() const
{
    Q_D(const QShortcut);
    return d->sc_enabled;
}

/*!
    \property QShortcut::context
    \brief the context in which the shortcut is valid

    A shortcut's context decides in which circumstances a shortcut is
    allowed to be triggered. The normal context is Qt::WindowShortcut,
    which allows the shortcut to trigger if the parent (the widget
    containing the shortcut) is a subwidget of the active top-level
    window.

    By default, this property is set to Qt::WindowShortcut.
*/
void QShortcut::setContext(Qt::ShortcutContext context)
{
    Q_D(QShortcut);
    if(d->sc_context == context)
        return;
    QAPP_CHECK("setContext");
    d->sc_context = context;
    d->redoGrab(QGuiApplicationPrivate::instance()->shortcutMap);
}

Qt::ShortcutContext QShortcut::context() const
{
    Q_D(const QShortcut);
    return d->sc_context;
}

/*!
    \property QShortcut::whatsThis
    \brief the shortcut's "What's This?" help text

    The text will be shown when the application is in "What's
    This?" mode and the user types the shortcut key() sequence.

    To set "What's This?" help on a menu item (with or without a
    shortcut key), set the help on the item's action.

    By default, this property contains an empty string.

    \sa QWhatsThis::inWhatsThisMode(), QAction::setWhatsThis()
*/
void QShortcut::setWhatsThis(const QString &text)
{
    Q_D(QShortcut);
    d->sc_whatsthis = text;
}

QString QShortcut::whatsThis() const
{
    Q_D(const QShortcut);
    return d->sc_whatsthis;
}

/*!
    \property QShortcut::autoRepeat
    \brief whether the shortcut can auto repeat
    \since 4.2

    If true, the shortcut will auto repeat when the keyboard shortcut
    combination is held down, provided that keyboard auto repeat is
    enabled on the system.
    The default value is true.
*/
void QShortcut::setAutoRepeat(bool on)
{
    Q_D(QShortcut);
    if (d->sc_autorepeat == on)
        return;
    QAPP_CHECK("setAutoRepeat");
    d->sc_autorepeat = on;
    QGuiApplicationPrivate::instance()->shortcutMap.setShortcutAutoRepeat(on, d->sc_id, this);
}

bool QShortcut::autoRepeat() const
{
    Q_D(const QShortcut);
    return d->sc_autorepeat;
}

/*!
    Returns the shortcut's ID.

    \sa QShortcutEvent::shortcutId()
*/
int QShortcut::id() const
{
    Q_D(const QShortcut);
    return d->sc_id;
}

/*!
    \internal
*/
bool QShortcut::event(QEvent *e)
{
    Q_D(QShortcut);
    if (d->sc_enabled && e->type() == QEvent::Shortcut) {
        auto se = static_cast<QShortcutEvent *>(e);
        if (se->shortcutId() == d->sc_id && se->key() == d->sc_sequence){
#if QT_CONFIG(whatsthis)
            if (QWhatsThis::inWhatsThisMode()) {
                QWhatsThis::showText(QCursor::pos(), d->sc_whatsthis);
            } else
#endif
            if (se->isAmbiguous())
                emit activatedAmbiguously();
            else
                emit activated();
            return true;
        }
    }
    return QObject::event(e);
}
#endif // QT_NO_SHORTCUT

QT_END_NAMESPACE

#include "moc_qshortcut.cpp"
