/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
