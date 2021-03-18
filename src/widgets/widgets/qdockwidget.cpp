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

#include "qdockwidget.h"

#include <qaction.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
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
#include <private/qstylesheetstyle_p.h>
#include <qpa/qplatformtheme.h>

#include "qdockwidget_p.h"
#include "qmainwindowlayout_p.h"

QT_BEGIN_NAMESPACE

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

    void enterEvent(QEvent *event) override;
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
    case QEvent::ScreenChangeInternal:
        m_iconSize = -1;
        break;
    default:
        break;
    }
    return QAbstractButton::event(event);
}

static inline bool isWindowsStyle(const QStyle *style)
{
    // Note: QStyleSheetStyle inherits QWindowsStyle
    const QStyle *effectiveStyle = style;

#if QT_CONFIG(style_stylesheet)
    if (style->inherits("QStyleSheetStyle"))
      effectiveStyle = static_cast<const QStyleSheetStyle *>(style)->baseStyle();
#endif
#if !defined(QT_NO_STYLE_PROXY)
    if (style->inherits("QProxyStyle"))
      effectiveStyle = static_cast<const QProxyStyle *>(style)->baseStyle();
#endif

    return effectiveStyle->inherits("QWindowsStyle");
}

QSize QDockWidgetTitleButton::dockButtonIconSize() const
{
    if (m_iconSize < 0) {
        m_iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
        // Dock Widget title buttons on Windows where historically limited to size 10
        // (from small icon size 16) since only a 10x10 XPM was provided.
        // Adding larger pixmaps to the icons thus caused the icons to grow; limit
        // this to qpiScaled(10) here.
        if (isWindowsStyle(style()))
            m_iconSize = qMin((10 * logicalDpiX()) / 96, m_iconSize);
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

void QDockWidgetTitleButton::enterEvent(QEvent *event)
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
    QPainter p(this);

    QStyleOptionToolButton opt;
    opt.init(this);
    opt.state |= QStyle::State_AutoRaise;

    if (style()->styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame, nullptr, this))
    {
        if (isEnabled() && underMouse() && !isChecked() && !isDown())
            opt.state |= QStyle::State_Raised;
        if (isChecked())
            opt.state |= QStyle::State_On;
        if (isDown())
            opt.state |= QStyle::State_Sunken;
        style()->drawPrimitive(QStyle::PE_PanelButtonTool, &opt, &p, this);
    }

    opt.icon = icon();
    opt.subControls = { };
    opt.activeSubControls = { };
    opt.features = QStyleOptionToolButton::None;
    opt.arrowType = Qt::NoArrow;
    opt.iconSize = dockButtonIconSize();
    style()->drawComplexControl(QStyle::CC_ToolButton, &opt, &p, this);
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
    static const bool xcb = !QGuiApplication::platformName().compare(QLatin1String("xcb"), Qt::CaseInsensitive);
    return !xcb;
#endif
}

/*! \internal
   Returns true if the dock widget managed by this layout should have a native
   window decoration or if Qt needs to draw it. The \a floating parameter
   overrides the floating current state of the dock widget.
 */
bool QDockWidgetLayout::nativeWindowDeco(bool floating) const
{
    return wmSupportsNativeWindowDeco() && floating && item_list.at(QDockWidgetLayout::TitleBar) == 0;
}


void QDockWidgetLayout::addItem(QLayoutItem*)
{
    qWarning("QDockWidgetLayout::addItem(): please use QDockWidgetLayout::setWidget()");
    return;
}

