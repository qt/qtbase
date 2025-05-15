// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdockwidget.h"

#include <qaction.h>
#include <qapplication.h>
#include <qdrawutil.h>
#include <qevent.h>
#include <qfontmetrics.h>
#include <qproxystyle.h>
#include <qwindow.h>
#include <qscreen.h>
#include <qmainwindow.h>
#include <qstylepainter.h>
#include <qtoolbutton.h>
#include <qdebug.h>

#include <private/qwidgetresizehandler_p.h>
#include <qpa/qplatformtheme.h>

#include <private/qhighdpiscaling_p.h>
#include "qdockwidget_p.h"
#include "qmainwindowlayout_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

extern QString qt_setWindowTitle_helperHelper(const QString&, const QWidget*); // qwidget.cpp

// qmainwindow.cpp
extern QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *window);

static const QMainWindow *mainwindow_from_dock(const QDockWidget *dock)
{
    for (const QWidget *p = dock->parentWidget(); p; p = p->parentWidget()) {
        if (const QMainWindow *window = qobject_cast<const QMainWindow*>(p))
            return window;
    }
    return nullptr;
}

static inline QMainWindowLayout *qt_mainwindow_layout_from_dock(const QDockWidget *dock)
{
    auto mainWindow = mainwindow_from_dock(dock);
    return mainWindow ? qt_mainwindow_layout(mainWindow) : nullptr;
}

static inline bool hasFeature(const QDockWidgetPrivate *priv, QDockWidget::DockWidgetFeature feature)
{ return (priv->features & feature) == feature; }

static inline bool hasFeature(const QDockWidget *dockwidget, QDockWidget::DockWidgetFeature feature)
{ return (dockwidget->features() & feature) == feature; }


/*
    A Dock Window:

    [+] is the float button
    [X] is the close button

    +-------------------------------+
    | Dock Window Title       [+][X]|
    +-------------------------------+
    |                               |
    | place to put the single       |
    | QDockWidget child (this space |
    | does not yet have a name)     |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    +-------------------------------+

*/

/******************************************************************************
** QDockWidgetTitleButton
*/

class QDockWidgetTitleButton : public QAbstractButton
{
    Q_OBJECT

public:
    QDockWidgetTitleButton(QDockWidget *dockWidget);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override
    { return sizeHint(); }

    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

protected:
    bool event(QEvent *event) override;

private:
    QSize dockButtonIconSize() const;

    mutable int m_iconSize = -1;
};

QDockWidgetTitleButton::QDockWidgetTitleButton(QDockWidget *dockWidget)
    : QAbstractButton(dockWidget)
{
    setFocusPolicy(Qt::NoFocus);
}

bool QDockWidgetTitleButton::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::StyleChange:
    case QEvent::DevicePixelRatioChange:
        m_iconSize = -1;
        break;
    default:
        break;
    }
    return QAbstractButton::event(event);
}

QSize QDockWidgetTitleButton::dockButtonIconSize() const
{
    if (m_iconSize < 0) {
        m_iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
        if (style()->styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame, nullptr, this))
            m_iconSize = (m_iconSize * 5) / 8;  // 16 -> 10
    }
    return QSize(m_iconSize, m_iconSize);
}

QSize QDockWidgetTitleButton::sizeHint() const
{
    ensurePolished();

    int size = 2*style()->pixelMetric(QStyle::PM_DockWidgetTitleBarButtonMargin, nullptr, this);
    if (!icon().isNull()) {
        const QSize sz = icon().actualSize(dockButtonIconSize());
        size += qMax(sz.width(), sz.height());
    }

    return QSize(size, size);
}

void QDockWidgetTitleButton::enterEvent(QEnterEvent *event)
{
    if (isEnabled()) update();
    QAbstractButton::enterEvent(event);
}

void QDockWidgetTitleButton::leaveEvent(QEvent *event)
{
    if (isEnabled()) update();
    QAbstractButton::leaveEvent(event);
}

void QDockWidgetTitleButton::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);

    QStyleOptionToolButton opt;
    opt.initFrom(this);
    opt.state |= QStyle::State_AutoRaise;

    if (style()->styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame, nullptr, this)) {
        if (isEnabled() && underMouse() && !isChecked() && !isDown())
            opt.state |= QStyle::State_Raised;
        if (isChecked())
            opt.state |= QStyle::State_On;
        if (isDown())
            opt.state |= QStyle::State_Sunken;
        p.drawPrimitive(QStyle::PE_PanelButtonTool, opt);
    } else if (isDown() || isChecked()) {
        // no frame, but the icon might have explicit pixmaps for QIcon::On
        opt.state |= QStyle::State_On | QStyle::State_Sunken;
    }

    opt.icon = icon();
    opt.subControls = { };
    opt.activeSubControls = { };
    opt.features = QStyleOptionToolButton::None;
    opt.arrowType = Qt::NoArrow;
    opt.iconSize = dockButtonIconSize();
    p.drawComplexControl(QStyle::CC_ToolButton, opt);
}

/******************************************************************************
** QDockWidgetLayout
*/

QDockWidgetLayout::QDockWidgetLayout(QWidget *parent)
    : QLayout(parent), verticalTitleBar(false), item_list(RoleCount, 0)
{
}

QDockWidgetLayout::~QDockWidgetLayout()
{
    qDeleteAll(item_list);
}

/*! \internal
    Returns true if the dock widget managed by this layout should have a native
    window decoration or if Qt needs to draw it.
 */
bool QDockWidgetLayout::nativeWindowDeco() const
{
    bool floating = parentWidget()->isWindow();
#if QT_CONFIG(tabbar)
    if (auto groupWindow =
            qobject_cast<const QDockWidgetGroupWindow *>(parentWidget()->parentWidget()))
        floating = floating || groupWindow->tabLayoutInfo();
#endif
    return nativeWindowDeco(floating);
}

/*! \internal
    Returns true if the window manager can draw natively the windows decoration
    of a dock widget
 */
bool QDockWidgetLayout::wmSupportsNativeWindowDeco()
{
#if defined(Q_OS_ANDROID)
    return false;
#else
    static const bool xcb = !QGuiApplication::platformName().compare("xcb"_L1, Qt::CaseInsensitive);
    static const bool wayland =
            QGuiApplication::platformName().startsWith("wayland"_L1, Qt::CaseInsensitive);
    return !(xcb || wayland);
#endif
}

/*! \internal
   Returns true if the dock widget managed by this layout should have a native
   window decoration or if Qt needs to draw it. The \a floating parameter
   overrides the floating current state of the dock widget.
 */
bool QDockWidgetLayout::nativeWindowDeco(bool floating) const
{
    return wmSupportsNativeWindowDeco() && floating && item_list.at(QDockWidgetLayout::TitleBar) == nullptr;
}


void QDockWidgetLayout::addItem(QLayoutItem*)
{
    qWarning("QDockWidgetLayout::addItem(): please use QDockWidgetLayout::setWidget()");
    return;
}

QLayoutItem *QDockWidgetLayout::itemAt(int index) const
{
    int cnt = 0;
    for (int i = 0; i < item_list.size(); ++i) {
        QLayoutItem *item = item_list.at(i);
        if (item == nullptr)
            continue;
        if (index == cnt++)
            return item;
    }
    return nullptr;
}

QLayoutItem *QDockWidgetLayout::takeAt(int index)
{
    int j = 0;
    for (int i = 0; i < item_list.size(); ++i) {
        QLayoutItem *item = item_list.at(i);
        if (item == nullptr)
            continue;
        if (index == j) {
            item_list[i] = 0;
            invalidate();
            return item;
        }
        ++j;
    }
    return nullptr;
}

int QDockWidgetLayout::count() const
{
    int result = 0;
    for (int i = 0; i < item_list.size(); ++i) {
        if (item_list.at(i))
            ++result;
    }
    return result;
}

