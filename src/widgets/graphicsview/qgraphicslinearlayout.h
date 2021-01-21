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

#ifndef QGRAPHICSLINEARLAYOUT_H
#define QGRAPHICSLINEARLAYOUT_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qgraphicsitem.h>
#include <QtWidgets/qgraphicslayout.h>

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QGraphicsLinearLayoutPrivate;

class Q_WIDGETS_EXPORT QGraphicsLinearLayout : public QGraphicsLayout
{
public:
    QGraphicsLinearLayout(QGraphicsLayoutItem *parent = nullptr);
    QGraphicsLinearLayout(Qt::Orientation orientation, QGraphicsLayoutItem *parent = nullptr);
    virtual ~QGraphicsLinearLayout();

    void setOrientation(Qt::Orientation orientation);
    Qt::Orientation orientation() const;

    inline void addItem(QGraphicsLayoutItem *item) { insertItem(-1, item); }
    inline void addStretch(int stretch = 1) { insertStretch(-1, stretch); }

    void insertItem(int index, QGraphicsLayoutItem *item);
    void insertStretch(int index, int stretch = 1);

    void removeItem(QGraphicsLayoutItem *item);
    void removeAt(int index) override;

    void setSpacing(qreal spacing);
    qreal spacing() const;
    void setItemSpacing(int index, qreal spacing);
    qreal itemSpacing(int index) const;

    void setStretchFactor(QGraphicsLayoutItem *item, int stretch);
    int stretchFactor(QGraphicsLayoutItem *item) const;

    void setAlignment(QGraphicsLayoutItem *item, Qt::Alignment alignment);
    Qt::Alignment alignment(QGraphicsLayoutItem *item) const;

    void setGeometry(const QRectF &rect) override;

    int count() const override;
    QGraphicsLayoutItem *itemAt(int index) const override;

    void invalidate() override;
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override;

#if 0 // ###
    Q5SizePolicy::ControlTypes controlTypes(LayoutSide side) const;
#endif

    void dump(int indent = 0) const;

protected:
#if 0
    QSize contentsSizeHint(Qt::SizeHint which, const QSize &constraint = QSize()) const;
#endif

private:
    Q_DISABLE_COPY(QGraphicsLinearLayout)
    Q_DECLARE_PRIVATE(QGraphicsLinearLayout)
};

QT_END_NAMESPACE

#endif
