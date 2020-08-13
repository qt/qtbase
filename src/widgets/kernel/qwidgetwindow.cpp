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

#include "private/qwindow_p.h"
#include "qwidgetwindow_p.h"
#include "qlayout.h"

#include "private/qwidget_p.h"
#include "private/qapplication_p.h"
#ifndef QT_NO_ACCESSIBILITY
#include <QtGui/qaccessible.h>
#endif
#include <private/qwidgetrepaintmanager_p.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformwindow.h>
#include <private/qgesturemanager_p.h>
#include <private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

Q_WIDGETS_EXPORT extern bool qt_tab_all_widgets();

Q_WIDGETS_EXPORT QWidget *qt_button_down = nullptr; // widget got last button-down

// popup control
QWidget *qt_popup_down = nullptr; // popup that contains the pressed widget
extern int openPopupCount;
bool qt_popup_down_closed = false; // qt_popup_down has been closed
bool qt_replay_popup_mouse_event = false;
extern bool qt_try_modal(QWidget *widget, QEvent::Type type);

class QWidgetWindowPrivate : public QWindowPrivate
{
    Q_DECLARE_PUBLIC(QWidgetWindow)
public:
    void setVisible(bool visible) override
    {
        Q_Q(QWidgetWindow);
        if (QWidget *widget = q->widget()) {
            // Check if the widget was already hidden, as this indicates it was done
            // explicitly and not because the parent window in this case made it hidden.
            // In which case do not automatically show the widget when the parent
            // window is shown.
            const bool wasHidden = widget->testAttribute(Qt::WA_WState_Hidden);
            QWidgetPrivate::get(widget)->setVisible(visible);
            if (!wasHidden)
                widget->setAttribute(Qt::WA_WState_ExplicitShowHide, false);
        } else {
            QWindowPrivate::setVisible(visible);
        }
    }

    QWindow *eventReceiver() override {
        Q_Q(QWidgetWindow);
        QWindow *w = q;
        while (w->parent() && qobject_cast<QWidgetWindow *>(w) && qobject_cast<QWidgetWindow *>(w->parent())) {
            w = w->parent();
        }
        return w;
    }

    void clearFocusObject() override
    {
        Q_Q(QWidgetWindow);
        QWidget *widget = q->widget();
        if (widget && widget->focusWidget())
            widget->focusWidget()->clearFocus();
    }

    QRectF closestAcceptableGeometry(const QRectF &rect) const override;
#if QT_CONFIG(opengl)
    QOpenGLContext *shareContext() const override;
#endif

    void processSafeAreaMarginsChanged() override
    {
        Q_Q(QWidgetWindow);
        if (QWidget *widget = q->widget())
            QWidgetPrivate::get(widget)->updateContentsRect();
    }
    bool allowClickThrough(const QPoint &) const override;
};

QRectF QWidgetWindowPrivate::closestAcceptableGeometry(const QRectF &rect) const
{
    Q_Q(const QWidgetWindow);
    const QWidget *widget = q->widget();
    if (!widget || !widget->isWindow() || !widget->hasHeightForWidth())
        return QRect();
    const QSize oldSize = rect.size().toSize();
    const QSize newSize = QLayout::closestAcceptableSize(widget, oldSize);
    if (newSize == oldSize)
        return QRectF();
    const int dw = newSize.width() - oldSize.width();
    const int dh = newSize.height() - oldSize.height();
    QRectF result = rect;
    const QRectF currentGeometry(widget->geometry());
    const qreal topOffset = result.top() - currentGeometry.top();
    const qreal bottomOffset = result.bottom() - currentGeometry.bottom();
    if (qAbs(topOffset) > qAbs(bottomOffset))
        result.setTop(result.top() - dh); // top edge drag
    else
        result.setBottom(result.bottom() + dh); // bottom edge drag
    const qreal leftOffset = result.left() - currentGeometry.left();
    const qreal rightOffset = result.right() - currentGeometry.right();
    if (qAbs(leftOffset) > qAbs(rightOffset))
        result.setLeft(result.left() - dw); // left edge drag
    else
        result.setRight(result.right() + dw); // right edge drag
    return result;
}

#if QT_CONFIG(opengl)
QOpenGLContext *QWidgetWindowPrivate::shareContext() const
{
    Q_Q(const QWidgetWindow);
    const QWidgetPrivate *widgetPrivate = QWidgetPrivate::get(q->widget());
    return widgetPrivate->shareContext();
}
#endif // opengl

QWidgetWindow::QWidgetWindow(QWidget *widget)
    : QWindow(*new QWidgetWindowPrivate(), nullptr)
    , m_widget(widget)
{
    updateObjectName();
    // Enable QOpenGLWidget/QQuickWidget children if the platform plugin supports it,
    // and the application developer has not explicitly disabled it.
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RasterGLSurface)
        && !QCoreApplication::testAttribute(Qt::AA_ForceRasterWidgets)) {
        setSurfaceType(QSurface::RasterGLSurface);
    }
    connect(widget, &QObject::objectNameChanged, this, &QWidgetWindow::updateObjectName);
    connect(this, SIGNAL(screenChanged(QScreen*)), this, SLOT(handleScreenChange()));
}

QWidgetWindow::~QWidgetWindow()
{
}

