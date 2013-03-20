/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>
#include <QPixmap>
#include <QFont>

#include "themeevent.h"
#include "theme.h"
#include "topbar.h"
#include "mainview.h"

TopBar::TopBar(QGraphicsView* mainView, QGraphicsWidget* parent) :
    GvbWidget(parent), m_mainView(mainView), m_isLimeTheme(false),
    m_orientation(TopBar::None), m_topBarPixmap(), m_sizesBlue(), m_sizesLime()
{
    setDefaultSizes();

    m_titleFont = Theme::p()->font(Theme::TitleBar);
    m_statusFont = Theme::p()->font(Theme::StatusBar);

    connect(Theme::p(), SIGNAL(themeChanged()), this, SLOT(themeChange()));
}

TopBar::~TopBar()
{
}

void TopBar::resizeEvent(QGraphicsSceneResizeEvent* /*event*/)
{
    //Check orientation
    QString topbarName;
    QSize mainViewSize = m_mainView->size();
    int rotationAngle = static_cast<MainView*>(m_mainView)->rotationAngle();
    if(rotationAngle == 90 || rotationAngle == 270 ) {
       int wd = mainViewSize.width();
       int ht = mainViewSize.height();
       mainViewSize.setWidth(ht);
       mainViewSize.setHeight(wd);
    }
    bool m_orientationChanged = false;
    if(mainViewSize.height() >= mainViewSize.width()) {
        if(m_orientation == TopBar::Landscape)
            m_orientationChanged = true;
        m_orientation = TopBar::Portrait;
        topbarName = "topbar.svg";
    }
    else {
        if(m_orientation == TopBar::Portrait)
            m_orientationChanged = true;
        m_orientation = TopBar::Landscape;
        topbarName = "topbar_horisontal.svg";
    }

    //Calculate new size, resize by height, don't make it wider than the screen
    QHash<QString, QSize>sizes = (Theme::p()->theme() == Theme::Blue) ?
        m_sizesBlue : m_sizesLime;

    //Get current size for topbarpixmap
    QSize currentSize = !m_topBarPixmap.isNull() && !m_orientationChanged ?
            m_topBarPixmap.size() : sizes[topbarName];
    QSize newSize = !m_orientationChanged ? QSize(currentSize) : sizes[topbarName];

    //Scale according to aspect ratio
    newSize.scale(size().toSize(), Qt::KeepAspectRatio);

    //fix width to window widht if previous scaling produced too narrow image
    if(newSize.width() < size().width()) {
        newSize.scale(size().toSize(), Qt::KeepAspectRatioByExpanding);
    }

    //Calculate scaling factor for rest of the graphics scaling
    qreal scaleFactor = (newSize.width() *1.0) / (currentSize.width() * 1.0);

    //Scale graphics, if the scalefactor applies
    //This is really heavy since the SVG graphics are read again from the resource
    if(scaleFactor != 1 || m_topBarPixmap.isNull() )  {
        m_topBarPixmap = Theme::p()->pixmap(topbarName, newSize );
        m_topBarUserIcon = Theme::p()->pixmap("user_default_icon.svg",
                !m_topBarUserIcon.isNull() && !m_orientationChanged ? m_topBarUserIcon.size()* scaleFactor : sizes["user_default_icon.svg"] * scaleFactor);

        m_topBarUserStatus = Theme::p()->pixmap("user_status_online.svg",
                !m_topBarUserStatus.isNull() && !m_orientationChanged ? m_topBarUserStatus.size() * scaleFactor : sizes["user_status_online.svg"] * scaleFactor);

        m_topBarStatusBarLeft = Theme::p()->pixmap("status_field_left.svg",
                !m_topBarStatusBarLeft.isNull() && !m_orientationChanged ? m_topBarStatusBarLeft.size()* scaleFactor : sizes["status_field_left.svg"] * scaleFactor);

        m_topBarStatusBarRight = Theme::p()->pixmap("status_field_right.svg",
                !m_topBarStatusBarRight.isNull() && !m_orientationChanged ? m_topBarStatusBarRight.size()* scaleFactor : sizes["status_field_right.svg"] * scaleFactor);

        m_topBarStatusBarMiddle = Theme::p()->pixmap("status_field_middle.svg",
                !m_topBarStatusBarMiddle.isNull() && !m_orientationChanged ? m_topBarStatusBarMiddle.size() * scaleFactor : QSize(185, sizes["status_field_middle.svg"].height()) * scaleFactor);

        //Update the sizeHint to match the size of the scaled m_topBarPixmap
        updateGeometry();

        //Point Update - Positions relative to the Top Bar "Backgroud" size.
        //TODO: consider some layout instead of calculating relative locations
        QSize topBarPixmapSize = m_topBarPixmap.size();
        QSize topBarUserIconSize = m_topBarUserIcon.size();
        QSize topBarUserStatusSize = m_topBarUserStatus.size();
        QSize topBarStatusBarLeftSize = m_topBarStatusBarLeft.size();
        QSize topBarStatusBarMiddleSize = m_topBarStatusBarMiddle.size();

        //Location for Title text 5% width, 35% height of the background pixmap
        m_topBarTitlePoint = QPoint(topBarPixmapSize.width()* 0.05,
                topBarPixmapSize.height() * 0.35);

        //User Icon location
        //Placing 70% of the width and 10% of the height of the top bar background
        m_topBarUserIconPoint = QPoint((topBarPixmapSize.width() * 0.7), (topBarPixmapSize.height() * 0.1));

        //If Blue theme is in use - position user status icon on the right side of the user icon
        if(!m_isLimeTheme) {
            //Place the status icon on top of the right edge of the user icon, lower it by 35% of the height of the user icon
            m_topBarUserStatusPoint = QPoint( ( (m_topBarUserIconPoint.x()+topBarUserIconSize.width() ) -
                    ( topBarUserStatusSize.width()/2 )),
                (m_topBarUserIconPoint.y() + (topBarUserIconSize.height() * 0.35 )));
        }
        //If Lime theme is in use - position user status icon on the left side of the user icon
        else {
            //Place the status icon on top of the left side of the user icon, lower it by 50% of the height of the user icon
            //and move left by 5% of the icon
            m_topBarUserStatusPoint = QPoint(  m_topBarUserIconPoint.x() + ( topBarUserIconSize.width() * 0.05),
                        (m_topBarUserIconPoint.y() + (topBarUserIconSize.height() * 0.5 )));
        }

        //Status bar
        //Placing the left side of the status bar  5% of the width, 50% of the height of the top bar background
        //Set the text baseline 80% of the height of the status bar
        m_topBarStatusBarLeftPoint = QPoint( (topBarPixmapSize.width()* 0.05),
                                                        (topBarPixmapSize.height() * 0.5));
        m_topBarStatusBarMiddlePoint = QPoint( (m_topBarStatusBarLeftPoint.x() + topBarStatusBarLeftSize.width()),
                                                            (m_topBarStatusBarLeftPoint.y()));
        m_topBarStatusBarRightPoint = QPoint( (m_topBarStatusBarMiddlePoint.x() + topBarStatusBarMiddleSize.width()),
                                                        (m_topBarStatusBarMiddlePoint.y() ) );
        m_topBarStatusBarTextPoint = QPoint(m_topBarStatusBarMiddlePoint.x(),
                                                        m_topBarStatusBarMiddlePoint.y() + (topBarStatusBarMiddleSize.height()*0.8) );
    } //if scalefactor
}

