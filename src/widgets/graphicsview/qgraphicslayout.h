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

#ifndef QGRAPHICSLAYOUT_H
#define QGRAPHICSLAYOUT_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qgraphicslayoutitem.h>

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QGraphicsLayoutPrivate;
class QGraphicsLayoutItem;
class QGraphicsWidget;

class Q_WIDGETS_EXPORT QGraphicsLayout : public QGraphicsLayoutItem
{
public:
    QGraphicsLayout(QGraphicsLayoutItem *parent = nullptr);
    ~QGraphicsLayout();

    void setContentsMargins(qreal left, qreal top, qreal right, qreal bottom);
    void getContentsMargins(qreal *left, qreal *top, qreal *right, qreal *bottom) const override;

    void activate();
    bool isActivated() const;
    virtual void invalidate();
    virtual void updateGeometry() override;

    virtual void widgetEvent(QEvent *e);

    virtual int count() const = 0;
    virtual QGraphicsLayoutItem *itemAt(int i) const = 0;
    virtual void removeAt(int index) = 0;

    static void setInstantInvalidatePropagation(bool enable);
    static bool instantInvalidatePropagation();
protected:
    QGraphicsLayout(QGraphicsLayoutPrivate &, QGraphicsLayoutItem *);
    void addChildLayoutItem(QGraphicsLayoutItem *layoutItem);

private:
    Q_DISABLE_COPY(QGraphicsLayout)
    Q_DECLARE_PRIVATE(QGraphicsLayout)
    friend class QGraphicsWidget;
};

#ifndef Q_CLANG_QDOC
Q_DECLARE_INTERFACE(QGraphicsLayout, "org.qt-project.Qt.QGraphicsLayout")
#endif

QT_END_NAMESPACE

#endif