QSize QDockWidgetLayout::sizeFromContent(const QSize &content, bool floating) const
{
    QSize result = content;

    if (verticalTitleBar) {
        result.setHeight(qMax(result.height(), minimumTitleWidth()));
        result.setWidth(qMax(content.width(), 0));
    } else {
        result.setHeight(qMax(result.height(), 0));
        result.setWidth(qMax(content.width(), minimumTitleWidth()));
    }

    QDockWidget *w = qobject_cast<QDockWidget*>(parentWidget());
    const bool nativeDeco = nativeWindowDeco(floating);

    int fw = floating && !nativeDeco
            ? w->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, nullptr, w)
            : 0;

    const int th = titleHeight();
    if (!nativeDeco) {
        if (verticalTitleBar)
            result += QSize(th + 2*fw, 2*fw);
        else
            result += QSize(2*fw, th + 2*fw);
    }

    result.setHeight(qMin(result.height(), (int) QWIDGETSIZE_MAX));
    result.setWidth(qMin(result.width(), (int) QWIDGETSIZE_MAX));

    if (content.width() < 0)
        result.setWidth(-1);
    if (content.height() < 0)
        result.setHeight(-1);

    const QMargins margins = w->contentsMargins();
    //we need to subtract the contents margin (it will be added by the caller)
    QSize min = w->minimumSize().shrunkBy(margins);
    QSize max = w->maximumSize().shrunkBy(margins);

    /* A floating dockwidget will automatically get its minimumSize set to the layout's
       minimum size + deco. We're *not* interested in this, we only take minimumSize()
       into account if the user set it herself. Otherwise we end up expanding the result
       of a calculation for a non-floating dock widget to a floating dock widget's
       minimum size + window decorations. */

    uint explicitMin = 0;
    uint explicitMax = 0;
    if (w->d_func()->extra != nullptr) {
        explicitMin = w->d_func()->extra->explicitMinSize;
        explicitMax = w->d_func()->extra->explicitMaxSize;
    }

    if (!(explicitMin & Qt::Horizontal) || min.width() == 0)
        min.setWidth(-1);
    if (!(explicitMin & Qt::Vertical) || min.height() == 0)
        min.setHeight(-1);

    if (!(explicitMax & Qt::Horizontal))
        max.setWidth(QWIDGETSIZE_MAX);
    if (!(explicitMax & Qt::Vertical))
        max.setHeight(QWIDGETSIZE_MAX);

    return result.boundedTo(max).expandedTo(min);
}

QSize QDockWidgetLayout::sizeHint() const
{
    QDockWidget *w = qobject_cast<QDockWidget*>(parentWidget());

    QSize content(-1, -1);
    if (item_list[Content] != 0)
        content = item_list[Content]->sizeHint();

    return sizeFromContent(content, w->isFloating());
}

QSize QDockWidgetLayout::maximumSize() const
{
    if (item_list[Content] != 0) {
        const QSize content = item_list[Content]->maximumSize();
        return sizeFromContent(content, parentWidget()->isWindow());
    } else {
        return parentWidget()->maximumSize();
    }

}

QSize QDockWidgetLayout::minimumSize() const
{
    QDockWidget *w = qobject_cast<QDockWidget*>(parentWidget());

    QSize content(0, 0);
    if (item_list[Content] != 0)
        content = item_list[Content]->minimumSize();

    return sizeFromContent(content, w->isFloating());
}

QWidget *QDockWidgetLayout::widgetForRole(Role r) const
{
    QLayoutItem *item = item_list.at(r);
    return item == nullptr ? nullptr : item->widget();
}

QLayoutItem *QDockWidgetLayout::itemForRole(Role r) const
{
    return item_list.at(r);
}

void QDockWidgetLayout::setWidgetForRole(Role r, QWidget *w)
{
    QWidget *old = widgetForRole(r);
    if (old != nullptr) {
        old->hide();
        removeWidget(old);
    }

    if (w != nullptr) {
        addChildWidget(w);
        item_list[r] = new QWidgetItemV2(w);
        w->show();
    } else {
        item_list[r] = 0;
    }

    invalidate();
}

static inline int pick(bool vertical, const QSize &size)
{
    return vertical ? size.height() : size.width();
}

static inline int perp(bool vertical, const QSize &size)
{
    return vertical ? size.width() : size.height();
}

int QDockWidgetLayout::minimumTitleWidth() const
{
    QDockWidget *q = qobject_cast<QDockWidget*>(parentWidget());

    if (QWidget *title = widgetForRole(TitleBar))
        return pick(verticalTitleBar, title->minimumSizeHint());

    QSize closeSize(0, 0);
    QSize floatSize(0, 0);
    if (hasFeature(q, QDockWidget::DockWidgetClosable)) {
        if (QLayoutItem *item = item_list[CloseButton])
            closeSize = item->widget()->sizeHint();
    }
    if (hasFeature(q, QDockWidget::DockWidgetFloatable)) {
        if (QLayoutItem *item = item_list[FloatButton])
            floatSize = item->widget()->sizeHint();
    }

    int titleHeight = this->titleHeight();

    int mw = q->style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, nullptr, q);
    int fw = q->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, nullptr, q);

    return pick(verticalTitleBar, closeSize)
            + pick(verticalTitleBar, floatSize)
            + titleHeight + 2*fw + 3*mw;
}

int QDockWidgetLayout::titleHeight() const
{
    QDockWidget *q = qobject_cast<QDockWidget*>(parentWidget());

    if (QWidget *title = widgetForRole(TitleBar))
        return perp(verticalTitleBar, title->sizeHint());

    QSize closeSize(0, 0);
    QSize floatSize(0, 0);
    if (QLayoutItem *item = item_list[CloseButton])
        closeSize = item->widget()->sizeHint();
    if (QLayoutItem *item = item_list[FloatButton])
        floatSize = item->widget()->sizeHint();

    int buttonHeight = qMax(perp(verticalTitleBar, closeSize),
                            perp(verticalTitleBar, floatSize));

    QFontMetrics titleFontMetrics = q->fontMetrics();
    int mw = q->style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, nullptr, q);

    return qMax(buttonHeight + 2, titleFontMetrics.height() + 2*mw);
}

void QDockWidgetLayout::setGeometry(const QRect &geometry)
{
    QDockWidget *q = qobject_cast<QDockWidget*>(parentWidget());

    bool nativeDeco = nativeWindowDeco();

    int fw = q->isFloating() && !nativeDeco
            ? q->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, nullptr, q)
            : 0;

    if (nativeDeco) {
        if (QLayoutItem *item = item_list[Content])
            item->setGeometry(geometry);
    } else {
        int titleHeight = this->titleHeight();

        if (verticalTitleBar) {
            _titleArea = QRect(QPoint(fw, fw),
                                QSize(titleHeight, geometry.height() - (fw * 2)));
        } else {
            _titleArea = QRect(QPoint(fw, fw),
                                QSize(geometry.width() - (fw * 2), titleHeight));
        }

        if (QLayoutItem *item = item_list[TitleBar]) {
            item->setGeometry(_titleArea);
        } else {
            QStyleOptionDockWidget opt;
            q->initStyleOption(&opt);

            if (QLayoutItem *item = item_list[CloseButton]) {
                if (!item->isEmpty()) {
                    QRect r = q->style()
                        ->subElementRect(QStyle::SE_DockWidgetCloseButton,
                                            &opt, q);
                    if (!r.isNull())
                        item->setGeometry(r);
                }
            }

            if (QLayoutItem *item = item_list[FloatButton]) {
                if (!item->isEmpty()) {
                    QRect r = q->style()
                        ->subElementRect(QStyle::SE_DockWidgetFloatButton,
                                            &opt, q);
                    if (!r.isNull())
                        item->setGeometry(r);
                }
            }
        }

        if (QLayoutItem *item = item_list[Content]) {
            QRect r = geometry;
            if (verticalTitleBar) {
                r.setLeft(_titleArea.right() + 1);
                r.adjust(0, fw, -fw, -fw);
            } else {
                r.setTop(_titleArea.bottom() + 1);
                r.adjust(fw, 0, -fw, -fw);
            }
            item->setGeometry(r);
        }
    }
}

