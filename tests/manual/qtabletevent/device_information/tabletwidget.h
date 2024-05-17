// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TABLETWIDGET_H
#define TABLETWIDGET_H

#include <QWidget>
#include <QTabletEvent>
#include <QPointer>
#include <QPointingDevice>
#include <QShortcut>

// a widget showing the information of the last tablet event
class TabletWidget : public QWidget
{
public:
    TabletWidget(bool mouseToo);
protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    void tabletEvent(QTabletEvent *event);
    void paintEvent(QPaintEvent *event);
    const char *deviceTypeToString(QInputDevice::DeviceType t);
    const char *pointerTypeToString(QPointingDevice::PointerType t);
    QString pointerCapabilitiesToString(QPointingDevice::Capabilities c);
    const char *buttonToString(Qt::MouseButton b);
    QString buttonsToString(Qt::MouseButtons bs);
    QString modifiersToString(Qt::KeyboardModifiers m);
private:
    void resetAttributes() {
        mDev.clear();
        mType = mXT = mYT = mZ = 0;
        mPress = mTangential = mRot = 0.0;
        mPos = mGPos = QPoint();
    }
    QPointer<const QPointingDevice> mDev;
    int mType;
    QPointF mPos, mGPos;
    int mXT, mYT, mZ;
    Qt::MouseButton mButton;
    Qt::MouseButtons mButtons;
    Qt::KeyboardModifiers mModifiers;
    qreal mPress, mTangential, mRot;
    bool mMouseToo;
    ulong mTimestamp;
    int mWheelEventCount;
    QShortcut mQuitShortcut;
};

#endif // TABLETWIDGET_H
