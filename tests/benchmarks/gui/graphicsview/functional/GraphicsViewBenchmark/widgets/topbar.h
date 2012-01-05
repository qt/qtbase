/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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
    inline QPoint getStatusBarLocation() { return m_topBarStatusBarMiddlePoint + 
        m_topBarStatusBarMiddle.rect().bottomLeft(); }
    
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