#ifndef QT_NO_ACCESSIBILITY
QAccessibleInterface *QWidgetWindow::accessibleRoot() const
{
    if (m_widget)
        return QAccessible::queryAccessibleInterface(m_widget);
    return nullptr;
}
#endif

QObject *QWidgetWindow::focusObject() const
{
    QWidget *windowWidget = m_widget;
    if (!windowWidget)
        return nullptr;

    // A window can't have a focus object if it's being destroyed.
    if (QWidgetPrivate::get(windowWidget)->data.in_destructor)
        return nullptr;

    QWidget *widget = windowWidget->focusWidget();

    if (!widget)
        widget = windowWidget;

    QObject *focusObj = QWidgetPrivate::get(widget)->focusObject();
    if (focusObj)
        return focusObj;

    return widget;
}

void QWidgetWindow::setNativeWindowVisibility(bool visible)
{
    Q_D(QWidgetWindow);
    // Call base class setVisible() implementation to run the QWindow
    // visibility logic. Don't call QWidgetWindowPrivate::setVisible()
    // since that will recurse back into QWidget code.
    d->QWindowPrivate::setVisible(visible);
}

static inline bool shouldBePropagatedToWidget(QEvent *event)
{
    switch (event->type()) {
    // Handing show events to widgets would cause them to be triggered twice
    case QEvent::Show:
    case QEvent::Hide:
    case QEvent::Timer:
    case QEvent::DynamicPropertyChange:
    case QEvent::ChildAdded:
    case QEvent::ChildRemoved:
        return false;
    default:
        return true;
    }
}

bool QWidgetWindowPrivate::allowClickThrough(const QPoint &) const
{
    return true;
}

bool QWidgetWindow::event(QEvent *event)
{
    if (!m_widget)
        return QWindow::event(event);

    if (m_widget->testAttribute(Qt::WA_DontShowOnScreen)) {
        // \a event is uninteresting for QWidgetWindow, the event was probably
        // generated before WA_DontShowOnScreen was set
        if (!shouldBePropagatedToWidget(event))
            return true;
        return QCoreApplication::forwardEvent(m_widget, event);
    }

    switch (event->type()) {
    case QEvent::Close: {
        // The widget might be deleted in the close event handler.
        QPointer<QObject> guard = this;
        handleCloseEvent(static_cast<QCloseEvent *>(event));
        if (guard)
            QWindow::event(event);
        return true;
    }

    case QEvent::Enter:
    case QEvent::Leave:
        handleEnterLeaveEvent(event);
        return true;

    // these should not be sent to QWidget, the corresponding events
    // are sent by QApplicationPrivate::notifyActiveWindowChange()
    case QEvent::FocusIn:
        handleFocusInEvent(static_cast<QFocusEvent *>(event));
        Q_FALLTHROUGH();
    case QEvent::FocusOut: {
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent ev(m_widget, state);
        QAccessible::updateAccessibility(&ev);
#endif
        return false; }

    case QEvent::FocusAboutToChange:
        if (QApplicationPrivate::focus_widget) {
            if (QApplicationPrivate::focus_widget->testAttribute(Qt::WA_InputMethodEnabled))
                QGuiApplication::inputMethod()->commit();

            QGuiApplication::forwardEvent(QApplicationPrivate::focus_widget, event);
        }
        return true;

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        handleKeyEvent(static_cast<QKeyEvent *>(event));
        return true;

    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
        handleMouseEvent(static_cast<QMouseEvent *>(event));
        return true;

    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
        handleNonClientAreaMouseEvent(static_cast<QMouseEvent *>(event));
        return true;

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        handleTouchEvent(static_cast<QTouchEvent *>(event));
        return true;

    case QEvent::Move:
        handleMoveEvent(static_cast<QMoveEvent *>(event));
        return true;

    case QEvent::Resize:
        handleResizeEvent(static_cast<QResizeEvent *>(event));
        return true;

#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
        handleWheelEvent(static_cast<QWheelEvent *>(event));
        return true;
#endif

#if QT_CONFIG(draganddrop)
    case QEvent::DragEnter:
        handleDragEnterEvent(static_cast<QDragEnterEvent *>(event));
        return true;
    case QEvent::DragMove:
        handleDragMoveEvent(static_cast<QDragMoveEvent *>(event));
        return true;
    case QEvent::DragLeave:
        handleDragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
        return true;
    case QEvent::Drop:
        handleDropEvent(static_cast<QDropEvent *>(event));
        return true;
#endif

    case QEvent::Expose:
        handleExposeEvent(static_cast<QExposeEvent *>(event));
        return true;

    case QEvent::WindowStateChange:
        QWindow::event(event); // Update QWindow::Visibility and emit signals.
        handleWindowStateChangedEvent(static_cast<QWindowStateChangeEvent *>(event));
        return true;

    case QEvent::ThemeChange: {
        QEvent widgetEvent(QEvent::ThemeChange);
        QCoreApplication::forwardEvent(m_widget, &widgetEvent, event);
    }
        return true;

#if QT_CONFIG(tabletevent)
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        handleTabletEvent(static_cast<QTabletEvent *>(event));
        return true;
#endif

#ifndef QT_NO_GESTURES
    case QEvent::NativeGesture:
        handleGestureEvent(static_cast<QNativeGestureEvent *>(event));
        return true;
#endif

#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        handleContextMenuEvent(static_cast<QContextMenuEvent *>(event));
        return true;
#endif // QT_NO_CONTEXTMENU

    case QEvent::WindowBlocked:
        qt_button_down = nullptr;
        break;

    case QEvent::UpdateRequest:
        // This is not the same as an UpdateRequest for a QWidget. That just
        // syncs the backing store while here we also must mark as dirty.
        m_widget->repaint();
        return true;

    default:
        break;
    }

    if (shouldBePropagatedToWidget(event) && QCoreApplication::forwardEvent(m_widget, event))
        return true;

    return QWindow::event(event);
}

