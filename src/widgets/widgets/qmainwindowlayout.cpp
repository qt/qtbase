/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
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

#include "qmainwindowlayout_p.h"

#if QT_CONFIG(dockwidget)
#include "qdockarealayout_p.h"
#include "qdockwidget.h"
#include "qdockwidget_p.h"
#endif
#if QT_CONFIG(toolbar)
#include "qtoolbar_p.h"
#include "qtoolbar.h"
#include "qtoolbarlayout_p.h"
#endif
#include "qmainwindow.h"
#include "qwidgetanimator_p.h"
#if QT_CONFIG(rubberband)
#include "qrubberband.h"
#endif
#if QT_CONFIG(tabbar)
#include "qtabbar_p.h"
#endif

#include <qapplication.h>
#if QT_CONFIG(statusbar)
#include <qstatusbar.h>
#endif
#include <qstring.h>
#include <qstyle.h>
#include <qstylepainter.h>
#include <qvarlengtharray.h>
#include <qstack.h>
#include <qmap.h>
#include <qtimer.h>
#include <qpointer.h>

#ifndef QT_NO_DEBUG_STREAM
#  include <qdebug.h>
#  include <qtextstream.h>
#endif

#include <private/qmenu_p.h>
#include <private/qapplication_p.h>
#include <private/qlayoutengine_p.h>
#include <private/qwidgetresizehandler_p.h>

QT_BEGIN_NAMESPACE

extern QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *window);

/******************************************************************************
** debug
*/

#if QT_CONFIG(dockwidget) && !defined(QT_NO_DEBUG_STREAM)

static void dumpLayout(QTextStream &qout, const QDockAreaLayoutInfo &layout, QString indent);

static void dumpLayout(QTextStream &qout, const QDockAreaLayoutItem &item, QString indent)
{
    qout << indent << "QDockAreaLayoutItem: "
            << "pos: " << item.pos << " size:" << item.size
            << " gap:" << (item.flags & QDockAreaLayoutItem::GapItem)
            << " keepSize:" << (item.flags & QDockAreaLayoutItem::KeepSize) << '\n';
    indent += QLatin1String("  ");
    if (item.widgetItem != 0) {
        qout << indent << "widget: "
            << item.widgetItem->widget()->metaObject()->className()
            << " \"" << item.widgetItem->widget()->windowTitle() << "\"\n";
    } else if (item.subinfo != 0) {
        qout << indent << "subinfo:\n";
        dumpLayout(qout, *item.subinfo, indent + QLatin1String("  "));
    } else if (item.placeHolderItem != 0) {
        QRect r = item.placeHolderItem->topLevelRect;
        qout << indent << "placeHolder: "
            << "pos: " << item.pos << " size:" << item.size
            << " gap:" << (item.flags & QDockAreaLayoutItem::GapItem)
            << " keepSize:" << (item.flags & QDockAreaLayoutItem::KeepSize)
            << " objectName:" << item.placeHolderItem->objectName
            << " hidden:" << item.placeHolderItem->hidden
            << " window:" << item.placeHolderItem->window
            << " rect:" << r.x() << ',' << r.y() << ' '
            << r.width() << 'x' << r.height() << '\n';
    }
}

static void dumpLayout(QTextStream &qout, const QDockAreaLayoutInfo &layout, QString indent)
{
    const QSize minSize = layout.minimumSize();
    qout << indent << "QDockAreaLayoutInfo: "
            << layout.rect.left() << ','
            << layout.rect.top() << ' '
            << layout.rect.width() << 'x'
            << layout.rect.height()
            << " min size: " << minSize.width() << ',' << minSize.height()
            << " orient:" << layout.o
#if QT_CONFIG(tabbar)
            << " tabbed:" << layout.tabbed
            << " tbshape:" << layout.tabBarShape
#endif
            << '\n';

    indent += QLatin1String("  ");

    for (int i = 0; i < layout.item_list.count(); ++i) {
        qout << indent << "Item: " << i << '\n';
        dumpLayout(qout, layout.item_list.at(i), indent + QLatin1String("  "));
    }
}

static void dumpLayout(QTextStream &qout, const QDockAreaLayout &layout)
{
    qout << "QDockAreaLayout: "
            << layout.rect.left() << ','
            << layout.rect.top() << ' '
            << layout.rect.width() << 'x'
            << layout.rect.height() << '\n';

    qout << "TopDockArea:\n";
    dumpLayout(qout, layout.docks[QInternal::TopDock], QLatin1String("  "));
    qout << "LeftDockArea:\n";
    dumpLayout(qout, layout.docks[QInternal::LeftDock], QLatin1String("  "));
    qout << "RightDockArea:\n";
    dumpLayout(qout, layout.docks[QInternal::RightDock], QLatin1String("  "));
    qout << "BottomDockArea:\n";
    dumpLayout(qout, layout.docks[QInternal::BottomDock], QLatin1String("  "));
}

QDebug operator<<(QDebug debug, const QDockAreaLayout &layout)
{
    QString s;
    QTextStream str(&s);
    dumpLayout(str, layout);
    debug << s;
    return debug;
}

QDebug operator<<(QDebug debug, const QMainWindowLayout *layout)
{
    debug << layout->layoutState.dockAreaLayout;
    return debug;
}

#endif // QT_CONFIG(dockwidget) && !defined(QT_NO_DEBUG)

/******************************************************************************
 ** QDockWidgetGroupWindow
 */
// QDockWidgetGroupWindow is the floating window containing several QDockWidgets floating together.
// (QMainWindow::GroupedDragging feature)
// QDockWidgetGroupLayout is the layout of that window and use a QDockAreaLayoutInfo to layout
// the QDockWidgets inside it.
// If there is only one QDockWidgets, or all QDockWidgets are tabbed together, it is equivalent
// of a floating QDockWidget (the title of the QDockWidget is the title of the window). But if there
// are nested QDockWidget, an additional title bar is there.
#if QT_CONFIG(dockwidget)
class QDockWidgetGroupLayout : public QLayout,
                               public QMainWindowLayoutSeparatorHelper<QDockWidgetGroupLayout>
{
    QWidgetResizeHandler *resizer;
public:
    QDockWidgetGroupLayout(QDockWidgetGroupWindow* parent) : QLayout(parent) {
        setSizeConstraint(QLayout::SetMinAndMaxSize);
        resizer = new QWidgetResizeHandler(parent);
        resizer->setMovingEnabled(false);
    }
    ~QDockWidgetGroupLayout() {
        layoutState.deleteAllLayoutItems();
    }

    void addItem(QLayoutItem*) override { Q_UNREACHABLE(); }
    int count() const override { return 0; }
    QLayoutItem* itemAt(int index) const override
    {
        int x = 0;
        return layoutState.itemAt(&x, index);
    }
    QLayoutItem* takeAt(int index) override
    {
        int x = 0;
        QLayoutItem *ret = layoutState.takeAt(&x, index);
        if (savedState.rect.isValid() && ret->widget()) {
            // we need to remove the item also from the saved state to prevent crash
            QList<int> path = savedState.indexOf(ret->widget());
            if (!path.isEmpty())
                savedState.remove(path);
            // Also, the item may be contained several times as a gap item.
            path = layoutState.indexOf(ret->widget());
            if (!path.isEmpty())
                layoutState.remove(path);
        }
        return ret;
    }
    QSize sizeHint() const override
    {
        int fw = frameWidth();
        return layoutState.sizeHint() + QSize(fw, fw);
    }
    QSize minimumSize() const override
    {
        int fw = frameWidth();
        return layoutState.minimumSize() + QSize(fw, fw);
    }
    QSize maximumSize() const override
    {
        int fw = frameWidth();
        return layoutState.maximumSize() + QSize(fw, fw);
    }
    void setGeometry(const QRect&r) override
    {
        groupWindow()->destroyOrHideIfEmpty();
        QDockAreaLayoutInfo *li = dockAreaLayoutInfo();
        if (li->isEmpty())
            return;
        int fw = frameWidth();
#if QT_CONFIG(tabbar)
        li->reparentWidgets(parentWidget());
#endif
        li->rect = r.adjusted(fw, fw, -fw, -fw);
        li->fitItems();
        li->apply(false);
        if (savedState.rect.isValid())
            savedState.rect = li->rect;
        resizer->setActive(QWidgetResizeHandler::Resize, !nativeWindowDeco());
    }

    QDockAreaLayoutInfo *dockAreaLayoutInfo() { return &layoutState; }

    bool nativeWindowDeco() const
    {
        return groupWindow()->hasNativeDecos();
    }

    int frameWidth() const
    {
        return nativeWindowDeco() ? 0 :
            parentWidget()->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, parentWidget());
    }

    QDockWidgetGroupWindow *groupWindow() const
    {
        return static_cast<QDockWidgetGroupWindow *>(parent());
    }

    QDockAreaLayoutInfo layoutState;
    QDockAreaLayoutInfo savedState;
};

bool QDockWidgetGroupWindow::event(QEvent *e)
{
    auto lay = static_cast<QDockWidgetGroupLayout *>(layout());
    if (lay && lay->windowEvent(e))
        return true;

    switch (e->type()) {
    case QEvent::Close:
        // Forward the close to the QDockWidget just as if its close button was pressed
        if (QDockWidget *dw = activeTabbedDockWidget()) {
            e->ignore();
            dw->close();
            adjustFlags();
        }
        return true;
    case QEvent::Move:
        // Let QDockWidgetPrivate::moseEvent handle the dragging
        if (QDockWidget *dw = activeTabbedDockWidget())
            static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(dw))->moveEvent(static_cast<QMoveEvent*>(e));
        return true;
    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
        // Let the QDockWidgetPrivate of the currently visible dock widget handle the drag and drop
        if (QDockWidget *dw = activeTabbedDockWidget())
            static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(dw))->nonClientAreaMouseEvent(static_cast<QMouseEvent*>(e));
        return true;
    case QEvent::ChildAdded:
        if (qobject_cast<QDockWidget *>(static_cast<QChildEvent*>(e)->child()))
            adjustFlags();
        break;
    case QEvent::LayoutRequest:
        // We might need to show the widget again
        destroyOrHideIfEmpty();
        break;
    case QEvent::Resize:
        updateCurrentGapRect();
        emit resized();
    default:
        break;
    }
    return QWidget::event(e);
}

void QDockWidgetGroupWindow::paintEvent(QPaintEvent *)
{
    QDockWidgetGroupLayout *lay = static_cast<QDockWidgetGroupLayout *>(layout());
    bool nativeDeco = lay->nativeWindowDeco();

    if (!nativeDeco) {
        QStyleOptionFrame framOpt;
        framOpt.init(this);
        QStylePainter p(this);
        p.drawPrimitive(QStyle::PE_FrameDockWidget, framOpt);
    }
}

QDockAreaLayoutInfo *QDockWidgetGroupWindow::layoutInfo() const
{
    return static_cast<QDockWidgetGroupLayout *>(layout())->dockAreaLayoutInfo();
}

/*! \internal
    If this is a floating tab bar returns the currently the QDockWidgetGroupWindow that contains
    tab, otherwise, return nullptr;
    \note: if there is only one QDockWidget, it's still considered as a floating tab
 */
const QDockAreaLayoutInfo *QDockWidgetGroupWindow::tabLayoutInfo() const
{
    const QDockAreaLayoutInfo *info = layoutInfo();
    while (info && !info->tabbed) {
        // There should be only one tabbed subinfo otherwise we are not a floating tab but a real
        // window
        const QDockAreaLayoutInfo *next = nullptr;
        bool isSingle = false;
        for (const auto &item : info->item_list) {
            if (item.skip() || (item.flags & QDockAreaLayoutItem::GapItem))
                continue;
            if (next || isSingle) // Two visible things
                return nullptr;
            if (item.subinfo)
                next = item.subinfo;
            else if (item.widgetItem)
                isSingle = true;
        }
        if (isSingle)
            return info;
        info = next;
    }
    return info;
}