void QDockWidgetLayout::setVerticalTitleBar(bool b)
{
    if (b == verticalTitleBar)
        return;
    verticalTitleBar = b;
    invalidate();
    parentWidget()->update();
}

/******************************************************************************
** QDockWidgetItem
*/

QDockWidgetItem::QDockWidgetItem(QDockWidget *dockWidget)
    : QWidgetItem(dockWidget)
{
}

QSize QDockWidgetItem::minimumSize() const
{
    QSize widgetMin(0, 0);
    if (QLayoutItem *item = dockWidgetChildItem())
        widgetMin = item->minimumSize();
    return dockWidgetLayout()->sizeFromContent(widgetMin, false);
}

QSize QDockWidgetItem::maximumSize() const
{
    if (QLayoutItem *item = dockWidgetChildItem()) {
        return dockWidgetLayout()->sizeFromContent(item->maximumSize(), false);
    } else {
        return QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
}


QSize QDockWidgetItem::sizeHint() const
{
    if (QLayoutItem *item = dockWidgetChildItem()) {
         return dockWidgetLayout()->sizeFromContent(item->sizeHint(), false);
    } else {
        return QWidgetItem::sizeHint();
    }
}

/******************************************************************************
** QDockWidgetPrivate
*/

void QDockWidgetPrivate::init()
{
    Q_Q(QDockWidget);

    QDockWidgetLayout *layout = new QDockWidgetLayout(q);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    QAbstractButton *button = new QDockWidgetTitleButton(q);
    button->setObjectName("qt_dockwidget_floatbutton"_L1);
    QObjectPrivate::connect(button, &QAbstractButton::clicked,
                            this, &QDockWidgetPrivate::toggleTopLevel);
    layout->setWidgetForRole(QDockWidgetLayout::FloatButton, button);

    button = new QDockWidgetTitleButton(q);
    button->setObjectName("qt_dockwidget_closebutton"_L1);
    QObject::connect(button, &QAbstractButton::clicked, q, &QDockWidget::close);
    layout->setWidgetForRole(QDockWidgetLayout::CloseButton, button);

    font = QApplication::font("QDockWidgetTitle");

#ifndef QT_NO_ACTION
    toggleViewAction = new QAction(q);
    toggleViewAction->setCheckable(true);
    toggleViewAction->setMenuRole(QAction::NoRole);
    fixedWindowTitle = qt_setWindowTitle_helperHelper(q->windowTitle(), q);
    toggleViewAction->setText(fixedWindowTitle);
    QObjectPrivate::connect(toggleViewAction, &QAction::triggered,
                            this, &QDockWidgetPrivate::toggleView);
#endif

    updateButtons();
}

/*!
    Initialize \a option with the values from this QDockWidget. This method
    is useful for subclasses when they need a QStyleOptionDockWidget, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QDockWidget::initStyleOption(QStyleOptionDockWidget *option) const
{
    Q_D(const QDockWidget);

    if (!option)
        return;
    QDockWidgetLayout *dwlayout = qobject_cast<QDockWidgetLayout*>(layout());

    QDockWidgetGroupWindow *floatingTab = qobject_cast<QDockWidgetGroupWindow*>(parent());
    // If we are in a floating tab, init from the parent because the attributes and the geometry
    // of the title bar should be taken from the floating window.
    option->initFrom(floatingTab && !isFloating() ? parentWidget() : this);
    option->rect = dwlayout->titleArea();
    option->title = d->fixedWindowTitle;
    option->closable = hasFeature(this, QDockWidget::DockWidgetClosable);
    option->movable = hasFeature(this, QDockWidget::DockWidgetMovable);
    option->floatable = hasFeature(this, QDockWidget::DockWidgetFloatable);

    QDockWidgetLayout *l = qobject_cast<QDockWidgetLayout*>(layout());
    option->verticalTitleBar = l->verticalTitleBar;
}

void QDockWidgetPrivate::toggleView(bool b)
{
    Q_Q(QDockWidget);
    if (b == q->isHidden()) {
        if (b)
            q->show();
        else
            q->close();
    }
}

void QDockWidgetPrivate::updateButtons()
{
    Q_Q(QDockWidget);
    QDockWidgetLayout *dwLayout = qobject_cast<QDockWidgetLayout*>(layout);

    QStyleOptionDockWidget opt;
    q->initStyleOption(&opt);

    bool customTitleBar = dwLayout->widgetForRole(QDockWidgetLayout::TitleBar) != nullptr;
    bool nativeDeco = dwLayout->nativeWindowDeco();
    bool hideButtons = nativeDeco || customTitleBar;

    bool canClose = hasFeature(this, QDockWidget::DockWidgetClosable);
    bool canFloat = hasFeature(this, QDockWidget::DockWidgetFloatable);

    QAbstractButton *button
        = qobject_cast<QAbstractButton*>(dwLayout->widgetForRole(QDockWidgetLayout::FloatButton));
    button->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarNormalButton, &opt, q));
    button->setVisible(canFloat && !hideButtons);
#if QT_CONFIG(accessibility)
    //: Accessible name for button undocking a dock widget (floating state)
    button->setAccessibleName(QDockWidget::tr("Float"));
    button->setAccessibleDescription(QDockWidget::tr("Undocks and re-attaches the dock widget"));
#endif
    button
        = qobject_cast <QAbstractButton*>(dwLayout->widgetForRole(QDockWidgetLayout::CloseButton));
    button->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarCloseButton, &opt, q));
    button->setVisible(canClose && !hideButtons);
#if QT_CONFIG(accessibility)
    //: Accessible name for button closing a dock widget
    button->setAccessibleName(QDockWidget::tr("Close"));
    button->setAccessibleDescription(QDockWidget::tr("Closes the dock widget"));
#endif

    layout->invalidate();
}

void QDockWidgetPrivate::toggleTopLevel()
{
    Q_Q(QDockWidget);
    q->setFloating(!q->isFloating());
}

/*! \internal
    Initialize the drag state structure and remember the position of the click.
    This is called when the mouse is pressed, but the dock is not yet dragged out.

    \a nca specify that the event comes from NonClientAreaMouseButtonPress
 */
void QDockWidgetPrivate::initDrag(const QPoint &pos, bool nca)
{
    Q_Q(QDockWidget);

    if (state != nullptr)
        return;

    QMainWindowLayout *layout = qt_mainwindow_layout_from_dock(q);
    Q_ASSERT(layout != nullptr);
    if (layout->pluggingWidget != nullptr) // the main window is animating a docking operation
        return;

    state = new QDockWidgetPrivate::DragState;
    state->pressPos = pos;
    state->globalPressPos = q->mapToGlobal(pos);
    state->widgetInitialPos = q->isFloating() ? q->pos() : q->mapToGlobal(QPoint(0, 0));
    state->dragging = false;
    state->widgetItem = nullptr;
    state->ownWidgetItem = false;
    state->nca = nca;
    state->ctrlDrag = false;
}

/*! \internal
    Actually start the drag and detach the dockwidget.
    The \a group parameter is true when we should potentially drag a group of
    tabbed widgets, and false if the dock widget should always be dragged
    alone.
 */
void QDockWidgetPrivate::startDrag(DragScope scope)
{
    Q_Q(QDockWidget);

    if (state == nullptr || state->dragging)
        return;

    QMainWindowLayout *layout = qt_mainwindow_layout_from_dock(q);
    Q_ASSERT(layout != nullptr);

#if QT_CONFIG(draganddrop)
    bool wasFloating = q->isFloating();
#endif

    state->widgetItem = layout->unplug(q, scope);
    if (state->widgetItem == nullptr) {
        /*  Dock widget has a QMainWindow parent, but was never inserted with
            QMainWindow::addDockWidget, so the QMainWindowLayout has no
            widget item for it. It will be newly created and deleted if it doesn't
            get dropped into a dock area. */
        QDockWidgetGroupWindow *floatingTab = qobject_cast<QDockWidgetGroupWindow*>(parent);
        if (floatingTab && !q->isFloating())
            state->widgetItem = new QDockWidgetGroupWindowItem(floatingTab);
        else
            state->widgetItem = new QDockWidgetItem(q);
        state->ownWidgetItem = true;
    }

    if (state->ctrlDrag)
        layout->restore();

    state->dragging = true;

#if QT_CONFIG(draganddrop)
    if (QMainWindowLayout::needsPlatformDrag()) {
        Qt::DropAction result =
                layout->performPlatformWidgetDrag(state->widgetItem, state->pressPos);
        if (result == Qt::IgnoreAction && !wasFloating) {
            layout->revert(state->widgetItem);
            delete state;
            state = nullptr;
        } else {
            endDrag(QDockWidgetPrivate::EndDragMode::LocationChange);
        }
    }
#endif
}

/*! \internal
    Ends the drag end drop operation of the QDockWidget.
    The \a abort parameter specifies that it ends because of programmatic state
    reset rather than mouse release event.
 */
void QDockWidgetPrivate::endDrag(EndDragMode mode)
{
    Q_Q(QDockWidget);
    Q_ASSERT(state != nullptr);

    q->releaseMouse();

    if (state->dragging) {
        const QMainWindow *mainWindow = mainwindow_from_dock(q);
        Q_ASSERT(mainWindow != nullptr);
        QMainWindowLayout *mwLayout = qt_mainwindow_layout(mainWindow);

        // if mainWindow is being deleted in an ongoing drag, make it a no-op instead of crashing
        if (!mwLayout)
            return;

        if (mode == EndDragMode::Abort || !mwLayout->plug(state->widgetItem)) {
            if (hasFeature(this, QDockWidget::DockWidgetFloatable)) {
                // This QDockWidget will now stay in the floating state.
                if (state->ownWidgetItem) {
                    delete state->widgetItem;
                    state->widgetItem = nullptr;
                }
                mwLayout->restore();
                QDockWidgetLayout *dwLayout = qobject_cast<QDockWidgetLayout*>(layout);
                if (!dwLayout->nativeWindowDeco()) {
                    // get rid of the X11BypassWindowManager window flag and activate the resizer
                    Qt::WindowFlags flags = q->windowFlags();
                    flags &= ~Qt::X11BypassWindowManagerHint;
                    q->setWindowFlags(flags);
                    setResizerActive(q->isFloating());
                    q->show();
                } else {
                    setResizerActive(false);
                }
                if (q->isFloating()) { // Might not be floating when dragging a QDockWidgetGroupWindow
                    undockedGeometry = q->geometry();
#if QT_CONFIG(tabwidget)
                    // is the widget located within the mainwindow?
                    const Qt::DockWidgetArea area = mainWindow->dockWidgetArea(q);
                    if (area != Qt::NoDockWidgetArea) {
                        tabPosition = mwLayout->tabPosition(area);
                    } else if (auto dwgw = qobject_cast<QDockWidgetGroupWindow *>(q->parent())) {
                        // DockWidget wasn't found in one of the docks within mainwindow
                        // => derive tabPosition from parent
                        tabPosition = mwLayout->tabPosition(toDockWidgetArea(dwgw->layoutInfo()->dockPos));
                    }
#endif
                    // Reparent, if the drag was out of a dock widget group window
                    if (mode == EndDragMode::LocationChange) {
                        if (auto *groupWindow = qobject_cast<QDockWidgetGroupWindow *>(q->parentWidget()))
                            groupWindow->reparentToMainWindow(q);
                    }
                }
                q->activateWindow();
            } else {
                // The tab was not plugged back in the QMainWindow but the QDockWidget cannot
                // stay floating, revert to the previous state.
                mwLayout->revert(state->widgetItem);
            }
        }
    }
    delete state;
    state = nullptr;
}

Qt::DockWidgetArea QDockWidgetPrivate::toDockWidgetArea(QInternal::DockPosition pos)
{
    switch (pos) {
    case QInternal::LeftDock:   return Qt::LeftDockWidgetArea;
    case QInternal::RightDock:  return Qt::RightDockWidgetArea;
    case QInternal::TopDock:    return Qt::TopDockWidgetArea;
    case QInternal::BottomDock: return Qt::BottomDockWidgetArea;
    default: break;
    }
    return Qt::NoDockWidgetArea;
}

void QDockWidgetPrivate::setResizerActive(bool active)
{
    Q_Q(QDockWidget);
    const auto *dwLayout = qobject_cast<QDockWidgetLayout *>(layout);
    if (dwLayout->nativeWindowDeco(q->isFloating()))
        return;

    if (active && !resizer)
        resizer = new QWidgetResizeHandler(q);
    if (resizer)
        resizer->setEnabled(active);
}

bool QDockWidgetPrivate::isAnimating() const
{
    Q_Q(const QDockWidget);

    QMainWindowLayout *mainWinLayout = qt_mainwindow_layout_from_dock(q);
    if (mainWinLayout == nullptr)
        return false;

    return (const void*)mainWinLayout->pluggingWidget == (const void*)q;
}

bool QDockWidgetPrivate::mousePressEvent(QMouseEvent *event)
{
#if QT_CONFIG(mainwindow)
    Q_Q(QDockWidget);

    QDockWidgetLayout *dwLayout
        = qobject_cast<QDockWidgetLayout*>(layout);

    if (!dwLayout->nativeWindowDeco()) {
        QRect titleArea = dwLayout->titleArea();

        QDockWidgetGroupWindow *floatingTab = qobject_cast<QDockWidgetGroupWindow*>(parent);

        if (event->button() != Qt::LeftButton ||
            !titleArea.contains(event->position().toPoint()) ||
            // check if the tool window is movable... do nothing if it
            // is not (but allow moving if the window is floating)
            (!hasFeature(this, QDockWidget::DockWidgetMovable) && !q->isFloating()) ||
            (qobject_cast<QMainWindow*>(parent) == nullptr && !floatingTab) ||
            isAnimating() || state != nullptr) {
            return false;
        }

        initDrag(event->position().toPoint(), false);

        if (state)
            state->ctrlDrag = (hasFeature(this, QDockWidget::DockWidgetFloatable) && event->modifiers() & Qt::ControlModifier) ||
                              (!hasFeature(this, QDockWidget::DockWidgetMovable) && q->isFloating());

        return true;
    }

#endif // QT_CONFIG(mainwindow)
    return false;
}

bool QDockWidgetPrivate::mouseDoubleClickEvent(QMouseEvent *event)
{
    QDockWidgetLayout *dwLayout = qobject_cast<QDockWidgetLayout*>(layout);

    if (!dwLayout->nativeWindowDeco()) {
        QRect titleArea = dwLayout->titleArea();

        if (event->button() == Qt::LeftButton && titleArea.contains(event->position().toPoint()) &&
            hasFeature(this, QDockWidget::DockWidgetFloatable)) {
            toggleTopLevel();
            return true;
        }
    }
    return false;
}

bool QDockWidgetPrivate::isTabbed() const
{
    Q_Q(const QDockWidget);
    QDockWidget *that = const_cast<QDockWidget *>(q);
    auto *mwLayout = qt_mainwindow_layout_from_dock(that);
    Q_ASSERT(mwLayout);
    return mwLayout->isDockWidgetTabbed(q);
}

bool QDockWidgetPrivate::mouseMoveEvent(QMouseEvent *event)
{
    bool ret = false;
#if QT_CONFIG(mainwindow)
    Q_Q(QDockWidget);

    if (!state)
        return ret;

    QDockWidgetLayout *dwlayout
        = qobject_cast<QDockWidgetLayout *>(layout);
    QMainWindowLayout *mwlayout = qt_mainwindow_layout_from_dock(q);
    if (!dwlayout->nativeWindowDeco()) {
        if (!state->dragging
            && mwlayout->pluggingWidget == nullptr
            && (event->position().toPoint() - state->pressPos).manhattanLength()
                > QApplication::startDragDistance()) {

#ifdef Q_OS_MACOS
            if (windowHandle() && !q->isFloating()) {
                // When using native widgets on mac, we have not yet been successful in
                // starting a drag on an NSView that belongs to one window (QMainWindow),
                // but continue the drag on another (QDockWidget). This is what happens if
                // we try to make this widget floating during a drag. So as a fall back
                // solution, we simply make this widget floating instead, when we would
                // otherwise start a drag.
                q->setFloating(true);
            } else
#endif
            {
                const DragScope scope = isTabbed() ? DragScope::Group : DragScope::Widget;
                startDrag(scope);
                q->grabMouse();
                ret = true;
            }
        }
    }

    if (state && state->dragging && !state->nca) {
        QMargins windowMargins = q->window()->windowHandle()->frameMargins();
        QPoint windowMarginOffset = QPoint(windowMargins.left(), windowMargins.top());

        // TODO maybe use QScreen API (if/when available) to simplify the below code.
        const QScreen *orgWdgScreen = QGuiApplication::screenAt(state->widgetInitialPos);
        const QScreen *screenFrom = QGuiApplication::screenAt(state->globalPressPos);
        const QScreen *screenTo = QGuiApplication::screenAt(event->globalPosition().toPoint());
        const QScreen *wdgScreen = q->screen();

        QPoint pos;
        if (Q_LIKELY(screenFrom && screenTo && wdgScreen && orgWdgScreen)) {
            const QPoint nativeWdgOrgPos = QHighDpiScaling::mapPositionToNative(
                    state->widgetInitialPos, orgWdgScreen->handle());
            const QPoint nativeTo = QHighDpiScaling::mapPositionToNative(
                    event->globalPosition().toPoint(), screenTo->handle());
            const QPoint nativeFrom = QHighDpiScaling::mapPositionToNative(state->globalPressPos,
                                                                           screenFrom->handle());

            // Calculate new nativePos based on startPos + mouse delta move.
            const QPoint nativeNewPos = nativeWdgOrgPos + (nativeTo - nativeFrom);
            pos = QHighDpiScaling::mapPositionFromNative(nativeNewPos, wdgScreen->handle())
                    - windowMarginOffset;
        } else {
            // Fallback in the unlikely case that source and target screens could not be established
            qCDebug(lcQpaDockWidgets)
                    << "QDockWidget failed to find relevant screen info. screenFrom:" << screenFrom
                    << "screenTo:" << screenTo << " wdgScreen:" << wdgScreen << "orgWdgScreen"
                    << orgWdgScreen;
            pos = event->globalPosition().toPoint() - state->pressPos - windowMarginOffset;
        }

        // If the newly floating dock widget has got a native title bar,
        // offset the position by the native title bar's height or width
        const int dx = q->geometry().x() - q->x();
        const int dy = q->geometry().y() - q->y();
        pos.rx() += dx;
        pos.ry() += dy;

        QDockWidgetGroupWindow *floatingTab = qobject_cast<QDockWidgetGroupWindow*>(parent);
        if (floatingTab && !q->isFloating())
            floatingTab->move(pos);
        else
            q->move(pos);
        if (state && !state->ctrlDrag)
            mwlayout->hover(state->widgetItem, event->globalPosition().toPoint());

        ret = true;
    }

#endif // QT_CONFIG(mainwindow)
    return ret;
}

bool QDockWidgetPrivate::mouseReleaseEvent(QMouseEvent *event)
{
#if QT_CONFIG(mainwindow)
#if QT_CONFIG(draganddrop)
    // if we are peforming a platform drag ignore the release here and end the drag when the actual
    // drag ends.
    if (QMainWindowLayout::needsPlatformDrag())
        return false;
#endif

    if (event->button() == Qt::LeftButton && state && !state->nca) {
        endDrag(EndDragMode::LocationChange);
        return true; //filter out the event
    }

#endif // QT_CONFIG(mainwindow)
    return false;
}

void QDockWidgetPrivate::nonClientAreaMouseEvent(QMouseEvent *event)
{
    Q_Q(QDockWidget);

    int fw = q->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, nullptr, q);

    QWidget *tl = q->topLevelWidget();
    QRect geo = tl->geometry();
    QRect titleRect = tl->frameGeometry();
    {
        titleRect.setLeft(geo.left());
        titleRect.setRight(geo.right());
        titleRect.setBottom(geo.top() - 1);
        titleRect.adjust(0, fw, 0, 0);
    }

    switch (event->type()) {
        case QEvent::NonClientAreaMouseButtonPress:
            if (!titleRect.contains(event->globalPosition().toPoint()))
                break;
            if (state != nullptr)
                break;
            if (qobject_cast<QMainWindow*>(parent) == nullptr && qobject_cast<QDockWidgetGroupWindow*>(parent) == nullptr)
                break;
            if (isAnimating())
                break;
            initDrag(event->position().toPoint(), true);
            if (state == nullptr)
                break;
            state->ctrlDrag = (event->modifiers() & Qt::ControlModifier) ||
                              (!hasFeature(this, QDockWidget::DockWidgetMovable) && q->isFloating());
            startDrag(DragScope::Group);
            break;
        case QEvent::NonClientAreaMouseMove:
            if (state == nullptr || !state->dragging)
                break;

#if !defined(Q_OS_MAC) && !defined(Q_OS_WASM)
            if (state->nca)
                endDrag(EndDragMode::LocationChange);
#endif
            break;
        case QEvent::NonClientAreaMouseButtonRelease:
#if defined(Q_OS_MAC) || defined(Q_OS_WASM)
                        if (state)
                            endDrag(EndDragMode::LocationChange);
#endif
                        break;
        case QEvent::NonClientAreaMouseButtonDblClick:
            toggleTopLevel();
            break;
        default:
            break;
    }
}

