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
#include "qdockarealayout_p.h"

#ifndef QT_NO_MAINWINDOW
#include "qdockwidget.h"
#include "qdockwidget_p.h"
#include "qtoolbar_p.h"
#include "qmainwindow.h"
#include "qtoolbar.h"
#include "qtoolbarlayout_p.h"
#include "qwidgetanimator_p.h"
#include "qrubberband.h"
#include "qtabbar_p.h"

#include <qapplication.h>
#include <qstatusbar.h>
#include <qstring.h>
#include <qstyle.h>
#include <qstylepainter.h>
#include <qvarlengtharray.h>
#include <qstack.h>
#include <qmap.h>
#include <qtimer.h>

#ifndef QT_NO_DEBUG_STREAM
#  include <qdebug.h>
#  include <qtextstream.h>
#endif

#include <private/qapplication_p.h>
#include <private/qlayoutengine_p.h>
#include <private/qwidgetresizehandler_p.h>
#if 0 // Used to be included in Qt4 for Q_WS_MAC
#   include <private/qcore_mac_p.h>
#   include <private/qt_cocoa_helpers_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

extern QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *window);

/******************************************************************************
** debug
*/

#if !defined(QT_NO_DOCKWIDGET) && !defined(QT_NO_DEBUG_STREAM)

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
            << " tabbed:" << layout.tabbed
            << " tbshape:" << layout.tabBarShape << '\n';

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

#endif // !defined(QT_NO_DOCKWIDGET) && !defined(QT_NO_DEBUG)

/******************************************************************************
 ** QDockWidgetGroupWindow
 */
// QDockWidgetGroupWindow is the floating window containing the tabbed dockwidgets in case several
// dockwidgets are dragged together (QMainWindow::GroupedDragging feature).
// QDockWidgetGroupLayout is the layout of that window and use a QDockAreaLayoutInfo to layout
// the tabs inside it.
#ifndef QT_NO_DOCKWIDGET
class QDockWidgetGroupLayout : public QLayout {
    QDockAreaLayoutInfo info;
    QWidgetResizeHandler *resizer;
public:
    QDockWidgetGroupLayout(QWidget* parent) : QLayout(parent) {
        setSizeConstraint(QLayout::SetMinAndMaxSize);
        resizer = new QWidgetResizeHandler(parent);
        resizer->setMovingEnabled(false);
    }
    ~QDockWidgetGroupLayout() {
        info.deleteAllLayoutItems();
    }

    void addItem(QLayoutItem*) Q_DECL_OVERRIDE { Q_UNREACHABLE(); }
    int count() const Q_DECL_OVERRIDE { return 0; }
    QLayoutItem* itemAt(int index) const Q_DECL_OVERRIDE
    {
        int x = 0;
        return info.itemAt(&x, index);
    }
    QLayoutItem* takeAt(int index) Q_DECL_OVERRIDE
    {
        int x = 0;
        return info.takeAt(&x, index);
    }
    QSize sizeHint() const Q_DECL_OVERRIDE
    {
        int fw = frameWidth();
        return info.sizeHint() + QSize(fw, fw);
    }
    QSize minimumSize() const Q_DECL_OVERRIDE
    {
        int fw = frameWidth();
        return info.minimumSize() + QSize(fw, fw);
    }
    QSize maximumSize() const Q_DECL_OVERRIDE
    {
        int fw = frameWidth();
        return info.maximumSize() + QSize(fw, fw);
    }
    void setGeometry(const QRect&r) Q_DECL_OVERRIDE
    {
        static_cast<QDockWidgetGroupWindow *>(parent())->destroyOrHideIfEmpty();
        QDockAreaLayoutInfo *li = layoutInfo();
        if (li->isEmpty())
            return;
        int fw = frameWidth();
        li->reparentWidgets(parentWidget());
        li->rect = r.adjusted(fw, fw, -fw, -fw);
        li->fitItems();
        li->apply(false);
        resizer->setActive(QWidgetResizeHandler::Resize, !nativeWindowDeco());
    }

    QDockAreaLayoutInfo *layoutInfo() {
        return &info;
    }

    bool nativeWindowDeco() const
    {
        return QDockWidgetLayout::wmSupportsNativeWindowDeco();
    }

    int frameWidth() const
    {
        return nativeWindowDeco() ? 0 :
            parentWidget()->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, parentWidget());
    }
};

bool QDockWidgetGroupWindow::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Close:
        // Forward the close to the QDockWidget just as if its close button was pressed
        if (QDockWidget *dw = topDockWidget()) {
            e->ignore();
            dw->close();
            adjustFlags();
        }
        return true;
    case QEvent::Move:
        // Let QDockWidgetPrivate::moseEvent handle the dragging
        if (QDockWidget *dw = topDockWidget())
            static_cast<QDockWidgetPrivate *>(QObjectPrivate::get(dw))->moveEvent(static_cast<QMoveEvent*>(e));
        return true;
    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
        // Let the QDockWidgetPrivate of the currently visible dock widget handle the drag and drop
        if (QDockWidget *dw = topDockWidget())
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
    return static_cast<QDockWidgetGroupLayout*>(layout())->layoutInfo();
}

/*! \internal
    Returns the currently active QDockWidget.
 */
QDockWidget *QDockWidgetGroupWindow::topDockWidget() const
{
    QDockAreaLayoutInfo *info = layoutInfo();
    QDockWidget *dw = 0;
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
    foreach (QTabBar *tb, findChildren<QTabBar *>(QString(), Qt::FindDirectChildrenOnly))
        tb->setParent(parentWidget());
    deleteLater();
}