/*! \internal
    If this is a floating tab bar returns the currently active QDockWidget, otherwise nullptr
 */
QDockWidget *QDockWidgetGroupWindow::activeTabbedDockWidget() const
{
    QDockWidget *dw = nullptr;
#if QT_CONFIG(tabbar)
    const QDockAreaLayoutInfo *info = tabLayoutInfo();
    if (!info)
        return nullptr;
    if (info->tabBar && info->tabBar->currentIndex() >= 0) {
        int i = info->tabIndexToListIndex(info->tabBar->currentIndex());
        if (i >= 0) {
            const QDockAreaLayoutItem &item = info->item_list.at(i);
            if (item.widgetItem)
                dw = qobject_cast<QDockWidget *>(item.widgetItem->widget());
        }
    }
    if (!dw) {
        for (int i = 0; !dw && i < info->item_list.count(); ++i) {
            const QDockAreaLayoutItem &item = info->item_list.at(i);
            if (item.skip())
                continue;
            if (!item.widgetItem)
                continue;
            dw = qobject_cast<QDockWidget *>(item.widgetItem->widget());
        }
    }
#endif
    return dw;
}

/*! \internal
    Destroy or hide this window if there is no more QDockWidget in it.
    Otherwise make sure it is shown.
 */
void QDockWidgetGroupWindow::destroyOrHideIfEmpty()
{
    if (!layoutInfo()->isEmpty()) {
        show(); // It might have been hidden,
        return;
    }
    // There might still be placeholders
    if (!layoutInfo()->item_list.isEmpty()) {
        hide();
        return;
    }

    // Make sure to reparent the possibly floating or hidden QDockWidgets to the parent
    foreach (QDockWidget *dw, findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
        bool wasFloating = dw->isFloating();
        bool wasHidden = dw->isHidden();
        dw->setParent(parentWidget());
        if (wasFloating) {
            dw->setFloating(true);
        } else {
            // maybe it was hidden, we still have to put it back in the main layout.
            QMainWindowLayout *ml =
                qt_mainwindow_layout(static_cast<QMainWindow *>(parentWidget()));
            Qt::DockWidgetArea area = ml->dockWidgetArea(this);
            if (area == Qt::NoDockWidgetArea)
                area = Qt::LeftDockWidgetArea;
            static_cast<QMainWindow *>(parentWidget())->addDockWidget(area, dw);
        }
        if (!wasHidden)
            dw->show();
    }
#if QT_CONFIG(tabbar)
    foreach (QTabBar *tb, findChildren<QTabBar *>(QString(), Qt::FindDirectChildrenOnly))
        tb->setParent(parentWidget());
#endif
    deleteLater();
}

/*! \internal
    Sets the flags of this window in accordance to the capabilities of the dock widgets
 */
