/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QTREEWIDGETITEMITERATOR_P_H
#define QTREEWIDGETITEMITERATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstack.h>

#include "qtreewidgetitemiterator.h"
#if QT_CONFIG(treewidget)

QT_BEGIN_NAMESPACE

class QTreeModel;
class QTreeWidgetItem;

class QTreeWidgetItemIteratorPrivate {
    Q_DECLARE_PUBLIC(QTreeWidgetItemIterator)
public:
    QTreeWidgetItemIteratorPrivate(QTreeWidgetItemIterator *q, QTreeModel *model)
        : m_currentIndex(0), m_model(model), q_ptr(q)
    {

    }

    QTreeWidgetItemIteratorPrivate(const QTreeWidgetItemIteratorPrivate& other)
        : m_currentIndex(other.m_currentIndex), m_model(other.m_model),
          m_parentIndex(other.m_parentIndex), q_ptr(other.q_ptr)
    {

    }

    QTreeWidgetItemIteratorPrivate &operator=(const QTreeWidgetItemIteratorPrivate& other)
    {
        m_currentIndex = other.m_currentIndex;
        m_parentIndex = other.m_parentIndex;
        m_model = other.m_model;
        return (*this);
    }

    ~QTreeWidgetItemIteratorPrivate()
    {
    }

    QTreeWidgetItem* nextSibling(const QTreeWidgetItem* item) const;
    void ensureValidIterator(const QTreeWidgetItem *itemToBeRemoved);

    QTreeWidgetItem *next(const QTreeWidgetItem *current);
    QTreeWidgetItem *previous(const QTreeWidgetItem *current);
private:
    int             m_currentIndex;
    QTreeModel     *m_model;        // This iterator class should not have ownership of the model.
    QStack<int>     m_parentIndex;
    QTreeWidgetItemIterator *q_ptr;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(treewidget)

#endif //QTREEWIDGETITEMITERATOR_P_H