void QDockWidgetPrivate::recalculatePressPos(QResizeEvent *event)
{
    qreal ratio = event->oldSize().width() / (1.0 * event->size().width());
    state->pressPos.setX(state->pressPos.x() / ratio);
}

/*! \internal
    Called when the QDockWidget or the QDockWidgetGroupWindow is moved
 */
void QDockWidgetPrivate::moveEvent(QMoveEvent *event)
{
    Q_Q(QDockWidget);

    if (state == nullptr || !state->dragging || !state->nca)
        return;

    if (!q->isWindow() && qobject_cast<QDockWidgetGroupWindow*>(parent) == nullptr)
        return;

    // When the native window frame is being dragged, all we get is these mouse
    // move events.

    if (state->ctrlDrag)
        return;

    QMainWindowLayout *layout = qt_mainwindow_layout_from_dock(q);
    Q_ASSERT(layout != nullptr);

    QPoint globalMousePos = event->pos() + state->pressPos;
    layout->hover(state->widgetItem, globalMousePos);
}

void QDockWidgetPrivate::unplug(const QRect &rect)
{
    Q_Q(QDockWidget);
    QRect r = rect;
    r.moveTopLeft(q->mapToGlobal(QPoint(0, 0)));
    QDockWidgetLayout *dwLayout = qobject_cast<QDockWidgetLayout*>(layout);
    if (dwLayout->nativeWindowDeco(true))
        r.adjust(0, dwLayout->titleHeight(), 0, 0);
    setWindowState({WindowState::Floating, WindowState::Unplug}, r);
}