void QDockWidgetGroupWindow::adjustFlags()
{
    Qt::WindowFlags oldFlags = windowFlags();
    Qt::WindowFlags flags = oldFlags;

    QDockWidget *top = activeTabbedDockWidget();
    if (!top) { // nested tabs, show window decoration
        flags =
            ((oldFlags & ~Qt::FramelessWindowHint) | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    } else if (static_cast<QDockWidgetGroupLayout *>(layout())->nativeWindowDeco()) {
        flags |= Qt::CustomizeWindowHint | Qt::WindowTitleHint;
        flags.setFlag(Qt::WindowCloseButtonHint, top->features() & QDockWidget::DockWidgetClosable);
        flags &= ~Qt::FramelessWindowHint;
    } else {
        flags &= ~(Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        flags |= Qt::FramelessWindowHint;
    }

    if (oldFlags != flags) {
        if (!windowHandle())
            create(); // The desired geometry is forgotten if we call setWindowFlags before having a window
        setWindowFlags(flags);
        const bool gainedNativeDecos = (oldFlags & Qt::FramelessWindowHint) && !(flags & Qt::FramelessWindowHint);
        const bool lostNativeDecos = !(oldFlags & Qt::FramelessWindowHint) && (flags & Qt::FramelessWindowHint);

        // Adjust the geometry after gaining/losing decos, so that the client area appears always
        // at the same place when tabbing
        if (lostNativeDecos) {
            QRect newGeometry = geometry();
            newGeometry.setTop(frameGeometry().top());
            const int bottomFrame = geometry().top() - frameGeometry().top();
            m_removedFrameSize = QSize((frameSize() - size()).width(), bottomFrame);
            setGeometry(newGeometry);
        } else if (gainedNativeDecos && m_removedFrameSize.isValid()) {
            QRect r = geometry();
            r.adjust(-m_removedFrameSize.width() / 2, 0,
                     -m_removedFrameSize.width() / 2, -m_removedFrameSize.height());
            setGeometry(r);
            m_removedFrameSize = QSize();
        }

        show(); // setWindowFlags hides the window
    }

    QWidget *titleBarOf = top ? top : parentWidget();
    setWindowTitle(titleBarOf->windowTitle());
    setWindowIcon(titleBarOf->windowIcon());
}

bool QDockWidgetGroupWindow::hasNativeDecos() const
{
    QDockWidget *dw = activeTabbedDockWidget();
    if (!dw) // We have a group of nested QDockWidgets (not just floating tabs)
        return true;

    if (!QDockWidgetLayout::wmSupportsNativeWindowDeco())
        return false;

    return dw->titleBarWidget() == nullptr;
}

/*
    The given widget is hovered over this floating group.
    This function will save the state and create a gap in the actual state.
    currentGapRect and currentGapPos will be set.
    One must call restore() or apply() after this function.
    Returns true if there was any change in the currentGapPos
 */
bool QDockWidgetGroupWindow::hover(QLayoutItem *widgetItem, const QPoint &mousePos)
{
    QDockAreaLayoutInfo &savedState = static_cast<QDockWidgetGroupLayout *>(layout())->savedState;
    if (savedState.isEmpty())
        savedState = *layoutInfo();

    QMainWindow::DockOptions opts = static_cast<QMainWindow *>(parentWidget())->dockOptions();
    bool nestingEnabled =
        (opts & QMainWindow::AllowNestedDocks) && !(opts & QMainWindow::ForceTabbedDocks);
    QDockAreaLayoutInfo::TabMode tabMode =
        nestingEnabled ? QDockAreaLayoutInfo::AllowTabs : QDockAreaLayoutInfo::ForceTabs;
    if (auto group = qobject_cast<QDockWidgetGroupWindow *>(widgetItem->widget())) {
        if (!group->tabLayoutInfo())
            tabMode = QDockAreaLayoutInfo::NoTabs;
    }

    QDockAreaLayoutInfo newState = savedState;
    if (newState.tabbed) {
        // insertion into a top-level tab
        newState.item_list = { QDockAreaLayoutItem(new QDockAreaLayoutInfo(newState)) };
        newState.item_list.first().size = pick(savedState.o, savedState.rect.size());
        newState.tabbed = false;
        newState.tabBar = nullptr;
    }

    auto newGapPos = newState.gapIndex(mousePos, nestingEnabled, tabMode);
    Q_ASSERT(!newGapPos.isEmpty());
    if (newGapPos == currentGapPos)
        return false; // gap is already there
    currentGapPos = newGapPos;
    newState.insertGap(currentGapPos, widgetItem);
    newState.fitItems();
    *layoutInfo() = std::move(newState);
    updateCurrentGapRect();
    layoutInfo()->apply(opts & QMainWindow::AnimatedDocks);
    return true;
}

void QDockWidgetGroupWindow::updateCurrentGapRect()
{
    if (!currentGapPos.isEmpty())
        currentGapRect = layoutInfo()->info(currentGapPos)->itemRect(currentGapPos.last(), true);
}

/*
    Remove the gap that was created by hover()
 */
void QDockWidgetGroupWindow::restore()
{
    QDockAreaLayoutInfo &savedState = static_cast<QDockWidgetGroupLayout *>(layout())->savedState;
    if (!savedState.isEmpty()) {
        *layoutInfo() = savedState;
        savedState = QDockAreaLayoutInfo();
    }
    currentGapRect = QRect();
    currentGapPos.clear();
    adjustFlags();
    layoutInfo()->fitItems();
    layoutInfo()->apply(static_cast<QMainWindow *>(parentWidget())->dockOptions()
                        & QMainWindow::AnimatedDocks);
}

/*
    Apply the state  that was created by hover
 */
void QDockWidgetGroupWindow::apply()
{
    static_cast<QDockWidgetGroupLayout *>(layout())->savedState.clear();
    currentGapRect = QRect();
    layoutInfo()->plug(currentGapPos);
    currentGapPos.clear();
    adjustFlags();
    layoutInfo()->apply(false);
}

#endif

/******************************************************************************
** QMainWindowLayoutState
*/

// we deal with all the #ifndefferry here so QMainWindowLayout code is clean

QMainWindowLayoutState::QMainWindowLayoutState(QMainWindow *win)
    :
#if QT_CONFIG(toolbar)
    toolBarAreaLayout(win),
#endif
#if QT_CONFIG(dockwidget)
    dockAreaLayout(win)
#else
    centralWidgetItem(0)
#endif

{
    mainWindow = win;
}

QSize QMainWindowLayoutState::sizeHint() const
{

    QSize result(0, 0);

#if QT_CONFIG(dockwidget)
    result = dockAreaLayout.sizeHint();
#else
    if (centralWidgetItem != 0)
        result = centralWidgetItem->sizeHint();
#endif

#if QT_CONFIG(toolbar)
    result = toolBarAreaLayout.sizeHint(result);
#endif // QT_CONFIG(toolbar)

    return result;
}

QSize QMainWindowLayoutState::minimumSize() const
{
    QSize result(0, 0);

#if QT_CONFIG(dockwidget)
    result = dockAreaLayout.minimumSize();
#else
    if (centralWidgetItem != 0)
        result = centralWidgetItem->minimumSize();
#endif

#if QT_CONFIG(toolbar)
    result = toolBarAreaLayout.minimumSize(result);
#endif // QT_CONFIG(toolbar)

    return result;
}

void QMainWindowLayoutState::apply(bool animated)
{
#if QT_CONFIG(toolbar)
    toolBarAreaLayout.apply(animated);
#endif

#if QT_CONFIG(dockwidget)
//    dumpLayout(dockAreaLayout, QString());
    dockAreaLayout.apply(animated);
#else
    if (centralWidgetItem != 0) {
        QMainWindowLayout *layout = qt_mainwindow_layout(mainWindow);
        Q_ASSERT(layout != 0);
        layout->widgetAnimator.animate(centralWidgetItem->widget(), centralWidgetRect, animated);
    }
#endif
}

void QMainWindowLayoutState::fitLayout()
{
    QRect r;
#if !QT_CONFIG(toolbar)
    r = rect;
#else
    toolBarAreaLayout.rect = rect;
    r = toolBarAreaLayout.fitLayout();
#endif // QT_CONFIG(toolbar)

#if QT_CONFIG(dockwidget)
    dockAreaLayout.rect = r;
    dockAreaLayout.fitLayout();
#else
    centralWidgetRect = r;
#endif
}

void QMainWindowLayoutState::deleteAllLayoutItems()
{
#if QT_CONFIG(toolbar)
    toolBarAreaLayout.deleteAllLayoutItems();
#endif

#if QT_CONFIG(dockwidget)
    dockAreaLayout.deleteAllLayoutItems();
#endif
}

void QMainWindowLayoutState::deleteCentralWidgetItem()
{
#if QT_CONFIG(dockwidget)
    delete dockAreaLayout.centralWidgetItem;
    dockAreaLayout.centralWidgetItem = 0;
#else
    delete centralWidgetItem;
    centralWidgetItem = 0;
#endif
}

QLayoutItem *QMainWindowLayoutState::itemAt(int index, int *x) const
{
#if QT_CONFIG(toolbar)
    if (QLayoutItem *ret = toolBarAreaLayout.itemAt(x, index))
        return ret;
#endif

#if QT_CONFIG(dockwidget)
    if (QLayoutItem *ret = dockAreaLayout.itemAt(x, index))
        return ret;
#else
    if (centralWidgetItem != 0 && (*x)++ == index)
        return centralWidgetItem;
#endif

    return 0;
}

QLayoutItem *QMainWindowLayoutState::takeAt(int index, int *x)
{
#if QT_CONFIG(toolbar)
    if (QLayoutItem *ret = toolBarAreaLayout.takeAt(x, index))
        return ret;
#endif

#if QT_CONFIG(dockwidget)
    if (QLayoutItem *ret = dockAreaLayout.takeAt(x, index))
        return ret;
#else
    if (centralWidgetItem != 0 && (*x)++ == index) {
        QLayoutItem *ret = centralWidgetItem;
        centralWidgetItem = 0;
        return ret;
    }
#endif

    return 0;
}

QList<int> QMainWindowLayoutState::indexOf(QWidget *widget) const
{
    QList<int> result;

#if QT_CONFIG(toolbar)
    // is it a toolbar?
    if (QToolBar *toolBar = qobject_cast<QToolBar*>(widget)) {
        result = toolBarAreaLayout.indexOf(toolBar);
        if (!result.isEmpty())
            result.prepend(0);
        return result;
    }
#endif

#if QT_CONFIG(dockwidget)
    // is it a dock widget?
    if (qobject_cast<QDockWidget *>(widget) || qobject_cast<QDockWidgetGroupWindow *>(widget)) {
        result = dockAreaLayout.indexOf(widget);
        if (!result.isEmpty())
            result.prepend(1);
        return result;
    }
#endif // QT_CONFIG(dockwidget)

    return result;
}

bool QMainWindowLayoutState::contains(QWidget *widget) const
{
#if QT_CONFIG(dockwidget)
    if (dockAreaLayout.centralWidgetItem != 0 && dockAreaLayout.centralWidgetItem->widget() == widget)
        return true;
    if (!dockAreaLayout.indexOf(widget).isEmpty())
        return true;
#else
    if (centralWidgetItem != 0 && centralWidgetItem->widget() == widget)
        return true;
#endif

#if QT_CONFIG(toolbar)
    if (!toolBarAreaLayout.indexOf(widget).isEmpty())
        return true;
#endif
    return false;
}

void QMainWindowLayoutState::setCentralWidget(QWidget *widget)
{
    QLayoutItem *item = 0;
    //make sure we remove the widget
    deleteCentralWidgetItem();

    if (widget != 0)
        item = new QWidgetItemV2(widget);

#if QT_CONFIG(dockwidget)
    dockAreaLayout.centralWidgetItem = item;
#else
    centralWidgetItem = item;
#endif
}

QWidget *QMainWindowLayoutState::centralWidget() const
{
    QLayoutItem *item = 0;

#if QT_CONFIG(dockwidget)
    item = dockAreaLayout.centralWidgetItem;
#else
    item = centralWidgetItem;
#endif

    if (item != 0)
        return item->widget();
    return 0;
}

QList<int> QMainWindowLayoutState::gapIndex(QWidget *widget,
                                            const QPoint &pos) const
{
    QList<int> result;

#if QT_CONFIG(toolbar)
    // is it a toolbar?
    if (qobject_cast<QToolBar*>(widget) != 0) {
        result = toolBarAreaLayout.gapIndex(pos);
        if (!result.isEmpty())
            result.prepend(0);
        return result;
    }
#endif

#if QT_CONFIG(dockwidget)
    // is it a dock widget?
    if (qobject_cast<QDockWidget *>(widget) != 0
            || qobject_cast<QDockWidgetGroupWindow *>(widget)) {
        bool disallowTabs = false;
#if QT_CONFIG(tabbar)
        if (auto *group = qobject_cast<QDockWidgetGroupWindow *>(widget)) {
            if (!group->tabLayoutInfo()) // Disallow to drop nested docks as a tab
                disallowTabs = true;
        }
#endif
        result = dockAreaLayout.gapIndex(pos, disallowTabs);
        if (!result.isEmpty())
            result.prepend(1);
        return result;
    }
#endif // QT_CONFIG(dockwidget)

    return result;
}

bool QMainWindowLayoutState::insertGap(const QList<int> &path, QLayoutItem *item)
{
    if (path.isEmpty())
        return false;

    int i = path.first();

#if QT_CONFIG(toolbar)
    if (i == 0) {
        Q_ASSERT(qobject_cast<QToolBar*>(item->widget()) != 0);
        return toolBarAreaLayout.insertGap(path.mid(1), item);
    }
#endif

#if QT_CONFIG(dockwidget)
    if (i == 1) {
        Q_ASSERT(qobject_cast<QDockWidget*>(item->widget()) || qobject_cast<QDockWidgetGroupWindow*>(item->widget()));
        return dockAreaLayout.insertGap(path.mid(1), item);
    }
#endif // QT_CONFIG(dockwidget)

    return false;
}

void QMainWindowLayoutState::remove(const QList<int> &path)
{
    int i = path.first();

#if QT_CONFIG(toolbar)
    if (i == 0)
        toolBarAreaLayout.remove(path.mid(1));
#endif

#if QT_CONFIG(dockwidget)
    if (i == 1)
        dockAreaLayout.remove(path.mid(1));
#endif // QT_CONFIG(dockwidget)
}

void QMainWindowLayoutState::remove(QLayoutItem *item)
{
#if QT_CONFIG(toolbar)
    toolBarAreaLayout.remove(item);
#endif

#if QT_CONFIG(dockwidget)
    // is it a dock widget?
    if (QDockWidget *dockWidget = qobject_cast<QDockWidget *>(item->widget())) {
        QList<int> path = dockAreaLayout.indexOf(dockWidget);
        if (!path.isEmpty())
            dockAreaLayout.remove(path);
    }
#endif // QT_CONFIG(dockwidget)
}

void QMainWindowLayoutState::clear()
{
#if QT_CONFIG(toolbar)
    toolBarAreaLayout.clear();
#endif

#if QT_CONFIG(dockwidget)
    dockAreaLayout.clear();
#else
    centralWidgetRect = QRect();
#endif

    rect = QRect();
}

bool QMainWindowLayoutState::isValid() const
{
    return rect.isValid();
}

QLayoutItem *QMainWindowLayoutState::item(const QList<int> &path)
{
    int i = path.first();

#if QT_CONFIG(toolbar)
    if (i == 0) {
        const QToolBarAreaLayoutItem *tbItem = toolBarAreaLayout.item(path.mid(1));
        Q_ASSERT(tbItem);
        return tbItem->widgetItem;
    }
#endif

#if QT_CONFIG(dockwidget)
    if (i == 1)
        return dockAreaLayout.item(path.mid(1)).widgetItem;
#endif // QT_CONFIG(dockwidget)

    return 0;
}

QRect QMainWindowLayoutState::itemRect(const QList<int> &path) const
{
    int i = path.first();

#if QT_CONFIG(toolbar)
    if (i == 0)
        return toolBarAreaLayout.itemRect(path.mid(1));
#endif

#if QT_CONFIG(dockwidget)
    if (i == 1)
        return dockAreaLayout.itemRect(path.mid(1));
#endif // QT_CONFIG(dockwidget)

    return QRect();
}

QRect QMainWindowLayoutState::gapRect(const QList<int> &path) const
{
    int i = path.first();

#if QT_CONFIG(toolbar)
    if (i == 0)
        return toolBarAreaLayout.itemRect(path.mid(1));
#endif

#if QT_CONFIG(dockwidget)
    if (i == 1)
        return dockAreaLayout.gapRect(path.mid(1));
#endif // QT_CONFIG(dockwidget)

    return QRect();
}

QLayoutItem *QMainWindowLayoutState::plug(const QList<int> &path)
{
    int i = path.first();

#if QT_CONFIG(toolbar)
    if (i == 0)
        return toolBarAreaLayout.plug(path.mid(1));
#endif

#if QT_CONFIG(dockwidget)
    if (i == 1)
        return dockAreaLayout.plug(path.mid(1));
#endif // QT_CONFIG(dockwidget)

    return 0;
}

QLayoutItem *QMainWindowLayoutState::unplug(const QList<int> &path, QMainWindowLayoutState *other)
{
    int i = path.first();

#if !QT_CONFIG(toolbar)
    Q_UNUSED(other);
#else
    if (i == 0)
        return toolBarAreaLayout.unplug(path.mid(1), other ? &other->toolBarAreaLayout : 0);
#endif

#if QT_CONFIG(dockwidget)
    if (i == 1)
        return dockAreaLayout.unplug(path.mid(1));
#endif // QT_CONFIG(dockwidget)

    return 0;
}

void QMainWindowLayoutState::saveState(QDataStream &stream) const
{
#if QT_CONFIG(dockwidget)
    dockAreaLayout.saveState(stream);
#if QT_CONFIG(tabbar)
    QList<QDockWidgetGroupWindow *> floatingTabs =
        mainWindow->findChildren<QDockWidgetGroupWindow *>(QString(), Qt::FindDirectChildrenOnly);

    foreach (QDockWidgetGroupWindow *floating, floatingTabs) {
        if (floating->layoutInfo()->isEmpty())
            continue;
        stream << uchar(QDockAreaLayout::FloatingDockWidgetTabMarker) << floating->geometry();
        floating->layoutInfo()->saveState(stream);
    }
#endif
#endif
#if QT_CONFIG(toolbar)
    toolBarAreaLayout.saveState(stream);
#endif
}

template <typename T>
static QList<T> findChildrenHelper(const QObject *o)
{
    const QObjectList &list = o->children();
    QList<T> result;

    for (int i=0; i < list.size(); ++i) {
        if (T t = qobject_cast<T>(list[i])) {
            result.append(t);
        }
    }

    return result;
}

#if QT_CONFIG(dockwidget)
static QList<QDockWidget*> allMyDockWidgets(const QWidget *mainWindow)
{
    QList<QDockWidget*> result;
    for (QObject *c : mainWindow->children()) {
        if (auto *dw = qobject_cast<QDockWidget*>(c)) {
            result.append(dw);
        } else if (auto *gw = qobject_cast<QDockWidgetGroupWindow*>(c)) {
            for (QObject *c : gw->children()) {
                if (auto *dw = qobject_cast<QDockWidget*>(c))
                    result.append(dw);
            }
        }
    }

    return result;
}
#endif // QT_CONFIG(dockwidget)

//pre4.3 tests the format that was used before 4.3
bool QMainWindowLayoutState::checkFormat(QDataStream &stream)
{
    while (!stream.atEnd()) {
        uchar marker;
        stream >> marker;
        switch(marker)
        {
#if QT_CONFIG(toolbar)
            case QToolBarAreaLayout::ToolBarStateMarker:
            case QToolBarAreaLayout::ToolBarStateMarkerEx:
                {
                    QList<QToolBar *> toolBars = findChildrenHelper<QToolBar*>(mainWindow);
                    if (!toolBarAreaLayout.restoreState(stream, toolBars, marker, true /*testing*/)) {
                            return false;
                    }
                }
                break;
#endif // QT_CONFIG(toolbar)

#if QT_CONFIG(dockwidget)
            case QDockAreaLayout::DockWidgetStateMarker:
                {
                    const auto dockWidgets = allMyDockWidgets(mainWindow);
                    if (!dockAreaLayout.restoreState(stream, dockWidgets, true /*testing*/)) {
                        return false;
                    }
                }
                break;
#if QT_CONFIG(tabbar)
            case QDockAreaLayout::FloatingDockWidgetTabMarker:
                {
                    QRect geom;
                    stream >> geom;
                    QDockAreaLayoutInfo info;
                    auto dockWidgets = allMyDockWidgets(mainWindow);
                    if (!info.restoreState(stream, dockWidgets, true /* testing*/))
                        return false;
                }
                break;
#endif // QT_CONFIG(tabbar)
#endif // QT_CONFIG(dockwidget)
            default:
                //there was an error during the parsing
                return false;
        }// switch
    } //while

    //everything went fine: it must be a pre-4.3 saved state
    return true;
}

bool QMainWindowLayoutState::restoreState(QDataStream &_stream,
                                        const QMainWindowLayoutState &oldState)
{
    //make a copy of the data so that we can read it more than once
    QByteArray copy;
    while(!_stream.atEnd()) {
        int length = 1024;
        QByteArray ba(length, '\0');
        length = _stream.readRawData(ba.data(), ba.size());
        ba.resize(length);
        copy += ba;
    }

    QDataStream ds(copy);
    if (!checkFormat(ds))
        return false;

    QDataStream stream(copy);

    while (!stream.atEnd()) {
        uchar marker;
        stream >> marker;
        switch(marker)
        {
#if QT_CONFIG(dockwidget)
            case QDockAreaLayout::DockWidgetStateMarker:
                {
                    const auto dockWidgets = allMyDockWidgets(mainWindow);
                    if (!dockAreaLayout.restoreState(stream, dockWidgets))
                        return false;

                    for (int i = 0; i < dockWidgets.size(); ++i) {
                        QDockWidget *w = dockWidgets.at(i);
                        QList<int> path = dockAreaLayout.indexOf(w);
                        if (path.isEmpty()) {
                            QList<int> oldPath = oldState.dockAreaLayout.indexOf(w);
                            if (oldPath.isEmpty()) {
                                continue;
                            }
                            QDockAreaLayoutInfo *info = dockAreaLayout.info(oldPath);
                            if (info == 0) {
                                continue;
                            }
                            info->item_list.append(QDockAreaLayoutItem(new QDockWidgetItem(w)));
                        }
                    }
                }
                break;
#if QT_CONFIG(tabwidget)
            case QDockAreaLayout::FloatingDockWidgetTabMarker:
            {
                auto dockWidgets = allMyDockWidgets(mainWindow);
                QDockWidgetGroupWindow* floatingTab = qt_mainwindow_layout(mainWindow)->createTabbedDockWindow();
                *floatingTab->layoutInfo() = QDockAreaLayoutInfo(&dockAreaLayout.sep, QInternal::LeftDock,
                                                                 Qt::Horizontal, QTabBar::RoundedSouth, mainWindow);
                QRect geometry;
                stream >> geometry;
                QDockAreaLayoutInfo *info = floatingTab->layoutInfo();
                if (!info->restoreState(stream, dockWidgets, false))
                    return false;
                geometry = QDockAreaLayout::constrainedRect(geometry, floatingTab);
                floatingTab->move(geometry.topLeft());
                floatingTab->resize(geometry.size());

                // Don't show an empty QDockWidgetGroupWindow if no dock widget is available yet.
                // reparentWidgets() would be triggered by show(), so do it explicitly here.
                if (info->onlyHasPlaceholders())
                    info->reparentWidgets(floatingTab);
                else
                    floatingTab->show();
            }
            break;
#endif // QT_CONFIG(tabwidget)
#endif // QT_CONFIG(dockwidget)

#if QT_CONFIG(toolbar)
            case QToolBarAreaLayout::ToolBarStateMarker:
            case QToolBarAreaLayout::ToolBarStateMarkerEx:
                {
                    QList<QToolBar *> toolBars = findChildrenHelper<QToolBar*>(mainWindow);
                    if (!toolBarAreaLayout.restoreState(stream, toolBars, marker))
                        return false;

                    for (int i = 0; i < toolBars.size(); ++i) {
                        QToolBar *w = toolBars.at(i);
                        QList<int> path = toolBarAreaLayout.indexOf(w);
                        if (path.isEmpty()) {
                            QList<int> oldPath = oldState.toolBarAreaLayout.indexOf(w);
                            if (oldPath.isEmpty()) {
                                continue;
                            }
                            toolBarAreaLayout.docks[oldPath.at(0)].insertToolBar(0, w);
                        }
                    }
                }
                break;
#endif // QT_CONFIG(toolbar)
            default:
                return false;
        }// switch
    } //while


    return true;
}

/******************************************************************************
** QMainWindowLayoutState - toolbars
*/

#if QT_CONFIG(toolbar)

static inline void validateToolBarArea(Qt::ToolBarArea &area)
{
    switch (area) {
    case Qt::LeftToolBarArea:
    case Qt::RightToolBarArea:
    case Qt::TopToolBarArea:
    case Qt::BottomToolBarArea:
        break;
    default:
        area = Qt::TopToolBarArea;
    }
}

static QInternal::DockPosition toDockPos(Qt::ToolBarArea area)
{
    switch (area) {
        case Qt::LeftToolBarArea: return QInternal::LeftDock;
        case Qt::RightToolBarArea: return QInternal::RightDock;
        case Qt::TopToolBarArea: return QInternal::TopDock;
        case Qt::BottomToolBarArea: return QInternal::BottomDock;
        default:
            break;
    }

    return QInternal::DockCount;
}

static Qt::ToolBarArea toToolBarArea(QInternal::DockPosition pos)
{
    switch (pos) {
        case QInternal::LeftDock:   return Qt::LeftToolBarArea;
        case QInternal::RightDock:  return Qt::RightToolBarArea;
        case QInternal::TopDock:    return Qt::TopToolBarArea;
        case QInternal::BottomDock: return Qt::BottomToolBarArea;
        default: break;
    }
    return Qt::NoToolBarArea;
}

static inline Qt::ToolBarArea toToolBarArea(int pos)
{
    return toToolBarArea(static_cast<QInternal::DockPosition>(pos));
}

void QMainWindowLayout::addToolBarBreak(Qt::ToolBarArea area)
{
    validateToolBarArea(area);

    layoutState.toolBarAreaLayout.addToolBarBreak(toDockPos(area));
    if (savedState.isValid())
        savedState.toolBarAreaLayout.addToolBarBreak(toDockPos(area));

    invalidate();
}

void QMainWindowLayout::insertToolBarBreak(QToolBar *before)
{
    layoutState.toolBarAreaLayout.insertToolBarBreak(before);
    if (savedState.isValid())
        savedState.toolBarAreaLayout.insertToolBarBreak(before);
    invalidate();
}

void QMainWindowLayout::removeToolBarBreak(QToolBar *before)
{
    layoutState.toolBarAreaLayout.removeToolBarBreak(before);
    if (savedState.isValid())
        savedState.toolBarAreaLayout.removeToolBarBreak(before);
    invalidate();
}

void QMainWindowLayout::moveToolBar(QToolBar *toolbar, int pos)
{
    layoutState.toolBarAreaLayout.moveToolBar(toolbar, pos);
    if (savedState.isValid())
        savedState.toolBarAreaLayout.moveToolBar(toolbar, pos);
    invalidate();
}

/* Removes the toolbar from the mainwindow so that it can be added again. Does not
   explicitly hide the toolbar. */
void QMainWindowLayout::removeToolBar(QToolBar *toolbar)
{
    if (toolbar) {
        QObject::disconnect(parentWidget(), SIGNAL(iconSizeChanged(QSize)),
                   toolbar, SLOT(_q_updateIconSize(QSize)));
        QObject::disconnect(parentWidget(), SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
                   toolbar, SLOT(_q_updateToolButtonStyle(Qt::ToolButtonStyle)));

        removeWidget(toolbar);
    }
}

/*!
    Adds \a toolbar to \a area, continuing the current line.
*/
void QMainWindowLayout::addToolBar(Qt::ToolBarArea area,
                                   QToolBar *toolbar,
                                   bool)
{
    validateToolBarArea(area);
    // let's add the toolbar to the layout
    addChildWidget(toolbar);
    QLayoutItem *item = layoutState.toolBarAreaLayout.addToolBar(toDockPos(area), toolbar);
    if (savedState.isValid() && item) {
        // copy the toolbar also in the saved state
        savedState.toolBarAreaLayout.insertItem(toDockPos(area), item);
    }
    invalidate();

    // this ensures that the toolbar has the right window flags (not floating any more)
    toolbar->d_func()->updateWindowFlags(false /*floating*/);
}

/*!
    Adds \a toolbar before \a before
*/
void QMainWindowLayout::insertToolBar(QToolBar *before, QToolBar *toolbar)
{
    addChildWidget(toolbar);
    QLayoutItem *item = layoutState.toolBarAreaLayout.insertToolBar(before, toolbar);
    if (savedState.isValid() && item) {
        // copy the toolbar also in the saved state
        savedState.toolBarAreaLayout.insertItem(before, item);
    }
    if (!currentGapPos.isEmpty() && currentGapPos.constFirst() == 0) {
        currentGapPos = layoutState.toolBarAreaLayout.currentGapIndex();
        if (!currentGapPos.isEmpty()) {
            currentGapPos.prepend(0);
            currentGapRect = layoutState.itemRect(currentGapPos);
        }
    }
    invalidate();
}

Qt::ToolBarArea QMainWindowLayout::toolBarArea(QToolBar *toolbar) const
{
    QInternal::DockPosition pos = layoutState.toolBarAreaLayout.findToolBar(toolbar);
    switch (pos) {
        case QInternal::LeftDock:   return Qt::LeftToolBarArea;
        case QInternal::RightDock:  return Qt::RightToolBarArea;
        case QInternal::TopDock:    return Qt::TopToolBarArea;
        case QInternal::BottomDock: return Qt::BottomToolBarArea;
        default: break;
    }
    return Qt::NoToolBarArea;
}

bool QMainWindowLayout::toolBarBreak(QToolBar *toolBar) const
{
    return layoutState.toolBarAreaLayout.toolBarBreak(toolBar);
}

void QMainWindowLayout::getStyleOptionInfo(QStyleOptionToolBar *option, QToolBar *toolBar) const
{
    option->toolBarArea = toolBarArea(toolBar);
    layoutState.toolBarAreaLayout.getStyleOptionInfo(option, toolBar);
}

void QMainWindowLayout::toggleToolBarsVisible()
{
    layoutState.toolBarAreaLayout.visible = !layoutState.toolBarAreaLayout.visible;
    if (!layoutState.mainWindow->isMaximized()) {
        QPoint topLeft = parentWidget()->geometry().topLeft();
        QRect r = parentWidget()->geometry();
        r = layoutState.toolBarAreaLayout.rectHint(r);
        r.moveTo(topLeft);
        parentWidget()->setGeometry(r);
    } else {
        update();
    }
}

#endif // QT_CONFIG(toolbar)

/******************************************************************************
** QMainWindowLayoutState - dock areas
*/

#if QT_CONFIG(dockwidget)

static QInternal::DockPosition toDockPos(Qt::DockWidgetArea area)
{
    switch (area) {
        case Qt::LeftDockWidgetArea: return QInternal::LeftDock;
        case Qt::RightDockWidgetArea: return QInternal::RightDock;
        case Qt::TopDockWidgetArea: return QInternal::TopDock;
        case Qt::BottomDockWidgetArea: return QInternal::BottomDock;
        default:
            break;
    }

    return QInternal::DockCount;
}

static Qt::DockWidgetArea toDockWidgetArea(QInternal::DockPosition pos)
{
    switch (pos) {
        case QInternal::LeftDock : return Qt::LeftDockWidgetArea;
        case QInternal::RightDock : return Qt::RightDockWidgetArea;
        case QInternal::TopDock : return Qt::TopDockWidgetArea;
        case QInternal::BottomDock : return Qt::BottomDockWidgetArea;
        default:
            break;
    }

    return Qt::NoDockWidgetArea;
}

inline static Qt::DockWidgetArea toDockWidgetArea(int pos)
{
    return toDockWidgetArea(static_cast<QInternal::DockPosition>(pos));
}

void QMainWindowLayout::setCorner(Qt::Corner corner, Qt::DockWidgetArea area)
{
    if (layoutState.dockAreaLayout.corners[corner] == area)
        return;
    layoutState.dockAreaLayout.corners[corner] = area;
    if (savedState.isValid())
        savedState.dockAreaLayout.corners[corner] = area;
    invalidate();
}

Qt::DockWidgetArea QMainWindowLayout::corner(Qt::Corner corner) const
{
    return layoutState.dockAreaLayout.corners[corner];
}

void QMainWindowLayout::addDockWidget(Qt::DockWidgetArea area,
                                             QDockWidget *dockwidget,
                                             Qt::Orientation orientation)
{
    addChildWidget(dockwidget);

    // If we are currently moving a separator, then we need to abort the move, since each
    // time we move the mouse layoutState is replaced by savedState modified by the move.
    if (!movingSeparator.isEmpty())
        endSeparatorMove(movingSeparatorPos);

    layoutState.dockAreaLayout.addDockWidget(toDockPos(area), dockwidget, orientation);
    emit dockwidget->dockLocationChanged(area);
    invalidate();
}

void QMainWindowLayout::tabifyDockWidget(QDockWidget *first, QDockWidget *second)
{
    addChildWidget(second);
    layoutState.dockAreaLayout.tabifyDockWidget(first, second);
    emit second->dockLocationChanged(dockWidgetArea(first));
    invalidate();
}

bool QMainWindowLayout::restoreDockWidget(QDockWidget *dockwidget)
{
    addChildWidget(dockwidget);
    if (!layoutState.dockAreaLayout.restoreDockWidget(dockwidget))
        return false;
    emit dockwidget->dockLocationChanged(dockWidgetArea(dockwidget));
    invalidate();
    return true;
}

#if QT_CONFIG(tabbar)
bool QMainWindowLayout::documentMode() const
{
    return _documentMode;
}

void QMainWindowLayout::setDocumentMode(bool enabled)
{
    if (_documentMode == enabled)
        return;

    _documentMode = enabled;

    // Update the document mode for all tab bars
    foreach (QTabBar *bar, usedTabBars)
        bar->setDocumentMode(_documentMode);
    foreach (QTabBar *bar, unusedTabBars)
        bar->setDocumentMode(_documentMode);
}
#endif // QT_CONFIG(tabbar)

void QMainWindowLayout::setVerticalTabsEnabled(bool enabled)
{
#if !QT_CONFIG(tabbar)
    Q_UNUSED(enabled);
#else
    if (verticalTabsEnabled == enabled)
        return;

    verticalTabsEnabled = enabled;

    updateTabBarShapes();
#endif // QT_CONFIG(tabbar)
}

#if QT_CONFIG(tabwidget)
QTabWidget::TabShape QMainWindowLayout::tabShape() const
{
    return _tabShape;
}

void QMainWindowLayout::setTabShape(QTabWidget::TabShape tabShape)
{
    if (_tabShape == tabShape)
        return;

    _tabShape = tabShape;

    updateTabBarShapes();
}

QTabWidget::TabPosition QMainWindowLayout::tabPosition(Qt::DockWidgetArea area) const
{
    return tabPositions[toDockPos(area)];
}

void QMainWindowLayout::setTabPosition(Qt::DockWidgetAreas areas, QTabWidget::TabPosition tabPosition)
{
    const Qt::DockWidgetArea dockWidgetAreas[] = {
        Qt::TopDockWidgetArea,
        Qt::LeftDockWidgetArea,
        Qt::BottomDockWidgetArea,
        Qt::RightDockWidgetArea
    };
    const QInternal::DockPosition dockPositions[] = {
        QInternal::TopDock,
        QInternal::LeftDock,
        QInternal::BottomDock,
        QInternal::RightDock
    };

    for (int i = 0; i < QInternal::DockCount; ++i)
        if (areas & dockWidgetAreas[i])
            tabPositions[dockPositions[i]] = tabPosition;

    updateTabBarShapes();
}

static inline QTabBar::Shape tabBarShapeFrom(QTabWidget::TabShape shape, QTabWidget::TabPosition position)
{
    const bool rounded = (shape == QTabWidget::Rounded);
    if (position == QTabWidget::North)
        return rounded ? QTabBar::RoundedNorth : QTabBar::TriangularNorth;
    if (position == QTabWidget::South)
        return rounded ? QTabBar::RoundedSouth : QTabBar::TriangularSouth;
    if (position == QTabWidget::East)
        return rounded ? QTabBar::RoundedEast : QTabBar::TriangularEast;
    if (position == QTabWidget::West)
        return rounded ? QTabBar::RoundedWest : QTabBar::TriangularWest;
    return QTabBar::RoundedNorth;
}
#endif // QT_CONFIG(tabwidget)

#if QT_CONFIG(tabbar)
void QMainWindowLayout::updateTabBarShapes()
{
#if QT_CONFIG(tabwidget)
    const QTabWidget::TabPosition vertical[] = {
        QTabWidget::West,
        QTabWidget::East,
        QTabWidget::North,
        QTabWidget::South
    };
#else
    const QTabBar::Shape vertical[] = {
        QTabBar::RoundedWest,
        QTabBar::RoundedEast,
        QTabBar::RoundedNorth,
        QTabBar::RoundedSouth
    };
#endif

    QDockAreaLayout &layout = layoutState.dockAreaLayout;

    for (int i = 0; i < QInternal::DockCount; ++i) {
#if QT_CONFIG(tabwidget)
        QTabWidget::TabPosition pos = verticalTabsEnabled ? vertical[i] : tabPositions[i];
        QTabBar::Shape shape = tabBarShapeFrom(_tabShape, pos);
#else
        QTabBar::Shape shape = verticalTabsEnabled ? vertical[i] : QTabBar::RoundedSouth;
#endif
        layout.docks[i].setTabBarShape(shape);
    }
}
#endif // QT_CONFIG(tabbar)

void QMainWindowLayout::splitDockWidget(QDockWidget *after,
                                        QDockWidget *dockwidget,
                                        Qt::Orientation orientation)
{
    addChildWidget(dockwidget);
    layoutState.dockAreaLayout.splitDockWidget(after, dockwidget, orientation);
    emit dockwidget->dockLocationChanged(dockWidgetArea(after));
    invalidate();
}

Qt::DockWidgetArea QMainWindowLayout::dockWidgetArea(QWidget *widget) const
{
    const QList<int> pathToWidget = layoutState.dockAreaLayout.indexOf(widget);
    if (pathToWidget.isEmpty())
        return Qt::NoDockWidgetArea;
    return toDockWidgetArea(pathToWidget.first());
}

void QMainWindowLayout::keepSize(QDockWidget *w)
{
    layoutState.dockAreaLayout.keepSize(w);
}

#if QT_CONFIG(tabbar)

// Handle custom tooltip, and allow to drag tabs away.
class QMainWindowTabBar : public QTabBar
{
    QMainWindow *mainWindow;
    QPointer<QDockWidget> draggingDock; // Currently dragging (detached) dock widget
public:
    QMainWindowTabBar(QMainWindow *parent);
protected:
    bool event(QEvent *e) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

};

QMainWindowTabBar::QMainWindowTabBar(QMainWindow *parent)
    : QTabBar(parent), mainWindow(parent)
{
    setExpanding(false);
}

void QMainWindowTabBar::mouseMoveEvent(QMouseEvent *e)
{
    // The QTabBar handles the moving (reordering) of tabs.
    // When QTabBarPrivate::dragInProgress is true, and that the mouse is outside of a region
    // around the QTabBar, we will consider the user wants to drag that QDockWidget away from this
    // tab area.

    QTabBarPrivate *d = static_cast<QTabBarPrivate*>(d_ptr.data());
    if (!draggingDock && (mainWindow->dockOptions() & QMainWindow::GroupedDragging)) {
        int offset = QApplication::startDragDistance() + 1;
        offset *= 3;
        QRect r = rect().adjusted(-offset, -offset, offset, offset);
        if (d->dragInProgress && !r.contains(e->pos()) && d->validIndex(d->pressedIndex)) {
            QMainWindowLayout* mlayout = qt_mainwindow_layout(mainWindow);
            QDockAreaLayoutInfo *info = mlayout->dockInfo(this);
            Q_ASSERT(info);
            int idx = info->tabIndexToListIndex(d->pressedIndex);
            const QDockAreaLayoutItem &item = info->item_list.at(idx);
            if (item.widgetItem
                    && (draggingDock = qobject_cast<QDockWidget *>(item.widgetItem->widget()))) {
                // We should drag this QDockWidget away by unpluging it.
                // First cancel the QTabBar's internal move
                d->moveTabFinished(d->pressedIndex);
                d->pressedIndex = -1;
                if (d->movingTab)
                    d->movingTab->setVisible(false);
                d->dragStartPosition = QPoint();

                // Then starts the drag using QDockWidgetPrivate's API
                QDockWidgetPrivate *dockPriv = static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(draggingDock));
                QDockWidgetLayout *dwlayout = static_cast<QDockWidgetLayout *>(draggingDock->layout());
                dockPriv->initDrag(dwlayout->titleArea().center(), true);
                dockPriv->startDrag(false);
                if (dockPriv->state)
                    dockPriv->state->ctrlDrag = e->modifiers() & Qt::ControlModifier;
            }
        }
    }

    if (draggingDock) {
        QDockWidgetPrivate *dockPriv = static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(draggingDock));
        if (dockPriv->state && dockPriv->state->dragging) {
            QPoint pos = e->globalPos() - dockPriv->state->pressPos;
            draggingDock->move(pos);
            // move will call QMainWindowLayout::hover
        }
    }
    QTabBar::mouseMoveEvent(e);
}