/*! \internal
    Sets the flags of this window in accordence to the capabilities of the dock widgets
 */
void QDockWidgetGroupWindow::adjustFlags()
{
    QDockWidget *top = topDockWidget();
    if (!top)
        return;
    const bool nativeDeco = static_cast<QDockWidgetGroupLayout *>(layout())->nativeWindowDeco();

    Qt::WindowFlags oldFlags = windowFlags();
    Qt::WindowFlags flags = oldFlags;
    if (nativeDeco) {
        flags |= Qt::CustomizeWindowHint | Qt::WindowTitleHint;
        flags.setFlag(Qt::WindowCloseButtonHint, top->features() & QDockWidget::DockWidgetClosable);
        flags &= ~Qt::FramelessWindowHint;
    } else {
        flags |= Qt::FramelessWindowHint;
    }
    if (oldFlags != flags) {
        setWindowFlags(flags);
        show(); // setWindowFlags hides the window
    }

    setWindowTitle(top->windowTitle());
    setWindowIcon(top->windowIcon());
}
#endif

/******************************************************************************
** QMainWindowLayoutState
*/

// we deal with all the #ifndefferry here so QMainWindowLayout code is clean

QMainWindowLayoutState::QMainWindowLayoutState(QMainWindow *win)
    :
#ifndef QT_NO_TOOLBAR
    toolBarAreaLayout(win),
#endif
#ifndef QT_NO_DOCKWIDGET
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

#ifndef QT_NO_DOCKWIDGET
    result = dockAreaLayout.sizeHint();
#else
    if (centralWidgetItem != 0)
        result = centralWidgetItem->sizeHint();
#endif

#ifndef QT_NO_TOOLBAR
    result = toolBarAreaLayout.sizeHint(result);
#endif // QT_NO_TOOLBAR

    return result;
}

QSize QMainWindowLayoutState::minimumSize() const
{
    QSize result(0, 0);

#ifndef QT_NO_DOCKWIDGET
    result = dockAreaLayout.minimumSize();
#else
    if (centralWidgetItem != 0)
        result = centralWidgetItem->minimumSize();
#endif

#ifndef QT_NO_TOOLBAR
    result = toolBarAreaLayout.minimumSize(result);
#endif // QT_NO_TOOLBAR

    return result;
}

void QMainWindowLayoutState::apply(bool animated)
{
#ifndef QT_NO_TOOLBAR
    toolBarAreaLayout.apply(animated);
#endif

#ifndef QT_NO_DOCKWIDGET
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
#ifdef QT_NO_TOOLBAR
    r = rect;
#else
    toolBarAreaLayout.rect = rect;
    r = toolBarAreaLayout.fitLayout();
#endif // QT_NO_TOOLBAR

#ifndef QT_NO_DOCKWIDGET
    dockAreaLayout.rect = r;
    dockAreaLayout.fitLayout();
#else
    centralWidgetRect = r;
#endif
}

void QMainWindowLayoutState::deleteAllLayoutItems()
{
#ifndef QT_NO_TOOLBAR
    toolBarAreaLayout.deleteAllLayoutItems();
#endif

#ifndef QT_NO_DOCKWIDGET
    dockAreaLayout.deleteAllLayoutItems();
#endif
}

void QMainWindowLayoutState::deleteCentralWidgetItem()
{
#ifndef QT_NO_DOCKWIDGET
    delete dockAreaLayout.centralWidgetItem;
    dockAreaLayout.centralWidgetItem = 0;
#else
    delete centralWidgetItem;
    centralWidgetItem = 0;
#endif
}

QLayoutItem *QMainWindowLayoutState::itemAt(int index, int *x) const
{
#ifndef QT_NO_TOOLBAR
    if (QLayoutItem *ret = toolBarAreaLayout.itemAt(x, index))
        return ret;
#endif

#ifndef QT_NO_DOCKWIDGET
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
#ifndef QT_NO_TOOLBAR
    if (QLayoutItem *ret = toolBarAreaLayout.takeAt(x, index))
        return ret;
#endif

#ifndef QT_NO_DOCKWIDGET
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

#ifndef QT_NO_TOOLBAR
    // is it a toolbar?
    if (QToolBar *toolBar = qobject_cast<QToolBar*>(widget)) {
        result = toolBarAreaLayout.indexOf(toolBar);
        if (!result.isEmpty())
            result.prepend(0);
        return result;
    }
#endif

#ifndef QT_NO_DOCKWIDGET
    // is it a dock widget?
    if (qobject_cast<QDockWidget *>(widget) || qobject_cast<QDockWidgetGroupWindow *>(widget)) {
        result = dockAreaLayout.indexOf(widget);
        if (!result.isEmpty())
            result.prepend(1);
        return result;
    }
#endif //QT_NO_DOCKWIDGET

    return result;
}

bool QMainWindowLayoutState::contains(QWidget *widget) const
{
#ifndef QT_NO_DOCKWIDGET
    if (dockAreaLayout.centralWidgetItem != 0 && dockAreaLayout.centralWidgetItem->widget() == widget)
        return true;
    if (!dockAreaLayout.indexOf(widget).isEmpty())
        return true;
#else
    if (centralWidgetItem != 0 && centralWidgetItem->widget() == widget)
        return true;
#endif

#ifndef QT_NO_TOOLBAR
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

#ifndef QT_NO_DOCKWIDGET
    dockAreaLayout.centralWidgetItem = item;
#else
    centralWidgetItem = item;
#endif
}