void QDockWidgetPrivate::plug(const QRect &rect)
{
    setWindowState(WindowStates(), rect);
}

void QDockWidgetPrivate::setWindowState(WindowStates states, const QRect &rect)
{
    Q_Q(QDockWidget);
    const bool floating = states.testFlag(WindowState::Floating);
    bool unplug = states.testFlag(WindowState::Unplug);

    if (!floating && parent) {
        QMainWindowLayout *mwlayout = qt_mainwindow_layout_from_dock(q);
        if (mwlayout && mwlayout->dockWidgetArea(q) == Qt::NoDockWidgetArea
                && !qobject_cast<QDockWidgetGroupWindow *>(parent))
            return; // this dockwidget can't be redocked
    }

    const bool wasFloating = q->isFloating();
    if (wasFloating) // Prevent repetitive unplugging from nested invocations (QTBUG-42818)
        unplug = false;
    const bool hidden = q->isHidden();

    if (q->isVisible())
        q->hide();

    Qt::WindowFlags flags = floating ? Qt::Tool : Qt::Widget;

    QDockWidgetLayout *dwLayout = qobject_cast<QDockWidgetLayout*>(layout);
    const bool nativeDeco = dwLayout->nativeWindowDeco(floating);

    if (nativeDeco) {
        flags |= Qt::CustomizeWindowHint | Qt::WindowTitleHint;
        if (hasFeature(this, QDockWidget::DockWidgetClosable))
            flags |= Qt::WindowCloseButtonHint;
    } else {
        flags |= Qt::FramelessWindowHint;
    }

#if QT_CONFIG(draganddrop)
    // If we are performing a platform drag the flag is not needed and we want to avoid recreating
    // the platform window when it would be removed later
    if (unplug && !QMainWindowLayout::needsPlatformDrag())
        flags |= Qt::X11BypassWindowManagerHint;
#endif

    q->setWindowFlags(flags);


    if (!rect.isNull())
            q->setGeometry(rect);

    updateButtons();

    if (!hidden)
        q->show();

    if (floating != wasFloating) {
        emit q->topLevelChanged(floating);
        if (!floating && parent) {
            QMainWindowLayout *mwlayout = qt_mainwindow_layout_from_dock(q);
            if (mwlayout)
                emit q->dockLocationChanged(mwlayout->dockWidgetArea(q));
        } else {
            emit q->dockLocationChanged(Qt::NoDockWidgetArea);
        }
    }

    setResizerActive(!unplug && floating && !nativeDeco);
}