void QMainWindowTabBar::mouseReleaseEvent(QMouseEvent *e)
{
    if (draggingDock && e->button() == Qt::LeftButton) {
        QDockWidgetPrivate *dockPriv = static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(draggingDock));
        if (dockPriv->state && dockPriv->state->dragging) {
            dockPriv->endDrag();
        }
        draggingDock = 0;
    }
    QTabBar::mouseReleaseEvent(e);
}

bool QMainWindowTabBar::event(QEvent *e)
{
    // show the tooltip if tab is too small to fit label

    if (e->type() != QEvent::ToolTip)
        return QTabBar::event(e);
    QSize size = this->size();
    QSize hint = sizeHint();
    if (shape() == QTabBar::RoundedWest || shape() == QTabBar::RoundedEast) {
        size = size.transposed();
        hint = hint.transposed();
    }
    if (size.width() < hint.width())
        return QTabBar::event(e);
    e->accept();
    return true;
}

QTabBar *QMainWindowLayout::getTabBar()
{
    QTabBar *result = 0;
    if (!unusedTabBars.isEmpty()) {
        result = unusedTabBars.takeLast();
    } else {
        result = new QMainWindowTabBar(static_cast<QMainWindow *>(parentWidget()));
        result->setDrawBase(true);
        result->setElideMode(Qt::ElideRight);
        result->setDocumentMode(_documentMode);
        result->setMovable(true);
        connect(result, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));
        connect(result, &QTabBar::tabMoved, this, &QMainWindowLayout::tabMoved);
    }

    usedTabBars.insert(result);
    return result;
}