QPointer<QWidget> qt_last_mouse_receiver = nullptr;

void QWidgetWindow::handleEnterLeaveEvent(QEvent *event)
{
#if !defined(Q_OS_MACOS) && !defined(Q_OS_IOS) // Cocoa tracks popups
    // Ignore all enter/leave events from QPA if we are not on the first-level context menu.
    // This prevents duplicated events on most platforms. Fake events will be delivered in
    // QWidgetWindow::handleMouseEvent(QMouseEvent *). Make an exception whether the widget
    // is already under mouse - let the mouse leave.
    if (QApplicationPrivate::inPopupMode() && m_widget != QApplication::activePopupWidget() && !m_widget->underMouse())
        return;
#endif
    if (event->type() == QEvent::Leave) {
        QWidget *enter = nullptr;
        // Check from window system event queue if the next queued enter targets a window
        // in the same window hierarchy (e.g. enter a child of this window). If so,
        // remove the enter event from queue and handle both in single dispatch.
        QWindowSystemInterfacePrivate::EnterEvent *systemEvent =
            static_cast<QWindowSystemInterfacePrivate::EnterEvent *>
            (QWindowSystemInterfacePrivate::peekWindowSystemEvent(QWindowSystemInterfacePrivate::Enter));
        const QPointF globalPosF = systemEvent ? systemEvent->globalPos : QGuiApplicationPrivate::lastCursorPosition;
        if (systemEvent) {
            if (QWidgetWindow *enterWindow = qobject_cast<QWidgetWindow *>(systemEvent->enter))
            {
                QWindow *thisParent = this;
                QWindow *enterParent = enterWindow;
                while (thisParent->parent())
                    thisParent = thisParent->parent();
                while (enterParent->parent())
                    enterParent = enterParent->parent();
                if (thisParent == enterParent) {
                    QGuiApplicationPrivate::currentMouseWindow = enterWindow;
                    enter = enterWindow->widget();
                    QWindowSystemInterfacePrivate::removeWindowSystemEvent(systemEvent);
                }
            }
        }
        // Enter-leave between sibling widgets is ignored when there is a mousegrabber - this makes
        // both native and non-native widgets work similarly.
        // When mousegrabbing, leaves are only generated if leaving the parent window.
        if (!enter || !QWidget::mouseGrabber()) {
            // Preferred leave target is the last mouse receiver, unless it has native window,
            // in which case it is assumed to receive it's own leave event when relevant.
            QWidget *leave = m_widget;
            if (qt_last_mouse_receiver && !qt_last_mouse_receiver->internalWinId())
                leave = qt_last_mouse_receiver.data();
            QApplicationPrivate::dispatchEnterLeave(enter, leave, globalPosF);
            qt_last_mouse_receiver = enter;
        }
    } else {
        const QEnterEvent *ee = static_cast<QEnterEvent *>(event);
        QWidget *child = m_widget->childAt(ee->pos());
        QWidget *receiver = child ? child : m_widget.data();
        QWidget *leave = nullptr;
        if (QApplicationPrivate::inPopupMode() && receiver == m_widget
                && qt_last_mouse_receiver != m_widget) {
            // This allows to deliver the leave event to the native widget
            // action on first-level menu.
            leave = qt_last_mouse_receiver;
        }
        QApplicationPrivate::dispatchEnterLeave(receiver, leave, ee->screenPos());
        qt_last_mouse_receiver = receiver;
    }
}

QWidget *QWidgetWindow::getFocusWidget(FocusWidgets fw)
{
    QWidget *tlw = m_widget;
    QWidget *w = tlw->nextInFocusChain();

    QWidget *last = tlw;

    uint focus_flag = qt_tab_all_widgets() ? Qt::TabFocus : Qt::StrongFocus;

    while (w != tlw)
    {
        if (((w->focusPolicy() & focus_flag) == focus_flag)
            && w->isVisibleTo(m_widget) && w->isEnabled())
        {
            last = w;
            if (fw == FirstFocusWidget)
                break;
        }
        w = w->nextInFocusChain();
    }

    return last;
}

void QWidgetWindow::handleFocusInEvent(QFocusEvent *e)
{
    QWidget *focusWidget = nullptr;
    if (e->reason() == Qt::BacktabFocusReason)
        focusWidget = getFocusWidget(LastFocusWidget);
    else if (e->reason() == Qt::TabFocusReason)
        focusWidget = getFocusWidget(FirstFocusWidget);

    if (focusWidget != nullptr)
        focusWidget->setFocus();
}