QWidget *QMainWindowLayoutState::centralWidget() const
{
    QLayoutItem *item = 0;

#ifndef QT_NO_DOCKWIDGET
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

#ifndef QT_NO_TOOLBAR
    // is it a toolbar?
    if (qobject_cast<QToolBar*>(widget) != 0) {
        result = toolBarAreaLayout.gapIndex(pos);
        if (!result.isEmpty())
            result.prepend(0);
        return result;
    }
#endif

#ifndef QT_NO_DOCKWIDGET
    // is it a dock widget?
    if (qobject_cast<QDockWidget *>(widget) != 0
            || qobject_cast<QDockWidgetGroupWindow *>(widget)) {
        result = dockAreaLayout.gapIndex(pos);
        if (!result.isEmpty())
            result.prepend(1);
        return result;
    }
#endif //QT_NO_DOCKWIDGET

    return result;
}

bool QMainWindowLayoutState::insertGap(const QList<int> &path, QLayoutItem *item)
{
    if (path.isEmpty())
        return false;

    int i = path.first();

#ifndef QT_NO_TOOLBAR
    if (i == 0) {
        Q_ASSERT(qobject_cast<QToolBar*>(item->widget()) != 0);
        return toolBarAreaLayout.insertGap(path.mid(1), item);
    }
#endif

#ifndef QT_NO_DOCKWIDGET
    if (i == 1) {
        Q_ASSERT(qobject_cast<QDockWidget*>(item->widget()) || qobject_cast<QDockWidgetGroupWindow*>(item->widget()));
        return dockAreaLayout.insertGap(path.mid(1), item);
    }
#endif //QT_NO_DOCKWIDGET

    return false;
}

void QMainWindowLayoutState::remove(const QList<int> &path)
{
    int i = path.first();

#ifndef QT_NO_TOOLBAR
    if (i == 0)
        toolBarAreaLayout.remove(path.mid(1));
#endif

#ifndef QT_NO_DOCKWIDGET
    if (i == 1)
        dockAreaLayout.remove(path.mid(1));
#endif //QT_NO_DOCKWIDGET
}

void QMainWindowLayoutState::remove(QLayoutItem *item)
{
#ifndef QT_NO_TOOLBAR
    toolBarAreaLayout.remove(item);
#endif

#ifndef QT_NO_DOCKWIDGET
    // is it a dock widget?
    if (QDockWidget *dockWidget = qobject_cast<QDockWidget *>(item->widget())) {
        QList<int> path = dockAreaLayout.indexOf(dockWidget);
        if (!path.isEmpty())
            dockAreaLayout.remove(path);
    }
#endif //QT_NO_DOCKWIDGET
}

void QMainWindowLayoutState::clear()
{
#ifndef QT_NO_TOOLBAR
    toolBarAreaLayout.clear();
#endif

#ifndef QT_NO_DOCKWIDGET
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

#ifndef QT_NO_TOOLBAR
    if (i == 0) {
        const QToolBarAreaLayoutItem *tbItem = toolBarAreaLayout.item(path.mid(1));
        Q_ASSERT(tbItem);
        return tbItem->widgetItem;
    }
#endif

#ifndef QT_NO_DOCKWIDGET
    if (i == 1)
        return dockAreaLayout.item(path.mid(1)).widgetItem;
#endif //QT_NO_DOCKWIDGET

    return 0;
}

QRect QMainWindowLayoutState::itemRect(const QList<int> &path) const
{
    int i = path.first();

#ifndef QT_NO_TOOLBAR
    if (i == 0)
        return toolBarAreaLayout.itemRect(path.mid(1));
#endif

#ifndef QT_NO_DOCKWIDGET
    if (i == 1)
        return dockAreaLayout.itemRect(path.mid(1));
#endif //QT_NO_DOCKWIDGET

    return QRect();
}

QRect QMainWindowLayoutState::gapRect(const QList<int> &path) const
{
    int i = path.first();

#ifndef QT_NO_TOOLBAR
    if (i == 0)
        return toolBarAreaLayout.itemRect(path.mid(1));
#endif

#ifndef QT_NO_DOCKWIDGET
    if (i == 1)
        return dockAreaLayout.gapRect(path.mid(1));
#endif //QT_NO_DOCKWIDGET

    return QRect();
}

QLayoutItem *QMainWindowLayoutState::plug(const QList<int> &path)
{
    int i = path.first();

#ifndef QT_NO_TOOLBAR
    if (i == 0)
        return toolBarAreaLayout.plug(path.mid(1));
#endif

#ifndef QT_NO_DOCKWIDGET
    if (i == 1)
        return dockAreaLayout.plug(path.mid(1));
#endif //QT_NO_DOCKWIDGET

    return 0;
}

QLayoutItem *QMainWindowLayoutState::unplug(const QList<int> &path, QMainWindowLayoutState *other)
{
    int i = path.first();

#ifdef QT_NO_TOOLBAR
    Q_UNUSED(other);
#else
    if (i == 0)
        return toolBarAreaLayout.unplug(path.mid(1), other ? &other->toolBarAreaLayout : 0);
#endif

#ifndef QT_NO_DOCKWIDGET
    if (i == 1)
        return dockAreaLayout.unplug(path.mid(1));
#endif //QT_NO_DOCKWIDGET

    return 0;
}

