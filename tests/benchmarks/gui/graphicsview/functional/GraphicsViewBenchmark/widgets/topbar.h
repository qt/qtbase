/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef __TOPBAR_H__
#define __TOPBAR_H__

#include <QObject>
#include <QHash>

#include "gvbwidget.h"

class QPixmap;
class QPoint;
class QGraphicsView;
class QFont;

class TopBar : public GvbWidget
{
    Q_OBJECT

public:
    enum Orientation
    {
        Portrait,
        Landscape,
        None
    };

public:
    TopBar(QGraphicsView* mainView, QGraphicsWidget* parent);
    ~TopBar();

public:
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
    QRectF boundingRect() const;
    void resizeEvent(QGraphicsSceneResizeEvent *event);
    inline QPoint getStatusBarLocation()
    {
        return m_topBarStatusBarMiddlePoint + m_topBarStatusBarMiddle.rect().bottomLeft();
    }

public slots:
    void themeChange();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

signals:
    void clicked(bool checked = false);

private:
    QSizeF sizeHint(Qt::SizeHint which,
        const QSizeF &constraint = QSizeF()) const;
    void setDefaultSizes();

private:
    Q_DISABLE_COPY(TopBar)

    QGraphicsView* m_mainView;
    bool m_isLimeTheme;
    Orientation m_orientation;

    //Fonts
    QFont m_titleFont;
    QFont m_statusFont;

    //Pixmaps
    QPixmap m_topBarPixmap;
    QPixmap m_topBarUserIcon;
    QPixmap m_topBarUserStatus;
    QPixmap m_topBarStatusBarLeft;
    QPixmap m_topBarStatusBarRight;
    QPixmap m_topBarStatusBarMiddle;

    //Drawing points
    QPoint m_topBarUserIconPoint;
    QPoint m_topBarUserStatusPoint;
    QPoint m_topBarStatusBarLeftPoint;
    QPoint m_topBarStatusBarRightPoint;
    QPoint m_topBarStatusBarMiddlePoint;
    QPoint m_topBarStatusBarTextPoint;
    QPoint m_topBarTitlePoint;

    //Sizes
    QHash<QString, QSize> m_sizesBlue;
    QHash<QString, QSize> m_sizesLime;
};

#endif // __TOPBAR_H__