void QWidgetWindow::handleNonClientAreaMouseEvent(QMouseEvent *e)
{
    QApplication::forwardEvent(m_widget, e);
}

void QWidgetWindow::handleMouseEvent(QMouseEvent *event)
{
    static const QEvent::Type contextMenuTrigger =
        QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::ContextMenuOnMouseRelease).toBool() ?
        QEvent::MouseButtonRelease : QEvent::MouseButtonPress;
    if (QApplicationPrivate::inPopupMode()) {
        QPointer<QWidget> activePopupWidget = QApplication::activePopupWidget();
        QPoint mapped = event->pos();
        if (activePopupWidget != m_widget)
            mapped = activePopupWidget->mapFromGlobal(event->globalPos());
        bool releaseAfter = false;
        QWidget *popupChild  = activePopupWidget->childAt(mapped);

        if (activePopupWidget != qt_popup_down) {
            qt_button_down = nullptr;
            qt_popup_down = nullptr;
        }

        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            qt_button_down = popupChild;
            qt_popup_down = activePopupWidget;
            qt_popup_down_closed = false;
            break;
        case QEvent::MouseButtonRelease:
            releaseAfter = true;
            break;
        default:
            break; // nothing for mouse move
        }

        int oldOpenPopupCount = openPopupCount;

        if (activePopupWidget->isEnabled()) {
            // deliver event
            qt_replay_popup_mouse_event = false;
            QPointer<QWidget> receiver = activePopupWidget;
            QPoint widgetPos = mapped;
            if (qt_button_down)
                receiver = qt_button_down;
            else if (popupChild)
                receiver = popupChild;
            if (receiver != activePopupWidget)
                widgetPos = receiver->mapFromGlobal(event->globalPos());

#if !defined(Q_OS_MACOS) && !defined(Q_OS_IOS) // Cocoa tracks popups
            const bool reallyUnderMouse = activePopupWidget->rect().contains(mapped);
            const bool underMouse = activePopupWidget->underMouse();
            if (underMouse != reallyUnderMouse) {
                if (reallyUnderMouse) {
                    const QPoint receiverMapped = receiver->mapFromGlobal(event->screenPos().toPoint());
                    // Prevent negative mouse position on enter event - this event
                    // should be properly handled in "handleEnterLeaveEvent()".
                    if (receiverMapped.x() >= 0 && receiverMapped.y() >= 0) {
                        QApplicationPrivate::dispatchEnterLeave(receiver, nullptr, event->screenPos());
                        qt_last_mouse_receiver = receiver;
                    }
                } else {
                    QApplicationPrivate::dispatchEnterLeave(nullptr, qt_last_mouse_receiver, event->screenPos());
                    qt_last_mouse_receiver = receiver;
                    receiver = activePopupWidget;
                }
            }
#endif
            if ((event->type() != QEvent::MouseButtonPress)
                    || !(event->flags().testFlag(Qt::MouseEventCreatedDoubleClick))) {
                // if the widget that was pressed is gone, then deliver move events without buttons
                const auto buttons = event->type() == QEvent::MouseMove && qt_popup_down_closed
                                   ? Qt::NoButton : event->buttons();
                QMouseEvent e(event->type(), widgetPos, event->windowPos(), event->screenPos(),
                              event->button(), buttons, event->modifiers(), event->source());
                e.setTimestamp(event->timestamp());
                QApplicationPrivate::sendMouseEvent(receiver, &e, receiver, receiver->window(), &qt_button_down, qt_last_mouse_receiver);
                qt_last_mouse_receiver = receiver;
            }
        } else {
            // close disabled popups when a mouse button is pressed or released
            switch (event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseButtonRelease:
                activePopupWidget->close();
                break;
            default:
                break;
            }
        }

        if (QApplication::activePopupWidget() != activePopupWidget
            && qt_replay_popup_mouse_event
            && QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::ReplayMousePressOutsidePopup).toBool()) {
            if (m_widget->windowType() != Qt::Popup)
                qt_button_down = nullptr;
            if (event->type() == QEvent::MouseButtonPress) {
                // the popup disappeared, replay the mouse press event
                QWidget *w = QApplication::widgetAt(event->globalPos());
                if (w && !QApplicationPrivate::isBlockedByModal(w)) {
                    // activate window of the widget under mouse pointer
                    if (!w->isActiveWindow()) {
                        w->activateWindow();
                        w->window()->raise();
                    }

                    if (auto win = qt_widget_private(w)->windowHandle(QWidgetPrivate::WindowHandleMode::Closest)) {
                        const QRect globalGeometry = win->isTopLevel()
                            ? win->geometry()
                            : QRect(win->mapToGlobal(QPoint(0, 0)), win->size());
                        if (globalGeometry.contains(event->globalPos())) {
                            // Use postEvent() to ensure the local QEventLoop terminates when called from QMenu::exec()
                            const QPoint localPos = win->mapFromGlobal(event->globalPos());
                            QMouseEvent *e = new QMouseEvent(QEvent::MouseButtonPress, localPos, localPos, event->globalPos(),
                                                             event->button(), event->buttons(), event->modifiers(), event->source());
                            QCoreApplicationPrivate::setEventSpontaneous(e, true);
                            e->setTimestamp(event->timestamp());
                            QCoreApplication::postEvent(win, e);
                        }
                    }
                }
            }
            qt_replay_popup_mouse_event = false;
#ifndef QT_NO_CONTEXTMENU
        } else if (event->type() == contextMenuTrigger
                   && event->button() == Qt::RightButton
                   && (openPopupCount == oldOpenPopupCount)) {
            QWidget *receiver = activePopupWidget;
            if (qt_button_down)
                receiver = qt_button_down;
            else if(popupChild)
                receiver = popupChild;
            QContextMenuEvent e(QContextMenuEvent::Mouse, mapped, event->globalPos(), event->modifiers());
            QApplication::forwardEvent(receiver, &e, event);
        }
