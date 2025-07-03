// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#if QT_CONFIG(draganddrop)
#include <qdrag.h>
#endif
#include <qmimedata.h>
#if QT_CONFIG(statusbar)
#include <qstatusbar.h>
#endif
#include <qstring.h>
#include <qstyle.h>
#include <qstylepainter.h>
#include <qvarlengtharray.h>
#include <qstack.h>
#include <qmap.h>
#include <qpointer.h>

#ifndef QT_NO_DEBUG_STREAM
#  include <qdebug.h>
#  include <qtextstream.h>
#endif

#include <private/qmenu_p.h>
#include <private/qapplication_p.h>
#include <private/qlayoutengine_p.h>
#include <private/qwidgetresizehandler_p.h>

#include <QScopedValueRollback>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
    indent += "  "_L1;
    if (item.widgetItem != nullptr) {
        qout << indent << "widget: "
            << item.widgetItem->widget()->metaObject()->className()
            << " \"" << item.widgetItem->widget()->windowTitle() << "\"\n";
    } else if (item.subinfo != nullptr) {
        qout << indent << "subinfo:\n";
        dumpLayout(qout, *item.subinfo, indent + "  "_L1);
    } else if (item.placeHolderItem != nullptr) {
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

    indent += "  "_L1;

    for (int i = 0; i < layout.item_list.size(); ++i) {
        qout << indent << "Item: " << i << '\n';
        dumpLayout(qout, layout.item_list.at(i), indent + "  "_L1);
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
    dumpLayout(qout, layout.docks[QInternal::TopDock], "  "_L1);
    qout << "LeftDockArea:\n";
    dumpLayout(qout, layout.docks[QInternal::LeftDock], "  "_L1);
    qout << "RightDockArea:\n";
    dumpLayout(qout, layout.docks[QInternal::RightDock], "  "_L1);
    qout << "BottomDockArea:\n";
    dumpLayout(qout, layout.docks[QInternal::BottomDock], "  "_L1);
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
    if (layout)
        return std::move(debug) << layout->layoutState.dockAreaLayout;
    return debug << "QMainWindowLayout(0x0)";
}

// Use this to dump item lists of all populated main window docks.
// Use DUMP macro inside QMainWindowLayout
#if 0
static void dumpItemLists(const QMainWindowLayout *layout, const char *function, const char *comment)
{
    for (int i = 0; i < QInternal::DockCount; ++i) {
        const auto &list = layout->layoutState.dockAreaLayout.docks[i].item_list;
        if (list.isEmpty())
            continue;
        qDebug() << function << comment << "Dock" << i << list;
    }
}
#define DUMP(comment) dumpItemLists(this, __FUNCTION__, comment)
#endif // 0

#endif // QT_CONFIG(dockwidget) && !defined(QT_NO_DEBUG)


/*!
    \internal
    QDockWidgetGroupWindow is a floating window, containing several QDockWidgets floating together.
    This requires QMainWindow::GroupedDragging to be enabled.
    QDockWidgets floating jointly in a QDockWidgetGroupWindow are considered to be docked.
    Their \c isFloating property is \c false.
    QDockWidget children of a QDockWidgetGroupWindow are either:
    \list
    \li tabbed (as long as Qt is compiled with the \c tabbar feature), or
    \li arranged next to each other, equivalent to the default on a main window dock.
    \endlist

    QDockWidgetGroupWindow uses QDockWidgetGroupLayout to lay out its QDockWidget children.
    It stores layout information in a QDockAreaLayoutInfo, including temporary spacer items
    and rubber bands.

    If its QDockWidget children are tabbed, the QDockWidgetGroupWindow shows the active QDockWidget's
    title as its own window title.

    QDockWidgetGroupWindow is designed to hold more than one QDockWidget.
    A QDockWidgetGroupWindow with only one QDockWidget child may occur only temporarily
    \list
    \li in its construction phase, or
    \li during a hover: While QDockWidget A is hovered over B, B is converted into a QDockWidgetGroupWindow.
    \endlist

    A QDockWidgetGroupWindow with only one QDockWidget child must never get focus, be dragged or dropped.
    To enforce this restriction, QDockWidgetGrouWindow will remove itself after its second QDockWidget
    child has been removed. It will make its last QDockWidget child a single, floating QDockWidget.
    Eventually, the empty QDockWidgetGroupWindow will call deleteLater() on itself.
*/


#if QT_CONFIG(dockwidget)
class QDockWidgetGroupLayout : public QLayout,
                               public QMainWindowLayoutSeparatorHelper<QDockWidgetGroupLayout>
{
    QWidgetResizeHandler *resizer;
public:
    QDockWidgetGroupLayout(QDockWidgetGroupWindow* parent) : QLayout(parent) {
        setSizeConstraint(QLayout::SetMinAndMaxSize);
        resizer = new QWidgetResizeHandler(parent);
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
        resizer->setEnabled(!nativeWindowDeco());
    }

    QDockAreaLayoutInfo *dockAreaLayoutInfo() { return &layoutState; }

#if QT_CONFIG(toolbar)
    QToolBarAreaLayout *toolBarAreaLayout()
    {
        return nullptr; // QDockWidgetGroupWindow doesn't have toolbars
    }
#endif

    bool nativeWindowDeco() const
    {
        return groupWindow()->hasNativeDecos();
    }

    int frameWidth() const
    {
        return nativeWindowDeco() ? 0 :
            parentWidget()->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, nullptr, parentWidget());
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
#if QT_CONFIG(tabbar)
        // Forward the close to the QDockWidget just as if its close button was pressed
        if (QDockWidget *dw = activeTabbedDockWidget()) {
            dw->close();
            adjustFlags();
        }
#endif
        return true;
    case QEvent::Move:
#if QT_CONFIG(tabbar)
        // Let QDockWidgetPrivate::moseEvent handle the dragging
        if (QDockWidget *dw = activeTabbedDockWidget())
            static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(dw))->moveEvent(static_cast<QMoveEvent*>(e));
#endif
        return true;
    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
#if QT_CONFIG(tabbar)
        // Let the QDockWidgetPrivate of the currently visible dock widget handle the drag and drop
        if (QDockWidget *dw = activeTabbedDockWidget())
            static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(dw))->nonClientAreaMouseEvent(static_cast<QMouseEvent*>(e));
#endif
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
        break;
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
        framOpt.initFrom(this);
        QStylePainter p(this);
        p.drawPrimitive(QStyle::PE_FrameDockWidget, framOpt);
    }
}

QDockAreaLayoutInfo *QDockWidgetGroupWindow::layoutInfo() const
{
    return static_cast<QDockWidgetGroupLayout *>(layout())->dockAreaLayoutInfo();
}

#if QT_CONFIG(tabbar)
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
        for (int i = 0; !dw && i < info->item_list.size(); ++i) {
            const QDockAreaLayoutItem &item = info->item_list.at(i);
            if (item.skip())
                continue;
            if (!item.widgetItem)
                continue;
            dw = qobject_cast<QDockWidget *>(item.widgetItem->widget());
        }
    }
    return dw;
}
#endif // QT_CONFIG(tabbar)

/*! \internal
    Destroy or hide this window if there is no more QDockWidget in it.
    Otherwise make sure it is shown.
 */
