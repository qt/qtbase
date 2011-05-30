/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qapplication_p.h"
#include "qcolormap.h"
#include "qpixmapcache.h"
#if !defined(QT_NO_GLIB)
#include "private/qeventdispatcher_glib_qpa_p.h"
#endif
#include "private/qeventdispatcher_qpa_p.h"
#ifndef QT_NO_CURSOR
#include "private/qcursor_p.h"
#endif

#include "private/qwidget_p.h"
#include "private/qevent_p.h"

#include "qgenericpluginfactory_qpa.h"
#include "private/qplatformintegrationfactory_qpa_p.h"
#include <qdesktopwidget.h>

#include <qinputcontext.h>
#include <QPlatformCursor>
#include <qdebug.h>
#include <QWindowSystemInterface>
#include "private/qwindowsysteminterface_qpa_p.h"
#include <QPlatformIntegration>

#include "qdesktopwidget_qpa_p.h"
#include "qwidgetwindow_qpa_p.h"

QT_BEGIN_NAMESPACE

static QString appName;
static QString appFont;
extern bool app_do_modal;
extern QWidgetList *qt_modal_stack;

QString QApplicationPrivate::appName() const
{
    return QT_PREPEND_NAMESPACE(appName);
}

void QApplicationPrivate::createEventDispatcher()
{
    QGuiApplicationPrivate::createEventDispatcher();
}

bool qt_try_modal(QWidget *widget, QEvent::Type type)
{
    QWidget * top = 0;

    if (QApplicationPrivate::tryModalHelper(widget, &top))
        return true;

    bool block_event  = false;
    bool paint_event = false;

    switch (type) {
#if 0
    case QEvent::Focus:
        if (!static_cast<QWSFocusEvent*>(event)->simpleData.get_focus)
            break;
        // drop through
#endif
    case QEvent::MouseButtonPress:                        // disallow mouse/key events
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        block_event         = true;
        break;
    default:
        break;
    }

    if ((block_event || paint_event) && top->parentWidget() == 0)
        top->raise();

    return !block_event;
}

void QApplicationPrivate::enterModal_sys(QWidget *widget)
{
    if (!qt_modal_stack)
        qt_modal_stack = new QWidgetList;
    qt_modal_stack->insert(0, widget);
    app_do_modal = true;
}

void QApplicationPrivate::leaveModal_sys(QWidget *widget)
{
    if (qt_modal_stack && qt_modal_stack->removeAll(widget)) {
        if (qt_modal_stack->isEmpty()) {
            delete qt_modal_stack;
            qt_modal_stack = 0;
        }
    }
    app_do_modal = qt_modal_stack != 0;
}

bool QApplicationPrivate::modalState()
{
    return app_do_modal;
}

QWidget *qt_tlw_for_window(QWindow *wnd)
{
    if (wnd)
        foreach (QWidget *tlw, qApp->topLevelWidgets())
            if (tlw->windowHandle() == wnd)
                return tlw;
    return 0;
}

void QApplicationPrivate::notifyActiveWindowChange(QWindow *previous)
{
    Q_UNUSED(previous);
    Q_Q(QApplication);
    QWindow *wnd = QGuiApplicationPrivate::active_window;
    QWidget *tlw = qt_tlw_for_window(wnd);
    if (!tlw)
        return;
    if (modalState()
        && !qt_try_modal(tlw, QEvent::MouseButtonRelease)
        && q->activeWindow())
        q->activeWindow()->activateWindow();
    else
        q->setActiveWindow(tlw);
}

void QApplicationPrivate::closePopup(QWidget *popup)
{
    Q_Q(QApplication);
    if (!popupWidgets)
        return;
    popupWidgets->removeAll(popup);

//###
//     if (popup == qt_popup_down) {
//         qt_button_down = 0;
//         qt_popup_down = 0;
//     }

    if (QApplicationPrivate::popupWidgets->count() == 0) {                // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = 0;

        //### replay mouse event?

        //### transfer/release mouse grab

        //### transfer/release keyboard grab

        //give back focus

        if (active_window) {
            if (QWidget *fw = active_window->focusWidget()) {
                if (fw != QApplication::focusWidget()) {
                    fw->setFocus(Qt::PopupFocusReason);
                } else {
                    QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                    q->sendEvent(fw, &e);
                }
            }
        }

    } else {
        // A popup was closed, so the previous popup gets the focus.

        QWidget* aw = QApplicationPrivate::popupWidgets->last();
        if (QWidget *fw = aw->focusWidget())
            fw->setFocus(Qt::PopupFocusReason);

        //### regrab the keyboard and mouse in case 'popup' lost the grab


    }

}

int openPopupCount = 0;
void QApplicationPrivate::openPopup(QWidget *popup)
{
    openPopupCount++;
    if (!popupWidgets) {                        // create list
        popupWidgets = new QWidgetList;

        /* only grab if you are the first/parent popup */
        //####   ->grabMouse(popup,true);
        //####   ->grabKeyboard(popup,true);
        //### popupGrabOk = true;
    }
    popupWidgets->append(popup);                // add to end of list

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    if (popup->focusWidget()) {
        popup->focusWidget()->setFocus(Qt::PopupFocusReason);
    } else if (popupWidgets->count() == 1) { // this was the first popup
        if (QWidget *fw = QApplication::focusWidget()) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            QApplication::sendEvent(fw, &e);
        }
    }
}

void QApplicationPrivate::initializeMultitouch_sys()
{
}