#else
            Q_UNUSED(contextMenuTrigger)
            Q_UNUSED(oldOpenPopupCount)
        }
#endif

        if (releaseAfter) {
            qt_button_down = nullptr;
            qt_popup_down_closed = false;
            qt_popup_down = nullptr;
        }
        return;
    }

    qt_popup_down_closed = false;
    // modal event handling
    if (QApplicationPrivate::instance()->modalState() && !qt_try_modal(m_widget, event->type()))
        return;

    // which child should have it?
    QWidget *widget = m_widget->childAt(event->pos());
    QPoint mapped = event->pos();

    if (!widget)
        widget = m_widget;

    const bool initialPress = event->buttons() == event->button();
    if (event->type() == QEvent::MouseButtonPress && initialPress)
        qt_button_down = widget;

    QWidget *receiver = QApplicationPrivate::pickMouseReceiver(m_widget, event->windowPos().toPoint(), &mapped, event->type(), event->buttons(),
                                                               qt_button_down, widget);
    if (!receiver)
        return;

    if ((event->type() != QEvent::MouseButtonPress)
        || !(event->flags().testFlag(Qt::MouseEventCreatedDoubleClick))) {

        // The preceding statement excludes MouseButtonPress events which caused
        // creation of a MouseButtonDblClick event. QTBUG-25831
        QMouseEvent translated(event->type(), mapped, event->windowPos(), event->screenPos(),
                               event->button(), event->buttons(), event->modifiers(), event->source());
        translated.setTimestamp(event->timestamp());
        QApplicationPrivate::sendMouseEvent(receiver, &translated, widget, m_widget,
                                            &qt_button_down, qt_last_mouse_receiver);
        event->setAccepted(translated.isAccepted());
    }
#ifndef QT_NO_CONTEXTMENU
    if (event->type() == contextMenuTrigger && event->button() == Qt::RightButton
        && m_widget->rect().contains(event->pos())) {
        QContextMenuEvent e(QContextMenuEvent::Mouse, mapped, event->globalPos(), event->modifiers());
        QGuiApplication::forwardEvent(receiver, &e, event);
    }
#endif
}

void QWidgetWindow::handleTouchEvent(QTouchEvent *event)
{
    if (event->type() == QEvent::TouchCancel) {
        QApplicationPrivate::translateTouchCancel(event->device(), event->timestamp());
        event->accept();
    } else if (QApplicationPrivate::inPopupMode()) {
        // Ignore touch events for popups. This will cause QGuiApplication to synthesise mouse
        // events instead, which QWidgetWindow::handleMouseEvent will forward correctly:
        event->ignore();
    } else {
        event->setAccepted(QApplicationPrivate::translateRawTouchEvent(m_widget, event->device(), event->touchPoints(), event->timestamp()));
    }
}

void QWidgetWindow::handleKeyEvent(QKeyEvent *event)
{
    if (QApplicationPrivate::instance()->modalState() && !qt_try_modal(m_widget, event->type()))
        return;

    QObject *receiver = QWidget::keyboardGrabber();
    if (!receiver && QApplicationPrivate::inPopupMode()) {
        QWidget *popup = QApplication::activePopupWidget();
        QWidget *popupFocusWidget = popup->focusWidget();
        receiver = popupFocusWidget ? popupFocusWidget : popup;
    }
    if (!receiver)
        receiver = focusObject();
    QGuiApplication::forwardEvent(receiver, event);
}

bool QWidgetWindow::updateSize()
{
    bool changed = false;
    if (m_widget->testAttribute(Qt::WA_OutsideWSRange))
        return changed;
    if (m_widget->data->crect.size() != geometry().size()) {
        changed = true;
        m_widget->data->crect.setSize(geometry().size());
    }

    updateMargins();
    return changed;
}

void QWidgetWindow::updateMargins()
{
    const QMargins margins = frameMargins();
    QTLWExtra *te = m_widget->d_func()->topData();
    te->posIncludesFrame= false;
    te->frameStrut.setCoords(margins.left(), margins.top(), margins.right(), margins.bottom());
    m_widget->data->fstrut_dirty = false;
}

static void sendScreenChangeRecursively(QWidget *widget)
{
    QEvent e(QEvent::ScreenChangeInternal);
    QCoreApplication::sendEvent(widget, &e);
    QWidgetPrivate *d = QWidgetPrivate::get(widget);
    for (int i = 0; i < d->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
        if (w)
            sendScreenChangeRecursively(w);
    }
}