QLayoutItem *QDockWidgetLayout::itemAt(int index) const
{
    int cnt = 0;
    for (int i = 0; i < item_list.count(); ++i) {
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
    for (int i = 0; i < item_list.count(); ++i) {
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
    for (int i = 0; i < item_list.count(); ++i) {
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
    button->setObjectName(QLatin1String("qt_dockwidget_floatbutton"));
    QObject::connect(button, SIGNAL(clicked()), q, SLOT(_q_toggleTopLevel()));
    layout->setWidgetForRole(QDockWidgetLayout::FloatButton, button);

    button = new QDockWidgetTitleButton(q);
    button->setObjectName(QLatin1String("qt_dockwidget_closebutton"));
    QObject::connect(button, SIGNAL(clicked()), q, SLOT(close()));
    layout->setWidgetForRole(QDockWidgetLayout::CloseButton, button);

    font = QApplication::font("QDockWidgetTitle");

#ifndef QT_NO_ACTION
    toggleViewAction = new QAction(q);
    toggleViewAction->setCheckable(true);
    toggleViewAction->setMenuRole(QAction::NoRole);
    fixedWindowTitle = qt_setWindowTitle_helperHelper(q->windowTitle(), q);
    toggleViewAction->setText(fixedWindowTitle);
    QObject::connect(toggleViewAction, SIGNAL(triggered(bool)),
                        q, SLOT(_q_toggleView(bool)));
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

void QDockWidgetPrivate::_q_toggleView(bool b)
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
#ifndef QT_NO_ACCESSIBILITY
    //: Accessible name for button undocking a dock widget (floating state)
    button->setAccessibleName(QDockWidget::tr("Float"));
    button->setAccessibleDescription(QDockWidget::tr("Undocks and re-attaches the dock widget"));
#endif
    button
        = qobject_cast <QAbstractButton*>(dwLayout->widgetForRole(QDockWidgetLayout::CloseButton));
    button->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarCloseButton, &opt, q));
    button->setVisible(canClose && !hideButtons);
#ifndef QT_NO_ACCESSIBILITY
    //: Accessible name for button closing a dock widget
    button->setAccessibleName(QDockWidget::tr("Close"));
    button->setAccessibleDescription(QDockWidget::tr("Closes the dock widget"));
#endif

    layout->invalidate();
}

void QDockWidgetPrivate::_q_toggleTopLevel()
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
void QDockWidgetPrivate::startDrag(bool group)
{
    Q_Q(QDockWidget);

    if (state == nullptr || state->dragging)
        return;

    QMainWindowLayout *layout = qt_mainwindow_layout_from_dock(q);
    Q_ASSERT(layout != nullptr);

    state->widgetItem = layout->unplug(q, group);
    if (state->widgetItem == nullptr) {
        /* I have a QMainWindow parent, but I was never inserted with
            QMainWindow::addDockWidget, so the QMainWindowLayout has no
            widget item for me. :( I have to create it myself, and then
            delete it if I don't get dropped into a dock area. */
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
}

/*! \internal
    Ends the drag end drop operation of the QDockWidget.
    The \a abort parameter specifies that it ends because of programmatic state
    reset rather than mouse release event.
 */
void QDockWidgetPrivate::endDrag(bool abort)
{
    Q_Q(QDockWidget);
    Q_ASSERT(state != nullptr);

    q->releaseMouse();

    if (state->dragging) {
        const QMainWindow *mainWindow = mainwindow_from_dock(q);
        Q_ASSERT(mainWindow != nullptr);
        QMainWindowLayout *mwLayout = qt_mainwindow_layout(mainWindow);

        if (abort || !mwLayout->plug(state->widgetItem)) {
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
                    tabPosition = mwLayout->tabPosition(mainWindow->dockWidgetArea(q));
#endif
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

void QDockWidgetPrivate::setResizerActive(bool active)
{
    Q_Q(QDockWidget);
    if (active && !resizer) {
        resizer = new QWidgetResizeHandler(q);
        resizer->setMovingEnabled(false);
    }
    if (resizer)
        resizer->setActive(QWidgetResizeHandler::Resize, active);
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
            !titleArea.contains(event->pos()) ||
            // check if the tool window is movable... do nothing if it
            // is not (but allow moving if the window is floating)
            (!hasFeature(this, QDockWidget::DockWidgetMovable) && !q->isFloating()) ||
            (qobject_cast<QMainWindow*>(parent) == 0 && !floatingTab) ||
            isAnimating() || state != nullptr) {
            return false;
        }

        initDrag(event->pos(), false);

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

        if (event->button() == Qt::LeftButton && titleArea.contains(event->pos()) &&
            hasFeature(this, QDockWidget::DockWidgetFloatable)) {
            _q_toggleTopLevel();
            return true;
        }
    }
    return false;
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
            && (event->pos() - state->pressPos).manhattanLength()
                > QApplication::startDragDistance()) {
            startDrag();
            q->grabMouse();
            ret = true;
        }
    }

    if (state->dragging && !state->nca) {
        QMargins windowMargins = q->window()->windowHandle()->frameMargins();
        QPoint windowMarginOffset = QPoint(windowMargins.left(), windowMargins.top());
        QPoint pos = event->globalPos() - state->pressPos - windowMarginOffset;

        QDockWidgetGroupWindow *floatingTab = qobject_cast<QDockWidgetGroupWindow*>(parent);
        if (floatingTab && !q->isFloating())
            floatingTab->move(pos);
        else
            q->move(pos);

        if (state && !state->ctrlDrag)
            mwlayout->hover(state->widgetItem, event->globalPos());

        ret = true;
    }

#endif // QT_CONFIG(mainwindow)
    return ret;
}

bool QDockWidgetPrivate::mouseReleaseEvent(QMouseEvent *event)
{
#if QT_CONFIG(mainwindow)

    if (event->button() == Qt::LeftButton && state && !state->nca) {
        endDrag();
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
            if (!titleRect.contains(event->globalPos()))
                break;
            if (state != nullptr)
                break;
            if (qobject_cast<QMainWindow*>(parent) == 0 && qobject_cast<QDockWidgetGroupWindow*>(parent) == 0)
                break;
            if (isAnimating())
                break;
            initDrag(event->pos(), true);
            if (state == nullptr)
                break;
            state->ctrlDrag = (event->modifiers() & Qt::ControlModifier) ||
                              (!hasFeature(this, QDockWidget::DockWidgetMovable) && q->isFloating());
            startDrag();
            break;
        case QEvent::NonClientAreaMouseMove:
            if (state == nullptr || !state->dragging)
                break;

#ifndef Q_OS_MAC
            if (state->nca) {
                endDrag();
            }
#endif
            break;
        case QEvent::NonClientAreaMouseButtonRelease:
#ifdef Q_OS_MAC
                        if (state)
                                endDrag();
#endif
                        break;
        case QEvent::NonClientAreaMouseButtonDblClick:
            _q_toggleTopLevel();
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

    if (!q->isWindow() && qobject_cast<QDockWidgetGroupWindow*>(parent) == 0)
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
    setWindowState(true, true, r);
}

void QDockWidgetPrivate::plug(const QRect &rect)
{
    setWindowState(false, false, rect);
}

void QDockWidgetPrivate::setWindowState(bool floating, bool unplug, const QRect &rect)
{
    Q_Q(QDockWidget);

    if (!floating && parent) {
        QMainWindowLayout *mwlayout = qt_mainwindow_layout_from_dock(q);
        if (mwlayout && mwlayout->dockWidgetArea(q) == Qt::NoDockWidgetArea
                && !qobject_cast<QDockWidgetGroupWindow *>(parent))
            return; // this dockwidget can't be redocked
    }

    bool wasFloating = q->isFloating();
    if (wasFloating) // Prevent repetitive unplugging from nested invocations (QTBUG-42818)
        unplug = false;
    bool hidden = q->isHidden();

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

    if (unplug)
        flags |= Qt::X11BypassWindowManagerHint;

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

    \sa QMainWindow, {Dock Widgets Example}
*/

/*!
    \enum QDockWidget::DockWidgetFeature

    \value DockWidgetClosable   The dock widget can be closed. On some systems the dock
                                widget always has a close button when it's floating
                                (for example on MacOS 10.5).
    \value DockWidgetMovable    The dock widget can be moved between docks
                                by the user.
    \value DockWidgetFloatable  The dock widget can be detached from the
                                main window, and floated as an independent
                                window.
    \value DockWidgetVerticalTitleBar The dock widget displays a vertical title
                                  bar on its left side. This can be used to
                                  increase the amount of vertical space in
                                  a QMainWindow.
    \value AllDockWidgetFeatures  (Deprecated) The dock widget can be closed, moved,
                                  and floated. Since new features might be added in future
                                  releases, the look and behavior of dock widgets might
                                  change if you use this flag. Please specify individual
                                  flags instead.
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
{ }

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
        if (floatingTab && !isFloating())
            floatingTab->adjustFlags();
        else
            d->setWindowState(true /*floating*/, true /*unplug*/);  //this ensures the native decoration is drawn
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

    A floating dock widget is presented to the user as an independent
    window "on top" of its parent QMainWindow, instead of being
    docked in the QMainWindow.

    By default, this property is \c true.

    When this property changes, the \c {topLevelChanged()} signal is emitted.

    \sa isWindow(), topLevelChanged()
*/
void QDockWidget::setFloating(bool floating)
{
    Q_D(QDockWidget);

    // the initial click of a double-click may have started a drag...
    if (d->state != nullptr)
        d->endDrag(true);

    QRect r = d->undockedGeometry;
    // Keep position when undocking for the first time.
    if (floating && isVisible() && !r.isValid())
        r = QRect(mapToGlobal(QPoint(0, 0)), size());

    d->setWindowState(floating, false, floating ? r : QRect());

    if (floating && r.isNull()) {
        if (x() < 0 || y() < 0) //may happen if we have been hidden
            move(QPoint());
        setAttribute(Qt::WA_Moved, false); //we want it at the default position
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
    case QEvent::ModifiedChange:
    case QEvent::WindowTitleChange:
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
        d->endDrag(true);
    QWidget::closeEvent(event);
}

/*! \reimp */
void QDockWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
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
            framOpt.init(this);
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
        d->toggleViewAction->setChecked(false);
        emit visibilityChanged(false);
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
            onTop = siblings.count() > 0 && siblings.last() == (QObject*)this;
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
        // (and Qt::AA_EnableHighDpiScaling is enabled). If that happens we should
        // update state->pressPos, otherwise it will be outside the window when the window shrinks.
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
*/

/*!
    \fn void QDockWidget::dockLocationChanged(Qt::DockWidgetArea area)
    \since 4.3

    This signal is emitted when the dock widget is moved to another
    dock \a area, or is moved to a different location in its current
    dock area. This happens when the dock widget is moved
    programmatically or is dragged to a new location by the user.
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
        d->setWindowState(true /*floating*/, true /*unplug*/);
    }
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

QT_END_NAMESPACE

#include "qdockwidget.moc"
#include "moc_qdockwidget.cpp"
#include "moc_qdockwidget_p.cpp"
