// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QGraphicsLayout>

class FlowLayout : public QGraphicsLayout
{
public:
    FlowLayout(QGraphicsLayoutItem *parent = nullptr);
    inline void addItem(QGraphicsLayoutItem *item);
    void insertItem(int index, QGraphicsLayoutItem *item);
    void setSpacing(Qt::Orientations o, qreal spacing);
    qreal spacing(Qt::Orientation o) const;

    // inherited functions
    void setGeometry(const QRectF &geom) override;

    int count() const override;
    QGraphicsLayoutItem *itemAt(int index) const override;
    void removeAt(int index) override;

protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override;

private:
    qreal doLayout(const QRectF &geom, bool applyNewGeometry) const;
    QSizeF minSize(const QSizeF &constraint) const;
    QSizeF prefSize() const;
    QSizeF maxSize() const;

    QList<QGraphicsLayoutItem *> m_items;
    qreal m_spacing[2] = {6, 6};
};


inline void FlowLayout::addItem(QGraphicsLayoutItem *item)
{
    insertItem(-1, item);
}

#endif // FLOWLAYOUT_H
