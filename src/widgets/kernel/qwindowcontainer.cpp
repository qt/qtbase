// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowcontainer_p.h"
#include "qwidget_p.h"
#include "qwidgetwindow_p.h"
#include <QtGui/qwindow.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <QDebug>

#if QT_CONFIG(mdiarea)
#include <QMdiSubWindow>
#endif
#include <QAbstractScrollArea>
#include <QPainter>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QWindowContainerPrivate : public QWidgetPrivate
{
public:
    Q_DECLARE_PUBLIC(QWindowContainer)

    QWindowContainerPrivate()
        : window(nullptr)
        , usesNativeWidgets(false)
    {
    }

    ~QWindowContainerPrivate() { }

    static QWindowContainerPrivate *get(QWidget *w) {
        QWindowContainer *wc = qobject_cast<QWindowContainer *>(w);
        if (wc)
            return wc->d_func();
        return nullptr;
    }

    void updateGeometry() {
        Q_Q(QWindowContainer);
        if (!q->isWindow() && (q->geometry().bottom() <= 0 || q->geometry().right() <= 0))
            /* Qt (e.g. QSplitter) sometimes prefer to hide a widget by *not* calling
               setVisible(false). This is often done by setting its coordinates to a sufficiently
               negative value so that its clipped outside the parent. Since a QWindow is not clipped
               to widgets in general, it needs to be dealt with as a special case.
            */
            window->setGeometry(q->geometry());
        else if (usesNativeWidgets)
            window->setGeometry(q->rect());
        else
            window->setGeometry(QRect(q->mapTo(q->window(), QPoint()), q->size()));
    }

    void updateUsesNativeWidgets()
    {
        if (window->parent() == nullptr)
            return;
        Q_Q(QWindowContainer);
        if (q->testAttribute(Qt::WA_DontCreateNativeAncestors))
            return;
        if (q->internalWinId()) {
            // Allow use native widgets if the window container is already a native widget
            usesNativeWidgets = true;
            return;
        }
        bool nativeWidgetSet = false;
        QWidget *p = q->parentWidget();
        while (p) {
            if (false
#if QT_CONFIG(mdiarea)
                || qobject_cast<QMdiSubWindow *>(p) != 0
#endif
#if QT_CONFIG(scrollarea)
                || qobject_cast<QAbstractScrollArea *>(p) != 0
#endif
                    ) {
                q->winId();
                nativeWidgetSet = true;
                break;
            }
            p = p->parentWidget();
        }
        usesNativeWidgets = nativeWidgetSet;
    }

    void markParentChain() {
        Q_Q(QWindowContainer);
        QWidget *p = q;
        while (p) {
            QWidgetPrivate *d = static_cast<QWidgetPrivate *>(QWidgetPrivate::get(p));
            d->createExtra();
            d->extra->hasWindowContainer = true;
            p = p->parentWidget();
        }
    }

    bool isStillAnOrphan() const {
        return window->parent() == &fakeParent;
    }

    QPointer<QWindow> window;
    QWindow fakeParent;

    uint usesNativeWidgets : 1;
};



/*!
    \fn QWidget *QWidget::createWindowContainer(QWindow *window, QWidget *parent, Qt::WindowFlags flags);

    Creates a QWidget that makes it possible to embed \a window into
    a QWidget-based application.

    The window container is created as a child of \a parent and with
    window flags \a flags.

    Once the window has been embedded into the container, the
    container will control the window's geometry and
    visibility. Explicit calls to QWindow::setGeometry(),
    QWindow::show() or QWindow::hide() on an embedded window is not
    recommended.

    The container takes over ownership of \a window. The window can
    be removed from the window container with a call to
    QWindow::setParent().

    The window container is attached as a native child window to the
    toplevel window it is a child of. When a window container is used
    as a child of a QAbstractScrollArea or QMdiArea, it will
    create a \l {Native Widgets vs Alien Widgets} {native window} for
    every widget in its parent chain to allow for proper stacking and
    clipping in this use case. Creating a native window for the window
    container also allows for proper stacking and clipping. This must
    be done before showing the window container. Applications with
    many native child windows may suffer from performance issues.

    The window container has a number of known limitations:

    \list

    \li Stacking order; The embedded window will stack on top of the
    widget hierarchy as an opaque box. The stacking order of multiple
    overlapping window container instances is undefined.

    \li Rendering Integration; The window container does not interoperate
    with QGraphicsProxyWidget, QWidget::render() or similar functionality.

    \li Focus Handling; It is possible to let the window container
    instance have any focus policy and it will delegate focus to the
    window via a call to QWindow::requestActivate(). However,
    returning to the normal focus chain from the QWindow instance will
    be up to the QWindow instance implementation itself. Also, whether
    QWindow::requestActivate() actually gives the window focus, is
    platform dependent.

    Since 6.8, if embedding a Qt Quick based window, tab presses will
    transition in and out of the embedded QML window, allowing focus to move
    to the next or previous focusable object in the window container chain.

    \li Using many window container instances in a QWidget-based
    application can greatly hurt the overall performance of the
    application.

    \li Since 6.7, if \a window belongs to a widget (that is, \a window
    was received from calling \l windowHandle()), no container will be
    created. Instead, this function will return the widget itself, after
    being reparented to \l parent. Since no container will be created,
    \a flags will be ignored. In other words, if \a window belongs to
    a widget, consider just reparenting that widget to \a parent instead
    of using this function.

    \endlist
 */