void QMainWindowLayoutState::saveState(QDataStream &stream) const
{
#ifndef QT_NO_DOCKWIDGET
    dockAreaLayout.saveState(stream);
#ifndef QT_NO_TABBAR
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
#ifndef QT_NO_TOOLBAR
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

#ifndef QT_NO_DOCKWIDGET
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
#endif // QT_NO_DOCKWIDGET

//pre4.3 tests the format that was used before 4.3
bool QMainWindowLayoutState::checkFormat(QDataStream &stream)
{
    while (!stream.atEnd()) {
        uchar marker;
        stream >> marker;
        switch(marker)
        {
#ifndef QT_NO_TOOLBAR
            case QToolBarAreaLayout::ToolBarStateMarker:
            case QToolBarAreaLayout::ToolBarStateMarkerEx:
                {
                    QList<QToolBar *> toolBars = findChildrenHelper<QToolBar*>(mainWindow);
                    if (!toolBarAreaLayout.restoreState(stream, toolBars, marker, true /*testing*/)) {
                            return false;
                    }
                }
                break;
#endif // QT_NO_TOOLBAR

#ifndef QT_NO_DOCKWIDGET
            case QDockAreaLayout::DockWidgetStateMarker:
                {
                    const auto dockWidgets = allMyDockWidgets(mainWindow);
                    if (!dockAreaLayout.restoreState(stream, dockWidgets, true /*testing*/)) {
                        return false;
                    }
                }
                break;
#ifndef QT_NO_TABBAR
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
#endif // QT_NO_TABBAR
#endif // QT_NO_DOCKWIDGET
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
#ifndef QT_NO_DOCKWIDGET
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
#ifndef QT_NO_TABBAR
            case QDockAreaLayout::FloatingDockWidgetTabMarker:
            {
                auto dockWidgets = allMyDockWidgets(mainWindow);
                QDockWidgetGroupWindow* floatingTab = qt_mainwindow_layout(mainWindow)->createTabbedDockWindow();
                *floatingTab->layoutInfo() = QDockAreaLayoutInfo(&dockAreaLayout.sep, QInternal::LeftDock,
                                                                 Qt::Horizontal, QTabBar::RoundedSouth, mainWindow);
                QRect geometry;
                stream >> geometry;
                if (!floatingTab->layoutInfo()->restoreState(stream, dockWidgets, false))
                    return false;
                geometry = QDockAreaLayout::constrainedRect(geometry, floatingTab);
                floatingTab->move(geometry.topLeft());
                floatingTab->resize(geometry.size());
                floatingTab->show();
            }
            break;
#endif // QT_NO_TABBAR
#endif // QT_NO_DOCKWIDGET

#ifndef QT_NO_TOOLBAR
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
#endif //QT_NO_TOOLBAR
            default:
                return false;
        }// switch
    } //while


    return true;
}

/******************************************************************************
** QMainWindowLayoutState - toolbars
*/

#ifndef QT_NO_TOOLBAR

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

#if 0 // Used to be included in Qt4 for Q_WS_MAC
        if (usesHIToolBar(toolbar)) {
            removeFromMacToolbar(toolbar);
        } else
#endif
        {
            removeWidget(toolbar);
        }
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
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    if ((area == Qt::TopToolBarArea)
            && layoutState.mainWindow->unifiedTitleAndToolBarOnMac()) {
        insertIntoMacToolbar(0, toolbar);
    } else
#endif
    {
        //let's add the toolbar to the layout
        addChildWidget(toolbar);
        QLayoutItem * item = layoutState.toolBarAreaLayout.addToolBar(toDockPos(area), toolbar);
        if (savedState.isValid() && item) {
            // copy the toolbar also in the saved state
            savedState.toolBarAreaLayout.insertItem(toDockPos(area), item);
        }
        invalidate();

        //this ensures that the toolbar has the right window flags (not floating any more)
        toolbar->d_func()->updateWindowFlags(false /*floating*/);
    }
}

