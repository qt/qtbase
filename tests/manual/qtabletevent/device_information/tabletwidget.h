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

#ifndef TABLETWIDGET_H
#define TABLETWIDGET_H

#include <QWidget>
#include <QTabletEvent>

// a widget showing the information of the last tablet event
class TabletWidget : public QWidget
{
public:
    TabletWidget(bool mouseToo);
protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    void tabletEvent(QTabletEvent *event);
    void paintEvent(QPaintEvent *event);
    const char *buttonToString(Qt::MouseButton b);
    QString buttonsToString(Qt::MouseButtons bs);
private:
    void resetAttributes() {
        mType = mDev = mPointerType = mXT = mYT = mZ = 0;
        mPress = mTangential = mRot = 0.0;
        mPos = mGPos = QPoint();
        mHiResGlobalPos = QPointF();
        mUnique = 0;
    }
    int mType;
    QPoint mPos, mGPos;
    QPointF mHiResGlobalPos;
    int mDev, mPointerType, mXT, mYT, mZ;
    Qt::MouseButton mButton;
    Qt::MouseButtons mButtons;
    qreal mPress, mTangential, mRot;
    qint64 mUnique;
    bool mMouseToo;
    ulong mTimestamp;
    int mWheelEventCount;
};

#endif // TABLETWIDGET_H