void QWidgetWindow::handleScreenChange()
{
    // Send an event recursively to the widget and its children.
    sendScreenChangeRecursively(m_widget);

    // Invalidate the backing store buffer and repaint immediately.
    if (screen())
        repaintWindow();
}

void QWidgetWindow::repaintWindow()
{
    if (!m_widget->isVisible() || !m_widget->updatesEnabled() || !m_widget->rect().isValid())
        return;

    QTLWExtra *tlwExtra = m_widget->window()->d_func()->maybeTopData();
    if (tlwExtra && tlwExtra->backingStore)
        tlwExtra->repaintManager->markDirty(m_widget->rect(), m_widget,
                                                 QWidgetRepaintManager::UpdateNow, QWidgetRepaintManager::BufferInvalid);
}

// Store normal geometry used for saving application settings.
void QWidgetWindow::updateNormalGeometry()
{
    QTLWExtra *tle = m_widget->d_func()->maybeTopData();
    if (!tle)
        return;
     // Ask platform window, default to widget geometry.
    QRect normalGeometry;
    if (const QPlatformWindow *pw = handle())
        normalGeometry = QHighDpi::fromNativePixels(pw->normalGeometry(), this);
    if (!normalGeometry.isValid() && !(m_widget->windowState() & ~Qt::WindowActive))
        normalGeometry = m_widget->geometry();
    if (normalGeometry.isValid())
        tle->normalGeometry = normalGeometry;
}

void QWidgetWindow::handleMoveEvent(QMoveEvent *event)
{
    if (m_widget->testAttribute(Qt::WA_OutsideWSRange))
        return;

    auto oldPosition = m_widget->data->crect.topLeft();
    auto newPosition = geometry().topLeft();

    if (!m_widget->isTopLevel()) {
        if (auto *nativeParent = m_widget->nativeParentWidget())
            newPosition = m_widget->parentWidget()->mapFrom(nativeParent, newPosition);
    }

    bool changed = newPosition != oldPosition;

    if (changed)
        m_widget->data->crect.moveTopLeft(newPosition);

    updateMargins(); // FIXME: Only do when changed?

    if (changed) {
        QMoveEvent widgetEvent(newPosition, oldPosition);
        QGuiApplication::forwardEvent(m_widget, &widgetEvent, event);
    }
}

void QWidgetWindow::handleResizeEvent(QResizeEvent *event)
{
    auto oldRect = m_widget->rect();

    if (updateSize()) {
        QGuiApplication::forwardEvent(m_widget, event);

        if (m_widget->d_func()->shouldPaintOnScreen()) {
            QRegion dirtyRegion = m_widget->rect();
            if (m_widget->testAttribute(Qt::WA_StaticContents))
                dirtyRegion -= oldRect;
            m_widget->d_func()->syncBackingStore(dirtyRegion);
        } else {
            m_widget->d_func()->syncBackingStore();
        }
    }
}

void QWidgetWindow::handleCloseEvent(QCloseEvent *event)
{
    bool is_closing = m_widget->d_func()->close_helper(QWidgetPrivate::CloseWithSpontaneousEvent);
    event->setAccepted(is_closing);
}

#if QT_CONFIG(wheelevent)

void QWidgetWindow::handleWheelEvent(QWheelEvent *event)
{
    if (QApplicationPrivate::instance()->modalState() && !qt_try_modal(m_widget, event->type()))
        return;

    QWidget *rootWidget = m_widget;
    QPoint pos = event->position().toPoint();

    // Use proper popup window for wheel event. Some QPA sends the wheel
    // event to the root menu, so redirect it to the proper popup window.
    QWidget *activePopupWidget = QApplication::activePopupWidget();
    if (activePopupWidget && activePopupWidget != m_widget) {
        rootWidget = activePopupWidget;
        pos = rootWidget->mapFromGlobal(event->globalPosition().toPoint());
    }

    // which child should have it?
    QWidget *widget = rootWidget->childAt(pos);

    if (!widget)
        widget = rootWidget;

    QPoint mapped = widget->mapFrom(rootWidget, pos);

#if QT_DEPRECATED_SINCE(5, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QWheelEvent translated(mapped, event->globalPos(), event->pixelDelta(), event->angleDelta(), event->delta(), event->orientation(), event->buttons(), event->modifiers(), event->phase(), event->source(), event->inverted());
QT_WARNING_POP
#else
    QWheelEvent translated(QPointF(mapped), event->globalPosition(), event->pixelDelta(), event->angleDelta(),
                           event->buttons(), event->modifiers(), event->phase(), event->inverted(), event->source());
#endif
    translated.setTimestamp(event->timestamp());
    QGuiApplication::forwardEvent(widget, &translated, event);
}

#endif // QT_CONFIG(wheelevent)

#if QT_CONFIG(draganddrop)

static QWidget *findDnDTarget(QWidget *parent, const QPoint &pos)
{
    // Find a target widget under mouse that accepts drops (QTBUG-22987).
    QWidget *widget = parent->childAt(pos);
    if (!widget)
        widget = parent;
    for ( ; widget && !widget->isWindow() && !widget->acceptDrops(); widget = widget->parentWidget()) ;
    if (widget && !widget->acceptDrops())
        widget = nullptr;
    return widget;
}