/*!
    \class QDockWidget

    \brief The QDockWidget class provides a widget that can be docked
    inside a QMainWindow or floated as a top-level window on the
    desktop.

    \ingroup mainwindow-classes
    \inmodule QtWidgets

    QDockWidget provides the concept of dock widgets, also know as
    tool palettes or utility windows.  Dock windows are secondary
    windows placed in the \e {dock widget area} around the
    \l{QMainWindow::centralWidget()}{central widget} in a
    QMainWindow.

    \image mainwindow-docks.png

    Dock windows can be moved inside their current area, moved into
    new areas and floated (e.g., undocked) by the end-user.  The
    QDockWidget API allows the programmer to restrict the dock widgets
    ability to move, float and close, as well as the areas in which
    they can be placed.

    \section1 Appearance

    A QDockWidget consists of a title bar and the content area.  The
    title bar displays the dock widgets
    \l{QWidget::windowTitle()}{window title},
    a \e float button and a \e close button.
    Depending on the state of the QDockWidget, the \e float and \e
    close buttons may be either disabled or not shown at all.

    The visual appearance of the title bar and buttons is dependent
    on the \l{QStyle}{style} in use.

    A QDockWidget acts as a wrapper for its child widget, set with setWidget().
    Custom size hints, minimum and maximum sizes and size policies should be
    implemented in the child widget. QDockWidget will respect them, adjusting
    its own constraints to include the frame and title. Size constraints
    should not be set on the QDockWidget itself, because they change depending
    on whether it is docked; a docked QDockWidget has no frame and a smaller title
    bar.

    \note On macOS, if the QDockWidget has a native window handle (for example,
    if winId() is called on it or the child widget), then due to a limitation it will not be
    possible to drag the dock widget when undocking. Starting the drag will undock
    the dock widget, but a second drag will be needed to move the dock widget itself.

    \sa QMainWindow
*/

/*!
    \enum QDockWidget::DockWidgetFeature

    \value DockWidgetClosable   The dock widget can be closed.
    \value DockWidgetMovable    The dock widget can be moved between docks
                                by the user.
    \value DockWidgetFloatable  The dock widget can be detached from the
                                main window, and floated as an independent
                                window.
    \value DockWidgetVerticalTitleBar The dock widget displays a vertical title
                                  bar on its left side. This can be used to
                                  increase the amount of vertical space in
                                  a QMainWindow.
    \value NoDockWidgetFeatures   The dock widget cannot be closed, moved,
                                  or floated.

    \omitvalue DockWidgetFeatureMask
    \omitvalue Reserved
*/

/*!
    \property QDockWidget::windowTitle
    \brief the dock widget title (caption)

    By default, this property contains an empty string.
*/

/*!
    Constructs a QDockWidget with parent \a parent and window flags \a
    flags. The dock widget will be placed in the left dock widget
    area.
*/
QDockWidget::QDockWidget(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(*new QDockWidgetPrivate, parent, flags)
{
    Q_D(QDockWidget);
    d->init();
}

/*!
    Constructs a QDockWidget with parent \a parent and window flags \a
    flags. The dock widget will be placed in the left dock widget
    area.

    The window title is set to \a title. This title is used when the
    QDockWidget is docked and undocked. It is also used in the context
    menu provided by QMainWindow.

    \sa setWindowTitle()
*/
QDockWidget::QDockWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags)
{
    setWindowTitle(title);
}

/*!
    Destroys the dock widget.
*/
QDockWidget::~QDockWidget()
{
    Q_D(QDockWidget);
    d->inDestructor = true;
    // Do all the unregistering while we're still a QDockWidget. Otherwise, it
    // would be ~QObject() which does that and then QDockAreaLayout::takeAt(),
    // acting on QEvent::ChildRemoved, will try to access our QWidget-ness when
    // replacing us with a QPlaceHolderItem, causing UB:
    setParent(nullptr);
}

/*!
    Returns the widget for the dock widget. This function returns zero
    if the widget has not been set.

    \sa setWidget()
*/
QWidget *QDockWidget::widget() const
{
    QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(this->layout());
    return layout->widgetForRole(QDockWidgetLayout::Content);
}

/*!
    Sets the widget for the dock widget to \a widget.

    If the dock widget is visible when \a widget is added, you must
    \l{QWidget::}{show()} it explicitly.

    Note that you must add the layout of the \a widget before you call
    this function; if not, the \a widget will not be visible.

    \sa widget()
*/
void QDockWidget::setWidget(QWidget *widget)
{
    QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(this->layout());
    layout->setWidgetForRole(QDockWidgetLayout::Content, widget);
}

/*!
    \property QDockWidget::features
    \brief whether the dock widget is movable, closable, and floatable

    By default, this property is set to a combination of DockWidgetClosable,
    DockWidgetMovable and DockWidgetFloatable.

    \sa DockWidgetFeature
*/

void QDockWidget::setFeatures(QDockWidget::DockWidgetFeatures features)
{
    Q_D(QDockWidget);
    features &= DockWidgetFeatureMask;
    if (d->features == features)
        return;
    const bool closableChanged = (d->features ^ features) & DockWidgetClosable;
    d->features = features;
    QDockWidgetLayout *layout
        = qobject_cast<QDockWidgetLayout*>(this->layout());
    layout->setVerticalTitleBar(features & DockWidgetVerticalTitleBar);
    d->updateButtons();
    d->toggleViewAction->setEnabled((d->features & DockWidgetClosable) == DockWidgetClosable);
    emit featuresChanged(d->features);
    update();
    if (closableChanged && layout->nativeWindowDeco()) {
        QDockWidgetGroupWindow *floatingTab = qobject_cast<QDockWidgetGroupWindow *>(parent());
        if (floatingTab && !isFloating()) {
            floatingTab->adjustFlags();
        } else {
            d->setWindowState({QDockWidgetPrivate::WindowState::Floating,
                               QDockWidgetPrivate::WindowState::Unplug});
        }
    }
}

QDockWidget::DockWidgetFeatures QDockWidget::features() const
{
    Q_D(const QDockWidget);
    return d->features;
}

