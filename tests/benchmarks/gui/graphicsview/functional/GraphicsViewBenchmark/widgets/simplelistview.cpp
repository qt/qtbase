/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "simplelistview.h"
#include "scrollbar.h"
#include "simplelistview.h"
#include "scrollbar.h"
#include "listitem.h"
#include "listitemcache.h"
#include "theme.h"

class SimpleListViewPrivate
{
    Q_DECLARE_PUBLIC(SimpleListView)

public:

    SimpleListViewPrivate(SimpleListView *button)
        : m_content(0)
        , m_layout(0)
        , m_twoColumns(false)
        , q_ptr(button)
        , m_listItemCaching(false)
    {
        Q_Q(SimpleListView);

        m_layout = new QGraphicsGridLayout();
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);
        m_layout->setColumnSpacing(0,0);
        m_layout->setColumnSpacing(1,0);
        m_layout->setRowSpacing(0,0);
        m_layout->setRowSpacing(1,0);
        m_content = new QGraphicsWidget;
        m_content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_content->setParentItem(q->viewport());
        m_content->setLayout(m_layout);

        q->horizontalScrollBar()->setSliderSize(0.0);

        QObject::connect(Theme::p(), SIGNAL(themeChanged()), q, SLOT(themeChange()));
    }

    ~SimpleListViewPrivate()
    {
        if (!m_content->parentItem() && !m_content->parent())
            delete m_content;
    }

    void resizeContents(QSizeF s)
    {
        Q_UNUSED(s);
        Q_Q(SimpleListView);

        if (!m_content)
            return;

        const bool caching = q->listItemCaching();
        q->setListItemCaching(false);

        m_content->resize(q->viewport()->size().width(),
                        m_layout->preferredHeight());
        const bool clip =
            m_content->size().width() > q->viewport()->size().width()
                || m_content->size().height() > q->viewport()->size().height();

        q->viewport()->setFlag(
                QGraphicsItem::ItemClipsChildrenToShape, clip);

        q->setListItemCaching(caching);
    }

    void resizeScrollBars()
    {
        Q_Q(SimpleListView);

        if (!m_content)
            return;

        m_content->resize(m_content->size().width(),
                          m_layout->preferredHeight());

        QRectF contentRect = m_content->boundingRect();
        QRectF listRect = q->boundingRect();

        // List do not have horizontal scroll bar visible.
        q->horizontalScrollBar()->setSliderSize(0.0);

        if (contentRect.height()-q->boundingRect().height() > 0) {
            q->verticalScrollBar()->setSliderSize(contentRect.height()-q->boundingRect().height());
            if (q->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff &&
                !q->verticalScrollBar()->isVisible()) {
                q->verticalScrollBar()->show();
            }
        }
        else if (q->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded ||
                 q->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff) {
            q->verticalScrollBar()->setSliderSize(0.0);
            q->verticalScrollBar()->hide();
        }
        else {
            q->verticalScrollBar()->setSliderSize(0.0);
        }

        qreal pos = 0.0;
        if ((m_content->boundingRect().height() - q->boundingRect().height()) != 0) {
            qreal min = qMin(-contentRect.top(), m_content->pos().y());
            qreal diff = contentRect.height() - listRect.height();
            pos = qAbs(contentRect.top() + min) / diff;
        }

        q->verticalScrollBar()->setSliderPosition(pos);
    }

    void updateListContents()
    {
        Q_Q(SimpleListView);

        const bool caching = q->listItemCaching();
        q->setListItemCaching(false);

        const QString defaultIcon = Theme::p()->pixmapPath()+"contact_default_icon.svg";
        const int itemCount = m_layout->count();

        for (int i=0; i<itemCount; ++i) {
            ListItem* item = static_cast<ListItem*>(m_layout->itemAt(i));

            // Update default icons
            const QString filename = item->icon(ListItem::LeftIcon)->fileName();
            if (filename.contains("contact_default_icon")) {
                item->icon(ListItem::LeftIcon)->setFileName(defaultIcon);
            }

            // Update status icons
            QString statusIcon = item->icon(ListItem::RightIcon)->fileName();
            const int index = statusIcon.indexOf("contact_status");
            if (index != -1) {
                statusIcon.remove(0, index);
                item->icon(ListItem::RightIcon)->setFileName(Theme::p()->pixmapPath()+statusIcon);
            }

            // Update fonts
            item->setFont(Theme::p()->font(Theme::ContactName), ListItem::FirstPos);
            item->setFont(Theme::p()->font(Theme::ContactNumber), ListItem::SecondPos);
            item->setFont(Theme::p()->font(Theme::ContactEmail), ListItem::ThirdPos);

            // Update list dividers
            if (i%2) {
                item->setBackgroundBrush(Theme::p()->listItemBackgroundBrushOdd());
                item->setBackgroundOpacity(Theme::p()->listItemBackgroundOpacityOdd());
            }
            else {
                item->setBackgroundBrush(Theme::p()->listItemBackgroundBrushEven());
                item->setBackgroundOpacity(Theme::p()->listItemBackgroundOpacityEven());
            }

            // Update borders
            item->setBorderPen(Theme::p()->listItemBorderPen());
            item->setRounding(Theme::p()->listItemRounding());

            // Update icons
            item->icon(ListItem::LeftIcon)->setRotation(Theme::p()->iconRotation(ListItem::LeftIcon));
            item->icon(ListItem::RightIcon)->setRotation(Theme::p()->iconRotation(ListItem::RightIcon));
            item->icon(ListItem::LeftIcon)->setOpacityEffectEnabled(Theme::p()->isIconOpacityEffectEnabled(ListItem::LeftIcon));
            item->icon(ListItem::RightIcon)->setOpacityEffectEnabled(Theme::p()->isIconOpacityEffectEnabled(ListItem::RightIcon));
            item->icon(ListItem::LeftIcon)->setSmoothTransformationEnabled(Theme::p()->isIconSmoothTransformationEnabled(ListItem::LeftIcon));
            item->icon(ListItem::RightIcon)->setSmoothTransformationEnabled(Theme::p()->isIconSmoothTransformationEnabled(ListItem::RightIcon));
        }
        q->setListItemCaching(caching);
    }

    void updateListItemBackgrounds(int index)
    {
        Q_Q(SimpleListView);

        const bool caching = q->listItemCaching();
        q->setListItemCaching(false);

        const int itemCount = m_layout->count();

        for (int i=index; i<itemCount; ++i) {
            ListItem* item = static_cast<ListItem*>(m_layout->itemAt(i));
            if (i%2) {
                item->setBackgroundBrush(Theme::p()->listItemBackgroundBrushOdd());
                item->setBackgroundOpacity(Theme::p()->listItemBackgroundOpacityOdd());
            }
            else {
                item->setBackgroundBrush(Theme::p()->listItemBackgroundBrushEven());
                item->setBackgroundOpacity(Theme::p()->listItemBackgroundOpacityEven());
            }
        }

        q->setListItemCaching(caching);
    }

    void setTwoColumns(const bool twoColumns)
    {
        if(twoColumns == m_twoColumns)
            return;

        Q_Q(SimpleListView);
        m_twoColumns = twoColumns;

        bool cache = q->listItemCaching();
        q->setListItemCaching(false);

        QList<QGraphicsLayoutItem *> moveditems;
        if(twoColumns) {
            int half = m_layout->count()/2;
            for (int i = m_layout->count()-1; i>=half; --i) {
                QGraphicsLayoutItem *item = m_layout->itemAt(i);
                m_layout->removeAt(i);
                moveditems.append(item);
            }
            for ( int i = 0; i < moveditems.count(); ++i)
                m_layout->addItem(moveditems.at(i), i, 1);

            m_layout->setColumnSpacing(0,0);
            m_layout->setColumnSpacing(1,0);
            m_layout->setRowSpacing(0,0);
            m_layout->setRowSpacing(1,0);
        }
        else {
            int count = m_layout->count()/2;
            for (int i = m_layout->count()-1;  i>=0; --i) {
                if (i >= count)
                    moveditems.append(m_layout->itemAt(i));
                else
                    moveditems.insert(moveditems.begin(), m_layout->itemAt(i));
                m_layout->removeAt(i);
            }
            for (int i = 0; i<moveditems.count(); ++i) {
                m_layout->addItem(moveditems.at(i), m_layout->count(), 0);
            }
        }

        resizeContents(q->size());
        resizeScrollBars();

        q->setListItemCaching(cache);
    }

    bool twoColumns()
    {
        return m_twoColumns;
    }

    QGraphicsWidget *m_content;
    QGraphicsGridLayout *m_layout;
    bool m_twoColumns;
    SimpleListView *q_ptr;
    bool m_listItemCaching;
};

