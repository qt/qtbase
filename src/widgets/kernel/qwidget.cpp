/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include "qapplication.h"
#include "qapplication_p.h"
#include "qbrush.h"
#include "qcursor.h"
#include "qdesktopwidget_p.h"
#include "qevent.h"
#include "qlayout.h"
#if QT_CONFIG(menu)
#include "qmenu.h"
#endif
#include "qmetaobject.h"
#include "qpixmap.h"
#include "qpointer.h"
#include "qstack.h"
#include "qstyle.h"
#include "qstylefactory.h"
#include "qvariant.h"
#include "qwidget.h"
#include "qstyleoption.h"
#include "qstylehints.h"
#ifndef QT_NO_ACCESSIBILITY
# include "qaccessible.h"
#endif
#include <qpa/qplatformwindow.h>
#include "private/qwidgetwindow_p.h"
#include "qpainter.h"
#include "qtooltip.h"
#if QT_CONFIG(whatsthis)
#include "qwhatsthis.h"
#endif
#include "qdebug.h"
#include "private/qstylesheetstyle_p.h"
#include "private/qstyle_p.h"
#include "qfileinfo.h"
#include "qscopeguard.h"
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qinputmethod.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/qoffscreensurface.h>

#if QT_CONFIG(graphicseffect)
#include <private/qgraphicseffect_p.h>
#endif
#include <qbackingstore.h>
#include <private/qwidgetrepaintmanager_p.h>
#include <private/qpaintengine_raster_p.h>

#include "qwidget_p.h"
#include <QtGui/private/qwindow_p.h>
#include "qaction_p.h"
#include "qlayout_p.h"
#if QT_CONFIG(graphicsview)
#include "QtWidgets/qgraphicsproxywidget.h"
#include "QtWidgets/qgraphicsscene.h"
#include "private/qgraphicsproxywidget_p.h"
#endif
#include "QtWidgets/qabstractscrollarea.h"
#include "private/qabstractscrollarea_p.h"
#include "private/qevent_p.h"

#include "private/qgesturemanager_p.h"

#ifdef QT_KEYPAD_NAVIGATION
#if QT_CONFIG(tabwidget)
#include "qtabwidget.h" // Needed in inTabWidget()
#endif
#endif // QT_KEYPAD_NAVIGATION

#include "qwindowcontainer_p.h"

#include <QtPlatformHeaders/qxcbwindowfunctions.h>

#include <private/qmemory_p.h>

// widget/widget data creation count
//#define QWIDGET_EXTRA_DEBUG
//#define ALIEN_DEBUG

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcWidgetPainting, "qt.widgets.painting", QtWarningMsg);

static inline bool qRectIntersects(const QRect &r1, const QRect &r2)
{
    return (qMax(r1.left(), r2.left()) <= qMin(r1.right(), r2.right()) &&
            qMax(r1.top(), r2.top()) <= qMin(r1.bottom(), r2.bottom()));
}

extern bool qt_sendSpontaneousEvent(QObject*, QEvent*); // qapplication.cpp
extern QDesktopWidget *qt_desktopWidget; // qapplication.cpp

QWidgetPrivate::QWidgetPrivate(int version)
    : QObjectPrivate(version)
      , focus_next(nullptr)
      , focus_prev(nullptr)
      , focus_child(nullptr)
      , layout(nullptr)
      , needsFlush(nullptr)
      , redirectDev(nullptr)
      , widgetItem(nullptr)
      , extraPaintEngine(nullptr)
      , polished(nullptr)
      , graphicsEffect(nullptr)
#if !defined(QT_NO_IM)
      , imHints(Qt::ImhNone)
#endif
#ifndef QT_NO_TOOLTIP
      , toolTipDuration(-1)
#endif
      , directFontResolveMask(0)
      , inheritedFontResolveMask(0)
      , directPaletteResolveMask(0)
      , inheritedPaletteResolveMask(0)
      , leftmargin(0)
      , topmargin(0)
      , rightmargin(0)
      , bottommargin(0)
      , leftLayoutItemMargin(0)
      , topLayoutItemMargin(0)
      , rightLayoutItemMargin(0)
      , bottomLayoutItemMargin(0)
      , hd(nullptr)
      , size_policy(QSizePolicy::Preferred, QSizePolicy::Preferred)
      , fg_role(QPalette::NoRole)
      , bg_role(QPalette::NoRole)
      , dirtyOpaqueChildren(1)
      , isOpaque(0)
      , retainSizeWhenHiddenChanged(0)
      , inDirtyList(0)
      , isScrolled(0)
      , isMoved(0)
      , usesDoubleBufferedGLContext(0)
      , mustHaveWindowHandle(0)
      , renderToTexture(0)
      , textureChildSeen(0)
#ifndef QT_NO_IM
      , inheritsInputMethodHints(0)
#endif
#ifndef QT_NO_OPENGL
      , renderToTextureReallyDirty(1)
      , renderToTextureComposeActive(0)
#endif
      , childrenHiddenByWState(0)
      , childrenShownByExpose(0)
#if defined(Q_OS_WIN)
      , noPaintOnScreen(0)
#endif
{
    if (Q_UNLIKELY(!qApp)) {
        qFatal("QWidget: Must construct a QApplication before a QWidget");
        return;
    }

    checkForIncompatibleLibraryVersion(version);

    isWidget = true;
    memset(high_attributes, 0, sizeof(high_attributes));

#ifdef QWIDGET_EXTRA_DEBUG
    static int count = 0;
    qDebug() << "widgets" << ++count;
#endif
}


QWidgetPrivate::~QWidgetPrivate()
{
    if (widgetItem)
        widgetItem->wid = nullptr;

    if (extra)
        deleteExtra();
}

/*!
    \internal
*/
void QWidgetPrivate::scrollChildren(int dx, int dy)
{
    Q_Q(QWidget);
    if (q->children().size() > 0) {        // scroll children
        QPoint pd(dx, dy);
        QObjectList childObjects = q->children();
        for (int i = 0; i < childObjects.size(); ++i) { // move all children
            QWidget *w = qobject_cast<QWidget*>(childObjects.at(i));
            if (w && !w->isWindow()) {
                QPoint oldp = w->pos();
                QRect  r(w->pos() + pd, w->size());
                w->data->crect = r;
                if (w->testAttribute(Qt::WA_WState_Created))
                    w->d_func()->setWSGeometry();
                w->d_func()->setDirtyOpaqueRegion();
                QMoveEvent e(r.topLeft(), oldp);
                QCoreApplication::sendEvent(w, &e);
            }
        }
    }
}

void QWidgetPrivate::setWSGeometry()
{
    Q_Q(QWidget);
    if (QWindow *window = q->windowHandle())
        window->setGeometry(data.crect);
}

void QWidgetPrivate::updateWidgetTransform(QEvent *event)
{
    Q_Q(QWidget);
    if (q == QGuiApplication::focusObject() || event->type() == QEvent::FocusIn) {
        QTransform t;
        QPoint p = q->mapTo(q->topLevelWidget(), QPoint(0,0));
        t.translate(p.x(), p.y());
        QGuiApplication::inputMethod()->setInputItemTransform(t);
        QGuiApplication::inputMethod()->setInputItemRectangle(q->rect());
        QGuiApplication::inputMethod()->update(Qt::ImInputItemClipRectangle);
    }
}

#ifdef QT_KEYPAD_NAVIGATION
QPointer<QWidget> QWidgetPrivate::editingWidget;

/*!
    Returns \c true if this widget currently has edit focus; otherwise false.

    This feature is only available in Qt for Embedded Linux.

    \sa setEditFocus(), QApplication::navigationMode()
*/
bool QWidget::hasEditFocus() const
{
    const QWidget* w = this;
    while (w->d_func()->extra && w->d_func()->extra->focus_proxy)
        w = w->d_func()->extra->focus_proxy;
    return QWidgetPrivate::editingWidget == w;
}

/*!
    \fn void QWidget::setEditFocus(bool enable)

    If \a enable is true, make this widget have edit focus, in which
    case Qt::Key_Up and Qt::Key_Down will be delivered to the widget
    normally; otherwise, Qt::Key_Up and Qt::Key_Down are used to
    change focus.

    This feature is only available in Qt for Embedded Linux.

    \sa hasEditFocus(), QApplication::navigationMode()
*/
void QWidget::setEditFocus(bool on)
{
    QWidget *f = this;
    while (f->d_func()->extra && f->d_func()->extra->focus_proxy)
        f = f->d_func()->extra->focus_proxy;

    if (QWidgetPrivate::editingWidget && QWidgetPrivate::editingWidget != f)
        QWidgetPrivate::editingWidget->setEditFocus(false);

    if (on && !f->hasFocus())
        f->setFocus();

    if ((!on && !QWidgetPrivate::editingWidget)
        || (on && QWidgetPrivate::editingWidget == f)) {
        return;
    }

    if (!on && QWidgetPrivate::editingWidget == f) {
        QWidgetPrivate::editingWidget = 0;
        QEvent event(QEvent::LeaveEditFocus);
        QCoreApplication::sendEvent(f, &event);
        QCoreApplication::sendEvent(f->style(), &event);
    } else if (on) {
        QWidgetPrivate::editingWidget = f;
        QEvent event(QEvent::EnterEditFocus);
        QCoreApplication::sendEvent(f, &event);
        QCoreApplication::sendEvent(f->style(), &event);
    }
}
#endif

/*!
    \property QWidget::autoFillBackground
    \brief whether the widget background is filled automatically
    \since 4.1

    If enabled, this property will cause Qt to fill the background of the
    widget before invoking the paint event. The color used is defined by the
    QPalette::Window color role from the widget's \l{QPalette}{palette}.

    In addition, Windows are always filled with QPalette::Window, unless the
    WA_OpaquePaintEvent or WA_NoSystemBackground attributes are set.

    This property cannot be turned off (i.e., set to false) if a widget's
    parent has a static gradient for its background.

    \warning Use this property with caution in conjunction with
    \l{Qt Style Sheets}. When a widget has a style sheet with a valid
    background or a border-image, this property is automatically disabled.

    By default, this property is \c false.

    \sa Qt::WA_OpaquePaintEvent, Qt::WA_NoSystemBackground,
    {QWidget#Transparency and Double Buffering}{Transparency and Double Buffering}
*/
bool QWidget::autoFillBackground() const
{
    Q_D(const QWidget);
    return d->extra && d->extra->autoFillBackground;
}

void QWidget::setAutoFillBackground(bool enabled)
{
    Q_D(QWidget);
    if (!d->extra)
        d->createExtra();
    if (d->extra->autoFillBackground == enabled)
        return;

    d->extra->autoFillBackground = enabled;
    d->updateIsOpaque();
    update();
    d->updateIsOpaque();
}

/*!
    \class QWidget
    \brief The QWidget class is the base class of all user interface objects.

    \ingroup basicwidgets
    \inmodule QtWidgets

    The widget is the atom of the user interface: it receives mouse, keyboard
    and other events from the window system, and paints a representation of
    itself on the screen. Every widget is rectangular, and they are sorted in a
    Z-order. A widget is clipped by its parent and by the widgets in front of
    it.

    A widget that is not embedded in a parent widget is called a window.
    Usually, windows have a frame and a title bar, although it is also possible
    to create windows without such decoration using suitable
    \l{Qt::WindowFlags}{window flags}). In Qt, QMainWindow and the various
    subclasses of QDialog are the most common window types.

    Every widget's constructor accepts one or two standard arguments:

    \list 1
        \li  \c{QWidget *parent = nullptr} is the parent of the new widget.
            If it is \nullptr (the default), the new widget will be a window.
            If not, it will be a child of \e parent, and be constrained by
            \e parent's geometry (unless you specify Qt::Window as window flag).
        \li  \c{Qt::WindowFlags f = { }} (where available) sets the window flags;
            the default is suitable for almost all widgets, but to get, for
            example, a window without a window system frame, you must use
            special flags.
    \endlist

    QWidget has many member functions, but some of them have little direct
    functionality; for example, QWidget has a font property, but never uses
    this itself. There are many subclasses which provide real functionality,
    such as QLabel, QPushButton, QListWidget, and QTabWidget.


    \section1 Top-Level and Child Widgets

    A widget without a parent widget is always an independent window (top-level
    widget). For these widgets, setWindowTitle() and setWindowIcon() set the
    title bar and icon respectively.

    Non-window widgets are child widgets, displayed within their parent
    widgets. Most widgets in Qt are mainly useful as child widgets. For
    example, it is possible to display a button as a top-level window, but most
    people prefer to put their buttons inside other widgets, such as QDialog.

    \image parent-child-widgets.png A parent widget containing various child widgets.

    The diagram above shows a QGroupBox widget being used to hold various child
    widgets in a layout provided by QGridLayout. The QLabel child widgets have
    been outlined to indicate their full sizes.

    If you want to use a QWidget to hold child widgets you will usually want to
    add a layout to the parent QWidget. See \l{Layout Management} for more
    information.


    \section1 Composite Widgets

    When a widget is used as a container to group a number of child widgets, it
    is known as a composite widget. These can be created by constructing a
    widget with the required visual properties - a QFrame, for example - and
    adding child widgets to it, usually managed by a layout. The above diagram
    shows such a composite widget that was created using Qt Designer.

    Composite widgets can also be created by subclassing a standard widget,
    such as QWidget or QFrame, and adding the necessary layout and child
    widgets in the constructor of the subclass. Many of the \l{Qt Widgets Examples}
    {examples provided with Qt} use this approach, and it is also covered in
    the Qt \l{Tutorials}.


    \section1 Custom Widgets and Painting

    Since QWidget is a subclass of QPaintDevice, subclasses can be used to
    display custom content that is composed using a series of painting
    operations with an instance of the QPainter class. This approach contrasts
    with the canvas-style approach used by the \l{Graphics View}
    {Graphics View Framework} where items are added to a scene by the
    application and are rendered by the framework itself.

    Each widget performs all painting operations from within its paintEvent()
    function. This is called whenever the widget needs to be redrawn, either
    as a result of some external change or when requested by the application.

    The \l{widgets/analogclock}{Analog Clock example} shows how a simple widget
    can handle paint events.


    \section1 Size Hints and Size Policies

    When implementing a new widget, it is almost always useful to reimplement
    sizeHint() to provide a reasonable default size for the widget and to set
    the correct size policy with setSizePolicy().

    By default, composite widgets which do not provide a size hint will be
    sized according to the space requirements of their child widgets.

    The size policy lets you supply good default behavior for the layout
    management system, so that other widgets can contain and manage yours
    easily. The default size policy indicates that the size hint represents
    the preferred size of the widget, and this is often good enough for many
    widgets.

    \note The size of top-level widgets are constrained to 2/3 of the desktop's
    height and width. You can resize() the widget manually if these bounds are
    inadequate.


    \section1 Events

    Widgets respond to events that are typically caused by user actions. Qt
    delivers events to widgets by calling specific event handler functions with
    instances of QEvent subclasses containing information about each event.

    If your widget only contains child widgets, you probably do not need to
    implement any event handlers. If you want to detect a mouse click in a
    child widget call the child's underMouse() function inside the widget's
    mousePressEvent().

    The \l{widgets/scribble}{Scribble example} implements a wider set of
    events to handle mouse movement, button presses, and window resizing.

    You will need to supply the behavior and content for your own widgets, but
    here is a brief overview of the events that are relevant to QWidget,
    starting with the most common ones:

    \list
        \li  paintEvent() is called whenever the widget needs to be repainted.
            Every widget displaying custom content must implement it. Painting
            using a QPainter can only take place in a paintEvent() or a
            function called by a paintEvent().
        \li  resizeEvent() is called when the widget has been resized.
        \li  mousePressEvent() is called when a mouse button is pressed while
            the mouse cursor is inside the widget, or when the widget has
            grabbed the mouse using grabMouse(). Pressing the mouse without
            releasing it is effectively the same as calling grabMouse().
        \li  mouseReleaseEvent() is called when a mouse button is released. A
            widget receives mouse release events when it has received the
            corresponding mouse press event. This means that if the user
            presses the mouse inside \e your widget, then drags the mouse
            somewhere else before releasing the mouse button, \e your widget
            receives the release event. There is one exception: if a popup menu
            appears while the mouse button is held down, this popup immediately
            steals the mouse events.
        \li  mouseDoubleClickEvent() is called when the user double-clicks in
            the widget. If the user double-clicks, the widget receives a mouse
            press event, a mouse release event, (a mouse click event,) a second
            mouse press, this event and finally a second mouse release event.
            (Some mouse move events may also be
            received if the mouse is not held steady during this operation.) It
            is \e{not possible} to distinguish a click from a double-click
            until the second click arrives. (This is one reason why most GUI
            books recommend that double-clicks be an extension of
            single-clicks, rather than trigger a different action.)
    \endlist

    Widgets that accept keyboard input need to reimplement a few more event
    handlers:

    \list
        \li  keyPressEvent() is called whenever a key is pressed, and again when
            a key has been held down long enough for it to auto-repeat. The
            \uicontrol Tab and \uicontrol Shift+Tab keys are only passed to the widget if
            they are not used by the focus-change mechanisms. To force those
            keys to be processed by your widget, you must reimplement
            QWidget::event().
        \li  focusInEvent() is called when the widget gains keyboard focus
            (assuming you have called setFocusPolicy()). Well-behaved widgets
            indicate that they own the keyboard focus in a clear but discreet
            way.
        \li  focusOutEvent() is called when the widget loses keyboard focus.
    \endlist

    You may be required to also reimplement some of the less common event
    handlers:

    \list
        \li  mouseMoveEvent() is called whenever the mouse moves while a mouse
            button is held down. This can be useful during drag and drop
            operations. If you call \l{setMouseTracking()}{setMouseTracking}(true),
            you get mouse move events even when no buttons are held down.
            (See also the \l{Drag and Drop} guide.)
        \li  keyReleaseEvent() is called whenever a key is released and while it
            is held down (if the key is auto-repeating). In that case, the
            widget will receive a pair of key release and key press event for
            every repeat. The \uicontrol Tab and \uicontrol Shift+Tab keys are only passed
            to the widget if they are not used by the focus-change mechanisms.
            To force those keys to be processed by your widget, you must
            reimplement QWidget::event().
        \li  wheelEvent() is called whenever the user turns the mouse wheel
            while the widget has the focus.
        \li  enterEvent() is called when the mouse enters the widget's screen
            space. (This excludes screen space owned by any of the widget's
            children.)
        \li  leaveEvent() is called when the mouse leaves the widget's screen
            space. If the mouse enters a child widget it will not cause a
            leaveEvent().
        \li  moveEvent() is called when the widget has been moved relative to
            its parent.
        \li  closeEvent() is called when the user closes the widget (or when
            close() is called).
    \endlist

    There are also some rather obscure events described in the documentation
    for QEvent::Type. To handle these events, you need to reimplement event()
    directly.

    The default implementation of event() handles \uicontrol Tab and \uicontrol Shift+Tab
    (to move the keyboard focus), and passes on most of the other events to
    one of the more specialized handlers above.

    Events and the mechanism used to deliver them are covered in
    \l{The Event System}.

    \section1 Groups of Functions and Properties

    \table
    \header \li Context \li Functions and Properties

    \row \li Window functions \li
        show(),
        hide(),
        raise(),
        lower(),
        close().

    \row \li Top-level windows \li
        \l windowModified, \l windowTitle, \l windowIcon,
        \l isActiveWindow, activateWindow(), \l minimized, showMinimized(),
        \l maximized, showMaximized(), \l fullScreen, showFullScreen(),
        showNormal().

    \row \li Window contents \li
        update(),
        repaint(),
        scroll().

    \row \li Geometry \li
        \l pos, x(), y(), \l rect, \l size, width(), height(), move(), resize(),
        \l sizePolicy, sizeHint(), minimumSizeHint(),
        updateGeometry(), layout(),
        \l frameGeometry, \l geometry, \l childrenRect, \l childrenRegion,
        adjustSize(),
        mapFromGlobal(), mapToGlobal(),
        mapFromParent(), mapToParent(),
        \l maximumSize, \l minimumSize, \l sizeIncrement,
        \l baseSize, setFixedSize()

    \row \li Mode \li
        \l visible, isVisibleTo(),
        \l enabled, isEnabledTo(),
        \l modal,
        isWindow(),
        \l mouseTracking,
        \l updatesEnabled,
        visibleRegion().

    \row \li Look and feel \li
        style(),
        setStyle(),
        \l styleSheet,
        \l cursor,
        \l font,
        \l palette,
        backgroundRole(), setBackgroundRole(),
        fontInfo(), fontMetrics().

    \row \li Keyboard focus functions \li
        \l focus, \l focusPolicy,
        setFocus(), clearFocus(), setTabOrder(), setFocusProxy(),
        focusNextChild(), focusPreviousChild().

    \row \li Mouse and keyboard grabbing \li
        grabMouse(), releaseMouse(),
        grabKeyboard(), releaseKeyboard(),
        mouseGrabber(), keyboardGrabber().

    \row \li Event handlers \li
        event(),
        mousePressEvent(),
        mouseReleaseEvent(),
        mouseDoubleClickEvent(),
        mouseMoveEvent(),
        keyPressEvent(),
        keyReleaseEvent(),
        focusInEvent(),
        focusOutEvent(),
        wheelEvent(),
        enterEvent(),
        leaveEvent(),
        paintEvent(),
        moveEvent(),
        resizeEvent(),
        closeEvent(),
        dragEnterEvent(),
        dragMoveEvent(),
        dragLeaveEvent(),
        dropEvent(),
        childEvent(),
        showEvent(),
        hideEvent(),
        customEvent().
        changeEvent(),

    \row \li System functions \li
        parentWidget(), window(), setParent(), winId(),
        find(), metric().

    \row \li Context menu \li
       contextMenuPolicy, contextMenuEvent(),
       customContextMenuRequested(), actions()

    \row \li Interactive help \li
        setToolTip(), setWhatsThis()

    \endtable


    \section1 Widget Style Sheets

    In addition to the standard widget styles for each platform, widgets can
    also be styled according to rules specified in a \l{styleSheet}
    {style sheet}. This feature enables you to customize the appearance of
    specific widgets to provide visual cues to users about their purpose. For
    example, a button could be styled in a particular way to indicate that it
    performs a destructive action.

    The use of widget style sheets is described in more detail in the
    \l{Qt Style Sheets} document.


    \section1 Transparency and Double Buffering

    Since Qt 4.0, QWidget automatically double-buffers its painting, so there
    is no need to write double-buffering code in paintEvent() to avoid
    flicker.

    Since Qt 4.1, the contents of parent widgets are propagated by
    default to each of their children as long as Qt::WA_PaintOnScreen is not
    set. Custom widgets can be written to take advantage of this feature by
    updating irregular regions (to create non-rectangular child widgets), or
    painting with colors that have less than full alpha component. The
    following diagram shows how attributes and properties of a custom widget
    can be fine-tuned to achieve different effects.

    \image propagation-custom.png

    In the above diagram, a semi-transparent rectangular child widget with an
    area removed is constructed and added to a parent widget (a QLabel showing
    a pixmap). Then, different properties and widget attributes are set to
    achieve different effects:

    \list
        \li  The left widget has no additional properties or widget attributes
            set. This default state suits most custom widgets using
            transparency, are irregularly-shaped, or do not paint over their
            entire area with an opaque brush.
        \li  The center widget has the \l autoFillBackground property set. This
            property is used with custom widgets that rely on the widget to
            supply a default background, and do not paint over their entire
            area with an opaque brush.
        \li  The right widget has the Qt::WA_OpaquePaintEvent widget attribute
            set. This indicates that the widget will paint over its entire area
            with opaque colors. The widget's area will initially be
            \e{uninitialized}, represented in the diagram with a red diagonal
            grid pattern that shines through the overpainted area. The
            Qt::WA_OpaquePaintArea attribute is useful for widgets that need to
            paint their own specialized contents quickly and do not need a
            default filled background.
    \endlist

    To rapidly update custom widgets with simple background colors, such as
    real-time plotting or graphing widgets, it is better to define a suitable
    background color (using setBackgroundRole() with the
    QPalette::Window role), set the \l autoFillBackground property, and only
    implement the necessary drawing functionality in the widget's paintEvent().

    To rapidly update custom widgets that constantly paint over their entire
    areas with opaque content, e.g., video streaming widgets, it is better to
    set the widget's Qt::WA_OpaquePaintEvent, avoiding any unnecessary overhead
    associated with repainting the widget's background.

    If a widget has both the Qt::WA_OpaquePaintEvent widget attribute \e{and}
    the \l autoFillBackground property set, the Qt::WA_OpaquePaintEvent
    attribute takes precedence. Depending on your requirements, you should
    choose either one of them.

    Since Qt 4.1, the contents of parent widgets are also propagated to
    standard Qt widgets. This can lead to some unexpected results if the
    parent widget is decorated in a non-standard way, as shown in the diagram
    below.

    \image propagation-standard.png

    The scope for customizing the painting behavior of standard Qt widgets,
    without resorting to subclassing, is slightly less than that possible for
    custom widgets. Usually, the desired appearance of a standard widget can be
    achieved by setting its \l autoFillBackground property.


    \section1 Creating Translucent Windows

    Since Qt 4.5, it has been possible to create windows with translucent regions
    on window systems that support compositing.

    To enable this feature in a top-level widget, set its Qt::WA_TranslucentBackground
    attribute with setAttribute() and ensure that its background is painted with
    non-opaque colors in the regions you want to be partially transparent.

    Platform notes:

    \list
    \li X11: This feature relies on the use of an X server that supports ARGB visuals
    and a compositing window manager.
    \li Windows: The widget needs to have the Qt::FramelessWindowHint window flag set
    for the translucency to work.
    \endlist


    \section1 Native Widgets vs Alien Widgets

    Introduced in Qt 4.4, alien widgets are widgets unknown to the windowing
    system. They do not have a native window handle associated with them. This
    feature significantly speeds up widget painting, resizing, and removes flicker.

    Should you require the old behavior with native windows, you can choose
    one of the following options:

    \list 1
        \li  Use the \c{QT_USE_NATIVE_WINDOWS=1} in your environment.
        \li  Set the Qt::AA_NativeWindows attribute on your application. All
            widgets will be native widgets.
        \li  Set the Qt::WA_NativeWindow attribute on widgets: The widget itself
            and all of its ancestors will become native (unless
            Qt::WA_DontCreateNativeAncestors is set).
        \li  Call QWidget::winId to enforce a native window (this implies 3).
        \li  Set the Qt::WA_PaintOnScreen attribute to enforce a native window
            (this implies 3).
    \endlist

    \sa QEvent, QPainter, QGridLayout, QBoxLayout

*/

QWidgetMapper *QWidgetPrivate::mapper = nullptr;          // widget with wid
QWidgetSet *QWidgetPrivate::allWidgets = nullptr;         // widgets with no wid


/*****************************************************************************
  QWidget member functions
 *****************************************************************************/

/*
    Widget state flags:
  \list
  \li Qt::WA_WState_Created The widget has a valid winId().
  \li Qt::WA_WState_Visible The widget is currently visible.
  \li Qt::WA_WState_Hidden The widget is hidden, i.e. it won't
  become visible unless you call show() on it. Qt::WA_WState_Hidden
  implies !Qt::WA_WState_Visible.
  \li Qt::WA_WState_CompressKeys Compress keyboard events.
  \li Qt::WA_WState_BlockUpdates Repaints and updates are disabled.
  \li Qt::WA_WState_InPaintEvent Currently processing a paint event.
  \li Qt::WA_WState_Reparented The widget has been reparented.
  \li Qt::WA_WState_ConfigPending A configuration (resize/move) event is pending.
  \li Qt::WA_WState_DND (Deprecated) The widget supports drag and drop, see setAcceptDrops().
  \endlist
*/

struct QWidgetExceptionCleaner
{
    /* this cleans up when the constructor throws an exception */
    static inline void cleanup(QWidget *that, QWidgetPrivate *d)
    {
#ifdef QT_NO_EXCEPTIONS
        Q_UNUSED(that);
        Q_UNUSED(d);
#else
        QWidgetPrivate::allWidgets->remove(that);
        if (d->focus_next != that) {
            if (d->focus_next)
                d->focus_next->d_func()->focus_prev = d->focus_prev;
            if (d->focus_prev)
                d->focus_prev->d_func()->focus_next = d->focus_next;
        }
#endif
    }
};

/*!
    Constructs a widget which is a child of \a parent, with  widget
    flags set to \a f.

    If \a parent is \nullptr, the new widget becomes a window. If
    \a parent is another widget, this widget becomes a child window
    inside \a parent. The new widget is deleted when its \a parent is
    deleted.

    The widget flags argument, \a f, is normally 0, but it can be set
    to customize the frame of a window (i.e. \a parent must be
    \nullptr). To customize the frame, use a value composed
    from the bitwise OR of any of the \l{Qt::WindowFlags}{window flags}.

    If you add a child widget to an already visible widget you must
    explicitly show the child to make it visible.

    Note that the X11 version of Qt may not be able to deliver all
    combinations of style flags on all systems. This is because on
    X11, Qt can only ask the window manager, and the window manager
    can override the application's settings. On Windows, Qt can set
    whatever flags you want.

    \sa windowFlags
*/
QWidget::QWidget(QWidget *parent, Qt::WindowFlags f)
    : QObject(*new QWidgetPrivate, nullptr), QPaintDevice()
{
    QT_TRY {
        d_func()->init(parent, f);
    } QT_CATCH(...) {
        QWidgetExceptionCleaner::cleanup(this, d_func());
        QT_RETHROW;
    }
}


/*! \internal
*/
QWidget::QWidget(QWidgetPrivate &dd, QWidget* parent, Qt::WindowFlags f)
    : QObject(dd, nullptr), QPaintDevice()
{
    Q_D(QWidget);
    QT_TRY {
        d->init(parent, f);
    } QT_CATCH(...) {
        QWidgetExceptionCleaner::cleanup(this, d_func());
        QT_RETHROW;
    }
}

/*!
    \internal
*/
int QWidget::devType() const
{
    return QInternal::Widget;
}


//### w is a "this" ptr, passed as a param because QWorkspace needs special logic
void QWidgetPrivate::adjustFlags(Qt::WindowFlags &flags, QWidget *w)
{
    bool customize =  (flags & (Qt::CustomizeWindowHint
            | Qt::FramelessWindowHint
            | Qt::WindowTitleHint
            | Qt::WindowSystemMenuHint
            | Qt::WindowMinimizeButtonHint
            | Qt::WindowMaximizeButtonHint
            | Qt::WindowCloseButtonHint
            | Qt::WindowContextHelpButtonHint));

    uint type = (flags & Qt::WindowType_Mask);

    if ((type == Qt::Widget || type == Qt::SubWindow) && w && !w->parent()) {
        type = Qt::Window;
        flags |= Qt::Window;
    }

    if (flags & Qt::CustomizeWindowHint) {
        // modify window flags to make them consistent.
        // Only enable this on non-Mac platforms. Since the old way of doing this would
        // interpret WindowSystemMenuHint as a close button and we can't change that behavior
        // we can't just add this in.
        if ((flags & (Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::WindowContextHelpButtonHint))
#  ifdef Q_OS_WIN
            && type != Qt::Dialog // QTBUG-2027, allow for menu-less dialogs.
#  endif
           ) {
            flags |= Qt::WindowSystemMenuHint;
            flags |= Qt::WindowTitleHint;
            flags &= ~Qt::FramelessWindowHint;
        }
    } else if (customize && !(flags & Qt::FramelessWindowHint)) {
        // if any of the window hints that affect the titlebar are set
        // and the window is supposed to have frame, we add a titlebar
        // and system menu by default.
        flags |= Qt::WindowSystemMenuHint;
        flags |= Qt::WindowTitleHint;
    }
    if (customize)
        ; // don't modify window flags if the user explicitly set them.
    else if (type == Qt::Dialog || type == Qt::Sheet) {
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
        // ### fixme: Qt 6: Never set Qt::WindowContextHelpButtonHint flag automatically
        if (!QApplicationPrivate::testAttribute(Qt::AA_DisableWindowContextHelpButton))
            flags |= Qt::WindowContextHelpButtonHint;
    } else if (type == Qt::Tool)
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
    else
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint |
                Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowFullscreenButtonHint;
    if (w->testAttribute(Qt::WA_TransparentForMouseEvents))
        flags |= Qt::WindowTransparentForInput;
}

void QWidgetPrivate::init(QWidget *parentWidget, Qt::WindowFlags f)
{
    Q_Q(QWidget);
    Q_ASSERT_X(q != parentWidget, Q_FUNC_INFO, "Cannot parent a QWidget to itself");

    if (Q_UNLIKELY(!qobject_cast<QApplication *>(QCoreApplication::instance())))
        qFatal("QWidget: Cannot create a QWidget without QApplication");

    Q_ASSERT(allWidgets);
    if (allWidgets)
        allWidgets->insert(q);

    int targetScreen = -1;
    if (parentWidget && parentWidget->windowType() == Qt::Desktop) {
        const QDesktopScreenWidget *sw = qobject_cast<const QDesktopScreenWidget *>(parentWidget);
        targetScreen = sw ? sw->screenNumber() : 0;
        parentWidget = nullptr;
    }

    q->data = &data;

#if QT_CONFIG(thread)
    if (!parent) {
        Q_ASSERT_X(q->thread() == qApp->thread(), "QWidget",
                   "Widgets must be created in the GUI thread.");
    }
#endif

    if (targetScreen >= 0) {
        topData()->initialScreenIndex = targetScreen;
        if (QWindow *window = q->windowHandle())
            window->setScreen(QGuiApplication::screens().value(targetScreen, nullptr));
    }

    data.fstrut_dirty = true;

    data.winid = 0;
    data.widget_attributes = 0;
    data.window_flags = f;
    data.window_state = 0;
    data.focus_policy = 0;
    data.context_menu_policy = Qt::DefaultContextMenu;
    data.window_modality = Qt::NonModal;

    data.sizehint_forced = 0;
    data.is_closing = 0;
    data.in_show = 0;
    data.in_set_window_state = 0;
    data.in_destructor = false;

    // Widgets with Qt::MSWindowsOwnDC (typically QGLWidget) must have a window handle.
    if (f & Qt::MSWindowsOwnDC) {
        mustHaveWindowHandle = 1;
        q->setAttribute(Qt::WA_NativeWindow);
    }

    q->setAttribute(Qt::WA_QuitOnClose); // might be cleared in adjustQuitOnCloseAttribute()
    adjustQuitOnCloseAttribute();

    q->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea);
    q->setAttribute(Qt::WA_WState_Hidden);

    //give potential windows a bigger "pre-initial" size; create() will give them a new size later
    data.crect = parentWidget ? QRect(0,0,100,30) : QRect(0,0,640,480);
    focus_next = focus_prev = q;

    if ((f & Qt::WindowType_Mask) == Qt::Desktop)
        q->create();
    else if (parentWidget)
        q->setParent(parentWidget, data.window_flags);
    else {
        adjustFlags(data.window_flags, q);
        resolveLayoutDirection();
        // opaque system background?
        const QBrush &background = q->palette().brush(QPalette::Window);
        setOpaque(q->isWindow() && background.style() != Qt::NoBrush && background.isOpaque());
    }
    data.fnt = QFont(data.fnt, q);

    q->setAttribute(Qt::WA_PendingMoveEvent);
    q->setAttribute(Qt::WA_PendingResizeEvent);

    if (++QWidgetPrivate::instanceCounter > QWidgetPrivate::maxInstances)
        QWidgetPrivate::maxInstances = QWidgetPrivate::instanceCounter;

    if (QApplicationPrivate::testAttribute(Qt::AA_ImmediateWidgetCreation)) // ### fixme: Qt 6: Remove AA_ImmediateWidgetCreation.
        q->create();

    QEvent e(QEvent::Create);
    QCoreApplication::sendEvent(q, &e);
    QCoreApplication::postEvent(q, new QEvent(QEvent::PolishRequest));

    extraPaintEngine = nullptr;
}

void QWidgetPrivate::createRecursively()
{
    Q_Q(QWidget);
    q->create(0, true, true);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (child && !child->isHidden() && !child->isWindow() && !child->testAttribute(Qt::WA_WState_Created))
            child->d_func()->createRecursively();
    }
}

QWindow *QWidgetPrivate::windowHandle(WindowHandleMode mode) const
{
    if (mode == WindowHandleMode::Direct || mode == WindowHandleMode::Closest) {
        if (QTLWExtra *x = maybeTopData()) {
            if (x->window != nullptr || mode == WindowHandleMode::Direct)
                return x->window;
        }
    }
    if (mode == WindowHandleMode::Closest) {
        if (auto nativeParent = q_func()->nativeParentWidget()) {
            if (auto window = nativeParent->windowHandle())
                return window;
        }
    }
    if (mode == WindowHandleMode::TopLevel || mode == WindowHandleMode::Closest) {
        if (auto topLevel = q_func()->topLevelWidget()) {
            if (auto window = topLevel ->windowHandle())
                return window;
        }
    }
    return nullptr;
}

QScreen *QWidgetPrivate::associatedScreen() const
{
    if (auto window = windowHandle(WindowHandleMode::Closest))
        return window->screen();
    return nullptr;
}

// ### fixme: Qt 6: Remove parameter window from QWidget::create()

/*!
    Creates a new widget window.

    The parameters \a window, \a initializeWindow, and \a destroyOldWindow
    are ignored in Qt 5. Please use QWindow::fromWinId() to create a
    QWindow wrapping a foreign window and pass it to
    QWidget::createWindowContainer() instead.

    \sa createWindowContainer(), QWindow::fromWinId()
*/

void QWidget::create(WId window, bool initializeWindow, bool destroyOldWindow)
{
    Q_UNUSED(initializeWindow);
    Q_UNUSED(destroyOldWindow);

    Q_D(QWidget);
    if (Q_UNLIKELY(window))
        qWarning("QWidget::create(): Parameter 'window' does not have any effect.");
    if (testAttribute(Qt::WA_WState_Created) && window == 0 && internalWinId())
        return;

    if (d->data.in_destructor)
        return;

    Qt::WindowType type = windowType();
    Qt::WindowFlags &flags = data->window_flags;

    if ((type == Qt::Widget || type == Qt::SubWindow) && !parentWidget()) {
        type = Qt::Window;
        flags |= Qt::Window;
    }

    if (QWidget *parent = parentWidget()) {
        if (type & Qt::Window) {
            if (!parent->testAttribute(Qt::WA_WState_Created))
                parent->createWinId();
        } else if (testAttribute(Qt::WA_NativeWindow) && !parent->internalWinId()
                   && !testAttribute(Qt::WA_DontCreateNativeAncestors)) {
            // We're about to create a native child widget that doesn't have a native parent;
            // enforce a native handle for the parent unless the Qt::WA_DontCreateNativeAncestors
            // attribute is set.
            d->createWinId();
            // Nothing more to do.
            Q_ASSERT(testAttribute(Qt::WA_WState_Created));
            Q_ASSERT(internalWinId());
            return;
        }
    }


    static const bool paintOnScreenEnv = qEnvironmentVariableIntValue("QT_ONSCREEN_PAINT") > 0;
    if (paintOnScreenEnv)
        setAttribute(Qt::WA_PaintOnScreen);

    if (QApplicationPrivate::testAttribute(Qt::AA_NativeWindows))
        setAttribute(Qt::WA_NativeWindow);

#ifdef ALIEN_DEBUG
    qDebug() << "QWidget::create:" << this << "parent:" << parentWidget()
             << "Alien?" << !testAttribute(Qt::WA_NativeWindow);
#endif

    d->updateIsOpaque();

    setAttribute(Qt::WA_WState_Created);                        // set created flag
    d->create();

    // A real toplevel window needs a paint manager
    if (isWindow() && windowType() != Qt::Desktop)
        d->topData()->repaintManager.reset(new QWidgetRepaintManager(this));

    d->setModal_sys();

    if (!isWindow() && parentWidget() && parentWidget()->testAttribute(Qt::WA_DropSiteRegistered))
        setAttribute(Qt::WA_DropSiteRegistered, true);

    // need to force the resting of the icon after changing parents
    if (testAttribute(Qt::WA_SetWindowIcon))
        d->setWindowIcon_sys();

    if (isWindow() && !d->topData()->iconText.isEmpty())
        d->setWindowIconText_helper(d->topData()->iconText);
    if (isWindow() && !d->topData()->caption.isEmpty())
        d->setWindowTitle_helper(d->topData()->caption);
    if (isWindow() && !d->topData()->filePath.isEmpty())
        d->setWindowFilePath_helper(d->topData()->filePath);
    if (windowType() != Qt::Desktop) {
        d->updateSystemBackground();

        if (isWindow() && !testAttribute(Qt::WA_SetWindowIcon))
            d->setWindowIcon_sys();
    }

    // Frame strut update needed in cases where there are native widgets such as QGLWidget,
    // as those force native window creation on their ancestors before they are shown.
    // If the strut is not updated, any subsequent move of the top level window before show
    // will cause window frame to be ignored when positioning the window.
    // Note that this only helps on platforms that handle window creation synchronously.
    d->updateFrameStrut();
}

void q_createNativeChildrenAndSetParent(const QWidget *parentWidget)
{
    QObjectList children = parentWidget->children();
    for (int i = 0; i < children.size(); i++) {
        if (children.at(i)->isWidgetType()) {
            const QWidget *childWidget = qobject_cast<const QWidget *>(children.at(i));
            if (childWidget) { // should not be necessary
                if (childWidget->testAttribute(Qt::WA_NativeWindow)) {
                    if (!childWidget->internalWinId())
                        childWidget->winId();
                    if (childWidget->windowHandle()) {
                        if (childWidget->isWindow()) {
                            childWidget->windowHandle()->setTransientParent(parentWidget->window()->windowHandle());
                        } else {
                            childWidget->windowHandle()->setParent(childWidget->nativeParentWidget()->windowHandle());
                        }
                    }
                } else {
                    q_createNativeChildrenAndSetParent(childWidget);
                }
            }
        }
    }

}

void QWidgetPrivate::create()
{
    Q_Q(QWidget);

    if (!q->testAttribute(Qt::WA_NativeWindow) && !q->isWindow())
        return; // we only care about real toplevels

    QWidgetWindow *win = topData()->window;
    // topData() ensures the extra is created but does not ensure 'window' is non-null
    // in case the extra was already valid.
    if (!win) {
        createTLSysExtra();
        win = topData()->window;
    }

    const auto dynamicPropertyNames = q->dynamicPropertyNames();
    for (const QByteArray &propertyName : dynamicPropertyNames) {
        if (!qstrncmp(propertyName, "_q_platform_", 12))
            win->setProperty(propertyName, q->property(propertyName));
    }

    Qt::WindowFlags &flags = data.window_flags;

#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
    if (q->testAttribute(Qt::WA_ContentsMarginsRespectsSafeArea))
        flags |= Qt::MaximizeUsingFullscreenGeometryHint;
#endif

    if (q->testAttribute(Qt::WA_ShowWithoutActivating))
        win->setProperty("_q_showWithoutActivating", QVariant(true));
    if (q->testAttribute(Qt::WA_MacAlwaysShowToolWindow))
        win->setProperty("_q_macAlwaysShowToolWindow", QVariant(true));
    setNetWmWindowTypes(true); // do nothing if none of WA_X11NetWmWindowType* is set
    win->setFlags(flags);
    fixPosIncludesFrame();
    if (q->testAttribute(Qt::WA_Moved)
        || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowManagement))
        win->setGeometry(q->geometry());
    else
        win->resize(q->size());
    if (win->isTopLevel()) {
        int screenNumber = topData()->initialScreenIndex;
        topData()->initialScreenIndex = -1;
        if (screenNumber < 0) {
            screenNumber = q->windowType() != Qt::Desktop
                ? QDesktopWidgetPrivate::screenNumber(q) : 0;
        }
        win->setScreen(QGuiApplication::screens().value(screenNumber, nullptr));
    }

    QSurfaceFormat format = win->requestedFormat();
    if ((flags & Qt::Window) && win->surfaceType() != QSurface::OpenGLSurface
            && q->testAttribute(Qt::WA_TranslucentBackground)) {
        format.setAlphaBufferSize(8);
    }
    win->setFormat(format);

    if (QWidget *nativeParent = q->nativeParentWidget()) {
        if (nativeParent->windowHandle()) {
            if (flags & Qt::Window) {
                win->setTransientParent(nativeParent->window()->windowHandle());
                win->setParent(nullptr);
            } else {
                win->setTransientParent(nullptr);
                win->setParent(nativeParent->windowHandle());
            }
        }
    }

    qt_window_private(win)->positionPolicy = topData()->posIncludesFrame ?
        QWindowPrivate::WindowFrameInclusive : QWindowPrivate::WindowFrameExclusive;

    if (q->windowType() != Qt::Desktop || q->testAttribute(Qt::WA_NativeWindow)) {
        win->create();
        // Enable nonclient-area events for QDockWidget and other NonClientArea-mouse event processing.
        if (QPlatformWindow *platformWindow = win->handle())
            platformWindow->setFrameStrutEventsEnabled(true);
    }

    data.window_flags = win->flags();
    if (!win->isTopLevel()) // In a Widget world foreign windows can only be top level
      data.window_flags &= ~Qt::ForeignWindow;

    if (!topData()->role.isNull())
        QXcbWindowFunctions::setWmWindowRole(win, topData()->role.toLatin1());

    QBackingStore *store = q->backingStore();

    if (!store) {
        if (q->windowType() != Qt::Desktop) {
            if (q->isTopLevel())
                q->setBackingStore(new QBackingStore(win));
        } else {
            q->setAttribute(Qt::WA_PaintOnScreen, true);
        }
    }

    setWindowModified_helper();

    if (win->handle()) {
        WId id = win->winId();
        // See the QPlatformWindow::winId() documentation
        Q_ASSERT(id != WId(0));
        setWinId(id);
    }

    // Check children and create windows for them if necessary
    q_createNativeChildrenAndSetParent(q);

    if (extra && !extra->mask.isEmpty())
        setMask_sys(extra->mask);

    if (data.crect.width() == 0 || data.crect.height() == 0) {
        q->setAttribute(Qt::WA_OutsideWSRange, true);
    } else if (q->isVisible()) {
        // If widget is already shown, set window visible, too
        win->setNativeWindowVisibility(true);
    }
}

#ifdef Q_OS_WIN
static const char activeXNativeParentHandleProperty[] = "_q_embedded_native_parent_handle";
#endif

void QWidgetPrivate::createTLSysExtra()
{
    Q_Q(QWidget);
    if (!extra->topextra->window && (q->testAttribute(Qt::WA_NativeWindow) || q->isWindow())) {
        extra->topextra->window = new QWidgetWindow(q);
        if (extra->minw || extra->minh)
            extra->topextra->window->setMinimumSize(QSize(extra->minw, extra->minh));
        if (extra->maxw != QWIDGETSIZE_MAX || extra->maxh != QWIDGETSIZE_MAX)
            extra->topextra->window->setMaximumSize(QSize(extra->maxw, extra->maxh));
        if (extra->topextra->opacity != 255 && q->isWindow())
            extra->topextra->window->setOpacity(qreal(extra->topextra->opacity) / qreal(255));

        const bool isTipLabel = q->inherits("QTipLabel");
        const bool isAlphaWidget = !isTipLabel && q->inherits("QAlphaWidget");
#ifdef Q_OS_WIN
        // Pass on native parent handle for Widget embedded into Active X.
        const QVariant activeXNativeParentHandle = q->property(activeXNativeParentHandleProperty);
        if (activeXNativeParentHandle.isValid())
            extra->topextra->window->setProperty(activeXNativeParentHandleProperty, activeXNativeParentHandle);
        if (isTipLabel || isAlphaWidget)
            extra->topextra->window->setProperty("_q_windowsDropShadow", QVariant(true));
#endif
        if (isTipLabel || isAlphaWidget || q->inherits("QRollEffect"))
            qt_window_private(extra->topextra->window)->setAutomaticPositionAndResizeEnabled(false);
    }

}

/*!
    Destroys the widget.

    All this widget's children are deleted first. The application
    exits if this widget is the main widget.
*/

QWidget::~QWidget()
{
    Q_D(QWidget);
    d->data.in_destructor = true;

#if defined (QT_CHECK_STATE)
    if (Q_UNLIKELY(paintingActive()))
        qWarning("QWidget: %s (%s) deleted while being painted", className(), name());
#endif

#ifndef QT_NO_GESTURES
    if (QGestureManager *manager = QGestureManager::instance(QGestureManager::DontForceCreation)) {
        // \forall Qt::GestureType type : ungrabGesture(type) (inlined)
        for (auto it = d->gestureContext.keyBegin(), end = d->gestureContext.keyEnd(); it != end; ++it)
            manager->cleanupCachedGestures(this, *it);
    }
    d->gestureContext.clear();
#endif

#ifndef QT_NO_ACTION
    // remove all actions from this widget
    for (int i = 0; i < d->actions.size(); ++i) {
        QActionPrivate *apriv = d->actions.at(i)->d_func();
        apriv->widgets.removeAll(this);
    }
    d->actions.clear();
#endif

#ifndef QT_NO_SHORTCUT
    // Remove all shortcuts grabbed by this
    // widget, unless application is closing
    if (!QApplicationPrivate::is_app_closing && testAttribute(Qt::WA_GrabbedShortcut))
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(0, this, QKeySequence());
#endif

    // delete layout while we still are a valid widget
    delete d->layout;
    d->layout = nullptr;
    // Remove myself from focus list

    Q_ASSERT(d->focus_next->d_func()->focus_prev == this);
    Q_ASSERT(d->focus_prev->d_func()->focus_next == this);

    if (d->focus_next != this) {
        d->focus_next->d_func()->focus_prev = d->focus_prev;
        d->focus_prev->d_func()->focus_next = d->focus_next;
        d->focus_next = d->focus_prev = nullptr;
    }


    QT_TRY {
#if QT_CONFIG(graphicsview)
        const QWidget* w = this;
        while (w->d_func()->extra && w->d_func()->extra->focus_proxy)
            w = w->d_func()->extra->focus_proxy;
        QWidget *window = w->window();
        QWExtra *e = window ? window->d_func()->extra.get() : nullptr ;
        if (!e || !e->proxyWidget || (w->parentWidget() && w->parentWidget()->d_func()->focus_child == this))
#endif
        clearFocus();
    } QT_CATCH(...) {
        // swallow this problem because we are in a destructor
    }

    d->setDirtyOpaqueRegion();

    if (isWindow() && isVisible() && internalWinId()) {
        QT_TRY {
            d->close_helper(QWidgetPrivate::CloseNoEvent);
        } QT_CATCH(...) {
            // if we're out of memory, at least hide the window.
            QT_TRY {
                hide();
            } QT_CATCH(...) {
                // and if that also doesn't work, then give up
            }
        }
    } else if (isVisible()) {
        qApp->d_func()->sendSyntheticEnterLeave(this);
    }

    if (QWidgetRepaintManager *repaintManager = d->maybeRepaintManager()) {
        repaintManager->removeDirtyWidget(this);
        if (testAttribute(Qt::WA_StaticContents))
            repaintManager->removeStaticWidget(this);
    }

    delete d->needsFlush;
    d->needsFlush = nullptr;

    // The next 20 lines are duplicated from QObject, but required here
    // since QWidget deletes is children itself
    bool blocked = d->blockSig;
    d->blockSig = 0; // unblock signals so we always emit destroyed()

    if (d->isSignalConnected(0)) {
        QT_TRY {
            emit destroyed(this);
        } QT_CATCH(...) {
            // all the signal/slots connections are still in place - if we don't
            // quit now, we will crash pretty soon.
            qWarning("Detected an unexpected exception in ~QWidget while emitting destroyed().");
            QT_RETHROW;
        }
    }

    if (d->declarativeData) {
        d->wasDeleted = true; // needed, so that destroying the declarative data does the right thing
        if (static_cast<QAbstractDeclarativeDataImpl*>(d->declarativeData)->ownedByQml1) {
            if (QAbstractDeclarativeData::destroyed_qml1)
                QAbstractDeclarativeData::destroyed_qml1(d->declarativeData, this);
        } else {
            if (QAbstractDeclarativeData::destroyed)
                QAbstractDeclarativeData::destroyed(d->declarativeData, this);
        }
        d->declarativeData = nullptr;                 // don't activate again in ~QObject
        d->wasDeleted = false;
    }

    d->blockSig = blocked;

    if (!d->children.isEmpty())
        d->deleteChildren();

    QCoreApplication::removePostedEvents(this);

    QT_TRY {
        destroy();                                        // platform-dependent cleanup
    } QT_CATCH(...) {
        // if this fails we can't do anything about it but at least we are not allowed to throw.
    }
    --QWidgetPrivate::instanceCounter;

    if (QWidgetPrivate::allWidgets) // might have been deleted by ~QApplication
        QWidgetPrivate::allWidgets->remove(this);

    QT_TRY {
        QEvent e(QEvent::Destroy);
        QCoreApplication::sendEvent(this, &e);
    } QT_CATCH(const std::exception&) {
        // if this fails we can't do anything about it but at least we are not allowed to throw.
    }

#if QT_CONFIG(graphicseffect)
    delete d->graphicsEffect;
#endif
}

int QWidgetPrivate::instanceCounter = 0;  // Current number of widget instances
int QWidgetPrivate::maxInstances = 0;     // Maximum number of widget instances

void QWidgetPrivate::setWinId(WId id)                // set widget identifier
{
    Q_Q(QWidget);
    // the user might create a widget with Qt::Desktop window
    // attribute (or create another QDesktopWidget instance), which
    // will have the same windowid (the root window id) as the
    // qt_desktopWidget. We should not add the second desktop widget
    // to the mapper.
    bool userDesktopWidget = qt_desktopWidget != nullptr && qt_desktopWidget != q && q->windowType() == Qt::Desktop;
    if (mapper && data.winid && !userDesktopWidget) {
        mapper->remove(data.winid);
    }

    const WId oldWinId = data.winid;

    data.winid = id;
    if (mapper && id && !userDesktopWidget) {
        mapper->insert(data.winid, q);
    }

    if(oldWinId != id) {
        QEvent e(QEvent::WinIdChange);
        QCoreApplication::sendEvent(q, &e);
    }
}

void QWidgetPrivate::createTLExtra()
{
    if (!extra)
        createExtra();
    if (!extra->topextra) {
        extra->topextra = qt_make_unique<QTLWExtra>();
        QTLWExtra* x = extra->topextra.get();
        x->backingStore = nullptr;
        x->sharedPainter = nullptr;
        x->incw = x->inch = 0;
        x->basew = x->baseh = 0;
        x->frameStrut.setCoords(0, 0, 0, 0);
        x->normalGeometry = QRect(0,0,-1,-1);
        x->savedFlags = { };
        x->opacity = 255;
        x->posIncludesFrame = 0;
        x->sizeAdjusted = false;
        x->embedded = 0;
        x->window = nullptr;
        x->initialScreenIndex = -1;

#ifdef QWIDGET_EXTRA_DEBUG
        static int count = 0;
        qDebug() << "tlextra" << ++count;
#endif
    }
}

/*!
  \internal
  Creates the widget extra data.
*/

void QWidgetPrivate::createExtra()
{
    if (!extra) {                                // if not exists
        extra = qt_make_unique<QWExtra>();
        extra->glContext = nullptr;
#if QT_CONFIG(graphicsview)
        extra->proxyWidget = nullptr;
#endif
        extra->minw = 0;
        extra->minh = 0;
        extra->maxw = QWIDGETSIZE_MAX;
        extra->maxh = QWIDGETSIZE_MAX;
        extra->customDpiX = 0;
        extra->customDpiY = 0;
        extra->explicitMinSize = 0;
        extra->explicitMaxSize = 0;
        extra->autoFillBackground = 0;
        extra->nativeChildrenForced = 0;
        extra->inRenderWithPainter = 0;
        extra->hasWindowContainer = false;
        extra->hasMask = 0;
        createSysExtra();
#ifdef QWIDGET_EXTRA_DEBUG
        static int count = 0;
        qDebug() << "extra" << ++count;
#endif
    }
}

void QWidgetPrivate::createSysExtra()
{
}

/*!
  \internal
  Deletes the widget extra data.
*/

void QWidgetPrivate::deleteExtra()
{
    if (extra) {                                // if exists
        deleteSysExtra();
#ifndef QT_NO_STYLE_STYLESHEET
        // dereference the stylesheet style
        if (QStyleSheetStyle *proxy = qt_styleSheet(extra->style))
            proxy->deref();
#endif
        if (extra->topextra) {
            deleteTLSysExtra();
            // extra->topextra->backingStore destroyed in QWidgetPrivate::deleteTLSysExtra()
        }
        // extra->xic destroyed in QWidget::destroy()
        extra.reset();
    }
}

void QWidgetPrivate::deleteSysExtra()
{
}

static void deleteBackingStore(QWidgetPrivate *d)
{
    QTLWExtra *topData = d->topData();

    delete topData->backingStore;
    topData->backingStore = nullptr;
}

void QWidgetPrivate::deleteTLSysExtra()
{
    if (extra && extra->topextra) {
        //the qplatformbackingstore may hold a reference to the window, so the backingstore
        //needs to be deleted first.

        extra->topextra->repaintManager.reset(nullptr);
        deleteBackingStore(this);
#ifndef QT_NO_OPENGL
        extra->topextra->widgetTextures.clear();
        extra->topextra->shareContext.reset();
#endif

        //the toplevel might have a context with a "qglcontext associated with it. We need to
        //delete the qglcontext before we delete the qplatformopenglcontext.
        //One unfortunate thing about this is that we potentially create a glContext just to
        //delete it straight afterwards.
        if (extra->topextra->window) {
            extra->topextra->window->destroy();
        }
        delete extra->topextra->window;
        extra->topextra->window = nullptr;

    }
}

/*
  Returns \c region of widgets above this which overlap with
  \a rect, which is in parent's coordinate system (same as crect).
*/

QRegion QWidgetPrivate::overlappedRegion(const QRect &rect, bool breakAfterFirst) const
{
    Q_Q(const QWidget);

    const QWidget *w = q;
    QRect r = rect;
    QPoint p;
    QRegion region;
    while (w) {
        if (w->isWindow())
            break;
        QWidgetPrivate *pd = w->parentWidget()->d_func();
        bool above = false;
        for (int i = 0; i < pd->children.size(); ++i) {
            QWidget *sibling = qobject_cast<QWidget *>(pd->children.at(i));
            if (!sibling || !sibling->isVisible() || sibling->isWindow())
                continue;
            if (!above) {
                above = (sibling == w);
                continue;
            }

            const QRect siblingRect = sibling->d_func()->effectiveRectFor(sibling->data->crect);
            if (qRectIntersects(siblingRect, r)) {
                const auto &siblingExtra = sibling->d_func()->extra;
                if (siblingExtra && siblingExtra->hasMask && !sibling->d_func()->graphicsEffect
                    && !siblingExtra->mask.translated(sibling->data->crect.topLeft()).intersects(r)) {
                    continue;
                }
                region += siblingRect.translated(-p);
                if (breakAfterFirst)
                    break;
            }
        }
        w = w->parentWidget();
        r.translate(pd->data.crect.topLeft());
        p += pd->data.crect.topLeft();
    }
    return region;
}

void QWidgetPrivate::syncBackingStore()
{
    if (shouldPaintOnScreen()) {
        paintOnScreen(dirty);
        dirty = QRegion();
    } else if (QWidgetRepaintManager *repaintManager = maybeRepaintManager()) {
        repaintManager->sync();
    }
}

void QWidgetPrivate::syncBackingStore(const QRegion &region)
{
    if (shouldPaintOnScreen())
        paintOnScreen(region);
    else if (QWidgetRepaintManager *repaintManager = maybeRepaintManager()) {
        repaintManager->sync(q_func(), region);
    }
}

void QWidgetPrivate::paintOnScreen(const QRegion &rgn)
{
    if (data.in_destructor)
        return;

    if (shouldDiscardSyncRequest())
        return;

    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_StaticContents)) {
        if (!extra)
            createExtra();
        extra->staticContentsSize = data.crect.size();
    }

    QPaintEngine *engine = q->paintEngine();

    // QGLWidget does not support partial updates if:
    // 1) The context is double buffered
    // 2) The context is single buffered and auto-fill background is enabled.
    const bool noPartialUpdateSupport = (engine && (engine->type() == QPaintEngine::OpenGL
                                                || engine->type() == QPaintEngine::OpenGL2))
                                        && (usesDoubleBufferedGLContext || q->autoFillBackground());
    QRegion toBePainted(noPartialUpdateSupport ? q->rect() : rgn);

    toBePainted &= clipRect();
    clipToEffectiveMask(toBePainted);
    if (toBePainted.isEmpty())
        return; // Nothing to repaint.

    drawWidget(q, toBePainted, QPoint(), QWidgetPrivate::DrawAsRoot | QWidgetPrivate::DrawPaintOnScreen, nullptr);

    if (Q_UNLIKELY(q->paintingActive()))
        qWarning("QWidget::repaint: It is dangerous to leave painters active on a widget outside of the PaintEvent");
}

void QWidgetPrivate::setUpdatesEnabled_helper(bool enable)
{
    Q_Q(QWidget);

    if (enable && !q->isWindow() && q->parentWidget() && !q->parentWidget()->updatesEnabled())
        return; // nothing we can do

    if (enable != q->testAttribute(Qt::WA_UpdatesDisabled))
        return; // nothing to do

    q->setAttribute(Qt::WA_UpdatesDisabled, !enable);
    if (enable)
        q->update();

    Qt::WidgetAttribute attribute = enable ? Qt::WA_ForceUpdatesDisabled : Qt::WA_UpdatesDisabled;
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && !w->isWindow() && !w->testAttribute(attribute))
            w->d_func()->setUpdatesEnabled_helper(enable);
    }
}

/*!
    \internal

    Propagate this widget's palette to all children, except style sheet
    widgets, and windows that don't enable window propagation (palettes don't
    normally propagate to windows).
*/
void QWidgetPrivate::propagatePaletteChange()
{
    Q_Q(QWidget);
    // Propagate a new inherited mask to all children.
#if QT_CONFIG(graphicsview)
    if (!q->parentWidget() && extra && extra->proxyWidget) {
        QGraphicsProxyWidget *p = extra->proxyWidget;
        inheritedPaletteResolveMask = p->d_func()->inheritedPaletteResolveMask | p->palette().resolve();
    } else
#endif // QT_CONFIG(graphicsview)
        if (q->isWindow() && !q->testAttribute(Qt::WA_WindowPropagation)) {
        inheritedPaletteResolveMask = 0;
    }

    directPaletteResolveMask = data.pal.resolve();
    auto mask = directPaletteResolveMask | inheritedPaletteResolveMask;

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    QEvent pc(QEvent::PaletteChange);
    QCoreApplication::sendEvent(q, &pc);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget*>(children.at(i));
        if (w && (!w->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles)
            && (!w->isWindow() || w->testAttribute(Qt::WA_WindowPropagation))) {
            QWidgetPrivate *wd = w->d_func();
            wd->inheritedPaletteResolveMask = mask;
            wd->resolvePalette();
        }
    }
}

/*
  Returns the widget's clipping rectangle.
*/
QRect QWidgetPrivate::clipRect() const
{
    Q_Q(const QWidget);
    const QWidget * w = q;
    if (!w->isVisible())
        return QRect();
    QRect r = effectiveRectFor(q->rect());
    int ox = 0;
    int oy = 0;
    while (w
            && w->isVisible()
            && !w->isWindow()
            && w->parentWidget()) {
        ox -= w->x();
        oy -= w->y();
        w = w->parentWidget();
        r &= QRect(ox, oy, w->width(), w->height());
    }
    return r;
}

/*
  Returns the widget's clipping region (without siblings).
*/
QRegion QWidgetPrivate::clipRegion() const
{
    Q_Q(const QWidget);
    if (!q->isVisible())
        return QRegion();
    QRegion r(q->rect());
    const QWidget * w = q;
    const QWidget *ignoreUpTo;
    int ox = 0;
    int oy = 0;
    while (w
           && w->isVisible()
           && !w->isWindow()
           && w->parentWidget()) {
        ox -= w->x();
        oy -= w->y();
        ignoreUpTo = w;
        w = w->parentWidget();
        r &= QRegion(ox, oy, w->width(), w->height());

        int i = 0;
        while(w->d_func()->children.at(i++) != static_cast<const QObject *>(ignoreUpTo))
            ;
        for ( ; i < w->d_func()->children.size(); ++i) {
            if(QWidget *sibling = qobject_cast<QWidget *>(w->d_func()->children.at(i))) {
                if(sibling->isVisible() && !sibling->isWindow()) {
                    QRect siblingRect(ox+sibling->x(), oy+sibling->y(),
                                      sibling->width(), sibling->height());
                    if (qRectIntersects(siblingRect, q->rect()))
                        r -= QRegion(siblingRect);
                }
            }
        }
    }
    return r;
}

void QWidgetPrivate::setSystemClip(QPaintEngine *paintEngine, qreal devicePixelRatio, const QRegion &region)
{
// Transform the system clip region from device-independent pixels to device pixels
    QTransform scaleTransform;
    scaleTransform.scale(devicePixelRatio, devicePixelRatio);

    paintEngine->d_func()->baseSystemClip = region;
    paintEngine->d_func()->setSystemTransform(scaleTransform);

}

#if QT_CONFIG(graphicseffect)
void QWidgetPrivate::invalidateGraphicsEffectsRecursively()
{
    Q_Q(QWidget);
    QWidget *w = q;
    do {
        if (w->graphicsEffect()) {
            QWidgetEffectSourcePrivate *sourced =
                static_cast<QWidgetEffectSourcePrivate *>(w->graphicsEffect()->source()->d_func());
            if (!sourced->updateDueToGraphicsEffect)
                w->graphicsEffect()->source()->d_func()->invalidateCache();
        }
        w = w->parentWidget();
    } while (w);
}
#endif // QT_CONFIG(graphicseffect)

void QWidgetPrivate::setDirtyOpaqueRegion()
{
    Q_Q(QWidget);

    dirtyOpaqueChildren = true;

#if QT_CONFIG(graphicseffect)
    invalidateGraphicsEffectsRecursively();
#endif // QT_CONFIG(graphicseffect)

    if (q->isWindow())
        return;

    QWidget *parent = q->parentWidget();
    if (!parent)
        return;

    // TODO: instead of setting dirtyflag, manipulate the dirtyregion directly?
    QWidgetPrivate *pd = parent->d_func();
    if (!pd->dirtyOpaqueChildren)
        pd->setDirtyOpaqueRegion();
}

const QRegion &QWidgetPrivate::getOpaqueChildren() const
{
    if (!dirtyOpaqueChildren)
        return opaqueChildren;

    QWidgetPrivate *that = const_cast<QWidgetPrivate*>(this);
    that->opaqueChildren = QRegion();

    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (!child || !child->isVisible() || child->isWindow())
            continue;

        const QPoint offset = child->geometry().topLeft();
        QWidgetPrivate *childd = child->d_func();
        QRegion r = childd->isOpaque ? child->rect() : childd->getOpaqueChildren();
        if (childd->extra && childd->extra->hasMask)
            r &= childd->extra->mask;
        if (r.isEmpty())
            continue;
        r.translate(offset);
        that->opaqueChildren += r;
    }

    that->opaqueChildren &= q_func()->rect();
    that->dirtyOpaqueChildren = false;

    return that->opaqueChildren;
}

void QWidgetPrivate::subtractOpaqueChildren(QRegion &source, const QRect &clipRect) const
{
    if (children.isEmpty() || clipRect.isEmpty())
        return;

    const QRegion &r = getOpaqueChildren();
    if (!r.isEmpty())
        source -= (r & clipRect);
}

//subtract any relatives that are higher up than me --- this is too expensive !!!
void QWidgetPrivate::subtractOpaqueSiblings(QRegion &sourceRegion, bool *hasDirtySiblingsAbove,
                                            bool alsoNonOpaque) const
{
    Q_Q(const QWidget);
    static int disableSubtractOpaqueSiblings = qEnvironmentVariableIntValue("QT_NO_SUBTRACTOPAQUESIBLINGS");
    if (disableSubtractOpaqueSiblings || q->isWindow())
        return;

    QRect clipBoundingRect;
    bool dirtyClipBoundingRect = true;

    QRegion parentClip;
    bool dirtyParentClip = true;

    QPoint parentOffset = data.crect.topLeft();

    const QWidget *w = q;

    while (w) {
        if (w->isWindow())
            break;
        QWidgetPrivate *pd = w->parentWidget()->d_func();
        const int myIndex = pd->children.indexOf(const_cast<QWidget *>(w));
        const QRect widgetGeometry = w->d_func()->effectiveRectFor(w->data->crect);
        for (int i = myIndex + 1; i < pd->children.size(); ++i) {
            QWidget *sibling = qobject_cast<QWidget *>(pd->children.at(i));
            if (!sibling || !sibling->isVisible() || sibling->isWindow())
                continue;

            const QRect siblingGeometry = sibling->d_func()->effectiveRectFor(sibling->data->crect);
            if (!qRectIntersects(siblingGeometry, widgetGeometry))
                continue;

            if (dirtyClipBoundingRect) {
                clipBoundingRect = sourceRegion.boundingRect();
                dirtyClipBoundingRect = false;
            }

            if (!qRectIntersects(siblingGeometry, clipBoundingRect.translated(parentOffset)))
                continue;

            if (dirtyParentClip) {
                parentClip = sourceRegion.translated(parentOffset);
                dirtyParentClip = false;
            }

            const QPoint siblingPos(sibling->data->crect.topLeft());
            const QRect siblingClipRect(sibling->d_func()->clipRect());
            QRegion siblingDirty(parentClip);
            siblingDirty &= (siblingClipRect.translated(siblingPos));
            const bool hasMask = sibling->d_func()->extra && sibling->d_func()->extra->hasMask
                                 && !sibling->d_func()->graphicsEffect;
            if (hasMask)
                siblingDirty &= sibling->d_func()->extra->mask.translated(siblingPos);
            if (siblingDirty.isEmpty())
                continue;

            if (sibling->d_func()->isOpaque || alsoNonOpaque) {
                if (hasMask) {
                    siblingDirty.translate(-parentOffset);
                    sourceRegion -= siblingDirty;
                } else {
                    sourceRegion -= siblingGeometry.translated(-parentOffset);
                }
            } else {
                if (hasDirtySiblingsAbove)
                    *hasDirtySiblingsAbove = true;
                if (sibling->d_func()->children.isEmpty())
                    continue;
                QRegion opaqueSiblingChildren(sibling->d_func()->getOpaqueChildren());
                opaqueSiblingChildren.translate(-parentOffset + siblingPos);
                sourceRegion -= opaqueSiblingChildren;
            }
            if (sourceRegion.isEmpty())
                return;

            dirtyClipBoundingRect = true;
            dirtyParentClip = true;
        }

        w = w->parentWidget();
        parentOffset += pd->data.crect.topLeft();
        dirtyParentClip = true;
    }
}

void QWidgetPrivate::clipToEffectiveMask(QRegion &region) const
{
    Q_Q(const QWidget);

    const QWidget *w = q;
    QPoint offset;

#if QT_CONFIG(graphicseffect)
    if (graphicsEffect) {
        w = q->parentWidget();
        offset -= data.crect.topLeft();
    }
#endif // QT_CONFIG(graphicseffect)

    while (w) {
        const QWidgetPrivate *wd = w->d_func();
        if (wd->extra && wd->extra->hasMask)
            region &= (w != q) ? wd->extra->mask.translated(offset) : wd->extra->mask;
        if (w->isWindow())
            return;
        offset -= wd->data.crect.topLeft();
        w = w->parentWidget();
    }
}

bool QWidgetPrivate::shouldPaintOnScreen() const
{
#if defined(QT_NO_BACKINGSTORE)
    return true;
#else
    Q_Q(const QWidget);
    if (q->testAttribute(Qt::WA_PaintOnScreen)
            || (!q->isWindow() && q->window()->testAttribute(Qt::WA_PaintOnScreen))) {
        return true;
    }

    return false;
#endif
}

void QWidgetPrivate::updateIsOpaque()
{
    // hw: todo: only needed if opacity actually changed
    setDirtyOpaqueRegion();

#if QT_CONFIG(graphicseffect)
    if (graphicsEffect) {
        // ### We should probably add QGraphicsEffect::isOpaque at some point.
        setOpaque(false);
        return;
    }
#endif // QT_CONFIG(graphicseffect)

    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_OpaquePaintEvent) || q->testAttribute(Qt::WA_PaintOnScreen)) {
        setOpaque(true);
        return;
    }

    const QPalette &pal = q->palette();

    if (q->autoFillBackground()) {
        const QBrush &autoFillBrush = pal.brush(q->backgroundRole());
        if (autoFillBrush.style() != Qt::NoBrush && autoFillBrush.isOpaque()) {
            setOpaque(true);
            return;
        }
    }

    if (q->isWindow() && !q->testAttribute(Qt::WA_NoSystemBackground)) {
        const QBrush &windowBrush = q->palette().brush(QPalette::Window);
        if (windowBrush.style() != Qt::NoBrush && windowBrush.isOpaque()) {
            setOpaque(true);
            return;
        }
    }
    setOpaque(false);
}

void QWidgetPrivate::setOpaque(bool opaque)
{
    if (isOpaque != opaque) {
        isOpaque = opaque;
        updateIsTranslucent();
    }
}

void QWidgetPrivate::updateIsTranslucent()
{
    Q_Q(QWidget);
    if (QWindow *window = q->windowHandle()) {
        QSurfaceFormat format = window->format();
        const int oldAlpha = format.alphaBufferSize();
        const int newAlpha = q->testAttribute(Qt::WA_TranslucentBackground)? 8 : 0;
        if (oldAlpha != newAlpha) {
            format.setAlphaBufferSize(newAlpha);
            window->setFormat(format);
        }
    }
}

static inline void fillRegion(QPainter *painter, const QRegion &rgn, const QBrush &brush)
{
    Q_ASSERT(painter);

    if (brush.style() == Qt::TexturePattern) {
        const QRect rect(rgn.boundingRect());
        painter->setClipRegion(rgn);
        painter->drawTiledPixmap(rect, brush.texture(), rect.topLeft());
    } else if (brush.gradient()
               && (brush.gradient()->coordinateMode() == QGradient::ObjectBoundingMode
                   || brush.gradient()->coordinateMode() == QGradient::ObjectMode)) {
        painter->save();
        painter->setClipRegion(rgn);
        painter->fillRect(0, 0, painter->device()->width(), painter->device()->height(), brush);
        painter->restore();
    } else {
        for (const QRect &rect : rgn)
            painter->fillRect(rect, brush);
    }
}

bool QWidgetPrivate::updateBrushOrigin(QPainter *painter, const QBrush &brush) const
{
#if QT_CONFIG(scrollarea)
    Q_Q(const QWidget);
    //If we are painting the viewport of a scrollarea, we must apply an offset to the brush in case we are drawing a texture
    if (brush.style() == Qt::NoBrush || brush.style() == Qt::SolidPattern)
        return false;
    QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(parent);
    if (scrollArea && scrollArea->viewport() == q) {
        QObjectData *scrollPrivate = static_cast<QWidget *>(scrollArea)->d_ptr.data();
        QAbstractScrollAreaPrivate *priv = static_cast<QAbstractScrollAreaPrivate *>(scrollPrivate);
        painter->setBrushOrigin(-priv->contentsOffset());
    }
#endif // QT_CONFIG(scrollarea)
    return true;
}

void QWidgetPrivate::paintBackground(QPainter *painter, const QRegion &rgn, DrawWidgetFlags flags) const
{
    Q_Q(const QWidget);

    bool brushOriginSet = false;
    const QBrush autoFillBrush = q->palette().brush(q->backgroundRole());

    if ((flags & DrawAsRoot) && !(q->autoFillBackground() && autoFillBrush.isOpaque())) {
        const QBrush bg = q->palette().brush(QPalette::Window);
        if (!brushOriginSet)
            brushOriginSet = updateBrushOrigin(painter, bg);
        if (!(flags & DontSetCompositionMode)) {
            //copy alpha straight in
            QPainter::CompositionMode oldMode = painter->compositionMode();
            painter->setCompositionMode(QPainter::CompositionMode_Source);
            fillRegion(painter, rgn, bg);
            painter->setCompositionMode(oldMode);
        } else {
            fillRegion(painter, rgn, bg);
        }
    }

    if (q->autoFillBackground()) {
        if (!brushOriginSet)
            brushOriginSet = updateBrushOrigin(painter, autoFillBrush);
        fillRegion(painter, rgn, autoFillBrush);
    }

    if (q->testAttribute(Qt::WA_StyledBackground)) {
        painter->setClipRegion(rgn);
        QStyleOption opt;
        opt.initFrom(q);
        q->style()->drawPrimitive(QStyle::PE_Widget, &opt, painter, q);
    }
}

/*
  \internal
  This function is called when a widget is hidden or destroyed.
  It resets some application global pointers that should only refer active,
  visible widgets.
*/

extern QWidget *qt_button_down;

void QWidgetPrivate::deactivateWidgetCleanup()
{
    Q_Q(QWidget);
    // If this was the active application window, reset it
    if (QApplication::activeWindow() == q)
        QApplication::setActiveWindow(nullptr);
    // If the is the active mouse press widget, reset it
    if (q == qt_button_down)
        qt_button_down = nullptr;
}


/*!
    Returns a pointer to the widget with window identifer/handle \a
    id.

    The window identifier type depends on the underlying window
    system, see \c qwindowdefs.h for the actual definition. If there
    is no widget with this identifier, \nullptr is returned.
*/

QWidget *QWidget::find(WId id)
{
    return QWidgetPrivate::mapper ? QWidgetPrivate::mapper->value(id, 0) : nullptr;
}



/*!
    \fn WId QWidget::internalWinId() const
    \internal
    Returns the window system identifier of the widget, or 0 if the widget is not created yet.

*/

/*!
    \fn WId QWidget::winId() const

    Returns the window system identifier of the widget.

    Portable in principle, but if you use it you are probably about to
    do something non-portable. Be careful.

    If a widget is non-native (alien) and winId() is invoked on it, that widget
    will be provided a native handle.

    This value may change at run-time. An event with type QEvent::WinIdChange
    will be sent to the widget following a change in window system identifier.

    \sa find()
*/
WId QWidget::winId() const
{
    if (!data->in_destructor
        && (!testAttribute(Qt::WA_WState_Created) || !internalWinId()))
    {
#ifdef ALIEN_DEBUG
        qDebug() << "QWidget::winId: creating native window for" << this;
#endif
        QWidget *that = const_cast<QWidget*>(this);
        that->setAttribute(Qt::WA_NativeWindow);
        that->d_func()->createWinId();
        return that->data->winid;
    }
    return data->winid;
}

void QWidgetPrivate::createWinId()
{
    Q_Q(QWidget);

#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::createWinId for" << q;
#endif
    const bool forceNativeWindow = q->testAttribute(Qt::WA_NativeWindow);
    if (!q->testAttribute(Qt::WA_WState_Created) || (forceNativeWindow && !q->internalWinId())) {
        if (!q->isWindow()) {
            QWidget *parent = q->parentWidget();
            QWidgetPrivate *pd = parent->d_func();
            if (forceNativeWindow && !q->testAttribute(Qt::WA_DontCreateNativeAncestors))
                parent->setAttribute(Qt::WA_NativeWindow);
            if (!parent->internalWinId()) {
                pd->createWinId();
            }

            for (int i = 0; i < pd->children.size(); ++i) {
                QWidget *w = qobject_cast<QWidget *>(pd->children.at(i));
                if (w && !w->isWindow() && (!w->testAttribute(Qt::WA_WState_Created)
                                            || (!w->internalWinId() && w->testAttribute(Qt::WA_NativeWindow)))) {
                    w->create();
                }
            }
        } else {
            q->create();
        }
    }
}

/*!
\internal
Ensures that the widget is set on the screen point is on. This is handy getting a correct
size hint before a resize in e.g QMenu and QToolTip.
Returns if the screen was changed.
*/

bool QWidgetPrivate::setScreenForPoint(const QPoint &pos)
{
    Q_Q(QWidget);
    if (!q->isWindow())
        return false;
    // Find the screen for pos and make the widget understand it is on that screen.
    return setScreen(QGuiApplication::screenAt(pos));
}

/*!
\internal
Ensures that the widget's QWindow is set to be on the given \a screen.
Returns true if the screen was changed.
*/

bool QWidgetPrivate::setScreen(QScreen *screen)
{
    Q_Q(QWidget);
    if (!screen || !q->isWindow())
        return false;
    const QScreen *currentScreen = windowHandle() ? windowHandle()->screen() : nullptr;
    if (currentScreen != screen) {
        if (!windowHandle()) // Try to create a window handle if not created.
            createWinId();
        if (windowHandle())
            windowHandle()->setScreen(screen);
        return true;
    }
    return false;
}

/*!
\internal
Ensures that the widget has a window system identifier, i.e. that it is known to the windowing system.

*/

void QWidget::createWinId()
{
    Q_D(QWidget);
#ifdef ALIEN_DEBUG
    qDebug()  << "QWidget::createWinId" << this;
#endif
//    qWarning("QWidget::createWinId is obsolete, please fix your code.");
    d->createWinId();
}

/*!
    \since 4.4

    Returns the effective window system identifier of the widget, i.e. the
    native parent's window system identifier.

    If the widget is native, this function returns the native widget ID.
    Otherwise, the window ID of the first native parent widget, i.e., the
    top-level widget that contains this widget, is returned.

    \note We recommend that you do not store this value as it is likely to
    change at run-time.

    \sa nativeParentWidget()
*/
WId QWidget::effectiveWinId() const
{
    const WId id = internalWinId();
    if (id || !testAttribute(Qt::WA_WState_Created))
        return id;
    if (const QWidget *realParent = nativeParentWidget())
        return realParent->internalWinId();
    return 0;
}

/*!
    If this is a native widget, return the associated QWindow.
    Otherwise return null.

    Native widgets include toplevel widgets, QGLWidget, and child widgets
    on which winId() was called.

    \since 5.0

    \sa winId(), screen()
*/
QWindow *QWidget::windowHandle() const
{
    Q_D(const QWidget);
    return d->windowHandle();
}

/*!
    Returns the screen the widget is on.

    \since 5.14

    \sa windowHandle()
*/
QScreen *QWidget::screen() const
{
    Q_D(const QWidget);
    if (auto associatedScreen = d->associatedScreen())
        return associatedScreen;
    if (auto topLevel = window()) {
        if (auto topData = qt_widget_private(topLevel)->topData()) {
            if (auto initialScreen = QGuiApplicationPrivate::screen_list.value(topData->initialScreenIndex))
                return initialScreen;
        }
        if (auto screenByPos = QGuiApplication::screenAt(topLevel->geometry().center()))
            return screenByPos;
    }
    return QGuiApplication::primaryScreen();
}

#ifndef QT_NO_STYLE_STYLESHEET

/*!
    \property QWidget::styleSheet
    \brief the widget's style sheet
    \since 4.2

    The style sheet contains a textual description of customizations to the
    widget's style, as described in the \l{Qt Style Sheets} document.

    Since Qt 4.5, Qt style sheets fully supports \macos.

    \warning Qt style sheets are currently not supported for custom QStyle
    subclasses. We plan to address this in some future release.

    \sa setStyle(), QApplication::styleSheet, {Qt Style Sheets}
*/
QString QWidget::styleSheet() const
{
    Q_D(const QWidget);
    if (!d->extra)
        return QString();
    return d->extra->styleSheet;
}

void QWidget::setStyleSheet(const QString& styleSheet)
{
    Q_D(QWidget);
    if (data->in_destructor)
        return;
    d->createExtra();

    QStyleSheetStyle *proxy = qt_styleSheet(d->extra->style);
    d->extra->styleSheet = styleSheet;
    if (styleSheet.isEmpty()) { // stylesheet removed
        if (!proxy)
            return;

        d->inheritStyle();
        return;
    }

    if (proxy) { // style sheet update
        if (d->polished)
            proxy->repolish(this);
        return;
    }

    if (testAttribute(Qt::WA_SetStyle)) {
        d->setStyle_helper(new QStyleSheetStyle(d->extra->style), true);
    } else {
        d->setStyle_helper(new QStyleSheetStyle(nullptr), true);
    }
}

#endif // QT_NO_STYLE_STYLESHEET

/*!
    \sa QWidget::setStyle(), QApplication::setStyle(), QApplication::style()
*/

QStyle *QWidget::style() const
{
    Q_D(const QWidget);

    if (d->extra && d->extra->style)
        return d->extra->style;
    return QApplication::style();
}

/*!
    Sets the widget's GUI style to \a style. The ownership of the style
    object is not transferred.

    If no style is set, the widget uses the application's style,
    QApplication::style() instead.

    Setting a widget's style has no effect on existing or future child
    widgets.

    \warning This function is particularly useful for demonstration
    purposes, where you want to show Qt's styling capabilities. Real
    applications should avoid it and use one consistent GUI style
    instead.

    \warning Qt style sheets are currently not supported for custom QStyle
    subclasses. We plan to address this in some future release.

    \sa style(), QStyle, QApplication::style(), QApplication::setStyle()
*/

void QWidget::setStyle(QStyle *style)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_SetStyle, style != nullptr);
    d->createExtra();
#ifndef QT_NO_STYLE_STYLESHEET
    if (QStyleSheetStyle *styleSheetStyle = qt_styleSheet(style)) {
        //if for some reason someone try to set a QStyleSheetStyle, ref it
        //(this may happen for exemple in QButtonDialogBox which propagates its style)
        styleSheetStyle->ref();
        d->setStyle_helper(style, false);
    } else if (qt_styleSheet(d->extra->style) || !qApp->styleSheet().isEmpty()) {
        // if we have an application stylesheet or have a proxy already, propagate
        d->setStyle_helper(new QStyleSheetStyle(style), true);
    } else
#endif
        d->setStyle_helper(style, false);
}

void QWidgetPrivate::setStyle_helper(QStyle *newStyle, bool propagate)
{
    Q_Q(QWidget);
    QStyle *oldStyle = q->style();

    createExtra();

#ifndef QT_NO_STYLE_STYLESHEET
    QPointer<QStyle> origStyle = extra->style;
#endif
    extra->style = newStyle;

    // repolish
    if (polished && q->windowType() != Qt::Desktop) {
        oldStyle->unpolish(q);
        q->style()->polish(q);
    }

    if (propagate) {
        // We copy the list because the order may be modified
        const QObjectList childrenList = children;
        for (int i = 0; i < childrenList.size(); ++i) {
            QWidget *c = qobject_cast<QWidget*>(childrenList.at(i));
            if (c)
                c->d_func()->inheritStyle();
        }
    }

#ifndef QT_NO_STYLE_STYLESHEET
    if (!qt_styleSheet(newStyle)) {
        if (const QStyleSheetStyle* cssStyle = qt_styleSheet(origStyle)) {
            cssStyle->clearWidgetFont(q);
        }
    }
#endif

    QEvent e(QEvent::StyleChange);
    QCoreApplication::sendEvent(q, &e);

#ifndef QT_NO_STYLE_STYLESHEET
    // dereference the old stylesheet style
    if (QStyleSheetStyle *proxy = qt_styleSheet(origStyle))
        proxy->deref();
#endif
}

// Inherits style from the current parent and propagates it as necessary
void QWidgetPrivate::inheritStyle()
{
#ifndef QT_NO_STYLE_STYLESHEET
    Q_Q(QWidget);

    QStyle *extraStyle = extra ? (QStyle*)extra->style : nullptr;

    QStyleSheetStyle *proxy = qt_styleSheet(extraStyle);

    if (!q->styleSheet().isEmpty()) {
        Q_ASSERT(proxy);
        proxy->repolish(q);
        return;
    }

    QStyle *origStyle = proxy ? proxy->base : extraStyle;
    QWidget *parent = q->parentWidget();
    QStyle *parentStyle = (parent && parent->d_func()->extra) ? (QStyle*)parent->d_func()->extra->style : nullptr;
    // If we have stylesheet on app or parent has stylesheet style, we need
    // to be running a proxy
    if (!qApp->styleSheet().isEmpty() || qt_styleSheet(parentStyle)) {
        QStyle *newStyle = parentStyle;
        if (q->testAttribute(Qt::WA_SetStyle))
            newStyle = new QStyleSheetStyle(origStyle);
        else if (QStyleSheetStyle *newProxy = qt_styleSheet(parentStyle))
            newProxy->ref();

        setStyle_helper(newStyle, true);
        return;
    }

    // So, we have no stylesheet on parent/app and we have an empty stylesheet
    // we just need our original style back
    if (origStyle == extraStyle) // is it any different?
        return;

    // We could have inherited the proxy from our parent (which has a custom style)
    // In such a case we need to start following the application style (i.e revert
    // the propagation behavior of QStyleSheetStyle)
    if (!q->testAttribute(Qt::WA_SetStyle))
        origStyle = nullptr;

    setStyle_helper(origStyle, true);
#endif // QT_NO_STYLE_STYLESHEET
}


/*!
    \fn bool QWidget::isWindow() const

    Returns \c true if the widget is an independent window, otherwise
    returns \c false.

    A window is a widget that isn't visually the child of any other
    widget and that usually has a frame and a
    \l{QWidget::setWindowTitle()}{window title}.

    A window can have a \l{QWidget::parentWidget()}{parent widget}.
    It will then be grouped with its parent and deleted when the
    parent is deleted, minimized when the parent is minimized etc. If
    supported by the window manager, it will also have a common
    taskbar entry with its parent.

    QDialog and QMainWindow widgets are by default windows, even if a
    parent widget is specified in the constructor. This behavior is
    specified by the Qt::Window flag.

    \sa window(), isModal(), parentWidget()
*/

/*!
    \property QWidget::modal
    \brief whether the widget is a modal widget

    This property only makes sense for windows. A modal widget
    prevents widgets in all other windows from getting any input.

    By default, this property is \c false.

    \sa isWindow(), windowModality, QDialog
*/

/*!
    \property QWidget::windowModality
    \brief which windows are blocked by the modal widget
    \since 4.1

    This property only makes sense for windows. A modal widget
    prevents widgets in other windows from getting input. The value of
    this property controls which windows are blocked when the widget
    is visible. Changing this property while the window is visible has
    no effect; you must hide() the widget first, then show() it again.

    By default, this property is Qt::NonModal.

    \sa isWindow(), QWidget::modal, QDialog
*/

Qt::WindowModality QWidget::windowModality() const
{
    return static_cast<Qt::WindowModality>(data->window_modality);
}

void QWidget::setWindowModality(Qt::WindowModality windowModality)
{
    data->window_modality = windowModality;
    // setModal_sys() will be called by setAttribute()
    setAttribute(Qt::WA_ShowModal, (data->window_modality != Qt::NonModal));
    setAttribute(Qt::WA_SetWindowModality, true);
}

void QWidgetPrivate::setModal_sys()
{
    Q_Q(QWidget);
    if (q->windowHandle())
        q->windowHandle()->setModality(q->windowModality());
}

/*!
    \fn bool QWidget::underMouse() const

    Returns \c true if the widget is under the mouse cursor; otherwise
    returns \c false.

    This value is not updated properly during drag and drop
    operations.

    \sa enterEvent(), leaveEvent()
*/

/*!
    \property QWidget::minimized
    \brief whether this widget is minimized (iconified)

    This property is only relevant for windows.

    By default, this property is \c false.

    \sa showMinimized(), visible, show(), hide(), showNormal(), maximized
*/
bool QWidget::isMinimized() const
{ return data->window_state & Qt::WindowMinimized; }

/*!
    Shows the widget minimized, as an icon.

    Calling this function only affects \l{isWindow()}{windows}.

    \sa showNormal(), showMaximized(), show(), hide(), isVisible(),
        isMinimized()
*/
void QWidget::showMinimized()
{
    bool isMin = isMinimized();
    if (isMin && isVisible())
        return;

    ensurePolished();

    if (!isMin)
        setWindowState((windowState() & ~Qt::WindowActive) | Qt::WindowMinimized);
    setVisible(true);
}

/*!
    \property QWidget::maximized
    \brief whether this widget is maximized

    This property is only relevant for windows.

    \note Due to limitations on some window systems, this does not always
    report the expected results (e.g., if the user on X11 maximizes the
    window via the window manager, Qt has no way of distinguishing this
    from any other resize). This is expected to improve as window manager
    protocols evolve.

    By default, this property is \c false.

    \sa windowState(), showMaximized(), visible, show(), hide(), showNormal(), minimized
*/
bool QWidget::isMaximized() const
{ return data->window_state & Qt::WindowMaximized; }



/*!
    Returns the current window state. The window state is a OR'ed
    combination of Qt::WindowState: Qt::WindowMinimized,
    Qt::WindowMaximized, Qt::WindowFullScreen, and Qt::WindowActive.

  \sa Qt::WindowState, setWindowState()
 */
Qt::WindowStates QWidget::windowState() const
{
    return Qt::WindowStates(data->window_state);
}

/*!\internal

   The function sets the window state on child widgets similar to
   setWindowState(). The difference is that the window state changed
   event has the isOverride() flag set. It exists mainly to keep
   QWorkspace working.
 */
void QWidget::overrideWindowState(Qt::WindowStates newstate)
{
    QWindowStateChangeEvent e(Qt::WindowStates(data->window_state), true);
    data->window_state  = newstate;
    QCoreApplication::sendEvent(this, &e);
}

/*!
    \fn void QWidget::setWindowState(Qt::WindowStates windowState)

    Sets the window state to \a windowState. The window state is a OR'ed
    combination of Qt::WindowState: Qt::WindowMinimized,
    Qt::WindowMaximized, Qt::WindowFullScreen, and Qt::WindowActive.

    If the window is not visible (i.e. isVisible() returns \c false), the
    window state will take effect when show() is called. For visible
    windows, the change is immediate. For example, to toggle between
    full-screen and normal mode, use the following code:

    \snippet code/src_gui_kernel_qwidget.cpp 0

    In order to restore and activate a minimized window (while
    preserving its maximized and/or full-screen state), use the following:

    \snippet code/src_gui_kernel_qwidget.cpp 1

    Calling this function will hide the widget. You must call show() to make
    the widget visible again.

    \note On some window systems Qt::WindowActive is not immediate, and may be
    ignored in certain cases.

    When the window state changes, the widget receives a changeEvent()
    of type QEvent::WindowStateChange.

    \sa Qt::WindowState, windowState()
*/
void QWidget::setWindowState(Qt::WindowStates newstate)
{
    Q_D(QWidget);
    Qt::WindowStates oldstate = windowState();
    if (newstate.testFlag(Qt::WindowMinimized)) // QTBUG-46763
       newstate.setFlag(Qt::WindowActive, false);
    if (oldstate == newstate)
        return;
    if (isWindow() && !testAttribute(Qt::WA_WState_Created))
        create();

    data->window_state = newstate;
    data->in_set_window_state = 1;
    if (isWindow()) {
        // Ensure the initial size is valid, since we store it as normalGeometry below.
        if (!testAttribute(Qt::WA_Resized) && !isVisible())
            adjustSize();

        d->createTLExtra();
        if (!(oldstate & (Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen)))
            d->topData()->normalGeometry = geometry();

        Q_ASSERT(windowHandle());
        windowHandle()->setWindowStates(newstate & ~Qt::WindowActive);
    }
    data->in_set_window_state = 0;

    if (newstate & Qt::WindowActive)
        activateWindow();

    QWindowStateChangeEvent e(oldstate);
    QCoreApplication::sendEvent(this, &e);
}

/*!
    \property QWidget::fullScreen
    \brief whether the widget is shown in full screen mode

    A widget in full screen mode occupies the whole screen area and does not
    display window decorations, such as a title bar.

    By default, this property is \c false.

    \sa windowState(), minimized, maximized
*/
bool QWidget::isFullScreen() const
{ return data->window_state & Qt::WindowFullScreen; }

/*!
    Shows the widget in full-screen mode.

    Calling this function only affects \l{isWindow()}{windows}.

    To return from full-screen mode, call showNormal().

    Full-screen mode works fine under Windows, but has certain
    problems under X. These problems are due to limitations of the
    ICCCM protocol that specifies the communication between X11
    clients and the window manager. ICCCM simply does not understand
    the concept of non-decorated full-screen windows. Therefore, the
    best we can do is to request a borderless window and place and
    resize it to fill the entire screen. Depending on the window
    manager, this may or may not work. The borderless window is
    requested using MOTIF hints, which are at least partially
    supported by virtually all modern window managers.

    An alternative would be to bypass the window manager entirely and
    create a window with the Qt::X11BypassWindowManagerHint flag. This
    has other severe problems though, like totally broken keyboard focus
    and very strange effects on desktop changes or when the user raises
    other windows.

    X11 window managers that follow modern post-ICCCM specifications
    support full-screen mode properly.

    \sa showNormal(), showMaximized(), show(), hide(), isVisible()
*/
void QWidget::showFullScreen()
{
    ensurePolished();

    setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowMaximized))
                   | Qt::WindowFullScreen);
    setVisible(true);
#if !defined Q_OS_QNX // On QNX this window will be activated anyway from libscreen
                      // activating it here before libscreen activates it causes problems
    activateWindow();
#endif
}

/*!
    Shows the widget maximized.

    Calling this function only affects \l{isWindow()}{windows}.

    On X11, this function may not work properly with certain window
    managers. See the \l{Window Geometry} documentation for an explanation.

    \sa setWindowState(), showNormal(), showMinimized(), show(), hide(), isVisible()
*/
void QWidget::showMaximized()
{
    ensurePolished();

    setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
                   | Qt::WindowMaximized);
    setVisible(true);
}

/*!
    Restores the widget after it has been maximized or minimized.

    Calling this function only affects \l{isWindow()}{windows}.

    \sa setWindowState(), showMinimized(), showMaximized(), show(), hide(), isVisible()
*/
void QWidget::showNormal()
{
    ensurePolished();

    setWindowState(windowState() & ~(Qt::WindowMinimized
                                     | Qt::WindowMaximized
                                     | Qt::WindowFullScreen));
    setVisible(true);
}

/*!
    Returns \c true if this widget would become enabled if \a ancestor is
    enabled; otherwise returns \c false.



    This is the case if neither the widget itself nor every parent up
    to but excluding \a ancestor has been explicitly disabled.

    isEnabledTo(0) returns false if this widget or any if its ancestors
    was explicitly disabled.

    The word ancestor here means a parent widget within the same window.

    Therefore isEnabledTo(0) stops at this widget's window, unlike
    isEnabled() which also takes parent windows into considerations.

    \sa setEnabled(), enabled
*/

bool QWidget::isEnabledTo(const QWidget *ancestor) const
{
    const QWidget * w = this;
    while (!w->testAttribute(Qt::WA_ForceDisabled)
            && !w->isWindow()
            && w->parentWidget()
            && w->parentWidget() != ancestor)
        w = w->parentWidget();
    return !w->testAttribute(Qt::WA_ForceDisabled);
}

#ifndef QT_NO_ACTION
/*!
    Appends the action \a action to this widget's list of actions.

    All QWidgets have a list of \l{QAction}s, however they can be
    represented graphically in many different ways. The default use of
    the QAction list (as returned by actions()) is to create a context
    QMenu.

    A QWidget should only have one of each action and adding an action
    it already has will not cause the same action to be in the widget twice.

    The ownership of \a action is not transferred to this QWidget.

    \sa removeAction(), insertAction(), actions(), QMenu
*/
void QWidget::addAction(QAction *action)
{
    insertAction(nullptr, action);
}

/*!
    Appends the actions \a actions to this widget's list of actions.

    \sa removeAction(), QMenu, addAction()
*/
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void QWidget::addActions(const QList<QAction *> &actions)
#else
void QWidget::addActions(QList<QAction*> actions)
#endif
{
    for(int i = 0; i < actions.count(); i++)
        insertAction(nullptr, actions.at(i));
}

/*!
    Inserts the action \a action to this widget's list of actions,
    before the action \a before. It appends the action if \a before is \nullptr or
    \a before is not a valid action for this widget.

    A QWidget should only have one of each action.

    \sa removeAction(), addAction(), QMenu, contextMenuPolicy, actions()
*/
void QWidget::insertAction(QAction *before, QAction *action)
{
    if (Q_UNLIKELY(!action)) {
        qWarning("QWidget::insertAction: Attempt to insert null action");
        return;
    }

    Q_D(QWidget);
    if(d->actions.contains(action))
        removeAction(action);

    int pos = d->actions.indexOf(before);
    if (pos < 0) {
        before = nullptr;
        pos = d->actions.size();
    }
    d->actions.insert(pos, action);

    QActionPrivate *apriv = action->d_func();
    apriv->widgets.append(this);

    QActionEvent e(QEvent::ActionAdded, action, before);
    QCoreApplication::sendEvent(this, &e);
}

/*!
    Inserts the actions \a actions to this widget's list of actions,
    before the action \a before. It appends the action if \a before is \nullptr or
    \a before is not a valid action for this widget.

    A QWidget can have at most one of each action.

    \sa removeAction(), QMenu, insertAction(), contextMenuPolicy
*/
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void QWidget::insertActions(QAction *before, const QList<QAction*> &actions)
#else
void QWidget::insertActions(QAction *before, QList<QAction*> actions)
#endif
{
    for(int i = 0; i < actions.count(); ++i)
        insertAction(before, actions.at(i));
}

/*!
    Removes the action \a action from this widget's list of actions.
    \sa insertAction(), actions(), insertAction()
*/
void QWidget::removeAction(QAction *action)
{
    if (!action)
        return;

    Q_D(QWidget);

    QActionPrivate *apriv = action->d_func();
    apriv->widgets.removeAll(this);

    if (d->actions.removeAll(action)) {
        QActionEvent e(QEvent::ActionRemoved, action);
        QCoreApplication::sendEvent(this, &e);
    }
}

/*!
    Returns the (possibly empty) list of this widget's actions.

    \sa contextMenuPolicy, insertAction(), removeAction()
*/
QList<QAction*> QWidget::actions() const
{
    Q_D(const QWidget);
    return d->actions;
}
#endif // QT_NO_ACTION

/*!
  \fn bool QWidget::isEnabledToTLW() const
  \obsolete

  This function is deprecated. It is equivalent to isEnabled()
*/

/*!
    \property QWidget::enabled
    \brief whether the widget is enabled

    In general an enabled widget handles keyboard and mouse events; a disabled
    widget does not. An exception is made with \l{QAbstractButton}.

    Some widgets display themselves differently when they are
    disabled. For example a button might draw its label grayed out. If
    your widget needs to know when it becomes enabled or disabled, you
    can use the changeEvent() with type QEvent::EnabledChange.

    Disabling a widget implicitly disables all its children. Enabling
    respectively enables all child widgets unless they have been
    explicitly disabled. It it not possible to explicitly enable a child
    widget which is not a window while its parent widget remains disabled.

    By default, this property is \c true.

    \sa isEnabledTo(), QKeyEvent, QMouseEvent, changeEvent()
*/
void QWidget::setEnabled(bool enable)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_ForceDisabled, !enable);
    d->setEnabled_helper(enable);
}

void QWidgetPrivate::setEnabled_helper(bool enable)
{
    Q_Q(QWidget);

    if (enable && !q->isWindow() && q->parentWidget() && !q->parentWidget()->isEnabled())
        return; // nothing we can do

    if (enable != q->testAttribute(Qt::WA_Disabled))
        return; // nothing to do

    q->setAttribute(Qt::WA_Disabled, !enable);
    updateSystemBackground();

    if (!enable && q->window()->focusWidget() == q) {
        bool parentIsEnabled = (!q->parentWidget() || q->parentWidget()->isEnabled());
        if (!parentIsEnabled || !q->focusNextChild())
            q->clearFocus();
    }

    Qt::WidgetAttribute attribute = enable ? Qt::WA_ForceDisabled : Qt::WA_Disabled;
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && !w->testAttribute(attribute))
            w->d_func()->setEnabled_helper(enable);
    }
#ifndef QT_NO_CURSOR
    if (q->testAttribute(Qt::WA_SetCursor) || q->isWindow()) {
        // enforce the windows behavior of clearing the cursor on
        // disabled widgets
        qt_qpa_set_cursor(q, false);
    }
#endif
#ifndef QT_NO_IM
    if (q->testAttribute(Qt::WA_InputMethodEnabled) && q->hasFocus()) {
        QWidget *focusWidget = effectiveFocusWidget();

        if (enable) {
            if (focusWidget->testAttribute(Qt::WA_InputMethodEnabled))
                QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        } else {
            QGuiApplication::inputMethod()->commit();
            QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        }
    }
#endif //QT_NO_IM
    QEvent e(QEvent::EnabledChange);
    QCoreApplication::sendEvent(q, &e);
}

/*!
    \property QWidget::acceptDrops
    \brief whether drop events are enabled for this widget

    Setting this property to true announces to the system that this
    widget \e may be able to accept drop events.

    If the widget is the desktop (windowType() == Qt::Desktop), this may
    fail if another application is using the desktop; you can call
    acceptDrops() to test if this occurs.

    \warning Do not modify this property in a drag and drop event handler.

    By default, this property is \c false.

    \sa {Drag and Drop}
*/
bool QWidget::acceptDrops() const
{
    return testAttribute(Qt::WA_AcceptDrops);
}

void QWidget::setAcceptDrops(bool on)
{
    setAttribute(Qt::WA_AcceptDrops, on);

}

/*!
    Disables widget input events if \a disable is true; otherwise
    enables input events.

    See the \l enabled documentation for more information.

    \sa isEnabledTo(), QKeyEvent, QMouseEvent, changeEvent()
*/
void QWidget::setDisabled(bool disable)
{
    setEnabled(!disable);
}

/*!
    \property QWidget::frameGeometry
    \brief geometry of the widget relative to its parent including any
    window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa geometry(), x(), y(), pos()
*/
QRect QWidget::frameGeometry() const
{
    Q_D(const QWidget);
    if (isWindow() && ! (windowType() == Qt::Popup)) {
        QRect fs = d->frameStrut();
        return QRect(data->crect.x() - fs.left(),
                     data->crect.y() - fs.top(),
                     data->crect.width() + fs.left() + fs.right(),
                     data->crect.height() + fs.top() + fs.bottom());
    }
    return data->crect;
}

/*!
    \property QWidget::x

    \brief the x coordinate of the widget relative to its parent including
    any window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property has a value of 0.

    \sa frameGeometry, y, pos
*/
int QWidget::x() const
{
    Q_D(const QWidget);
    if (isWindow() && ! (windowType() == Qt::Popup))
        return data->crect.x() - d->frameStrut().left();
    return data->crect.x();
}

/*!
    \property QWidget::y
    \brief the y coordinate of the widget relative to its parent and
    including any window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property has a value of 0.

    \sa frameGeometry, x, pos
*/
int QWidget::y() const
{
    Q_D(const QWidget);
    if (isWindow() && ! (windowType() == Qt::Popup))
        return data->crect.y() - d->frameStrut().top();
    return data->crect.y();
}

/*!
    \property QWidget::pos
    \brief the position of the widget within its parent widget

    If the widget is a window, the position is that of the widget on
    the desktop, including its frame.

    When changing the position, the widget, if visible, receives a
    move event (moveEvent()) immediately. If the widget is not
    currently visible, it is guaranteed to receive an event before it
    is shown.

    By default, this property contains a position that refers to the
    origin.

    \warning Calling move() or setGeometry() inside moveEvent() can
    lead to infinite recursion.

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    \sa frameGeometry, size, x(), y()
*/
QPoint QWidget::pos() const
{
    Q_D(const QWidget);
    QPoint result = data->crect.topLeft();
    if (isWindow() && ! (windowType() == Qt::Popup))
        if (!d->maybeTopData() || !d->maybeTopData()->posIncludesFrame)
            result -= d->frameStrut().topLeft();
    return result;
}

/*!
    \property QWidget::geometry
    \brief the geometry of the widget relative to its parent and
    excluding the window frame

    When changing the geometry, the widget, if visible, receives a
    move event (moveEvent()) and/or a resize event (resizeEvent())
    immediately. If the widget is not currently visible, it is
    guaranteed to receive appropriate events before it is shown.

    The size component is adjusted if it lies outside the range
    defined by minimumSize() and maximumSize().

    \warning Calling setGeometry() inside resizeEvent() or moveEvent()
    can lead to infinite recursion.

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa frameGeometry(), rect(), move(), resize(), moveEvent(),
        resizeEvent(), minimumSize(), maximumSize()
*/

/*!
    \property QWidget::normalGeometry

    \brief the geometry of the widget as it will appear when shown as
    a normal (not maximized or full screen) top-level widget

    For child widgets this property always holds an empty rectangle.

    By default, this property contains an empty rectangle.

    \sa QWidget::windowState(), QWidget::geometry
*/

/*!
    \property QWidget::size
    \brief the size of the widget excluding any window frame

    If the widget is visible when it is being resized, it receives a resize event
    (resizeEvent()) immediately. If the widget is not currently
    visible, it is guaranteed to receive an event before it is shown.

    The size is adjusted if it lies outside the range defined by
    minimumSize() and maximumSize().

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \warning Calling resize() or setGeometry() inside resizeEvent() can
    lead to infinite recursion.

    \note Setting the size to \c{QSize(0, 0)} will cause the widget to not
    appear on screen. This also applies to windows.

    \sa pos, geometry, minimumSize, maximumSize, resizeEvent(), adjustSize()
*/

/*!
    \property QWidget::width
    \brief the width of the widget excluding any window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    \note Do not use this function to find the width of a screen on
    a \l{QDesktopWidget}{multiple screen desktop}. Read
    \l{QDesktopWidget#Screen Geometry}{this note} for details.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa geometry, height, size
*/

/*!
    \property QWidget::height
    \brief the height of the widget excluding any window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    \note Do not use this function to find the height of a screen
    on a \l{QDesktopWidget}{multiple screen desktop}. Read
    \l{QDesktopWidget#Screen Geometry}{this note} for details.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa geometry, width, size
*/

/*!
    \property QWidget::rect
    \brief the internal geometry of the widget excluding any window
    frame

    The rect property equals QRect(0, 0, width(), height()).

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa size
*/


QRect QWidget::normalGeometry() const
{
    Q_D(const QWidget);
    if (!d->extra || !d->extra->topextra)
        return QRect();

    if (!isMaximized() && !isFullScreen())
        return geometry();

    return d->topData()->normalGeometry;
}


/*!
    \property QWidget::childrenRect
    \brief the bounding rectangle of the widget's children

    Hidden children are excluded.

    By default, for a widget with no children, this property contains a
    rectangle with zero width and height located at the origin.

    \sa childrenRegion(), geometry()
*/

QRect QWidget::childrenRect() const
{
    Q_D(const QWidget);
    QRect r(0, 0, 0, 0);
    for (int i = 0; i < d->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
        if (w && !w->isWindow() && !w->isHidden())
            r |= w->geometry();
    }
    return r;
}

/*!
    \property QWidget::childrenRegion
    \brief the combined region occupied by the widget's children

    Hidden children are excluded.

    By default, for a widget with no children, this property contains an
    empty region.

    \sa childrenRect(), geometry(), mask()
*/

QRegion QWidget::childrenRegion() const
{
    Q_D(const QWidget);
    QRegion r;
    for (int i = 0; i < d->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
        if (w && !w->isWindow() && !w->isHidden()) {
            QRegion mask = w->mask();
            if (mask.isEmpty())
                r |= w->geometry();
            else
                r |= mask.translated(w->pos());
        }
    }
    return r;
}


/*!
    \property QWidget::minimumSize
    \brief the widget's minimum size

    The widget cannot be resized to a smaller size than the minimum
    widget size. The widget's size is forced to the minimum size if
    the current size is smaller.

    The minimum size set by this function will override the minimum size
    defined by QLayout. In order to unset the minimum size, use a
    value of \c{QSize(0, 0)}.

    By default, this property contains a size with zero width and height.

    \sa minimumWidth, minimumHeight, maximumSize, sizeIncrement
*/

QSize QWidget::minimumSize() const
{
    Q_D(const QWidget);
    return d->extra ? QSize(d->extra->minw, d->extra->minh) : QSize(0, 0);
}

/*!
    \property QWidget::maximumSize
    \brief the widget's maximum size in pixels

    The widget cannot be resized to a larger size than the maximum
    widget size.

    By default, this property contains a size in which both width and height
    have values of 16777215.

    \note The definition of the \c QWIDGETSIZE_MAX macro limits the maximum size
    of widgets.

    \sa maximumWidth, maximumHeight, minimumSize, sizeIncrement
*/

QSize QWidget::maximumSize() const
{
    Q_D(const QWidget);
    return d->extra ? QSize(d->extra->maxw, d->extra->maxh)
                 : QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}


/*!
    \property QWidget::minimumWidth
    \brief the widget's minimum width in pixels

    This property corresponds to the width held by the \l minimumSize property.

    By default, this property has a value of 0.

    \sa minimumSize, minimumHeight
*/

/*!
    \property QWidget::minimumHeight
    \brief the widget's minimum height in pixels

    This property corresponds to the height held by the \l minimumSize property.

    By default, this property has a value of 0.

    \sa minimumSize, minimumWidth
*/

/*!
    \property QWidget::maximumWidth
    \brief the widget's maximum width in pixels

    This property corresponds to the width held by the \l maximumSize property.

    By default, this property contains a value of 16777215.

    \note The definition of the \c QWIDGETSIZE_MAX macro limits the maximum size
    of widgets.

    \sa maximumSize, maximumHeight
*/

/*!
    \property QWidget::maximumHeight
    \brief the widget's maximum height in pixels

    This property corresponds to the height held by the \l maximumSize property.

    By default, this property contains a value of 16777215.

    \note The definition of the \c QWIDGETSIZE_MAX macro limits the maximum size
    of widgets.

    \sa maximumSize, maximumWidth
*/

/*!
    \property QWidget::sizeIncrement
    \brief the size increment of the widget

    When the user resizes the window, the size will move in steps of
    sizeIncrement().width() pixels horizontally and
    sizeIncrement.height() pixels vertically, with baseSize() as the
    basis. Preferred widget sizes are for non-negative integers \e i
    and \e j:
    \snippet code/src_gui_kernel_qwidget.cpp 2

    Note that while you can set the size increment for all widgets, it
    only affects windows.

    By default, this property contains a size with zero width and height.

    \warning The size increment has no effect under Windows, and may
    be disregarded by the window manager on X11.

    \sa size, minimumSize, maximumSize
*/
QSize QWidget::sizeIncrement() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra)
        ? QSize(d->extra->topextra->incw, d->extra->topextra->inch)
        : QSize(0, 0);
}

/*!
    \property QWidget::baseSize
    \brief the base size of the widget

    The base size is used to calculate a proper widget size if the
    widget defines sizeIncrement().

    By default, for a newly-created widget, this property contains a size with
    zero width and height.

    \sa setSizeIncrement()
*/

QSize QWidget::baseSize() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra)
        ? QSize(d->extra->topextra->basew, d->extra->topextra->baseh)
        : QSize(0, 0);
}

bool QWidgetPrivate::setMinimumSize_helper(int &minw, int &minh)
{
    Q_Q(QWidget);

    int mw = minw, mh = minh;
    if (mw == QWIDGETSIZE_MAX)
        mw = 0;
    if (mh == QWIDGETSIZE_MAX)
        mh = 0;
    if (Q_UNLIKELY(minw > QWIDGETSIZE_MAX || minh > QWIDGETSIZE_MAX)) {
        qWarning("QWidget::setMinimumSize: (%s/%s) "
                "The largest allowed size is (%d,%d)",
                 q->objectName().toLocal8Bit().data(), q->metaObject()->className(), QWIDGETSIZE_MAX,
                QWIDGETSIZE_MAX);
        minw = mw = qMin<int>(minw, QWIDGETSIZE_MAX);
        minh = mh = qMin<int>(minh, QWIDGETSIZE_MAX);
    }
    if (Q_UNLIKELY(minw < 0 || minh < 0)) {
        qWarning("QWidget::setMinimumSize: (%s/%s) Negative sizes (%d,%d) "
                "are not possible",
                q->objectName().toLocal8Bit().data(), q->metaObject()->className(), minw, minh);
        minw = mw = qMax(minw, 0);
        minh = mh = qMax(minh, 0);
    }
    createExtra();
    if (extra->minw == mw && extra->minh == mh)
        return false;
    extra->minw = mw;
    extra->minh = mh;
    extra->explicitMinSize = (mw ? Qt::Horizontal : 0) | (mh ? Qt::Vertical : 0);
    return true;
}

void QWidgetPrivate::setConstraints_sys()
{
    Q_Q(QWidget);
    if (extra && q->windowHandle()) {
        QWindow *win = q->windowHandle();
        QWindowPrivate *winp = qt_window_private(win);

        winp->minimumSize = QSize(extra->minw, extra->minh);
        winp->maximumSize = QSize(extra->maxw, extra->maxh);

        if (extra->topextra) {
            winp->baseSize = QSize(extra->topextra->basew, extra->topextra->baseh);
            winp->sizeIncrement = QSize(extra->topextra->incw, extra->topextra->inch);
        }

        if (winp->platformWindow) {
            fixPosIncludesFrame();
            winp->platformWindow->propagateSizeHints();
        }
    }
}

/*!
    \overload

    This function corresponds to setMinimumSize(QSize(minw, minh)).
    Sets the minimum width to \a minw and the minimum height to \a
    minh.
*/

void QWidget::setMinimumSize(int minw, int minh)
{
    Q_D(QWidget);
    if (!d->setMinimumSize_helper(minw, minh))
        return;

    if (isWindow())
        d->setConstraints_sys();
    if (minw > width() || minh > height()) {
        bool resized = testAttribute(Qt::WA_Resized);
        bool maximized = isMaximized();
        resize(qMax(minw,width()), qMax(minh,height()));
        setAttribute(Qt::WA_Resized, resized); //not a user resize
        if (maximized)
            data->window_state = data->window_state | Qt::WindowMaximized;
    }
#if QT_CONFIG(graphicsview)
    if (d->extra) {
        if (d->extra->proxyWidget)
            d->extra->proxyWidget->setMinimumSize(minw, minh);
    }
#endif
    d->updateGeometry_helper(d->extra->minw == d->extra->maxw && d->extra->minh == d->extra->maxh);
}

bool QWidgetPrivate::setMaximumSize_helper(int &maxw, int &maxh)
{
    Q_Q(QWidget);
    if (Q_UNLIKELY(maxw > QWIDGETSIZE_MAX || maxh > QWIDGETSIZE_MAX)) {
        qWarning("QWidget::setMaximumSize: (%s/%s) "
                "The largest allowed size is (%d,%d)",
                 q->objectName().toLocal8Bit().data(), q->metaObject()->className(), QWIDGETSIZE_MAX,
                QWIDGETSIZE_MAX);
        maxw = qMin<int>(maxw, QWIDGETSIZE_MAX);
        maxh = qMin<int>(maxh, QWIDGETSIZE_MAX);
    }
    if (Q_UNLIKELY(maxw < 0 || maxh < 0)) {
        qWarning("QWidget::setMaximumSize: (%s/%s) Negative sizes (%d,%d) "
                "are not possible",
                q->objectName().toLocal8Bit().data(), q->metaObject()->className(), maxw, maxh);
        maxw = qMax(maxw, 0);
        maxh = qMax(maxh, 0);
    }
    createExtra();
    if (extra->maxw == maxw && extra->maxh == maxh)
        return false;
    extra->maxw = maxw;
    extra->maxh = maxh;
    extra->explicitMaxSize = (maxw != QWIDGETSIZE_MAX ? Qt::Horizontal : 0) |
                             (maxh != QWIDGETSIZE_MAX ? Qt::Vertical : 0);
    return true;
}

/*!
    \overload

    This function corresponds to setMaximumSize(QSize(\a maxw, \a
    maxh)). Sets the maximum width to \a maxw and the maximum height
    to \a maxh.
*/
void QWidget::setMaximumSize(int maxw, int maxh)
{
    Q_D(QWidget);
    if (!d->setMaximumSize_helper(maxw, maxh))
        return;

    if (isWindow())
        d->setConstraints_sys();
    if (maxw < width() || maxh < height()) {
        bool resized = testAttribute(Qt::WA_Resized);
        resize(qMin(maxw,width()), qMin(maxh,height()));
        setAttribute(Qt::WA_Resized, resized); //not a user resize
    }

#if QT_CONFIG(graphicsview)
    if (d->extra) {
        if (d->extra->proxyWidget)
            d->extra->proxyWidget->setMaximumSize(maxw, maxh);
    }
#endif

    d->updateGeometry_helper(d->extra->minw == d->extra->maxw && d->extra->minh == d->extra->maxh);
}

/*!
    \overload

    Sets the x (width) size increment to \a w and the y (height) size
    increment to \a h.
*/
void QWidget::setSizeIncrement(int w, int h)
{
    Q_D(QWidget);
    d->createTLExtra();
    QTLWExtra* x = d->topData();
    if (x->incw == w && x->inch == h)
        return;
    x->incw = w;
    x->inch = h;
    if (isWindow())
        d->setConstraints_sys();
}

/*!
    \overload

    This corresponds to setBaseSize(QSize(\a basew, \a baseh)). Sets
    the widgets base size to width \a basew and height \a baseh.
*/
void QWidget::setBaseSize(int basew, int baseh)
{
    Q_D(QWidget);
    d->createTLExtra();
    QTLWExtra* x = d->topData();
    if (x->basew == basew && x->baseh == baseh)
        return;
    x->basew = basew;
    x->baseh = baseh;
    if (isWindow())
        d->setConstraints_sys();
}

/*!
    Sets both the minimum and maximum sizes of the widget to \a s,
    thereby preventing it from ever growing or shrinking.

    This will override the default size constraints set by QLayout.

    To remove constraints, set the size to QWIDGETSIZE_MAX.

    Alternatively, if you want the widget to have a
    fixed size based on its contents, you can call
    QLayout::setSizeConstraint(QLayout::SetFixedSize);

    \sa maximumSize, minimumSize
*/

void QWidget::setFixedSize(const QSize & s)
{
    setFixedSize(s.width(), s.height());
}


/*!
    \fn void QWidget::setFixedSize(int w, int h)
    \overload

    Sets the width of the widget to \a w and the height to \a h.
*/

void QWidget::setFixedSize(int w, int h)
{
    Q_D(QWidget);
    bool minSizeSet = d->setMinimumSize_helper(w, h);
    bool maxSizeSet = d->setMaximumSize_helper(w, h);
    if (!minSizeSet && !maxSizeSet)
        return;

    if (isWindow())
        d->setConstraints_sys();
    else
        d->updateGeometry_helper(true);

    if (w != QWIDGETSIZE_MAX || h != QWIDGETSIZE_MAX)
        resize(w, h);
}

void QWidget::setMinimumWidth(int w)
{
    Q_D(QWidget);
    d->createExtra();
    uint expl = d->extra->explicitMinSize | (w ? Qt::Horizontal : 0);
    setMinimumSize(w, minimumSize().height());
    d->extra->explicitMinSize = expl;
}

void QWidget::setMinimumHeight(int h)
{
    Q_D(QWidget);
    d->createExtra();
    uint expl = d->extra->explicitMinSize | (h ? Qt::Vertical : 0);
    setMinimumSize(minimumSize().width(), h);
    d->extra->explicitMinSize = expl;
}

void QWidget::setMaximumWidth(int w)
{
    Q_D(QWidget);
    d->createExtra();
    uint expl = d->extra->explicitMaxSize | (w == QWIDGETSIZE_MAX ? 0 : Qt::Horizontal);
    setMaximumSize(w, maximumSize().height());
    d->extra->explicitMaxSize = expl;
}

void QWidget::setMaximumHeight(int h)
{
    Q_D(QWidget);
    d->createExtra();
    uint expl = d->extra->explicitMaxSize | (h == QWIDGETSIZE_MAX ? 0 : Qt::Vertical);
    setMaximumSize(maximumSize().width(), h);
    d->extra->explicitMaxSize = expl;
}

/*!
    Sets both the minimum and maximum width of the widget to \a w
    without changing the heights. Provided for convenience.

    \sa sizeHint(), minimumSize(), maximumSize(), setFixedSize()
*/

void QWidget::setFixedWidth(int w)
{
    Q_D(QWidget);
    d->createExtra();
    uint explMin = d->extra->explicitMinSize | Qt::Horizontal;
    uint explMax = d->extra->explicitMaxSize | Qt::Horizontal;
    setMinimumSize(w, minimumSize().height());
    setMaximumSize(w, maximumSize().height());
    d->extra->explicitMinSize = explMin;
    d->extra->explicitMaxSize = explMax;
}


/*!
    Sets both the minimum and maximum heights of the widget to \a h
    without changing the widths. Provided for convenience.

    \sa sizeHint(), minimumSize(), maximumSize(), setFixedSize()
*/

void QWidget::setFixedHeight(int h)
{
    Q_D(QWidget);
    d->createExtra();
    uint explMin = d->extra->explicitMinSize | Qt::Vertical;
    uint explMax = d->extra->explicitMaxSize | Qt::Vertical;
    setMinimumSize(minimumSize().width(), h);
    setMaximumSize(maximumSize().width(), h);
    d->extra->explicitMinSize = explMin;
    d->extra->explicitMaxSize = explMax;
}


/*!
    Translates the widget coordinate \a pos to the coordinate system
    of \a parent. The \a parent must not be \nullptr and must be a parent
    of the calling widget.

    \sa mapFrom(), mapToParent(), mapToGlobal(), underMouse()
*/

QPoint QWidget::mapTo(const QWidget * parent, const QPoint & pos) const
{
    QPoint p = pos;
    if (parent) {
        const QWidget * w = this;
        while (w != parent) {
            Q_ASSERT_X(w, "QWidget::mapTo(const QWidget *parent, const QPoint &pos)",
                       "parent must be in parent hierarchy");
            p = w->mapToParent(p);
            w = w->parentWidget();
        }
    }
    return p;
}


/*!
    Translates the widget coordinate \a pos from the coordinate system
    of \a parent to this widget's coordinate system. The \a parent
    must not be \nullptr and must be a parent of the calling widget.

    \sa mapTo(), mapFromParent(), mapFromGlobal(), underMouse()
*/

QPoint QWidget::mapFrom(const QWidget * parent, const QPoint & pos) const
{
    QPoint p(pos);
    if (parent) {
        const QWidget * w = this;
        while (w != parent) {
            Q_ASSERT_X(w, "QWidget::mapFrom(const QWidget *parent, const QPoint &pos)",
                       "parent must be in parent hierarchy");

            p = w->mapFromParent(p);
            w = w->parentWidget();
        }
    }
    return p;
}


/*!
    Translates the widget coordinate \a pos to a coordinate in the
    parent widget.

    Same as mapToGlobal() if the widget has no parent.

    \sa mapFromParent(), mapTo(), mapToGlobal(), underMouse()
*/

QPoint QWidget::mapToParent(const QPoint &pos) const
{
    return pos + data->crect.topLeft();
}

/*!
    Translates the parent widget coordinate \a pos to widget
    coordinates.

    Same as mapFromGlobal() if the widget has no parent.

    \sa mapToParent(), mapFrom(), mapFromGlobal(), underMouse()
*/

QPoint QWidget::mapFromParent(const QPoint &pos) const
{
    return pos - data->crect.topLeft();
}


/*!
    Returns the window for this widget, i.e. the next ancestor widget
    that has (or could have) a window-system frame.

    If the widget is a window, the widget itself is returned.

    Typical usage is changing the window title:

    \snippet code/src_gui_kernel_qwidget.cpp 3

    \sa isWindow()
*/

QWidget *QWidget::window() const
{
    QWidget *w = const_cast<QWidget *>(this);
    QWidget *p = w->parentWidget();
    while (!w->isWindow() && p) {
        w = p;
        p = p->parentWidget();
    }
    return w;
}

/*!
    \since 4.4

    Returns the native parent for this widget, i.e. the next ancestor widget
    that has a system identifier, or \nullptr if it does not have any native
    parent.

    \sa effectiveWinId()
*/
QWidget *QWidget::nativeParentWidget() const
{
    QWidget *parent = parentWidget();
    while (parent && !parent->internalWinId())
        parent = parent->parentWidget();
    return parent;
}

/*! \fn QWidget *QWidget::topLevelWidget() const
    \obsolete

    Use window() instead.
*/



/*!
  Returns the background role of the widget.

  The background role defines the brush from the widget's \l palette that
  is used to render the background.

  If no explicit background role is set, the widget inherts its parent
  widget's background role.

  \sa setBackgroundRole(), foregroundRole()
 */
QPalette::ColorRole QWidget::backgroundRole() const
{

    const QWidget *w = this;
    do {
        QPalette::ColorRole role = w->d_func()->bg_role;
        if (role != QPalette::NoRole)
            return role;
        if (w->isWindow() || w->windowType() == Qt::SubWindow)
            break;
        w = w->parentWidget();
    } while (w);
    return QPalette::Window;
}

/*!
  Sets the background role of the widget to \a role.

  The background role defines the brush from the widget's \l palette that
  is used to render the background.

  If \a role is QPalette::NoRole, then the widget inherits its
  parent's background role.

  Note that styles are free to choose any color from the palette.
  You can modify the palette or set a style sheet if you don't
  achieve the result you want with setBackgroundRole().

  \sa backgroundRole(), foregroundRole()
 */

void QWidget::setBackgroundRole(QPalette::ColorRole role)
{
    Q_D(QWidget);
    d->bg_role = role;
    d->updateSystemBackground();
    d->propagatePaletteChange();
    d->updateIsOpaque();
}

/*!
  Returns the foreground role.

  The foreground role defines the color from the widget's \l palette that
  is used to draw the foreground.

  If no explicit foreground role is set, the function returns a role
  that contrasts with the background role.

  \sa setForegroundRole(), backgroundRole()
 */
QPalette::ColorRole QWidget::foregroundRole() const
{
    Q_D(const QWidget);
    QPalette::ColorRole rl = QPalette::ColorRole(d->fg_role);
    if (rl != QPalette::NoRole)
        return rl;
    QPalette::ColorRole role = QPalette::WindowText;
    switch (backgroundRole()) {
    case QPalette::Button:
        role = QPalette::ButtonText;
        break;
    case QPalette::Base:
        role = QPalette::Text;
        break;
    case QPalette::Dark:
    case QPalette::Shadow:
        role = QPalette::Light;
        break;
    case QPalette::Highlight:
        role = QPalette::HighlightedText;
        break;
    case QPalette::ToolTipBase:
        role = QPalette::ToolTipText;
        break;
    default:
        ;
    }
    return role;
}

/*!
  Sets the foreground role of the widget to \a role.

  The foreground role defines the color from the widget's \l palette that
  is used to draw the foreground.

  If \a role is QPalette::NoRole, the widget uses a foreground role
  that contrasts with the background role.

  Note that styles are free to choose any color from the palette.
  You can modify the palette or set a style sheet if you don't
  achieve the result you want with setForegroundRole().

  \sa foregroundRole(), backgroundRole()
 */
void QWidget::setForegroundRole(QPalette::ColorRole role)
{
    Q_D(QWidget);
    d->fg_role = role;
    d->updateSystemBackground();
    d->propagatePaletteChange();
}

/*!
    \property QWidget::palette
    \brief the widget's palette

    This property describes the widget's palette. The palette is used by the
    widget's style when rendering standard components, and is available as a
    means to ensure that custom widgets can maintain consistency with the
    native platform's look and feel. It's common that different platforms, or
    different styles, have different palettes.

    When you assign a new palette to a widget, the color roles from this
    palette are combined with the widget's default palette to form the
    widget's final palette. The palette entry for the widget's background role
    is used to fill the widget's background (see QWidget::autoFillBackground),
    and the foreground role initializes QPainter's pen.

    The default depends on the system environment. QApplication maintains a
    system/theme palette which serves as a default for all widgets.  There may
    also be special palette defaults for certain types of widgets (e.g., on
    Windows Vista, all classes that derive from QMenuBar have a special
    default palette). You can also define default palettes for widgets
    yourself by passing a custom palette and the name of a widget to
    QApplication::setPalette(). Finally, the style always has the option of
    polishing the palette as it's assigned (see QStyle::polish()).

    QWidget propagates explicit palette roles from parent to child. If you
    assign a brush or color to a specific role on a palette and assign that
    palette to a widget, that role will propagate to all the widget's
    children, overriding any system defaults for that role. Note that palettes
    by default don't propagate to windows (see isWindow()) unless the
    Qt::WA_WindowPropagation attribute is enabled.

    QWidget's palette propagation is similar to its font propagation.

    The current style, which is used to render the content of all standard Qt
    widgets, is free to choose colors and brushes from the widget palette, or
    in some cases, to ignore the palette (partially, or completely). In
    particular, certain styles like GTK style, Mac style, and Windows Vista
    style, depend on third party APIs to render the content of widgets,
    and these styles typically do not follow the palette. Because of this,
    assigning roles to a widget's palette is not guaranteed to change the
    appearance of the widget. Instead, you may choose to apply a \l {styleSheet}.

    \warning Do not use this function in conjunction with \l{Qt Style Sheets}.
    When using style sheets, the palette of a widget can be customized using
    the "color", "background-color", "selection-color",
    "selection-background-color" and "alternate-background-color".

    \sa QGuiApplication::palette(), QWidget::font(), {Qt Style Sheets}
*/
const QPalette &QWidget::palette() const
{
    if (!isEnabled()) {
        data->pal.setCurrentColorGroup(QPalette::Disabled);
    } else if ((!isVisible() || isActiveWindow())
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
        && !QApplicationPrivate::isBlockedByModal(const_cast<QWidget *>(this))
#endif
        ) {
        data->pal.setCurrentColorGroup(QPalette::Active);
    } else {
        data->pal.setCurrentColorGroup(QPalette::Inactive);
    }
    return data->pal;
}

void QWidget::setPalette(const QPalette &palette)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_SetPalette, palette.resolve() != 0);

    // Determine which palette is inherited from this widget's ancestors and
    // QApplication::palette, resolve this against \a palette (attributes from
    // the inherited palette are copied over this widget's palette). Then
    // propagate this palette to this widget's children.
    QPalette naturalPalette = d->naturalWidgetPalette(d->inheritedPaletteResolveMask);
    QPalette resolvedPalette = palette.resolve(naturalPalette);
    d->setPalette_helper(resolvedPalette);
}

/*!
    \internal

    Returns the palette that the widget \a w inherits from its ancestors and
    QApplication::palette. \a inheritedMask is the combination of the widget's
    ancestors palette request masks (i.e., which attributes from the parent
    widget's palette are implicitly imposed on this widget by the user). Note
    that this font does not take into account the palette set on \a w itself.
*/
QPalette QWidgetPrivate::naturalWidgetPalette(uint inheritedMask) const
{
    Q_Q(const QWidget);

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    QPalette naturalPalette = QApplication::palette(q);
    if ((!q->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles)
        && (!q->isWindow() || q->testAttribute(Qt::WA_WindowPropagation)
#if QT_CONFIG(graphicsview)
            || (extra && extra->proxyWidget)
#endif // QT_CONFIG(graphicsview)
            )) {
        if (QWidget *p = q->parentWidget()) {
            if (!p->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles) {
                if (!naturalPalette.isCopyOf(QGuiApplication::palette())) {
                    QPalette inheritedPalette = p->palette();
                    inheritedPalette.resolve(inheritedMask);
                    naturalPalette = inheritedPalette.resolve(naturalPalette);
                } else {
                    naturalPalette = p->palette();
                }
            }
        }
#if QT_CONFIG(graphicsview)
        else if (extra && extra->proxyWidget) {
            QPalette inheritedPalette = extra->proxyWidget->palette();
            inheritedPalette.resolve(inheritedMask);
            naturalPalette = inheritedPalette.resolve(naturalPalette);
        }
#endif // QT_CONFIG(graphicsview)
    }
    naturalPalette.resolve(0);
    return naturalPalette;
}
/*!
    \internal

    Determine which palette is inherited from this widget's ancestors and
    QApplication::palette, resolve this against this widget's palette
    (attributes from the inherited palette are copied over this widget's
    palette). Then propagate this palette to this widget's children.
*/
void QWidgetPrivate::resolvePalette()
{
    QPalette naturalPalette = naturalWidgetPalette(inheritedPaletteResolveMask);
    QPalette resolvedPalette = data.pal.resolve(naturalPalette);
    setPalette_helper(resolvedPalette);
}

void QWidgetPrivate::setPalette_helper(const QPalette &palette)
{
    Q_Q(QWidget);
    if (data.pal == palette && data.pal.resolve() == palette.resolve())
        return;
    data.pal = palette;
    updateSystemBackground();
    propagatePaletteChange();
    updateIsOpaque();
    q->update();
    updateIsOpaque();
}

void QWidgetPrivate::updateSystemBackground()
{
}

/*!
    \property QWidget::font
    \brief the font currently set for the widget

    This property describes the widget's requested font. The font is used by
    the widget's style when rendering standard components, and is available as
    a means to ensure that custom widgets can maintain consistency with the
    native platform's look and feel. It's common that different platforms, or
    different styles, define different fonts for an application.

    When you assign a new font to a widget, the properties from this font are
    combined with the widget's default font to form the widget's final
    font. You can call fontInfo() to get a copy of the widget's final
    font. The final font is also used to initialize QPainter's font.

    The default depends on the system environment. QApplication maintains a
    system/theme font which serves as a default for all widgets.  There may
    also be special font defaults for certain types of widgets. You can also
    define default fonts for widgets yourself by passing a custom font and the
    name of a widget to QApplication::setFont(). Finally, the font is matched
    against Qt's font database to find the best match.

    QWidget propagates explicit font properties from parent to child. If you
    change a specific property on a font and assign that font to a widget,
    that property will propagate to all the widget's children, overriding any
    system defaults for that property. Note that fonts by default don't
    propagate to windows (see isWindow()) unless the Qt::WA_WindowPropagation
    attribute is enabled.

    QWidget's font propagation is similar to its palette propagation.

    The current style, which is used to render the content of all standard Qt
    widgets, is free to choose to use the widget font, or in some cases, to
    ignore it (partially, or completely). In particular, certain styles like
    GTK style, Mac style, and Windows Vista style, apply special
    modifications to the widget font to match the platform's native look and
    feel. Because of this, assigning properties to a widget's font is not
    guaranteed to change the appearance of the widget. Instead, you may choose
    to apply a \l styleSheet.

    \note If \l{Qt Style Sheets} are used on the same widget as setFont(),
    style sheets will take precedence if the settings conflict.

    \sa fontInfo(), fontMetrics()
*/

void QWidget::setFont(const QFont &font)
{
    Q_D(QWidget);

#ifndef QT_NO_STYLE_STYLESHEET
    const QStyleSheetStyle* style;
    if (d->extra && (style = qt_styleSheet(d->extra->style)))
        style->saveWidgetFont(this, font);
#endif

    setAttribute(Qt::WA_SetFont, font.resolve() != 0);

    // Determine which font is inherited from this widget's ancestors and
    // QApplication::font, resolve this against \a font (attributes from the
    // inherited font are copied over). Then propagate this font to this
    // widget's children.
    QFont naturalFont = d->naturalWidgetFont(d->inheritedFontResolveMask);
    QFont resolvedFont = font.resolve(naturalFont);
    d->setFont_helper(resolvedFont);
}

/*
    \internal

    Returns the font that the widget \a w inherits from its ancestors and
    QApplication::font. \a inheritedMask is the combination of the widget's
    ancestors font request masks (i.e., which attributes from the parent
    widget's font are implicitly imposed on this widget by the user). Note
    that this font does not take into account the font set on \a w itself.

    ### Stylesheet has a different font propagation mechanism. When a stylesheet
        is applied, fonts are not propagated anymore
*/
QFont QWidgetPrivate::naturalWidgetFont(uint inheritedMask) const
{
    Q_Q(const QWidget);

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    QFont naturalFont = QApplication::font(q);
    if ((!q->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles)
        && (!q->isWindow() || q->testAttribute(Qt::WA_WindowPropagation)
#if QT_CONFIG(graphicsview)
            || (extra && extra->proxyWidget)
#endif // QT_CONFIG(graphicsview)
            )) {
        if (QWidget *p = q->parentWidget()) {
            if (!p->testAttribute(Qt::WA_StyleSheet) || useStyleSheetPropagationInWidgetStyles) {
                if (!naturalFont.isCopyOf(QApplication::font())) {
                    if (inheritedMask != 0) {
                        QFont inheritedFont = p->font();
                        inheritedFont.resolve(inheritedMask);
                        naturalFont = inheritedFont.resolve(naturalFont);
                    } // else nothing to do (naturalFont = naturalFont)
                } else {
                    naturalFont = p->font();
                }
            }
        }
#if QT_CONFIG(graphicsview)
        else if (extra && extra->proxyWidget) {
            if (inheritedMask != 0) {
                QFont inheritedFont = extra->proxyWidget->font();
                inheritedFont.resolve(inheritedMask);
                naturalFont = inheritedFont.resolve(naturalFont);
            } // else nothing to do (naturalFont = naturalFont)
        }
#endif // QT_CONFIG(graphicsview)
    }
    naturalFont.resolve(0);
    return naturalFont;
}

/*!
    \internal

    Returns a font suitable for inheritance, where only locally set attributes are considered resolved.
*/
QFont QWidgetPrivate::localFont() const
{
    QFont localfont = data.fnt;
    localfont.resolve(directFontResolveMask);
    return localfont;
}

/*!
    \internal

    Determine which font is implicitly imposed on this widget by its ancestors
    and QApplication::font, resolve this against its own font (attributes from
    the implicit font are copied over). Then propagate this font to this
    widget's children.
*/
void QWidgetPrivate::resolveFont()
{
    QFont naturalFont = naturalWidgetFont(inheritedFontResolveMask);
    QFont resolvedFont = localFont().resolve(naturalFont);
    setFont_helper(resolvedFont);
}

/*!
    \internal

    Assign \a font to this widget, and propagate it to all children, except
    style sheet widgets (handled differently) and windows that don't enable
    window propagation.  \a implicitMask is the union of all ancestor widgets'
    font request masks, and determines which attributes from this widget's
    font should propagate.
*/
void QWidgetPrivate::updateFont(const QFont &font)
{
    Q_Q(QWidget);
#ifndef QT_NO_STYLE_STYLESHEET
    const QStyleSheetStyle* cssStyle;
    cssStyle = extra ? qt_styleSheet(extra->style) : nullptr;
    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);
#endif

    data.fnt = QFont(font, q);

    // Combine new mask with natural mask and propagate to children.
#if QT_CONFIG(graphicsview)
    if (!q->parentWidget() && extra && extra->proxyWidget) {
        QGraphicsProxyWidget *p = extra->proxyWidget;
        inheritedFontResolveMask = p->d_func()->inheritedFontResolveMask | p->font().resolve();
    } else
#endif // QT_CONFIG(graphicsview)
    if (q->isWindow() && !q->testAttribute(Qt::WA_WindowPropagation)) {
        inheritedFontResolveMask = 0;
    }
    uint newMask = data.fnt.resolve() | inheritedFontResolveMask;
    // Set the font as also having resolved inherited traits, so the result of reading QWidget::font()
    // isn't all weak information, but save the original mask to be able to let new changes on the
    // parent widget font propagate correctly.
    directFontResolveMask = data.fnt.resolve();
    data.fnt.resolve(newMask);

    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget*>(children.at(i));
        if (w) {
            if (0) {
#ifndef QT_NO_STYLE_STYLESHEET
            } else if (!useStyleSheetPropagationInWidgetStyles && w->testAttribute(Qt::WA_StyleSheet)) {
                // Style sheets follow a different font propagation scheme.
                if (cssStyle)
                    cssStyle->updateStyleSheetFont(w);
#endif
            } else if ((!w->isWindow() || w->testAttribute(Qt::WA_WindowPropagation))) {
                // Propagate font changes.
                QWidgetPrivate *wd = w->d_func();
                wd->inheritedFontResolveMask = newMask;
                wd->resolveFont();
            }
        }
    }

#ifndef QT_NO_STYLE_STYLESHEET
    if (!useStyleSheetPropagationInWidgetStyles && cssStyle) {
        cssStyle->updateStyleSheetFont(q);
    }
#endif

    QEvent e(QEvent::FontChange);
    QCoreApplication::sendEvent(q, &e);
}

void QWidgetPrivate::setLayoutDirection_helper(Qt::LayoutDirection direction)
{
    Q_Q(QWidget);

    if ( (direction == Qt::RightToLeft) == q->testAttribute(Qt::WA_RightToLeft))
        return;
    q->setAttribute(Qt::WA_RightToLeft, (direction == Qt::RightToLeft));
    if (!children.isEmpty()) {
        for (int i = 0; i < children.size(); ++i) {
            QWidget *w = qobject_cast<QWidget*>(children.at(i));
            if (w && !w->isWindow() && !w->testAttribute(Qt::WA_SetLayoutDirection))
                w->d_func()->setLayoutDirection_helper(direction);
        }
    }
    QEvent e(QEvent::LayoutDirectionChange);
    QCoreApplication::sendEvent(q, &e);
}

void QWidgetPrivate::resolveLayoutDirection()
{
    Q_Q(const QWidget);
    if (!q->testAttribute(Qt::WA_SetLayoutDirection))
        setLayoutDirection_helper(q->isWindow() ? QGuiApplication::layoutDirection() : q->parentWidget()->layoutDirection());
}

/*!
    \property QWidget::layoutDirection

    \brief the layout direction for this widget

    By default, this property is set to Qt::LeftToRight.

    When the layout direction is set on a widget, it will propagate to
    the widget's children, but not to a child that is a window and not
    to a child for which setLayoutDirection() has been explicitly
    called. Also, child widgets added \e after setLayoutDirection()
    has been called for the parent do not inherit the parent's layout
    direction.

    This method no longer affects text layout direction since Qt 4.7.

    \sa QApplication::layoutDirection
*/
void QWidget::setLayoutDirection(Qt::LayoutDirection direction)
{
    Q_D(QWidget);

    if (direction == Qt::LayoutDirectionAuto) {
        unsetLayoutDirection();
        return;
    }

    setAttribute(Qt::WA_SetLayoutDirection);
    d->setLayoutDirection_helper(direction);
}

Qt::LayoutDirection QWidget::layoutDirection() const
{
    return testAttribute(Qt::WA_RightToLeft) ? Qt::RightToLeft : Qt::LeftToRight;
}

void QWidget::unsetLayoutDirection()
{
    Q_D(QWidget);
    setAttribute(Qt::WA_SetLayoutDirection, false);
    d->resolveLayoutDirection();
}

/*!
    \fn QFontMetrics QWidget::fontMetrics() const

    Returns the font metrics for the widget's current font.
    Equivalent to \c QFontMetrics(widget->font()).

    \sa font(), fontInfo(), setFont()
*/

/*!
    \fn QFontInfo QWidget::fontInfo() const

    Returns the font info for the widget's current font.
    Equivalent to \c QFontInfo(widget->font()).

    \sa font(), fontMetrics(), setFont()
*/


/*!
    \property QWidget::cursor
    \brief the cursor shape for this widget

    The mouse cursor will assume this shape when it's over this
    widget. See the \l{Qt::CursorShape}{list of predefined cursor objects} for a range of useful shapes.

    An editor widget might use an I-beam cursor:
    \snippet code/src_gui_kernel_qwidget.cpp 6

    If no cursor has been set, or after a call to unsetCursor(), the
    parent's cursor is used.

    By default, this property contains a cursor with the Qt::ArrowCursor
    shape.

    Some underlying window implementations will reset the cursor if it
    leaves a widget even if the mouse is grabbed. If you want to have
    a cursor set for all widgets, even when outside the window, consider
    QGuiApplication::setOverrideCursor().

    \sa QGuiApplication::setOverrideCursor()
*/

#ifndef QT_NO_CURSOR
QCursor QWidget::cursor() const
{
    Q_D(const QWidget);
    if (testAttribute(Qt::WA_SetCursor))
        return (d->extra && d->extra->curs)
            ? *d->extra->curs
            : QCursor(Qt::ArrowCursor);
    if (isWindow() || !parentWidget())
        return QCursor(Qt::ArrowCursor);
    return parentWidget()->cursor();
}

void QWidget::setCursor(const QCursor &cursor)
{
    Q_D(QWidget);
    if (cursor.shape() != Qt::ArrowCursor
        || (d->extra && d->extra->curs))
    {
        d->createExtra();
        d->extra->curs = qt_make_unique<QCursor>(cursor);
    }
    setAttribute(Qt::WA_SetCursor);
    d->setCursor_sys(cursor);

    QEvent event(QEvent::CursorChange);
    QCoreApplication::sendEvent(this, &event);
}

void QWidgetPrivate::setCursor_sys(const QCursor &cursor)
{
    Q_UNUSED(cursor);
    Q_Q(QWidget);
    qt_qpa_set_cursor(q, false);
}

void QWidget::unsetCursor()
{
    Q_D(QWidget);
    if (d->extra)
        d->extra->curs.reset();
    if (!isWindow())
        setAttribute(Qt::WA_SetCursor, false);
    d->unsetCursor_sys();

    QEvent event(QEvent::CursorChange);
    QCoreApplication::sendEvent(this, &event);
}

void QWidgetPrivate::unsetCursor_sys()
{
    Q_Q(QWidget);
    qt_qpa_set_cursor(q, false);
}

static inline void applyCursor(QWidget *w, const QCursor &c)
{
    if (QWindow *window = w->windowHandle())
        window->setCursor(c);
}

static inline void unsetCursor(QWidget *w)
{
    if (QWindow *window = w->windowHandle())
        window->unsetCursor();
}

void qt_qpa_set_cursor(QWidget *w, bool force)
{
    if (!w->testAttribute(Qt::WA_WState_Created))
        return;

    static QPointer<QWidget> lastUnderMouse = nullptr;
    if (force) {
        lastUnderMouse = w;
    } else if (lastUnderMouse) {
        const WId lastWinId = lastUnderMouse->effectiveWinId();
        const WId winId = w->effectiveWinId();
        if (lastWinId && lastWinId == winId)
            w = lastUnderMouse;
    } else if (!w->internalWinId()) {
        return; // The mouse is not under this widget, and it's not native, so don't change it.
    }

    while (!w->internalWinId() && w->parentWidget() && !w->isWindow()
           && !w->testAttribute(Qt::WA_SetCursor))
        w = w->parentWidget();

    QWidget *nativeParent = w;
    if (!w->internalWinId())
        nativeParent = w->nativeParentWidget();
    if (!nativeParent || !nativeParent->internalWinId())
        return;

    if (w->isWindow() || w->testAttribute(Qt::WA_SetCursor)) {
        if (w->isEnabled())
            applyCursor(nativeParent, w->cursor());
        else
            // Enforce the windows behavior of clearing the cursor on
            // disabled widgets.
            unsetCursor(nativeParent);
    } else {
        unsetCursor(nativeParent);
    }
}
#endif

/*!
    \enum QWidget::RenderFlag

    This enum describes how to render the widget when calling QWidget::render().

    \value DrawWindowBackground If you enable this option, the widget's background
    is rendered into the target even if autoFillBackground is not set. By default,
    this option is enabled.

    \value DrawChildren If you enable this option, the widget's children
    are rendered recursively into the target. By default, this option is enabled.

    \value IgnoreMask If you enable this option, the widget's QWidget::mask()
    is ignored when rendering into the target. By default, this option is disabled.

    \since 4.3
*/

/*!
    \since 4.3

    Renders the \a sourceRegion of this widget into the \a target
    using \a renderFlags to determine how to render. Rendering
    starts at \a targetOffset in the \a target. For example:

    \snippet code/src_gui_kernel_qwidget.cpp 7

    If \a sourceRegion is a null region, this function will use QWidget::rect() as
    the region, i.e. the entire widget.

    Ensure that you call QPainter::end() for the \a target device's
    active painter (if any) before rendering. For example:

    \snippet code/src_gui_kernel_qwidget.cpp 8

    \note To obtain the contents of a QOpenGLWidget, use QOpenGLWidget::grabFramebuffer()
    instead.

    \note To obtain the contents of a QGLWidget (deprecated), use
    QGLWidget::grabFrameBuffer() or QGLWidget::renderPixmap() instead.
*/
void QWidget::render(QPaintDevice *target, const QPoint &targetOffset,
                     const QRegion &sourceRegion, RenderFlags renderFlags)
{
    QPainter p(target);
    render(&p, targetOffset, sourceRegion, renderFlags);
}

/*!
    \overload

    Renders the widget into the \a painter's QPainter::device().

    Transformations and settings applied to the \a painter will be used
    when rendering.

    \note The \a painter must be active. On \macos the widget will be
    rendered into a QPixmap and then drawn by the \a painter.

    \sa QPainter::device()
*/
void QWidget::render(QPainter *painter, const QPoint &targetOffset,
                     const QRegion &sourceRegion, RenderFlags renderFlags)
{
    if (Q_UNLIKELY(!painter)) {
        qWarning("QWidget::render: Null pointer to painter");
        return;
    }

    if (Q_UNLIKELY(!painter->isActive())) {
        qWarning("QWidget::render: Cannot render with an inactive painter");
        return;
    }

    const qreal opacity = painter->opacity();
    if (qFuzzyIsNull(opacity))
        return; // Fully transparent.

    Q_D(QWidget);
    const bool inRenderWithPainter = d->extra && d->extra->inRenderWithPainter;
    const QRegion toBePainted = !inRenderWithPainter ? d->prepareToRender(sourceRegion, renderFlags)
                                                     : sourceRegion;
    if (toBePainted.isEmpty())
        return;

    if (!d->extra)
        d->createExtra();
    d->extra->inRenderWithPainter = true;

    QPaintEngine *engine = painter->paintEngine();
    Q_ASSERT(engine);
    QPaintEnginePrivate *enginePriv = engine->d_func();
    Q_ASSERT(enginePriv);
    QPaintDevice *target = engine->paintDevice();
    Q_ASSERT(target);

    // Render via a pixmap when dealing with non-opaque painters or printers.
    if (!inRenderWithPainter && (opacity < 1.0 || (target->devType() == QInternal::Printer))) {
        d->render_helper(painter, targetOffset, toBePainted, renderFlags);
        d->extra->inRenderWithPainter = inRenderWithPainter;
        return;
    }

    // Set new shared painter.
    QPainter *oldPainter = d->sharedPainter();
    d->setSharedPainter(painter);

    // Save current system clip, viewport and transform,
    const QTransform oldTransform = enginePriv->systemTransform;
    const QRegion oldSystemClip = enginePriv->systemClip;
    const QRegion oldBaseClip = enginePriv->baseSystemClip;
    const QRegion oldSystemViewport = enginePriv->systemViewport;

    // This ensures that all painting triggered by render() is clipped to the current engine clip.
    if (painter->hasClipping()) {
        const QRegion painterClip = painter->deviceTransform().map(painter->clipRegion());
        enginePriv->setSystemViewport(oldSystemClip.isEmpty() ? painterClip : oldSystemClip & painterClip);
    } else {
        enginePriv->setSystemViewport(oldSystemClip);
    }

    d->render(target, targetOffset, toBePainted, renderFlags);

    // Restore system clip, viewport and transform.
    enginePriv->baseSystemClip = oldBaseClip;
    enginePriv->setSystemTransformAndViewport(oldTransform, oldSystemViewport);
    enginePriv->systemStateChanged();

    // Restore shared painter.
    d->setSharedPainter(oldPainter);

    d->extra->inRenderWithPainter = inRenderWithPainter;
}

static void sendResizeEvents(QWidget *target)
{
    QResizeEvent e(target->size(), QSize());
    QCoreApplication::sendEvent(target, &e);

    const QObjectList children = target->children();
    for (int i = 0; i < children.size(); ++i) {
        if (!children.at(i)->isWidgetType())
            continue;
        QWidget *child = static_cast<QWidget*>(children.at(i));
        if (!child->isWindow() && child->testAttribute(Qt::WA_PendingResizeEvent))
            sendResizeEvents(child);
    }
}

/*!
    \since 5.0

    Renders the widget into a pixmap restricted by the
    given \a rectangle. If the widget has any children, then
    they are also painted in the appropriate positions.

    If a rectangle with an invalid size is specified  (the default),
    the entire widget is painted.

    \sa render(), QPixmap
*/
QPixmap QWidget::grab(const QRect &rectangle)
{
    Q_D(QWidget);
    if (testAttribute(Qt::WA_PendingResizeEvent) || !testAttribute(Qt::WA_WState_Created))
        sendResizeEvents(this);

    const QWidget::RenderFlags renderFlags = QWidget::DrawWindowBackground | QWidget::DrawChildren | QWidget::IgnoreMask;

    const bool oldDirtyOpaqueChildren =  d->dirtyOpaqueChildren;
    QRect r(rectangle);
    if (r.width() < 0 || r.height() < 0) {
        // For grabbing widgets that haven't been shown yet,
        // we trigger the layouting mechanism to determine the widget's size.
        r = d->prepareToRender(QRegion(), renderFlags).boundingRect();
        r.setTopLeft(rectangle.topLeft());
    }

    if (!r.intersects(rect()))
        return QPixmap();

    const qreal dpr = devicePixelRatioF();
    QPixmap res((QSizeF(r.size()) * dpr).toSize());
    res.setDevicePixelRatio(dpr);
    if (!d->isOpaque)
        res.fill(Qt::transparent);
    d->render(&res, QPoint(), QRegion(r), renderFlags);

    d->dirtyOpaqueChildren = oldDirtyOpaqueChildren;
    return res;
}

/*!
    \brief The graphicsEffect function returns a pointer to the
    widget's graphics effect.

    If the widget has no graphics effect, \nullptr is returned.

    \since 4.6

    \sa setGraphicsEffect()
*/
#if QT_CONFIG(graphicseffect)
QGraphicsEffect *QWidget::graphicsEffect() const
{
    Q_D(const QWidget);
    return d->graphicsEffect;
}
#endif // QT_CONFIG(graphicseffect)

/*!

  \brief The setGraphicsEffect function is for setting the widget's graphics effect.

    Sets \a effect as the widget's effect. If there already is an effect installed
    on this widget, QWidget will delete the existing effect before installing
    the new \a effect.

    If \a effect is the installed effect on a different widget, setGraphicsEffect() will remove
    the effect from the widget and install it on this widget.

    QWidget takes ownership of \a effect.

    \note This function will apply the effect on itself and all its children.

    \note Graphics effects are not supported for OpenGL-based widgets, such as QGLWidget,
    QOpenGLWidget and QQuickWidget.

    \since 4.6

    \sa graphicsEffect()
*/
#if QT_CONFIG(graphicseffect)
void QWidget::setGraphicsEffect(QGraphicsEffect *effect)
{
    Q_D(QWidget);
    if (d->graphicsEffect == effect)
        return;

    if (d->graphicsEffect) {
        d->invalidateBackingStore(rect());
        delete d->graphicsEffect;
        d->graphicsEffect = nullptr;
    }

    if (effect) {
        // Set new effect.
        QGraphicsEffectSourcePrivate *sourced = new QWidgetEffectSourcePrivate(this);
        QGraphicsEffectSource *source = new QGraphicsEffectSource(*sourced);
        d->graphicsEffect = effect;
        effect->d_func()->setGraphicsEffectSource(source);
        update();
    }

    d->updateIsOpaque();
}
#endif // QT_CONFIG(graphicseffect)

bool QWidgetPrivate::isAboutToShow() const
{
    if (data.in_show)
        return true;

    Q_Q(const QWidget);
    if (q->isHidden())
        return false;

    // The widget will be shown if any of its ancestors are about to show.
    QWidget *parent = q->parentWidget();
    return parent ? parent->d_func()->isAboutToShow() : false;
}

QRegion QWidgetPrivate::prepareToRender(const QRegion &region, QWidget::RenderFlags renderFlags)
{
    Q_Q(QWidget);
    const bool isVisible = q->isVisible();

    // Make sure the widget is laid out correctly.
    if (!isVisible && !isAboutToShow()) {
        QWidget *topLevel = q->window();
        (void)topLevel->d_func()->topData(); // Make sure we at least have top-data.
        topLevel->ensurePolished();

        // Invalidate the layout of hidden ancestors (incl. myself) and pretend
        // they're not explicitly hidden.
        QWidget *widget = q;
        QWidgetList hiddenWidgets;
        while (widget) {
            if (widget->isHidden()) {
                widget->setAttribute(Qt::WA_WState_Hidden, false);
                hiddenWidgets.append(widget);
                if (!widget->isWindow() && widget->parentWidget()->d_func()->layout)
                    widget->d_func()->updateGeometry_helper(true);
            }
            widget = widget->parentWidget();
        }

        // Activate top-level layout.
        if (topLevel->d_func()->layout)
            topLevel->d_func()->layout->activate();

        // Adjust size if necessary.
        QTLWExtra *topLevelExtra = topLevel->d_func()->maybeTopData();
        if (topLevelExtra && !topLevelExtra->sizeAdjusted
            && !topLevel->testAttribute(Qt::WA_Resized)) {
            topLevel->adjustSize();
            topLevel->setAttribute(Qt::WA_Resized, false);
        }

        // Activate child layouts.
        topLevel->d_func()->activateChildLayoutsRecursively();

        // We're not cheating with WA_WState_Hidden anymore.
        for (int i = 0; i < hiddenWidgets.size(); ++i) {
            QWidget *widget = hiddenWidgets.at(i);
            widget->setAttribute(Qt::WA_WState_Hidden);
            if (!widget->isWindow() && widget->parentWidget()->d_func()->layout)
                widget->parentWidget()->d_func()->layout->invalidate();
        }
    } else if (isVisible) {
        q->window()->d_func()->sendPendingMoveAndResizeEvents(true, true);
    }

    // Calculate the region to be painted.
    QRegion toBePainted = !region.isEmpty() ? region : QRegion(q->rect());
    if (!(renderFlags & QWidget::IgnoreMask) && extra && extra->hasMask)
        toBePainted &= extra->mask;
    return toBePainted;
}

void QWidgetPrivate::render_helper(QPainter *painter, const QPoint &targetOffset, const QRegion &toBePainted,
                                   QWidget::RenderFlags renderFlags)
{
    Q_ASSERT(painter);
    Q_ASSERT(!toBePainted.isEmpty());

    Q_Q(QWidget);
    const QTransform originalTransform = painter->worldTransform();
    const bool useDeviceCoordinates = originalTransform.isScaling();
    if (!useDeviceCoordinates) {
        // Render via a pixmap.
        const QRect rect = toBePainted.boundingRect();
        const QSize size = rect.size();
        if (size.isNull())
            return;

        const qreal pixmapDevicePixelRatio = painter->device()->devicePixelRatioF();
        QPixmap pixmap(size * pixmapDevicePixelRatio);
        pixmap.setDevicePixelRatio(pixmapDevicePixelRatio);

        if (!(renderFlags & QWidget::DrawWindowBackground) || !isOpaque)
            pixmap.fill(Qt::transparent);
        q->render(&pixmap, QPoint(), toBePainted, renderFlags);

        const bool restore = !(painter->renderHints() & QPainter::SmoothPixmapTransform);
        painter->setRenderHints(QPainter::SmoothPixmapTransform, true);

        painter->drawPixmap(targetOffset, pixmap);

        if (restore)
            painter->setRenderHints(QPainter::SmoothPixmapTransform, false);

    } else {
        // Render via a pixmap in device coordinates (to avoid pixmap scaling).
        QTransform transform = originalTransform;
        transform.translate(targetOffset.x(), targetOffset.y());

        QPaintDevice *device = painter->device();
        Q_ASSERT(device);

        // Calculate device rect.
        const QRectF rect(toBePainted.boundingRect());
        QRect deviceRect = transform.mapRect(QRectF(0, 0, rect.width(), rect.height())).toAlignedRect();
        deviceRect &= QRect(0, 0, device->width(), device->height());

        QPixmap pixmap(deviceRect.size());
        pixmap.fill(Qt::transparent);

        // Create a pixmap device coordinate painter.
        QPainter pixmapPainter(&pixmap);
        pixmapPainter.setRenderHints(painter->renderHints());
        transform *= QTransform::fromTranslate(-deviceRect.x(), -deviceRect.y());
        pixmapPainter.setTransform(transform);

        q->render(&pixmapPainter, QPoint(), toBePainted, renderFlags);
        pixmapPainter.end();

        // And then draw the pixmap.
        painter->setTransform(QTransform());
        painter->drawPixmap(deviceRect.topLeft(), pixmap);
        painter->setTransform(originalTransform);
    }
}

void QWidgetPrivate::drawWidget(QPaintDevice *pdev, const QRegion &rgn, const QPoint &offset, DrawWidgetFlags flags,
                                QPainter *sharedPainter, QWidgetRepaintManager *repaintManager)
{
    if (rgn.isEmpty())
        return;

    Q_Q(QWidget);

    qCInfo(lcWidgetPainting) << "Drawing" << rgn << "of" << q << "at" << offset
        << "into paint device" << pdev << "with" << flags;

    const bool asRoot = flags & DrawAsRoot;
    bool onScreen = shouldPaintOnScreen();

#if QT_CONFIG(graphicseffect)
    if (graphicsEffect && graphicsEffect->isEnabled()) {
        QGraphicsEffectSource *source = graphicsEffect->d_func()->source;
        QWidgetEffectSourcePrivate *sourced = static_cast<QWidgetEffectSourcePrivate *>
                                                         (source->d_func());
        if (!sourced->context) {
            QWidgetPaintContext context(pdev, rgn, offset, flags, sharedPainter, repaintManager);
            sourced->context = &context;
            if (!sharedPainter) {
                setSystemClip(pdev->paintEngine(), pdev->devicePixelRatioF(), rgn.translated(offset));
                QPainter p(pdev);
                p.translate(offset);
                context.painter = &p;
                graphicsEffect->draw(&p);
                setSystemClip(pdev->paintEngine(), 1, QRegion());
            } else {
                context.painter = sharedPainter;
                if (sharedPainter->worldTransform() != sourced->lastEffectTransform) {
                    sourced->invalidateCache();
                    sourced->lastEffectTransform = sharedPainter->worldTransform();
                }
                sharedPainter->save();
                sharedPainter->translate(offset);
                setSystemClip(sharedPainter->paintEngine(), sharedPainter->device()->devicePixelRatioF(), rgn.translated(offset));
                graphicsEffect->draw(sharedPainter);
                setSystemClip(sharedPainter->paintEngine(), 1, QRegion());
                sharedPainter->restore();
            }
            sourced->context = nullptr;

            if (repaintManager)
                repaintManager->markNeedsFlush(q, rgn, offset);

            return;
        }
    }
#endif // QT_CONFIG(graphicseffect)

    const bool alsoOnScreen = flags & DrawPaintOnScreen;
    const bool recursive = flags & DrawRecursive;
    const bool alsoInvisible = flags & DrawInvisible;

    Q_ASSERT(sharedPainter ? sharedPainter->isActive() : true);

    QRegion toBePainted(rgn);
    if (asRoot && !alsoInvisible)
        toBePainted &= clipRect(); //(rgn & visibleRegion());
    if (!(flags & DontSubtractOpaqueChildren))
        subtractOpaqueChildren(toBePainted, q->rect());

    if (!toBePainted.isEmpty()) {
        if (!onScreen || alsoOnScreen) {
            //update the "in paint event" flag
            if (Q_UNLIKELY(q->testAttribute(Qt::WA_WState_InPaintEvent)))
                qWarning("QWidget::repaint: Recursive repaint detected");
            q->setAttribute(Qt::WA_WState_InPaintEvent);

            //clip away the new area
            QPaintEngine *paintEngine = pdev->paintEngine();
            if (paintEngine) {
                setRedirected(pdev, -offset);

                if (sharedPainter)
                    setSystemClip(pdev->paintEngine(), pdev->devicePixelRatioF(), toBePainted);
                else
                    paintEngine->d_func()->systemRect = q->data->crect;

                //paint the background
                if ((asRoot || q->autoFillBackground() || onScreen || q->testAttribute(Qt::WA_StyledBackground))
                    && !q->testAttribute(Qt::WA_OpaquePaintEvent) && !q->testAttribute(Qt::WA_NoSystemBackground)) {
#ifndef QT_NO_OPENGL
                    beginBackingStorePainting();
#endif
                    QPainter p(q);
                    paintBackground(&p, toBePainted, (asRoot || onScreen) ? (flags | DrawAsRoot) : DrawWidgetFlags());
#ifndef QT_NO_OPENGL
                    endBackingStorePainting();
#endif
                }

                if (!sharedPainter)
                    setSystemClip(pdev->paintEngine(), pdev->devicePixelRatioF(), toBePainted.translated(offset));

                if (!onScreen && !asRoot && !isOpaque && q->testAttribute(Qt::WA_TintedBackground)) {
#ifndef QT_NO_OPENGL
                    beginBackingStorePainting();
#endif
                    QPainter p(q);
                    QColor tint = q->palette().window().color();
                    tint.setAlphaF(qreal(.6));
                    p.fillRect(toBePainted.boundingRect(), tint);
#ifndef QT_NO_OPENGL
                    endBackingStorePainting();
#endif
                }
            }

#if 0
            qDebug() << "painting" << q << "opaque ==" << isOpaque();
            qDebug() << "clipping to" << toBePainted << "location == " << offset
                     << "geometry ==" << QRect(q->mapTo(q->window(), QPoint(0, 0)), q->size());
#endif

            bool skipPaintEvent = false;
#ifndef QT_NO_OPENGL
            if (renderToTexture) {
                // This widget renders into a texture which is composed later. We just need to
                // punch a hole in the backingstore, so the texture will be visible.
                beginBackingStorePainting();
                if (!q->testAttribute(Qt::WA_AlwaysStackOnTop) && repaintManager) {
                    QPainter p(q);
                    p.setCompositionMode(QPainter::CompositionMode_Source);
                    p.fillRect(q->rect(), Qt::transparent);
                } else if (!repaintManager) {
                    // We are not drawing to a backingstore: fall back to QImage
                    QImage img = grabFramebuffer();
                    // grabFramebuffer() always sets the format to RGB32
                    // regardless of whether it is transparent or not.
                    if (img.format() == QImage::Format_RGB32)
                        img.reinterpretAsFormat(QImage::Format_ARGB32_Premultiplied);
                    QPainter p(q);
                    p.drawImage(q->rect(), img);
                    skipPaintEvent = true;
                }
                endBackingStorePainting();
                if (renderToTextureReallyDirty)
                    renderToTextureReallyDirty = 0;
                else
                    skipPaintEvent = true;
            }
#endif // QT_NO_OPENGL

            if (!skipPaintEvent) {
                //actually send the paint event
                sendPaintEvent(toBePainted);
            }

            if (repaintManager)
                repaintManager->markNeedsFlush(q, toBePainted, offset);

            //restore
            if (paintEngine) {
                restoreRedirected();
                if (!sharedPainter)
                    paintEngine->d_func()->systemRect = QRect();
                else
                    paintEngine->d_func()->currentClipDevice = nullptr;

                setSystemClip(pdev->paintEngine(), 1, QRegion());
            }
            q->setAttribute(Qt::WA_WState_InPaintEvent, false);
            if (Q_UNLIKELY(q->paintingActive()))
                qWarning("QWidget::repaint: It is dangerous to leave painters active on a widget outside of the PaintEvent");

            if (paintEngine && paintEngine->autoDestruct()) {
                delete paintEngine;
            }
        } else if (q->isWindow()) {
            QPaintEngine *engine = pdev->paintEngine();
            if (engine) {
                QPainter p(pdev);
                p.setClipRegion(toBePainted);
                const QBrush bg = q->palette().brush(QPalette::Window);
                if (bg.style() == Qt::TexturePattern)
                    p.drawTiledPixmap(q->rect(), bg.texture());
                else
                    p.fillRect(q->rect(), bg);

                if (engine->autoDestruct())
                    delete engine;
            }
        }
    }

    if (recursive && !children.isEmpty()) {
        paintSiblingsRecursive(pdev, children, children.size() - 1, rgn, offset, flags & ~DrawAsRoot,
                               sharedPainter, repaintManager);
    }
}

void QWidgetPrivate::sendPaintEvent(const QRegion &toBePainted)
{
    Q_Q(QWidget);
    QPaintEvent e(toBePainted);
    QCoreApplication::sendSpontaneousEvent(q, &e);

#ifndef QT_NO_OPENGL
    if (renderToTexture)
        resolveSamples();
#endif // QT_NO_OPENGL
}

void QWidgetPrivate::render(QPaintDevice *target, const QPoint &targetOffset,
                            const QRegion &sourceRegion, QWidget::RenderFlags renderFlags)
{
    if (Q_UNLIKELY(!target)) {
        qWarning("QWidget::render: null pointer to paint device");
        return;
    }

    const bool inRenderWithPainter = extra && extra->inRenderWithPainter;
    QRegion paintRegion = !inRenderWithPainter
                          ? prepareToRender(sourceRegion, renderFlags)
                          : sourceRegion;
    if (paintRegion.isEmpty())
        return;

    QPainter *oldSharedPainter = inRenderWithPainter ? sharedPainter() : nullptr;

    // Use the target's shared painter if set (typically set when doing
    // "other->render(widget);" in the widget's paintEvent.
    if (target->devType() == QInternal::Widget) {
        QWidgetPrivate *targetPrivate = static_cast<QWidget *>(target)->d_func();
        if (targetPrivate->extra && targetPrivate->extra->inRenderWithPainter) {
            QPainter *targetPainter = targetPrivate->sharedPainter();
            if (targetPainter && targetPainter->isActive())
                setSharedPainter(targetPainter);
        }
    }

    // Use the target's redirected device if set and adjust offset and paint
    // region accordingly. This is typically the case when people call render
    // from the paintEvent.
    QPoint offset = targetOffset;
    offset -= paintRegion.boundingRect().topLeft();
    QPoint redirectionOffset;
    QPaintDevice *redirected = nullptr;

    if (target->devType() == QInternal::Widget)
        redirected = static_cast<QWidget *>(target)->d_func()->redirected(&redirectionOffset);

    if (redirected) {
        target = redirected;
        offset -= redirectionOffset;
    }

    if (!inRenderWithPainter) { // Clip handled by shared painter (in qpainter.cpp).
        if (QPaintEngine *targetEngine = target->paintEngine()) {
            const QRegion targetSystemClip = targetEngine->systemClip();
            if (!targetSystemClip.isEmpty())
                paintRegion &= targetSystemClip.translated(-offset);
        }
    }

    // Set backingstore flags.
    DrawWidgetFlags flags = DrawPaintOnScreen | DrawInvisible;
    if (renderFlags & QWidget::DrawWindowBackground)
        flags |= DrawAsRoot;

    if (renderFlags & QWidget::DrawChildren)
        flags |= DrawRecursive;
    else
        flags |= DontSubtractOpaqueChildren;

    flags |= DontSetCompositionMode;

    // Render via backingstore.
    drawWidget(target, paintRegion, offset, flags, sharedPainter());

    // Restore shared painter.
    if (oldSharedPainter)
        setSharedPainter(oldSharedPainter);
}

void QWidgetPrivate::paintSiblingsRecursive(QPaintDevice *pdev, const QObjectList& siblings, int index, const QRegion &rgn,
                                            const QPoint &offset, DrawWidgetFlags flags
                                            , QPainter *sharedPainter, QWidgetRepaintManager *repaintManager)
{
    QWidget *w = nullptr;
    QRect boundingRect;
    bool dirtyBoundingRect = true;
    const bool exludeOpaqueChildren = (flags & DontDrawOpaqueChildren);
    const bool excludeNativeChildren = (flags & DontDrawNativeChildren);

    do {
        QWidget *x =  qobject_cast<QWidget*>(siblings.at(index));
        if (x && !(exludeOpaqueChildren && x->d_func()->isOpaque) && !x->isHidden() && !x->isWindow()
            && !(excludeNativeChildren && x->internalWinId())) {
            if (dirtyBoundingRect) {
                boundingRect = rgn.boundingRect();
                dirtyBoundingRect = false;
            }

            if (qRectIntersects(boundingRect, x->d_func()->effectiveRectFor(x->data->crect))) {
                w = x;
                break;
            }
        }
        --index;
    } while (index >= 0);

    if (!w)
        return;

    QWidgetPrivate *wd = w->d_func();
    const QPoint widgetPos(w->data->crect.topLeft());
    const bool hasMask = wd->extra && wd->extra->hasMask && !wd->graphicsEffect;
    if (index > 0) {
        QRegion wr(rgn);
        if (wd->isOpaque)
            wr -= hasMask ? wd->extra->mask.translated(widgetPos) : w->data->crect;
        paintSiblingsRecursive(pdev, siblings, --index, wr, offset, flags,
                               sharedPainter, repaintManager);
    }

    if (w->updatesEnabled()
#if QT_CONFIG(graphicsview)
            && (!w->d_func()->extra || !w->d_func()->extra->proxyWidget)
#endif // QT_CONFIG(graphicsview)
       ) {
        QRegion wRegion(rgn);
        wRegion &= wd->effectiveRectFor(w->data->crect);
        wRegion.translate(-widgetPos);
        if (hasMask)
            wRegion &= wd->extra->mask;
        wd->drawWidget(pdev, wRegion, offset + widgetPos, flags, sharedPainter, repaintManager);
    }
}

#if QT_CONFIG(graphicseffect)
QRectF QWidgetEffectSourcePrivate::boundingRect(Qt::CoordinateSystem system) const
{
    if (system != Qt::DeviceCoordinates)
        return m_widget->rect();

    if (Q_UNLIKELY(!context)) {
        // Device coordinates without context not yet supported.
        qWarning("QGraphicsEffectSource::boundingRect: Not yet implemented, lacking device context");
        return QRectF();
    }

    return context->painter->worldTransform().mapRect(m_widget->rect());
}

void QWidgetEffectSourcePrivate::draw(QPainter *painter)
{
    if (!context || context->painter != painter) {
        m_widget->render(painter);
        return;
    }

    // The region saved in the context is neither clipped to the rect
    // nor the mask, so we have to clip it here before calling drawWidget.
    QRegion toBePainted = context->rgn;
    toBePainted &= m_widget->rect();
    QWidgetPrivate *wd = qt_widget_private(m_widget);
    if (wd->extra && wd->extra->hasMask)
        toBePainted &= wd->extra->mask;

    wd->drawWidget(context->pdev, toBePainted, context->offset, context->flags,
                   context->sharedPainter, context->repaintManager);
}

QPixmap QWidgetEffectSourcePrivate::pixmap(Qt::CoordinateSystem system, QPoint *offset,
                                           QGraphicsEffect::PixmapPadMode mode) const
{
    const bool deviceCoordinates = (system == Qt::DeviceCoordinates);
    if (Q_UNLIKELY(!context && deviceCoordinates)) {
        // Device coordinates without context not yet supported.
        qWarning("QGraphicsEffectSource::pixmap: Not yet implemented, lacking device context");
        return QPixmap();
    }

    QPoint pixmapOffset;
    QRectF sourceRect = m_widget->rect();

    if (deviceCoordinates) {
        const QTransform &painterTransform = context->painter->worldTransform();
        sourceRect = painterTransform.mapRect(sourceRect);
        pixmapOffset = painterTransform.map(pixmapOffset);
    }

    QRect effectRect;

    if (mode == QGraphicsEffect::PadToEffectiveBoundingRect)
        effectRect = m_widget->graphicsEffect()->boundingRectFor(sourceRect).toAlignedRect();
    else if (mode == QGraphicsEffect::PadToTransparentBorder)
        effectRect = sourceRect.adjusted(-1, -1, 1, 1).toAlignedRect();
    else
        effectRect = sourceRect.toAlignedRect();

    if (offset)
        *offset = effectRect.topLeft();

    pixmapOffset -= effectRect.topLeft();

    qreal dpr(1.0);
    if (const auto *paintDevice = context->painter->device())
        dpr = paintDevice->devicePixelRatioF();
    else
        qWarning("QWidgetEffectSourcePrivate::pixmap: Painter not active");
    QPixmap pixmap(effectRect.size() * dpr);
    pixmap.setDevicePixelRatio(dpr);

    pixmap.fill(Qt::transparent);
    m_widget->render(&pixmap, pixmapOffset, QRegion(), QWidget::DrawChildren);
    return pixmap;
}
#endif // QT_CONFIG(graphicseffect)

#if QT_CONFIG(graphicsview)
/*!
    \internal

    Finds the nearest widget embedded in a graphics proxy widget along the chain formed by this
    widget and its ancestors. The search starts at \a origin (inclusive).
    If successful, the function returns the proxy that embeds the widget, or \nullptr if no
    embedded widget was found.
*/
QGraphicsProxyWidget *QWidgetPrivate::nearestGraphicsProxyWidget(const QWidget *origin)
{
    if (origin) {
        const auto &extra = origin->d_func()->extra;
        if (extra && extra->proxyWidget)
            return extra->proxyWidget;
        return nearestGraphicsProxyWidget(origin->parentWidget());
    }
    return nullptr;
}
#endif

/*!
    \property QWidget::locale
    \brief the widget's locale
    \since 4.3

    As long as no special locale has been set, this is either
    the parent's locale or (if this widget is a top level widget),
    the default locale.

    If the widget displays dates or numbers, these should be formatted
    using the widget's locale.

    \sa QLocale, QLocale::setDefault()
*/

void QWidgetPrivate::setLocale_helper(const QLocale &loc, bool forceUpdate)
{
    Q_Q(QWidget);
    if (locale == loc && !forceUpdate)
        return;

    locale = loc;

    if (!children.isEmpty()) {
        for (int i = 0; i < children.size(); ++i) {
            QWidget *w = qobject_cast<QWidget*>(children.at(i));
            if (!w)
                continue;
            if (w->testAttribute(Qt::WA_SetLocale))
                continue;
            if (w->isWindow() && !w->testAttribute(Qt::WA_WindowPropagation))
                continue;
            w->d_func()->setLocale_helper(loc, forceUpdate);
        }
    }
    QEvent e(QEvent::LocaleChange);
    QCoreApplication::sendEvent(q, &e);
}

void QWidget::setLocale(const QLocale &locale)
{
    Q_D(QWidget);

    setAttribute(Qt::WA_SetLocale);
    d->setLocale_helper(locale);
}

QLocale QWidget::locale() const
{
    Q_D(const QWidget);

    return d->locale;
}

void QWidgetPrivate::resolveLocale()
{
    Q_Q(const QWidget);

    if (!q->testAttribute(Qt::WA_SetLocale)) {
        QWidget *parent = q->parentWidget();
        setLocale_helper(!parent || (q->isWindow() && !q->testAttribute(Qt::WA_WindowPropagation))
                         ? QLocale() : parent->locale());
    }
}

void QWidget::unsetLocale()
{
    Q_D(QWidget);
    setAttribute(Qt::WA_SetLocale, false);
    d->resolveLocale();
}

/*!
    \property QWidget::windowTitle
    \brief the window title (caption)

    This property only makes sense for top-level widgets, such as
    windows and dialogs. If no caption has been set, the title is based of the
    \l windowFilePath. If neither of these is set, then the title is
    an empty string.

    If you use the \l windowModified mechanism, the window title must
    contain a "[*]" placeholder, which indicates where the '*' should
    appear. Normally, it should appear right after the file name
    (e.g., "document1.txt[*] - Text Editor"). If the \l
    windowModified property is \c false (the default), the placeholder
    is simply removed.

    On some desktop platforms (including Windows and Unix), the application name
    (from QGuiApplication::applicationDisplayName) is added at the end of the
    window title, if set. This is done by the QPA plugin, so it is shown to the
    user, but isn't part of the windowTitle string.

    \sa windowIcon, windowModified, windowFilePath
*/
QString QWidget::windowTitle() const
{
    Q_D(const QWidget);
    if (d->extra && d->extra->topextra) {
        if (!d->extra->topextra->caption.isEmpty())
            return d->extra->topextra->caption;
        if (!d->extra->topextra->filePath.isEmpty())
            return QFileInfo(d->extra->topextra->filePath).fileName() + QLatin1String("[*]");
    }
    return QString();
}

/*!
    Returns a modified window title with the [*] place holder
    replaced according to the rules described in QWidget::setWindowTitle

    This function assumes that "[*]" can be quoted by another
    "[*]", so it will replace two place holders by one and
    a single last one by either "*" or nothing depending on
    the modified flag.

    \internal
*/
QString qt_setWindowTitle_helperHelper(const QString &title, const QWidget *widget)
{
    Q_ASSERT(widget);

    QString cap = title;
    if (cap.isEmpty())
        return cap;

    QLatin1String placeHolder("[*]");
    int index = cap.indexOf(placeHolder);

    // here the magic begins
    while (index != -1) {
        index += placeHolder.size();
        int count = 1;
        while (cap.indexOf(placeHolder, index) == index) {
            ++count;
            index += placeHolder.size();
        }

        if (count%2) { // odd number of [*] -> replace last one
            int lastIndex = cap.lastIndexOf(placeHolder, index - 1);
            if (widget->isWindowModified()
             && widget->style()->styleHint(QStyle::SH_TitleBar_ModifyNotification, nullptr, widget))
                cap.replace(lastIndex, 3, QWidget::tr("*"));
            else
                cap.remove(lastIndex, 3);
        }

        index = cap.indexOf(placeHolder, index);
    }

    cap.replace(QLatin1String("[*][*]"), placeHolder);

    return cap;
}

void QWidgetPrivate::setWindowTitle_helper(const QString &title)
{
    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_WState_Created))
        setWindowTitle_sys(qt_setWindowTitle_helperHelper(title, q));
}

void QWidgetPrivate::setWindowTitle_sys(const QString &caption)
{
    Q_Q(QWidget);
    if (!q->isWindow())
        return;

    if (QWindow *window = q->windowHandle())
        window->setTitle(caption);

}

void QWidgetPrivate::setWindowIconText_helper(const QString &title)
{
    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_WState_Created))
        setWindowIconText_sys(qt_setWindowTitle_helperHelper(title, q));
}

void QWidgetPrivate::setWindowIconText_sys(const QString &iconText)
{
    Q_Q(QWidget);
    // ### The QWidget property is deprecated, but the XCB window function is not.
    // It should remain available for the rare application that needs it.
    if (QWindow *window = q->windowHandle())
        QXcbWindowFunctions::setWmWindowIconText(window, iconText);
}

/*!
    \fn void QWidget::windowIconTextChanged(const QString &iconText)

    This signal is emitted when the window's icon text has changed, with the
    new \a iconText as an argument.

    \since 5.2
    \obsolete

    This signal is deprecated.
*/

void QWidget::setWindowIconText(const QString &iconText)
{
    if (QWidget::windowIconText() == iconText)
        return;

    Q_D(QWidget);
    d->topData()->iconText = iconText;
    d->setWindowIconText_helper(iconText);

    QEvent e(QEvent::IconTextChange);
    QCoreApplication::sendEvent(this, &e);

    emit windowIconTextChanged(iconText);
}

/*!
    \fn void QWidget::windowTitleChanged(const QString &title)

    This signal is emitted when the window's title has changed, with the
    new \a title as an argument.

    \since 5.2
*/

void QWidget::setWindowTitle(const QString &title)
{
    if (QWidget::windowTitle() == title && !title.isEmpty() && !title.isNull())
        return;

    Q_D(QWidget);
    d->topData()->caption = title;
    d->setWindowTitle_helper(title);

    QEvent e(QEvent::WindowTitleChange);
    QCoreApplication::sendEvent(this, &e);

    emit windowTitleChanged(title);
}


/*!
    \property QWidget::windowIcon
    \brief the widget's icon

    This property only makes sense for windows. If no icon
    has been set, windowIcon() returns the application icon
    (QApplication::windowIcon()).

    \note On \macos, window icons represent the active document,
    and will not be displayed unless a file path has also been
    set using setWindowFilePath.

    \sa windowTitle, setWindowFilePath
*/
QIcon QWidget::windowIcon() const
{
    const QWidget *w = this;
    while (w) {
        const QWidgetPrivate *d = w->d_func();
        if (d->extra && d->extra->topextra && d->extra->topextra->icon)
            return *d->extra->topextra->icon;
        w = w->parentWidget();
    }
    return QApplication::windowIcon();
}

void QWidgetPrivate::setWindowIcon_helper()
{
    Q_Q(QWidget);
    QEvent e(QEvent::WindowIconChange);

    // Do not send the event if the widget is a top level.
    // In that case, setWindowIcon_sys does it, and event propagation from
    // QWidgetWindow to the top level QWidget ensures that the event reaches
    // the top level anyhow
    if (!q->windowHandle())
        QCoreApplication::sendEvent(q, &e);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && !w->isWindow())
            QCoreApplication::sendEvent(w, &e);
    }
}

/*!
    \fn void QWidget::windowIconChanged(const QIcon &icon)

    This signal is emitted when the window's icon has changed, with the
    new \a icon as an argument.

    \since 5.2
*/

void QWidget::setWindowIcon(const QIcon &icon)
{
    Q_D(QWidget);

    setAttribute(Qt::WA_SetWindowIcon, !icon.isNull());
    d->createTLExtra();

    if (!d->extra->topextra->icon)
        d->extra->topextra->icon = qt_make_unique<QIcon>(icon);
    else
        *d->extra->topextra->icon = icon;

    d->setWindowIcon_sys();
    d->setWindowIcon_helper();

    emit windowIconChanged(icon);
}

void QWidgetPrivate::setWindowIcon_sys()
{
    Q_Q(QWidget);
    if (QWindow *window = q->windowHandle())
        window->setIcon(q->windowIcon());
}

/*!
    \property QWidget::windowIconText
    \brief the text to be displayed on the icon of a minimized window

    This property only makes sense for windows. If no icon
    text has been set, this accessor returns an empty string.
    It is only implemented on the X11 platform, and only certain
    window managers use this window property.

    \obsolete
    This property is deprecated.

    \sa windowIcon, windowTitle
*/

QString QWidget::windowIconText() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra) ? d->extra->topextra->iconText : QString();
}

/*!
    \property QWidget::windowFilePath
    \since 4.4
    \brief the file path associated with a widget

    This property only makes sense for windows. It associates a file path with
    a window. If you set the file path, but have not set the window title, Qt
    sets the window title to the file name of the specified path, obtained using
    QFileInfo::fileName().

    If the window title is set at any point, then the window title takes precedence and
    will be shown instead of the file path string.

    Additionally, on \macos, this has an added benefit that it sets the
    \l{http://developer.apple.com/documentation/UserExperience/Conceptual/OSXHIGuidelines/XHIGWindows/chapter_17_section_3.html}{proxy icon}
    for the window, assuming that the file path exists.

    If no file path is set, this property contains an empty string.

    By default, this property contains an empty string.

    \sa windowTitle, windowIcon
*/

QString QWidget::windowFilePath() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra) ? d->extra->topextra->filePath : QString();
}

void QWidget::setWindowFilePath(const QString &filePath)
{
    if (filePath == windowFilePath())
        return;

    Q_D(QWidget);

    d->createTLExtra();
    d->extra->topextra->filePath = filePath;
    d->setWindowFilePath_helper(filePath);
}

void QWidgetPrivate::setWindowFilePath_helper(const QString &filePath)
{
    if (extra->topextra && extra->topextra->caption.isEmpty()) {
#ifdef Q_OS_MAC
        setWindowTitle_helper(QFileInfo(filePath).fileName());
#else
        Q_Q(QWidget);
        Q_UNUSED(filePath);
        setWindowTitle_helper(q->windowTitle());
#endif
    }
#ifdef Q_OS_MAC
    setWindowFilePath_sys(filePath);
#endif
}

void QWidgetPrivate::setWindowFilePath_sys(const QString &filePath)
{
    Q_Q(QWidget);
    if (!q->isWindow())
        return;

    if (QWindow *window = q->windowHandle())
        window->setFilePath(filePath);
}

/*!
    Returns the window's role, or an empty string.

    \sa windowIcon, windowTitle
*/

QString QWidget::windowRole() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra) ? d->extra->topextra->role : QString();
}

/*!
    Sets the window's role to \a role. This only makes sense for
    windows on X11.
*/
void QWidget::setWindowRole(const QString &role)
{
    Q_D(QWidget);
    d->createTLExtra();
    d->topData()->role = role;
    if (windowHandle())
        QXcbWindowFunctions::setWmWindowRole(windowHandle(), role.toLatin1());
}

/*!
    \property QWidget::mouseTracking
    \brief whether mouse tracking is enabled for the widget

    If mouse tracking is disabled (the default), the widget only
    receives mouse move events when at least one mouse button is
    pressed while the mouse is being moved.

    If mouse tracking is enabled, the widget receives mouse move
    events even if no buttons are pressed.

    \sa mouseMoveEvent()
*/

/*!
    \property QWidget::tabletTracking
    \brief whether tablet tracking is enabled for the widget
    \since 5.9

    If tablet tracking is disabled (the default), the widget only
    receives tablet move events when the stylus is in contact with
    the tablet, or at least one stylus button is pressed,
    while the stylus is being moved.

    If tablet tracking is enabled, the widget receives tablet move
    events even while hovering in proximity.  This is useful for
    monitoring position as well as the auxiliary properties such
    as rotation and tilt, and providing feedback in the UI.

    \sa tabletEvent()
*/


/*!
    Sets the widget's focus proxy to widget \a w. If \a w is \nullptr, the
    function resets this widget to have no focus proxy.

    Some widgets can "have focus", but create a child widget, such as
    QLineEdit, to actually handle the focus. In this case, the widget
    can set the line edit to be its focus proxy.

    setFocusProxy() sets the widget which will actually get focus when
    "this widget" gets it. If there is a focus proxy, setFocus() and
    hasFocus() operate on the focus proxy. If "this widget" is the focus
    widget, then setFocusProxy() moves focus to the new focus proxy.

    \sa focusProxy()
*/

void QWidget::setFocusProxy(QWidget * w)
{
    Q_D(QWidget);
    if (!w && !d->extra)
        return;

    for (QWidget* fp  = w; fp; fp = fp->focusProxy()) {
        if (Q_UNLIKELY(fp == this)) {
            qWarning("QWidget: %s (%s) already in focus proxy chain", metaObject()->className(), objectName().toLocal8Bit().constData());
            return;
        }
    }

    const bool moveFocusToProxy = (QApplicationPrivate::focus_widget == this);

    d->createExtra();
    d->extra->focus_proxy = w;

    if (moveFocusToProxy)
        setFocus(Qt::OtherFocusReason);
}


/*!
    Returns the focus proxy, or \nullptr if there is no focus proxy.

    \sa setFocusProxy()
*/

QWidget *QWidget::focusProxy() const
{
    Q_D(const QWidget);
    return d->extra ? d->extra->focus_proxy.data() : nullptr;
}


/*!
    \property QWidget::focus
    \brief whether this widget (or its focus proxy) has the keyboard
    input focus

    By default, this property is \c false.

    \note Obtaining the value of this property for a widget is effectively equivalent
    to checking whether QApplication::focusWidget() refers to the widget.

    \sa setFocus(), clearFocus(), setFocusPolicy(), QApplication::focusWidget()
*/
bool QWidget::hasFocus() const
{
    const QWidget* w = this;
    while (w->d_func()->extra && w->d_func()->extra->focus_proxy)
        w = w->d_func()->extra->focus_proxy;
#if QT_CONFIG(graphicsview)
    if (QWidget *window = w->window()) {
        const auto &e = window->d_func()->extra;
        if (e && e->proxyWidget && e->proxyWidget->hasFocus() && window->focusWidget() == w)
            return true;
    }
#endif // QT_CONFIG(graphicsview)
    return (QApplication::focusWidget() == w);
}

/*!
    Gives the keyboard input focus to this widget (or its focus
    proxy) if this widget or one of its parents is the \l{isActiveWindow()}{active window}. The \a reason argument will
    be passed into any focus event sent from this function, it is used
    to give an explanation of what caused the widget to get focus.
    If the window is not active, the widget will be given the focus when
    the window becomes active.

    First, a focus about to change event is sent to the focus widget (if any) to
    tell it that it is about to lose the focus. Then focus is changed, a
    focus out event is sent to the previous focus item and a focus in event is sent
    to the new item to tell it that it just received the focus.
    (Nothing happens if the focus in and focus out widgets are the
    same.)

    \note On embedded platforms, setFocus() will not cause an input panel
    to be opened by the input method. If you want this to happen, you
    have to send a QEvent::RequestSoftwareInputPanel event to the
    widget yourself.

    setFocus() gives focus to a widget regardless of its focus policy,
    but does not clear any keyboard grab (see grabKeyboard()).

    Be aware that if the widget is hidden, it will not accept focus
    until it is shown.

    \warning If you call setFocus() in a function which may itself be
    called from focusOutEvent() or focusInEvent(), you may get an
    infinite recursion.

    \sa hasFocus(), clearFocus(), focusInEvent(), focusOutEvent(),
    setFocusPolicy(), focusWidget(), QApplication::focusWidget(), grabKeyboard(),
    grabMouse(), {Keyboard Focus in Widgets}, QEvent::RequestSoftwareInputPanel
*/

void QWidget::setFocus(Qt::FocusReason reason)
{
    if (!isEnabled())
        return;

    QWidget *f = d_func()->deepestFocusProxy();
    if (!f)
        f = this;

    if (QApplication::focusWidget() == f)
        return;

#if QT_CONFIG(graphicsview)
    QWidget *previousProxyFocus = nullptr;
    if (const auto &topData = window()->d_func()->extra) {
        if (topData->proxyWidget && topData->proxyWidget->hasFocus()) {
            previousProxyFocus = topData->proxyWidget->widget()->focusWidget();
            if (previousProxyFocus && previousProxyFocus->focusProxy())
                previousProxyFocus = previousProxyFocus->focusProxy();
            if (previousProxyFocus == f && !topData->proxyWidget->d_func()->proxyIsGivingFocus)
                return;
        }
    }
#endif

#if QT_CONFIG(graphicsview)
    // Update proxy state
    if (const auto &topData = window()->d_func()->extra) {
        if (topData->proxyWidget && !topData->proxyWidget->hasFocus()) {
            f->d_func()->updateFocusChild();
            topData->proxyWidget->d_func()->focusFromWidgetToProxy = 1;
            topData->proxyWidget->setFocus(reason);
            topData->proxyWidget->d_func()->focusFromWidgetToProxy = 0;
        }
    }
#endif

    if (f->isActiveWindow()) {
        QWidget *prev = QApplicationPrivate::focus_widget;
        if (prev) {
            if (reason != Qt::PopupFocusReason && reason != Qt::MenuBarFocusReason
                && prev->testAttribute(Qt::WA_InputMethodEnabled)) {
                QGuiApplication::inputMethod()->commit();
            }

            if (reason != Qt::NoFocusReason) {
                QFocusEvent focusAboutToChange(QEvent::FocusAboutToChange, reason);
                QCoreApplication::sendEvent(prev, &focusAboutToChange);
            }
        }

        f->d_func()->updateFocusChild();

        QApplicationPrivate::setFocusWidget(f, reason);
#ifndef QT_NO_ACCESSIBILITY
        // menus update the focus manually and this would create bogus events
        if (!(f->inherits("QMenuBar") || f->inherits("QMenu") || f->inherits("QMenuItem")))
        {
            QAccessibleEvent event(f, QAccessible::Focus);
            QAccessible::updateAccessibility(&event);
        }
#endif
#if QT_CONFIG(graphicsview)
        if (const auto &topData = window()->d_func()->extra) {
            if (topData->proxyWidget) {
                if (previousProxyFocus && previousProxyFocus != f) {
                    // Send event to self
                    QFocusEvent event(QEvent::FocusOut, reason);
                    QPointer<QWidget> that = previousProxyFocus;
                    QCoreApplication::sendEvent(previousProxyFocus, &event);
                    if (that)
                        QCoreApplication::sendEvent(that->style(), &event);
                }
                if (!isHidden()) {
#if QT_CONFIG(graphicsview)
                    // Update proxy state
                    if (const auto &topData = window()->d_func()->extra)
                        if (topData->proxyWidget && topData->proxyWidget->hasFocus())
                            topData->proxyWidget->d_func()->updateProxyInputMethodAcceptanceFromWidget();
#endif
                    // Send event to self
                    QFocusEvent event(QEvent::FocusIn, reason);
                    QPointer<QWidget> that = f;
                    QCoreApplication::sendEvent(f, &event);
                    if (that)
                        QCoreApplication::sendEvent(that->style(), &event);
                }
            }
        }
#endif
    } else {
        f->d_func()->updateFocusChild();
    }
}


/*!\internal
 * A focus proxy can have its own focus proxy, which can have its own
 * proxy, and so on. This helper function returns the widget that sits
 * at the bottom of the proxy chain, and therefore the one that should
 * normally get focus if this widget receives a focus request.
 */
QWidget *QWidgetPrivate::deepestFocusProxy() const
{
    Q_Q(const QWidget);

    QWidget *focusProxy = q->focusProxy();
    if (!focusProxy)
        return nullptr;

    while (QWidget *nextFocusProxy = focusProxy->focusProxy())
        focusProxy = nextFocusProxy;

    return focusProxy;
}

static inline bool isEmbedded(const QWindow *w)
{
     const auto platformWindow = w->handle();
     return platformWindow && platformWindow->isEmbedded();
}

void QWidgetPrivate::setFocus_sys()
{
    Q_Q(QWidget);
    // Embedded native widget may have taken the focus; get it back to toplevel
    // if that is the case (QTBUG-25852)
    // Do not activate in case the popup menu opens another application (QTBUG-70810)
    // unless the application is embedded (QTBUG-71991).
    if (QWindow *nativeWindow = q->testAttribute(Qt::WA_WState_Created) ? q->window()->windowHandle() : nullptr) {
        if (nativeWindow->type() != Qt::Popup && nativeWindow != QGuiApplication::focusWindow()
            && (QGuiApplication::applicationState() == Qt::ApplicationActive
                || QCoreApplication::testAttribute(Qt::AA_PluginApplication)
                || isEmbedded(nativeWindow))) {
            nativeWindow->requestActivate();
        }
    }
}

// updates focus_child on parent widgets to point into this widget
void QWidgetPrivate::updateFocusChild()
{
    Q_Q(QWidget);

    QWidget *w = q;
    if (q->isHidden()) {
        while (w && w->isHidden()) {
            w->d_func()->focus_child = q;
            w = w->isWindow() ? nullptr : w->parentWidget();
        }
    } else {
        while (w) {
            w->d_func()->focus_child = q;
            w = w->isWindow() ? nullptr : w->parentWidget();
        }
    }

    if (QTLWExtra *extra = q->window()->d_func()->maybeTopData()) {
        if (extra->window)
            emit extra->window->focusObjectChanged(q);
    }
}

/*!
    \fn void QWidget::setFocus()
    \overload

    Gives the keyboard input focus to this widget (or its focus
    proxy) if this widget or one of its parents is the
    \l{isActiveWindow()}{active window}.
*/

/*!
    Takes keyboard input focus from the widget.

    If the widget has active focus, a \l{focusOutEvent()}{focus out event} is sent to this widget to tell it that it has
    lost the focus.

    This widget must enable focus setting in order to get the keyboard
    input focus, i.e. it must call setFocusPolicy().

    \sa hasFocus(), setFocus(), focusInEvent(), focusOutEvent(),
    setFocusPolicy(), QApplication::focusWidget()
*/

void QWidget::clearFocus()
{
    if (hasFocus()) {
        if (testAttribute(Qt::WA_InputMethodEnabled))
            QGuiApplication::inputMethod()->commit();

        QFocusEvent focusAboutToChange(QEvent::FocusAboutToChange);
        QCoreApplication::sendEvent(this, &focusAboutToChange);
    }

    QWidget *w = this;
    while (w) {
        // Just like setFocus(), we update (clear) the focus_child of our parents
        if (w->d_func()->focus_child == this)
            w->d_func()->focus_child = nullptr;
        w = w->parentWidget();
    }

    // Since we've unconditionally cleared the focus_child of our parents, we need
    // to report this to the rest of Qt. Note that the focus_child is not the same
    // thing as the application's focusWidget, which is why this piece of code is
    // not inside the hasFocus() block below.
    if (QTLWExtra *extra = window()->d_func()->maybeTopData()) {
        if (extra->window)
            emit extra->window->focusObjectChanged(extra->window->focusObject());
    }

#if QT_CONFIG(graphicsview)
    const auto &topData = d_func()->extra;
    if (topData && topData->proxyWidget)
        topData->proxyWidget->clearFocus();
#endif

    if (hasFocus()) {
        // Update proxy state
        QApplicationPrivate::setFocusWidget(nullptr, Qt::OtherFocusReason);
#ifndef QT_NO_ACCESSIBILITY
        QAccessibleEvent event(this, QAccessible::Focus);
        QAccessible::updateAccessibility(&event);
#endif
    }
}


/*!
    \fn bool QWidget::focusNextChild()

    Finds a new widget to give the keyboard focus to, as appropriate
    for \uicontrol Tab, and returns \c true if it can find a new widget, or
    false if it can't.

    \sa focusPreviousChild()
*/

/*!
    \fn bool QWidget::focusPreviousChild()

    Finds a new widget to give the keyboard focus to, as appropriate
    for \uicontrol Shift+Tab, and returns \c true if it can find a new widget,
    or false if it can't.

    \sa focusNextChild()
*/

/*!
    Finds a new widget to give the keyboard focus to, as appropriate
    for Tab and Shift+Tab, and returns \c true if it can find a new
    widget, or false if it can't.

    If \a next is true, this function searches forward, if \a next
    is false, it searches backward.

    Sometimes, you will want to reimplement this function. For
    example, a web browser might reimplement it to move its "current
    active link" forward or backward, and call
    focusNextPrevChild() only when it reaches the last or
    first link on the "page".

    Child widgets call focusNextPrevChild() on their parent widgets,
    but only the window that contains the child widgets decides where
    to redirect focus. By reimplementing this function for an object,
    you thus gain control of focus traversal for all child widgets.

    \sa focusNextChild(), focusPreviousChild()
*/

bool QWidget::focusNextPrevChild(bool next)
{
    QWidget* p = parentWidget();
    bool isSubWindow = (windowType() == Qt::SubWindow);
    if (!isWindow() && !isSubWindow && p)
        return p->focusNextPrevChild(next);
#if QT_CONFIG(graphicsview)
    Q_D(QWidget);
    if (d->extra && d->extra->proxyWidget)
        return d->extra->proxyWidget->focusNextPrevChild(next);
#endif

    bool wrappingOccurred = false;
    QWidget *w = QApplicationPrivate::focusNextPrevChild_helper(this, next,
                                                                &wrappingOccurred);
    if (!w) return false;

    Qt::FocusReason reason = next ? Qt::TabFocusReason : Qt::BacktabFocusReason;

    /* If we are about to wrap the focus chain, give the platform
     * implementation a chance to alter the wrapping behavior.  This is
     * especially needed when the window is embedded in a window created by
     * another process.
     */
    if (wrappingOccurred) {
        QWindow *window = windowHandle();
        if (window != nullptr) {
            QWindowPrivate *winp = qt_window_private(window);

            if (winp->platformWindow != nullptr) {
                QFocusEvent event(QEvent::FocusIn, reason);
                event.ignore();
                winp->platformWindow->windowEvent(&event);
                if (event.isAccepted()) return true;
            }
        }
    }

    w->setFocus(reason);
    return true;
}

/*!
    Returns the last child of this widget that setFocus had been
    called on.  For top level widgets this is the widget that will get
    focus in case this window gets activated

    This is not the same as QApplication::focusWidget(), which returns
    the focus widget in the currently active window.
*/

QWidget *QWidget::focusWidget() const
{
    return const_cast<QWidget *>(d_func()->focus_child);
}

/*!
    Returns the next widget in this widget's focus chain.

    \sa previousInFocusChain()
*/
QWidget *QWidget::nextInFocusChain() const
{
    return const_cast<QWidget *>(d_func()->focus_next);
}

/*!
    \brief The previousInFocusChain function returns the previous
    widget in this widget's focus chain.

    \sa nextInFocusChain()

    \since 4.6
*/
QWidget *QWidget::previousInFocusChain() const
{
    return const_cast<QWidget *>(d_func()->focus_prev);
}

/*!
    \property QWidget::isActiveWindow
    \brief whether this widget's window is the active window

    The active window is the window that contains the widget that has
    keyboard focus (The window may still have focus if it has no
    widgets or none of its widgets accepts keyboard focus).

    When popup windows are visible, this property is \c true for both the
    active window \e and for the popup.

    By default, this property is \c false.

    \sa activateWindow(), QApplication::activeWindow()
*/
bool QWidget::isActiveWindow() const
{
    QWidget *tlw = window();
    if(tlw == QApplication::activeWindow() || (isVisible() && (tlw->windowType() == Qt::Popup)))
        return true;

#if QT_CONFIG(graphicsview)
    if (const auto &tlwExtra = tlw->d_func()->extra) {
        if (isVisible() && tlwExtra->proxyWidget)
            return tlwExtra->proxyWidget->isActiveWindow();
    }
#endif

    if (style()->styleHint(QStyle::SH_Widget_ShareActivation, nullptr, this)) {
        if(tlw->windowType() == Qt::Tool &&
           !tlw->isModal() &&
           (!tlw->parentWidget() || tlw->parentWidget()->isActiveWindow()))
           return true;
        QWidget *w = QApplication::activeWindow();
        while(w && tlw->windowType() == Qt::Tool &&
              !w->isModal() && w->parentWidget()) {
            w = w->parentWidget()->window();
            if(w == tlw)
                return true;
        }
    }

    // Check for an active window container
    if (QWindow *ww = QGuiApplication::focusWindow()) {
        while (ww) {
            QWidgetWindow *qww = qobject_cast<QWidgetWindow *>(ww);
            QWindowContainer *qwc = qww ? qobject_cast<QWindowContainer *>(qww->widget()) : 0;
            if (qwc && qwc->topLevelWidget() == tlw)
                return true;
            ww = ww->parent();
        }
    }

    // Check if platform adaptation thinks the window is active. This is necessary for
    // example in case of ActiveQt servers that are embedded into another application.
    // Those are separate processes that are not part of the parent application Qt window/widget
    // hierarchy, so they need to rely on native methods to determine if they are part of the
    // active window.
    if (const QWindow *w = tlw->windowHandle()) {
        if (w->handle())
            return w->handle()->isActive();
    }

    return false;
}

/*!
    Puts the \a second widget after the \a first widget in the focus order.

    It effectively removes the \a second widget from its focus chain and
    inserts it after the \a first widget.

    Note that since the tab order of the \a second widget is changed, you
    should order a chain like this:

    \snippet code/src_gui_kernel_qwidget.cpp 9

    \e not like this:

    \snippet code/src_gui_kernel_qwidget.cpp 10

    If \a first or \a second has a focus proxy, setTabOrder()
    correctly substitutes the proxy.

    \note Since Qt 5.10: A widget that has a child as focus proxy is understood as
    a compound widget. When setting a tab order between one or two compound widgets, the
    local tab order inside each will be preserved. This means that if both widgets are
    compound widgets, the resulting tab order will be from the last child inside
    \a first, to the first child inside \a second.

    \sa setFocusPolicy(), setFocusProxy(), {Keyboard Focus in Widgets}
*/
void QWidget::setTabOrder(QWidget* first, QWidget *second)
{
    if (!first || !second || first == second
            || first->focusPolicy() == Qt::NoFocus
            || second->focusPolicy() == Qt::NoFocus)
        return;

    if (Q_UNLIKELY(first->window() != second->window())) {
        qWarning("QWidget::setTabOrder: 'first' and 'second' must be in the same window");
        return;
    }

    auto determineLastFocusChild = [](QWidget *target, QWidget *&lastFocusChild)
    {
        // Since we need to repeat the same logic for both 'first' and 'second', we add a function that
        // determines the last focus child for a widget, taking proxies and compound widgets into account.
        // If the target is not a compound widget (it doesn't have a focus proxy that points to a child),
        // 'lastFocusChild' will be set to the target itself.
        lastFocusChild = target;

        QWidget *focusProxy = target->d_func()->deepestFocusProxy();
        if (!focusProxy || !target->isAncestorOf(focusProxy))
            return;

        lastFocusChild = focusProxy;

        for (QWidget *focusNext = lastFocusChild->d_func()->focus_next;
             focusNext != focusProxy && target->isAncestorOf(focusNext) && focusNext->window() == focusProxy->window();
             focusNext = focusNext->d_func()->focus_next) {
            if (focusNext->focusPolicy() != Qt::NoFocus)
                lastFocusChild = focusNext;
        }
    };
    auto setPrev = [](QWidget *w, QWidget *prev)
    {
        w->d_func()->focus_prev = prev;
    };
    auto setNext = [](QWidget *w, QWidget *next)
    {
        w->d_func()->focus_next = next;
    };

    // remove the second widget from the chain
    QWidget *lastFocusChildOfSecond;
    determineLastFocusChild(second, lastFocusChildOfSecond);
    {
        QWidget *oldPrev = second->d_func()->focus_prev;
        QWidget *prevWithFocus = oldPrev;
        while (prevWithFocus->focusPolicy() == Qt::NoFocus)
            prevWithFocus = prevWithFocus->d_func()->focus_prev;
        // only widgets between first and second -> all is fine
        if (prevWithFocus == first)
            return;
        QWidget *oldNext = lastFocusChildOfSecond->d_func()->focus_next;
        setPrev(oldNext, oldPrev);
        setNext(oldPrev, oldNext);
    }

    // insert the second widget into the chain
    QWidget *lastFocusChildOfFirst;
    determineLastFocusChild(first, lastFocusChildOfFirst);
    {
        QWidget *oldNext = lastFocusChildOfFirst->d_func()->focus_next;
        setPrev(second, lastFocusChildOfFirst);
        setNext(lastFocusChildOfFirst, second);
        setPrev(oldNext, lastFocusChildOfSecond);
        setNext(lastFocusChildOfSecond, oldNext);
    }
}

/*!\internal

  Moves the relevant subwidgets of this widget from the \a oldtlw's
  tab chain to that of the new parent, if there's anything to move and
  we're really moving

  This function is called from QWidget::reparent() *after* the widget
  has been reparented.

  \sa reparent()
*/

void QWidgetPrivate::reparentFocusWidgets(QWidget * oldtlw)
{
    Q_Q(QWidget);
    if (oldtlw == q->window())
        return; // nothing to do

    if(focus_child)
        focus_child->clearFocus();

    // separate the focus chain into new (children of myself) and old (the rest)
    QWidget *firstOld = nullptr;
    //QWidget *firstNew = q; //invariant
    QWidget *o = nullptr; // last in the old list
    QWidget *n = q; // last in the new list

    bool prevWasNew = true;
    QWidget *w = focus_next;

    //Note: for efficiency, we do not maintain the list invariant inside the loop
    //we append items to the relevant list, and we optimize by not changing pointers
    //when subsequent items are going into the same list.
    while (w  != q) {
        bool currentIsNew =  q->isAncestorOf(w);
        if (currentIsNew) {
            if (!prevWasNew) {
                //prev was old -- append to new list
                n->d_func()->focus_next = w;
                w->d_func()->focus_prev = n;
            }
            n = w;
        } else {
            if (prevWasNew) {
                //prev was new -- append to old list, if there is one
                if (o) {
                    o->d_func()->focus_next = w;
                    w->d_func()->focus_prev = o;
                } else {
                    // "create" the old list
                    firstOld = w;
                }
            }
            o = w;
        }
        w = w->d_func()->focus_next;
        prevWasNew = currentIsNew;
    }

    //repair the old list:
    if (firstOld) {
        o->d_func()->focus_next = firstOld;
        firstOld->d_func()->focus_prev = o;
    }

    if (!q->isWindow()) {
        QWidget *topLevel = q->window();
        //insert new chain into toplevel's chain

        QWidget *prev = topLevel->d_func()->focus_prev;

        topLevel->d_func()->focus_prev = n;
        prev->d_func()->focus_next = q;

        focus_prev = prev;
        n->d_func()->focus_next = topLevel;
    } else {
        //repair the new list
            n->d_func()->focus_next = q;
            focus_prev = n;
    }

}

/*!\internal

  Measures the shortest distance from a point to a rect.

  This function is called from QDesktopwidget::screen(QPoint) to find the
  closest screen for a point.
  In directional KeypadNavigation, it is called to find the closest
  widget to the current focus widget center.
*/
int QWidgetPrivate::pointToRect(const QPoint &p, const QRect &r)
{
    int dx = 0;
    int dy = 0;
    if (p.x() < r.left())
        dx = r.left() - p.x();
    else if (p.x() > r.right())
        dx = p.x() - r.right();
    if (p.y() < r.top())
        dy = r.top() - p.y();
    else if (p.y() > r.bottom())
        dy = p.y() - r.bottom();
    return dx + dy;
}

/*!
    \property QWidget::frameSize
    \brief the size of the widget including any window frame

    By default, this property contains a value that depends on the user's
    platform and screen geometry.
*/
QSize QWidget::frameSize() const
{
    Q_D(const QWidget);
    if (isWindow() && !(windowType() == Qt::Popup)) {
        QRect fs = d->frameStrut();
        return QSize(data->crect.width() + fs.left() + fs.right(),
                      data->crect.height() + fs.top() + fs.bottom());
    }
    return data->crect.size();
}

/*! \fn void QWidget::move(int x, int y)

    \overload

    This corresponds to move(QPoint(\a x, \a y)).
*/

void QWidget::move(const QPoint &p)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_Moved);
    if (testAttribute(Qt::WA_WState_Created)) {
        if (isWindow())
            d->topData()->posIncludesFrame = false;
        d->setGeometry_sys(p.x() + geometry().x() - QWidget::x(),
                       p.y() + geometry().y() - QWidget::y(),
                       width(), height(), true);
        d->setDirtyOpaqueRegion();
    } else {
        // no frame yet: see also QWidgetPrivate::fixPosIncludesFrame(), QWindowPrivate::PositionPolicy.
        if (isWindow())
            d->topData()->posIncludesFrame = true;
        data->crect.moveTopLeft(p); // no frame yet
        setAttribute(Qt::WA_PendingMoveEvent);
    }

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasMoved(this);
}

// move() was invoked with Qt::WA_WState_Created not set (frame geometry
// unknown), that is, crect has a position including the frame.
// If we can determine the frame strut, fix that and clear the flag.
// FIXME: This does not play well with window states other than
// Qt::WindowNoState, as we depend on calling setGeometry() on the
// platform window after fixing up the position so that the new
// geometry is reflected in the platform window, but when the frame
// comes in after the window has been shown (e.g. maximized), we're
// not in a position to do that kind of fixup.
void QWidgetPrivate::fixPosIncludesFrame()
{
    Q_Q(QWidget);
    if (QTLWExtra *te = maybeTopData()) {
        if (te->posIncludesFrame) {
            // For Qt::WA_DontShowOnScreen, assume a frame of 0 (for
            // example, in QGraphicsProxyWidget).
            if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
                te->posIncludesFrame = 0;
            } else {
                if (q->windowHandle() && q->windowHandle()->handle()) {
                    updateFrameStrut();
                    if (!q->data->fstrut_dirty) {
                        data.crect.translate(te->frameStrut.x(), te->frameStrut.y());
                        te->posIncludesFrame = 0;
                    }
                } // windowHandle()
            } // !WA_DontShowOnScreen
        } // posIncludesFrame
    } // QTLWExtra
}

/*! \fn void QWidget::resize(int w, int h)
    \overload

    This corresponds to resize(QSize(\a w, \a h)).
*/

void QWidget::resize(const QSize &s)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_Resized);
    if (testAttribute(Qt::WA_WState_Created)) {
        d->fixPosIncludesFrame();
        d->setGeometry_sys(geometry().x(), geometry().y(), s.width(), s.height(), false);
        d->setDirtyOpaqueRegion();
    } else {
        const auto oldRect = data->crect;
        data->crect.setSize(s.boundedTo(maximumSize()).expandedTo(minimumSize()));
        if (oldRect != data->crect)
            setAttribute(Qt::WA_PendingResizeEvent);
    }
}

void QWidget::setGeometry(const QRect &r)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_Resized);
    setAttribute(Qt::WA_Moved);
    if (isWindow())
        d->topData()->posIncludesFrame = 0;
    if (testAttribute(Qt::WA_WState_Created)) {
        d->setGeometry_sys(r.x(), r.y(), r.width(), r.height(), true);
        d->setDirtyOpaqueRegion();
    } else {
        const auto oldRect = data->crect;
        data->crect.setTopLeft(r.topLeft());
        data->crect.setSize(r.size().boundedTo(maximumSize()).expandedTo(minimumSize()));
        if (oldRect != data->crect) {
            setAttribute(Qt::WA_PendingMoveEvent);
            setAttribute(Qt::WA_PendingResizeEvent);
        }
    }

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasMoved(this);
}

void QWidgetPrivate::setGeometry_sys(int x, int y, int w, int h, bool isMove)
{
    Q_Q(QWidget);
    if (extra) {                                // any size restrictions?
        w = qMin(w,extra->maxw);
        h = qMin(h,extra->maxh);
        w = qMax(w,extra->minw);
        h = qMax(h,extra->minh);
    }

    if (q->isWindow() && q->windowHandle()) {
        QPlatformIntegration *integration = QGuiApplicationPrivate::platformIntegration();
        if (!integration->hasCapability(QPlatformIntegration::NonFullScreenWindows)) {
            x = 0;
            y = 0;
            w = q->windowHandle()->width();
            h = q->windowHandle()->height();
        }
    }

    QPoint oldp = q->geometry().topLeft();
    QSize olds = q->size();
    QRect r(x, y, w, h);

    bool isResize = olds != r.size();
    if (!isMove)
        isMove = oldp != r.topLeft();


    // We only care about stuff that changes the geometry, or may
    // cause the window manager to change its state
    if (r.size() == olds && oldp == r.topLeft())
        return;

    if (!data.in_set_window_state) {
        q->data->window_state &= ~Qt::WindowMaximized;
        q->data->window_state &= ~Qt::WindowFullScreen;
        if (q->isWindow())
            topData()->normalGeometry = QRect(0, 0, -1, -1);
    }

    QPoint oldPos = q->pos();
    data.crect = r;

    bool needsShow = false;

    if (q->isWindow() || q->windowHandle()) {
        if (!(data.window_state & Qt::WindowFullScreen) && (w == 0 || h == 0)) {
            q->setAttribute(Qt::WA_OutsideWSRange, true);
            if (q->isVisible())
                hide_sys();
            data.crect = QRect(x, y, w, h);
        } else if (q->testAttribute(Qt::WA_OutsideWSRange)) {
            q->setAttribute(Qt::WA_OutsideWSRange, false);
            needsShow = true;
        }
    }

    if (q->isVisible()) {
        if (!q->testAttribute(Qt::WA_DontShowOnScreen) && !q->testAttribute(Qt::WA_OutsideWSRange)) {
            if (QWindow *win = q->windowHandle()) {
                if (q->isWindow()) {
                    if (isResize && !isMove)
                        win->resize(w, h);
                    else if (isMove && !isResize)
                        win->setPosition(x, y);
                    else
                        win->setGeometry(q->geometry());
                } else {
                    QPoint posInNativeParent =  q->mapTo(q->nativeParentWidget(),QPoint());
                    win->setGeometry(QRect(posInNativeParent,r.size()));
                }

                if (needsShow)
                    show_sys();
            }

            if (!q->isWindow()) {
                if (renderToTexture) {
                    QRegion updateRegion(q->geometry());
                    updateRegion += QRect(oldPos, olds);
                    q->parentWidget()->d_func()->invalidateBackingStore(updateRegion);
                } else if (isMove && !isResize) {
                    moveRect(QRect(oldPos, olds), x - oldPos.x(), y - oldPos.y());
                } else {
                    invalidateBackingStore_resizeHelper(oldPos, olds);
                }
            }
        }

        if (isMove) {
            QMoveEvent e(q->pos(), oldPos);
            QCoreApplication::sendEvent(q, &e);
        }
        if (isResize) {
            QResizeEvent e(r.size(), olds);
            QCoreApplication::sendEvent(q, &e);
            if (q->windowHandle())
                q->update();
        }
    } else { // not visible
        if (isMove && q->pos() != oldPos)
            q->setAttribute(Qt::WA_PendingMoveEvent, true);
        if (isResize)
            q->setAttribute(Qt::WA_PendingResizeEvent, true);
    }

}

/*!
    \since 4.2
    Saves the current geometry and state for top-level widgets.

    To save the geometry when the window closes, you can
    implement a close event like this:

    \snippet code/src_gui_kernel_qwidget.cpp 11

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    Use QMainWindow::saveState() to save the geometry and the state of
    toolbars and dock widgets.

    \sa restoreGeometry(), QMainWindow::saveState(), QMainWindow::restoreState()
*/
QByteArray QWidget::saveGeometry() const
{
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_0);
    const quint32 magicNumber = 0x1D9D0CB;
    // Version history:
    // - Qt 4.2 - 4.8.6, 5.0 - 5.3    : Version 1.0
    // - Qt 4.8.6 - today, 5.4 - today: Version 2.0, save screen width in addition to check for high DPI scaling.
    // - Qt 5.12 - today              : Version 3.0, save QWidget::geometry()
    quint16 majorVersion = 3;
    quint16 minorVersion = 0;
    const int screenNumber = QDesktopWidgetPrivate::screenNumber(this);
    stream << magicNumber
           << majorVersion
           << minorVersion
           << frameGeometry()
           << normalGeometry()
           << qint32(screenNumber)
           << quint8(windowState() & Qt::WindowMaximized)
           << quint8(windowState() & Qt::WindowFullScreen)
           << qint32(QDesktopWidgetPrivate::screenGeometry(screenNumber).width()) // added in 2.0
           << geometry(); // added in 3.0
    return array;
}

static void checkRestoredGeometry(const QRect &availableGeometry, QRect *restoredGeometry,
                                  int frameHeight)
{
    if (!restoredGeometry->intersects(availableGeometry)) {
        restoredGeometry->moveBottom(qMin(restoredGeometry->bottom(), availableGeometry.bottom()));
        restoredGeometry->moveLeft(qMax(restoredGeometry->left(), availableGeometry.left()));
        restoredGeometry->moveRight(qMin(restoredGeometry->right(), availableGeometry.right()));
    }
    restoredGeometry->moveTop(qMax(restoredGeometry->top(), availableGeometry.top() + frameHeight));
}

/*!
    \since 4.2

    Restores the geometry and state of top-level widgets stored in the
    byte array \a geometry. Returns \c true on success; otherwise
    returns \c false.

    If the restored geometry is off-screen, it will be modified to be
    inside the available screen geometry.

    To restore geometry saved using QSettings, you can use code like
    this:

    \snippet code/src_gui_kernel_qwidget.cpp 12

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    Use QMainWindow::restoreState() to restore the geometry and the
    state of toolbars and dock widgets.

    \sa saveGeometry(), QSettings, QMainWindow::saveState(), QMainWindow::restoreState()
*/
bool QWidget::restoreGeometry(const QByteArray &geometry)
{
    if (geometry.size() < 4)
        return false;
    QDataStream stream(geometry);
    stream.setVersion(QDataStream::Qt_4_0);

    const quint32 magicNumber = 0x1D9D0CB;
    quint32 storedMagicNumber;
    stream >> storedMagicNumber;
    if (storedMagicNumber != magicNumber)
        return false;

    const quint16 currentMajorVersion = 3;
    quint16 majorVersion = 0;
    quint16 minorVersion = 0;

    stream >> majorVersion >> minorVersion;

    if (majorVersion > currentMajorVersion)
        return false;
    // (Allow all minor versions.)

    QRect restoredFrameGeometry;
    QRect restoredGeometry;
    QRect restoredNormalGeometry;
    qint32 restoredScreenNumber;
    quint8 maximized;
    quint8 fullScreen;
    qint32 restoredScreenWidth = 0;

    stream >> restoredFrameGeometry // Only used for sanity checks in version 0
           >> restoredNormalGeometry
           >> restoredScreenNumber
           >> maximized
           >> fullScreen;

    if (majorVersion > 1)
        stream >> restoredScreenWidth;
    if (majorVersion > 2)
        stream >> restoredGeometry;

    // ### Qt 6 - Perhaps it makes sense to dumb down the restoreGeometry() logic, see QTBUG-69104

    if (restoredScreenNumber >= QDesktopWidgetPrivate::numScreens())
        restoredScreenNumber = QDesktopWidgetPrivate::primaryScreen();
    const qreal screenWidthF = qreal(QDesktopWidgetPrivate::screenGeometry(restoredScreenNumber).width());
    // Sanity check bailing out when large variations of screen sizes occur due to
    // high DPI scaling or different levels of DPI awareness.
    if (restoredScreenWidth) {
        const qreal factor = qreal(restoredScreenWidth) / screenWidthF;
        if (factor < 0.8 || factor > 1.25)
            return false;
    } else {
        // Saved by Qt 5.3 and earlier, try to prevent too large windows
        // unless the size will be adapted by maximized or fullscreen.
        if (!maximized && !fullScreen && qreal(restoredFrameGeometry.width()) / screenWidthF > 1.5)
            return false;
    }

    const int frameHeight = 20;

    if (!restoredNormalGeometry.isValid())
        restoredNormalGeometry = QRect(QPoint(0, frameHeight), sizeHint());
    if (!restoredNormalGeometry.isValid()) {
        // use the widget's adjustedSize if the sizeHint() doesn't help
        restoredNormalGeometry.setSize(restoredNormalGeometry
                                       .size()
                                       .expandedTo(d_func()->adjustedSize()));
    }

    const QRect availableGeometry = QDesktopWidgetPrivate::availableGeometry(restoredScreenNumber);

    // Modify the restored geometry if we are about to restore to coordinates
    // that would make the window "lost". This happens if:
    // - The restored geometry is completely oustside the available geometry
    // - The title bar is outside the available geometry.

    checkRestoredGeometry(availableGeometry, &restoredGeometry, frameHeight);
    checkRestoredGeometry(availableGeometry, &restoredNormalGeometry, frameHeight);

    if (maximized || fullScreen) {
        // set geometry before setting the window state to make
        // sure the window is maximized to the right screen.
        Qt::WindowStates ws = windowState();
#ifndef Q_OS_WIN
        setGeometry(restoredNormalGeometry);
#else
        if (ws & Qt::WindowFullScreen) {
            // Full screen is not a real window state on Windows.
            move(availableGeometry.topLeft());
        } else if (ws & Qt::WindowMaximized) {
            // Setting a geometry on an already maximized window causes this to be
            // restored into a broken, half-maximized state, non-resizable state (QTBUG-4397).
            // Move the window in normal state if needed.
            if (restoredScreenNumber != QDesktopWidgetPrivate::screenNumber(this)) {
                setWindowState(Qt::WindowNoState);
                setGeometry(restoredNormalGeometry);
            }
        } else {
            setGeometry(restoredNormalGeometry);
        }
#endif // Q_OS_WIN
        if (maximized)
            ws |= Qt::WindowMaximized;
        if (fullScreen)
            ws |= Qt::WindowFullScreen;
       setWindowState(ws);
       d_func()->topData()->normalGeometry = restoredNormalGeometry;
    } else {
        setWindowState(windowState() & ~(Qt::WindowMaximized | Qt::WindowFullScreen));
        if (majorVersion > 2)
            setGeometry(restoredGeometry);
        else
            setGeometry(restoredNormalGeometry);
    }
    return true;
}

/*!\fn void QWidget::setGeometry(int x, int y, int w, int h)
    \overload

    This corresponds to setGeometry(QRect(\a x, \a y, \a w, \a h)).
*/

/*!
  Sets the margins around the contents of the widget to have the sizes
  \a left, \a top, \a right, and \a bottom. The margins are used by
  the layout system, and may be used by subclasses to specify the area
  to draw in (e.g. excluding the frame).

  Changing the margins will trigger a resizeEvent().

  \sa contentsRect(), contentsMargins()
*/
void QWidget::setContentsMargins(int left, int top, int right, int bottom)
{
    Q_D(QWidget);
    if (left == d->leftmargin && top == d->topmargin
         && right == d->rightmargin && bottom == d->bottommargin)
        return;
    d->leftmargin = left;
    d->topmargin = top;
    d->rightmargin = right;
    d->bottommargin = bottom;

    d->updateContentsRect();
}

/*!
  \overload
  \since 4.6

  \brief The setContentsMargins function sets the margins around the
  widget's contents.

  Sets the margins around the contents of the widget to have the
  sizes determined by \a margins. The margins are
  used by the layout system, and may be used by subclasses to
  specify the area to draw in (e.g. excluding the frame).

  Changing the margins will trigger a resizeEvent().

  \sa contentsRect(), contentsMargins()
*/
void QWidget::setContentsMargins(const QMargins &margins)
{
    setContentsMargins(margins.left(), margins.top(),
                       margins.right(), margins.bottom());
}

void QWidgetPrivate::updateContentsRect()
{
    Q_Q(QWidget);

    if (layout)
        layout->update(); //force activate; will do updateGeometry
    else
        q->updateGeometry();

    if (q->isVisible()) {
        q->update();
        QResizeEvent e(q->data->crect.size(), q->data->crect.size());
        QCoreApplication::sendEvent(q, &e);
    } else {
        q->setAttribute(Qt::WA_PendingResizeEvent, true);
    }

    QEvent e(QEvent::ContentsRectChange);
    QCoreApplication::sendEvent(q, &e);
}

#if QT_DEPRECATED_SINCE(5, 14)
/*!
    \obsolete
    Use contentsMargins().

  Returns the widget's contents margins for \a left, \a top, \a
  right, and \a bottom.

  \sa setContentsMargins(), contentsRect()
 */
void QWidget::getContentsMargins(int *left, int *top, int *right, int *bottom) const
{
    QMargins m = contentsMargins();
    if (left)
        *left = m.left();
    if (top)
        *top = m.top();
    if (right)
        *right = m.right();
    if (bottom)
        *bottom = m.bottom();
}
#endif

// FIXME: Move to qmargins.h for next minor Qt release
QMargins operator|(const QMargins &m1, const QMargins &m2)
{
    return QMargins(qMax(m1.left(), m2.left()), qMax(m1.top(), m2.top()),
        qMax(m1.right(), m2.right()), qMax(m1.bottom(), m2.bottom()));
}

/*!
  \since 4.6

  \brief The contentsMargins function returns the widget's contents margins.

  \sa setContentsMargins(), contentsRect()
 */
QMargins QWidget::contentsMargins() const
{
    Q_D(const QWidget);
    QMargins userMargins(d->leftmargin, d->topmargin, d->rightmargin, d->bottommargin);
    return testAttribute(Qt::WA_ContentsMarginsRespectsSafeArea) ?
        userMargins | d->safeAreaMargins() : userMargins;
}

/*!
    Returns the area inside the widget's margins.

    \sa setContentsMargins(), contentsMargins()
*/
QRect QWidget::contentsRect() const
{
    return rect() - contentsMargins();
}

QMargins QWidgetPrivate::safeAreaMargins() const
{
    Q_Q(const QWidget);
    QWidget *nativeWidget = q->window();
    if (!nativeWidget->windowHandle())
        return QMargins();

    QPlatformWindow *platformWindow = nativeWidget->windowHandle()->handle();
    if (!platformWindow)
        return QMargins();

    QMargins safeAreaMargins = platformWindow->safeAreaMargins();

    if (!q->isWindow()) {
        // In theory the native parent widget already has a contents rect reflecting
        // the safe area of that widget, but we can't be sure that the widget or child
        // widgets of that widget have respected the contents rect when setting their
        // geometry, so we need to manually compute the safe area.

        // Unless the native widget doesn't have any margins, in which case there's
        // nothing for us to compute.
        if (safeAreaMargins.isNull())
            return QMargins();

        // Or, if one of our ancestors are in a layout that does not have WA_LayoutOnEntireRect
        // set, then we know that the layout has already taken care of placing us inside the
        // safe area, by taking the contents rect of its parent widget into account.
        const QWidget *assumedSafeWidget = nullptr;
        for (const QWidget *w = q; w != nativeWidget; w = w->parentWidget()) {
            QWidget *parentWidget = w->parentWidget();
            if (parentWidget->testAttribute(Qt::WA_LayoutOnEntireRect))
                continue; // Layout not going to help us

            QLayout *layout = parentWidget->layout();
            if (!layout)
                continue;

            if (layout->geometry().isNull())
                continue; // Layout hasn't been activated yet

            if (layout->indexOf(const_cast<QWidget *>(w)) < 0)
                continue; // Widget is not in layout

            assumedSafeWidget = w;
            break;
        }

#if !defined(QT_DEBUG)
        if (assumedSafeWidget) {
            // We found a layout that we assume will take care of keeping us within the safe area
            // For debug builds we still map the safe area using the fallback logic, so that we
            // can detect any misbehaving layouts.
            return QMargins();
        }
#endif

        // In all other cases we need to map the safe area of the native parent to the widget.
        // This depends on the widget being positioned and sized already, which means the initial
        // layout will be wrong, but the layout will then adjust itself.
        QPoint topLeftMargins = q->mapFrom(nativeWidget, QPoint(safeAreaMargins.left(), safeAreaMargins.top()));
        QRect widgetRect = q->isVisible() ? q->visibleRegion().boundingRect() : q->rect();
        QPoint bottomRightMargins = widgetRect.bottomRight() - q->mapFrom(nativeWidget,
            nativeWidget->rect().bottomRight() - QPoint(safeAreaMargins.right(), safeAreaMargins.bottom()));

        // Margins should never be negative
        safeAreaMargins = QMargins(qMax(0, topLeftMargins.x()), qMax(0, topLeftMargins.y()),
            qMax(0, bottomRightMargins.x()), qMax(0, bottomRightMargins.y()));

        if (!safeAreaMargins.isNull() && assumedSafeWidget) {
            QLayout *layout = assumedSafeWidget->parentWidget()->layout();
            qWarning() << layout << "is laying out" << assumedSafeWidget
                << "outside of the contents rect of" << layout->parentWidget();
            return QMargins(); // Return empty margin to visually highlight the error
        }
    }

    return safeAreaMargins;
}

/*!
  \fn void QWidget::customContextMenuRequested(const QPoint &pos)

  This signal is emitted when the widget's \l contextMenuPolicy is
  Qt::CustomContextMenu, and the user has requested a context menu on
  the widget. The position \a pos is the position of the context menu
  event that the widget receives. Normally this is in widget
  coordinates. The exception to this rule is QAbstractScrollArea and
  its subclasses that map the context menu event to coordinates of the
  \l{QAbstractScrollArea::viewport()}{viewport()}.


  \sa mapToGlobal(), QMenu, contextMenuPolicy
*/


/*!
    \property QWidget::contextMenuPolicy
    \brief how the widget shows a context menu

    The default value of this property is Qt::DefaultContextMenu,
    which means the contextMenuEvent() handler is called. Other values
    are Qt::NoContextMenu, Qt::PreventContextMenu,
    Qt::ActionsContextMenu, and Qt::CustomContextMenu. With
    Qt::CustomContextMenu, the signal customContextMenuRequested() is
    emitted.

    \sa contextMenuEvent(), customContextMenuRequested(), actions()
*/

Qt::ContextMenuPolicy QWidget::contextMenuPolicy() const
{
    return (Qt::ContextMenuPolicy)data->context_menu_policy;
}

void QWidget::setContextMenuPolicy(Qt::ContextMenuPolicy policy)
{
    data->context_menu_policy = (uint) policy;
}

/*!
    \property QWidget::focusPolicy
    \brief the way the widget accepts keyboard focus

    The policy is Qt::TabFocus if the widget accepts keyboard
    focus by tabbing, Qt::ClickFocus if the widget accepts
    focus by clicking, Qt::StrongFocus if it accepts both, and
    Qt::NoFocus (the default) if it does not accept focus at
    all.

    You must enable keyboard focus for a widget if it processes
    keyboard events. This is normally done from the widget's
    constructor. For instance, the QLineEdit constructor calls
    setFocusPolicy(Qt::StrongFocus).

    If the widget has a focus proxy, then the focus policy will
    be propagated to it.

    \sa focusInEvent(), focusOutEvent(), keyPressEvent(), keyReleaseEvent(), enabled
*/


Qt::FocusPolicy QWidget::focusPolicy() const
{
    return (Qt::FocusPolicy)data->focus_policy;
}

void QWidget::setFocusPolicy(Qt::FocusPolicy policy)
{
    data->focus_policy = (uint) policy;
    Q_D(QWidget);
    if (d->extra && d->extra->focus_proxy)
        d->extra->focus_proxy->setFocusPolicy(policy);
}

/*!
    \property QWidget::updatesEnabled
    \brief whether updates are enabled

    An updates enabled widget receives paint events and has a system
    background; a disabled widget does not. This also implies that
    calling update() and repaint() has no effect if updates are
    disabled.

    By default, this property is \c true.

    setUpdatesEnabled() is normally used to disable updates for a
    short period of time, for instance to avoid screen flicker during
    large changes. In Qt, widgets normally do not generate screen
    flicker, but on X11 the server might erase regions on the screen
    when widgets get hidden before they can be replaced by other
    widgets. Disabling updates solves this.

    Example:
    \snippet code/src_gui_kernel_qwidget.cpp 13

    Disabling a widget implicitly disables all its children. Enabling a widget
    enables all child widgets \e except top-level widgets or those that
    have been explicitly disabled. Re-enabling updates implicitly calls
    update() on the widget.

    \sa paintEvent()
*/
void QWidget::setUpdatesEnabled(bool enable)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_ForceUpdatesDisabled, !enable);
    d->setUpdatesEnabled_helper(enable);
}

/*!
    Shows the widget and its child widgets.

    This is equivalent to calling showFullScreen(), showMaximized(), or setVisible(true),
    depending on the platform's default behavior for the window flags.

     \sa raise(), showEvent(), hide(), setVisible(), showMinimized(), showMaximized(),
    showNormal(), isVisible(), windowFlags()
*/
void QWidget::show()
{
    Qt::WindowState defaultState = QGuiApplicationPrivate::platformIntegration()->defaultWindowState(data->window_flags);
    if (defaultState == Qt::WindowFullScreen)
        showFullScreen();
    else if (defaultState == Qt::WindowMaximized)
        showMaximized();
    else
        setVisible(true); // Don't call showNormal() as not to clobber Qt::Window(Max/Min)imized
}

/*! \internal

   Makes the widget visible in the isVisible() meaning of the word.
   It is only called for toplevels or widgets with visible parents.
 */
void QWidgetPrivate::show_recursive()
{
    Q_Q(QWidget);
    // polish if necessary

    if (!q->testAttribute(Qt::WA_WState_Created))
        createRecursively();
    q->ensurePolished();

    if (!q->isWindow() && q->parentWidget()->d_func()->layout && !q->parentWidget()->data->in_show)
        q->parentWidget()->d_func()->layout->activate();
    // activate our layout before we and our children become visible
    if (layout)
        layout->activate();

    show_helper();
}

void QWidgetPrivate::sendPendingMoveAndResizeEvents(bool recursive, bool disableUpdates)
{
    Q_Q(QWidget);

    disableUpdates = disableUpdates && q->updatesEnabled();
    if (disableUpdates)
        q->setAttribute(Qt::WA_UpdatesDisabled);

    if (q->testAttribute(Qt::WA_PendingMoveEvent)) {
        QMoveEvent e(data.crect.topLeft(), data.crect.topLeft());
        QCoreApplication::sendEvent(q, &e);
        q->setAttribute(Qt::WA_PendingMoveEvent, false);
    }

    if (q->testAttribute(Qt::WA_PendingResizeEvent)) {
        QResizeEvent e(data.crect.size(), QSize());
        QCoreApplication::sendEvent(q, &e);
        q->setAttribute(Qt::WA_PendingResizeEvent, false);
    }

    if (disableUpdates)
        q->setAttribute(Qt::WA_UpdatesDisabled, false);

    if (!recursive)
        return;

    for (int i = 0; i < children.size(); ++i) {
        if (QWidget *child = qobject_cast<QWidget *>(children.at(i)))
            child->d_func()->sendPendingMoveAndResizeEvents(recursive, disableUpdates);
    }
}

void QWidgetPrivate::activateChildLayoutsRecursively()
{
    sendPendingMoveAndResizeEvents(false, true);

    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (!child || child->isHidden() || child->isWindow())
            continue;

        child->ensurePolished();

        // Activate child's layout
        QWidgetPrivate *childPrivate = child->d_func();
        if (childPrivate->layout)
            childPrivate->layout->activate();

        // Pretend we're visible.
        const bool wasVisible = child->isVisible();
        if (!wasVisible)
            child->setAttribute(Qt::WA_WState_Visible);

        // Do the same for all my children.
        childPrivate->activateChildLayoutsRecursively();

        // We're not cheating anymore.
        if (!wasVisible)
            child->setAttribute(Qt::WA_WState_Visible, false);
    }
}

void QWidgetPrivate::show_helper()
{
    Q_Q(QWidget);
    data.in_show = true; // qws optimization
    // make sure we receive pending move and resize events
    sendPendingMoveAndResizeEvents();

    // become visible before showing all children
    q->setAttribute(Qt::WA_WState_Visible);

    // finally show all children recursively
    showChildren(false);



    const bool isWindow = q->isWindow();
#if QT_CONFIG(graphicsview)
    bool isEmbedded = isWindow && q->graphicsProxyWidget() != nullptr;
#else
    bool isEmbedded = false;
#endif

    // popup handling: new popups and tools need to be raised, and
    // existing popups must be closed. Also propagate the current
    // windows's KeyboardFocusChange status.
    if (isWindow && !isEmbedded) {
        if ((q->windowType() == Qt::Tool) || (q->windowType() == Qt::Popup) || q->windowType() == Qt::ToolTip) {
            q->raise();
            if (q->parentWidget() && q->parentWidget()->window()->testAttribute(Qt::WA_KeyboardFocusChange))
                q->setAttribute(Qt::WA_KeyboardFocusChange);
        } else {
            while (QApplication::activePopupWidget()) {
                if (!QApplication::activePopupWidget()->close())
                    break;
            }
        }
    }

    // Automatic embedding of child windows of widgets already embedded into
    // QGraphicsProxyWidget when they are shown the first time.
#if QT_CONFIG(graphicsview)
    if (isWindow) {
        if (!isEmbedded && !bypassGraphicsProxyWidget(q)) {
            QGraphicsProxyWidget *ancestorProxy = nearestGraphicsProxyWidget(q->parentWidget());
            if (ancestorProxy) {
                isEmbedded = true;
                ancestorProxy->d_func()->embedSubWindow(q);
            }
        }
    }
#else
    Q_UNUSED(isEmbedded);
#endif

    // send the show event before showing the window
    QShowEvent showEvent;
    QCoreApplication::sendEvent(q, &showEvent);

    show_sys();

    if (!isEmbedded && q->windowType() == Qt::Popup)
        qApp->d_func()->openPopup(q);

#ifndef QT_NO_ACCESSIBILITY
    if (q->windowType() != Qt::ToolTip) {    // Tooltips are read aloud twice in MS narrator.
        QAccessibleEvent event(q, QAccessible::ObjectShow);
        QAccessible::updateAccessibility(&event);
    }
#endif

    if (QApplicationPrivate::hidden_focus_widget == q) {
        QApplicationPrivate::hidden_focus_widget = nullptr;
        q->setFocus(Qt::OtherFocusReason);
    }

    // Process events when showing a Qt::SplashScreen widget before the event loop
    // is spinnning; otherwise it might not show up on particular platforms.
    // This makes QSplashScreen behave the same on all platforms.
    if (!qApp->d_func()->in_exec && q->windowType() == Qt::SplashScreen)
        QCoreApplication::processEvents();

    data.in_show = false;  // reset qws optimization
}

void QWidgetPrivate::show_sys()
{
    Q_Q(QWidget);

    auto window = qobject_cast<QWidgetWindow *>(windowHandle());

    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        invalidateBackingStore(q->rect());
        q->setAttribute(Qt::WA_Mapped);
        // add our window the modal window list (native dialogs)
        if (window && q->isWindow()
#if QT_CONFIG(graphicsview)
            && (!extra || !extra->proxyWidget)
#endif
            && q->windowModality() != Qt::NonModal) {
            QGuiApplicationPrivate::showModalWindow(window);
        }
        return;
    }

    if (renderToTexture && !q->isWindow())
        QCoreApplication::postEvent(q->parentWidget(), new QUpdateLaterEvent(q->geometry()));
    else
        QCoreApplication::postEvent(q, new QUpdateLaterEvent(q->rect()));

    if ((!q->isWindow() && !q->testAttribute(Qt::WA_NativeWindow))
            || q->testAttribute(Qt::WA_OutsideWSRange)) {
        return;
    }

    if (window) {
        if (q->isWindow())
            fixPosIncludesFrame();
        QRect geomRect = q->geometry();
        if (!q->isWindow()) {
            QPoint topLeftOfWindow = q->mapTo(q->nativeParentWidget(),QPoint());
            geomRect.moveTopLeft(topLeftOfWindow);
        }
        const QRect windowRect = window->geometry();
        if (windowRect != geomRect) {
            if (q->testAttribute(Qt::WA_Moved)
                || !QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowManagement))
                window->setGeometry(geomRect);
            else
                window->resize(geomRect.size());
        }

#ifndef QT_NO_CURSOR
        qt_qpa_set_cursor(q, false); // Needed in case cursor was set before show
#endif
        invalidateBackingStore(q->rect());
        window->setNativeWindowVisibility(true);
        // Was the window moved by the Window system or QPlatformWindow::initialGeometry() ?
        if (window->isTopLevel()) {
            const QPoint crectTopLeft = q->data->crect.topLeft();
            const QPoint windowTopLeft = window->geometry().topLeft();
            if (crectTopLeft == QPoint(0, 0) && windowTopLeft != crectTopLeft)
                q->data->crect.moveTopLeft(windowTopLeft);
        }
    }
}

/*!
    Hides the widget. This function is equivalent to
    setVisible(false).


    \note If you are working with QDialog or its subclasses and you invoke
    the show() function after this function, the dialog will be displayed in
    its original position.

    \sa hideEvent(), isHidden(), show(), setVisible(), isVisible(), close()
*/
void QWidget::hide()
{
    setVisible(false);
}

/*!\internal
 */
void QWidgetPrivate::hide_helper()
{
    Q_Q(QWidget);

    bool isEmbedded = false;
#if QT_CONFIG(graphicsview)
    isEmbedded = q->isWindow() && !bypassGraphicsProxyWidget(q) && nearestGraphicsProxyWidget(q->parentWidget()) != nullptr;
#else
    Q_UNUSED(isEmbedded);
#endif

    if (!isEmbedded && (q->windowType() == Qt::Popup))
        qApp->d_func()->closePopup(q);

    q->setAttribute(Qt::WA_Mapped, false);
    hide_sys();

    bool wasVisible = q->testAttribute(Qt::WA_WState_Visible);

    if (wasVisible) {
        q->setAttribute(Qt::WA_WState_Visible, false);

    }

    QHideEvent hideEvent;
    QCoreApplication::sendEvent(q, &hideEvent);
    hideChildren(false);

    // next bit tries to move the focus if the focus widget is now
    // hidden.
    if (wasVisible) {
        qApp->d_func()->sendSyntheticEnterLeave(q);
        QWidget *fw = QApplication::focusWidget();
        while (fw &&  !fw->isWindow()) {
            if (fw == q) {
                q->focusNextPrevChild(true);
                break;
            }
            fw = fw->parentWidget();
        }
    }

    if (QWidgetRepaintManager *repaintManager = maybeRepaintManager())
        repaintManager->removeDirtyWidget(q);

#ifndef QT_NO_ACCESSIBILITY
    if (wasVisible) {
        QAccessibleEvent event(q, QAccessible::ObjectHide);
        QAccessible::updateAccessibility(&event);
    }
#endif
}

void QWidgetPrivate::hide_sys()
{
    Q_Q(QWidget);

    auto window = qobject_cast<QWidgetWindow *>(windowHandle());

    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        q->setAttribute(Qt::WA_Mapped, false);
        // remove our window from the modal window list (native dialogs)
        if (window && q->isWindow()
#if QT_CONFIG(graphicsview)
            && (!extra || !extra->proxyWidget)
#endif
            && q->windowModality() != Qt::NonModal) {
            QGuiApplicationPrivate::hideModalWindow(window);
        }
        // do not return here, if window non-zero, we must hide it
    }

    deactivateWidgetCleanup();

    if (!q->isWindow()) {
        QWidget *p = q->parentWidget();
        if (p &&p->isVisible()) {
            if (renderToTexture)
                p->d_func()->invalidateBackingStore(q->geometry());
            else
                invalidateBackingStore(q->rect());
        }
    } else {
        invalidateBackingStore(q->rect());
    }

    if (window)
        window->setNativeWindowVisibility(false);
}

/*!
    \fn bool QWidget::isHidden() const

    Returns \c true if the widget is hidden, otherwise returns \c false.

    A hidden widget will only become visible when show() is called on
    it. It will not be automatically shown when the parent is shown.

    To check visibility, use !isVisible() instead (notice the exclamation mark).

    isHidden() implies !isVisible(), but a widget can be not visible
    and not hidden at the same time. This is the case for widgets that are children of
    widgets that are not visible.


    Widgets are hidden if:
    \list
        \li they were created as independent windows,
        \li they were created as children of visible widgets,
        \li hide() or setVisible(false) was called.
    \endlist
*/

void QWidget::setVisible(bool visible)
{
    if (testAttribute(Qt::WA_WState_ExplicitShowHide) && testAttribute(Qt::WA_WState_Hidden) == !visible)
        return;

    // Remember that setVisible was called explicitly
    setAttribute(Qt::WA_WState_ExplicitShowHide);

    Q_D(QWidget);
    d->setVisible(visible);
}

// This method is called from QWidgetWindow in response to QWindow::setVisible,
// and should match the semantics of QWindow::setVisible. QWidget::setVisible on
// the other hand keeps track of WA_WState_ExplicitShowHide in addition.
void QWidgetPrivate::setVisible(bool visible)
{
    Q_Q(QWidget);
    if (visible) { // show
        // Designer uses a trick to make grabWidget work without showing
        if (!q->isWindow() && q->parentWidget() && q->parentWidget()->isVisible()
            && !q->parentWidget()->testAttribute(Qt::WA_WState_Created))
            q->parentWidget()->window()->d_func()->createRecursively();

        //create toplevels but not children of non-visible parents
        QWidget *pw = q->parentWidget();
        if (!q->testAttribute(Qt::WA_WState_Created)
            && (q->isWindow() || pw->testAttribute(Qt::WA_WState_Created))) {
            q->create();
        }

        bool wasResized = q->testAttribute(Qt::WA_Resized);
        Qt::WindowStates initialWindowState = q->windowState();

        // polish if necessary
        q->ensurePolished();

        // whether we need to inform the parent widget immediately
        bool needUpdateGeometry = !q->isWindow() && q->testAttribute(Qt::WA_WState_Hidden);
        // we are no longer hidden
        q->setAttribute(Qt::WA_WState_Hidden, false);

        if (needUpdateGeometry)
            updateGeometry_helper(true);

        // activate our layout before we and our children become visible
        if (layout)
            layout->activate();

        if (!q->isWindow()) {
            QWidget *parent = q->parentWidget();
            while (parent && parent->isVisible() && parent->d_func()->layout  && !parent->data->in_show) {
                parent->d_func()->layout->activate();
                if (parent->isWindow())
                    break;
                parent = parent->parentWidget();
            }
            if (parent)
                parent->d_func()->setDirtyOpaqueRegion();
        }

        // adjust size if necessary
        if (!wasResized
            && (q->isWindow() || !q->parentWidget()->d_func()->layout))  {
            if (q->isWindow()) {
                q->adjustSize();
                if (q->windowState() != initialWindowState)
                    q->setWindowState(initialWindowState);
            } else {
                q->adjustSize();
            }
            q->setAttribute(Qt::WA_Resized, false);
        }

        q->setAttribute(Qt::WA_KeyboardFocusChange, false);

        if (q->isWindow() || q->parentWidget()->isVisible()) {
            show_helper();

            qApp->d_func()->sendSyntheticEnterLeave(q);
        }

        QEvent showToParentEvent(QEvent::ShowToParent);
        QCoreApplication::sendEvent(q, &showToParentEvent);
    } else { // hide
        if (QApplicationPrivate::hidden_focus_widget == q)
            QApplicationPrivate::hidden_focus_widget = nullptr;

        // hw: The test on getOpaqueRegion() needs to be more intelligent
        // currently it doesn't work if the widget is hidden (the region will
        // be clipped). The real check should be testing the cached region
        // (and dirty flag) directly.
        if (!q->isWindow() && q->parentWidget()) // && !d->getOpaqueRegion().isEmpty())
            q->parentWidget()->d_func()->setDirtyOpaqueRegion();

        if (!q->testAttribute(Qt::WA_WState_Hidden)) {
            q->setAttribute(Qt::WA_WState_Hidden);
            if (q->testAttribute(Qt::WA_WState_Created))
                hide_helper();
        }

        // invalidate layout similar to updateGeometry()
        if (!q->isWindow() && q->parentWidget()) {
            if (q->parentWidget()->d_func()->layout)
                q->parentWidget()->d_func()->layout->invalidate();
            else if (q->parentWidget()->isVisible())
                QCoreApplication::postEvent(q->parentWidget(), new QEvent(QEvent::LayoutRequest));
        }

        QEvent hideToParentEvent(QEvent::HideToParent);
        QCoreApplication::sendEvent(q, &hideToParentEvent);
    }
}

/*!
    Convenience function, equivalent to setVisible(!\a hidden).
*/
void QWidget::setHidden(bool hidden)
{
    setVisible(!hidden);
}

void QWidgetPrivate::_q_showIfNotHidden()
{
    Q_Q(QWidget);
    if ( !(q->isHidden() && q->testAttribute(Qt::WA_WState_ExplicitShowHide)) )
        q->setVisible(true);
}

void QWidgetPrivate::showChildren(bool spontaneous)
{
    QList<QObject*> childList = children;
    for (int i = 0; i < childList.size(); ++i) {
        QWidget *widget = qobject_cast<QWidget*>(childList.at(i));
        if (widget && widget->windowHandle() && !widget->testAttribute(Qt::WA_WState_ExplicitShowHide))
            widget->setAttribute(Qt::WA_WState_Hidden, false);
        if (!widget
            || widget->isWindow()
            || widget->testAttribute(Qt::WA_WState_Hidden))
            continue;
        if (spontaneous) {
            widget->setAttribute(Qt::WA_Mapped);
            widget->d_func()->showChildren(true);
            QShowEvent e;
            QApplication::sendSpontaneousEvent(widget, &e);
        } else {
            if (widget->testAttribute(Qt::WA_WState_ExplicitShowHide))
                widget->d_func()->show_recursive();
            else
                widget->show();
        }
    }
}

void QWidgetPrivate::hideChildren(bool spontaneous)
{
    QList<QObject*> childList = children;
    for (int i = 0; i < childList.size(); ++i) {
        QWidget *widget = qobject_cast<QWidget*>(childList.at(i));
        if (!widget || widget->isWindow() || widget->testAttribute(Qt::WA_WState_Hidden))
            continue;

        if (spontaneous)
            widget->setAttribute(Qt::WA_Mapped, false);
        else
            widget->setAttribute(Qt::WA_WState_Visible, false);
        widget->d_func()->hideChildren(spontaneous);
        QHideEvent e;
        if (spontaneous) {
            QApplication::sendSpontaneousEvent(widget, &e);
        } else {
            QCoreApplication::sendEvent(widget, &e);
            if (widget->internalWinId()
                && widget->testAttribute(Qt::WA_DontCreateNativeAncestors)) {
                // hide_sys() on an ancestor won't have any affect on this
                // widget, so it needs an explicit hide_sys() of its own
                widget->d_func()->hide_sys();
            }
        }
        qApp->d_func()->sendSyntheticEnterLeave(widget);
#ifndef QT_NO_ACCESSIBILITY
        if (!spontaneous) {
            QAccessibleEvent event(widget, QAccessible::ObjectHide);
            QAccessible::updateAccessibility(&event);
        }
#endif
    }
}

bool QWidgetPrivate::close_helper(CloseMode mode)
{
    if (data.is_closing)
        return true;

    Q_Q(QWidget);
    data.is_closing = 1;

    QPointer<QWidget> that = q;
    QPointer<QWidget> parentWidget = (q->parentWidget() && !QObjectPrivate::get(q->parentWidget())->wasDeleted) ? q->parentWidget() : nullptr;

    bool quitOnClose = q->testAttribute(Qt::WA_QuitOnClose);
    if (mode != CloseNoEvent) {
        QCloseEvent e;
        if (mode == CloseWithSpontaneousEvent)
            QApplication::sendSpontaneousEvent(q, &e);
        else
            QCoreApplication::sendEvent(q, &e);
        if (!that.isNull() && !e.isAccepted()) {
            data.is_closing = 0;
            return false;
        }
    }

    if (!that.isNull() && !q->isHidden())
        q->hide();

    // Attempt to close the application only if this has WA_QuitOnClose set and a non-visible parent
    quitOnClose = quitOnClose && (parentWidget.isNull() || !parentWidget->isVisible());

    if (quitOnClose) {
        /* if there is no non-withdrawn primary window left (except
           the ones without QuitOnClose), we emit the lastWindowClosed
           signal */
        QWidgetList list = QApplication::topLevelWidgets();
        bool lastWindowClosed = true;
        for (int i = 0; i < list.size(); ++i) {
            QWidget *w = list.at(i);
            if (!w->isVisible() || w->parentWidget() || !w->testAttribute(Qt::WA_QuitOnClose))
                continue;
            lastWindowClosed = false;
            break;
        }
        if (lastWindowClosed) {
            QGuiApplicationPrivate::emitLastWindowClosed();
            QCoreApplicationPrivate *applicationPrivate = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(QCoreApplication::instance()));
            applicationPrivate->maybeQuit();
        }
    }


    if (!that.isNull()) {
        data.is_closing = 0;
        if (q->testAttribute(Qt::WA_DeleteOnClose)) {
            q->setAttribute(Qt::WA_DeleteOnClose, false);
            q->deleteLater();
        }
    }
    return true;
}


/*!
    Closes this widget. Returns \c true if the widget was closed;
    otherwise returns \c false.

    First it sends the widget a QCloseEvent. The widget is
    \l{hide()}{hidden} if it \l{QEvent::accept()}{accepts}
    the close event. If it \l{QEvent::ignore()}{ignores}
    the event, nothing happens. The default
    implementation of QWidget::closeEvent() accepts the close event.

    If the widget has the Qt::WA_DeleteOnClose flag, the widget
    is also deleted. A close events is delivered to the widget no
    matter if the widget is visible or not.

    The \l QApplication::lastWindowClosed() signal is emitted when the
    last visible primary window (i.e. window with no parent) with the
    Qt::WA_QuitOnClose attribute set is closed. By default this
    attribute is set for all widgets except transient windows such as
    splash screens, tool windows, and popup menus.

*/

bool QWidget::close()
{
    return d_func()->close_helper(QWidgetPrivate::CloseWithEvent);
}

/*!
    \property QWidget::visible
    \brief whether the widget is visible

    Calling setVisible(true) or show() sets the widget to visible
    status if all its parent widgets up to the window are visible. If
    an ancestor is not visible, the widget won't become visible until
    all its ancestors are shown. If its size or position has changed,
    Qt guarantees that a widget gets move and resize events just
    before it is shown. If the widget has not been resized yet, Qt
    will adjust the widget's size to a useful default using
    adjustSize().

    Calling setVisible(false) or hide() hides a widget explicitly. An
    explicitly hidden widget will never become visible, even if all
    its ancestors become visible, unless you show it.

    A widget receives show and hide events when its visibility status
    changes. Between a hide and a show event, there is no need to
    waste CPU cycles preparing or displaying information to the user.
    A video application, for example, might simply stop generating new
    frames.

    A widget that happens to be obscured by other windows on the
    screen is considered to be visible. The same applies to iconified
    windows and windows that exist on another virtual
    desktop (on platforms that support this concept). A widget
    receives spontaneous show and hide events when its mapping status
    is changed by the window system, e.g. a spontaneous hide event
    when the user minimizes the window, and a spontaneous show event
    when the window is restored again.

    You almost never have to reimplement the setVisible() function. If
    you need to change some settings before a widget is shown, use
    showEvent() instead. If you need to do some delayed initialization
    use the Polish event delivered to the event() function.

    \sa show(), hide(), isHidden(), isVisibleTo(), isMinimized(),
    showEvent(), hideEvent()
*/


/*!
    Returns \c true if this widget would become visible if \a ancestor is
    shown; otherwise returns \c false.

    The true case occurs if neither the widget itself nor any parent
    up to but excluding \a ancestor has been explicitly hidden.

    This function will still return true if the widget is obscured by
    other windows on the screen, but could be physically visible if it
    or they were to be moved.

    isVisibleTo(0) is identical to isVisible().

    \sa show(), hide(), isVisible()
*/

bool QWidget::isVisibleTo(const QWidget *ancestor) const
{
    if (!ancestor)
        return isVisible();
    const QWidget * w = this;
    while (!w->isHidden()
            && !w->isWindow()
            && w->parentWidget()
            && w->parentWidget() != ancestor)
        w = w->parentWidget();
    return !w->isHidden();
}


/*!
    Returns the unobscured region where paint events can occur.

    For visible widgets, this is an approximation of the area not
    covered by other widgets; otherwise, this is an empty region.

    The repaint() function calls this function if necessary, so in
    general you do not need to call it.

*/
QRegion QWidget::visibleRegion() const
{
    Q_D(const QWidget);

    QRect clipRect = d->clipRect();
    if (clipRect.isEmpty())
        return QRegion();
    QRegion r(clipRect);
    d->subtractOpaqueChildren(r, clipRect);
    d->subtractOpaqueSiblings(r);
    return r;
}


QSize QWidgetPrivate::adjustedSize() const
{
    Q_Q(const QWidget);

    QSize s = q->sizeHint();

    if (q->isWindow()) {
        Qt::Orientations exp;
        if (layout) {
            if (layout->hasHeightForWidth())
                s.setHeight(layout->totalHeightForWidth(s.width()));
            exp = layout->expandingDirections();
        } else
        {
            if (q->sizePolicy().hasHeightForWidth())
                s.setHeight(q->heightForWidth(s.width()));
            exp = q->sizePolicy().expandingDirections();
        }
        if (exp & Qt::Horizontal)
            s.setWidth(qMax(s.width(), 200));
        if (exp & Qt::Vertical)
            s.setHeight(qMax(s.height(), 100));

        QRect screen = QDesktopWidgetPrivate::screenGeometry(q->pos());

        s.setWidth(qMin(s.width(), screen.width()*2/3));
        s.setHeight(qMin(s.height(), screen.height()*2/3));

        if (QTLWExtra *extra = maybeTopData())
            extra->sizeAdjusted = true;
    }

    if (!s.isValid()) {
        QRect r = q->childrenRect(); // get children rectangle
        if (r.isNull())
            return s;
        s = r.size() + QSize(2 * r.x(), 2 * r.y());
    }

    return s;
}

/*!
    Adjusts the size of the widget to fit its contents.

    This function uses sizeHint() if it is valid, i.e., the size hint's width
    and height are \>= 0. Otherwise, it sets the size to the children
    rectangle that covers all child widgets (the union of all child widget
    rectangles).

    For windows, the screen size is also taken into account. If the sizeHint()
    is less than (200, 100) and the size policy is \l{QSizePolicy::Expanding}
    {expanding}, the window will be at least (200, 100). The maximum size of
    a window is 2/3 of the screen's width and height.

    \sa sizeHint(), childrenRect()
*/

void QWidget::adjustSize()
{
    Q_D(QWidget);
    ensurePolished();
    QSize s = d->adjustedSize();

    if (d->layout)
        d->layout->activate();

    if (s.isValid())
        resize(s);
}


/*!
    \property QWidget::sizeHint
    \brief the recommended size for the widget

    If the value of this property is an invalid size, no size is
    recommended.

    The default implementation of sizeHint() returns an invalid size
    if there is no layout for this widget, and returns the layout's
    preferred size otherwise.

    \sa QSize::isValid(), minimumSizeHint(), sizePolicy(),
    setMinimumSize(), updateGeometry()
*/

QSize QWidget::sizeHint() const
{
    Q_D(const QWidget);
    if (d->layout)
        return d->layout->totalSizeHint();
    return QSize(-1, -1);
}

/*!
    \property QWidget::minimumSizeHint
    \brief the recommended minimum size for the widget

    If the value of this property is an invalid size, no minimum size
    is recommended.

    The default implementation of minimumSizeHint() returns an invalid
    size if there is no layout for this widget, and returns the
    layout's minimum size otherwise. Most built-in widgets reimplement
    minimumSizeHint().

    \l QLayout will never resize a widget to a size smaller than the
    minimum size hint unless minimumSize() is set or the size policy is
    set to QSizePolicy::Ignore. If minimumSize() is set, the minimum
    size hint will be ignored.

    \sa QSize::isValid(), resize(), setMinimumSize(), sizePolicy()
*/
QSize QWidget::minimumSizeHint() const
{
    Q_D(const QWidget);
    if (d->layout)
        return d->layout->totalMinimumSize();
    return QSize(-1, -1);
}


/*!
    \fn QWidget *QWidget::parentWidget() const

    Returns the parent of this widget, or \nullptr if it does not have any
    parent widget.
*/


/*!
    Returns \c true if this widget is a parent, (or grandparent and so on
    to any level), of the given \a child, and both widgets are within
    the same window; otherwise returns \c false.
*/

bool QWidget::isAncestorOf(const QWidget *child) const
{
    while (child) {
        if (child == this)
            return true;
        if (child->isWindow())
            return false;
        child = child->parentWidget();
    }
    return false;
}

/*****************************************************************************
  QWidget event handling
 *****************************************************************************/

/*!
    This is the main event handler; it handles event \a event. You can
    reimplement this function in a subclass, but we recommend using
    one of the specialized event handlers instead.

    Key press and release events are treated differently from other
    events. event() checks for Tab and Shift+Tab and tries to move the
    focus appropriately. If there is no widget to move the focus to
    (or the key press is not Tab or Shift+Tab), event() calls
    keyPressEvent().

    Mouse and tablet event handling is also slightly special: only
    when the widget is \l enabled, event() will call the specialized
    handlers such as mousePressEvent(); otherwise it will discard the
    event.

    This function returns \c true if the event was recognized, otherwise
    it returns \c false.  If the recognized event was accepted (see \l
    QEvent::accepted), any further processing such as event
    propagation to the parent widget stops.

    \sa closeEvent(), focusInEvent(), focusOutEvent(), enterEvent(),
    keyPressEvent(), keyReleaseEvent(), leaveEvent(),
    mouseDoubleClickEvent(), mouseMoveEvent(), mousePressEvent(),
    mouseReleaseEvent(), moveEvent(), paintEvent(), resizeEvent(),
    QObject::event(), QObject::timerEvent()
*/

bool QWidget::event(QEvent *event)
{
    Q_D(QWidget);

    // ignore mouse and key events when disabled
    if (!isEnabled()) {
        switch(event->type()) {
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        case QEvent::TabletMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::TouchCancel:
        case QEvent::ContextMenu:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
#if QT_CONFIG(wheelevent)
        case QEvent::Wheel:
#endif
            return false;
        default:
            break;
        }
    }
    switch (event->type()) {
    case QEvent::PlatformSurface: {
        // Sync up QWidget's view of whether or not the widget has been created
        switch (static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType()) {
        case QPlatformSurfaceEvent::SurfaceCreated:
            if (!testAttribute(Qt::WA_WState_Created))
                create();
            break;
        case QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed:
            if (testAttribute(Qt::WA_WState_Created)) {
                // Child windows have already been destroyed by QWindow,
                // so we skip them here.
                destroy(false, false);
            }
            break;
        }
        break;
    }
    case QEvent::MouseMove:
        mouseMoveEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent((QMouseEvent*)event);
        break;
#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
        wheelEvent((QWheelEvent*)event);
        break;
#endif
#if QT_CONFIG(tabletevent)
    case QEvent::TabletMove:
        if (static_cast<QTabletEvent *>(event)->buttons() == Qt::NoButton && !testAttribute(Qt::WA_TabletTracking))
            break;
        Q_FALLTHROUGH();
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
        tabletEvent((QTabletEvent*)event);
        break;
#endif
    case QEvent::KeyPress: {
        QKeyEvent *k = (QKeyEvent *)event;
        bool res = false;
        if (!(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {  //### Add MetaModifier?
            if (k->key() == Qt::Key_Backtab
                || (k->key() == Qt::Key_Tab && (k->modifiers() & Qt::ShiftModifier)))
                res = focusNextPrevChild(false);
            else if (k->key() == Qt::Key_Tab)
                res = focusNextPrevChild(true);
            if (res)
                break;
        }
        keyPressEvent(k);
#ifdef QT_KEYPAD_NAVIGATION
        if (!k->isAccepted() && QApplication::keypadNavigationEnabled()
            && !(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier))) {
            if (QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder) {
                if (k->key() == Qt::Key_Up)
                    res = focusNextPrevChild(false);
                else if (k->key() == Qt::Key_Down)
                    res = focusNextPrevChild(true);
            } else if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
                if (k->key() == Qt::Key_Up)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionNorth);
                else if (k->key() == Qt::Key_Right)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionEast);
                else if (k->key() == Qt::Key_Down)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionSouth);
                else if (k->key() == Qt::Key_Left)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionWest);
            }
            if (res) {
                k->accept();
                break;
            }
        }
#endif
#if QT_CONFIG(whatsthis)
        if (!k->isAccepted()
            && k->modifiers() & Qt::ShiftModifier && k->key() == Qt::Key_F1
            && d->whatsThis.size()) {
            QWhatsThis::showText(mapToGlobal(inputMethodQuery(Qt::ImCursorRectangle).toRect().center()), d->whatsThis, this);
            k->accept();
        }
#endif
    }
        break;

    case QEvent::KeyRelease:
        keyReleaseEvent((QKeyEvent*)event);
        Q_FALLTHROUGH();
    case QEvent::ShortcutOverride:
        break;

    case QEvent::InputMethod:
        inputMethodEvent((QInputMethodEvent *) event);
        break;

    case QEvent::InputMethodQuery:
        if (testAttribute(Qt::WA_InputMethodEnabled)) {
            QInputMethodQueryEvent *query = static_cast<QInputMethodQueryEvent *>(event);
            Qt::InputMethodQueries queries = query->queries();
            for (uint i = 0; i < 32; ++i) {
                Qt::InputMethodQuery q = (Qt::InputMethodQuery)(int)(queries & (1<<i));
                if (q) {
                    QVariant v = inputMethodQuery(q);
                    if (q == Qt::ImEnabled && !v.isValid() && isEnabled())
                        v = QVariant(true); // special case for Qt4 compatibility
                    query->setValue(q, v);
                }
            }
            query->accept();
        }
        break;

    case QEvent::PolishRequest:
        ensurePolished();
        break;

    case QEvent::Polish: {
        style()->polish(this);
        setAttribute(Qt::WA_WState_Polished);
        if (!QApplication::font(this).isCopyOf(QApplication::font()))
            d->resolveFont();
        if (!QApplication::palette(this).isCopyOf(QGuiApplication::palette()))
            d->resolvePalette();
    }
        break;

    case QEvent::ApplicationWindowIconChange:
        if (isWindow() && !testAttribute(Qt::WA_SetWindowIcon)) {
            d->setWindowIcon_sys();
            d->setWindowIcon_helper();
        }
        break;
    case QEvent::FocusIn:
        focusInEvent((QFocusEvent*)event);
        d->updateWidgetTransform(event);
        break;

    case QEvent::FocusOut:
        focusOutEvent((QFocusEvent*)event);
        break;

    case QEvent::Enter:
#if QT_CONFIG(statustip)
        if (d->statusTip.size()) {
            QStatusTipEvent tip(d->statusTip);
            QCoreApplication::sendEvent(const_cast<QWidget *>(this), &tip);
        }
#endif
        enterEvent(event);
        break;

    case QEvent::Leave:
#if QT_CONFIG(statustip)
        if (d->statusTip.size()) {
            QString empty;
            QStatusTipEvent tip(empty);
            QCoreApplication::sendEvent(const_cast<QWidget *>(this), &tip);
        }
#endif
        leaveEvent(event);
        break;

    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
        update();
        break;

    case QEvent::Paint:
        // At this point the event has to be delivered, regardless
        // whether the widget isVisible() or not because it
        // already went through the filters
        paintEvent((QPaintEvent*)event);
        break;

    case QEvent::Move:
        moveEvent((QMoveEvent*)event);
        d->updateWidgetTransform(event);
        break;

    case QEvent::Resize:
        resizeEvent((QResizeEvent*)event);
        d->updateWidgetTransform(event);
        break;

    case QEvent::Close:
        closeEvent((QCloseEvent *)event);
        break;

#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        switch (data->context_menu_policy) {
        case Qt::PreventContextMenu:
            break;
        case Qt::DefaultContextMenu:
            contextMenuEvent(static_cast<QContextMenuEvent *>(event));
            break;
        case Qt::CustomContextMenu:
            emit customContextMenuRequested(static_cast<QContextMenuEvent *>(event)->pos());
            break;
#if QT_CONFIG(menu)
        case Qt::ActionsContextMenu:
            if (d->actions.count()) {
                QMenu::exec(d->actions, static_cast<QContextMenuEvent *>(event)->globalPos(),
                            nullptr, this);
                break;
            }
            Q_FALLTHROUGH();
#endif
        default:
            event->ignore();
            break;
        }
        break;
#endif // QT_NO_CONTEXTMENU

#if QT_CONFIG(draganddrop)
    case QEvent::Drop:
        dropEvent((QDropEvent*) event);
        break;

    case QEvent::DragEnter:
        dragEnterEvent((QDragEnterEvent*) event);
        break;

    case QEvent::DragMove:
        dragMoveEvent((QDragMoveEvent*) event);
        break;

    case QEvent::DragLeave:
        dragLeaveEvent((QDragLeaveEvent*) event);
        break;
#endif

    case QEvent::Show:
        showEvent((QShowEvent*) event);
        break;

    case QEvent::Hide:
        hideEvent((QHideEvent*) event);
        break;

    case QEvent::ShowWindowRequest:
        if (!isHidden())
            d->show_sys();
        break;

    case QEvent::ApplicationFontChange:
        d->resolveFont();
        break;
    case QEvent::ApplicationPaletteChange:
        if (!(windowType() == Qt::Desktop))
            d->resolvePalette();
        break;

    case QEvent::ToolBarChange:
    case QEvent::ActivationChange:
    case QEvent::EnabledChange:
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::PaletteChange:
    case QEvent::WindowTitleChange:
    case QEvent::IconTextChange:
    case QEvent::ModifiedChange:
    case QEvent::MouseTrackingChange:
    case QEvent::TabletTrackingChange:
    case QEvent::ParentChange:
    case QEvent::LocaleChange:
    case QEvent::MacSizeChange:
    case QEvent::ContentsRectChange:
    case QEvent::ThemeChange:
    case QEvent::ReadOnlyChange:
        changeEvent(event);
        break;

    case QEvent::WindowStateChange: {
        const bool wasMinimized = static_cast<const QWindowStateChangeEvent *>(event)->oldState() & Qt::WindowMinimized;
        if (wasMinimized != isMinimized()) {
            QWidget *widget = const_cast<QWidget *>(this);
            if (wasMinimized) {
                // Always send the spontaneous events here, otherwise it can break the application!
                if (!d->childrenShownByExpose) {
                    // Show widgets only when they are not yet shown by the expose event
                    d->showChildren(true);
                    QShowEvent showEvent;
                    QCoreApplication::sendSpontaneousEvent(widget, &showEvent);
                }
                d->childrenHiddenByWState = false; // Set it always to "false" when window is restored
            } else {
                QHideEvent hideEvent;
                QCoreApplication::sendSpontaneousEvent(widget, &hideEvent);
                d->hideChildren(true);
                d->childrenHiddenByWState = true;
            }
            d->childrenShownByExpose = false; // Set it always to "false" when window state changes
        }
        changeEvent(event);
    }
        break;

    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate: {
        if (isVisible() && !palette().isEqual(QPalette::Active, QPalette::Inactive))
            update();
        QList<QObject*> childList = d->children;
        for (int i = 0; i < childList.size(); ++i) {
            QWidget *w = qobject_cast<QWidget *>(childList.at(i));
            if (w && w->isVisible() && !w->isWindow())
                QCoreApplication::sendEvent(w, event);
        }
        break; }

    case QEvent::LanguageChange:
        changeEvent(event);
        {
            QList<QObject*> childList = d->children;
            for (int i = 0; i < childList.size(); ++i) {
                QObject *o = childList.at(i);
                if (o)
                    QCoreApplication::sendEvent(o, event);
            }
        }
        update();
        break;

    case QEvent::ApplicationLayoutDirectionChange:
        d->resolveLayoutDirection();
        break;

    case QEvent::LayoutDirectionChange:
        if (d->layout)
            d->layout->invalidate();
        update();
        changeEvent(event);
        break;
    case QEvent::UpdateRequest:
        d->syncBackingStore();
        break;
    case QEvent::UpdateLater:
        update(static_cast<QUpdateLaterEvent*>(event)->region());
        break;
    case QEvent::StyleAnimationUpdate:
        if (isVisible() && !window()->isMinimized()) {
            event->accept();
            update();
        }
        break;

    case QEvent::WindowBlocked:
    case QEvent::WindowUnblocked:
        if (!d->children.isEmpty()) {
            QWidget *modalWidget = QApplication::activeModalWidget();
            for (int i = 0; i < d->children.size(); ++i) {
                QObject *o = d->children.at(i);
                if (o && o != modalWidget && o->isWidgetType()) {
                    QWidget *w  = static_cast<QWidget *>(o);
                    // do not forward the event to child windows; QApplication does this for us
                    if (!w->isWindow())
                        QCoreApplication::sendEvent(w, event);
                }
            }
        }
        break;
#ifndef QT_NO_TOOLTIP
    case QEvent::ToolTip:
        if (!d->toolTip.isEmpty())
            QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), d->toolTip, this, QRect(), d->toolTipDuration);
        else
            event->ignore();
        break;
#endif
#if QT_CONFIG(whatsthis)
    case QEvent::WhatsThis:
        if (d->whatsThis.size())
            QWhatsThis::showText(static_cast<QHelpEvent *>(event)->globalPos(), d->whatsThis, this);
        else
            event->ignore();
        break;
    case QEvent::QueryWhatsThis:
        if (d->whatsThis.isEmpty())
            event->ignore();
        break;
#endif
    case QEvent::EmbeddingControl:
        d->topData()->frameStrut.setCoords(0 ,0, 0, 0);
        data->fstrut_dirty = false;
        break;
#ifndef QT_NO_ACTION
    case QEvent::ActionAdded:
    case QEvent::ActionRemoved:
    case QEvent::ActionChanged:
        actionEvent((QActionEvent*)event);
        break;
#endif

    case QEvent::KeyboardLayoutChange:
        {
            changeEvent(event);

            // inform children of the change
            QList<QObject*> childList = d->children;
            for (int i = 0; i < childList.size(); ++i) {
                QWidget *w = qobject_cast<QWidget *>(childList.at(i));
                if (w && w->isVisible() && !w->isWindow())
                    QCoreApplication::sendEvent(w, event);
            }
            break;
        }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
    {
        event->ignore();
        break;
    }
#ifndef QT_NO_GESTURES
    case QEvent::Gesture:
        event->ignore();
        break;
#endif
    case QEvent::ScreenChangeInternal:
        if (const QTLWExtra *te = d->maybeTopData()) {
            const QWindow *win = te->window;
            d->setWinId((win && win->handle()) ? win->handle()->winId() : 0);
        }
        if (d->data.fnt.d->dpi != logicalDpiY())
            d->updateFont(d->data.fnt);
#ifndef QT_NO_OPENGL
        d->renderToTextureReallyDirty = 1;
#endif
        break;
#ifndef QT_NO_PROPERTIES
    case QEvent::DynamicPropertyChange: {
        const QByteArray &propName = static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName();
        if (propName.length() == 13 && !qstrncmp(propName, "_q_customDpi", 12)) {
            uint value = property(propName.constData()).toUInt();
            if (!d->extra)
                d->createExtra();
            const char axis = propName.at(12);
            if (axis == 'X')
                d->extra->customDpiX = value;
            else if (axis == 'Y')
                d->extra->customDpiY = value;
            d->updateFont(d->data.fnt);
        }
        if (windowHandle() && !qstrncmp(propName, "_q_platform_", 12))
            windowHandle()->setProperty(propName, property(propName));
        Q_FALLTHROUGH();
    }
#endif
    default:
        return QObject::event(event);
    }
    return true;
}

/*!
  This event handler can be reimplemented to handle state changes.

  The state being changed in this event can be retrieved through the \a event
  supplied.

  Change events include: QEvent::ToolBarChange,
  QEvent::ActivationChange, QEvent::EnabledChange, QEvent::FontChange,
  QEvent::StyleChange, QEvent::PaletteChange,
  QEvent::WindowTitleChange, QEvent::IconTextChange,
  QEvent::ModifiedChange, QEvent::MouseTrackingChange,
  QEvent::ParentChange, QEvent::WindowStateChange,
  QEvent::LanguageChange, QEvent::LocaleChange,
  QEvent::LayoutDirectionChange, QEvent::ReadOnlyChange.

*/
void QWidget::changeEvent(QEvent * event)
{
    switch(event->type()) {
    case QEvent::EnabledChange: {
        update();
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::State s;
        s.disabled = true;
        QAccessibleStateChangeEvent event(this, s);
        QAccessible::updateAccessibility(&event);
#endif
        break;
    }

    case QEvent::FontChange:
    case QEvent::StyleChange: {
        Q_D(QWidget);
        update();
        updateGeometry();
        if (d->layout)
            d->layout->invalidate();
        break;
    }

    case QEvent::PaletteChange:
        update();
        break;

    case QEvent::ThemeChange:
        if (QGuiApplication::desktopSettingsAware() && windowType() != Qt::Desktop
            && qApp && !QCoreApplication::closingDown()) {
            if (testAttribute(Qt::WA_WState_Polished))
                QApplication::style()->unpolish(this);
            if (testAttribute(Qt::WA_WState_Polished))
                QApplication::style()->polish(this);
            QEvent styleChangedEvent(QEvent::StyleChange);
            QCoreApplication::sendEvent(this, &styleChangedEvent);
            if (isVisible())
                update();
        }
        break;

#ifdef Q_OS_MAC
    case QEvent::MacSizeChange:
        updateGeometry();
        break;
#endif

    default:
        break;
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive mouse move events for the widget.

    If mouse tracking is switched off, mouse move events only occur if
    a mouse button is pressed while the mouse is being moved. If mouse
    tracking is switched on, mouse move events occur even if no mouse
    button is pressed.

    QMouseEvent::pos() reports the position of the mouse cursor,
    relative to this widget. For press and release events, the
    position is usually the same as the position of the last mouse
    move event, but it might be different if the user's hand shakes.
    This is a feature of the underlying window system, not Qt.

    If you want to show a tooltip immediately, while the mouse is
    moving (e.g., to get the mouse coordinates with QMouseEvent::pos()
    and show them as a tooltip), you must first enable mouse tracking
    as described above. Then, to ensure that the tooltip is updated
    immediately, you must call QToolTip::showText() instead of
    setToolTip() in your implementation of mouseMoveEvent().

    \sa setMouseTracking(), mousePressEvent(), mouseReleaseEvent(),
    mouseDoubleClickEvent(), event(), QMouseEvent, {Scribble Example}
*/

void QWidget::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive mouse press events for the widget.

    If you create new widgets in the mousePressEvent() the
    mouseReleaseEvent() may not end up where you expect, depending on
    the underlying window system (or X11 window manager), the widgets'
    location and maybe more.

    The default implementation implements the closing of popup widgets
    when you click outside the window. For other widget types it does
    nothing.

    \sa mouseReleaseEvent(), mouseDoubleClickEvent(),
    mouseMoveEvent(), event(), QMouseEvent, {Scribble Example}
*/

void QWidget::mousePressEvent(QMouseEvent *event)
{
    event->ignore();
    if ((windowType() == Qt::Popup)) {
        event->accept();
        QWidget* w;
        while ((w = QApplication::activePopupWidget()) && w != this){
            w->close();
            if (QApplication::activePopupWidget() == w) // widget does not want to disappear
                w->hide(); // hide at least
        }
        if (!rect().contains(event->pos())){
            close();
        }
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive mouse release events for the widget.

    \sa mousePressEvent(), mouseDoubleClickEvent(),
    mouseMoveEvent(), event(), QMouseEvent, {Scribble Example}
*/

void QWidget::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive mouse double click events for the widget.

    The default implementation calls mousePressEvent().

    \note The widget will also receive mouse press and mouse release
    events in addition to the double click event. And if another widget
    that overlaps this widget disappears in response to press or
    release events, then this widget will only receive the double click
    event. It is up to the developer to ensure that the application
    interprets these events correctly.

    \sa mousePressEvent(), mouseReleaseEvent(), mouseMoveEvent(),
    event(), QMouseEvent
*/

void QWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    mousePressEvent(event);
}

#if QT_CONFIG(wheelevent)
/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive wheel events for the widget.

    If you reimplement this handler, it is very important that you
    \l{QEvent}{ignore()} the event if you do not handle
    it, so that the widget's parent can interpret it.

    The default implementation ignores the event.

    \sa QEvent::ignore(), QEvent::accept(), event(),
    QWheelEvent
*/

void QWidget::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}
#endif // QT_CONFIG(wheelevent)

#if QT_CONFIG(tabletevent)
/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive tablet events for the widget.

    If you reimplement this handler, it is very important that you
    \l{QEvent}{ignore()} the event if you do not handle
    it, so that the widget's parent can interpret it.

    The default implementation ignores the event.

    If tablet tracking is switched off, tablet move events only occur if the
    stylus is in contact with the tablet, or at least one stylus button is
    pressed, while the stylus is being moved. If tablet tracking is switched on,
    tablet move events occur even while the stylus is hovering in proximity of
    the tablet, with no buttons pressed.

    \sa QEvent::ignore(), QEvent::accept(), event(), setTabletTracking(),
    QTabletEvent
*/

void QWidget::tabletEvent(QTabletEvent *event)
{
    event->ignore();
}
#endif // QT_CONFIG(tabletevent)

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive key press events for the widget.

    A widget must call setFocusPolicy() to accept focus initially and
    have focus in order to receive a key press event.

    If you reimplement this handler, it is very important that you
    call the base class implementation if you do not act upon the key.

    The default implementation closes popup widgets if the user
    presses the key sequence for QKeySequence::Cancel (typically the
    Escape key). Otherwise the event is ignored, so that the widget's
    parent can interpret it.

    Note that QKeyEvent starts with isAccepted() == true, so you do not
    need to call QKeyEvent::accept() - just do not call the base class
    implementation if you act upon the key.

    \sa keyReleaseEvent(), setFocusPolicy(),
    focusInEvent(), focusOutEvent(), event(), QKeyEvent, {Tetrix Example}
*/

void QWidget::keyPressEvent(QKeyEvent *event)
{
#ifndef QT_NO_SHORTCUT
    if ((windowType() == Qt::Popup) && event->matches(QKeySequence::Cancel)) {
        event->accept();
        close();
    } else
#endif
    {
        event->ignore();
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive key release events for the widget.

    A widget must \l{setFocusPolicy()}{accept focus}
    initially and \l{hasFocus()}{have focus} in order to
    receive a key release event.

    If you reimplement this handler, it is very important that you
    call the base class implementation if you do not act upon the key.

    The default implementation ignores the event, so that the widget's
    parent can interpret it.

    Note that QKeyEvent starts with isAccepted() == true, so you do not
    need to call QKeyEvent::accept() - just do not call the base class
    implementation if you act upon the key.

    \sa keyPressEvent(), QEvent::ignore(), setFocusPolicy(),
    focusInEvent(), focusOutEvent(), event(), QKeyEvent
*/

void QWidget::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
}

/*!
    \fn void QWidget::focusInEvent(QFocusEvent *event)

    This event handler can be reimplemented in a subclass to receive
    keyboard focus events (focus received) for the widget. The event
    is passed in the \a event parameter

    A widget normally must setFocusPolicy() to something other than
    Qt::NoFocus in order to receive focus events. (Note that the
    application programmer can call setFocus() on any widget, even
    those that do not normally accept focus.)

    The default implementation updates the widget (except for windows
    that do not specify a focusPolicy()).

    \sa focusOutEvent(), setFocusPolicy(), keyPressEvent(),
    keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusInEvent(QFocusEvent *)
{
    if (focusPolicy() != Qt::NoFocus || !isWindow()) {
        update();
    }
}

/*!
    \fn void QWidget::focusOutEvent(QFocusEvent *event)

    This event handler can be reimplemented in a subclass to receive
    keyboard focus events (focus lost) for the widget. The events is
    passed in the \a event parameter.

    A widget normally must setFocusPolicy() to something other than
    Qt::NoFocus in order to receive focus events. (Note that the
    application programmer can call setFocus() on any widget, even
    those that do not normally accept focus.)

    The default implementation updates the widget (except for windows
    that do not specify a focusPolicy()).

    \sa focusInEvent(), setFocusPolicy(), keyPressEvent(),
    keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusOutEvent(QFocusEvent *)
{
    if (focusPolicy() != Qt::NoFocus || !isWindow())
        update();

#if !defined(QT_PLATFORM_UIKIT)
    // FIXME: revisit autoSIP logic, QTBUG-42906
    if (qApp->autoSipEnabled() && testAttribute(Qt::WA_InputMethodEnabled))
        QGuiApplication::inputMethod()->hide();
#endif
}

/*!
    \fn void QWidget::enterEvent(QEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget enter events which are passed in the \a event parameter.

    An event is sent to the widget when the mouse cursor enters the
    widget.

    \sa leaveEvent(), mouseMoveEvent(), event()
*/

void QWidget::enterEvent(QEvent *)
{
}

// ### Qt 6: void QWidget::enterEvent(QEnterEvent *).

/*!
    \fn void QWidget::leaveEvent(QEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget leave events which are passed in the \a event parameter.

    A leave event is sent to the widget when the mouse cursor leaves
    the widget.

    \sa enterEvent(), mouseMoveEvent(), event()
*/

void QWidget::leaveEvent(QEvent *)
{
}

/*!
    \fn void QWidget::paintEvent(QPaintEvent *event)

    This event handler can be reimplemented in a subclass to receive paint
    events passed in \a event.

    A paint event is a request to repaint all or part of a widget. It can
    happen for one of the following reasons:

    \list
        \li repaint() or update() was invoked,
        \li the widget was obscured and has now been uncovered, or
        \li many other reasons.
    \endlist

    Many widgets can simply repaint their entire surface when asked to, but
    some slow widgets need to optimize by painting only the requested region:
    QPaintEvent::region(). This speed optimization does not change the result,
    as painting is clipped to that region during event processing. QListView
    and QTableView do this, for example.

    Qt also tries to speed up painting by merging multiple paint events into
    one. When update() is called several times or the window system sends
    several paint events, Qt merges these events into one event with a larger
    region (see QRegion::united()). The repaint() function does not permit this
    optimization, so we suggest using update() whenever possible.

    When the paint event occurs, the update region has normally been erased, so
    you are painting on the widget's background.

    The background can be set using setBackgroundRole() and setPalette().

    Since Qt 4.0, QWidget automatically double-buffers its painting, so there
    is no need to write double-buffering code in paintEvent() to avoid flicker.

    \note Generally, you should refrain from calling update() or repaint()
    \b{inside} a paintEvent(). For example, calling update() or repaint() on
    children inside a paintEvent() results in undefined behavior; the child may
    or may not get a paint event.

    \warning If you are using a custom paint engine without Qt's backingstore,
    Qt::WA_PaintOnScreen must be set. Otherwise, QWidget::paintEngine() will
    never be called; the backingstore will be used instead.

    \sa event(), repaint(), update(), QPainter, QPixmap, QPaintEvent,
    {Analog Clock Example}
*/

void QWidget::paintEvent(QPaintEvent *)
{
}


/*!
    \fn void QWidget::moveEvent(QMoveEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget move events which are passed in the \a event parameter.
    When the widget receives this event, it is already at the new
    position.

    The old position is accessible through QMoveEvent::oldPos().

    \sa resizeEvent(), event(), move(), QMoveEvent
*/

void QWidget::moveEvent(QMoveEvent *)
{
}


/*!
    This event handler can be reimplemented in a subclass to receive
    widget resize events which are passed in the \a event parameter.
    When resizeEvent() is called, the widget already has its new
    geometry. The old size is accessible through
    QResizeEvent::oldSize().

    The widget will be erased and receive a paint event immediately
    after processing the resize event. No drawing need be (or should
    be) done inside this handler.


    \sa moveEvent(), event(), resize(), QResizeEvent, paintEvent(),
        {Scribble Example}
*/

void QWidget::resizeEvent(QResizeEvent * /* event */)
{
}

#ifndef QT_NO_ACTION
/*!
    \fn void QWidget::actionEvent(QActionEvent *event)

    This event handler is called with the given \a event whenever the
    widget's actions are changed.

    \sa addAction(), insertAction(), removeAction(), actions(), QActionEvent
*/
void QWidget::actionEvent(QActionEvent *)
{

}
#endif

/*!
    This event handler is called with the given \a event when Qt receives a window
    close request for a top-level widget from the window system.

    By default, the event is accepted and the widget is closed. You can reimplement
    this function to change the way the widget responds to window close requests.
    For example, you can prevent the window from closing by calling \l{QEvent::}{ignore()}
    on all events.

    Main window applications typically use reimplementations of this function to check
    whether the user's work has been saved and ask for permission before closing.
    For example, the \l{Application Example} uses a helper function to determine whether
    or not to close the window:

    \snippet mainwindows/application/mainwindow.cpp 3
    \snippet mainwindows/application/mainwindow.cpp 4

    \sa event(), hide(), close(), QCloseEvent, {Application Example}
*/

void QWidget::closeEvent(QCloseEvent *event)
{
    event->accept();
}

#ifndef QT_NO_CONTEXTMENU
/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive widget context menu events.

    The handler is called when the widget's \l contextMenuPolicy is
    Qt::DefaultContextMenu.

    The default implementation ignores the context event.
    See the \l QContextMenuEvent documentation for more details.

    \sa event(), QContextMenuEvent, customContextMenuRequested()
*/

void QWidget::contextMenuEvent(QContextMenuEvent *event)
{
    event->ignore();
}
#endif // QT_NO_CONTEXTMENU


/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive Input Method composition events. This handler
    is called when the state of the input method changes.

    Note that when creating custom text editing widgets, the
    Qt::WA_InputMethodEnabled window attribute must be set explicitly
    (using the setAttribute() function) in order to receive input
    method events.

    The default implementation calls event->ignore(), which rejects the
    Input Method event. See the \l QInputMethodEvent documentation for more
    details.

    \sa event(), QInputMethodEvent
*/
void QWidget::inputMethodEvent(QInputMethodEvent *event)
{
    event->ignore();
}

/*!
    This method is only relevant for input widgets. It is used by the
    input method to query a set of properties of the widget to be
    able to support complex input method operations as support for
    surrounding text and reconversions.

    \a query specifies which property is queried.

    \sa inputMethodEvent(), QInputMethodEvent, QInputMethodQueryEvent, inputMethodHints
*/
QVariant QWidget::inputMethodQuery(Qt::InputMethodQuery query) const
{
    switch(query) {
    case Qt::ImCursorRectangle:
        return QRect(width()/2, 0, 1, height());
    case Qt::ImFont:
        return font();
    case Qt::ImAnchorPosition:
        // Fallback.
        return inputMethodQuery(Qt::ImCursorPosition);
    case Qt::ImHints:
        return (int)inputMethodHints();
    case Qt::ImInputItemClipRectangle:
        return d_func()->clipRect();
    default:
        return QVariant();
    }
}

/*!
    \property QWidget::inputMethodHints
    \brief What input method specific hints the widget has.

    This is only relevant for input widgets. It is used by
    the input method to retrieve hints as to how the input method
    should operate. For example, if the Qt::ImhFormattedNumbersOnly flag
    is set, the input method may change its visual components to reflect
    that only numbers can be entered.

    \warning Some widgets require certain flags in order to work as
    intended. To set a flag, do \c{w->setInputMethodHints(w->inputMethodHints()|f)}
    instead of \c{w->setInputMethodHints(f)}.

    \note The flags are only hints, so the particular input method
          implementation is free to ignore them. If you want to be
          sure that a certain type of characters are entered,
          you should also set a QValidator on the widget.

    The default value is Qt::ImhNone.

    \since 4.6

    \sa inputMethodQuery()
*/
Qt::InputMethodHints QWidget::inputMethodHints() const
{
#ifndef QT_NO_IM
    const QWidgetPrivate *priv = d_func();
    while (priv->inheritsInputMethodHints) {
        priv = priv->q_func()->parentWidget()->d_func();
        Q_ASSERT(priv);
    }
    return priv->imHints;
#else //QT_NO_IM
    return 0;
#endif //QT_NO_IM
}

void QWidget::setInputMethodHints(Qt::InputMethodHints hints)
{
#ifndef QT_NO_IM
    Q_D(QWidget);
    if (d->imHints == hints)
        return;
    d->imHints = hints;
    if (this == QGuiApplication::focusObject())
        QGuiApplication::inputMethod()->update(Qt::ImHints);
#else
    Q_UNUSED(hints);
#endif //QT_NO_IM
}


#if QT_CONFIG(draganddrop)

/*!
    \fn void QWidget::dragEnterEvent(QDragEnterEvent *event)

    This event handler is called when a drag is in progress and the
    mouse enters this widget. The event is passed in the \a event parameter.

    If the event is ignored, the widget won't receive any \l{dragMoveEvent()}{drag
    move events}.

    See the \l{dnd.html}{Drag-and-drop documentation} for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDragEnterEvent
*/
void QWidget::dragEnterEvent(QDragEnterEvent *)
{
}

/*!
    \fn void QWidget::dragMoveEvent(QDragMoveEvent *event)

    This event handler is called if a drag is in progress, and when
    any of the following conditions occur: the cursor enters this widget,
    the cursor moves within this widget, or a modifier key is pressed on
    the keyboard while this widget has the focus. The event is passed
    in the \a event parameter.

    See the \l{dnd.html}{Drag-and-drop documentation} for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDragMoveEvent
*/
void QWidget::dragMoveEvent(QDragMoveEvent *)
{
}

/*!
    \fn void QWidget::dragLeaveEvent(QDragLeaveEvent *event)

    This event handler is called when a drag is in progress and the
    mouse leaves this widget. The event is passed in the \a event
    parameter.

    See the \l{dnd.html}{Drag-and-drop documentation} for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDragLeaveEvent
*/
void QWidget::dragLeaveEvent(QDragLeaveEvent *)
{
}

/*!
    \fn void QWidget::dropEvent(QDropEvent *event)

    This event handler is called when the drag is dropped on this
    widget. The event is passed in the \a event parameter.

    See the \l{dnd.html}{Drag-and-drop documentation} for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDropEvent
*/
void QWidget::dropEvent(QDropEvent *)
{
}

#endif // QT_CONFIG(draganddrop)

/*!
    \fn void QWidget::showEvent(QShowEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget show events which are passed in the \a event parameter.

    Non-spontaneous show events are sent to widgets immediately
    before they are shown. The spontaneous show events of windows are
    delivered afterwards.

    Note: A widget receives spontaneous show and hide events when its
    mapping status is changed by the window system, e.g. a spontaneous
    hide event when the user minimizes the window, and a spontaneous
    show event when the window is restored again. After receiving a
    spontaneous hide event, a widget is still considered visible in
    the sense of isVisible().

    \sa visible, event(), QShowEvent
*/
void QWidget::showEvent(QShowEvent *)
{
}

/*!
    \fn void QWidget::hideEvent(QHideEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget hide events. The event is passed in the \a event parameter.

    Hide events are sent to widgets immediately after they have been
    hidden.

    Note: A widget receives spontaneous show and hide events when its
    mapping status is changed by the window system, e.g. a spontaneous
    hide event when the user minimizes the window, and a spontaneous
    show event when the window is restored again. After receiving a
    spontaneous hide event, a widget is still considered visible in
    the sense of isVisible().

    \sa visible, event(), QHideEvent
*/
void QWidget::hideEvent(QHideEvent *)
{
}

/*!
    This special event handler can be reimplemented in a subclass to
    receive native platform events identified by \a eventType
    which are passed in the \a message parameter.

    In your reimplementation of this function, if you want to stop the
    event being handled by Qt, return true and set \a result. The \a result
    parameter has meaning only on Windows. If you return false, this native
    event is passed back to Qt, which translates the event into a Qt event
    and sends it to the widget.

    \note Events are only delivered to this event handler if the widget
    has a native window handle.

    \note This function superseedes the event filter functions
    x11Event(), winEvent() and macEvent() of Qt 4.

    \sa QAbstractNativeEventFilter

    \table
    \header \li Platform \li Event Type Identifier \li Message Type \li Result Type
    \row \li Windows \li "windows_generic_MSG" \li MSG * \li LRESULT
    \row \li macOS \li "NSEvent" \li NSEvent * \li
    \row \li XCB \li "xcb_generic_event_t" \li xcb_generic_event_t * \li
    \endtable
*/

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool QWidget::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#else
bool QWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}

/*!
    Ensures that the widget and its children have been polished by
    QStyle (i.e., have a proper font and palette).

    QWidget calls this function after it has been fully constructed
    but before it is shown the very first time. You can call this
    function if you want to ensure that the widget is polished before
    doing an operation, e.g., the correct font size might be needed in
    the widget's sizeHint() reimplementation. Note that this function
    \e is called from the default implementation of sizeHint().

    Polishing is useful for final initialization that must happen after
    all constructors (from base classes as well as from subclasses)
    have been called.

    If you need to change some settings when a widget is polished,
    reimplement event() and handle the QEvent::Polish event type.

    \b{Note:} The function is declared const so that it can be called from
    other const functions (e.g., sizeHint()).

    \sa event()
*/
void QWidget::ensurePolished() const
{
    Q_D(const QWidget);

    const QMetaObject *m = metaObject();
    if (m == d->polished)
        return;
    d->polished = m;

    QEvent e(QEvent::Polish);
    QCoreApplication::sendEvent(const_cast<QWidget *>(this), &e);

    // polish children after 'this'
    QList<QObject*> children = d->children;
    for (int i = 0; i < children.size(); ++i) {
        QObject *o = children.at(i);
        if(!o->isWidgetType())
            continue;
        if (QWidget *w = qobject_cast<QWidget *>(o))
            w->ensurePolished();
    }

    if (d->parent && d->sendChildEvents) {
        QChildEvent e(QEvent::ChildPolished, const_cast<QWidget *>(this));
        QCoreApplication::sendEvent(d->parent, &e);
    }
}

/*!
    Returns the mask currently set on a widget. If no mask is set the
    return value will be an empty region.

    \sa setMask(), clearMask(), QRegion::isEmpty(), {Shaped Clock Example}
*/
QRegion QWidget::mask() const
{
    Q_D(const QWidget);
    return d->extra ? d->extra->mask : QRegion();
}

/*!
    Returns the layout manager that is installed on this widget, or \nullptr
    if no layout manager is installed.

    The layout manager sets the geometry of the widget's children
    that have been added to the layout.

    \sa setLayout(), sizePolicy(), {Layout Management}
*/
QLayout *QWidget::layout() const
{
    return d_func()->layout;
}


/*!
    \fn void QWidget::setLayout(QLayout *layout)

    Sets the layout manager for this widget to \a layout.

    If there already is a layout manager installed on this widget,
    QWidget won't let you install another. You must first delete the
    existing layout manager (returned by layout()) before you can
    call setLayout() with the new layout.

    If \a layout is the layout manager on a different widget, setLayout()
    will reparent the layout and make it the layout manager for this widget.

    Example:

    \snippet layouts/layouts.cpp 24

    An alternative to calling this function is to pass this widget to
    the layout's constructor.

    The QWidget will take ownership of \a layout.

    \sa layout(), {Layout Management}
*/

void QWidget::setLayout(QLayout *l)
{
    if (Q_UNLIKELY(!l)) {
        qWarning("QWidget::setLayout: Cannot set layout to 0");
        return;
    }
    if (layout()) {
        if (Q_UNLIKELY(layout() != l))
            qWarning("QWidget::setLayout: Attempting to set QLayout \"%s\" on %s \"%s\", which already has a"
                     " layout", l->objectName().toLocal8Bit().data(), metaObject()->className(),
                     objectName().toLocal8Bit().data());
        return;
    }

    QObject *oldParent = l->parent();
    if (oldParent && oldParent != this) {
        if (oldParent->isWidgetType()) {
            // Steal the layout off a widget parent. Takes effect when
            // morphing laid-out container widgets in Designer.
            QWidget *oldParentWidget = static_cast<QWidget *>(oldParent);
            oldParentWidget->takeLayout();
        } else {
            qWarning("QWidget::setLayout: Attempting to set QLayout \"%s\" on %s \"%s\", when the QLayout already has a parent",
                     l->objectName().toLocal8Bit().data(), metaObject()->className(),
                     objectName().toLocal8Bit().data());
            return;
        }
    }

    Q_D(QWidget);
    l->d_func()->topLevel = true;
    d->layout = l;
    if (oldParent != this) {
        l->setParent(this);
        l->d_func()->reparentChildWidgets(this);
        l->invalidate();
    }

    if (isWindow() && d->maybeTopData())
        d->topData()->sizeAdjusted = false;
}

/*!
    \fn QLayout *QWidget::takeLayout()

    Remove the layout from the widget.
    \since 4.5
*/

QLayout *QWidget::takeLayout()
{
    Q_D(QWidget);
    QLayout *l =  layout();
    if (!l)
        return nullptr;
    d->layout = nullptr;
    l->setParent(nullptr);
    return l;
}

/*!
    \property QWidget::sizePolicy
    \brief the default layout behavior of the widget

    If there is a QLayout that manages this widget's children, the
    size policy specified by that layout is used. If there is no such
    QLayout, the result of this function is used.

    The default policy is Preferred/Preferred, which means that the
    widget can be freely resized, but prefers to be the size
    sizeHint() returns. Button-like widgets set the size policy to
    specify that they may stretch horizontally, but are fixed
    vertically. The same applies to lineedit controls (such as
    QLineEdit, QSpinBox or an editable QComboBox) and other
    horizontally orientated widgets (such as QProgressBar).
    QToolButton's are normally square, so they allow growth in both
    directions. Widgets that support different directions (such as
    QSlider, QScrollBar or QHeader) specify stretching in the
    respective direction only. Widgets that can provide scroll bars
    (usually subclasses of QScrollArea) tend to specify that they can
    use additional space, and that they can make do with less than
    sizeHint().

    \sa sizeHint(), QLayout, QSizePolicy, updateGeometry()
*/
QSizePolicy QWidget::sizePolicy() const
{
    Q_D(const QWidget);
    return d->size_policy;
}

void QWidget::setSizePolicy(QSizePolicy policy)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_WState_OwnSizePolicy);
    if (policy == d->size_policy)
        return;

    if (d->size_policy.retainSizeWhenHidden() != policy.retainSizeWhenHidden())
        d->retainSizeWhenHiddenChanged = 1;

    d->size_policy = policy;

#if QT_CONFIG(graphicsview)
    if (const auto &extra = d->extra) {
        if (extra->proxyWidget)
            extra->proxyWidget->setSizePolicy(policy);
    }
#endif

    updateGeometry();
    d->retainSizeWhenHiddenChanged = 0;

    if (isWindow() && d->maybeTopData())
        d->topData()->sizeAdjusted = false;
}

/*!
    \fn void QWidget::setSizePolicy(QSizePolicy::Policy horizontal, QSizePolicy::Policy vertical)
    \overload

    Sets the size policy of the widget to \a horizontal and \a
    vertical, with standard stretch and no height-for-width.

    \sa QSizePolicy::QSizePolicy()
*/

/*!
    Returns the preferred height for this widget, given the width \a w.

    If this widget has a layout, the default implementation returns
    the layout's preferred height.  if there is no layout, the default
    implementation returns -1 indicating that the preferred height
    does not depend on the width.
*/

int QWidget::heightForWidth(int w) const
{
    if (layout() && layout()->hasHeightForWidth())
        return layout()->totalHeightForWidth(w);
    return -1;
}


/*!
    \since 5.0

    Returns \c true if the widget's preferred height depends on its width; otherwise returns \c false.
*/
bool QWidget::hasHeightForWidth() const
{
    Q_D(const QWidget);
    return d->layout ? d->layout->hasHeightForWidth() : d->size_policy.hasHeightForWidth();
}

/*!
    \fn QWidget *QWidget::childAt(int x, int y) const

    Returns the visible child widget at the position (\a{x}, \a{y})
    in the widget's coordinate system. If there is no visible child
    widget at the specified position, the function returns \nullptr.
*/

/*!
    \overload

    Returns the visible child widget at point \a p in the widget's own
    coordinate system.
*/

QWidget *QWidget::childAt(const QPoint &p) const
{
    return d_func()->childAt_helper(p, false);
}

QWidget *QWidgetPrivate::childAt_helper(const QPoint &p, bool ignoreChildrenInDestructor) const
{
    if (children.isEmpty())
        return nullptr;

    if (!pointInsideRectAndMask(p))
        return nullptr;
    return childAtRecursiveHelper(p, ignoreChildrenInDestructor);
}

QWidget *QWidgetPrivate::childAtRecursiveHelper(const QPoint &p, bool ignoreChildrenInDestructor) const
{
    for (int i = children.size() - 1; i >= 0; --i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (!child || child->isWindow() || child->isHidden() || child->testAttribute(Qt::WA_TransparentForMouseEvents)
            || (ignoreChildrenInDestructor && child->data->in_destructor)) {
            continue;
        }

        // Map the point 'p' from parent coordinates to child coordinates.
        QPoint childPoint = p;
        childPoint -= child->data->crect.topLeft();

        // Check if the point hits the child.
        if (!child->d_func()->pointInsideRectAndMask(childPoint))
            continue;

        // Do the same for the child's descendants.
        if (QWidget *w = child->d_func()->childAtRecursiveHelper(childPoint, ignoreChildrenInDestructor))
            return w;

        // We have found our target; namely the child at position 'p'.
        return child;
    }
    return nullptr;
}

void QWidgetPrivate::updateGeometry_helper(bool forceUpdate)
{
    Q_Q(QWidget);
    if (widgetItem)
        widgetItem->invalidateSizeCache();
    QWidget *parent;
    if (forceUpdate || !extra || extra->minw != extra->maxw || extra->minh != extra->maxh) {
        const int isHidden = q->isHidden() && !size_policy.retainSizeWhenHidden() && !retainSizeWhenHiddenChanged;

        if (!q->isWindow() && !isHidden && (parent = q->parentWidget())) {
            if (parent->d_func()->layout)
                parent->d_func()->layout->invalidate();
            else if (parent->isVisible())
                QCoreApplication::postEvent(parent, new QEvent(QEvent::LayoutRequest));
        }
    }
}

/*!
    Notifies the layout system that this widget has changed and may
    need to change geometry.

    Call this function if the sizeHint() or sizePolicy() have changed.

    For explicitly hidden widgets, updateGeometry() is a no-op. The
    layout system will be notified as soon as the widget is shown.
*/

void QWidget::updateGeometry()
{
    Q_D(QWidget);
    d->updateGeometry_helper(false);
}

/*! \property QWidget::windowFlags

    Window flags are a combination of a type (e.g. Qt::Dialog) and
    zero or more hints to the window system (e.g.
    Qt::FramelessWindowHint).

    If the widget had type Qt::Widget or Qt::SubWindow and becomes a
    window (Qt::Window, Qt::Dialog, etc.), it is put at position (0,
    0) on the desktop. If the widget is a window and becomes a
    Qt::Widget or Qt::SubWindow, it is put at position (0, 0)
    relative to its parent widget.

    \note This function calls setParent() when changing the flags for
    a window, causing the widget to be hidden. You must call show() to make
    the widget visible again..

    \sa windowType(), setWindowFlag(), {Window Flags Example}
*/
void QWidget::setWindowFlags(Qt::WindowFlags flags)
{
    Q_D(QWidget);
    d->setWindowFlags(flags);
}

/*!
    \since 5.9

    Sets the window flag \a flag on this widget if \a on is true;
    otherwise clears the flag.

    \sa setWindowFlags(), windowFlags(), windowType()
*/
void QWidget::setWindowFlag(Qt::WindowType flag, bool on)
{
    Q_D(QWidget);
    if (on)
        d->setWindowFlags(data->window_flags | flag);
    else
        d->setWindowFlags(data->window_flags & ~flag);
}

/*! \internal

    Implemented in QWidgetPrivate so that QMdiSubWindowPrivate can reimplement it.
*/
void QWidgetPrivate::setWindowFlags(Qt::WindowFlags flags)
{
    Q_Q(QWidget);
    if (q->data->window_flags == flags)
        return;

    if ((q->data->window_flags | flags) & Qt::Window) {
        // the old type was a window and/or the new type is a window
        QPoint oldPos = q->pos();
        bool visible = q->isVisible();
        const bool windowFlagChanged = (q->data->window_flags ^ flags) & Qt::Window;
        q->setParent(q->parentWidget(), flags);

        // if both types are windows or neither of them are, we restore
        // the old position
        if (!windowFlagChanged && (visible || q->testAttribute(Qt::WA_Moved)))
            q->move(oldPos);
        // for backward-compatibility we change Qt::WA_QuitOnClose attribute value only when the window was recreated.
        adjustQuitOnCloseAttribute();
    } else {
        q->data->window_flags = flags;
    }
}

/*!
    Sets the window flags for the widget to \a flags,
    \e without telling the window system.

    \warning Do not call this function unless you really know what
    you're doing.

    \sa setWindowFlags()
*/
void QWidget::overrideWindowFlags(Qt::WindowFlags flags)
{
    data->window_flags = flags;
}

/*!
    \fn Qt::WindowType QWidget::windowType() const

    Returns the window type of this widget. This is identical to
    windowFlags() & Qt::WindowType_Mask.

    \sa windowFlags
*/

/*!
    Sets the parent of the widget to \a parent, and resets the window
    flags. The widget is moved to position (0, 0) in its new parent.

    If the new parent widget is in a different window, the
    reparented widget and its children are appended to the end of the
    \l{setFocusPolicy()}{tab chain} of the new parent
    widget, in the same internal order as before. If one of the moved
    widgets had keyboard focus, setParent() calls clearFocus() for that
    widget.

    If the new parent widget is in the same window as the
    old parent, setting the parent doesn't change the tab order or
    keyboard focus.

    If the "new" parent widget is the old parent widget, this function
    does nothing.

    \note The widget becomes invisible as part of changing its parent,
    even if it was previously visible. You must call show() to make the
    widget visible again.

    \warning It is very unlikely that you will ever need this
    function. If you have a widget that changes its content
    dynamically, it is far easier to use \l QStackedWidget.

    \sa setWindowFlags()
*/
void QWidget::setParent(QWidget *parent)
{
    if (parent == parentWidget())
        return;
    setParent((QWidget*)parent, windowFlags() & ~Qt::WindowType_Mask);
}

#ifndef QT_NO_OPENGL
static void sendWindowChangeToTextureChildrenRecursively(QWidget *widget)
{
    QWidgetPrivate *d = QWidgetPrivate::get(widget);
    if (d->renderToTexture) {
        QEvent e(QEvent::WindowChangeInternal);
        QCoreApplication::sendEvent(widget, &e);
    }

    for (int i = 0; i < d->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
        if (w && !w->isWindow() && QWidgetPrivate::get(w)->textureChildSeen)
            sendWindowChangeToTextureChildrenRecursively(w);
    }
}
#endif

/*!
    \overload

    This function also takes widget flags, \a f as an argument.
*/

void QWidget::setParent(QWidget *parent, Qt::WindowFlags f)
{
    Q_D(QWidget);
    Q_ASSERT_X(this != parent, Q_FUNC_INFO, "Cannot parent a QWidget to itself");
#ifdef QT_DEBUG
    const auto checkForParentChildLoops = qScopeGuard([&](){
        int depth = 0;
        auto p = parentWidget();
        while (p) {
            if (++depth == QObjectPrivate::CheckForParentChildLoopsWarnDepth) {
                qWarning("QWidget %p (class: '%s', object name: '%s') may have a loop in its parent-child chain; "
                         "this is undefined behavior",
                         this, metaObject()->className(), qPrintable(objectName()));
            }
            p = p->parentWidget();
        }
    });
#endif

    bool resized = testAttribute(Qt::WA_Resized);
    bool wasCreated = testAttribute(Qt::WA_WState_Created);
    QWidget *oldtlw = window();

    if (f & Qt::Window) // Frame geometry likely changes, refresh.
        d->data.fstrut_dirty = true;

    QWidget *desktopWidget = nullptr;
    if (parent && parent->windowType() == Qt::Desktop)
        desktopWidget = parent;
    bool newParent = (parent != parentWidget()) || !wasCreated || desktopWidget;

    if (newParent && parent && !desktopWidget) {
        if (testAttribute(Qt::WA_NativeWindow) && !QCoreApplication::testAttribute(Qt::AA_DontCreateNativeWidgetSiblings))
            parent->d_func()->enforceNativeChildren();
        else if (parent->d_func()->nativeChildrenForced() || parent->testAttribute(Qt::WA_PaintOnScreen))
            setAttribute(Qt::WA_NativeWindow);
    }

    if (wasCreated) {
        if (!testAttribute(Qt::WA_WState_Hidden)) {
            hide();
            setAttribute(Qt::WA_WState_ExplicitShowHide, false);
        }
        if (newParent) {
            QEvent e(QEvent::ParentAboutToChange);
            QCoreApplication::sendEvent(this, &e);
        }
    }
    if (newParent && isAncestorOf(focusWidget()))
        focusWidget()->clearFocus();

    d->setParent_sys(parent, f);

    if (desktopWidget)
        parent = nullptr;

#ifndef QT_NO_OPENGL
    if (d->textureChildSeen && parent) {
        // set the textureChildSeen flag up the whole parent chain
        QWidgetPrivate::get(parent)->setTextureChildSeen();
    }
#endif

    if (QWidgetRepaintManager *oldPaintManager = oldtlw->d_func()->maybeRepaintManager()) {
        if (newParent)
            oldPaintManager->removeDirtyWidget(this);
        // Move the widget and all its static children from
        // the old backing store to the new one.
        oldPaintManager->moveStaticWidgets(this);
    }

    // ### fixme: Qt 6: Remove AA_ImmediateWidgetCreation.
    if (QApplicationPrivate::testAttribute(Qt::AA_ImmediateWidgetCreation) && !testAttribute(Qt::WA_WState_Created))
        create();

    d->reparentFocusWidgets(oldtlw);
    setAttribute(Qt::WA_Resized, resized);

    const bool useStyleSheetPropagationInWidgetStyles =
        QCoreApplication::testAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);

    if (!useStyleSheetPropagationInWidgetStyles && !testAttribute(Qt::WA_StyleSheet)
        && (!parent || !parent->testAttribute(Qt::WA_StyleSheet))) {
        // if the parent has a font set or inherited, then propagate the mask to the new child
        if (parent) {
            const auto pd = parent->d_func();
            d->inheritedFontResolveMask = pd->directFontResolveMask | pd->inheritedFontResolveMask;
            d->inheritedPaletteResolveMask = pd->directPaletteResolveMask | pd->inheritedPaletteResolveMask;
        }
        d->resolveFont();
        d->resolvePalette();
    }
    d->resolveLayoutDirection();
    d->resolveLocale();

    // Note: GL widgets under WGL or EGL will always need a ParentChange
    // event to handle recreation/rebinding of the GL context, hence the
    // (f & Qt::MSWindowsOwnDC) clause (which is set on QGLWidgets on all
    // platforms).
    if (newParent
#if defined(QT_OPENGL_ES)
        || (f & Qt::MSWindowsOwnDC)
#endif
        ) {
        // propagate enabled updates enabled state to non-windows
        if (!isWindow()) {
            if (!testAttribute(Qt::WA_ForceDisabled))
                d->setEnabled_helper(parent ? parent->isEnabled() : true);
            if (!testAttribute(Qt::WA_ForceUpdatesDisabled))
                d->setUpdatesEnabled_helper(parent ? parent->updatesEnabled() : true);
        }
        d->inheritStyle();

        // send and post remaining QObject events
        if (parent && d->sendChildEvents) {
            QChildEvent e(QEvent::ChildAdded, this);
            QCoreApplication::sendEvent(parent, &e);
        }

        if (parent && d->sendChildEvents && d->polished) {
            QChildEvent e(QEvent::ChildPolished, this);
            QCoreApplication::sendEvent(parent, &e);
        }

        QEvent e(QEvent::ParentChange);
        QCoreApplication::sendEvent(this, &e);
    }
#ifndef QT_NO_OPENGL
    //renderToTexture widgets also need to know when their top-level window changes
    if (d->textureChildSeen && oldtlw != window()) {
        sendWindowChangeToTextureChildrenRecursively(this);
    }
#endif

    if (!wasCreated) {
        if (isWindow() || parentWidget()->isVisible())
            setAttribute(Qt::WA_WState_Hidden, true);
        else if (!testAttribute(Qt::WA_WState_ExplicitShowHide))
            setAttribute(Qt::WA_WState_Hidden, false);
    }

    d->updateIsOpaque();

#if QT_CONFIG(graphicsview)
    // Embed the widget into a proxy if the parent is embedded.
    // ### Doesn't handle reparenting out of an embedded widget.
    if (oldtlw->graphicsProxyWidget()) {
        if (QGraphicsProxyWidget *ancestorProxy = d->nearestGraphicsProxyWidget(oldtlw))
            ancestorProxy->d_func()->unembedSubWindow(this);
    }
    if (isWindow() && parent && !graphicsProxyWidget() && !bypassGraphicsProxyWidget(this)) {
        if (QGraphicsProxyWidget *ancestorProxy = d->nearestGraphicsProxyWidget(parent))
            ancestorProxy->d_func()->embedSubWindow(this);
    }
#endif

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasChanged(this);
}

void QWidgetPrivate::setParent_sys(QWidget *newparent, Qt::WindowFlags f)
{
    Q_Q(QWidget);

    Qt::WindowFlags oldFlags = data.window_flags;
    bool wasCreated = q->testAttribute(Qt::WA_WState_Created);

    int targetScreen = -1;
    // Handle a request to move the widget to a particular screen
    if (newparent && newparent->windowType() == Qt::Desktop) {
        // make sure the widget is created on the same screen as the
        // programmer specified desktop widget
        const QDesktopScreenWidget *sw = qobject_cast<const QDesktopScreenWidget *>(newparent);
        targetScreen = sw ? sw->screenNumber() : 0;
        newparent = nullptr;
    }

    setWinId(0);

    if (parent != newparent) {
        QObjectPrivate::setParent_helper(newparent); //### why does this have to be done in the _sys function???
        if (q->windowHandle()) {
            q->windowHandle()->setFlags(f);
            QWidget *parentWithWindow =
                newparent ? (newparent->windowHandle() ? newparent : newparent->nativeParentWidget()) : nullptr;
            if (parentWithWindow) {
                QWidget *topLevel = parentWithWindow->window();
                if ((f & Qt::Window) && topLevel && topLevel->windowHandle()) {
                    q->windowHandle()->setTransientParent(topLevel->windowHandle());
                    q->windowHandle()->setParent(nullptr);
                } else {
                    q->windowHandle()->setTransientParent(nullptr);
                    q->windowHandle()->setParent(parentWithWindow->windowHandle());
                }
            } else {
                q->windowHandle()->setTransientParent(nullptr);
                q->windowHandle()->setParent(nullptr);
            }
        }
    }

    if (!newparent) {
        f |= Qt::Window;
        if (targetScreen == -1) {
            if (parent)
                targetScreen = QDesktopWidgetPrivate::screenNumber(q->parentWidget()->window());
        }
    }

    bool explicitlyHidden = q->testAttribute(Qt::WA_WState_Hidden) && q->testAttribute(Qt::WA_WState_ExplicitShowHide);

    // Reparenting toplevel to child
    if (wasCreated && !(f & Qt::Window) && (oldFlags & Qt::Window) && !q->testAttribute(Qt::WA_NativeWindow)) {
        if (extra && extra->hasWindowContainer)
            QWindowContainer::toplevelAboutToBeDestroyed(q);

        QWindow *newParentWindow = newparent->windowHandle();
        if (!newParentWindow)
            if (QWidget *npw = newparent->nativeParentWidget())
                newParentWindow = npw->windowHandle();

        Q_FOREACH (QObject *child, q->windowHandle()->children()) {
            QWindow *childWindow = qobject_cast<QWindow *>(child);
            if (!childWindow)
                continue;

            QWidgetWindow *childWW = qobject_cast<QWidgetWindow *>(childWindow);
            QWidget *childWidget = childWW ? childWW->widget() : nullptr;
            if (!childWW || (childWidget && childWidget->testAttribute(Qt::WA_NativeWindow)))
                childWindow->setParent(newParentWindow);
        }
        q->destroy();
    }

    adjustFlags(f, q);
    data.window_flags = f;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);

    if (newparent && wasCreated && (q->testAttribute(Qt::WA_NativeWindow) || (f & Qt::Window)))
        q->createWinId();

    if (q->isWindow() || (!newparent || newparent->isVisible()) || explicitlyHidden)
        q->setAttribute(Qt::WA_WState_Hidden);
    q->setAttribute(Qt::WA_WState_ExplicitShowHide, explicitlyHidden);

    // move the window to the selected screen
    if (!newparent && targetScreen != -1) {
        // only if it is already created
        if (q->testAttribute(Qt::WA_WState_Created))
            q->windowHandle()->setScreen(QGuiApplication::screens().value(targetScreen, 0));
        else
            topData()->initialScreenIndex = targetScreen;
    }
}

/*!
    Scrolls the widget including its children \a dx pixels to the
    right and \a dy downward. Both \a dx and \a dy may be negative.

    After scrolling, the widgets will receive paint events for
    the areas that need to be repainted. For widgets that Qt knows to
    be opaque, this is only the newly exposed parts.
    For example, if an opaque widget is scrolled 8 pixels to the left,
    only an 8-pixel wide stripe at the right edge needs updating.

    Since widgets propagate the contents of their parents by default,
    you need to set the \l autoFillBackground property, or use
    setAttribute() to set the Qt::WA_OpaquePaintEvent attribute, to make
    a widget opaque.

    For widgets that use contents propagation, a scroll will cause an
    update of the entire scroll area.

    \sa {Transparency and Double Buffering}
*/

void QWidget::scroll(int dx, int dy)
{
    if ((!updatesEnabled() && children().size() == 0) || !isVisible())
        return;
    if (dx == 0 && dy == 0)
        return;
    Q_D(QWidget);
#if QT_CONFIG(graphicsview)
    if (QGraphicsProxyWidget *proxy = QWidgetPrivate::nearestGraphicsProxyWidget(this)) {
        // Graphics View maintains its own dirty region as a list of rects;
        // until we can connect item updates directly to the view, we must
        // separately add a translated dirty region.
        for (const QRect &rect : d->dirty)
            proxy->update(rect.translated(dx, dy));
        proxy->scroll(dx, dy, proxy->subWidgetRect(this));
        return;
    }
#endif
    d->setDirtyOpaqueRegion();
    d->scroll_sys(dx, dy);
}

void QWidgetPrivate::scroll_sys(int dx, int dy)
{
    Q_Q(QWidget);
    scrollChildren(dx, dy);
    scrollRect(q->rect(), dx, dy);
}

/*!
    \overload

    This version only scrolls \a r and does not move the children of
    the widget.

    If \a r is empty or invalid, the result is undefined.

    \sa QScrollArea
*/
void QWidget::scroll(int dx, int dy, const QRect &r)
{

    if ((!updatesEnabled() && children().size() == 0) || !isVisible())
        return;
    if (dx == 0 && dy == 0)
        return;
    Q_D(QWidget);
#if QT_CONFIG(graphicsview)
    if (QGraphicsProxyWidget *proxy = QWidgetPrivate::nearestGraphicsProxyWidget(this)) {
        // Graphics View maintains its own dirty region as a list of rects;
        // until we can connect item updates directly to the view, we must
        // separately add a translated dirty region.
        if (!d->dirty.isEmpty()) {
            for (const QRect &rect : d->dirty.translated(dx, dy) & r)
                proxy->update(rect);
        }
        proxy->scroll(dx, dy, r.translated(proxy->subWidgetRect(this).topLeft().toPoint()));
        return;
    }
#endif
    d->scroll_sys(dx, dy, r);
}

void QWidgetPrivate::scroll_sys(int dx, int dy, const QRect &r)
{
    scrollRect(r, dx, dy);
}

/*!
    Repaints the widget directly by calling paintEvent() immediately,
    unless updates are disabled or the widget is hidden.

    We suggest only using repaint() if you need an immediate repaint,
    for example during animation. In almost all circumstances update()
    is better, as it permits Qt to optimize for speed and minimize
    flicker.

    \warning If you call repaint() in a function which may itself be
    called from paintEvent(), you may get infinite recursion. The
    update() function never causes recursion.

    \sa update(), paintEvent(), setUpdatesEnabled()
*/

void QWidget::repaint()
{
    repaint(rect());
}

/*! \overload

    This version repaints a rectangle (\a x, \a y, \a w, \a h) inside
    the widget.

    If \a w is negative, it is replaced with \c{width() - x}, and if
    \a h is negative, it is replaced width \c{height() - y}.
*/
void QWidget::repaint(int x, int y, int w, int h)
{
    if (x > data->crect.width() || y > data->crect.height())
        return;

    if (w < 0)
        w = data->crect.width()  - x;
    if (h < 0)
        h = data->crect.height() - y;

    repaint(QRect(x, y, w, h));
}

/*! \overload

    This version repaints a rectangle \a rect inside the widget.
*/
void QWidget::repaint(const QRect &rect)
{
    Q_D(QWidget);
    d->repaint(rect);
}

/*!
    \overload

    This version repaints a region \a rgn inside the widget.
*/
void QWidget::repaint(const QRegion &rgn)
{
    Q_D(QWidget);
    d->repaint(rgn);
}

template <typename T>
void QWidgetPrivate::repaint(T r)
{
    Q_Q(QWidget);

    if (!q->isVisible() || !q->updatesEnabled() || r.isEmpty())
        return;

    QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
    if (tlwExtra && tlwExtra->backingStore)
        tlwExtra->repaintManager->markDirty(r, q, QWidgetRepaintManager::UpdateNow);
}

/*!
    Updates the widget unless updates are disabled or the widget is
    hidden.

    This function does not cause an immediate repaint; instead it
    schedules a paint event for processing when Qt returns to the main
    event loop. This permits Qt to optimize for more speed and less
    flicker than a call to repaint() does.

    Calling update() several times normally results in just one
    paintEvent() call.

    Qt normally erases the widget's area before the paintEvent() call.
    If the Qt::WA_OpaquePaintEvent widget attribute is set, the widget is
    responsible for painting all its pixels with an opaque color.

    \sa repaint(), paintEvent(), setUpdatesEnabled(), {Analog Clock Example}
*/
void QWidget::update()
{
    update(rect());
}

/*! \fn void QWidget::update(int x, int y, int w, int h)
    \overload

    This version updates a rectangle (\a x, \a y, \a w, \a h) inside
    the widget.
*/

/*!
    \overload

    This version updates a rectangle \a rect inside the widget.
*/
void QWidget::update(const QRect &rect)
{
    Q_D(QWidget);
    d->update(rect);
}

/*!
    \overload

    This version repaints a region \a rgn inside the widget.
*/
void QWidget::update(const QRegion &rgn)
{
    Q_D(QWidget);
    d->update(rgn);
}

template <typename T>
void QWidgetPrivate::update(T r)
{
    Q_Q(QWidget);

    if (!q->isVisible() || !q->updatesEnabled())
        return;

    T clipped = r & q->rect();

    if (clipped.isEmpty())
        return;

    if (q->testAttribute(Qt::WA_WState_InPaintEvent)) {
        QCoreApplication::postEvent(q, new QUpdateLaterEvent(clipped));
        return;
    }

    QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
    if (tlwExtra && tlwExtra->backingStore)
        tlwExtra->repaintManager->markDirty(clipped, q);
}

 /*!
  \internal

  This just sets the corresponding attribute bit to 1 or 0
 */
static void setAttribute_internal(Qt::WidgetAttribute attribute, bool on, QWidgetData *data,
                                  QWidgetPrivate *d)
{
    if (attribute < int(8*sizeof(uint))) {
        if (on)
            data->widget_attributes |= (1<<attribute);
        else
            data->widget_attributes &= ~(1<<attribute);
    } else {
        const int x = attribute - 8*sizeof(uint);
        const int int_off = x / (8*sizeof(uint));
        if (on)
            d->high_attributes[int_off] |= (1<<(x-(int_off*8*sizeof(uint))));
        else
            d->high_attributes[int_off] &= ~(1<<(x-(int_off*8*sizeof(uint))));
    }
}

#ifdef Q_OS_MAC
void QWidgetPrivate::macUpdateSizeAttribute()
{
    Q_Q(QWidget);
    QEvent event(QEvent::MacSizeChange);
    QCoreApplication::sendEvent(q, &event);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && (!w->isWindow() || w->testAttribute(Qt::WA_WindowPropagation))
              && !w->testAttribute(Qt::WA_MacMiniSize) // no attribute set? inherit from parent
              && !w->testAttribute(Qt::WA_MacSmallSize)
              && !w->testAttribute(Qt::WA_MacNormalSize))
            w->d_func()->macUpdateSizeAttribute();
    }
    resolveFont();
}
#endif

/*!
    Sets the attribute \a attribute on this widget if \a on is true;
    otherwise clears the attribute.

    \sa testAttribute()
*/
void QWidget::setAttribute(Qt::WidgetAttribute attribute, bool on)
{
    if (testAttribute(attribute) == on)
        return;

    Q_D(QWidget);
    Q_STATIC_ASSERT_X(sizeof(d->high_attributes)*8 >= (Qt::WA_AttributeCount - sizeof(uint)*8),
                      "QWidget::setAttribute(WidgetAttribute, bool): "
                      "QWidgetPrivate::high_attributes[] too small to contain all attributes in WidgetAttribute");
#ifdef Q_OS_WIN
    // ### Don't use PaintOnScreen+paintEngine() to do native painting in some future release
    if (attribute == Qt::WA_PaintOnScreen && on && windowType() != Qt::Desktop && !inherits("QGLWidget")) {
        // see ::paintEngine for details
        paintEngine();
        if (d->noPaintOnScreen)
            return;
    }
#endif

    // Don't set WA_NativeWindow on platforms that don't support it -- except for QGLWidget, which depends on it
    if (attribute == Qt::WA_NativeWindow && !d->mustHaveWindowHandle) {
        QPlatformIntegration *platformIntegration = QGuiApplicationPrivate::platformIntegration();
        if (!platformIntegration->hasCapability(QPlatformIntegration::NativeWidgets))
            return;
    }

    setAttribute_internal(attribute, on, data, d);

    switch (attribute) {

#if QT_CONFIG(draganddrop)
    case Qt::WA_AcceptDrops:  {
        if (on && !testAttribute(Qt::WA_DropSiteRegistered))
            setAttribute(Qt::WA_DropSiteRegistered, true);
        else if (!on && (isWindow() || !parentWidget() || !parentWidget()->testAttribute(Qt::WA_DropSiteRegistered)))
            setAttribute(Qt::WA_DropSiteRegistered, false);
        QEvent e(QEvent::AcceptDropsChange);
        QCoreApplication::sendEvent(this, &e);
        break;
    }
    case Qt::WA_DropSiteRegistered:  {
        for (int i = 0; i < d->children.size(); ++i) {
            QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
            if (w && !w->isWindow() && !w->testAttribute(Qt::WA_AcceptDrops) && w->testAttribute(Qt::WA_DropSiteRegistered) != on)
                w->setAttribute(Qt::WA_DropSiteRegistered, on);
        }
        break;
    }
#endif

    case Qt::WA_NoChildEventsForParent:
        d->sendChildEvents = !on;
        break;
    case Qt::WA_NoChildEventsFromChildren:
        d->receiveChildEvents = !on;
        break;
    case Qt::WA_MacNormalSize:
    case Qt::WA_MacSmallSize:
    case Qt::WA_MacMiniSize:
#ifdef Q_OS_MAC
        {
            // We can only have one of these set at a time
            const Qt::WidgetAttribute MacSizes[] = { Qt::WA_MacNormalSize, Qt::WA_MacSmallSize,
                                                     Qt::WA_MacMiniSize };
            for (int i = 0; i < 3; ++i) {
                if (MacSizes[i] != attribute)
                    setAttribute_internal(MacSizes[i], false, data, d);
            }
            d->macUpdateSizeAttribute();
        }
#endif
        break;
    case Qt::WA_ShowModal:
        if (!on) {
            // reset modality type to NonModal when clearing WA_ShowModal
            data->window_modality = Qt::NonModal;
        } else if (data->window_modality == Qt::NonModal) {
            // determine the modality type if it hasn't been set prior
            // to setting WA_ShowModal. set the default to WindowModal
            // if we are the child of a group leader; otherwise use
            // ApplicationModal.
            QWidget *w = parentWidget();
            if (w)
                w = w->window();
            while (w && !w->testAttribute(Qt::WA_GroupLeader)) {
                w = w->parentWidget();
                if (w)
                    w = w->window();
            }
            data->window_modality = (w && w->testAttribute(Qt::WA_GroupLeader))
                                    ? Qt::WindowModal
                                    : Qt::ApplicationModal;
            // Some window managers do not allow us to enter modality after the
            // window is visible.The window must be hidden before changing the
            // windowModality property and then reshown.
        }
        if (testAttribute(Qt::WA_WState_Created)) {
            // don't call setModal_sys() before create()
            d->setModal_sys();
        }
        break;
    case Qt::WA_MouseTracking: {
        QEvent e(QEvent::MouseTrackingChange);
        QCoreApplication::sendEvent(this, &e);
        break; }
    case Qt::WA_TabletTracking: {
        QEvent e(QEvent::TabletTrackingChange);
        QCoreApplication::sendEvent(this, &e);
        break; }
    case Qt::WA_NativeWindow: {
        d->createTLExtra();
        if (on)
            d->createTLSysExtra();
#ifndef QT_NO_IM
        QWidget *focusWidget = d->effectiveFocusWidget();
        if (on && !internalWinId() && this == QGuiApplication::focusObject()
            && focusWidget->testAttribute(Qt::WA_InputMethodEnabled)) {
            QGuiApplication::inputMethod()->commit();
            QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        }
        if (!QCoreApplication::testAttribute(Qt::AA_DontCreateNativeWidgetSiblings) && parentWidget())
            parentWidget()->d_func()->enforceNativeChildren();
        if (on && !internalWinId() && testAttribute(Qt::WA_WState_Created))
            d->createWinId();
        if (isEnabled() && focusWidget->isEnabled() && this == QGuiApplication::focusObject()
            && focusWidget->testAttribute(Qt::WA_InputMethodEnabled)) {
            QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        }
#endif //QT_NO_IM
        break;
    }
    case Qt::WA_PaintOnScreen:
        d->updateIsOpaque();
        Q_FALLTHROUGH();
    case Qt::WA_OpaquePaintEvent:
        d->updateIsOpaque();
        break;
    case Qt::WA_NoSystemBackground:
        d->updateIsOpaque();
        Q_FALLTHROUGH();
    case Qt::WA_UpdatesDisabled:
        d->updateSystemBackground();
        break;
    case Qt::WA_TransparentForMouseEvents:
        break;
    case Qt::WA_InputMethodEnabled: {
#ifndef QT_NO_IM
        if (QGuiApplication::focusObject() == this) {
            if (!on)
                QGuiApplication::inputMethod()->commit();
            QGuiApplication::inputMethod()->update(Qt::ImEnabled);
        }
#endif //QT_NO_IM
        break;
    }
    case Qt::WA_WindowPropagation:
        d->resolvePalette();
        d->resolveFont();
        d->resolveLocale();
        break;
    case Qt::WA_DontShowOnScreen: {
        if (on && isVisible()) {
            // Make sure we keep the current state and only hide the widget
            // from the desktop. show_sys will only update platform specific
            // attributes at this point.
            d->hide_sys();
            d->show_sys();
        }
        break;
    }

    case Qt::WA_X11NetWmWindowTypeDesktop:
    case Qt::WA_X11NetWmWindowTypeDock:
    case Qt::WA_X11NetWmWindowTypeToolBar:
    case Qt::WA_X11NetWmWindowTypeMenu:
    case Qt::WA_X11NetWmWindowTypeUtility:
    case Qt::WA_X11NetWmWindowTypeSplash:
    case Qt::WA_X11NetWmWindowTypeDialog:
    case Qt::WA_X11NetWmWindowTypeDropDownMenu:
    case Qt::WA_X11NetWmWindowTypePopupMenu:
    case Qt::WA_X11NetWmWindowTypeToolTip:
    case Qt::WA_X11NetWmWindowTypeNotification:
    case Qt::WA_X11NetWmWindowTypeCombo:
    case Qt::WA_X11NetWmWindowTypeDND:
        d->setNetWmWindowTypes();
        break;

    case Qt::WA_StaticContents:
        if (QWidgetRepaintManager *repaintManager = d->maybeRepaintManager()) {
            if (on)
                repaintManager->addStaticWidget(this);
            else
                repaintManager->removeStaticWidget(this);
        }
        break;
    case Qt::WA_TranslucentBackground:
        if (on)
            setAttribute(Qt::WA_NoSystemBackground);
        d->updateIsTranslucent();

        break;
    case Qt::WA_AcceptTouchEvents:
        break;
    default:
        break;
    }
}

/*! \fn bool QWidget::testAttribute(Qt::WidgetAttribute attribute) const

  Returns \c true if attribute \a attribute is set on this widget;
  otherwise returns \c false.

  \sa setAttribute()
 */
bool QWidget::testAttribute_helper(Qt::WidgetAttribute attribute) const
{
    Q_D(const QWidget);
    const int x = attribute - 8*sizeof(uint);
    const int int_off = x / (8*sizeof(uint));
    return (d->high_attributes[int_off] & (1<<(x-(int_off*8*sizeof(uint)))));
}

/*!
  \property QWidget::windowOpacity

  \brief The level of opacity for the window.

  The valid range of opacity is from 1.0 (completely opaque) to
  0.0 (completely transparent).

  By default the value of this property is 1.0.

  This feature is available on Embedded Linux, \macos, Windows,
  and X11 platforms that support the Composite extension.

  \note On X11 you need to have a composite manager running,
  and the X11 specific _NET_WM_WINDOW_OPACITY atom needs to be
  supported by the window manager you are using.

  \warning Changing this property from opaque to transparent might issue a
  paint event that needs to be processed before the window is displayed
  correctly. This affects mainly the use of QScreen::grabWindow(). Also note
  that semi-transparent windows update and resize significantly slower than
  opaque windows.

  \sa setMask()
*/
qreal QWidget::windowOpacity() const
{
    Q_D(const QWidget);
    return (isWindow() && d->maybeTopData()) ? d->maybeTopData()->opacity / 255. : 1.0;
}

void QWidget::setWindowOpacity(qreal opacity)
{
    Q_D(QWidget);
    if (!isWindow())
        return;

    opacity = qBound(qreal(0.0), opacity, qreal(1.0));
    QTLWExtra *extra = d->topData();
    extra->opacity = uint(opacity * 255);
    setAttribute(Qt::WA_WState_WindowOpacitySet);
    d->setWindowOpacity_sys(opacity);

    if (!testAttribute(Qt::WA_WState_Created))
        return;

#if QT_CONFIG(graphicsview)
    if (QGraphicsProxyWidget *proxy = graphicsProxyWidget()) {
        // Avoid invalidating the cache if set.
        if (proxy->cacheMode() == QGraphicsItem::NoCache)
            proxy->update();
        else if (QGraphicsScene *scene = proxy->scene())
            scene->update(proxy->sceneBoundingRect());
        return;
    }
#endif
}

void QWidgetPrivate::setWindowOpacity_sys(qreal level)
{
    Q_Q(QWidget);
    if (q->windowHandle())
        q->windowHandle()->setOpacity(level);
}

/*!
    \property QWidget::windowModified
    \brief whether the document shown in the window has unsaved changes

    A modified window is a window whose content has changed but has
    not been saved to disk. This flag will have different effects
    varied by the platform. On \macos the close button will have a
    modified look; on other platforms, the window title will have an
    '*' (asterisk).

    The window title must contain a "[*]" placeholder, which
    indicates where the '*' should appear. Normally, it should appear
    right after the file name (e.g., "document1.txt[*] - Text
    Editor"). If the window isn't modified, the placeholder is simply
    removed.

    Note that if a widget is set as modified, all its ancestors will
    also be set as modified. However, if you call \c
    {setWindowModified(false)} on a widget, this will not propagate to
    its parent because other children of the parent might have been
    modified.

    \sa windowTitle, {Application Example}, {SDI Example}, {MDI Example}
*/
bool QWidget::isWindowModified() const
{
    return testAttribute(Qt::WA_WindowModified);
}

void QWidget::setWindowModified(bool mod)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_WindowModified, mod);

    d->setWindowModified_helper();

    QEvent e(QEvent::ModifiedChange);
    QCoreApplication::sendEvent(this, &e);
}

void QWidgetPrivate::setWindowModified_helper()
{
    Q_Q(QWidget);
    QWindow *window = q->windowHandle();
    if (!window)
        return;
    QPlatformWindow *platformWindow = window->handle();
    if (!platformWindow)
        return;
    bool on = q->testAttribute(Qt::WA_WindowModified);
    if (!platformWindow->setWindowModified(on)) {
        if (Q_UNLIKELY(on && !q->windowTitle().contains(QLatin1String("[*]"))))
            qWarning("QWidget::setWindowModified: The window title does not contain a '[*]' placeholder");
        setWindowTitle_helper(q->windowTitle());
        setWindowIconText_helper(q->windowIconText());
    }
}

#ifndef QT_NO_TOOLTIP
/*!
  \property QWidget::toolTip

  \brief the widget's tooltip

  Note that by default tooltips are only shown for widgets that are
  children of the active window. You can change this behavior by
  setting the attribute Qt::WA_AlwaysShowToolTips on the \e window,
  not on the widget with the tooltip.

  If you want to control a tooltip's behavior, you can intercept the
  event() function and catch the QEvent::ToolTip event (e.g., if you
  want to customize the area for which the tooltip should be shown).

  By default, this property contains an empty string.

  \sa QToolTip, statusTip, whatsThis
*/
void QWidget::setToolTip(const QString &s)
{
    Q_D(QWidget);
    d->toolTip = s;

    QEvent event(QEvent::ToolTipChange);
    QCoreApplication::sendEvent(this, &event);
}

QString QWidget::toolTip() const
{
    Q_D(const QWidget);
    return d->toolTip;
}

/*!
  \property QWidget::toolTipDuration
  \brief the widget's tooltip duration
  \since 5.2

  Specifies how long time the tooltip will be displayed, in milliseconds.
  If the value is -1 (default) the duration is calculated depending on the length of the tooltip.

  \sa toolTip
*/

void QWidget::setToolTipDuration(int msec)
{
    Q_D(QWidget);
    d->toolTipDuration = msec;
}

int QWidget::toolTipDuration() const
{
    Q_D(const QWidget);
    return d->toolTipDuration;
}

#endif // QT_NO_TOOLTIP


#if QT_CONFIG(statustip)
/*!
  \property QWidget::statusTip
  \brief the widget's status tip

  By default, this property contains an empty string.

  \sa toolTip, whatsThis
*/
void QWidget::setStatusTip(const QString &s)
{
    Q_D(QWidget);
    d->statusTip = s;
}

QString QWidget::statusTip() const
{
    Q_D(const QWidget);
    return d->statusTip;
}
#endif // QT_CONFIG(statustip)

#if QT_CONFIG(whatsthis)
/*!
  \property QWidget::whatsThis

  \brief the widget's What's This help text.

  By default, this property contains an empty string.

  \sa QWhatsThis, QWidget::toolTip, QWidget::statusTip
*/
void QWidget::setWhatsThis(const QString &s)
{
    Q_D(QWidget);
    d->whatsThis = s;
}

QString QWidget::whatsThis() const
{
    Q_D(const QWidget);
    return d->whatsThis;
}
#endif // QT_CONFIG(whatsthis)

#ifndef QT_NO_ACCESSIBILITY
/*!
  \property QWidget::accessibleName

  \brief the widget's name as seen by assistive technologies

  This is the primary name by which assistive technology such as screen readers
  announce this widget. For most widgets setting this property is not required.
  For example for QPushButton the button's text will be used.

  It is important to set this property when the widget does not provide any
  text. For example a button that only contains an icon needs to set this
  property to work with screen readers.
  The name should be short and equivalent to the visual information conveyed
  by the widget.

  This property has to be \l{Internationalization with Qt}{localized}.

  By default, this property contains an empty string.

  \sa QWidget::accessibleDescription, QAccessibleInterface::text()
*/
void QWidget::setAccessibleName(const QString &name)
{
    Q_D(QWidget);
    d->accessibleName = name;
    QAccessibleEvent event(this, QAccessible::NameChanged);
    QAccessible::updateAccessibility(&event);
}

QString QWidget::accessibleName() const
{
    Q_D(const QWidget);
    return d->accessibleName;
}

/*!
  \property QWidget::accessibleDescription

  \brief the widget's description as seen by assistive technologies

  The accessible description of a widget should convey what a widget does.
  While the \l accessibleName should be a short and consise string (e.g. \gui{Save}),
  the description should give more context, such as \gui{Saves the current document}.

  This property has to be \l{Internationalization with Qt}{localized}.

  By default, this property contains an empty string and Qt falls back
  to using the tool tip to provide this information.

  \sa QWidget::accessibleName, QAccessibleInterface::text()
*/
void QWidget::setAccessibleDescription(const QString &description)
{
    Q_D(QWidget);
    d->accessibleDescription = description;
    QAccessibleEvent event(this, QAccessible::DescriptionChanged);
    QAccessible::updateAccessibility(&event);
}

QString QWidget::accessibleDescription() const
{
    Q_D(const QWidget);
    return d->accessibleDescription;
}
#endif // QT_NO_ACCESSIBILITY

#ifndef QT_NO_SHORTCUT
/*!
    Adds a shortcut to Qt's shortcut system that watches for the given
    \a key sequence in the given \a context. If the \a context is
    Qt::ApplicationShortcut, the shortcut applies to the application as a
    whole. Otherwise, it is either local to this widget, Qt::WidgetShortcut,
    or to the window itself, Qt::WindowShortcut.

    If the same \a key sequence has been grabbed by several widgets,
    when the \a key sequence occurs a QEvent::Shortcut event is sent
    to all the widgets to which it applies in a non-deterministic
    order, but with the ``ambiguous'' flag set to true.

    \warning You should not normally need to use this function;
    instead create \l{QAction}s with the shortcut key sequences you
    require (if you also want equivalent menu options and toolbar
    buttons), or create \l{QShortcut}s if you just need key sequences.
    Both QAction and QShortcut handle all the event filtering for you,
    and provide signals which are triggered when the user triggers the
    key sequence, so are much easier to use than this low-level
    function.

    \sa releaseShortcut(), setShortcutEnabled()
*/
int QWidget::grabShortcut(const QKeySequence &key, Qt::ShortcutContext context)
{
    Q_ASSERT(qApp);
    if (key.isEmpty())
        return 0;
    setAttribute(Qt::WA_GrabbedShortcut);
    return QGuiApplicationPrivate::instance()->shortcutMap.addShortcut(this, key, context, qWidgetShortcutContextMatcher);
}

/*!
    Removes the shortcut with the given \a id from Qt's shortcut
    system. The widget will no longer receive QEvent::Shortcut events
    for the shortcut's key sequence (unless it has other shortcuts
    with the same key sequence).

    \warning You should not normally need to use this function since
    Qt's shortcut system removes shortcuts automatically when their
    parent widget is destroyed. It is best to use QAction or
    QShortcut to handle shortcuts, since they are easier to use than
    this low-level function. Note also that this is an expensive
    operation.

    \sa grabShortcut(), setShortcutEnabled()
*/
void QWidget::releaseShortcut(int id)
{
    Q_ASSERT(qApp);
    if (id)
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(id, this, 0);
}

/*!
    If \a enable is true, the shortcut with the given \a id is
    enabled; otherwise the shortcut is disabled.

    \warning You should not normally need to use this function since
    Qt's shortcut system enables/disables shortcuts automatically as
    widgets become hidden/visible and gain or lose focus. It is best
    to use QAction or QShortcut to handle shortcuts, since they are
    easier to use than this low-level function.

    \sa grabShortcut(), releaseShortcut()
*/
void QWidget::setShortcutEnabled(int id, bool enable)
{
    Q_ASSERT(qApp);
    if (id)
        QGuiApplicationPrivate::instance()->shortcutMap.setShortcutEnabled(enable, id, this, 0);
}

/*!
    \since 4.2

    If \a enable is true, auto repeat of the shortcut with the
    given \a id is enabled; otherwise it is disabled.

    \sa grabShortcut(), releaseShortcut()
*/
void QWidget::setShortcutAutoRepeat(int id, bool enable)
{
    Q_ASSERT(qApp);
    if (id)
        QGuiApplicationPrivate::instance()->shortcutMap.setShortcutAutoRepeat(enable, id, this, 0);
}
#endif // QT_NO_SHORTCUT

/*!
    Updates the widget's micro focus.
*/
void QWidget::updateMicroFocus()
{
    // updating everything since this is currently called for any kind of state change
    if (this == QGuiApplication::focusObject())
        QGuiApplication::inputMethod()->update(Qt::ImQueryAll);
}

/*!
    Raises this widget to the top of the parent widget's stack.

    After this call the widget will be visually in front of any
    overlapping sibling widgets.

    \note When using activateWindow(), you can call this function to
    ensure that the window is stacked on top.

    \sa lower(), stackUnder()
*/

void QWidget::raise()
{
    Q_D(QWidget);
    if (!isWindow()) {
        QWidget *p = parentWidget();
        const int parentChildCount = p->d_func()->children.size();
        if (parentChildCount < 2)
            return;
        const int from = p->d_func()->children.indexOf(this);
        Q_ASSERT(from >= 0);
        // Do nothing if the widget is already in correct stacking order _and_ created.
        if (from != parentChildCount -1)
            p->d_func()->children.move(from, parentChildCount - 1);
        if (!testAttribute(Qt::WA_WState_Created) && p->testAttribute(Qt::WA_WState_Created))
            create();
        else if (from == parentChildCount - 1)
            return;

        QRegion region(rect());
        d->subtractOpaqueSiblings(region);
        d->invalidateBackingStore(region);
    }
    if (testAttribute(Qt::WA_WState_Created))
        d->raise_sys();

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasRaised(this);

    QEvent e(QEvent::ZOrderChange);
    QCoreApplication::sendEvent(this, &e);
}

void QWidgetPrivate::raise_sys()
{
    Q_Q(QWidget);
    if (q->isWindow() || q->testAttribute(Qt::WA_NativeWindow)) {
        q->windowHandle()->raise();
    } else if (renderToTexture) {
        if (QWidget *p = q->parentWidget()) {
            setDirtyOpaqueRegion();
            p->d_func()->invalidateBackingStore(effectiveRectFor(q->geometry()));
        }
    }
}

/*!
    Lowers the widget to the bottom of the parent widget's stack.

    After this call the widget will be visually behind (and therefore
    obscured by) any overlapping sibling widgets.

    \sa raise(), stackUnder()
*/

void QWidget::lower()
{
    Q_D(QWidget);
    if (!isWindow()) {
        QWidget *p = parentWidget();
        const int parentChildCount = p->d_func()->children.size();
        if (parentChildCount < 2)
            return;
        const int from = p->d_func()->children.indexOf(this);
        Q_ASSERT(from >= 0);
        // Do nothing if the widget is already in correct stacking order _and_ created.
        if (from != 0)
            p->d_func()->children.move(from, 0);
        if (!testAttribute(Qt::WA_WState_Created) && p->testAttribute(Qt::WA_WState_Created))
            create();
        else if (from == 0)
            return;
    }
    if (testAttribute(Qt::WA_WState_Created))
        d->lower_sys();

    if (d->extra && d->extra->hasWindowContainer)
        QWindowContainer::parentWasLowered(this);

    QEvent e(QEvent::ZOrderChange);
    QCoreApplication::sendEvent(this, &e);
}

void QWidgetPrivate::lower_sys()
{
    Q_Q(QWidget);
    if (q->isWindow() || q->testAttribute(Qt::WA_NativeWindow)) {
        Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
        q->windowHandle()->lower();
    } else if (QWidget *p = q->parentWidget()) {
        setDirtyOpaqueRegion();
        p->d_func()->invalidateBackingStore(effectiveRectFor(q->geometry()));
    }
}

/*!
    Places the widget under \a w in the parent widget's stack.

    To make this work, the widget itself and \a w must be siblings.

    \sa raise(), lower()
*/
void QWidget::stackUnder(QWidget* w)
{
    Q_D(QWidget);
    QWidget *p = parentWidget();
    if (!w || isWindow() || p != w->parentWidget() || this == w)
        return;
    if (p) {
        int from = p->d_func()->children.indexOf(this);
        int to = p->d_func()->children.indexOf(w);
        Q_ASSERT(from >= 0);
        Q_ASSERT(to >= 0);
        if (from < to)
            --to;
        // Do nothing if the widget is already in correct stacking order _and_ created.
        if (from != to)
            p->d_func()->children.move(from, to);
        if (!testAttribute(Qt::WA_WState_Created) && p->testAttribute(Qt::WA_WState_Created))
            create();
        else if (from == to)
            return;
    }
    if (testAttribute(Qt::WA_WState_Created))
        d->stackUnder_sys(w);

    QEvent e(QEvent::ZOrderChange);
    QCoreApplication::sendEvent(this, &e);
}

void QWidgetPrivate::stackUnder_sys(QWidget*)
{
    Q_Q(QWidget);
    if (QWidget *p = q->parentWidget()) {
        setDirtyOpaqueRegion();
        p->d_func()->invalidateBackingStore(effectiveRectFor(q->geometry()));
    }
}

/*!
    \fn bool QWidget::isTopLevel() const
    \obsolete

    Use isWindow() instead.
*/

/*!
    \fn bool QWidget::isRightToLeft() const
    \internal
*/

/*!
    \fn bool QWidget::isLeftToRight() const
    \internal
*/

/*!
     \macro QWIDGETSIZE_MAX
     \relates QWidget

     Defines the maximum size for a QWidget object.

     The largest allowed size for a widget is QSize(QWIDGETSIZE_MAX,
     QWIDGETSIZE_MAX), i.e. QSize (16777215,16777215).

     \sa QWidget::setMaximumSize()
*/

/*!
    \fn QWidget::setupUi(QWidget *widget)

    Sets up the user interface for the specified \a widget.

    \note This function is available with widgets that derive from user
    interface descriptions created using \l{uic}.

    \sa {Using a Designer UI File in Your C++ Application}
*/

QRect QWidgetPrivate::frameStrut() const
{
    Q_Q(const QWidget);
    if (!q->isWindow() || (q->windowType() == Qt::Desktop) || q->testAttribute(Qt::WA_DontShowOnScreen)) {
        // x2 = x1 + w - 1, so w/h = 1
        return QRect(0, 0, 1, 1);
    }

    if (data.fstrut_dirty
        // ### Fix properly for 4.3
        && q->isVisible()
        && q->testAttribute(Qt::WA_WState_Created))
        const_cast<QWidgetPrivate *>(this)->updateFrameStrut();

    return maybeTopData() ? maybeTopData()->frameStrut : QRect();
}

void QWidgetPrivate::updateFrameStrut()
{
    Q_Q(QWidget);
    if (q->data->fstrut_dirty) {
        if (QTLWExtra *te = maybeTopData()) {
            if (te->window && te->window->handle()) {
                const QMargins margins = te->window->frameMargins();
                if (!margins.isNull()) {
                    te->frameStrut.setCoords(margins.left(), margins.top(), margins.right(), margins.bottom());
                    q->data->fstrut_dirty = false;
                }
            }
        }
    }
}

#ifdef QT_KEYPAD_NAVIGATION
/*!
    \internal

    Changes the focus  from the current focusWidget to a widget in
    the \a direction.

    Returns \c true, if there was a widget in that direction
*/
bool QWidgetPrivate::navigateToDirection(Direction direction)
{
    QWidget *targetWidget = widgetInNavigationDirection(direction);
    if (targetWidget)
        targetWidget->setFocus();
    return (targetWidget != 0);
}

/*!
    \internal

    Searches for a widget that is positioned in the \a direction, starting
    from the current focusWidget.

    Returns the pointer to a found widget or \nullptr, if there was no widget
    in that direction.
*/
QWidget *QWidgetPrivate::widgetInNavigationDirection(Direction direction)
{
    const QWidget *sourceWidget = QApplication::focusWidget();
    if (!sourceWidget)
        return nullptr;
    const QRect sourceRect = sourceWidget->rect().translated(sourceWidget->mapToGlobal(QPoint()));
    const int sourceX =
            (direction == DirectionNorth || direction == DirectionSouth) ?
                (sourceRect.left() + (sourceRect.right() - sourceRect.left()) / 2)
                :(direction == DirectionEast ? sourceRect.right() : sourceRect.left());
    const int sourceY =
            (direction == DirectionEast || direction == DirectionWest) ?
                (sourceRect.top() + (sourceRect.bottom() - sourceRect.top()) / 2)
                :(direction == DirectionSouth ? sourceRect.bottom() : sourceRect.top());
    const QPoint sourcePoint(sourceX, sourceY);
    const QPoint sourceCenter = sourceRect.center();
    const QWidget *sourceWindow = sourceWidget->window();

    QWidget *targetWidget = nullptr;
    int shortestDistance = INT_MAX;

    const auto targetCandidates = QApplication::allWidgets();
    for (QWidget *targetCandidate : targetCandidates) {

        const QRect targetCandidateRect = targetCandidate->rect().translated(targetCandidate->mapToGlobal(QPoint()));

        // For focus proxies, the child widget handling the focus can have keypad navigation focus,
        // but the owner of the proxy cannot.
        // Additionally, empty widgets should be ignored.
        if (targetCandidate->focusProxy() || targetCandidateRect.isEmpty())
            continue;

        // Only navigate to a target widget that...
        if (       targetCandidate != sourceWidget
                   // ...takes the focus,
                && targetCandidate->focusPolicy() & Qt::TabFocus
                   // ...is above if DirectionNorth,
                && !(direction == DirectionNorth && targetCandidateRect.bottom() > sourceRect.top())
                   // ...is on the right if DirectionEast,
                && !(direction == DirectionEast  && targetCandidateRect.left()   < sourceRect.right())
                   // ...is below if DirectionSouth,
                && !(direction == DirectionSouth && targetCandidateRect.top()    < sourceRect.bottom())
                   // ...is on the left if DirectionWest,
                && !(direction == DirectionWest  && targetCandidateRect.right()  > sourceRect.left())
                   // ...is enabled,
                && targetCandidate->isEnabled()
                   // ...is visible,
                && targetCandidate->isVisible()
                   // ...is in the same window,
                && targetCandidate->window() == sourceWindow) {
            const int targetCandidateDistance = pointToRect(sourcePoint, targetCandidateRect);
            if (targetCandidateDistance < shortestDistance) {
                shortestDistance = targetCandidateDistance;
                targetWidget = targetCandidate;
            }
        }
    }
    return targetWidget;
}

/*!
    \internal

    Tells us if it there is currently a reachable widget by keypad navigation in
    a certain \a orientation.
    If no navigation is possible, occurring key events in that \a orientation may
    be used to interact with the value in the focused widget, even though it
    currently has not the editFocus.

    \sa QWidgetPrivate::widgetInNavigationDirection(), QWidget::hasEditFocus()
*/
bool QWidgetPrivate::canKeypadNavigate(Qt::Orientation orientation)
{
    return orientation == Qt::Horizontal?
            (QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionEast)
                    || QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionWest))
            :(QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionNorth)
                    || QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionSouth));
}
/*!
    \internal

    Checks, if the \a widget is inside a QTabWidget. If is is inside
    one, left/right key events will be used to switch between tabs in keypad
    navigation. If there is no QTabWidget, the horizontal key events can be used
to
    interact with the value in the focused widget, even though it currently has
    not the editFocus.

    \sa QWidget::hasEditFocus()
*/
bool QWidgetPrivate::inTabWidget(QWidget *widget)
{
    for (QWidget *tabWidget = widget; tabWidget; tabWidget = tabWidget->parentWidget())
        if (qobject_cast<const QTabWidget*>(tabWidget))
            return true;
    return false;
}
#endif

/*!
    \since 5.0
    \internal

    Sets the backing store to be the \a store specified.
    The QWidget will take ownership of the \a store.
*/
void QWidget::setBackingStore(QBackingStore *store)
{
    // ### createWinId() ??

    if (!isTopLevel())
        return;

    Q_D(QWidget);

    QTLWExtra *topData = d->topData();
    if (topData->backingStore == store)
        return;

    QBackingStore *oldStore = topData->backingStore;
    deleteBackingStore(d);
    topData->backingStore = store;

    QWidgetRepaintManager *repaintManager = d->maybeRepaintManager();
    if (!repaintManager)
        return;

    if (isTopLevel()) {
        if (repaintManager->backingStore() != oldStore && repaintManager->backingStore() != store)
            delete repaintManager->backingStore();
        repaintManager->setBackingStore(store);
    }
}

/*!
    \since 5.0

    Returns the QBackingStore this widget will be drawn into.
*/
QBackingStore *QWidget::backingStore() const
{
    Q_D(const QWidget);
    QTLWExtra *extra = d->maybeTopData();
    if (extra && extra->backingStore)
        return extra->backingStore;

    QWidgetRepaintManager *repaintManager = d->maybeRepaintManager();
    return repaintManager ? repaintManager->backingStore() : nullptr;
}

void QWidgetPrivate::getLayoutItemMargins(int *left, int *top, int *right, int *bottom) const
{
    if (left)
        *left = (int)leftLayoutItemMargin;
    if (top)
        *top = (int)topLayoutItemMargin;
    if (right)
        *right = (int)rightLayoutItemMargin;
    if (bottom)
        *bottom = (int)bottomLayoutItemMargin;
}

void QWidgetPrivate::setLayoutItemMargins(int left, int top, int right, int bottom)
{
    if (leftLayoutItemMargin == left
        && topLayoutItemMargin == top
        && rightLayoutItemMargin == right
        && bottomLayoutItemMargin == bottom)
        return;

    Q_Q(QWidget);
    leftLayoutItemMargin = (signed char)left;
    topLayoutItemMargin = (signed char)top;
    rightLayoutItemMargin = (signed char)right;
    bottomLayoutItemMargin = (signed char)bottom;
    q->updateGeometry();
}

void QWidgetPrivate::setLayoutItemMargins(QStyle::SubElement element, const QStyleOption *opt)
{
    Q_Q(QWidget);
    QStyleOption myOpt;
    if (!opt) {
        myOpt.initFrom(q);
        myOpt.rect.setRect(0, 0, 32768, 32768);     // arbitrary
        opt = &myOpt;
    }

    QRect liRect = q->style()->subElementRect(element, opt, q);
    if (liRect.isValid()) {
        leftLayoutItemMargin = (signed char)(opt->rect.left() - liRect.left());
        topLayoutItemMargin = (signed char)(opt->rect.top() - liRect.top());
        rightLayoutItemMargin = (signed char)(liRect.right() - opt->rect.right());
        bottomLayoutItemMargin = (signed char)(liRect.bottom() - opt->rect.bottom());
    } else {
        leftLayoutItemMargin = 0;
        topLayoutItemMargin = 0;
        rightLayoutItemMargin = 0;
        bottomLayoutItemMargin = 0;
    }
}
// resets the Qt::WA_QuitOnClose attribute to the default value for transient widgets.
void QWidgetPrivate::adjustQuitOnCloseAttribute()
{
    Q_Q(QWidget);

    if (!q->parentWidget()) {
        Qt::WindowType type = q->windowType();
        if (type == Qt::Widget || type == Qt::SubWindow)
            type = Qt::Window;
        if (type != Qt::Widget && type != Qt::Window && type != Qt::Dialog)
            q->setAttribute(Qt::WA_QuitOnClose, false);
    }
}

QOpenGLContext *QWidgetPrivate::shareContext() const
{
#ifdef QT_NO_OPENGL
    return 0;
#else
    if (!extra || !extra->topextra || !extra->topextra->window)
        return nullptr;

    if (!extra->topextra->shareContext) {
        auto ctx = qt_make_unique<QOpenGLContext>();
        ctx->setShareContext(qt_gl_global_share_context());
        ctx->setFormat(extra->topextra->window->format());
        ctx->setScreen(extra->topextra->window->screen());
        ctx->create();
        extra->topextra->shareContext = std::move(ctx);
    }
    return extra->topextra->shareContext.get();
#endif // QT_NO_OPENGL
}

#ifndef QT_NO_OPENGL
void QWidgetPrivate::sendComposeStatus(QWidget *w, bool end)
{
    QWidgetPrivate *wd = QWidgetPrivate::get(w);
    if (!wd->textureChildSeen)
        return;
    if (end)
        wd->endCompose();
    else
        wd->beginCompose();
    for (int i = 0; i < wd->children.size(); ++i) {
        w = qobject_cast<QWidget *>(wd->children.at(i));
        if (w && !w->isWindow() && !w->isHidden() && QWidgetPrivate::get(w)->textureChildSeen)
            sendComposeStatus(w, end);
    }
}
#endif // QT_NO_OPENGL

Q_WIDGETS_EXPORT QWidgetData *qt_qwidget_data(QWidget *widget)
{
    return widget->data;
}

Q_WIDGETS_EXPORT QWidgetPrivate *qt_widget_private(QWidget *widget)
{
    return widget->d_func();
}


#if QT_CONFIG(graphicsview)
/*!
   \since 4.5

   Returns the proxy widget for the corresponding embedded widget in a graphics
   view; otherwise returns \nullptr.

   \sa QGraphicsProxyWidget::createProxyForChildWidget(),
       QGraphicsScene::addWidget()
 */
QGraphicsProxyWidget *QWidget::graphicsProxyWidget() const
{
    Q_D(const QWidget);
    if (d->extra) {
        return d->extra->proxyWidget;
    }
    return nullptr;
}
#endif

#ifndef QT_NO_GESTURES
/*!
    Subscribes the widget to a given \a gesture with specific \a flags.

    \sa ungrabGesture(), QGestureEvent
    \since 4.6
*/
void QWidget::grabGesture(Qt::GestureType gesture, Qt::GestureFlags flags)
{
    Q_D(QWidget);
    d->gestureContext.insert(gesture, flags);
    (void)QGestureManager::instance(); // create a gesture manager
}

/*!
    Unsubscribes the widget from a given \a gesture type

    \sa grabGesture(), QGestureEvent
    \since 4.6
*/
void QWidget::ungrabGesture(Qt::GestureType gesture)
{
    // if you modify this function, check the inlined version in ~QWidget, too
    Q_D(QWidget);
    if (d->gestureContext.remove(gesture)) {
        if (QGestureManager *manager = QGestureManager::instance())
            manager->cleanupCachedGestures(this, gesture);
    }
}
#endif // QT_NO_GESTURES

/*!
    \fn void QWidget::destroy(bool destroyWindow, bool destroySubWindows)

    Frees up window system resources. Destroys the widget window if \a
    destroyWindow is true.

    destroy() calls itself recursively for all the child widgets,
    passing \a destroySubWindows for the \a destroyWindow parameter.
    To have more control over destruction of subwidgets, destroy
    subwidgets selectively first.

    This function is usually called from the QWidget destructor.
*/
void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    Q_D(QWidget);

    d->aboutToDestroy();
    if (!isWindow() && parentWidget())
        parentWidget()->d_func()->invalidateBackingStore(d->effectiveRectFor(geometry()));
    d->deactivateWidgetCleanup();

    if ((windowType() == Qt::Popup) && qApp)
        qApp->d_func()->closePopup(this);

    if (this == QApplicationPrivate::active_window)
        QApplication::setActiveWindow(nullptr);
    if (QWidget::mouseGrabber() == this)
        releaseMouse();
    if (QWidget::keyboardGrabber() == this)
        releaseKeyboard();

    setAttribute(Qt::WA_WState_Created, false);

    if (windowType() != Qt::Desktop) {
        if (destroySubWindows) {
            QObjectList childList(children());
            for (int i = 0; i < childList.size(); i++) {
                QWidget *widget = qobject_cast<QWidget *>(childList.at(i));
                if (widget && widget->testAttribute(Qt::WA_NativeWindow)) {
                    if (widget->windowHandle()) {
                        widget->destroy();
                    }
                }
            }
        }
        if (destroyWindow) {
            d->deleteTLSysExtra();
        } else {
            if (parentWidget() && parentWidget()->testAttribute(Qt::WA_WState_Created)) {
                d->hide_sys();
            }
        }

        d->setWinId(0);
    }
}

/*!
    \fn QPaintEngine *QWidget::paintEngine() const

    Returns the widget's paint engine.

    Note that this function should not be called explicitly by the
    user, since it's meant for reimplementation purposes only. The
    function is called by Qt internally, and the default
    implementation may not always return a valid pointer.
*/
QPaintEngine *QWidget::paintEngine() const
{
    qWarning("QWidget::paintEngine: Should no longer be called");

#ifdef Q_OS_WIN
    // We set this bit which is checked in setAttribute for
    // Qt::WA_PaintOnScreen. We do this to allow these two scenarios:
    //
    // 1. Users accidentally set Qt::WA_PaintOnScreen on X and port to
    // Windows which would mean suddenly their widgets stop working.
    //
    // 2. Users set paint on screen and subclass paintEngine() to
    // return 0, in which case we have a "hole" in the backingstore
    // allowing use of GDI or DirectX directly.
    //
    // 1 is WRONG, but to minimize silent failures, we have set this
    // bit to ignore the setAttribute call. 2. needs to be
    // supported because its our only means of embedding native
    // graphics stuff.
    const_cast<QWidgetPrivate *>(d_func())->noPaintOnScreen = 1;
#endif

    return nullptr; //##### @@@
}

// Do not call QWindow::mapToGlobal() until QPlatformWindow is properly showing.
static inline bool canMapPosition(QWindow *window)
{
    return window->handle() && !qt_window_private(window)->resizeEventPending;
}

#if QT_CONFIG(graphicsview)
static inline QGraphicsProxyWidget *graphicsProxyWidget(const QWidget *w)
{
    QGraphicsProxyWidget *result = nullptr;
    const QWidgetPrivate *d = qt_widget_private(const_cast<QWidget *>(w));
    if (d->extra)
        result = d->extra->proxyWidget;
    return result;
}
#endif // QT_CONFIG(graphicsview)

struct MapToGlobalTransformResult {
    QTransform transform;
    QWindow *window;
};

static MapToGlobalTransformResult mapToGlobalTransform(const QWidget *w)
{
    MapToGlobalTransformResult result;
    result.window = nullptr;
    for ( ; w ; w = w->parentWidget()) {
#if QT_CONFIG(graphicsview)
        if (QGraphicsProxyWidget *qgpw = graphicsProxyWidget(w)) {
            if (const QGraphicsScene *scene = qgpw->scene()) {
                const QList <QGraphicsView *> views = scene->views();
                if (!views.isEmpty()) {
                    result.transform *= qgpw->sceneTransform();
                    result.transform *= views.first()->viewportTransform();
                    w = views.first()->viewport();
                }
            }
        }
#endif // QT_CONFIG(graphicsview)
        QWindow *window = w->windowHandle();
        if (window && canMapPosition(window)) {
            result.window = window;
            break;
        }

        const QPoint topLeft = w->geometry().topLeft();
        result.transform.translate(topLeft.x(), topLeft.y());
        if (w->isWindow())
            break;
    }
    return result;
}

/*!
    \fn QPoint QWidget::mapToGlobal(const QPoint &pos) const

    Translates the widget coordinate \a pos to global screen
    coordinates. For example, \c{mapToGlobal(QPoint(0,0))} would give
    the global coordinates of the top-left pixel of the widget.

    \sa mapFromGlobal(), mapTo(), mapToParent()
*/
QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    const MapToGlobalTransformResult t = mapToGlobalTransform(this);
    const QPoint g = t.transform.map(pos);
    return t.window ? t.window->mapToGlobal(g) : g;
}

/*!
    \fn QPoint QWidget::mapFromGlobal(const QPoint &pos) const

    Translates the global screen coordinate \a pos to widget
    coordinates.

    \sa mapToGlobal(), mapFrom(), mapFromParent()
*/
QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
   const MapToGlobalTransformResult t = mapToGlobalTransform(this);
   const QPoint windowLocal = t.window ? t.window->mapFromGlobal(pos) : pos;
   return t.transform.inverted().map(windowLocal);
}

QWidget *qt_pressGrab = nullptr;
QWidget *qt_mouseGrb = nullptr;
static bool mouseGrabWithCursor = false;
static QWidget *keyboardGrb = nullptr;

static inline QWindow *grabberWindow(const QWidget *w)
{
    QWindow *window = w->windowHandle();
    if (!window)
        if (const QWidget *nativeParent = w->nativeParentWidget())
            window = nativeParent->windowHandle();
    return window;
}

#ifndef QT_NO_CURSOR
static void grabMouseForWidget(QWidget *widget, const QCursor *cursor = nullptr)
#else
static void grabMouseForWidget(QWidget *widget)
#endif
{
    if (qt_mouseGrb)
        qt_mouseGrb->releaseMouse();

    mouseGrabWithCursor = false;
    if (QWindow *window = grabberWindow(widget)) {
#ifndef QT_NO_CURSOR
        if (cursor) {
            mouseGrabWithCursor = true;
            QGuiApplication::setOverrideCursor(*cursor);
        }
#endif // !QT_NO_CURSOR
        window->setMouseGrabEnabled(true);
    }

    qt_mouseGrb = widget;
    qt_pressGrab = nullptr;
}

static void releaseMouseGrabOfWidget(QWidget *widget)
{
    if (qt_mouseGrb == widget) {
        if (QWindow *window = grabberWindow(widget)) {
#ifndef QT_NO_CURSOR
            if (mouseGrabWithCursor) {
                QGuiApplication::restoreOverrideCursor();
                mouseGrabWithCursor = false;
            }
#endif // !QT_NO_CURSOR
            window->setMouseGrabEnabled(false);
        }
    }
    qt_mouseGrb = nullptr;
}

/*!
    \fn void QWidget::grabMouse()

    Grabs the mouse input.

    This widget receives all mouse events until releaseMouse() is
    called; other widgets get no mouse events at all. Keyboard
    events are not affected. Use grabKeyboard() if you want to grab
    that.

    \warning Bugs in mouse-grabbing applications very often lock the
    terminal. Use this function with extreme caution, and consider
    using the \c -nograb command line option while debugging.

    It is almost never necessary to grab the mouse when using Qt, as
    Qt grabs and releases it sensibly. In particular, Qt grabs the
    mouse when a mouse button is pressed and keeps it until the last
    button is released.

    \note Only visible widgets can grab mouse input. If isVisible()
    returns \c false for a widget, that widget cannot call grabMouse().

    \note On Windows, grabMouse() only works when the mouse is inside a window
    owned by the process.
    On \macos, grabMouse() only works when the mouse is inside the frame of that widget.

    \sa releaseMouse(), grabKeyboard(), releaseKeyboard()
*/
void QWidget::grabMouse()
{
    grabMouseForWidget(this);
}

/*!
    \fn void QWidget::grabMouse(const QCursor &cursor)
    \overload grabMouse()

    Grabs the mouse input and changes the cursor shape.

    The cursor will assume shape \a cursor (for as long as the mouse
    focus is grabbed) and this widget will be the only one to receive
    mouse events until releaseMouse() is called().

    \warning Grabbing the mouse might lock the terminal.

    \note See the note in QWidget::grabMouse().

    \sa releaseMouse(), grabKeyboard(), releaseKeyboard(), setCursor()
*/
#ifndef QT_NO_CURSOR
void QWidget::grabMouse(const QCursor &cursor)
{
    grabMouseForWidget(this, &cursor);
}
#endif

bool QWidgetPrivate::stealMouseGrab(bool grab)
{
    // This is like a combination of grab/releaseMouse() but with error checking
    // and it has no effect on the result of mouseGrabber().
    Q_Q(QWidget);
    QWindow *window = grabberWindow(q);
    return window ? window->setMouseGrabEnabled(grab) : false;
}

/*!
    \fn void QWidget::releaseMouse()

    Releases the mouse grab.

    \sa grabMouse(), grabKeyboard(), releaseKeyboard()
*/
void QWidget::releaseMouse()
{
    releaseMouseGrabOfWidget(this);
}

/*!
    \fn void QWidget::grabKeyboard()

    Grabs the keyboard input.

    This widget receives all keyboard events until releaseKeyboard()
    is called; other widgets get no keyboard events at all. Mouse
    events are not affected. Use grabMouse() if you want to grab that.

    The focus widget is not affected, except that it doesn't receive
    any keyboard events. setFocus() moves the focus as usual, but the
    new focus widget receives keyboard events only after
    releaseKeyboard() is called.

    If a different widget is currently grabbing keyboard input, that
    widget's grab is released first.

    \sa releaseKeyboard(), grabMouse(), releaseMouse(), focusWidget()
*/
void QWidget::grabKeyboard()
{
    if (keyboardGrb)
        keyboardGrb->releaseKeyboard();
    if (QWindow *window = grabberWindow(this))
        window->setKeyboardGrabEnabled(true);
    keyboardGrb = this;
}

bool QWidgetPrivate::stealKeyboardGrab(bool grab)
{
    // This is like a combination of grab/releaseKeyboard() but with error
    // checking and it has no effect on the result of keyboardGrabber().
    Q_Q(QWidget);
    QWindow *window = grabberWindow(q);
    return window ? window->setKeyboardGrabEnabled(grab) : false;
}

/*!
    \fn void QWidget::releaseKeyboard()

    Releases the keyboard grab.

    \sa grabKeyboard(), grabMouse(), releaseMouse()
*/
void QWidget::releaseKeyboard()
{
    if (keyboardGrb == this) {
        if (QWindow *window = grabberWindow(this))
            window->setKeyboardGrabEnabled(false);
        keyboardGrb = nullptr;
    }
}

/*!
    \fn QWidget *QWidget::mouseGrabber()

    Returns the widget that is currently grabbing the mouse input.

    If no widget in this application is currently grabbing the mouse,
    \nullptr is returned.

    \sa grabMouse(), keyboardGrabber()
*/
QWidget *QWidget::mouseGrabber()
{
    if (qt_mouseGrb)
        return qt_mouseGrb;
    return qt_pressGrab;
}

/*!
    \fn QWidget *QWidget::keyboardGrabber()

    Returns the widget that is currently grabbing the keyboard input.

    If no widget in this application is currently grabbing the
    keyboard, \nullptr is returned.

    \sa grabMouse(), mouseGrabber()
*/
QWidget *QWidget::keyboardGrabber()
{
    return keyboardGrb;
}

/*!
    \fn void QWidget::activateWindow()

    Sets the top-level widget containing this widget to be the active
    window.

    An active window is a visible top-level window that has the
    keyboard input focus.

    This function performs the same operation as clicking the mouse on
    the title bar of a top-level window. On X11, the result depends on
    the Window Manager. If you want to ensure that the window is
    stacked on top as well you should also call raise(). Note that the
    window must be visible, otherwise activateWindow() has no effect.

    On Windows, if you are calling this when the application is not
    currently the active one then it will not make it the active
    window.  It will change the color of the taskbar entry to indicate
    that the window has changed in some way. This is because Microsoft
    does not allow an application to interrupt what the user is currently
    doing in another application.

    \sa isActiveWindow(), window(), show(), QWindowsWindowFunctions::setWindowActivationBehavior()
*/
void QWidget::activateWindow()
{
    QWindow *const wnd = window()->windowHandle();

    if (wnd)
        wnd->requestActivate();
}

/*!

    Internal implementation of the virtual QPaintDevice::metric()
    function.

    \a m is the metric to get.
*/
int QWidget::metric(PaintDeviceMetric m) const
{
    QWindow *topLevelWindow = nullptr;
    QScreen *screen = nullptr;
    if (QWidget *topLevel = window()) {
        topLevelWindow = topLevel->windowHandle();
        if (topLevelWindow)
            screen = topLevelWindow->screen();
    }
    if (!screen && QGuiApplication::primaryScreen())
        screen = QGuiApplication::primaryScreen();

    if (!screen) {
        if (m == PdmDpiX || m == PdmDpiY)
              return 72;
        return QPaintDevice::metric(m);
    }
    int val;
    if (m == PdmWidth) {
        val = data->crect.width();
    } else if (m == PdmWidthMM) {
        val = data->crect.width() * screen->physicalSize().width() / screen->geometry().width();
    } else if (m == PdmHeight) {
        val = data->crect.height();
    } else if (m == PdmHeightMM) {
        val = data->crect.height() * screen->physicalSize().height() / screen->geometry().height();
    } else if (m == PdmDepth) {
        return screen->depth();
    } else if (m == PdmDpiX) {
        for (const QWidget *p = this; p; p = p->parentWidget()) {
            if (p->d_func()->extra && p->d_func()->extra->customDpiX)
                return p->d_func()->extra->customDpiX;
        }
        return qRound(screen->logicalDotsPerInchX());
    } else if (m == PdmDpiY) {
        for (const QWidget *p = this; p; p = p->parentWidget()) {
            if (p->d_func()->extra && p->d_func()->extra->customDpiY)
                return p->d_func()->extra->customDpiY;
        }
        return qRound(screen->logicalDotsPerInchY());
    } else if (m == PdmPhysicalDpiX) {
        return qRound(screen->physicalDotsPerInchX());
    } else if (m == PdmPhysicalDpiY) {
        return qRound(screen->physicalDotsPerInchY());
    } else if (m == PdmDevicePixelRatio) {
        return topLevelWindow ? topLevelWindow->devicePixelRatio() : qApp->devicePixelRatio();
    } else if (m == PdmDevicePixelRatioScaled) {
        return (QPaintDevice::devicePixelRatioFScale() *
                (topLevelWindow ? topLevelWindow->devicePixelRatio() : qApp->devicePixelRatio()));
    } else {
        val = QPaintDevice::metric(m);// XXX
    }
    return val;
}

/*!
    Initializes the \a painter pen, background and font to the same as
    the given widget's. This function is called automatically when the
    painter is opened on a QWidget.
*/
void QWidget::initPainter(QPainter *painter) const
{
    const QPalette &pal = palette();
    painter->d_func()->state->pen = QPen(pal.brush(foregroundRole()), 1);
    painter->d_func()->state->bgBrush = pal.brush(backgroundRole());
    QFont f(font(), const_cast<QWidget *>(this));
    painter->d_func()->state->deviceFont = f;
    painter->d_func()->state->font = f;
}

/*!
    \internal

    Do PaintDevice rendering with the specified \a offset.
*/
QPaintDevice *QWidget::redirected(QPoint *offset) const
{
    return d_func()->redirected(offset);
}

/*!
    \internal

    A painter that is shared among other instances of QPainter.
*/
QPainter *QWidget::sharedPainter() const
{
    // Someone sent a paint event directly to the widget
    if (!d_func()->redirectDev)
        return nullptr;

    QPainter *sp = d_func()->sharedPainter();
    if (!sp || !sp->isActive())
        return nullptr;

    if (sp->paintEngine()->paintDevice() != d_func()->redirectDev)
        return nullptr;

    return sp;
}

/*!
    \fn void QWidget::setMask(const QRegion &region)
    \overload

    Causes only the parts of the widget which overlap \a region to be
    visible. If the region includes pixels outside the rect() of the
    widget, window system controls in that area may or may not be
    visible, depending on the platform.

    Note that this effect can be slow if the region is particularly
    complex.

    \sa windowOpacity
*/
void QWidget::setMask(const QRegion &newMask)
{
    Q_D(QWidget);

    d->createExtra();
    if (newMask == d->extra->mask)
        return;

#ifndef QT_NO_BACKINGSTORE
    const QRegion oldMask(d->extra->mask);
#endif

    d->extra->mask = newMask;
    d->extra->hasMask = !newMask.isEmpty();

    if (!testAttribute(Qt::WA_WState_Created))
        return;

    d->setMask_sys(newMask);

#ifndef QT_NO_BACKINGSTORE
    if (!isVisible())
        return;

    if (!d->extra->hasMask) {
        // Mask was cleared; update newly exposed area.
        QRegion expose(rect());
        expose -= oldMask;
        if (!expose.isEmpty()) {
            d->setDirtyOpaqueRegion();
            update(expose);
        }
        return;
    }

    if (!isWindow()) {
        // Update newly exposed area on the parent widget.
        QRegion parentExpose(rect());
        parentExpose -= newMask;
        if (!parentExpose.isEmpty()) {
            d->setDirtyOpaqueRegion();
            parentExpose.translate(data->crect.topLeft());
            parentWidget()->update(parentExpose);
        }

        // Update newly exposed area on this widget
        if (!oldMask.isEmpty())
            update(newMask - oldMask);
    }
#endif
}

void QWidgetPrivate::setMask_sys(const QRegion &region)
{
    Q_Q(QWidget);
    if (QWindow *window = q->windowHandle())
        window->setMask(region);
}

/*!
    \fn void QWidget::setMask(const QBitmap &bitmap)

    Causes only the pixels of the widget for which \a bitmap has a
    corresponding 1 bit to be visible. If the region includes pixels
    outside the rect() of the widget, window system controls in that
    area may or may not be visible, depending on the platform.

    Note that this effect can be slow if the region is particularly
    complex.

    The following code shows how an image with an alpha channel can be
    used to generate a mask for a widget:

    \snippet widget-mask/main.cpp 0

    The label shown by this code is masked using the image it contains,
    giving the appearance that an irregularly-shaped image is being drawn
    directly onto the screen.

    Masked widgets receive mouse events only on their visible
    portions.

    \sa clearMask(), windowOpacity(), {Shaped Clock Example}
*/
void QWidget::setMask(const QBitmap &bitmap)
{
    setMask(QRegion(bitmap));
}

/*!
    \fn void QWidget::clearMask()

    Removes any mask set by setMask().

    \sa setMask()
*/
void QWidget::clearMask()
{
    Q_D(QWidget);
    if (!d->extra || !d->extra->hasMask)
        return;
    setMask(QRegion());
}

void QWidgetPrivate::setWidgetParentHelper(QObject *widgetAsObject, QObject *newParent)
{
    Q_ASSERT(widgetAsObject->isWidgetType());
    Q_ASSERT(!newParent || newParent->isWidgetType());
    QWidget *widget = static_cast<QWidget*>(widgetAsObject);
    widget->setParent(static_cast<QWidget*>(newParent));
}

void QWidgetPrivate::setNetWmWindowTypes(bool skipIfMissing)
{
    Q_Q(QWidget);

    if (!q->windowHandle())
        return;

    int wmWindowType = 0;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDesktop))
        wmWindowType |= QXcbWindowFunctions::Desktop;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDock))
        wmWindowType |= QXcbWindowFunctions::Dock;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeToolBar))
        wmWindowType |= QXcbWindowFunctions::Toolbar;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeMenu))
        wmWindowType |= QXcbWindowFunctions::Menu;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeUtility))
        wmWindowType |= QXcbWindowFunctions::Utility;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeSplash))
        wmWindowType |= QXcbWindowFunctions::Splash;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDialog))
        wmWindowType |= QXcbWindowFunctions::Dialog;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDropDownMenu))
        wmWindowType |= QXcbWindowFunctions::DropDownMenu;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypePopupMenu))
        wmWindowType |= QXcbWindowFunctions::PopupMenu;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeToolTip))
        wmWindowType |= QXcbWindowFunctions::Tooltip;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeNotification))
        wmWindowType |= QXcbWindowFunctions::Notification;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeCombo))
        wmWindowType |= QXcbWindowFunctions::Combo;
    if (q->testAttribute(Qt::WA_X11NetWmWindowTypeDND))
        wmWindowType |= QXcbWindowFunctions::Dnd;

    if (wmWindowType == 0 && skipIfMissing)
        return;

    QXcbWindowFunctions::setWmWindowType(q->windowHandle(), static_cast<QXcbWindowFunctions::WmWindowType>(wmWindowType));
}

#ifndef QT_NO_DEBUG_STREAM

static inline void formatWidgetAttributes(QDebug debug, const QWidget *widget)
{
    const QMetaObject *qtMo = qt_getEnumMetaObject(Qt::WA_AttributeCount);
    const QMetaEnum me = qtMo->enumerator(qtMo->indexOfEnumerator("WidgetAttribute"));
    debug << ", attributes=[";
    int count = 0;
    for (int a = 0; a < Qt::WA_AttributeCount; ++a) {
        if (widget->testAttribute(static_cast<Qt::WidgetAttribute>(a))) {
            if (count++)
                debug << ',';
            debug << me.valueToKey(a);
        }
    }
    debug << ']';
}

QDebug operator<<(QDebug debug, const QWidget *widget)
{
    const QDebugStateSaver saver(debug);
    debug.nospace();
    if (widget) {
        debug << widget->metaObject()->className() << '(' << (const void *)widget;
        if (!widget->objectName().isEmpty())
            debug << ", name=" << widget->objectName();
        if (debug.verbosity() > 2) {
            const QRect geometry = widget->geometry();
            const QRect frameGeometry = widget->frameGeometry();
            if (widget->isVisible())
                debug << ", visible";
            if (!widget->isEnabled())
                debug << ", disabled";
            debug << ", states=" << widget->windowState()
                << ", type=" << widget->windowType() << ", flags=" <<  widget->windowFlags();
            formatWidgetAttributes(debug, widget);
            if (widget->isWindow())
                debug << ", window";
            debug << ", " << geometry.width() << 'x' << geometry.height()
                << Qt::forcesign << geometry.x() << geometry.y() << Qt::noforcesign;
            if (frameGeometry != geometry) {
                const QMargins margins(geometry.x() - frameGeometry.x(),
                                       geometry.y() - frameGeometry.y(),
                                       frameGeometry.right() - geometry.right(),
                                       frameGeometry.bottom() - geometry.bottom());
                debug << ", margins=" << margins;
            }
            debug << ", devicePixelRatio=" << widget->devicePixelRatioF();
            if (const WId wid = widget->internalWinId())
                debug << ", winId=0x" << Qt::hex << wid << Qt::dec;
        }
        debug << ')';
    } else {
        debug << "QWidget(0x0)";
    }
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#include "moc_qwidget.cpp"

