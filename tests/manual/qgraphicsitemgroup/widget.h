/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WIDGET_H
#define WIDGET_H

#include "customitem.h"

#include <QWidget>
#include <QGraphicsItemGroup>
#include <QPainter>

namespace Ui {
    class Widget;
}

class QGraphicsOpacityEffect;
class QPropertyAnimation;

class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget(QWidget *parent = 0);
    ~Widget();

protected Q_SLOTS:
    void on_rotate_valueChanged(int value);
    void on_scale_valueChanged(int value);
    void on_rotateItem_valueChanged(int value);
    void on_scaleItem_valueChanged(int value);
    void on_group_clicked();
    void on_dismantle_clicked();
    void on_merge_clicked();
    void onSceneSelectionChanged();
    void on_ungroup_clicked();
    void on_buttonGroup_buttonClicked();

private:
    void updateUngroupButton();
    CustomItem * checkedItem() const;

    Ui::Widget *ui;
    CustomScene *scene;
    CustomItem *rectBlue;
    CustomItem *rectRed;
    CustomItem *rectGreen;
    CustomItem *rectYellow;
    QGraphicsOpacityEffect* effect;
    QPropertyAnimation *fadeIn;
    QPropertyAnimation *fadeOut;
    int previousSelectionCount;
};

#endif // WIDGET_H