// Allocates a new separator widget if needed
QWidget *QMainWindowLayout::getSeparatorWidget()
{
    QWidget *result = 0;
    if (!unusedSeparatorWidgets.isEmpty()) {
        result = unusedSeparatorWidgets.takeLast();
    } else {
        result = new QWidget(parentWidget());
        result->setAttribute(Qt::WA_MouseNoMask, true);
        result->setAutoFillBackground(false);
        result->setObjectName(QLatin1String("qt_qmainwindow_extended_splitter"));
    }
    usedSeparatorWidgets.insert(result);
    return result;
}

/*! \internal
    Returns a pointer QDockAreaLayoutInfo which contains this \a widget directly
    (in its internal list)
 */
QDockAreaLayoutInfo *QMainWindowLayout::dockInfo(QWidget *widget)
{
    QDockAreaLayoutInfo *info = layoutState.dockAreaLayout.info(widget);
    if (info)
        return info;
    foreach (QDockWidgetGroupWindow *dwgw,
             parent()->findChildren<QDockWidgetGroupWindow*>(QString(), Qt::FindDirectChildrenOnly)) {
        info = dwgw->layoutInfo()->info(widget);
        if (info)
            return info;
    }
    return 0;
}

void QMainWindowLayout::tabChanged()
{
    QTabBar *tb = qobject_cast<QTabBar*>(sender());
    if (tb == 0)
        return;
    QDockAreaLayoutInfo *info = dockInfo(tb);
    if (info == 0)
        return;

    QDockWidget *activated = info->apply(false);

    if (activated)
        emit static_cast<QMainWindow *>(parentWidget())->tabifiedDockWidgetActivated(activated);

    if (auto dwgw = qobject_cast<QDockWidgetGroupWindow*>(tb->parentWidget()))
        dwgw->adjustFlags();

    if (QWidget *w = centralWidget())
        w->raise();
}