void QWidgetWindow::handleDragEnterEvent(QDragEnterEvent *event, QWidget *widget)
{
    Q_ASSERT(m_dragTarget == nullptr);
    if (!widget)
        widget = findDnDTarget(m_widget, event->pos());
    if (!widget) {
        event->ignore();
        return;
    }
    m_dragTarget = widget;

    const QPoint mapped = widget->mapFromGlobal(m_widget->mapToGlobal(event->pos()));
    QDragEnterEvent translated(mapped, event->possibleActions(), event->mimeData(),
                               event->mouseButtons(), event->keyboardModifiers());
    QGuiApplication::forwardEvent(m_dragTarget, &translated, event);
    event->setAccepted(translated.isAccepted());
    event->setDropAction(translated.dropAction());
}

void QWidgetWindow::handleDragMoveEvent(QDragMoveEvent *event)
{
    QPointer<QWidget> widget = findDnDTarget(m_widget, event->pos());
    if (!widget) {
        event->ignore();
        if (m_dragTarget) { // Send DragLeave to previous
            QDragLeaveEvent leaveEvent;
            QGuiApplication::forwardEvent(m_dragTarget, &leaveEvent, event);
            m_dragTarget = nullptr;
        }
    } else {
        const QPoint mapped = widget->mapFromGlobal(m_widget->mapToGlobal(event->pos()));
        QDragMoveEvent translated(mapped, event->possibleActions(), event->mimeData(),
                                  event->mouseButtons(), event->keyboardModifiers());

        if (widget == m_dragTarget) { // Target widget unchanged: Send DragMove
            translated.setDropAction(event->dropAction());
            translated.setAccepted(event->isAccepted());
            QGuiApplication::forwardEvent(m_dragTarget, &translated, event);
        } else {
            if (m_dragTarget) { // Send DragLeave to previous
                QDragLeaveEvent leaveEvent;
                QGuiApplication::forwardEvent(m_dragTarget, &leaveEvent, event);
                m_dragTarget = nullptr;
            }
            // widget might have been deleted when handling the leaveEvent
            if (widget) {
                // Send DragEnter to new widget.
                handleDragEnterEvent(static_cast<QDragEnterEvent*>(event), widget);
                // Handling 'DragEnter' should suffice for the application.
                translated.setDropAction(event->dropAction());
                translated.setAccepted(event->isAccepted());
                // The drag enter event is always immediately followed by a drag move event,
                // see QDragEnterEvent documentation.
                if (m_dragTarget)
                    QGuiApplication::forwardEvent(m_dragTarget, &translated, event);
            }
        }
        event->setAccepted(translated.isAccepted());
        event->setDropAction(translated.dropAction());
    }
}

void QWidgetWindow::handleDragLeaveEvent(QDragLeaveEvent *event)
{
    if (m_dragTarget)
        QGuiApplication::forwardEvent(m_dragTarget, event);
    m_dragTarget = nullptr;
}

void QWidgetWindow::handleDropEvent(QDropEvent *event)
{
    if (Q_UNLIKELY(m_dragTarget.isNull())) {
        qWarning() << m_widget << ": No drag target set.";
        event->ignore();
        return;
    }
    const QPoint mapped = m_dragTarget->mapFromGlobal(m_widget->mapToGlobal(event->pos()));
    QDropEvent translated(mapped, event->possibleActions(), event->mimeData(), event->mouseButtons(), event->keyboardModifiers());
    QGuiApplication::forwardEvent(m_dragTarget, &translated, event);
    event->setAccepted(translated.isAccepted());
    event->setDropAction(translated.dropAction());
    m_dragTarget = nullptr;
}

#endif // QT_CONFIG(draganddrop)

void QWidgetWindow::handleExposeEvent(QExposeEvent *event)
{
    QWidgetPrivate *wPriv = m_widget->d_func();
    const bool exposed = isExposed();

    if (wPriv->childrenHiddenByWState) {
        // If widgets has been previously hidden by window state change event
        // and they aren't yet shown...
        if (exposed) {
            // If the window becomes exposed...
            if (!wPriv->childrenShownByExpose) {
                // ... and they haven't been shown by this function yet - show it.
                wPriv->showChildren(true);
                QShowEvent showEvent;
                QCoreApplication::forwardEvent(m_widget, &showEvent, event);
                wPriv->childrenShownByExpose = true;
            }
        } else {
            // If the window becomes not exposed...
            if (wPriv->childrenShownByExpose) {
                // ... and child widgets was previously shown by the expose event - hide widgets again.
                // This is a workaround, because sometimes when window is minimized programatically,
                // the QPA can notify that the window is exposed after changing window state to minimized
                // and then, the QPA can send next expose event with null exposed region (not exposed).
                wPriv->hideChildren(true);
                QHideEvent hideEvent;
                QCoreApplication::forwardEvent(m_widget, &hideEvent, event);
                wPriv->childrenShownByExpose = false;
            }
        }
    }

    if (exposed) {
        // QTBUG-39220, QTBUG-58575: set all (potentially fully obscured parent widgets) mapped.
        m_widget->setAttribute(Qt::WA_Mapped);
        for (QWidget *p = m_widget->parentWidget(); p && !p->testAttribute(Qt::WA_Mapped); p = p->parentWidget())
            p->setAttribute(Qt::WA_Mapped);
        if (!event->region().isNull())
            wPriv->syncBackingStore(event->region());
    } else {
        m_widget->setAttribute(Qt::WA_Mapped, false);
    }
}

