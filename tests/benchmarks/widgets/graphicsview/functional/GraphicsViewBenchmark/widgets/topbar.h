// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