void QMainWindowLayout::tabMoved(int from, int to)
{
    QTabBar *tb = qobject_cast<QTabBar*>(sender());
    Q_ASSERT(tb);
    QDockAreaLayoutInfo *info = dockInfo(tb);
    Q_ASSERT(info);

    info->moveTab(from, to);
}
#endif // QT_CONFIG(tabbar)

void QMainWindowLayout::raise(QDockWidget *widget)
{
#if QT_CONFIG(tabbar)
    QDockAreaLayoutInfo *info = dockInfo(widget);
    if (info == 0)
        return;
    if (!info->tabbed)
        return;
    info->setCurrentTab(widget);
#endif
}

#endif // QT_CONFIG(dockwidget)


/******************************************************************************
** QMainWindowLayoutState - layout interface
*/

int QMainWindowLayout::count() const
{
    qWarning("QMainWindowLayout::count: ?");
    return 0; //#################################################
}

QLayoutItem *QMainWindowLayout::itemAt(int index) const
{
    int x = 0;

    if (QLayoutItem *ret = layoutState.itemAt(index, &x))
        return ret;

    if (statusbar && x++ == index)
        return statusbar;

    return 0;
}

QLayoutItem *QMainWindowLayout::takeAt(int index)
{
    int x = 0;

    if (QLayoutItem *ret = layoutState.takeAt(index, &x)) {
        // the widget might in fact have been destroyed by now
        if (QWidget *w = ret->widget()) {
            widgetAnimator.abort(w);
            if (w == pluggingWidget)
                pluggingWidget = 0;
        }

        if (savedState.isValid() ) {
            //we need to remove the item also from the saved state to prevent crash
            savedState.remove(ret);
            //Also, the item may be contained several times as a gap item.
            layoutState.remove(ret);
        }

#if QT_CONFIG(toolbar)
        if (!currentGapPos.isEmpty() && currentGapPos.constFirst() == 0) {
            currentGapPos = layoutState.toolBarAreaLayout.currentGapIndex();
            if (!currentGapPos.isEmpty()) {
                currentGapPos.prepend(0);
                currentGapRect = layoutState.itemRect(currentGapPos);
            }
        }
#endif

        return ret;
    }

    if (statusbar && x++ == index) {
        QLayoutItem *ret = statusbar;
        statusbar = 0;
        return ret;
    }

    return 0;
}

void QMainWindowLayout::setGeometry(const QRect &_r)
{
    if (savedState.isValid())
        return;

    QRect r = _r;

    QLayout::setGeometry(r);

    if (statusbar) {
        QRect sbr(QPoint(r.left(), 0),
                  QSize(r.width(), statusbar->heightForWidth(r.width()))
                  .expandedTo(statusbar->minimumSize()));
        sbr.moveBottom(r.bottom());
        QRect vr = QStyle::visualRect(parentWidget()->layoutDirection(), _r, sbr);
        statusbar->setGeometry(vr);
        r.setBottom(sbr.top() - 1);
    }

    layoutState.rect = r;
    layoutState.fitLayout();
    applyState(layoutState, false);
}

void QMainWindowLayout::addItem(QLayoutItem *)
{ qWarning("QMainWindowLayout::addItem: Please use the public QMainWindow API instead"); }

QSize QMainWindowLayout::sizeHint() const
{
    if (!szHint.isValid()) {
        szHint = layoutState.sizeHint();
        const QSize sbHint = statusbar ? statusbar->sizeHint() : QSize(0, 0);
        szHint = QSize(qMax(sbHint.width(), szHint.width()),
                        sbHint.height() + szHint.height());
    }
    return szHint;
}

QSize QMainWindowLayout::minimumSize() const
{
    if (!minSize.isValid()) {
        minSize = layoutState.minimumSize();
        const QSize sbMin = statusbar ? statusbar->minimumSize() : QSize(0, 0);
        minSize = QSize(qMax(sbMin.width(), minSize.width()),
                        sbMin.height() + minSize.height());
    }
    return minSize;
}

void QMainWindowLayout::invalidate()
{
    QLayout::invalidate();
    minSize = szHint = QSize();
}

#if QT_CONFIG(dockwidget)
void QMainWindowLayout::setCurrentHoveredFloat(QDockWidgetGroupWindow *w)
{
    if (currentHoveredFloat != w) {
        if (currentHoveredFloat) {
            disconnect(currentHoveredFloat.data(), &QObject::destroyed,
                       this, &QMainWindowLayout::updateGapIndicator);
            disconnect(currentHoveredFloat.data(), &QDockWidgetGroupWindow::resized,
                       this, &QMainWindowLayout::updateGapIndicator);
            if (currentHoveredFloat)
                currentHoveredFloat->restore();
        } else if (w) {
            restore(true);
        }

        currentHoveredFloat = w;

        if (w) {
            connect(w, &QObject::destroyed,
                    this, &QMainWindowLayout::updateGapIndicator, Qt::UniqueConnection);
            connect(w, &QDockWidgetGroupWindow::resized,
                    this, &QMainWindowLayout::updateGapIndicator, Qt::UniqueConnection);
        }

        updateGapIndicator();
    }
}
#endif // QT_CONFIG(dockwidget)

/******************************************************************************
** QMainWindowLayout - remaining stuff
*/

static void fixToolBarOrientation(QLayoutItem *item, int dockPos)
{
#if QT_CONFIG(toolbar)
    QToolBar *toolBar = qobject_cast<QToolBar*>(item->widget());
    if (toolBar == 0)
        return;

    QRect oldGeo = toolBar->geometry();

    QInternal::DockPosition pos
        = static_cast<QInternal::DockPosition>(dockPos);
    Qt::Orientation o = pos == QInternal::TopDock || pos == QInternal::BottomDock
                        ? Qt::Horizontal : Qt::Vertical;
    if (o != toolBar->orientation())
        toolBar->setOrientation(o);

    QSize hint = toolBar->sizeHint().boundedTo(toolBar->maximumSize())
                    .expandedTo(toolBar->minimumSize());

    if (toolBar->size() != hint) {
        QRect newGeo(oldGeo.topLeft(), hint);
        if (toolBar->layoutDirection() == Qt::RightToLeft)
            newGeo.moveRight(oldGeo.right());
        toolBar->setGeometry(newGeo);
    }

#else
    Q_UNUSED(item);
    Q_UNUSED(dockPos);
#endif
}

void QMainWindowLayout::revert(QLayoutItem *widgetItem)
{
    if (!savedState.isValid())
        return;

    QWidget *widget = widgetItem->widget();
    layoutState = savedState;
    currentGapPos = layoutState.indexOf(widget);
    if (currentGapPos.isEmpty())
        return;
    fixToolBarOrientation(widgetItem, currentGapPos.at(1));
    layoutState.unplug(currentGapPos);
    layoutState.fitLayout();
    currentGapRect = layoutState.itemRect(currentGapPos);

    plug(widgetItem);
}

bool QMainWindowLayout::plug(QLayoutItem *widgetItem)
{
#if QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget) && QT_CONFIG(tabbar)
    if (currentHoveredFloat) {
        QWidget *widget = widgetItem->widget();
        QList<int> previousPath = layoutState.indexOf(widget);
        if (!previousPath.isEmpty())
            layoutState.remove(previousPath);
        previousPath = currentHoveredFloat->layoutInfo()->indexOf(widget);
        // Let's remove the widget from any possible group window
        foreach (QDockWidgetGroupWindow *dwgw,
                parent()->findChildren<QDockWidgetGroupWindow*>(QString(), Qt::FindDirectChildrenOnly)) {
            if (dwgw == currentHoveredFloat)
                continue;
            QList<int> path = dwgw->layoutInfo()->indexOf(widget);
            if (!path.isEmpty())
                dwgw->layoutInfo()->remove(path);
        }
        currentGapRect = QRect();
        currentHoveredFloat->apply();
        if (!previousPath.isEmpty())
            currentHoveredFloat->layoutInfo()->remove(previousPath);
        QRect globalRect = currentHoveredFloat->currentGapRect;
        globalRect.moveTopLeft(currentHoveredFloat->mapToGlobal(globalRect.topLeft()));
        pluggingWidget = widget;
        widgetAnimator.animate(widget, globalRect, dockOptions & QMainWindow::AnimatedDocks);
        return true;
    }
#endif

    if (!parentWidget()->isVisible() || parentWidget()->isMinimized() || currentGapPos.isEmpty())
        return false;

    fixToolBarOrientation(widgetItem, currentGapPos.at(1));

    QWidget *widget = widgetItem->widget();

#if QT_CONFIG(dockwidget)
    // Let's remove the widget from any possible group window
    foreach (QDockWidgetGroupWindow *dwgw,
            parent()->findChildren<QDockWidgetGroupWindow*>(QString(), Qt::FindDirectChildrenOnly)) {
        QList<int> path = dwgw->layoutInfo()->indexOf(widget);
        if (!path.isEmpty())
            dwgw->layoutInfo()->remove(path);
    }
#endif

    QList<int> previousPath = layoutState.indexOf(widget);

    const QLayoutItem *it = layoutState.plug(currentGapPos);
    if (!it)
        return false;
    Q_ASSERT(it == widgetItem);
    if (!previousPath.isEmpty())
        layoutState.remove(previousPath);

    pluggingWidget = widget;
    QRect globalRect = currentGapRect;
    globalRect.moveTopLeft(parentWidget()->mapToGlobal(globalRect.topLeft()));
#if QT_CONFIG(dockwidget)
    if (qobject_cast<QDockWidget*>(widget) != 0) {
        QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(widget->layout());
        if (layout->nativeWindowDeco()) {
            globalRect.adjust(0, layout->titleHeight(), 0, 0);
        } else {
            int fw = widget->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, widget);
            globalRect.adjust(-fw, -fw, fw, fw);
        }
    }
