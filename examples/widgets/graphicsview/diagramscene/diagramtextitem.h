// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIAGRAMTEXTITEM_H
#define DIAGRAMTEXTITEM_H

#include <QGraphicsTextItem>

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

//! [0]
class DiagramTextItem : public QGraphicsTextItem
{
    Q_OBJECT

public:
    enum { Type = UserType + 3 };

    DiagramTextItem(QGraphicsItem *parent = nullptr);

    int type() const override { return Type; }

signals:
    void lostFocus(DiagramTextItem *item);
    void selectedChange(QGraphicsItem *item);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
};
//! [0]

#endif // DIAGRAMTEXTITEM_H