void QWidgetWindow::handleWindowStateChangedEvent(QWindowStateChangeEvent *event)
{
    // QWindow does currently not know 'active'.
    Qt::WindowStates eventState = event->oldState();
    Qt::WindowStates widgetState = m_widget->windowState();
    Qt::WindowStates windowState = windowStates();
    if (widgetState & Qt::WindowActive)
        eventState |= Qt::WindowActive;

    // Determine the new widget state, remember maximized/full screen
    // during minimized.
    if (windowState & Qt::WindowMinimized) {
        widgetState |= Qt::WindowMinimized;
    } else {
        widgetState = windowState | (widgetState & Qt::WindowActive);
        if (windowState) // Maximized or FullScreen
            updateNormalGeometry();
    }

    // Sent event if the state changed (that is, it is not triggered by
    // QWidget::setWindowState(), which also sends an event to the widget).
    if (widgetState != Qt::WindowStates::Int(m_widget->data->window_state)) {
        m_widget->data->window_state = uint(widgetState);
        QWindowStateChangeEvent widgetEvent(eventState);
        QGuiApplication::forwardEvent(m_widget, &widgetEvent, event);
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool QWidgetWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#else
bool QWidgetWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
    return m_widget->nativeEvent(eventType, message, result);
}

#if QT_CONFIG(tabletevent)
void QWidgetWindow::handleTabletEvent(QTabletEvent *event)
{
    static QPointer<QWidget> qt_tablet_target = nullptr;

    QWidget *widget = qt_tablet_target;

    if (!widget) {
        widget = m_widget->childAt(event->pos());
        if (event->type() == QEvent::TabletPress) {
            if (!widget)
                widget = m_widget;
            qt_tablet_target = widget;
        }
    }

    if (widget) {
        QPointF delta = event->globalPosF() - event->globalPos();
        QPointF mapped = widget->mapFromGlobal(event->globalPos()) + delta;
        QTabletEvent ev(event->type(), mapped, event->globalPosF(), event->deviceType(), event->pointerType(),
                        event->pressure(), event->xTilt(), event->yTilt(), event->tangentialPressure(),
                        event->rotation(), event->z(), event->modifiers(), event->uniqueId(), event->button(), event->buttons());
        ev.setTimestamp(event->timestamp());
        ev.setAccepted(false);
        QGuiApplication::forwardEvent(widget, &ev, event);
        event->setAccepted(ev.isAccepted());
    }

    if (event->type() == QEvent::TabletRelease && event->buttons() == Qt::NoButton)
        qt_tablet_target = nullptr;
}
#endif // QT_CONFIG(tabletevent)

#ifndef QT_NO_GESTURES
void QWidgetWindow::handleGestureEvent(QNativeGestureEvent *e)
{
    // copy-pasted code to find correct widget follows:
    QObject *receiver = nullptr;
    if (QApplicationPrivate::inPopupMode()) {
        QWidget *popup = QApplication::activePopupWidget();
        QWidget *popupFocusWidget = popup->focusWidget();
        receiver = popupFocusWidget ? popupFocusWidget : popup;
    }
    if (!receiver)
        receiver = QApplication::widgetAt(e->globalPos());
    if (!receiver)
        receiver = m_widget; // last resort

    QApplication::forwardEvent(receiver, e);
}
#endif // QT_NO_GESTURES

#ifndef QT_NO_CONTEXTMENU
void QWidgetWindow::handleContextMenuEvent(QContextMenuEvent *e)
{
    // We are only interested in keyboard originating context menu events here,
    // mouse originated context menu events for widgets are generated in mouse handling methods.
    if (e->reason() != QContextMenuEvent::Keyboard)
        return;

    QWidget *fw = QWidget::keyboardGrabber();
    if (!fw) {
        if (QApplication::activePopupWidget()) {
            fw = (QApplication::activePopupWidget()->focusWidget()
                  ? QApplication::activePopupWidget()->focusWidget()
                  : QApplication::activePopupWidget());
        } else if (QApplication::focusWidget()) {
            fw = QApplication::focusWidget();
        } else {
            fw = m_widget;
        }
    }
    if (fw && fw->isEnabled()) {
        QPoint pos = fw->inputMethodQuery(Qt::ImCursorRectangle).toRect().center();
        QContextMenuEvent widgetEvent(QContextMenuEvent::Keyboard, pos, fw->mapToGlobal(pos),
                                      e->modifiers());
        QGuiApplication::forwardEvent(fw, &widgetEvent, e);
    }
}
#endif // QT_NO_CONTEXTMENU

void QWidgetWindow::updateObjectName()
{
    QString name = m_widget->objectName();
    if (name.isEmpty())
        name = QString::fromUtf8(m_widget->metaObject()->className()) + QLatin1String("Class");
    name += QLatin1String("Window");
    setObjectName(name);
}

QT_END_NAMESPACE

#include "moc_qwidgetwindow_p.cpp"