/*!
    \property QDockWidget::floating
    \brief whether the dock widget is floating

    A floating dock widget is presented to the user as a single, independent
    window "on top" of its parent QMainWindow, instead of being docked
    either in the QMainWindow, or in a group of tabbed dock widgets.

    Floating dock widgets can be individually positioned and resized, both
    programmatically or by mouse interaction.

    By default, this property is \c true.

    When this property changes, the \c {topLevelChanged()} signal is emitted.

    \sa isWindow(), topLevelChanged()
*/
void QDockWidget::setFloating(bool floating)
{
    Q_D(QDockWidget);
    d->setFloating(floating);
}

/*!
   \internal implementation of setFloating
 */
void QDockWidgetPrivate::setFloating(bool floating)
{
    Q_Q(QDockWidget);
    // the initial click of a double-click may have started a drag...
    if (state != nullptr)
        endDrag(QDockWidgetPrivate::EndDragMode::Abort);

    // Keep position when undocking for the first time.
    QRect r = undockedGeometry;
    if (floating && q->isVisible() && !r.isValid())
        r = QRect(q->mapToGlobal(QPoint(0, 0)), q->size());

    // Reparent, if setFloating() was called on a floating tab
    // Reparenting has to happen before setWindowState.
    // The reparented dock widget will inherit visibility from the floating tab.
    // => Remember visibility and the necessity to update it.
    enum class VisibilityRule {
        NoUpdate,
        Show,
        Hide,
    };

    VisibilityRule updateRule = VisibilityRule::NoUpdate;

    if (floating && !q->isFloating()) {
        if (auto *groupWindow = qobject_cast<QDockWidgetGroupWindow *>(q->parentWidget())) {
            updateRule = groupWindow->isVisible() ? VisibilityRule::Show : VisibilityRule::Hide;
            q->setParent(groupWindow->parentWidget());
        }
    }

    WindowStates states;
    states.setFlag(WindowState::Floating, floating);
    setWindowState(states, floating ? r : QRect());

    if (floating && r.isNull()) {
        if (q->x() < 0 || q->y() < 0) //may happen if we have been hidden
            q->move(QPoint());
        q->setAttribute(Qt::WA_Moved, false); //we want it at the default position
    }

    switch (updateRule) {
    case VisibilityRule::NoUpdate:
        break;
    case VisibilityRule::Show:
        q->show();
        break;
    case VisibilityRule::Hide:
        q->hide();
        break;
    }
}

/*!
    \property QDockWidget::allowedAreas
    \brief areas where the dock widget may be placed

    The default is Qt::AllDockWidgetAreas.

    \sa Qt::DockWidgetArea
*/

void QDockWidget::setAllowedAreas(Qt::DockWidgetAreas areas)
{
    Q_D(QDockWidget);
    areas &= Qt::DockWidgetArea_Mask;
    if (areas == d->allowedAreas)
        return;
    d->allowedAreas = areas;
    emit allowedAreasChanged(d->allowedAreas);
}

Qt::DockWidgetAreas QDockWidget::allowedAreas() const
{
    Q_D(const QDockWidget);
    return d->allowedAreas;
}

/*!
    \fn bool QDockWidget::isAreaAllowed(Qt::DockWidgetArea area) const

    Returns \c true if this dock widget can be placed in the given \a area;
    otherwise returns \c false.
*/