/*!
    Adds \a toolbar before \a before
*/
void QMainWindowLayout::insertToolBar(QToolBar *before, QToolBar *toolbar)
{
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    if (usesHIToolBar(before)) {
        insertIntoMacToolbar(before, toolbar);
    } else
#endif
    {
        addChildWidget(toolbar);
        QLayoutItem * item = layoutState.toolBarAreaLayout.insertToolBar(before, toolbar);
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
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    if (pos == QInternal::DockCount) {
        if (qtoolbarsInUnifiedToolbarList.contains(toolbar))
            return Qt::TopToolBarArea;
    }
#endif
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
    bool updateNonUnifiedParts = true;
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    if (layoutState.mainWindow->unifiedTitleAndToolBarOnMac()) {
        // If we hit this case, someone has pressed the "toolbar button" which will
        // toggle the unified toolbar visibility, because that's what the user wants.
        // We might be in a situation where someone has hidden all the toolbars
        // beforehand (maybe in construction), but now they've hit this button and
        // and are expecting the items to show. What do we do?
        // 1) Check the visibility of all the toolbars, if one is visible, do nothing, this
        //    preserves what people would expect (these toolbars were visible when I clicked last time).
        // 2) If NONE are visible, then show them all. Again, this preserves the user expectation
        //    of, "I want to see the toolbars." The user may get more toolbars than expected, but this
        //    is better seeing nothing.
        // Don't worry about any of this if we are going invisible. This does mean we may get
        // into issues when switching into and out of fullscreen mode, but this is probably minor.
        // If we ever need to do hiding, that would have to be taken care of after the unified toolbar
        // has finished hiding.
        // People can of course handle the QEvent::ToolBarChange event themselves and do
        // WHATEVER they want if they don't like what we are doing (though the unified toolbar
        // will fire regardless).

        // Check if we REALLY need to update the geometry below. If we only have items in the
        // unified toolbar, all the docks will be empty, so there's very little point
        // in doing the geometry as Apple will do it (we also avoid flicker in Cocoa as well).
        // FWIW, layoutState.toolBarAreaLayout.visible and the state of the unified toolbar
        // visibility can get out of sync. I really don't think it's a big issue. It is kept
        // to a minimum because we only change the visibility if we absolutely must.
        // update the "non unified parts."
        updateNonUnifiedParts = !layoutState.toolBarAreaLayout.isEmpty();

        // We get this function before the unified toolbar does its thing.
        // So, the value will be opposite of what we expect.
        bool goingVisible = !macWindowToolbarIsVisible(qt_mac_window_for(layoutState.mainWindow));
        if (goingVisible) {
            const int ToolBarCount = qtoolbarsInUnifiedToolbarList.size();
            bool needAllVisible = true;
            for (int i = 0; i < ToolBarCount; ++i) {
                if (!qtoolbarsInUnifiedToolbarList.at(i)->isHidden()) {
                    needAllVisible = false;
                    break;
                }
            }
            if (needAllVisible) {
                QBoolBlocker blocker(blockVisiblityCheck);  // Disable the visibilty check because
                                                            // the toggle has already happened.
                for (int i = 0; i < ToolBarCount; ++i)
                    qtoolbarsInUnifiedToolbarList.at(i)->setVisible(true);
            }
        }
        if (!updateNonUnifiedParts)
            layoutState.toolBarAreaLayout.visible = goingVisible;
    }
#endif
    if (updateNonUnifiedParts) {
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
}

#endif // QT_NO_TOOLBAR

/******************************************************************************
** QMainWindowLayoutState - dock areas
*/

#ifndef QT_NO_DOCKWIDGET

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

#ifndef QT_NO_TABBAR
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
#endif // QT_NO_TABBAR

void QMainWindowLayout::setVerticalTabsEnabled(bool enabled)
{
#ifdef QT_NO_TABBAR
    Q_UNUSED(enabled);
#else
    if (verticalTabsEnabled == enabled)
        return;

    verticalTabsEnabled = enabled;

    updateTabBarShapes();
#endif // QT_NO_TABBAR
}

#ifndef QT_NO_TABWIDGET
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
#endif // QT_NO_TABWIDGET

#ifndef QT_NO_TABBAR
void QMainWindowLayout::updateTabBarShapes()
{
#ifndef QT_NO_TABWIDGET
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
#ifndef QT_NO_TABWIDGET
        QTabWidget::TabPosition pos = verticalTabsEnabled ? vertical[i] : tabPositions[i];
        QTabBar::Shape shape = tabBarShapeFrom(_tabShape, pos);
#else
        QTabBar::Shape shape = verticalTabsEnabled ? vertical[i] : QTabBar::RoundedSouth;
#endif
        layout.docks[i].setTabBarShape(shape);
    }
}
#endif // QT_NO_TABBAR

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

#ifndef QT_NO_TABBAR

// Handle custom tooltip, and allow to drag tabs away.
class QMainWindowTabBar : public QTabBar
{
    QMainWindow *mainWindow;
    QDockWidget *draggingDock; // Currently dragging (detached) dock widget
public:
    QMainWindowTabBar(QMainWindow *parent);
protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent*) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent*) Q_DECL_OVERRIDE;

};

QMainWindowTabBar::QMainWindowTabBar(QMainWindow *parent)
    : QTabBar(parent), mainWindow(parent), draggingDock(0)
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
#endif // QT_NO_TABBAR

bool QMainWindowLayout::startSeparatorMove(const QPoint &pos)
{
    movingSeparator = layoutState.dockAreaLayout.findSeparator(pos);

    if (movingSeparator.isEmpty())
        return false;

    layoutState.dockAreaLayout.fallbackToSizeHints = false;

    savedState = layoutState;
    movingSeparatorPos = movingSeparatorOrigin = pos;

    return true;
}

bool QMainWindowLayout::separatorMove(const QPoint &pos)
{
    if (movingSeparator.isEmpty())
        return false;
    movingSeparatorPos = pos;
    separatorMoveTimer.start(0, this);
    return true;
}

bool QMainWindowLayout::endSeparatorMove(const QPoint&)
{
    if (movingSeparator.isEmpty())
        return false;
    movingSeparator.clear();
    savedState.clear();
    return true;
}

void QMainWindowLayout::raise(QDockWidget *widget)
{
    QDockAreaLayoutInfo *info = dockInfo(widget);
    if (info == 0)
        return;
#ifndef QT_NO_TABBAR
    if (!info->tabbed)
        return;
    info->setCurrentTab(widget);
#endif
}

#endif // QT_NO_DOCKWIDGET


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

#ifndef QT_NO_TOOLBAR
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
#if 0 // Used to be included in Qt4 for Q_WS_MAC
        const QSize storedSize = minSize;
        int minWidth = 0;
        foreach (QToolBar *toolbar, qtoolbarsInUnifiedToolbarList) {
            minWidth += toolbar->sizeHint().width() + 20;
        }
        minSize = QSize(qMax(minWidth, storedSize.width()), storedSize.height());
