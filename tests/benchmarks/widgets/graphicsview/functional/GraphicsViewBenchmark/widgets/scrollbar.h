// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <QGraphicsWidget>
#include <QPixmap>

class ScrollBarPrivate;

class ScrollBar : public QGraphicsWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ScrollBar)

public:

    ScrollBar(Qt::Orientation orientation, QGraphicsWidget *parent=0);
    virtual ~ScrollBar();

public:

    bool sliderDown() const;
    qreal sliderPosition() const;
    qreal sliderSize() const;
    void setSliderSize(const qreal s);

signals:

    void sliderPressed();

    void sliderPositionChange(qreal position);

public slots:

    void setSliderPosition(qreal pos);
    void themeChange();

private:

    void paint(QPainter *painter,
        const QStyleOptionGraphicsItem *option,
        QWidget *widget);

    QSizeF sizeHint(Qt::SizeHint which,
        const QSizeF &constraint = QSizeF()) const;

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
    Q_DISABLE_COPY(ScrollBar)
    ScrollBarPrivate *d_ptr;
};

#endif // SCROLLBAR_H
