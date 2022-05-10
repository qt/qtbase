// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
#ifndef CARD_H
#define CARD_H

#include <QtWidgets>
#include <QList>

class CardLayout : public QLayout
{
public:
    CardLayout(int spacing): QLayout()
    { setSpacing(spacing); }
    CardLayout(int spacing, QWidget *parent): QLayout(parent)
    { setSpacing(spacing); }
    ~CardLayout();

    void addItem(QLayoutItem *item) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    int count() const override;
    QLayoutItem *itemAt(int) const override;
    QLayoutItem *takeAt(int) override;
    void setGeometry(const QRect &rect) override;

private:
    QList<QLayoutItem *> m_items;
};
#endif
//! [0]


//! [1]
//#include "card.h"
//! [1]

//! [2]
int CardLayout::count() const
{
    // QList::size() returns the number of QLayoutItems in m_items
    return m_items.size();
}
//! [2]

//! [3]
QLayoutItem *CardLayout::itemAt(int idx) const
{
    // QList::value() performs index checking, and returns nullptr if we are
    // outside the valid range
    return m_items.value(idx);
}

QLayoutItem *CardLayout::takeAt(int idx)
{
    // QList::take does not do index checking
    return idx >= 0 && idx < m_items.size() ? m_items.takeAt(idx) : 0;
}
//! [3]


//! [4]
void CardLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
}
//! [4]


//! [5]
CardLayout::~CardLayout()
{
     QLayoutItem *item;
     while ((item = takeAt(0)))
         delete item;
}
//! [5]


//! [6]
void CardLayout::setGeometry(const QRect &r)
{
    QLayout::setGeometry(r);

    if (m_items.size() == 0)
        return;

    int w = r.width() - (m_items.count() - 1) * spacing();
    int h = r.height() - (m_items.count() - 1) * spacing();
    int i = 0;
    while (i < m_items.size()) {
        QLayoutItem *o = m_items.at(i);
        QRect geom(r.x() + i * spacing(), r.y() + i * spacing(), w, h);
        o->setGeometry(geom);
        ++i;
    }
}
//! [6]


//! [7]
QSize CardLayout::sizeHint() const
{
    QSize s(0, 0);
    int n = m_items.count();
    if (n > 0)
        s = QSize(100, 70); //start with a nice default size
    int i = 0;
    while (i < n) {
        QLayoutItem *o = m_items.at(i);
        s = s.expandedTo(o->sizeHint());
        ++i;
    }
    return s + n * QSize(spacing(), spacing());
}

QSize CardLayout::minimumSize() const
{
    QSize s(0, 0);
    int n = m_items.count();
    int i = 0;
    while (i < n) {
        QLayoutItem *o = m_items.at(i);
        s = s.expandedTo(o->minimumSize());
        ++i;
    }
    return s + n * QSize(spacing(), spacing());
}
//! [7]