#endif
    }
    return minSize;
}

void QMainWindowLayout::invalidate()
{
    QLayout::invalidate();
    minSize = szHint = QSize();
}

/******************************************************************************
** QMainWindowLayout - remaining stuff
*/

static void fixToolBarOrientation(QLayoutItem *item, int dockPos)
{
#ifndef QT_NO_TOOLBAR
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
#ifndef QT_NO_DOCKWIDGET
    if (currentHoveredFloat) {
        QWidget *widget = widgetItem->widget();
        QList<int> previousPath = layoutState.indexOf(widget);
        if (!previousPath.isEmpty())
            layoutState.remove(previousPath);
        // Let's remove the widget from any possible group window
        foreach (QDockWidgetGroupWindow *dwgw,
                parent()->findChildren<QDockWidgetGroupWindow*>(QString(), Qt::FindDirectChildrenOnly)) {
            QList<int> path = dwgw->layoutInfo()->indexOf(widget);
            if (!path.isEmpty())
                dwgw->layoutInfo()->remove(path);
        }
        currentGapRect = QRect();

        if (QDockWidget *dropTo = qobject_cast<QDockWidget*>(currentHoveredFloat)) {
            //dropping to a normal widget, we mutate it in a QDockWidgetGroupWindow with two tabs
            QDockWidgetGroupWindow *floatingTabs = createTabbedDockWindow();
            floatingTabs->setGeometry(dropTo->geometry());
            QDockAreaLayoutInfo *info = floatingTabs->layoutInfo();
            *info = QDockAreaLayoutInfo(&layoutState.dockAreaLayout.sep, QInternal::LeftDock,
                                        Qt::Horizontal, QTabBar::RoundedSouth,
                                        static_cast<QMainWindow*>(parentWidget()));
            info->tabbed = true;
            QLayout *parentLayout = currentHoveredFloat->parentWidget()->layout();
            info->item_list.append(QDockAreaLayoutItem(parentLayout->takeAt(parentLayout->indexOf(currentHoveredFloat))));

            dropTo->setParent(floatingTabs);
            dropTo->show();
            dropTo->d_func()->plug(QRect());
            currentHoveredFloat = floatingTabs;
        }

        QDockWidgetGroupWindow *dwgw = qobject_cast<QDockWidgetGroupWindow *>(currentHoveredFloat);
        Q_ASSERT(dwgw);
        Q_ASSERT(dwgw->layoutInfo()->tabbed); // because floating group should always be tabbed
        previousPath = dwgw->layoutInfo()->indexOf(widget);
        if (!previousPath.isEmpty())
            dwgw->layoutInfo()->remove(previousPath);
        dwgw->layoutInfo()->tab(0, widgetItem);
        QRect globalRect = dwgw->layoutInfo()->tabContentRect();
        globalRect.moveTopLeft(dwgw->mapToGlobal(globalRect.topLeft()));
        pluggingWidget = widget;
        widgetAnimator.animate(widget, globalRect, dockOptions & QMainWindow::AnimatedDocks);
        return true;
    }
#endif

    if (!parentWidget()->isVisible() || parentWidget()->isMinimized() || currentGapPos.isEmpty())
        return false;

    fixToolBarOrientation(widgetItem, currentGapPos.at(1));

    QWidget *widget = widgetItem->widget();

#ifndef QT_NO_DOCKWIDGET
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
#ifndef QT_NO_DOCKWIDGET
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
#ifndef QT_NO_TOOLBAR
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

#ifndef QT_NO_DOCKWIDGET
        if (QDockWidgetGroupWindow *dwgw = qobject_cast<QDockWidgetGroupWindow *>(widget)) {
            // When the animated widget was a QDockWidgetGroupWindow, it means each of the
            // embedded QDockWidget needs to be plugged back into the QMainWindow layout.
            savedState.clear();
            QDockAreaLayoutInfo* info = dwgw->layoutInfo();
            QDockAreaLayoutInfo* parentInfo;
            QList<int> path;

            if (QDockWidgetGroupWindow *dropTo = qobject_cast<QDockWidgetGroupWindow *>(currentHoveredFloat)) {
                parentInfo = dropTo->layoutInfo();
                Q_ASSERT(parentInfo->tabbed);
                path = parentInfo->indexOf(widget);
                Q_ASSERT(path.size() == 1);
            } else {
                path = layoutState.dockAreaLayout.indexOf(widget);
                Q_ASSERT(path.size() >= 2);
                parentInfo = layoutState.dockAreaLayout.info(path);
                Q_ASSERT(parentInfo);
            }

            if (parentInfo->tabbed) {
                // merge the two tab widgets
                int idx = path.constLast();
                Q_ASSERT(parentInfo->item_list[idx].widgetItem->widget() == dwgw);
                delete parentInfo->item_list[idx].widgetItem;
                parentInfo->item_list.removeAt(idx);
                std::copy(info->item_list.cbegin(), info->item_list.cend(),
                          std::inserter(parentInfo->item_list, parentInfo->item_list.begin() + idx));
                quintptr currentId = info->currentTabId();
                *info = QDockAreaLayoutInfo();
                parentInfo->reparentWidgets(currentHoveredFloat ? currentHoveredFloat.data() : parentWidget());
                parentInfo->updateTabBar();
                parentInfo->setCurrentTabId(currentId);
            } else {
                QDockAreaLayoutItem &item = layoutState.dockAreaLayout.item(path);
                Q_ASSERT(item.widgetItem->widget() == dwgw);
                delete item.widgetItem;
                item.widgetItem = 0;
                item.subinfo = new QDockAreaLayoutInfo(qMove(*info));
                *info = QDockAreaLayoutInfo();
                item.subinfo->reparentWidgets(parentWidget());
                item.subinfo->setTabBarShape(parentInfo->tabBarShape);
            }
            dwgw->destroyOrHideIfEmpty();
        }

        if (QDockWidget *dw = qobject_cast<QDockWidget*>(widget)) {
            if (currentHoveredFloat) {
                dw->setParent(currentHoveredFloat);
                dw->show();
            }
            dw->d_func()->plug(currentGapRect);
        }
#endif
#ifndef QT_NO_TOOLBAR
        if (QToolBar *tb = qobject_cast<QToolBar*>(widget))
            tb->d_func()->plug(currentGapRect);
#endif

        savedState.clear();
        currentGapPos.clear();
        pluggingWidget = 0;
        currentHoveredFloat = Q_NULLPTR;
        //applying the state will make sure that the currentGap is updated correctly
        //and all the geometries (especially the one from the central widget) is correct
        layoutState.apply(false);

#ifndef QT_NO_DOCKWIDGET
#ifndef QT_NO_TABBAR
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
#ifndef QT_NO_DOCKWIDGET
        parentWidget()->update(layoutState.dockAreaLayout.separatorRegion());
#ifndef QT_NO_TABBAR
        foreach (QTabBar *tab_bar, usedTabBars)
            tab_bar->show();
#endif // QT_NO_TABBAR
#endif // QT_NO_DOCKWIDGET
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
#ifndef QT_NO_DOCKWIDGET
#ifndef QT_NO_TABBAR
    , _documentMode(false)
    , verticalTabsEnabled(false)
#ifndef QT_NO_TABWIDGET
    , _tabShape(QTabWidget::Rounded)
#endif
#endif
#endif // QT_NO_DOCKWIDGET
    , widgetAnimator(this)
    , pluggingWidget(0)
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    , blockVisiblityCheck(false)
#endif
{
    if (parentLayout)
        setParent(parentLayout);

#ifndef QT_NO_DOCKWIDGET
#ifndef QT_NO_TABBAR
    sep = mainwindow->style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent, 0, mainwindow);
#endif

#ifndef QT_NO_TABWIDGET
    for (int i = 0; i < QInternal::DockCount; ++i)
        tabPositions[i] = QTabWidget::South;
#endif
#endif // QT_NO_DOCKWIDGET
    pluggingWidget = 0;

    setObjectName(mainwindow->objectName() + QLatin1String("_layout"));
}