QWidget *QWidget::createWindowContainer(QWindow *window, QWidget *parent, Qt::WindowFlags flags)
{
    // Embedding a QWidget in a window container doesn't make sense,
    // and has various issues in practice, so just return the widget
    // itself.
    if (auto *widgetWindow = qobject_cast<QWidgetWindow *>(window)) {
        QWidget *widget = widgetWindow->widget();
        if (flags != Qt::WindowFlags()) {
            qWarning() << window << "refers to a widget:" << widget
                       << "WindowFlags" << flags << "will be ignored.";
        }
        widget->setParent(parent);
        return widget;
    }
    return new QWindowContainer(window, parent, flags);
}

/*!
    \internal
 */

QWindowContainer::QWindowContainer(QWindow *embeddedWindow, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(*new QWindowContainerPrivate, parent, flags)
{
    Q_D(QWindowContainer);
    if (Q_UNLIKELY(!embeddedWindow)) {
        qWarning("QWindowContainer: embedded window cannot be null");
        return;
    }

    d->window = embeddedWindow;
    d->window->installEventFilter(this);

    QString windowName = d->window->objectName();
    if (windowName.isEmpty())
        windowName = QString::fromUtf8(d->window->metaObject()->className());
    d->fakeParent.setObjectName(windowName + "ContainerFakeParent"_L1);

    d->window->setParent(&d->fakeParent);
    d->window->parent()->installEventFilter(this);
    d->window->setFlag(Qt::SubWindow);

    setAcceptDrops(true);

    connect(containedWindow(), &QWindow::minimumHeightChanged, this, &QWindowContainer::updateGeometry);
    connect(containedWindow(), &QWindow::minimumWidthChanged, this, &QWindowContainer::updateGeometry);
}

QWindow *QWindowContainer::containedWindow() const
{
    Q_D(const QWindowContainer);
    return d->window;
}

/*!
    \internal
 */

QWindowContainer::~QWindowContainer()
{
    Q_D(QWindowContainer);

    // Call destroy() explicitly first. The dtor would do this too, but
    // QEvent::PlatformSurface delivery relies on virtuals. Getting
    // SurfaceAboutToBeDestroyed can be essential for OpenGL, Vulkan, etc.
    // QWindow subclasses in particular. Keep these working.
    if (d->window) {
        d->window->removeEventFilter(this);
        d->window->destroy();
    }

    delete d->window;
}

/*!
    \internal
 */

bool QWindowContainer::eventFilter(QObject *o, QEvent *e)
{
    Q_D(QWindowContainer);
    if (!d->window)
        return false;

    if (e->type() == QEvent::ChildRemoved) {
        QChildEvent *ce = static_cast<QChildEvent *>(e);
        if (ce->child() == d->window) {
            o->removeEventFilter(this);
            d->window->removeEventFilter(this);
            d->window = nullptr;
        }
    } else if (e->type() == QEvent::FocusIn) {
        if (o == d->window)
            setFocus(Qt::ActiveWindowFocusReason);
    }
    return false;
}

/*!
    \internal
 */

bool QWindowContainer::event(QEvent *e)
{
    Q_D(QWindowContainer);
    if (!d->window)
        return QWidget::event(e);

    QEvent::Type type = e->type();
    switch (type) {
    // The only thing we are interested in is making sure our sizes stay
    // in sync, so do a catch-all case.
    case QEvent::Resize:
        d->updateGeometry();
        break;
    case QEvent::Move:
        d->updateGeometry();
        break;
    case QEvent::PolishRequest:
        d->updateGeometry();
        break;
    case QEvent::Show:
        d->updateUsesNativeWidgets();
        if (d->isStillAnOrphan()) {
            d->window->parent()->removeEventFilter(this);
            d->window->setParent(d->usesNativeWidgets
                                 ? windowHandle()
                                 : window()->windowHandle());
            d->fakeParent.destroy();
            if (d->window->parent())
                d->window->parent()->installEventFilter(this);
        }
        if (d->window->parent()) {
            d->markParentChain();
            d->window->show();
        }
        break;
    case QEvent::Hide:
        if (d->window->parent())
            d->window->hide();
        break;
    case QEvent::FocusIn:
        if (d->window->parent()) {
            if (QGuiApplication::focusWindow() != d->window) {
                QFocusEvent *event = static_cast<QFocusEvent *>(e);
                const auto reason = event->reason();
                QWindowPrivate::FocusTarget target = QWindowPrivate::FocusTarget::Current;
                if (reason == Qt::TabFocusReason)
                    target = QWindowPrivate::FocusTarget::First;
                else if (reason == Qt::BacktabFocusReason)
                    target = QWindowPrivate::FocusTarget::Last;
                qt_window_private(d->window)->setFocusToTarget(target, reason);
                d->window->requestActivate();
            }
        }
        break;
#if QT_CONFIG(draganddrop)
    case QEvent::Drop:
    case QEvent::DragMove:
    case QEvent::DragLeave:
        QCoreApplication::sendEvent(d->window, e);
        return e->isAccepted();
    case QEvent::DragEnter:
        // Don't reject drag events for the entire widget when one
        // item rejects the drag enter
        QCoreApplication::sendEvent(d->window, e);
        e->accept();
        return true;
#endif

    case QEvent::Paint:
    {
        static bool needsPunch = !QGuiApplicationPrivate::platformIntegration()->hasCapability(
            QPlatformIntegration::TopStackedNativeChildWindows);
        if (needsPunch) {
            QPainter p(this);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.fillRect(rect(), Qt::transparent);
        }
        break;
    }

    default:
        break;
    }

    return QWidget::event(e);
}

QSize QWindowContainer::minimumSizeHint() const
{
    return containedWindow() ? containedWindow()->minimumSize() : QSize(0, 0);
}

typedef void (*qwindowcontainer_traverse_callback)(QWidget *parent);
static void qwindowcontainer_traverse(QWidget *parent, qwindowcontainer_traverse_callback callback)
{
    const QObjectList &children = parent->children();
    for (int i=0; i<children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w) {
            QWidgetPrivate *wd = static_cast<QWidgetPrivate *>(QWidgetPrivate::get(w));
            if (wd->extra && wd->extra->hasWindowContainer)
                callback(w);
        }
    }
}

