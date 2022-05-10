// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "eventfilter.h"
#include <QtGui/QMouseEvent>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

#if defined(QT_WIDGETS_LIB)
#  define HAVE_APPLICATION
#endif
#if defined(QT_GUI_LIB)
#  define HAVE_GUI_APPLICATION
#endif

#ifdef HAVE_APPLICATION
#  include <QApplication>
#  include <QWidget>
#endif
#ifdef HAVE_GUI_APPLICATION
#  include <QtGui/QGuiApplication>
#  include <QtGui/QWindow>
#endif

namespace QtDiag {

EventFilter::EventFilter(EventCategories eventCategories, QObject *p)
    : QObject(p)
{
    init(eventCategories);
}

EventFilter::EventFilter(QObject *p)
    : QObject(p)
{
    init(EventCategories(0xFFFFFFF));
}

void EventFilter::init(EventCategories eventCategories)
{
    m_objectTypes = OtherType | QWidgetType | QWindowType;

    if (eventCategories & MouseEvents) {
        m_eventTypes << QEvent::MouseButtonPress << QEvent::MouseButtonRelease
            << QEvent::MouseButtonDblClick << QEvent::NonClientAreaMouseButtonPress
            << QEvent::NonClientAreaMouseButtonRelease
            << QEvent::NonClientAreaMouseButtonDblClick
            << QEvent::Wheel << QEvent::Enter << QEvent::Leave;
    }
    if (eventCategories & MouseMoveEvents)
        m_eventTypes << QEvent::MouseMove << QEvent::NonClientAreaMouseMove;
    if (eventCategories & TouchEvents) {
        m_eventTypes << QEvent::TouchBegin << QEvent::TouchUpdate << QEvent::TouchEnd;
        m_eventTypes << QEvent::TouchCancel;
    }
    if (eventCategories & TabletEvents) {
        m_eventTypes << QEvent::TabletEnterProximity << QEvent::TabletLeaveProximity
            << QEvent::TabletMove << QEvent::TabletPress << QEvent::TabletRelease;
    }
    if (eventCategories & DragAndDropEvents) {
        m_eventTypes << QEvent::DragEnter << QEvent::DragMove << QEvent::DragLeave
            << QEvent::Drop << QEvent::DragResponse;
    }
    if (eventCategories & KeyEvents) {
        m_eventTypes << QEvent::KeyPress << QEvent::KeyRelease << QEvent::ShortcutOverride
            << QEvent::Shortcut;
    }
    if (eventCategories & FocusEvents) {
        m_eventTypes
            << QEvent::FocusAboutToChange
            << QEvent::FocusIn << QEvent::FocusOut;
    }
    if (eventCategories & GeometryEvents)
        m_eventTypes << QEvent::Move << QEvent::Resize;
    if (eventCategories & PaintEvents) {
        m_eventTypes << QEvent::UpdateRequest << QEvent::Paint
            << QEvent::Show << QEvent::Hide;
        m_eventTypes << QEvent::Expose;
    }
    if (eventCategories & StateChangeEvents) {
        m_eventTypes
            << QEvent::WindowStateChange
            << QEvent::WindowBlocked << QEvent::WindowUnblocked
            << QEvent::ApplicationStateChange
            << QEvent::ApplicationActivate << QEvent::ApplicationDeactivate;
    }
    if (eventCategories & TimerEvents)
        m_eventTypes << QEvent::Timer << QEvent::ZeroTimerEvent;
    if (eventCategories & ObjectEvents) {
        m_eventTypes << QEvent::ChildAdded << QEvent::ChildPolished
            << QEvent::ChildRemoved << QEvent::Create << QEvent::Destroy;
    }
    if (eventCategories & InputMethodEvents) {
        m_eventTypes << QEvent::InputMethod;
        m_eventTypes << QEvent::InputMethodQuery;
    }
#ifndef QT_NO_GESTURES
    if (eventCategories & GestureEvents) {
        m_eventTypes << QEvent::Gesture << QEvent::GestureOverride
            << QEvent::NativeGesture;
    }
#endif
}

static inline bool matchesType(const QObject *o, EventFilter::ObjectTypes types)
{
    if (o->isWidgetType())
        return types & EventFilter::QWidgetType;
    if (o->isWindowType())
        return types & EventFilter::QWindowType;
    return types & EventFilter::OtherType;
}

void EventFilter::formatObject(const QObject *o, QDebug debug)
{
    if (o) {
        debug << o->metaObject()->className();
        const QString on = o->objectName();
        if (!on.isEmpty())
            debug << '/' << on;
    } else {
        debug << "null";
    }
}

QDebug operator<<(QDebug d, const formatQObject &fo)
{
    EventFilter::formatObject(fo.m_object, d);
    return d;
}

static void formatApplicationState(QDebug debug)
{
#if defined(HAVE_APPLICATION)
    if (const QWidget *mw = QApplication::activeModalWidget())
        debug << "\n  QApplication::activeModalWidget = " << formatQObject(mw);
    if (const QWidget *pw = QApplication::activePopupWidget())
        debug << "\n  QApplication::activePopupWidget = " << formatQObject(pw);
    debug << "\n  QApplication::activeWindow      = " << formatQObject(QApplication::activeWindow());
#endif // HAVE_APPLICATION
#if defined(HAVE_GUI_APPLICATION)
    if (const QWindow *mw = QGuiApplication::modalWindow()) {
        debug << "\n  QGuiApplication::modalWindow    = " << formatQObject(mw);
    }
    const QObject *focusObject = QGuiApplication::focusObject();
    const QObject *focusWindow = QGuiApplication::focusWindow();
    debug << "\n  QGuiApplication::focusObject    = " << formatQObject(focusObject);
    if (focusWindow && focusWindow != focusObject)
        debug << "\n  QGuiApplication::focusWindow    = " << formatQObject(focusWindow);
#endif // HAVE_GUI_APPLICATION
}

#ifdef HAVE_APPLICATION
static void formatMouseState(QObject *o, QDebug debug)
{
    if (o->isWidgetType()) {
        auto w = static_cast<const QWidget *>(o);
        if (QWidget::mouseGrabber() == w)
            debug << " [grabbed]";
        if (w->hasMouseTracking())
            debug << " [tracking]";
    }
}
#endif // HAVE_APPLICATION

bool EventFilter::eventFilter(QObject *o, QEvent *e)
{
    static int n = 0;
    if (matchesType(o, m_objectTypes) && m_eventTypes.contains(e->type())) {
        QDebug debug = qDebug().nospace();
        debug << '#' << n++ << ' ';
        formatObject(o, debug);
        debug << ' ' << e;
        switch (e->type()) {
        case QEvent::FocusAboutToChange:
        case QEvent::FocusIn:
            formatApplicationState(debug);
            break;
#ifdef HAVE_APPLICATION
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::NonClientAreaMouseButtonDblClick:
        case QEvent::NonClientAreaMouseButtonPress:
        case QEvent::NonClientAreaMouseButtonRelease:
        case QEvent::NonClientAreaMouseMove:
        case QEvent::Enter:
        case QEvent::Leave:
            formatMouseState(o, debug);
            break;
#endif // HAVE_APPLICATION
        default:
            break;
        }
    }
    return false;
}

} // namespace QtDiag
