/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
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

#include "tabletwidget.h"
#include <QPainter>
#include <QApplication>
#include <QDebug>
#include <QMetaObject>
#include <QMetaEnum>

TabletWidget::TabletWidget(bool mouseToo) : mMouseToo(mouseToo)
{
    QPalette newPalette = palette();
    newPalette.setColor(QPalette::Window, Qt::white);
    newPalette.setColor(QPalette::WindowText, Qt::black);
    setPalette(newPalette);
    qApp->installEventFilter(this);
    resetAttributes();
}

bool TabletWidget::eventFilter(QObject *, QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::TabletEnterProximity:
    case QEvent::TabletLeaveProximity:
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
        {
            QTabletEvent *event = static_cast<QTabletEvent*>(ev);
            mType = event->type();
            mPos = event->pos();
            mGPos = event->globalPos();
            mHiResGlobalPos = event->hiResGlobalPos();
            mDev = event->device();
            mPointerType = event->pointerType();
            mUnique = event->uniqueId();
            mXT = event->xTilt();
            mYT = event->yTilt();
            mZ = event->z();
            mPress = event->pressure();
            mTangential = event->tangentialPressure();
            mRot = event->rotation();
            mButton = event->button();
            mButtons = event->buttons();
            mTimestamp = event->timestamp();
            if (isVisible())
                update();
            break;
        }
    case QEvent::MouseMove:
        if (mMouseToo) {
            resetAttributes();
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            mType = event->type();
            mPos = event->pos();
            mGPos = event->globalPos();
            mTimestamp = event->timestamp();
        }
    default:
        break;
    }
    return false;
}

void TabletWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QStringList eventInfo;

    QString typeString("Event type: ");
    switch (mType) {
    case QEvent::TabletEnterProximity:
        typeString += "QEvent::TabletEnterProximity";
        break;
    case QEvent::TabletLeaveProximity:
        typeString += "QEvent::TabletLeaveProximity";
        break;
    case QEvent::TabletMove:
        typeString += "QEvent::TabletMove";
        break;
    case QEvent::TabletPress:
        typeString += "QEvent::TabletPress";
        break;
    case QEvent::TabletRelease:
        typeString += "QEvent::TabletRelease";
        break;
    case QEvent::MouseMove:
        typeString += "QEvent::MouseMove";
        break;
    }
    eventInfo << typeString;

    eventInfo << QString("Global position: %1 %2").arg(QString::number(mGPos.x()), QString::number(mGPos.y()));
    eventInfo << QString("Local position: %1 %2").arg(QString::number(mPos.x()), QString::number(mPos.y()));
    eventInfo << QString("Timestamp: %1").arg(QString::number(mTimestamp));
    if (mType == QEvent::TabletEnterProximity || mType == QEvent::TabletLeaveProximity
        || mType == QEvent::TabletMove || mType == QEvent::TabletPress
        || mType == QEvent::TabletRelease) {

        eventInfo << QString("High res global position: %1 %2").arg(QString::number(mHiResGlobalPos.x()), QString::number(mHiResGlobalPos.y()));

        QString pointerType("Pointer type: ");
        switch (mPointerType) {
        case QTabletEvent::UnknownPointer:
            pointerType += "QTabletEvent::UnknownPointer";
            break;
        case QTabletEvent::Pen:
            pointerType += "QTabletEvent::Pen";
            break;
        case QTabletEvent::Cursor:
            pointerType += "QTabletEvent::Cursor";
            break;
        case QTabletEvent::Eraser:
            pointerType += "QTabletEvent::Eraser";
            break;
        }
        eventInfo << pointerType;

        QString deviceString = "Device type: ";
        switch (mDev) {
        case QTabletEvent::NoDevice:
            deviceString += "QTabletEvent::NoDevice";
            break;
        case QTabletEvent::Puck:
            deviceString += "QTabletEvent::Puck";
            break;
        case QTabletEvent::Stylus:
            deviceString += "QTabletEvent::Stylus";
            break;
        case QTabletEvent::Airbrush:
            deviceString += "QTabletEvent::Airbrush";
            break;
        case QTabletEvent::FourDMouse:
            deviceString += "QTabletEvent::FourDMouse";
            break;
        case QTabletEvent::RotationStylus:
            deviceString += "QTabletEvent::RotationStylus";
            break;
        }
        eventInfo << deviceString;

        eventInfo << QString("Button: %1 (0x%2)").arg(buttonToString(mButton)).arg(mButton, 0, 16);
        eventInfo << QString("Buttons currently pressed: %1 (0x%2)").arg(buttonsToString(mButtons)).arg(mButtons, 0, 16);
        eventInfo << QString("Pressure: %1").arg(QString::number(mPress));
        eventInfo << QString("Tangential pressure: %1").arg(QString::number(mTangential));
        eventInfo << QString("Rotation: %1").arg(QString::number(mRot));
        eventInfo << QString("xTilt: %1").arg(QString::number(mXT));
        eventInfo << QString("yTilt: %1").arg(QString::number(mYT));
        eventInfo << QString("z: %1").arg(QString::number(mZ));

        eventInfo << QString("Unique Id: %1").arg(QString::number(mUnique));
    }

    QString text = eventInfo.join("\n");
    painter.drawText(rect(), text);
}

const char *TabletWidget::buttonToString(Qt::MouseButton b)
{
    static int enumIdx = QObject::staticQtMetaObject.indexOfEnumerator("MouseButtons");
    return QObject::staticQtMetaObject.enumerator(enumIdx).valueToKey(b);
}

QString TabletWidget::buttonsToString(Qt::MouseButtons bs)
{
    QStringList ret;
    for (int i = 0; (uint)(1 << i) <= Qt::MaxMouseButton; ++i) {
        Qt::MouseButton b = static_cast<Qt::MouseButton>(1 << i);
        if (bs.testFlag(b))
            ret << buttonToString(b);
    }
    return ret.join("|");
}

void TabletWidget::tabletEvent(QTabletEvent *event)
{
    event->accept();
}