SimpleListView::SimpleListView(QGraphicsWidget *parent)
    : AbstractScrollArea(parent)
    , d_ptr(new SimpleListViewPrivate(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(0, 0, 0, 0);
    verticalScrollBar()->hide();
    horizontalScrollBar()->hide();
}

SimpleListView::~SimpleListView()
{
    Q_D(SimpleListView);

    if (d->m_content) {
        d->m_content->setParentItem(0);
    }

    delete d_ptr;
}

void SimpleListView::addItem(QGraphicsWidget *item)
{
    Q_D(SimpleListView);

    Q_ASSERT(item);

    insertItem(d->m_layout->count(), item);
}

void SimpleListView::insertItem(int index, QGraphicsWidget *item)
{
    Q_D(SimpleListView);

    // Grid layout doe not have insert item method.
    // We need to first remove all items, add new item and
    // append old items to layout.
    QList<QGraphicsLayoutItem *> moveditems;

    for (int i = d->m_layout->count()-1; i >= index; --i) {
        moveditems.append(d->m_layout->itemAt(i));
        d->m_layout->removeAt(i);
    }
    moveditems.append(item);

    for (int i = moveditems.count()-1; i>=0; --i) {
        d->m_layout->addItem(moveditems.at(i), d->m_layout->count(), 0);
    }

    ListItemCache *cache = new ListItemCache;
    item->setGraphicsEffect(cache);
    cache->setEnabled(listItemCaching());

    d->resizeScrollBars();
    d->updateListItemBackgrounds(index);
}

QGraphicsWidget *SimpleListView::takeItem(int index)
{
    Q_D(SimpleListView);

    QGraphicsWidget *item = 0;

    if (index >= 0 && d->m_layout->count() > index) {
        QList<QGraphicsLayoutItem *> moveditems;
        for (int i = d->m_layout->count()-1; i >=0; --i)  {
            if (index != i)
                moveditems.insert(moveditems.begin(), d->m_layout->itemAt(i));
            else {
                item = static_cast<QGraphicsWidget*>(d->m_layout->itemAt(index));
                item->setGraphicsEffect(0);
            }

            d->m_layout->removeAt(i);
        }

        for (int i = 0; i < moveditems.count(); ++i)
            d->m_layout->addItem(moveditems.at(i), d->m_layout->count(), 0);
    }
    d->resizeScrollBars();
    return item;
}

QGraphicsWidget *SimpleListView::itemAt(int row)
{
    Q_D(SimpleListView);

    QGraphicsWidget *item = 0;

    if (row >= 0 && d->m_layout->count() > row) {
        item = static_cast<QGraphicsWidget*>(d->m_layout->itemAt(row));
    }

    return item;
}

int SimpleListView::itemCount()
{
    Q_D(SimpleListView);
    return d->m_layout->count();
}

bool SimpleListView::listItemCaching() const
{
    Q_D(const SimpleListView);
    return d->m_listItemCaching;
}

void SimpleListView::setListItemCaching(bool enabled)
{
    Q_D(SimpleListView);

    if (d->m_listItemCaching == enabled)
        return;

    d->m_listItemCaching = enabled;

    for (int i = 0; i < d->m_layout->count(); ++i) {
        ListItem *item = static_cast<ListItem*>(d->m_layout->itemAt(i));
        ListItemCache *cache = static_cast<ListItemCache *>(
            item->graphicsEffect());
        cache->setEnabled(enabled);
    }
}

void SimpleListView::scrollContentsBy(qreal dx, qreal dy)
{
    Q_D(SimpleListView);
    Q_UNUSED(dx)
    QRectF contentRect = d->m_content->boundingRect();
    QRectF viewportRect = viewport()->boundingRect();
    QPointF contentPos = d->m_content->pos();

    qreal newy = contentPos.y() - dy;
    qreal miny = qMin(qreal(0.0), -(contentRect.height() - viewportRect.height()));

    if (newy < miny)
        newy = miny;
    else if (newy > 0)
        newy = 0.0;

    d->m_content->setPos(contentPos.x(), newy);
}

void SimpleListView::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    Q_D(SimpleListView);

    AbstractScrollArea::resizeEvent(event);
    d->resizeContents(event->newSize());
    d->resizeScrollBars();
}

QSizeF SimpleListView::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_D(const SimpleListView);

    if (which == Qt::PreferredSize)
        return d->m_content->size();

    return AbstractScrollArea::sizeHint(which, constraint);
}

void SimpleListView::themeChange()
{
    Q_D(SimpleListView);

    d->updateListContents();
    d->resizeScrollBars();
}

void SimpleListView::setTwoColumns(const bool twoColumns)
{
    Q_D(SimpleListView);
    d->setTwoColumns(twoColumns);
}

bool SimpleListView::twoColumns()
{
    Q_D(SimpleListView);
    return d->twoColumns();
}