void TopBar::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
    //Topbar background
    painter->drawPixmap(option->exposedRect, m_topBarPixmap, option->exposedRect);

    //User Icon
    painter->drawPixmap(m_topBarUserIconPoint, m_topBarUserIcon);

    //User Status
    painter->drawPixmap(m_topBarUserStatusPoint, m_topBarUserStatus);

    //Status bar
    painter->drawPixmap(m_topBarStatusBarLeftPoint, m_topBarStatusBarLeft);
    painter->drawPixmap(m_topBarStatusBarMiddlePoint, m_topBarStatusBarMiddle);
    painter->drawPixmap(m_topBarStatusBarRightPoint, m_topBarStatusBarRight);

    //Title text
    painter->save();
    painter->setFont(m_titleFont);
    painter->setOpacity(0.7);
    painter->setPen(Qt::white);
    painter->drawText(m_topBarTitlePoint, QString("Contacts") );
    //Status text
    painter->setFont(m_statusFont);
    painter->setOpacity(1.0);
    painter->drawText(m_topBarStatusBarTextPoint, QString("My Status (fixed)") );
    painter->restore();
}

QRectF TopBar::boundingRect() const
{
    //It's possible that m_topBarPixmap is not allocated yet,
    //in this case default size is used for setting boundingRect
    QHash<QString, QSize>sizes = (Theme::p()->theme() == Theme::Blue) ?
        m_sizesBlue : m_sizesLime;

    if (!m_topBarPixmap.isNull())
        return QRectF(0, 0, m_topBarPixmap.size().width(), m_topBarPixmap.size().height());
    else
        return QRectF(0, 0, sizes["topbar.svg"].width(), sizes["topbar.svg"].height());
}