void QApplicationPrivate::cleanupMultitouch_sys()
{
}

void QApplicationPrivate::initializeWidgetPaletteHash()
{
}

void QApplication::setCursorFlashTime(int msecs)
{
    QApplicationPrivate::cursor_flash_time = msecs;
}

int QApplication::cursorFlashTime()
{
    return QApplicationPrivate::cursor_flash_time;
}

void QApplication::setDoubleClickInterval(int ms)
{
    QApplicationPrivate::mouse_double_click_time = ms;
}

int QApplication::doubleClickInterval()
{
    return QApplicationPrivate::mouse_double_click_time;
}

void QApplication::setKeyboardInputInterval(int ms)
{
    QApplicationPrivate::keyboard_input_time = ms;
}

int QApplication::keyboardInputInterval()
{
    return QApplicationPrivate::keyboard_input_time;
}

#ifndef QT_NO_WHEELEVENT
void QApplication::setWheelScrollLines(int lines)
{
    QApplicationPrivate::wheel_scroll_lines = lines;
}

int QApplication::wheelScrollLines()
{
    return QApplicationPrivate::wheel_scroll_lines;
}
#endif

void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
{
    switch (effect) {
    case Qt::UI_AnimateMenu:
        QApplicationPrivate::animate_menu = enable;
        break;
    case Qt::UI_FadeMenu:
        if (enable)
            QApplicationPrivate::animate_menu = true;
        QApplicationPrivate::fade_menu = enable;
        break;
    case Qt::UI_AnimateCombo:
        QApplicationPrivate::animate_combo = enable;
        break;
    case Qt::UI_AnimateTooltip:
        QApplicationPrivate::animate_tooltip = enable;
        break;
    case Qt::UI_FadeTooltip:
        if (enable)
            QApplicationPrivate::animate_tooltip = true;
        QApplicationPrivate::fade_tooltip = enable;
        break;
    case Qt::UI_AnimateToolBox:
        QApplicationPrivate::animate_toolbox = enable;
        break;
    default:
        QApplicationPrivate::animate_ui = enable;
        break;
    }
}

bool QApplication::isEffectEnabled(Qt::UIEffect effect)
{
    if (QColormap::instance().depth() < 16 || !QApplicationPrivate::animate_ui)
        return false;

    switch(effect) {
    case Qt::UI_AnimateMenu:
        return QApplicationPrivate::animate_menu;
    case Qt::UI_FadeMenu:
        return QApplicationPrivate::fade_menu;
    case Qt::UI_AnimateCombo:
        return QApplicationPrivate::animate_combo;
    case Qt::UI_AnimateTooltip:
        return QApplicationPrivate::animate_tooltip;
    case Qt::UI_FadeTooltip:
        return QApplicationPrivate::fade_tooltip;
    case Qt::UI_AnimateToolBox:
        return QApplicationPrivate::animate_toolbox;
    default:
        return QApplicationPrivate::animate_ui;
    }
}

QWidget *QApplication::topLevelAt(const QPoint &pos)
{
    QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();

    QList<QPlatformScreen *> screens = pi->screens();
    QList<QPlatformScreen *>::const_iterator screen = screens.constBegin();
    QList<QPlatformScreen *>::const_iterator end = screens.constEnd();

    // The first screen in a virtual environment should know about all top levels
    if (pi->isVirtualDesktop()) {
        QWidgetWindow *w = qobject_cast<QWidgetWindow *>((*screen)->topLevelAt(pos));
        return w ? w->widget() : 0;
    }

    while (screen != end) {
        if ((*screen)->geometry().contains(pos)) {
            QWidgetWindow *w = qobject_cast<QWidgetWindow *>((*screen)->topLevelAt(pos));
            return w ? w->widget() : 0;
        }
        ++screen;
    }
    return 0;
}

void QApplication::beep()
{
}

void QApplication::alert(QWidget *, int)
{
}

QPlatformNativeInterface *QApplication::platformNativeInterface()
{
    QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    return pi->nativeInterface();
}

#ifndef QT_NO_QWS_INPUTMETHODS
class QDummyInputContext : public QInputContext
{
public:
    explicit QDummyInputContext(QObject* parent = 0) : QInputContext(parent) {}
    ~QDummyInputContext() {}
    QString identifierName() { return QString(); }
    QString language() { return QString(); }

    void reset() {}
    bool isComposing() const { return false; }

};
#endif // QT_NO_QWS_INPUTMETHODS

void qt_init(QApplicationPrivate *, int type)
{
    Q_UNUSED(type);

    qApp->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QColormap::initialize();

    qApp->setObjectName(appName);

#ifndef QT_NO_QWS_INPUTMETHODS
        qApp->setInputContext(new QDummyInputContext(qApp));
#endif
}

void qt_cleanup()
{
    QPixmapCache::clear();
    QColormap::cleanup();
    delete QApplicationPrivate::inputContext;
    QApplicationPrivate::inputContext = 0;

    QApplicationPrivate::active_window = 0; //### this should not be necessary
}


#ifdef QT3_SUPPORT
void QApplication::setMainWidget(QWidget *mainWidget)
{
    QApplicationPrivate::main_widget = mainWidget;
    if (QApplicationPrivate::main_widget && windowIcon().isNull()
        && QApplicationPrivate::main_widget->testAttribute(Qt::WA_SetWindowIcon))
        setWindowIcon(QApplicationPrivate::main_widget->windowIcon());
}
#endif

QT_END_NAMESPACE