QMainWindowLayout::~QMainWindowLayout()
{
    layoutState.deleteAllLayoutItems();
    layoutState.deleteCentralWidgetItem();

#if 0 // Used to be included in Qt4 for Q_WS_MAC
    cleanUpMacToolbarItems();
#endif

    delete statusbar;
}

void QMainWindowLayout::setDockOptions(QMainWindow::DockOptions opts)
{
    if (opts == dockOptions)
        return;

    dockOptions = opts;

#ifndef QT_NO_DOCKWIDGET
    setVerticalTabsEnabled(opts & QMainWindow::VerticalTabs);
#endif

    invalidate();
}

#ifndef QT_NO_STATUSBAR
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
#endif // QT_NO_STATUSBAR

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
#ifndef QT_NO_DOCKWIDGET
        savedState.dockAreaLayout.centralWidgetItem = layoutState.dockAreaLayout.centralWidgetItem;
        savedState.dockAreaLayout.fallbackToSizeHints = true;
#else
        savedState.centralWidgetItem = layoutState.centralWidgetItem;
#endif
    }
    invalidate();
}

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
#if !defined(QT_NO_DOCKWIDGET) && !defined(QT_NO_TABBAR)
    if (!widget->isWindow() && qobject_cast<const QDockWidgetGroupWindow *>(widget->parentWidget())) {
        if (group) {
            // We are just dragging a floating window as it, not need to do anything, we just have to
            // look up the corresponding QWidgetItem* if it exists
            QList<int> tabbedWindowPath = layoutState.indexOf(widget->parentWidget());
            return tabbedWindowPath.isEmpty() ? 0 : layoutState.item(tabbedWindowPath);
        } else {
            // We are unplugging a dock widget from a floating window.
            if (QDockWidget *dw = qobject_cast<QDockWidget*>(widget)) {
                dw->d_func()->unplug(widget->geometry());
                return 0;
            }
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

#ifndef QT_NO_DOCKWIDGET
    if (QDockWidget *dw = qobject_cast<QDockWidget*>(widget)) {
        Q_ASSERT(path.constFirst() == 1);
        bool actualGroup = false;
#ifndef QT_NO_TABBAR
        if (group && (dockOptions & QMainWindow::GroupedDragging) && path.size() > 3) {
            QDockAreaLayoutItem &parentItem = layoutState.dockAreaLayout.item(path.mid(1, path.size() - 2));
            if (parentItem.subinfo && parentItem.subinfo->tabbed) {
                // The QDockWidget is part of a group of tab and we need to unplug them all.
                actualGroup = true;
                path.removeLast();

                QDockWidgetGroupWindow* floatingTabs = createTabbedDockWindow();
                QDockAreaLayoutInfo* info = floatingTabs->layoutInfo();
                *info = qMove(*parentItem.subinfo);
                delete parentItem.subinfo;
                parentItem.subinfo = 0;
                floatingTabs->setGeometry(info->rect.translated(parentWidget()->pos()));
                floatingTabs->show();
                floatingTabs->raise();
                item = new QDockWidgetGroupWindowItem(floatingTabs);
                parentItem.widgetItem = item;
                savedState = layoutState;
            }
        }
#endif // QT_NO_TABBAR
        if (!actualGroup) {
            dw->d_func()->unplug(r);
        }
    }
#endif // QT_NO_DOCKWIDGET
#ifndef QT_NO_TOOLBAR
    if (QToolBar *tb = qobject_cast<QToolBar*>(widget)) {
        tb->d_func()->unplug(r);
    }
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
#ifndef QT_NO_RUBBERBAND
    if ((!widgetAnimator.animating() && !currentGapPos.isEmpty()) || currentHoveredFloat) {
        QWidget *expectedParent = currentHoveredFloat ? currentHoveredFloat.data() : parentWidget();
        if (!gapIndicator) {
            gapIndicator = new QRubberBand(QRubberBand::Rectangle, expectedParent);
            // For accessibility to identify this special widget.
            gapIndicator->setObjectName(QLatin1String("qt_rubberband"));
        } else if (gapIndicator->parent() != expectedParent) {
            gapIndicator->setParent(expectedParent);
        }
        gapIndicator->setGeometry(currentHoveredFloat ? currentHoveredFloat->rect() : currentGapRect);
        gapIndicator->show();
        gapIndicator->raise();
    } else if (gapIndicator) {
        gapIndicator->hide();
    }
#endif //QT_NO_RUBBERBAND
}

void QMainWindowLayout::hover(QLayoutItem *widgetItem, const QPoint &mousePos)
{
    if (!parentWidget()->isVisible() || parentWidget()->isMinimized()
        || pluggingWidget != 0 || widgetItem == 0)
        return;

    QWidget *widget = widgetItem->widget();

#ifndef QT_NO_DOCKWIDGET
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

            currentHoveredFloat = w;
            restore(true);
            return;
        }
    }
    currentHoveredFloat = Q_NULLPTR;