/*! \reimp */
void QDockWidget::changeEvent(QEvent *event)
{
    Q_D(QDockWidget);
    QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(this->layout());

    switch (event->type()) {
    case QEvent::WindowTitleChange:
        if (isFloating() && windowHandle() && d->topData() && windowHandle()->isVisible()) {
            // From QWidget::setWindowTitle(): Propagate window title without signal emission
            d->topData()->caption = windowHandle()->title();
            d->setWindowTitle_helper(windowHandle()->title());
        }
        Q_FALLTHROUGH();
    case QEvent::ModifiedChange:
        update(layout->titleArea());
#ifndef QT_NO_ACTION
        d->fixedWindowTitle = qt_setWindowTitle_helperHelper(windowTitle(), this);
        d->toggleViewAction->setText(d->fixedWindowTitle);
#endif
#if QT_CONFIG(tabbar)
        {
            if (QMainWindowLayout *winLayout = qt_mainwindow_layout_from_dock(this)) {
                if (QDockAreaLayoutInfo *info = winLayout->layoutState.dockAreaLayout.info(this))
                    info->updateTabBar();
            }
        }
#endif // QT_CONFIG(tabbar)
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

/*! \reimp */
void QDockWidget::closeEvent(QCloseEvent *event)
{
    Q_D(QDockWidget);
    if (d->state)
        d->endDrag(QDockWidgetPrivate::EndDragMode::Abort);

    // For non-closable widgets, don't allow closing, except when the mainwindow
    // is hidden, as otherwise an application wouldn't be able to be shut down.
    const QMainWindow *win = qobject_cast<QMainWindow*>(parentWidget());
    const bool canClose = (d->features & DockWidgetClosable)
                       || (!win || !win->isVisible());
    event->setAccepted(canClose);
}

/*! \reimp */
void QDockWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    Q_D(QDockWidget);

    QDockWidgetLayout *layout
        = qobject_cast<QDockWidgetLayout*>(this->layout());
    bool customTitleBar = layout->widgetForRole(QDockWidgetLayout::TitleBar) != nullptr;
    bool nativeDeco = layout->nativeWindowDeco();

    if (!nativeDeco && !customTitleBar) {
        QStylePainter p(this);
        // ### Add PixelMetric to change spacers, so style may show border
        // when not floating.
        if (isFloating()) {
            QStyleOptionFrame framOpt;
            framOpt.initFrom(this);
            p.drawPrimitive(QStyle::PE_FrameDockWidget, framOpt);
        }

        // Title must be painted after the frame, since the areas overlap, and
        // the title may wish to extend out to all sides (eg. Vista style)
        QStyleOptionDockWidget titleOpt;
        initStyleOption(&titleOpt);
        if (font() == QApplication::font("QDockWidget")) {
            titleOpt.fontMetrics = QFontMetrics(d->font);
            p.setFont(d->font);
        }

        p.drawControl(QStyle::CE_DockWidgetTitle, titleOpt);
    }
}

/*! \reimp */
bool QDockWidget::event(QEvent *event)
{
    Q_D(QDockWidget);

    QMainWindow *win = qobject_cast<QMainWindow*>(parentWidget());
    QMainWindowLayout *layout = qt_mainwindow_layout_from_dock(this);

    switch (event->type()) {
#ifndef QT_NO_ACTION
    case QEvent::Hide:
        if (layout != nullptr)
            layout->keepSize(this);
        // If we are in the destructor, don't emit any signals, as those might
        // be handled by a slot that requires this dock widget to still be alive.
        if (!d->inDestructor) {
            d->toggleViewAction->setChecked(false);
            emit visibilityChanged(false);
        }
        break;
    case QEvent::Show: {
        d->toggleViewAction->setChecked(true);
        QPoint parentTopLeft(0, 0);
        if (isWindow()) {
            const QScreen *screen = d->associatedScreen();
            parentTopLeft = screen
                ? screen->availableVirtualGeometry().topLeft()
                : QGuiApplication::primaryScreen()->availableVirtualGeometry().topLeft();
        }
        emit visibilityChanged(geometry().right() >= parentTopLeft.x() && geometry().bottom() >= parentTopLeft.y());
}
        break;
#endif
    case QEvent::ApplicationLayoutDirectionChange:
    case QEvent::LayoutDirectionChange:
    case QEvent::StyleChange:
    case QEvent::ParentChange:
        d->updateButtons();
        break;
    case QEvent::ZOrderChange: {
        bool onTop = false;
        if (win != nullptr) {
            const QObjectList &siblings = win->children();
            onTop = siblings.size() > 0 && siblings.last() == (QObject*)this;
        }
#if QT_CONFIG(tabbar)
        if (!isFloating() && layout != nullptr && onTop)
            layout->raise(this);
#endif
        break;
    }
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        update(qobject_cast<QDockWidgetLayout *>(this->layout())->titleArea());
        break;
    case QEvent::ContextMenu:
        if (d->state) {
            event->accept();
            return true;
        }
        break;
        // return true after calling the handler since we don't want
        // them to be passed onto the default handlers
    case QEvent::MouseButtonPress:
        if (d->mousePressEvent(static_cast<QMouseEvent *>(event)))
            return true;
        break;
    case QEvent::MouseButtonDblClick:
        if (d->mouseDoubleClickEvent(static_cast<QMouseEvent *>(event)))
            return true;
        break;
    case QEvent::MouseMove:
        if (d->mouseMoveEvent(static_cast<QMouseEvent *>(event)))
            return true;
        break;
    case QEvent::MouseButtonRelease:
        if (d->mouseReleaseEvent(static_cast<QMouseEvent *>(event)))
            return true;
        break;
    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
        d->nonClientAreaMouseEvent(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::Move:
        d->moveEvent(static_cast<QMoveEvent*>(event));
        break;
    case QEvent::Resize:
        // if the mainwindow is plugging us, we don't want to update undocked geometry
        if (isFloating() && layout != nullptr && layout->pluggingWidget != this)
            d->undockedGeometry = geometry();

        // Usually the window won't get resized while it's being moved, but it can happen,
        // for example on Windows when moving to a screen with bigger scale factor
        // If that happens we should update state->pressPos, otherwise it will be outside
        // the window when the window shrinks.
        if (d->state && d->state->dragging)
            d->recalculatePressPos(static_cast<QResizeEvent*>(event));
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

#ifndef QT_NO_ACTION
/*!
  Returns a checkable action that can be added to menus and toolbars so that
  the user can show or close this dock widget.

  The action's text is set to the dock widget's window title.

  The QAction object is owned by the QDockWidget. It will be automatically
  deleted when the QDockWidget is destroyed.

  \note The action can not be used to programmatically show or hide the dock
  widget. Use the \l visible property for that.

  \sa QAction::text, QWidget::windowTitle
 */
QAction * QDockWidget::toggleViewAction() const
{
    Q_D(const QDockWidget);
    return d->toggleViewAction;
}
#endif // QT_NO_ACTION

/*!
    \fn void QDockWidget::featuresChanged(QDockWidget::DockWidgetFeatures features)

    This signal is emitted when the \l features property changes. The
    \a features parameter gives the new value of the property.
*/

/*!
    \fn void QDockWidget::topLevelChanged(bool topLevel)

    This signal is emitted when the \l floating property changes.
    The \a topLevel parameter is true if the dock widget is now floating;
    otherwise it is false.

    \sa isWindow()
*/

/*!
    \fn void QDockWidget::allowedAreasChanged(Qt::DockWidgetAreas allowedAreas)

    This signal is emitted when the \l allowedAreas property changes. The
    \a allowedAreas parameter gives the new value of the property.
*/

/*!
    \fn void QDockWidget::visibilityChanged(bool visible)
    \since 4.3

    This signal is emitted when the dock widget becomes \a visible (or
    invisible). This happens when the widget is hidden or shown, as
    well as when it is docked in a tabbed dock area and its tab
    becomes selected or unselected.

    \note The signal can differ from QWidget::isVisible(). This can be the case, if
    a dock widget is minimized or tabified and associated to a non-selected or
    inactive tab.
*/

/*!
    \fn void QDockWidget::dockLocationChanged(Qt::DockWidgetArea area)
    \since 4.3

    This signal is emitted when the dock widget is moved to another
    dock \a area, or is moved to a different location in its current
    dock area. This happens when the dock widget is moved
    programmatically or is dragged to a new location by the user.
    \sa dockLocation(), setDockLocation()
*/

/*!
    \since 4.3

    Sets an arbitrary \a widget as the dock widget's title bar. If \a widget
    is \nullptr, any custom title bar widget previously set on the dock widget
    is removed, but not deleted, and the default title bar will be used
    instead.

    If a title bar widget is set, QDockWidget will not use native window
    decorations when it is floated.

    Here are some tips for implementing custom title bars:

    \list
    \li Mouse events that are not explicitly handled by the title bar widget
       must be ignored by calling QMouseEvent::ignore(). These events then
       propagate to the QDockWidget parent, which handles them in the usual
       manner, moving when the title bar is dragged, docking and undocking
       when it is double-clicked, etc.

    \li When DockWidgetVerticalTitleBar is set on QDockWidget, the title
       bar widget is repositioned accordingly. In resizeEvent(), the title
       bar should check what orientation it should assume:
       \snippet code/src_gui_widgets_qdockwidget.cpp 0

    \li The title bar widget must have a valid QWidget::sizeHint() and
       QWidget::minimumSizeHint(). These functions should take into account
       the current orientation of the title bar.

    \li It is not possible to remove a title bar from a dock widget. However,
       a similar effect can be achieved by setting a default constructed
       QWidget as the title bar widget.
    \endlist

    Using qobject_cast() as shown above, the title bar widget has full access
    to its parent QDockWidget. Hence it can perform such operations as docking
    and hiding in response to user actions.

    \sa titleBarWidget(), DockWidgetVerticalTitleBar
*/

void QDockWidget::setTitleBarWidget(QWidget *widget)
{
    Q_D(QDockWidget);
    QDockWidgetLayout *layout
        = qobject_cast<QDockWidgetLayout*>(this->layout());
    layout->setWidgetForRole(QDockWidgetLayout::TitleBar, widget);
    d->updateButtons();
    if (isWindow()) {
        //this ensures the native decoration is drawn
        d->setWindowState({QDockWidgetPrivate::WindowState::Floating,
                           QDockWidgetPrivate::WindowState::Unplug});
    }
}

/*!
    \since 6.9

    Assigns this dock widget to \a area. If docked at another dock location, it
    will move to \a area. If floating or part of floating tabs, the next call
    of setFloating(false) will dock it at \a area.

    \note setDockLocation(Qt::NoDockLocation) is equivalent to setFloating(true).

    \sa dockLocation(), dockLocationChanged()
 */
void QDockWidget::setDockLocation(Qt::DockWidgetArea area)
{
    if (area == Qt::NoDockWidgetArea && !isFloating()) {
        setFloating(true);
        return;
    }

    auto *mainWindow = const_cast<QMainWindow *>(mainwindow_from_dock(this));
    Q_ASSERT(mainWindow);
    mainWindow->addDockWidget(area, this);
}

/*!
    \property QDockWidget::dockLocation
    \since 6.9

    \brief the current dock location, or Qt::NoDockLocation if this dock widget
    is floating or has no mainwindow parent.
 */
Qt::DockWidgetArea QDockWidget::dockLocation() const
{
    // QDockWidgetPrivate::setWindowState() emits NoDockWidgetArea if
    // the dock widget becomes floating.
    // QMainWindowLayout::dockWidgetArea() always returns the area where
    // the dock widget's item_list is kept.
    if (isFloating())
        return Qt::NoDockWidgetArea;

    auto *mainWindow = mainwindow_from_dock(this);
    Q_ASSERT(mainWindow);
    // FIXME in Qt 7: Make dockWidgetArea take a const QDockWidget* argument
    return mainWindow->dockWidgetArea(const_cast<QDockWidget *>(this));
}

/*!
    \since 4.3
    Returns the custom title bar widget set on the QDockWidget, or
    \nullptr if no custom title bar has been set.

    \sa setTitleBarWidget()
*/

QWidget *QDockWidget::titleBarWidget() const
{
    QDockWidgetLayout *layout
        = qobject_cast<QDockWidgetLayout*>(this->layout());
    return layout->widgetForRole(QDockWidgetLayout::TitleBar);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QDockWidget *dockWidget)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();

    if (!dockWidget) {
        dbg << "QDockWidget(0x0)";
        return dbg;
    }

    dbg << "QDockWidget(" << static_cast<const void *>(dockWidget);
    dbg << "->(ObjectName=" << dockWidget->objectName();
    dbg << "; floating=" << dockWidget->isFloating();
    dbg << "; features=" << dockWidget->features();
    dbg << ";))";
    return dbg;
}
#endif // QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#include "qdockwidget.moc"
#include "moc_qdockwidget.cpp"
#include "moc_qdockwidget_p.cpp"