void QWindowContainer::toplevelAboutToBeDestroyed(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        if (d->window->parent())
            d->window->parent()->removeEventFilter(parent);
        d->window->setParent(&d->fakeParent);
        d->window->parent()->installEventFilter(parent);
    }
    qwindowcontainer_traverse(parent, toplevelAboutToBeDestroyed);
}

void QWindowContainer::parentWasChanged(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        if (d->window->parent()) {
            d->updateUsesNativeWidgets();
            d->markParentChain();
            QWidget *toplevel = d->usesNativeWidgets ? parent : parent->window();
            if (!toplevel->windowHandle()) {
                QWidgetPrivate *tld = static_cast<QWidgetPrivate *>(QWidgetPrivate::get(toplevel));
                tld->createTLExtra();
                tld->createTLSysExtra();
                Q_ASSERT(toplevel->windowHandle());
            }
            d->window->parent()->removeEventFilter(parent);
            d->window->setParent(toplevel->windowHandle());
            toplevel->windowHandle()->installEventFilter(parent);
            d->fakeParent.destroy();
            d->updateGeometry();
        }
    }
    qwindowcontainer_traverse(parent, parentWasChanged);
}

void QWindowContainer::parentWasMoved(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        if (!d->window)
            return;
        else if (d->window->parent())
            d->updateGeometry();
    }
    qwindowcontainer_traverse(parent, parentWasMoved);
}

void QWindowContainer::parentWasRaised(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        if (!d->window)
            return;
        else if (d->window->parent())
            d->window->raise();
    }
    qwindowcontainer_traverse(parent, parentWasRaised);
}

void QWindowContainer::parentWasLowered(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        if (!d->window)
            return;
        else if (d->window->parent())
            d->window->lower();
    }
    qwindowcontainer_traverse(parent, parentWasLowered);
}

QT_END_NAMESPACE

#include "moc_qwindowcontainer_p.cpp"
