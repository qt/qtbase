// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qshortcut.h"
#include "private/qshortcut_p.h"

#include "private/qwidget_p.h"

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
#if QT_CONFIG(action)
#  include <private/qaction_p.h>
#endif
#include <private/qwidgetwindow_p.h>
#include <qpa/qplatformmenu.h>

QT_BEGIN_NAMESPACE

static bool correctWidgetContext(Qt::ShortcutContext context, QWidget *w, QWidget *active_window);
#if QT_CONFIG(graphicsview)
static bool correctGraphicsWidgetContext(Qt::ShortcutContext context, QGraphicsWidget *w, QWidget *active_window);
#endif
#if QT_CONFIG(action)
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

#if QT_CONFIG(action)
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
            w = qobject_cast<QWidget *>(s->parent());
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

#if QT_CONFIG(action)
static bool correctActionContext(Qt::ShortcutContext context, QAction *a, QWidget *active_window)
{
    const QObjectList associatedObjects = a->associatedObjects();
#if defined(DEBUG_QSHORTCUTMAP)
    if (associatedObjects.isEmpty())
        qDebug() << a << "not connected to any widgets; won't trigger";
#endif
    for (auto object : associatedObjects) {
#if QT_CONFIG(menu)
        if (auto menu = qobject_cast<QMenu *>(object)) {
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
            if (a->isVisible() && a->isEnabled() && correctActionContext(context, a, active_window))
                return true;
        } else
#endif
        if (auto widget = qobject_cast<QWidget*>(object)) {
            if (correctWidgetContext(context, widget, active_window))
                return true;
        }
#if QT_CONFIG(graphicsview)
        else if (auto graphicsWidget = qobject_cast<QGraphicsWidget*>(object)) {
            if (correctGraphicsWidgetContext(context, graphicsWidget, active_window))
                return true;
        }
#endif
    }

    return false;
}
#endif // QT_CONFIG(action)

class QtWidgetsShortcutPrivate : public QShortcutPrivate
{
    Q_DECLARE_PUBLIC(QShortcut)
public:
    QtWidgetsShortcutPrivate() = default;

    QShortcutMap::ContextMatcher contextMatcher() const override
    { return qWidgetShortcutContextMatcher; }

    bool handleWhatsThis() override;
};

bool QtWidgetsShortcutPrivate::handleWhatsThis()
{
#if QT_CONFIG(whatsthis)
    if (QWhatsThis::inWhatsThisMode()) {
        QWhatsThis::showText(QCursor::pos(), sc_whatsthis);
        return true;
    }
#endif
    return false;
}

QShortcutPrivate *QApplicationPrivate::createShortcutPrivate() const
{
    return new QtWidgetsShortcutPrivate;
}

QT_END_NAMESPACE
