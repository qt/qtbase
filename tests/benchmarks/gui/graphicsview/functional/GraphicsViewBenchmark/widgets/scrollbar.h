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