void TopBar::themeChange()
{
    m_titleFont = Theme::p()->font(Theme::TitleBar);
    m_statusFont = Theme::p()->font(Theme::StatusBar);

    //Calculate the scaling factor
    QHash<QString, QSize>sizes = (Theme::p()->theme() == Theme::Blue) ?
        m_sizesBlue : m_sizesLime;

    QString topbarString= m_orientation == TopBar::Portrait ?
            "topbar.svg" : "topbar_horisontal.svg";

    QSize topBarSize = sizes[topbarString];
    QSize newSize = QSize(topBarSize);

    //Scale according to aspect ratio
    newSize.scale(size().toSize(), Qt::KeepAspectRatio);

    //fix width to window widht if previous scaling produced too narrow image
    if(newSize.width() < size().width()) {
        newSize.scale(size().toSize(), Qt::KeepAspectRatioByExpanding);
    }

    //Calculate scaling factor for rest of the graphics scaling
    qreal scaleFactor = (newSize.width() *1.0) / (topBarSize.width() * 1.0);

    //Background
    m_topBarPixmap = Theme::p()->pixmap(topbarString, sizes[topbarString] * scaleFactor);

    //User Icon
    m_topBarUserIcon = Theme::p()->pixmap("user_default_icon.svg", sizes["user_default_icon.svg"] * scaleFactor);

    //User Status
    m_topBarUserStatus = Theme::p()->pixmap("user_status_online.svg", sizes["user_status_online.svg"] * scaleFactor);

    //Status Bar
    m_topBarStatusBarLeft = Theme::p()->pixmap("status_field_left.svg", sizes["status_field_left.svg"] * scaleFactor);
    m_topBarStatusBarRight = Theme::p()->pixmap("status_field_right.svg", sizes["status_field_right.svg"] * scaleFactor);
    m_topBarStatusBarMiddle = Theme::p()->pixmap("status_field_middle.svg",
            QSize(185, sizes["status_field_middle.svg"].height())* scaleFactor);

    //Update Drawing points for Top Bar elements, points are relative to the top bar background size
    QSize topBarPixmapSize = m_topBarPixmap.size();
    QSize topBarUserIconSize = m_topBarUserIcon.size();
    QSize topBarUserStatusSize = m_topBarUserStatus.size();
    QSize topBarStatusBarLeftSize = m_topBarStatusBarLeft.size();
    QSize topBarStatusBarMiddleSize = m_topBarStatusBarMiddle.size();

    //Theme Check
    (Theme::p()->theme() == Theme::Lime) ? m_isLimeTheme = true : m_isLimeTheme = false;

    //User Icon location
    //Placing 70% of the width and 10% of the height of the top bar background
    m_topBarUserIconPoint = QPoint((0.7*topBarPixmapSize.width()), (0.1*topBarPixmapSize.height()));

    //If Blue theme is in use - position user status icon on the right side of the user icon
    if(!m_isLimeTheme) {
        //Place the status icon on top of the right edge of the user icon, lower it by 35% of the height of the user icon
        m_topBarUserStatusPoint = QPoint( ( (m_topBarUserIconPoint.x()+topBarUserIconSize.width() ) - ( topBarUserStatusSize.width()/2 )),
            (m_topBarUserIconPoint.y() + (topBarUserIconSize.height() * 0.35 )));
    }
    //If Lime theme is in use - position user status icon on the left side of the user icon
    else {
        //Place the status icon on top of the left side of the user icon, lower it by 50% of the height of the user icon
        //and move left by 5% of the icon
        m_topBarUserStatusPoint = QPoint(  m_topBarUserIconPoint.x() + ( topBarUserIconSize.width() * 0.05),
                    (m_topBarUserIconPoint.y() + (topBarUserIconSize.height() * 0.5 )));
    }

    //Status bar
    //Placing the left side of the status bar  5% of the width, 50% of the height of the top bar background
    //Set the text baseline 80% of the height of the status bar
    m_topBarStatusBarLeftPoint = QPoint( (topBarPixmapSize.width()* 0.05),
                                                    (topBarPixmapSize.height() * 0.5));
    m_topBarStatusBarMiddlePoint = QPoint( (m_topBarStatusBarLeftPoint.x() + topBarStatusBarLeftSize.width()),
                                                        (m_topBarStatusBarLeftPoint.y()));
    m_topBarStatusBarRightPoint = QPoint( (m_topBarStatusBarMiddlePoint.x() + topBarStatusBarMiddleSize.width()),
                                                    (m_topBarStatusBarMiddlePoint.y() ) );
    m_topBarStatusBarTextPoint = QPoint(m_topBarStatusBarMiddlePoint.x(),
                                                    m_topBarStatusBarMiddlePoint.y() + (topBarStatusBarMiddleSize.height()*0.8) );

    update();
}