void QDockWidgetGroupWindow::destroyOrHideIfEmpty()
{
    const QDockAreaLayoutInfo *info = layoutInfo();
    if (!info->isEmpty()) {
        show(); // It might have been hidden,
        return;
    }
    // There might still be placeholders
    if (!info->item_list.isEmpty()) {
        hide();
        return;
    }

    // Make sure to reparent the possibly floating or hidden QDockWidgets to the parent
    const auto dockWidgetsList = dockWidgets();
    for (QDockWidget *dw : dockWidgetsList) {
        const bool wasFloating = dw->isFloating();
        const bool wasHidden = dw->isHidden();
        dw->setParent(parentWidget());
        qCDebug(lcQpaDockWidgets) << "Reparented:" << dw << "to" << parentWidget() << "by" << this;
        if (wasFloating) {
            dw->setFloating(true);
        } else {
            // maybe it was hidden, we still have to put it back in the main layout.
            QMainWindowLayout *ml =
                qt_mainwindow_layout(static_cast<QMainWindow *>(parentWidget()));
            Qt::DockWidgetArea area = ml->dockWidgetArea(this);
            if (area == Qt::NoDockWidgetArea)
                area = Qt::LeftDockWidgetArea; // FIXME: DockWidget doesn't save original docking area
            static_cast<QMainWindow *>(parentWidget())->addDockWidget(area, dw);
            qCDebug(lcQpaDockWidgets) << "Redocked to Mainwindow:" << area << dw << "by" << this;
        }
        if (!wasHidden)
            dw->show();
    }
    Q_ASSERT(qobject_cast<QMainWindow *>(parentWidget()));
    auto *mainWindow = static_cast<QMainWindow *>(parentWidget());
    QMainWindowLayout *mwLayout = qt_mainwindow_layout(mainWindow);
    QDockAreaLayoutInfo &parentInfo = mwLayout->layoutState.dockAreaLayout.docks[layoutInfo()->dockPos];
    std::unique_ptr<QLayoutItem> cleanup = parentInfo.takeWidgetItem(this);
    parentInfo.remove(this);
    deleteLater();
}

/*!
   \internal
   \return \c true if the group window has at least one visible QDockWidget child,
   otherwise false.
 */
bool QDockWidgetGroupWindow::hasVisibleDockWidgets() const
{
    const auto &children = findChildren<QDockWidget *>(Qt::FindChildrenRecursively);
    for (auto child : children) {
        // WA_WState_Visible is set on the dock widget, associated to the active tab
        // and unset on all others.
        // WA_WState_Hidden is set if the dock widgets have been explicitly hidden.
        // This is the relevant information to check (equivalent to !child->isHidden()).
        if (!child->testAttribute(Qt::WA_WState_Hidden))
            return true;
    }
    return false;
}

/*! \internal
    Sets the flags of this window in accordance to the capabilities of the dock widgets
 */
void QDockWidgetGroupWindow::adjustFlags()
{
    Qt::WindowFlags oldFlags = windowFlags();
    Qt::WindowFlags flags = oldFlags;

#if QT_CONFIG(tabbar)
    QDockWidget *top = activeTabbedDockWidget();
#else
    QDockWidget *top = nullptr;
#endif
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

        setVisible(hasVisibleDockWidgets());
    }

    QWidget *titleBarOf = top ? top : parentWidget();
    setWindowTitle(titleBarOf->windowTitle());
    setWindowIcon(titleBarOf->windowIcon());
}