#endif
    widgetAnimator.animate(widget, globalRect, dockOptions & QMainWindow::AnimatedDocks);

    return true;
}

void QMainWindowLayout::animationFinished(QWidget *widget)
{
    //this function is called from within the Widget Animator whenever an animation is finished
    //on a certain widget
#if QT_CONFIG(toolbar)
    if (QToolBar *tb = qobject_cast<QToolBar*>(widget)) {
        QToolBarLayout *tbl = qobject_cast<QToolBarLayout*>(tb->layout());
        if (tbl->animating) {
            tbl->animating = false;
            if (tbl->expanded)
                tbl->layoutActions(tb->size());
            tb->update();
        }
    }
#endif

    if (widget == pluggingWidget) {

#if QT_CONFIG(dockwidget)
#if QT_CONFIG(tabbar)
        if (QDockWidgetGroupWindow *dwgw = qobject_cast<QDockWidgetGroupWindow *>(widget)) {
            // When the animated widget was a QDockWidgetGroupWindow, it means each of the
            // embedded QDockWidget needs to be plugged back into the QMainWindow layout.
            savedState.clear();
            QDockAreaLayoutInfo *srcInfo = dwgw->layoutInfo();
            const QDockAreaLayoutInfo *srcTabInfo = dwgw->tabLayoutInfo();
            QDockAreaLayoutInfo *dstParentInfo;
            QList<int> dstPath;

            if (currentHoveredFloat) {
                dstPath = currentHoveredFloat->layoutInfo()->indexOf(widget);
                Q_ASSERT(dstPath.size() >= 1);
                dstParentInfo = currentHoveredFloat->layoutInfo()->info(dstPath);
            } else {
                dstPath = layoutState.dockAreaLayout.indexOf(widget);
                Q_ASSERT(dstPath.size() >= 2);
                dstParentInfo = layoutState.dockAreaLayout.info(dstPath);
            }
            Q_ASSERT(dstParentInfo);
            int idx = dstPath.constLast();
            Q_ASSERT(dstParentInfo->item_list[idx].widgetItem->widget() == dwgw);
            if (dstParentInfo->tabbed && srcTabInfo) {
                // merge the two tab widgets
                delete dstParentInfo->item_list[idx].widgetItem;
                dstParentInfo->item_list.removeAt(idx);
                std::copy(srcTabInfo->item_list.cbegin(), srcTabInfo->item_list.cend(),
                          std::inserter(dstParentInfo->item_list,
                                        dstParentInfo->item_list.begin() + idx));
                quintptr currentId = srcTabInfo->currentTabId();
                *srcInfo = QDockAreaLayoutInfo();
                dstParentInfo->reparentWidgets(currentHoveredFloat ? currentHoveredFloat.data()
                                                                   : parentWidget());
                dstParentInfo->updateTabBar();
                dstParentInfo->setCurrentTabId(currentId);
            } else {
                QDockAreaLayoutItem &item = dstParentInfo->item_list[idx];
                Q_ASSERT(item.widgetItem->widget() == dwgw);
                delete item.widgetItem;
                item.widgetItem = 0;
                item.subinfo = new QDockAreaLayoutInfo(std::move(*srcInfo));
                *srcInfo = QDockAreaLayoutInfo();
                item.subinfo->reparentWidgets(currentHoveredFloat ? currentHoveredFloat.data()
                                                                  : parentWidget());
                item.subinfo->setTabBarShape(dstParentInfo->tabBarShape);
            }
            dwgw->destroyOrHideIfEmpty();
        }
#endif

        if (QDockWidget *dw = qobject_cast<QDockWidget*>(widget)) {
            dw->setParent(currentHoveredFloat ? currentHoveredFloat.data() : parentWidget());
            dw->show();
            dw->d_func()->plug(currentGapRect);
        }
#endif
#if QT_CONFIG(toolbar)
        if (QToolBar *tb = qobject_cast<QToolBar*>(widget))
            tb->d_func()->plug(currentGapRect);
#endif

        savedState.clear();
        currentGapPos.clear();
        pluggingWidget = 0;
#if QT_CONFIG(dockwidget)
        setCurrentHoveredFloat(nullptr);
#endif
        //applying the state will make sure that the currentGap is updated correctly
        //and all the geometries (especially the one from the central widget) is correct
        layoutState.apply(false);

#if QT_CONFIG(dockwidget)
#if QT_CONFIG(tabbar)
        if (qobject_cast<QDockWidget*>(widget) != 0) {
            // info() might return null if the widget is destroyed while
            // animating but before the animationFinished signal is received.
            if (QDockAreaLayoutInfo *info = dockInfo(widget))
                info->setCurrentTab(widget);
        }
#endif
#endif
    }

    if (!widgetAnimator.animating()) {
        //all animations are finished
#if QT_CONFIG(dockwidget)
        parentWidget()->update(layoutState.dockAreaLayout.separatorRegion());
#if QT_CONFIG(tabbar)
        foreach (QTabBar *tab_bar, usedTabBars)
            tab_bar->show();
#endif // QT_CONFIG(tabbar)
#endif // QT_CONFIG(dockwidget)
    }

    updateGapIndicator();
}

void QMainWindowLayout::restore(bool keepSavedState)
{
    if (!savedState.isValid())
        return;

    layoutState = savedState;
    applyState(layoutState);
    if (!keepSavedState)
        savedState.clear();
    currentGapPos.clear();
    pluggingWidget = 0;
    updateGapIndicator();
}

QMainWindowLayout::QMainWindowLayout(QMainWindow *mainwindow, QLayout *parentLayout)
    : QLayout(parentLayout ? static_cast<QWidget *>(0) : mainwindow)
    , layoutState(mainwindow)
    , savedState(mainwindow)
    , dockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks)
    , statusbar(0)
#if QT_CONFIG(dockwidget)
#if QT_CONFIG(tabbar)
    , _documentMode(false)
    , verticalTabsEnabled(false)
#if QT_CONFIG(tabwidget)
    , _tabShape(QTabWidget::Rounded)
#endif
#endif
#endif // QT_CONFIG(dockwidget)
    , widgetAnimator(this)
    , pluggingWidget(0)
{
    if (parentLayout)
        setParent(parentLayout);

#if QT_CONFIG(dockwidget)
#if QT_CONFIG(tabbar)
    sep = mainwindow->style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent, 0, mainwindow);
#endif

#if QT_CONFIG(tabwidget)
    for (int i = 0; i < QInternal::DockCount; ++i)
        tabPositions[i] = QTabWidget::South;
#endif
#endif // QT_CONFIG(dockwidget)
    pluggingWidget = 0;

    setObjectName(mainwindow->objectName() + QLatin1String("_layout"));
}

QMainWindowLayout::~QMainWindowLayout()
{
    layoutState.deleteAllLayoutItems();
    layoutState.deleteCentralWidgetItem();

    delete statusbar;
}

void QMainWindowLayout::setDockOptions(QMainWindow::DockOptions opts)
{
    if (opts == dockOptions)
        return;

    dockOptions = opts;

#if QT_CONFIG(dockwidget)
    setVerticalTabsEnabled(opts & QMainWindow::VerticalTabs);
#endif

    invalidate();
}

#if QT_CONFIG(statusbar)
QStatusBar *QMainWindowLayout::statusBar() const
{ return statusbar ? qobject_cast<QStatusBar *>(statusbar->widget()) : 0; }

void QMainWindowLayout::setStatusBar(QStatusBar *sb)
{
    if (sb)
        addChildWidget(sb);
    delete statusbar;
    statusbar = sb ? new QWidgetItemV2(sb) : 0;
    invalidate();
}
#endif // QT_CONFIG(statusbar)

QWidget *QMainWindowLayout::centralWidget() const
{
    return layoutState.centralWidget();
}

void QMainWindowLayout::setCentralWidget(QWidget *widget)
{
    if (widget != 0)
        addChildWidget(widget);
    layoutState.setCentralWidget(widget);
    if (savedState.isValid()) {
#if QT_CONFIG(dockwidget)
        savedState.dockAreaLayout.centralWidgetItem = layoutState.dockAreaLayout.centralWidgetItem;
        savedState.dockAreaLayout.fallbackToSizeHints = true;
#else
        savedState.centralWidgetItem = layoutState.centralWidgetItem;
#endif
    }
    invalidate();
}

#if QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget)
/*! \internal
  This helper function is called by QMainWindowLayout::unplug if QMainWindow::GroupedDragging is
  set and we are dragging the title bar of a non-floating QDockWidget.
  If one should unplug the whole group, do so and return true, otherwise return false.
  \a item is pointing to the QLayoutItem that holds the QDockWidget, but will be updated to the
  QLayoutItem that holds the new QDockWidgetGroupWindow if the group is unplugged.
*/
static bool unplugGroup(QMainWindowLayout *layout, QLayoutItem **item,
                        QDockAreaLayoutItem &parentItem)
{
    if (!parentItem.subinfo || !parentItem.subinfo->tabbed)
        return false;

    // The QDockWidget is part of a group of tab and we need to unplug them all.

    QDockWidgetGroupWindow *floatingTabs = layout->createTabbedDockWindow();
    QDockAreaLayoutInfo *info = floatingTabs->layoutInfo();
    *info = std::move(*parentItem.subinfo);
    delete parentItem.subinfo;
    parentItem.subinfo = nullptr;
    floatingTabs->setGeometry(info->rect.translated(layout->parentWidget()->pos()));
    floatingTabs->show();
    floatingTabs->raise();
    *item = new QDockWidgetGroupWindowItem(floatingTabs);
    parentItem.widgetItem = *item;
    return true;
}
#endif

/*! \internal
    Unplug \a widget (QDockWidget or QToolBar) from it's parent container.

    If \a group is true we might actually unplug the group of tabs this
    widget is part if QMainWindow::GroupedDragging is set. When \a group
    is false, the widget itself is always unplugged alone

    Returns the QLayoutItem of the dragged element.
    The layout item is kept in the layout but set as a gap item.
 */