QSizeF TopBar::sizeHint(Qt::SizeHint which,
        const QSizeF &constraint) const
{
    //It's possible that m_topBarPixmap is not allocated yet,
    //in this case default size is used for setting size hint
    QHash<QString, QSize>sizes = (Theme::p()->theme() == Theme::Blue) ?
        m_sizesBlue : m_sizesLime;

    int height = !m_topBarPixmap.isNull() ?
            m_topBarPixmap.height() : sizes["topbar.svg"].height();

    switch (which)
    {
    case Qt::MinimumSize:
        return QSizeF(-1, height);

    case Qt::MaximumSize:
        return QSizeF(-1, height);

    default:
        return QGraphicsWidget::sizeHint(which, constraint);
    }
}

void TopBar::setDefaultSizes()
{
    m_sizesBlue["topbar.svg"] = QSize(356,96);
    m_sizesBlue["topbar_horisontal.svg"] = QSize(636,96);
    m_sizesBlue["user_default_icon.svg"] = QSize(68,68);
    m_sizesBlue["user_status_online.svg"] = QSize(38,38);
    m_sizesBlue["status_field_left.svg"] = QSize(14,24);
    m_sizesBlue["status_field_right.svg"] = QSize(10,24);
    m_sizesBlue["status_field_middle.svg"] = QSize(14,24);

    m_sizesLime["topbar.svg"] = QSize(356,96);
    m_sizesLime["topbar_horisontal.svg"] = QSize(636,96);
    m_sizesLime["user_default_icon.svg"] = QSize(84,68);
    m_sizesLime["user_status_online.svg"] = QSize(24,24);
    m_sizesLime["status_field_left.svg"] = QSize(14,24);
    m_sizesLime["status_field_right.svg"] = QSize(10,24);
    m_sizesLime["status_field_middle.svg"] = QSize(14,24);
}

void TopBar::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QRect rect = m_topBarStatusBarMiddle.rect();
    rect.moveTopLeft(m_topBarStatusBarMiddlePoint);
    QPointF scenePoint = event->scenePos();
    if(rect.contains(scenePoint.toPoint())) {
        emit clicked();
    }
}