#endif //QT_NO_DOCKWIDGET

    QPoint pos = parentWidget()->mapFromGlobal(mousePos);

    if (!savedState.isValid())
        savedState = layoutState;

    QList<int> path = savedState.gapIndex(widget, pos);

    if (!path.isEmpty()) {
        bool allowed = false;

#ifndef QT_NO_DOCKWIDGET
        if (QDockWidget *dw = qobject_cast<QDockWidget*>(widget))
            allowed = dw->isAreaAllowed(toDockWidgetArea(path.at(1)));

        if (qobject_cast<QDockWidgetGroupWindow *>(widget))
            allowed = true;
#endif
#ifndef QT_NO_TOOLBAR
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

#ifndef QT_NO_DOCKWIDGET
    parentWidget()->update(layoutState.dockAreaLayout.separatorRegion());
#endif
    layoutState = newState;
    applyState(layoutState);

    updateGapIndicator();
}

QDockWidgetGroupWindow *QMainWindowLayout::createTabbedDockWindow()
{
    QDockWidgetGroupWindow* f = new QDockWidgetGroupWindow(parentWidget(), Qt::Tool);
    new QDockWidgetGroupLayout(f);
    return f;
}

void QMainWindowLayout::applyState(QMainWindowLayoutState &newState, bool animate)
{
#ifndef QT_NO_DOCKWIDGET
#ifndef QT_NO_TABBAR
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

#endif // QT_NO_TABBAR
#endif // QT_NO_DOCKWIDGET
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

#ifndef QT_NO_DOCKWIDGET
    if (parentWidget()->isVisible()) {
#ifndef QT_NO_TABBAR
        foreach (QTabBar *tab_bar, usedTabBars)
            tab_bar->show();

#endif
    }
#endif // QT_NO_DOCKWIDGET

    return true;
}


// Returns if this toolbar *should* be using HIToolbar. Won't work for all in between cases
// for example, you have a toolbar in the top area and then you suddenly turn on
// HIToolbar.
bool QMainWindowLayout::usesHIToolBar(QToolBar *toolbar) const
{
#if 1 // Used to be excluded in Qt4 for Q_WS_MAC
    Q_UNUSED(toolbar);
    return false;
#else
    return qtoolbarsInUnifiedToolbarList.contains(toolbar)
           || ((toolBarArea(toolbar) == Qt::TopToolBarArea)
                && layoutState.mainWindow->unifiedTitleAndToolBarOnMac());
#endif
}

void QMainWindowLayout::timerEvent(QTimerEvent *e)
{
#ifndef QT_NO_DOCKWIDGET
    if (e->timerId() == separatorMoveTimer.timerId()) {
        //let's move the separators
        separatorMoveTimer.stop();
        if (movingSeparator.isEmpty())
            return;
        if (movingSeparatorOrigin == movingSeparatorPos)
            return;

        //when moving the separator, we need to update the previous position
        parentWidget()->update(layoutState.dockAreaLayout.separatorRegion());

        layoutState = savedState;
        layoutState.dockAreaLayout.separatorMove(movingSeparator, movingSeparatorOrigin,
                                                    movingSeparatorPos);
        movingSeparatorPos = movingSeparatorOrigin;
    }
#endif
    QLayout::timerEvent(e);
}


QT_END_NAMESPACE

#include "moc_qmainwindowlayout_p.cpp"

#endif // QT_NO_MAINWINDOW