bool QDockWidgetGroupWindow::hasNativeDecos() const
{
#if QT_CONFIG(tabbar)
    QDockWidget *dw = activeTabbedDockWidget();
    if (!dw) // We have a group of nested QDockWidgets (not just floating tabs)
        return true;

    if (!QDockWidgetLayout::wmSupportsNativeWindowDeco())
        return false;

    return dw->titleBarWidget() == nullptr;
#else
    return true;
#endif
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
    QDockAreaLayoutInfo newState = savedState;
    bool nestingEnabled =
        (opts & QMainWindow::AllowNestedDocks) && !(opts & QMainWindow::ForceTabbedDocks);
    QDockAreaLayoutInfo::TabMode tabMode =
#if !QT_CONFIG(tabbar)
        QDockAreaLayoutInfo::NoTabs;
#else
        nestingEnabled ? QDockAreaLayoutInfo::AllowTabs : QDockAreaLayoutInfo::ForceTabs;
    if (auto group = qobject_cast<QDockWidgetGroupWindow *>(widgetItem->widget())) {
        if (!group->tabLayoutInfo())
            tabMode = QDockAreaLayoutInfo::NoTabs;
    }
    if (newState.tabbed) {
        // insertion into a top-level tab
        newState.item_list = { QDockAreaLayoutItem(new QDockAreaLayoutInfo(newState)) };
        newState.item_list.first().size = pick(savedState.o, savedState.rect.size());
        newState.tabbed = false;
        newState.tabBar = nullptr;
    }
#endif

    auto newGapPos = newState.gapIndex(mousePos, nestingEnabled, tabMode);
    Q_ASSERT(!newGapPos.isEmpty());

    // Do not insert a new gap item, if the current position already is a gap,
    // or if the group window contains one
    if (newGapPos == currentGapPos || newState.hasGapItem(newGapPos))
        return false;

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

void QDockWidgetGroupWindow::childEvent(QChildEvent *event)
{
    switch (event->type()) {
    case QEvent::ChildRemoved:
        if (auto *dockWidget = qobject_cast<QDockWidget *>(event->child()))
            dockWidget->removeEventFilter(this);
        destroyIfSingleItemLeft();
        break;
    case QEvent::ChildAdded:
        if (auto *dockWidget = qobject_cast<QDockWidget *>(event->child()))
            dockWidget->installEventFilter(this);
        break;
    default:
        break;
    }
}

bool QDockWidgetGroupWindow::eventFilter(QObject *obj, QEvent *event)
{
    auto *dockWidget = qobject_cast<QDockWidget *>(obj);
    if (!dockWidget)
        return QWidget::eventFilter(obj, event);

    switch (event->type()) {
    case QEvent::Close:
        // We don't want closed dock widgets in a floating tab
        // => dock it to the main dock, before closing;
        reparentToMainWindow(dockWidget);
        dockWidget->setFloating(false);
        break;

    case QEvent::Hide:
        // if the dock widget is not an active tab, it is hidden anyway.
        // if it is the active tab, hide the whole group.
        if (dockWidget->isVisible())
            hide();
        break;

    default:
        break;
    }
    return QWidget::eventFilter(obj, event);
}

void QDockWidgetGroupWindow::destroyIfSingleItemLeft()
{
    const auto &dockWidgets = this->dockWidgets();

    // Handle only the last dock
    if (dockWidgets.count() != 1)
        return;

    auto *lastDockWidget = dockWidgets.at(0);

    // If the last remaining dock widget is not in the group window's item_list,
    // a group window is being docked on a main window docking area.
    // => don't interfere
    if (layoutInfo()->indexOf(lastDockWidget).isEmpty())
        return;

    Q_ASSERT(qobject_cast<QMainWindow *>(parentWidget()));
    auto *mainWindow = static_cast<QMainWindow *>(parentWidget());
    QMainWindowLayout *mwLayout = qt_mainwindow_layout(mainWindow);

    // Unplug the last remaining dock widget and hide the group window, to avoid flickering
    mwLayout->unplug(lastDockWidget, QDockWidgetPrivate::DragScope::Widget);
    lastDockWidget->setGeometry(geometry());
    hide();

    // Re-parent last dock widget
    reparentToMainWindow(lastDockWidget);

    // the group window could still have placeholder items => clear everything
    layoutInfo()->deleteAllLayoutItems();
    layoutInfo()->item_list.clear();

    destroyOrHideIfEmpty();
}

void QDockWidgetGroupWindow::reparentToMainWindow(QDockWidget *dockWidget)
{
    // reparent a dockWidget to the main window
    // - remove it from the floating dock's layout info
    // - insert it to the main dock's layout info
    // Finally, set draggingDock to nullptr, since the drag is finished.
    Q_ASSERT(qobject_cast<QMainWindow *>(parentWidget()));
    auto *mainWindow = static_cast<QMainWindow *>(parentWidget());
    QMainWindowLayout *mwLayout = qt_mainwindow_layout(mainWindow);
    Q_ASSERT(mwLayout);
    QDockAreaLayoutInfo &parentInfo = mwLayout->layoutState.dockAreaLayout.docks[layoutInfo()->dockPos];
    dockWidget->removeEventFilter(this);
    parentInfo.add(dockWidget);
    std::unique_ptr<QLayoutItem> cleanup = layoutInfo()->takeWidgetItem(dockWidget);
    layoutInfo()->remove(dockWidget);
    const bool wasFloating = dockWidget->isFloating();
    const bool wasVisible = dockWidget->isVisible();
    dockWidget->setParent(mainWindow);
    dockWidget->setFloating(wasFloating);
    dockWidget->setVisible(wasVisible);
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
    if (centralWidgetItem)
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
    if (centralWidgetItem)
        result = centralWidgetItem->minimumSize();
#endif

#if QT_CONFIG(toolbar)
    result = toolBarAreaLayout.minimumSize(result);
#endif // QT_CONFIG(toolbar)

    return result;
}

/*!
    \internal

    Returns whether the layout fits into the main window.
*/
bool QMainWindowLayoutState::fits() const
{
    Q_ASSERT(mainWindow);

    QSize size;

#if QT_CONFIG(dockwidget)
    size = dockAreaLayout.minimumStableSize();
#endif

#if QT_CONFIG(toolbar)
    size.rwidth() += toolBarAreaLayout.docks[QInternal::LeftDock].rect.width();
    size.rwidth() += toolBarAreaLayout.docks[QInternal::RightDock].rect.width();
    size.rheight() += toolBarAreaLayout.docks[QInternal::TopDock].rect.height();
    size.rheight() += toolBarAreaLayout.docks[QInternal::BottomDock].rect.height();
#endif

    return size.width() <= mainWindow->width() && size.height() <= mainWindow->height();
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
    if (centralWidgetItem) {
        QMainWindowLayout *layout = qt_mainwindow_layout(mainWindow);
        Q_ASSERT(layout);
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
    dockAreaLayout.centralWidgetItem = nullptr;
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
    if (centralWidgetItem  && (*x)++ == index)
        return centralWidgetItem;
#endif

    return nullptr;
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
    if (centralWidgetItem && (*x)++ == index) {
        QLayoutItem *ret = centralWidgetItem;
        centralWidgetItem = nullptr;
        return ret;
    }
#endif

    return nullptr;
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
    if (dockAreaLayout.centralWidgetItem != nullptr && dockAreaLayout.centralWidgetItem->widget() == widget)
        return true;
    if (!dockAreaLayout.indexOf(widget).isEmpty())
        return true;
#else
    if (centralWidgetItem && centralWidgetItem->widget() == widget)
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
    QLayoutItem *item = nullptr;
    //make sure we remove the widget
    deleteCentralWidgetItem();

    if (widget != nullptr)
        item = new QWidgetItemV2(widget);

#if QT_CONFIG(dockwidget)
    dockAreaLayout.centralWidgetItem = item;
#else
    centralWidgetItem = item;
#endif
}

QWidget *QMainWindowLayoutState::centralWidget() const
{
    QLayoutItem *item = nullptr;

#if QT_CONFIG(dockwidget)
    item = dockAreaLayout.centralWidgetItem;
#else
    item = centralWidgetItem;
#endif

    if (item != nullptr)
        return item->widget();
    return nullptr;
}

QList<int> QMainWindowLayoutState::gapIndex(QWidget *widget,
                                            const QPoint &pos) const
{
    QList<int> result;

#if QT_CONFIG(toolbar)
    // is it a toolbar?
    if (qobject_cast<QToolBar*>(widget) != nullptr) {
        result = toolBarAreaLayout.gapIndex(pos);
        if (!result.isEmpty())
            result.prepend(0);
        return result;
    }
#endif

#if QT_CONFIG(dockwidget)
    // is it a dock widget?
    if (qobject_cast<QDockWidget *>(widget) != nullptr
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
        Q_ASSERT(qobject_cast<QToolBar*>(item->widget()) != nullptr);
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

    return nullptr;
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

    return nullptr;
}

QLayoutItem *QMainWindowLayoutState::unplug(const QList<int> &path, QMainWindowLayoutState *other)
{
    int i = path.first();

#if !QT_CONFIG(toolbar)
    Q_UNUSED(other);
#else
    if (i == 0)
        return toolBarAreaLayout.unplug(path.mid(1), other ? &other->toolBarAreaLayout : nullptr);
#endif

#if QT_CONFIG(dockwidget)
    if (i == 1)
        return dockAreaLayout.unplug(path.mid(1));
#endif // QT_CONFIG(dockwidget)

    return nullptr;
}

void QMainWindowLayoutState::saveState(QDataStream &stream) const
{
#if QT_CONFIG(dockwidget)
    dockAreaLayout.saveState(stream);
#if QT_CONFIG(tabbar)
    const QList<QDockWidgetGroupWindow *> floatingTabs =
        mainWindow->findChildren<QDockWidgetGroupWindow *>(Qt::FindDirectChildrenOnly);

    for (QDockWidgetGroupWindow *floating : floatingTabs) {
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
    ds.setVersion(_stream.version());
    if (!checkFormat(ds))
        return false;

    QDataStream stream(copy);
    stream.setVersion(_stream.version());

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
                            if (info == nullptr) {
                                continue;
                            }
                            info->add(w);
                        }
                    }
                }
                break;
#if QT_CONFIG(tabwidget)
            case QDockAreaLayout::FloatingDockWidgetTabMarker:
            {
                auto dockWidgets = allMyDockWidgets(mainWindow);
                QDockWidgetGroupWindow* floatingTab = qt_mainwindow_layout(mainWindow)->createTabbedDockWindow();
                *floatingTab->layoutInfo() = QDockAreaLayoutInfo(
                    &dockAreaLayout.sep, QInternal::LeftDock, // FIXME: DockWidget doesn't save original docking area
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
                            toolBarAreaLayout.docks[oldPath.at(0)].insertToolBar(nullptr, w);
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

static constexpr Qt::ToolBarArea validateToolBarArea(Qt::ToolBarArea area)
{
    switch (area) {
    case Qt::LeftToolBarArea:
    case Qt::RightToolBarArea:
    case Qt::TopToolBarArea:
    case Qt::BottomToolBarArea:
        return area;
    default:
        break;
    }
    return Qt::TopToolBarArea;
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
    area = validateToolBarArea(area);

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
    area = validateToolBarArea(area);
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

Qt::ToolBarArea QMainWindowLayout::toolBarArea(const QToolBar *toolbar) const
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

inline static Qt::DockWidgetArea toDockWidgetArea(int pos)
{
    return QDockWidgetPrivate::toDockWidgetArea(static_cast<QInternal::DockPosition>(pos));
}

// Checks if QDockWidgetGroupWindow or QDockWidget can be plugged the area indicated by path.
// Returns false if called with invalid widget type or if compiled without dockwidget support.
static bool isAreaAllowed(QWidget *widget, const QList<int> &path)
{
    Q_ASSERT_X((path.size() > 1), "isAreaAllowed", "invalid path size");
    const Qt::DockWidgetArea area = toDockWidgetArea(path[1]);

    // Read permissions directly from a single dock widget
    if (QDockWidget *dw = qobject_cast<QDockWidget *>(widget)) {
        const bool allowed = dw->isAreaAllowed(area);
        if (!allowed)
            qCDebug(lcQpaDockWidgets) << "No permission for single DockWidget" << widget << "to dock on" << area;
        return allowed;
    }

    // Read permissions from a DockWidgetGroupWindow depending on its DockWidget children
    if (QDockWidgetGroupWindow *dwgw = qobject_cast<QDockWidgetGroupWindow *>(widget)) {
        const QList<QDockWidget *> children = dwgw->findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly);

        if (children.size() == 1) {
            // Group window has a single child => read its permissions
            const bool allowed = children.at(0)->isAreaAllowed(area);
            if (!allowed)
                qCDebug(lcQpaDockWidgets) << "No permission for DockWidgetGroupWindow" << widget << "to dock on" << area;
            return allowed;
        } else {
            // Group window has more than one or no children => dock it anywhere
            qCDebug(lcQpaDockWidgets) << "DockWidgetGroupWindow" << widget << "has" << children.size() << "children:";
            qCDebug(lcQpaDockWidgets) << children;
            qCDebug(lcQpaDockWidgets) << "DockWidgetGroupWindow" << widget << "can dock at" << area << "and anywhere else.";
            return true;
        }
    }
    qCDebug(lcQpaDockWidgets) << "Docking requested for invalid widget type (coding error)." << widget << area;
    return false;
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

// Returns the rectangle of a dockWidgetArea
// if max is true, the maximum possible rectangle for dropping is returned
// the current visible rectangle otherwise
QRect QMainWindowLayout::dockWidgetAreaRect(const Qt::DockWidgetArea area, DockWidgetAreaSize size) const
{
    const QInternal::DockPosition dockPosition = toDockPos(area);

    // Called with invalid dock widget area
    if (dockPosition == QInternal::DockCount) {
        qCDebug(lcQpaDockWidgets) << "QMainWindowLayout::dockWidgetAreaRect called with" << area;
        return QRect();
    }

    const QDockAreaLayout dl = layoutState.dockAreaLayout;

    // Return maximum or visible rectangle
    return (size == Maximum) ? dl.gapRect(dockPosition) : dl.docks[dockPosition].rect;
}

/*!
    \internal
    Add \a dockwidget to \a area in \a orientation.
 */
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
void QMainWindowLayout::tabifyDockWidget(QDockWidget *first, QDockWidget *second)
{
    applyRestoredState();
    addChildWidget(second);
    layoutState.dockAreaLayout.tabifyDockWidget(first, second);
    emit second->dockLocationChanged(dockWidgetArea(first));
    invalidate();
}

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
    for (QTabBar *bar : std::as_const(usedTabBars))
        bar->setDocumentMode(_documentMode);
}

void QMainWindowLayout::setVerticalTabsEnabled(bool enabled)
{
    if (verticalTabsEnabled == enabled)
        return;

    verticalTabsEnabled = enabled;

    updateTabBarShapes();
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
    const QInternal::DockPosition dockPos = toDockPos(area);
    if (dockPos < QInternal::DockCount)
        return tabPositions[dockPos];
    qWarning("QMainWindowLayout::tabPosition called with out-of-bounds value '%d'", int(area));
    return QTabWidget::North;
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

QTabBar::Shape _q_tb_tabBarShapeFrom(QTabWidget::TabShape shape, QTabWidget::TabPosition position);
#endif // QT_CONFIG(tabwidget)

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
        QTabBar::Shape shape = _q_tb_tabBarShapeFrom(_tabShape, pos);
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
    applyRestoredState();
    addChildWidget(dockwidget);
    layoutState.dockAreaLayout.splitDockWidget(after, dockwidget, orientation);
    emit dockwidget->dockLocationChanged(dockWidgetArea(after));
    invalidate();
}

Qt::DockWidgetArea QMainWindowLayout::dockWidgetArea(const QWidget *widget) const
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
    Q_OBJECT
    QPointer<QMainWindow> mainWindow;
    QPointer<QDockWidget> draggingDock; // Currently dragging (detached) dock widget
public:
    QMainWindowTabBar(QMainWindow *parent);
    ~QMainWindowTabBar();
    QDockWidget *dockAt(int index) const;
    QList<QDockWidget *> dockWidgets() const;
    bool contains(const QDockWidget *dockWidget) const;
protected:
    bool event(QEvent *e) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

};

QDebug operator<<(QDebug debug, const QMainWindowTabBar *bar)
{
    if (!bar)
        return debug << "QMainWindowTabBar(0x0)";
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "QMainWindowTabBar(" << static_cast<const void *>(bar) << ", ";
    debug.nospace().noquote() << "ParentWidget=(" << bar->parentWidget() << "), ";
    const auto dockWidgets = bar->dockWidgets();
    if (dockWidgets.isEmpty())
        debug.nospace().noquote() << "No QDockWidgets";
    else
        debug.nospace().noquote() << "DockWidgets(" << dockWidgets << ")";
    debug.nospace().noquote() << ")";
    return debug;
}

QMainWindowTabBar *QMainWindowLayout::findTabBar(const QDockWidget *dockWidget) const
{
     for (auto *bar : usedTabBars) {
        Q_ASSERT(qobject_cast<QMainWindowTabBar *>(bar));
        auto *tabBar = static_cast<QMainWindowTabBar *>(bar);
        if (tabBar->contains(dockWidget))
            return tabBar;
    }
    return nullptr;
}

QMainWindowTabBar::QMainWindowTabBar(QMainWindow *parent)
    : QTabBar(parent), mainWindow(parent)
{
    setExpanding(false);
}

QList<QDockWidget *> QMainWindowTabBar::dockWidgets() const
{
    QList<QDockWidget *> docks;
    for (int i = 0; i < count(); ++i) {
        if (QDockWidget *dock = dockAt(i))
            docks << dock;
    }
    return docks;
}

bool QMainWindowTabBar::contains(const QDockWidget *dockWidget) const
{
    for (int i = 0; i < count(); ++i) {
        if (dockAt(i) == dockWidget)
            return true;
    }
    return false;
}

// When a dock widget is removed from a floating tab,
// Events need to be processed for the tab bar to realize that the dock widget is gone.
// In this case count() counts the dock widget in transition and accesses dockAt
// with an out-of-bounds index.
// => return nullptr in contrast to other xxxxxAt() functions
QDockWidget *QMainWindowTabBar::dockAt(int index) const
{
    QMainWindowTabBar *that = const_cast<QMainWindowTabBar *>(this);
    QMainWindowLayout* mlayout = qt_mainwindow_layout(mainWindow);
    QDockAreaLayoutInfo *info = mlayout ? mlayout->dockInfo(that) : nullptr;
    if (!info)
        return nullptr;

    const int itemIndex = info->tabIndexToListIndex(index);
    if (itemIndex >= 0) {
        Q_ASSERT(itemIndex < info->item_list.count());
        const QDockAreaLayoutItem &item = info->item_list.at(itemIndex);
        return item.widgetItem ? qobject_cast<QDockWidget *>(item.widgetItem->widget()) : nullptr;
    }

    return nullptr;
}

/*!
   \internal
   Move \a dockWidget to its ideal unplug position.
   \list
   \li If \a dockWidget has a title bar widget, place its center under the mouse cursor.
   \li Otherwise place it in the middle of the title bar's long side, with a
       QApplication::startDragDistance() offset on the short side.
   \endlist
 */
static void moveToUnplugPosition(QPoint mouse, QDockWidget *dockWidget)
{
    Q_ASSERT(dockWidget);

    if (auto *tbWidget = dockWidget->titleBarWidget()) {
        dockWidget->move(mouse - tbWidget->rect().center());
        return;
    }

    const bool vertical = dockWidget->features().testFlag(QDockWidget::DockWidgetVerticalTitleBar);
    const int deltaX = vertical ? QApplication::startDragDistance() : dockWidget->width() / 2;
    const int deltaY = vertical ? dockWidget->height() / 2 : QApplication::startDragDistance();
    dockWidget->move(mouse - QPoint(deltaX, deltaY));
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
        if (d->dragInProgress && !r.contains(e->position().toPoint()) && d->validIndex(d->pressedIndex)) {
            draggingDock = dockAt(d->pressedIndex);
            if (draggingDock) {
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
                dockPriv->startDrag(QDockWidgetPrivate::DragScope::Widget);
                if (dockPriv->state)
                    dockPriv->state->ctrlDrag = e->modifiers() & Qt::ControlModifier;
            }
        }
    }

    if (draggingDock) {
        QDockWidgetPrivate *dockPriv = static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(draggingDock));
        if (dockPriv->state && dockPriv->state->dragging) {
            // move will call QMainWindowLayout::hover
            moveToUnplugPosition(e->globalPosition().toPoint(), draggingDock);
        }
    }
    QTabBar::mouseMoveEvent(e);
}

QMainWindowTabBar::~QMainWindowTabBar()
{
    // Use qobject_cast to verify that we are not already in the (QWidget)
    // destructor of mainWindow
    if (!qobject_cast<QMainWindow *>(mainWindow) || mainWindow == parentWidget())
        return;

    // tab bar is not parented to the main window
    // => can only be a dock widget group window
    auto *mwLayout = qt_mainwindow_layout(mainWindow);
    if (!mwLayout)
        return;
    mwLayout->usedTabBars.remove(this);
}

void QMainWindowTabBar::mouseReleaseEvent(QMouseEvent *e)
{
    if (draggingDock && e->button() == Qt::LeftButton) {
        QDockWidgetPrivate *dockPriv = static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(draggingDock));
        if (dockPriv->state && dockPriv->state->dragging)
            dockPriv->endDrag(QDockWidgetPrivate::EndDragMode::LocationChange);

        draggingDock = nullptr;
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

QList<QDockWidget *> QMainWindowLayout::tabifiedDockWidgets(const QDockWidget *dockWidget) const
{
    const auto *bar = findTabBar(dockWidget);
    if (!bar)
        return {};

    QList<QDockWidget *> buddies = bar->dockWidgets();
    // Return only other dock widgets associated with dockWidget in a tab bar.
    // If dockWidget is alone in a tab bar, return an empty list.
    buddies.removeOne(dockWidget);
    return buddies;
}

bool QMainWindowLayout::isDockWidgetTabbed(const QDockWidget *dockWidget) const
{
    // A single dock widget in a tab bar is not considered to be tabbed.
    // This is to make sure, we don't drag an empty QDockWidgetGroupWindow around.
    // => only consider tab bars with two or more tabs.
    const auto *bar = findTabBar(dockWidget);
    return bar && bar->count() > 1;
}

void QMainWindowLayout::unuseTabBar(QTabBar *bar)
{
    Q_ASSERT(qobject_cast<QMainWindowTabBar *>(bar));
    delete bar;
}

QTabBar *QMainWindowLayout::getTabBar()
{
    if (!usedTabBars.isEmpty() && !isInRestoreState) {
        /*
            If dock widgets have been removed and added while the main window was
            hidden, then the layout hasn't been activated yet, and tab bars from empty
            docking areas haven't been put in the cache yet.
        */
        activate();
    }

    QTabBar *bar = new QMainWindowTabBar(static_cast<QMainWindow *>(parentWidget()));
    bar->setDrawBase(true);
    bar->setElideMode(Qt::ElideRight);
    bar->setDocumentMode(_documentMode);
    bar->setMovable(true);
    connect(bar, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));
    connect(bar, &QTabBar::tabMoved, this, &QMainWindowLayout::tabMoved);

    usedTabBars.insert(bar);
    return bar;
}

QWidget *QMainWindowLayout::getSeparatorWidget()
{
    auto *separator = new QWidget(parentWidget());
    separator->setAttribute(Qt::WA_MouseNoMask, true);
    separator->setAutoFillBackground(false);
    separator->setObjectName("qt_qmainwindow_extended_splitter"_L1);
    usedSeparatorWidgets.insert(separator);
    return separator;
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
    const auto groups =
            parent()->findChildren<QDockWidgetGroupWindow*>(Qt::FindDirectChildrenOnly);
    for (QDockWidgetGroupWindow *dwgw : groups) {
        info = dwgw->layoutInfo()->info(widget);
        if (info)
            return info;
    }
    return nullptr;
}

void QMainWindowLayout::tabChanged()
{
    QTabBar *tb = qobject_cast<QTabBar*>(sender());
    if (tb == nullptr)
        return;
    QDockAreaLayoutInfo *info = dockInfo(tb);
    if (info == nullptr)
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

void QMainWindowLayout::raise(QDockWidget *widget)
{
    QDockAreaLayoutInfo *info = dockInfo(widget);
    if (info == nullptr)
        return;
    if (!info->tabbed)
        return;
    info->setCurrentTab(widget);
}
#endif // QT_CONFIG(tabbar)

#endif // QT_CONFIG(dockwidget)


/******************************************************************************
** QMainWindowLayoutState - layout interface
*/

int QMainWindowLayout::count() const
{
    int result = 0;
    while (itemAt(result))
        ++result;
    return result;
}

QLayoutItem *QMainWindowLayout::itemAt(int index) const
{
    int x = 0;

    if (QLayoutItem *ret = layoutState.itemAt(index, &x))
        return ret;

    if (statusbar && x++ == index)
        return statusbar;

    return nullptr;
}

QLayoutItem *QMainWindowLayout::takeAt(int index)
{
    int x = 0;

    if (QLayoutItem *ret = layoutState.takeAt(index, &x)) {
        // the widget might in fact have been destroyed by now
        if (QWidget *w = ret->widget()) {
            widgetAnimator.abort(w);
            if (w == pluggingWidget)
                pluggingWidget = nullptr;
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
        statusbar = nullptr;
        return ret;
    }

    return nullptr;
}


/*!
    \internal

    restoredState stores what we earlier read from storage, but it couldn't
    be applied as the mainwindow wasn't large enough (yet) to fit the state.
    Usually, the restored state would be applied lazily in setGeometry below.
    However, if the mainwindow's layout is modified (e.g. by a call to tabify or
    splitDockWidgets), then we have to forget the restored state as it might contain
    dangling pointers (QDockWidgetLayoutItem has a copy constructor that copies the
    layout item pointer, and splitting or tabify might have to delete some of those
    layout structures).

    Functions that might result in the QMainWindowLayoutState storing dangling pointers
    have to call this function first, so that the restoredState becomes the actual state
    first, and is forgotten afterwards.
*/
void QMainWindowLayout::applyRestoredState()
{
    if (restoredState) {
        layoutState = *restoredState;
        restoredState.reset();
        discardRestoredStateTimer.stop();
    }
}

void QMainWindowLayout::setGeometry(const QRect &_r)
{
    // Check if the state is valid, and avoid replacing it again if it is currently used
    // in applyState
    if (savedState.isValid() || (restoredState && isInApplyState))
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

    if (restoredState) {
        /*
            The main window was hidden and was going to be maximized or full-screened when
            the state was restored. The state might have been for a larger window size than
            the current size (in _r), and the window might still be in the process of being
            shown and transitioning to the final size (there's no reliable way of knowing
            this across different platforms). Try again with the restored state.
        */
        layoutState = *restoredState;
        if (restoredState->fits()) {
            restoredState.reset();
            discardRestoredStateTimer.stop();
        } else {
            /*
                Try again in the next setGeometry call, but discard the restored state
                after 150ms without any further tries. That's a reasonably short amount of
                time during which we can expect the windowing system to either have completed
                showing the window, or resized the window once more (which then restarts the
                timer in timerEvent).
                If the windowing system is done, then the user won't have had a chance to
                change the layout interactively AND trigger another resize.
            */
            discardRestoredStateTimer.start(150, this);
        }
    }

    layoutState.rect = r;

    layoutState.fitLayout();
    applyState(layoutState, false);
}

void QMainWindowLayout::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == discardRestoredStateTimer.timerId()) {
        discardRestoredStateTimer.stop();
        restoredState.reset();
    }
    QLayout::timerEvent(e);
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
    if (toolBar == nullptr)
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
        const auto groups =
                parent()->findChildren<QDockWidgetGroupWindow*>(Qt::FindDirectChildrenOnly);
        for (QDockWidgetGroupWindow *dwgw : groups) {
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
    const auto groups =
            parent()->findChildren<QDockWidgetGroupWindow*>(Qt::FindDirectChildrenOnly);
    for (QDockWidgetGroupWindow *dwgw : groups) {
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
    if (qobject_cast<QDockWidget*>(widget) != nullptr) {
        QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(widget->layout());
        if (layout->nativeWindowDeco()) {
            globalRect.adjust(0, layout->titleHeight(), 0, 0);
        } else {
            int fw = widget->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, nullptr, widget);
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
                item.widgetItem = nullptr;
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
        pluggingWidget = nullptr;
#if QT_CONFIG(dockwidget)
        setCurrentHoveredFloat(nullptr);
#endif
        //applying the state will make sure that the currentGap is updated correctly
        //and all the geometries (especially the one from the central widget) is correct
        layoutState.apply(false);

#if QT_CONFIG(dockwidget)
#if QT_CONFIG(tabbar)
        if (qobject_cast<QDockWidget*>(widget) != nullptr) {
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
        const auto usedTabBarsCopy = usedTabBars; // list potentially modified by animations
        for (QTabBar *tab_bar : usedTabBarsCopy) {
            if (usedTabBars.contains(tab_bar)) // Showing a tab bar can cause another to be deleted.
               tab_bar->show();
        }
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
    pluggingWidget = nullptr;
    updateGapIndicator();
}

QMainWindowLayout::QMainWindowLayout(QMainWindow *mainwindow, QLayout *parentLayout)
    : QLayout(parentLayout ? static_cast<QWidget *>(nullptr) : mainwindow)
    , layoutState(mainwindow)
    , savedState(mainwindow)
    , dockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks)
    , statusbar(nullptr)
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
    , pluggingWidget(nullptr)
{
    if (parentLayout)
        setParent(parentLayout);

#if QT_CONFIG(dockwidget)
#if QT_CONFIG(tabbar)
    sep = mainwindow->style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent, nullptr, mainwindow);
#endif

#if QT_CONFIG(tabwidget)
    for (int i = 0; i < QInternal::DockCount; ++i)
        tabPositions[i] = QTabWidget::South;
#endif
#endif // QT_CONFIG(dockwidget)
    pluggingWidget = nullptr;

    setObjectName(mainwindow->objectName() + "_layout"_L1);
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

#if QT_CONFIG(dockwidget) && QT_CONFIG(tabbar)
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
    statusbar = sb ? new QWidgetItemV2(sb) : nullptr;
    invalidate();
}
#endif // QT_CONFIG(statusbar)

QWidget *QMainWindowLayout::centralWidget() const
{
    return layoutState.centralWidget();
}

void QMainWindowLayout::setCentralWidget(QWidget *widget)
{
    if (widget != nullptr)
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

#if QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget)
static QTabBar::Shape tabwidgetPositionToTabBarShape(QWidget *w)
{
    QTabBar::Shape result = QTabBar::RoundedSouth;
    if (qobject_cast<QDockWidget *>(w)) {
        switch (static_cast<QDockWidgetPrivate *>(qt_widget_private(w))->tabPosition) {
        case QTabWidget::North:
            result = QTabBar::RoundedNorth;
            break;
        case QTabWidget::South:
            result = QTabBar::RoundedSouth;
            break;
        case QTabWidget::West:
            result = QTabBar::RoundedWest;
            break;
        case QTabWidget::East:
            result = QTabBar::RoundedEast;
            break;
        }
    }
    return result;
}
#endif // QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget)

/*! \internal
    Unplug \a widget (QDockWidget or QToolBar) from it's parent container.

    If \a group is true we might actually unplug the group of tabs this
    widget is part if QMainWindow::GroupedDragging is set. When \a group
    is false, the widget itself is always unplugged alone

    Returns the QLayoutItem of the dragged element.
    The layout item is kept in the layout but set as a gap item.
 */
QLayoutItem *QMainWindowLayout::unplug(QWidget *widget, QDockWidgetPrivate::DragScope scope)
{
#if QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget)
    auto *groupWindow = qobject_cast<const QDockWidgetGroupWindow *>(widget->parentWidget());
    if (!widget->isWindow() && groupWindow) {
        if (scope == QDockWidgetPrivate::DragScope::Group && groupWindow->tabLayoutInfo()) {
            // We are just dragging a floating window as it, not need to do anything, we just have to
            // look up the corresponding QWidgetItem* if it exists
            if (QDockAreaLayoutInfo *info = dockInfo(widget->parentWidget())) {
                QList<int> groupWindowPath = info->indexOf(widget->parentWidget());
                return groupWindowPath.isEmpty() ? nullptr : info->item(groupWindowPath).widgetItem;
            }
            qCDebug(lcQpaDockWidgets) << "Drag only:" << widget << "Group:" << (scope == QDockWidgetPrivate::DragScope::Group);
            return nullptr;
        }
        QList<int> path = groupWindow->layoutInfo()->indexOf(widget);
        QDockAreaLayoutItem parentItem = groupWindow->layoutInfo()->item(path);
        QLayoutItem *item = parentItem.widgetItem;
        if (scope == QDockWidgetPrivate::DragScope::Group && path.size() > 1
            && unplugGroup(this, &item, parentItem)) {
            qCDebug(lcQpaDockWidgets) << "Unplugging:" << widget << "from" << item;
            return item;
        } else {
            // We are unplugging a single dock widget from a floating window.
            QDockWidget *dockWidget = qobject_cast<QDockWidget *>(widget);
            Q_ASSERT(dockWidget); // cannot be a QDockWidgetGroupWindow because it's not floating.
            dockWidget->d_func()->unplug(widget->geometry());

            qCDebug(lcQpaDockWidgets) << "Unplugged from floating dock:" << widget << "from" << parentItem.widgetItem;
            return item;
        }
    }
#endif
    QList<int> path = layoutState.indexOf(widget);
    if (path.isEmpty())
        return nullptr;

    QLayoutItem *item = layoutState.item(path);
    if (widget->isWindow())
        return item;

    QRect r = layoutState.itemRect(path);
    savedState = layoutState;

#if QT_CONFIG(dockwidget)
    if (QDockWidget *dw = qobject_cast<QDockWidget*>(widget)) {
        Q_ASSERT(path.constFirst() == 1);
#if QT_CONFIG(tabwidget)
        if (scope == QDockWidgetPrivate::DragScope::Group && (dockOptions & QMainWindow::GroupedDragging) && path.size() > 3
            && unplugGroup(this, &item,
                           layoutState.dockAreaLayout.item(path.mid(1, path.size() - 2)))) {
            path.removeLast();
            savedState = layoutState;
        } else
#endif // QT_CONFIG(tabwidget)
        {
            // Dock widget is unplugged from a main window dock
            // => height or width need to be decreased by separator size
            switch (dockWidgetArea(dw)) {
            case Qt::LeftDockWidgetArea:
            case Qt::RightDockWidgetArea:
                r.setHeight(r.height() - sep);
                break;
            case Qt::TopDockWidgetArea:
            case Qt::BottomDockWidgetArea:
                r.setWidth(r.width() - sep);
                break;
            case Qt::NoDockWidgetArea:
            case Qt::DockWidgetArea_Mask:
                break;
            }

            // Depending on the title bar layout (vertical / horizontal),
            // width and height have to provide minimum space for window handles
            // and mouse dragging.
            // Assuming horizontal title bar, if the dock widget does not have a layout.
            const auto *layout = qobject_cast<QDockWidgetLayout *>(dw->layout());
            const bool verticalTitleBar = layout ? layout->verticalTitleBar : false;
            const int tbHeight = QApplication::style()
                      ? QApplication::style()->pixelMetric(QStyle::PixelMetric::PM_TitleBarHeight, nullptr, dw)
                      : 20;
            const int minHeight = verticalTitleBar ? 2 * tbHeight : tbHeight;
            const int minWidth = verticalTitleBar ? tbHeight : 2 * tbHeight;
            r.setSize(r.size().expandedTo(QSize(minWidth, minHeight)));
            qCDebug(lcQpaDockWidgets) << dw << "will be unplugged with size" << r.size();

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
    Q_UNUSED(scope);
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
            gapIndicator->setObjectName("qt_rubberband"_L1);
        } else if (gapIndicator->parent() != expectedParent) {
            gapIndicator->setParent(expectedParent);
        }

        // Prevent re-entry in case of size change
        const bool sigBlockState = gapIndicator->signalsBlocked();
        auto resetSignals = qScopeGuard([this, sigBlockState](){ gapIndicator->blockSignals(sigBlockState); });
        gapIndicator->blockSignals(true);

#if QT_CONFIG(dockwidget)
        if (currentHoveredFloat)
            gapIndicator->setGeometry(currentHoveredFloat->currentGapRect);
        else
#endif
            gapIndicator->setGeometry(currentGapRect);

        gapIndicator->show();
        gapIndicator->raise();

        // Reset signal state

    } else if (gapIndicator) {
        gapIndicator->hide();
    }

#endif // QT_CONFIG(rubberband)
}

void QMainWindowLayout::hover(QLayoutItem *hoverTarget,
                              const QPoint &mousePos) {
  if (!parentWidget()->isVisible() || parentWidget()->isMinimized() ||
      pluggingWidget != nullptr || hoverTarget == nullptr)
    return;

  QWidget *widget = hoverTarget->widget();

#if QT_CONFIG(dockwidget)
    widget->raise();
    if ((dockOptions & QMainWindow::GroupedDragging) && (qobject_cast<QDockWidget*>(widget)
            || qobject_cast<QDockWidgetGroupWindow *>(widget))) {

        // Check if we are over another floating dock widget
        QVarLengthArray<QWidget *, 10> candidates;
        const auto siblings = parentWidget()->children();
        for (QObject *c : siblings) {
            QWidget *w = qobject_cast<QWidget*>(c);
            if (!w)
                continue;

            // Handle only dock widgets and group windows
            if (!qobject_cast<QDockWidget*>(w) && !qobject_cast<QDockWidgetGroupWindow *>(w))
                continue;

            // Check permission to dock on another dock widget or floating dock
            // FIXME in Qt 7

            if (w != widget && w->isWindow() && w->isVisible() && !w->isMinimized())
                candidates << w;

            if (QDockWidgetGroupWindow *group = qobject_cast<QDockWidgetGroupWindow *>(w)) {
                // floating QDockWidgets have a QDockWidgetGroupWindow as a parent,
                // if they have been hovered over
                const auto groupChildren = group->children();
                for (QObject *c : groupChildren) {
                    if (QDockWidget *dw = qobject_cast<QDockWidget*>(c)) {
                        if (dw != widget && dw->isFloating() && dw->isVisible() && !dw->isMinimized())
                            candidates << dw;
                    }
                }
            }
        }

        for (QWidget *w : candidates) {
            const QScreen *screen1 = qt_widget_private(widget)->associatedScreen();
            const QScreen *screen2 = qt_widget_private(w)->associatedScreen();
            if (screen1 && screen2 && screen1 != screen2)
                continue;
            if (!w->geometry().contains(mousePos))
                continue;

#if QT_CONFIG(tabwidget)
            if (auto dropTo = qobject_cast<QDockWidget *>(w)) {

                // w is the drop target's widget
                w = dropTo->widget();

                // Create a floating tab, unless already existing
                if (!qobject_cast<QDockWidgetGroupWindow *>(w)) {
                    QDockWidgetGroupWindow *floatingTabs = createTabbedDockWindow();
                    floatingTabs->setGeometry(dropTo->geometry());
                    QDockAreaLayoutInfo *info = floatingTabs->layoutInfo();
                    const QTabBar::Shape shape = tabwidgetPositionToTabBarShape(dropTo);

                    // dropTo and widget may be in a state where they transition
                    // from being a group window child to a single floating dock widget.
                    // In that case, their path to a main window dock may not have been
                    // updated yet.
                    // => ask both and fall back to dock 1 (right dock)
                    QInternal::DockPosition dockPosition = toDockPos(dockWidgetArea(dropTo));
                    if (dockPosition == QInternal::DockPosition::DockCount)
                        dockPosition = toDockPos(dockWidgetArea(widget));
                    if (dockPosition == QInternal::DockPosition::DockCount)
                        dockPosition = QInternal::DockPosition::RightDock;

                    *info = QDockAreaLayoutInfo(&layoutState.dockAreaLayout.sep, dockPosition,
                                                Qt::Horizontal, shape,
                                                static_cast<QMainWindow *>(parentWidget()));
                    info->tabBar = getTabBar();
                    info->tabbed = true;
                    info->add(dropTo);
                    QDockAreaLayoutInfo &parentInfo = layoutState.dockAreaLayout.docks[dockPosition];
                    parentInfo.add(floatingTabs);
                    dropTo->setParent(floatingTabs);
                    qCDebug(lcQpaDockWidgets) << "Wrapping" << widget << "into floating tabs" << floatingTabs;
                    w = floatingTabs;
                }

                // Show the drop target and raise widget to foreground
                dropTo->show();
                qCDebug(lcQpaDockWidgets) << "Showing" << dropTo;
                widget->raise();
                qCDebug(lcQpaDockWidgets) << "Raising" << widget;
            }
#endif
            auto *groupWindow = qobject_cast<QDockWidgetGroupWindow *>(w);
            Q_ASSERT(groupWindow);
            if (groupWindow->hover(hoverTarget, groupWindow->mapFromGlobal(mousePos))) {
                setCurrentHoveredFloat(groupWindow);
                applyState(layoutState); // update the tabbars
            }
            return;
        }
    }

    // If a temporary group window has been created during a hover,
    // remove it, if it has only one dockwidget child
    if (currentHoveredFloat)
        currentHoveredFloat->destroyIfSingleItemLeft();

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
        allowed = isAreaAllowed(widget, path);
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
        fixToolBarOrientation(hoverTarget, 2); // 2 = top dock, ie. horizontal
        restore(true);
        return;
    }

    fixToolBarOrientation(hoverTarget, currentGapPos.at(1));

    QMainWindowLayoutState newState = savedState;

    if (!newState.insertGap(path, hoverTarget)) {
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
    // applying the state can lead to showing separator widgets, which would lead to a re-layout
    // (even though the separator widgets are not really part of the layout)
    // break the loop
    if (isInApplyState)
        return;
    isInApplyState = true;
#if QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget)
    QSet<QTabBar*> used = newState.dockAreaLayout.usedTabBars();
    const auto groups =
            parent()->findChildren<QDockWidgetGroupWindow*>(Qt::FindDirectChildrenOnly);
    for (QDockWidgetGroupWindow *dwgw : groups)
        used += dwgw->layoutInfo()->usedTabBars();

    const QSet<QTabBar*> retired = usedTabBars - used;
    usedTabBars = used;
    for (QTabBar *tab_bar : retired) {
        unuseTabBar(tab_bar);
    }

    if (sep == 1) {
        const QSet<QWidget*> usedSeps = newState.dockAreaLayout.usedSeparatorWidgets();
        const QSet<QWidget*> retiredSeps = usedSeparatorWidgets - usedSeps;
        usedSeparatorWidgets = usedSeps;
        for (QWidget *sepWidget : retiredSeps)
            delete sepWidget;
    }

    for (int i = 0; i < QInternal::DockCount; ++i)
        newState.dockAreaLayout.docks[i].reparentWidgets(parentWidget());

#endif // QT_CONFIG(dockwidget) && QT_CONFIG(tabwidget)
    newState.apply(dockOptions & QMainWindow::AnimatedDocks && animate);
    isInApplyState = false;
}

void QMainWindowLayout::saveState(QDataStream &stream) const
{
    layoutState.saveState(stream);
}

bool QMainWindowLayout::restoreState(QDataStream &stream)
{
    QScopedValueRollback<bool> guard(isInRestoreState, true);
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
    } else {
        /*
            The state might not fit into the size of the widget as it gets shown, but
            if the window is expected to be maximized or full screened, then we might
            get several resizes as part of that transition, at the end of which the
            state might fit. So keep the restored state around for now and try again
            later in setGeometry.
        */
        if ((parentWidget()->windowState() & (Qt::WindowFullScreen | Qt::WindowMaximized))
            && !layoutState.fits()) {
            restoredState.reset(new QMainWindowLayoutState(layoutState));
        }
    }

    savedState.deleteAllLayoutItems();
    savedState.clear();

#if QT_CONFIG(dockwidget)
    if (parentWidget()->isVisible()) {
#if QT_CONFIG(tabbar)
        for (QTabBar *tab_bar : std::as_const(usedTabBars))
            tab_bar->show();

#endif
    }
#endif // QT_CONFIG(dockwidget)

    return true;
}

#if QT_CONFIG(draganddrop)
bool QMainWindowLayout::needsPlatformDrag()
{
    static const bool wayland =
            QGuiApplication::platformName().startsWith("wayland"_L1, Qt::CaseInsensitive);
    return wayland;
}

Qt::DropAction QMainWindowLayout::performPlatformWidgetDrag(QLayoutItem *widgetItem,
                                                            const QPoint &pressPosition)
{
    draggingWidget = widgetItem;
    QWidget *widget = widgetItem->widget();
    auto drag = QDrag(widget);
    auto mimeData = new QMimeData();
    auto window = widgetItem->widget()->windowHandle();

    auto serialize = [](const auto &object) {
        QByteArray data;
        QDataStream dataStream(&data, QIODevice::WriteOnly);
        dataStream << object;
        return data;
    };
    mimeData->setData("application/x-qt-mainwindowdrag-window"_L1,
                      serialize(reinterpret_cast<qintptr>(window)));
    mimeData->setData("application/x-qt-mainwindowdrag-position"_L1, serialize(pressPosition));
    drag.setMimeData(mimeData);

    auto result = drag.exec();

    draggingWidget = nullptr;
    return result;
}
#endif

QT_END_NAMESPACE

#include "qmainwindowlayout.moc"
#include "moc_qmainwindowlayout_p.cpp"