QLayoutItem *QMainWindowLayout::unplug(QWidget *widget, bool group)
{
#if QT_CONFIG(dockwidget) && QT_CONFIG(tabbar)
    auto *groupWindow = qobject_cast<const QDockWidgetGroupWindow *>(widget->parentWidget());
    if (!widget->isWindow() && groupWindow) {
        if (group && groupWindow->tabLayoutInfo()) {
            // We are just dragging a floating window as it, not need to do anything, we just have to
            // look up the corresponding QWidgetItem* if it exists
            if (QDockAreaLayoutInfo *info = dockInfo(widget->parentWidget())) {
                QList<int> groupWindowPath = info->indexOf(widget->parentWidget());
                return groupWindowPath.isEmpty() ? nullptr : info->item(groupWindowPath).widgetItem;
            }
            return nullptr;
        }
        QList<int> path = groupWindow->layoutInfo()->indexOf(widget);
        QLayoutItem *item = groupWindow->layoutInfo()->item(path).widgetItem;
        if (group && path.size() > 1
            && unplugGroup(this, &item,
                           groupWindow->layoutInfo()->item(path.mid(0, path.size() - 1)))) {
            return item;
        } else {
            // We are unplugging a dock widget from a floating window.
            QDockWidget *dw = qobject_cast<QDockWidget *>(widget);
            Q_ASSERT(dw); // cannot be a QDockWidgetGroupWindow because it's not floating.
            dw->d_func()->unplug(widget->geometry());
            groupWindow->layoutInfo()->fitItems();
            groupWindow->layoutInfo()->apply(dockOptions & QMainWindow::AnimatedDocks);
            return item;
        }
    }
#endif
    QList<int> path = layoutState.indexOf(widget);
    if (path.isEmpty())
        return 0;

    QLayoutItem *item = layoutState.item(path);
    if (widget->isWindow())
        return item;

    QRect r = layoutState.itemRect(path);
    savedState = layoutState;

#if QT_CONFIG(dockwidget)
    if (QDockWidget *dw = qobject_cast<QDockWidget*>(widget)) {
        Q_ASSERT(path.constFirst() == 1);
#if QT_CONFIG(tabwidget)
        if (group && (dockOptions & QMainWindow::GroupedDragging) && path.size() > 3
            && unplugGroup(this, &item,
                           layoutState.dockAreaLayout.item(path.mid(1, path.size() - 2)))) {
            path.removeLast();
            savedState = layoutState;
        } else
#endif // QT_CONFIG(tabwidget)
        {
            dw->d_func()->unplug(r);
        }
    }
#endif // QT_CONFIG(dockwidget)
#if QT_CONFIG(toolbar)
    if (QToolBar *tb = qobject_cast<QToolBar*>(widget)) {
        tb->d_func()->unplug(r);
    }
#endif

#if !QT_CONFIG(dockwidget) || !QT_CONFIG(tabbar)
    Q_UNUSED(group);
#endif

    layoutState.unplug(path ,&savedState);
    savedState.fitLayout();
    currentGapPos = path;
    currentGapRect = r;
    updateGapIndicator();

    fixToolBarOrientation(item, currentGapPos.at(1));

    return item;
}

void QMainWindowLayout::updateGapIndicator()
{
#if QT_CONFIG(rubberband)
    if (!widgetAnimator.animating() && (!currentGapPos.isEmpty()
#if QT_CONFIG(dockwidget)
                                        || currentHoveredFloat
#endif
                                        )) {
        QWidget *expectedParent =
#if QT_CONFIG(dockwidget)
            currentHoveredFloat ? currentHoveredFloat.data() :
#endif
            parentWidget();
        if (!gapIndicator) {
            gapIndicator = new QRubberBand(QRubberBand::Rectangle, expectedParent);
            // For accessibility to identify this special widget.
            gapIndicator->setObjectName(QLatin1String("qt_rubberband"));
        } else if (gapIndicator->parent() != expectedParent) {
            gapIndicator->setParent(expectedParent);
        }

#if QT_CONFIG(dockwidget)
        if (currentHoveredFloat)
            gapIndicator->setGeometry(currentHoveredFloat->currentGapRect);
        else
#endif
            gapIndicator->setGeometry(currentGapRect);
        gapIndicator->show();
        gapIndicator->raise();
    } else if (gapIndicator) {
        gapIndicator->hide();
    }
#endif // QT_CONFIG(rubberband)
}

void QMainWindowLayout::hover(QLayoutItem *widgetItem, const QPoint &mousePos)
{
    if (!parentWidget()->isVisible() || parentWidget()->isMinimized()
        || pluggingWidget != 0 || widgetItem == 0)
        return;

    QWidget *widget = widgetItem->widget();

#if QT_CONFIG(dockwidget)
    if ((dockOptions & QMainWindow::GroupedDragging) && (qobject_cast<QDockWidget*>(widget)
            || qobject_cast<QDockWidgetGroupWindow *>(widget))) {

        // Check if we are over another floating dock widget
        QVarLengthArray<QWidget *, 10> candidates;
        foreach (QObject *c, parentWidget()->children()) {
            QWidget *w = qobject_cast<QWidget*>(c);
            if (!w)
                continue;
            if (!qobject_cast<QDockWidget*>(w) && !qobject_cast<QDockWidgetGroupWindow *>(w))
                continue;
            if (w != widget && w->isTopLevel() && w->isVisible() && !w->isMinimized())
                candidates << w;
            if (QDockWidgetGroupWindow *group = qobject_cast<QDockWidgetGroupWindow *>(w)) {
                // Sometimes, there are floating QDockWidget that have a QDockWidgetGroupWindow as a parent.
                foreach (QObject *c, group->children()) {
                    if (QDockWidget *dw = qobject_cast<QDockWidget*>(c)) {
                        if (dw != widget && dw->isFloating() && dw->isVisible() && !dw->isMinimized())
                            candidates << dw;
                    }
                }
            }
        }
        for (QWidget *w : candidates) {
            QWindow *handle1 = widget->windowHandle();
            QWindow *handle2 = w->windowHandle();
            if (handle1 && handle2 && handle1->screen() != handle2->screen())
                continue;
            if (!w->geometry().contains(mousePos))
                continue;

            if (auto dropTo = qobject_cast<QDockWidget *>(w)) {
                // dropping to a normal widget, we mutate it in a QDockWidgetGroupWindow with two
                // tabs
                QDockWidgetGroupWindow *floatingTabs = createTabbedDockWindow(); // FIXME
                floatingTabs->setGeometry(dropTo->geometry());
                QDockAreaLayoutInfo *info = floatingTabs->layoutInfo();
                *info = QDockAreaLayoutInfo(&layoutState.dockAreaLayout.sep, QInternal::LeftDock,
                                            Qt::Horizontal, QTabBar::RoundedSouth,
                                            static_cast<QMainWindow *>(parentWidget()));
                info->tabbed = true;
                QLayout *parentLayout = dropTo->parentWidget()->layout();
                info->item_list.append(
                    QDockAreaLayoutItem(parentLayout->takeAt(parentLayout->indexOf(dropTo))));

                dropTo->setParent(floatingTabs);
                dropTo->show();
                dropTo->d_func()->plug(QRect());
                w = floatingTabs;
                widget->raise(); // raise, as our newly created drop target is now on top
            }
            Q_ASSERT(qobject_cast<QDockWidgetGroupWindow *>(w));
            auto group = static_cast<QDockWidgetGroupWindow *>(w);
            if (group->hover(widgetItem, group->mapFromGlobal(mousePos))) {
                setCurrentHoveredFloat(group);
                applyState(layoutState); // update the tabbars
            }
            return;
        }
    }
    setCurrentHoveredFloat(nullptr);
    layoutState.dockAreaLayout.fallbackToSizeHints = false;
#endif // QT_CONFIG(dockwidget)

    QPoint pos = parentWidget()->mapFromGlobal(mousePos);

    if (!savedState.isValid())
        savedState = layoutState;

    QList<int> path = savedState.gapIndex(widget, pos);

    if (!path.isEmpty()) {
        bool allowed = false;

#if QT_CONFIG(dockwidget)
        if (QDockWidget *dw = qobject_cast<QDockWidget*>(widget))
            allowed = dw->isAreaAllowed(toDockWidgetArea(path.at(1)));

        if (qobject_cast<QDockWidgetGroupWindow *>(widget))
            allowed = true;
#endif
#if QT_CONFIG(toolbar)
        if (QToolBar *tb = qobject_cast<QToolBar*>(widget))
            allowed = tb->isAreaAllowed(toToolBarArea(path.at(1)));
#endif

        if (!allowed)
            path.clear();
    }

    if (path == currentGapPos)
        return; // the gap is already there

    currentGapPos = path;
    if (path.isEmpty()) {
        fixToolBarOrientation(widgetItem, 2); // 2 = top dock, ie. horizontal
        restore(true);
        return;
    }

    fixToolBarOrientation(widgetItem, currentGapPos.at(1));

    QMainWindowLayoutState newState = savedState;

    if (!newState.insertGap(path, widgetItem)) {
        restore(true); // not enough space
        return;
    }

    QSize min = newState.minimumSize();
    QSize size = newState.rect.size();

    if (min.width() > size.width() || min.height() > size.height()) {
        restore(true);
        return;
    }

    newState.fitLayout();

    currentGapRect = newState.gapRect(currentGapPos);

#if QT_CONFIG(dockwidget)
    parentWidget()->update(layoutState.dockAreaLayout.separatorRegion());
#endif
    layoutState = std::move(newState);
    applyState(layoutState);

    updateGapIndicator();
}

#if QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget)
QDockWidgetGroupWindow *QMainWindowLayout::createTabbedDockWindow()
{
    QDockWidgetGroupWindow* f = new QDockWidgetGroupWindow(parentWidget(), Qt::Tool);
    new QDockWidgetGroupLayout(f);
    return f;
}
#endif

void QMainWindowLayout::applyState(QMainWindowLayoutState &newState, bool animate)
{
#if QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget)
    QSet<QTabBar*> used = newState.dockAreaLayout.usedTabBars();
    foreach (QDockWidgetGroupWindow *dwgw,
             parent()->findChildren<QDockWidgetGroupWindow*>(QString(), Qt::FindDirectChildrenOnly)) {
        used += dwgw->layoutInfo()->usedTabBars();
    }

    QSet<QTabBar*> retired = usedTabBars - used;
    usedTabBars = used;
    foreach (QTabBar *tab_bar, retired) {
        tab_bar->hide();
        while (tab_bar->count() > 0)
            tab_bar->removeTab(0);
        unusedTabBars.append(tab_bar);
    }

    if (sep == 1) {
        QSet<QWidget*> usedSeps = newState.dockAreaLayout.usedSeparatorWidgets();
        QSet<QWidget*> retiredSeps = usedSeparatorWidgets - usedSeps;
        usedSeparatorWidgets = usedSeps;
        foreach (QWidget *sepWidget, retiredSeps) {
            unusedSeparatorWidgets.append(sepWidget);
        }
    }

    for (int i = 0; i < QInternal::DockCount; ++i)
        newState.dockAreaLayout.docks[i].reparentWidgets(parentWidget());

#endif // QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget)
    newState.apply(dockOptions & QMainWindow::AnimatedDocks && animate);
}

void QMainWindowLayout::saveState(QDataStream &stream) const
{
    layoutState.saveState(stream);
}

bool QMainWindowLayout::restoreState(QDataStream &stream)
{
    savedState = layoutState;
    layoutState.clear();
    layoutState.rect = savedState.rect;

    if (!layoutState.restoreState(stream, savedState)) {
        layoutState.deleteAllLayoutItems();
        layoutState = savedState;
        if (parentWidget()->isVisible())
            applyState(layoutState, false); // hides tabBars allocated by newState
        return false;
    }

    if (parentWidget()->isVisible()) {
        layoutState.fitLayout();
        applyState(layoutState, false);
    }

    savedState.deleteAllLayoutItems();
    savedState.clear();

#if QT_CONFIG(dockwidget)
    if (parentWidget()->isVisible()) {
#if QT_CONFIG(tabbar)
        foreach (QTabBar *tab_bar, usedTabBars)
            tab_bar->show();

#endif
    }
#endif // QT_CONFIG(dockwidget)

    return true;
}

QT_END_NAMESPACE

#include "moc_qmainwindowlayout_p.cpp"
