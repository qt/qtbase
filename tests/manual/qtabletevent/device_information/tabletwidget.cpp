// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tabletwidget.h"
#include <QPainter>
#include <QApplication>
#include <QDebug>
#include <QMetaObject>
#include <QMetaEnum>

TabletWidget::TabletWidget(bool mouseToo) : mMouseToo(mouseToo), mWheelEventCount(0), mQuitShortcut(QKeySequence::Quit, this)
{
    QPalette newPalette = palette();
    newPalette.setColor(QPalette::Window, Qt::white);
    newPalette.setColor(QPalette::WindowText, Qt::black);
    setPalette(newPalette);
    qApp->installEventFilter(this);
    resetAttributes();
    connect(&mQuitShortcut, SIGNAL(activated()), qApp, SLOT(quit()));
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
            mDev = event->pointingDevice();
            if (!mDev)
                qWarning() << "missing device in tablet event";
            mType = event->type();
            mPos = event->position();
            mGPos = event->globalPosition();
            mXT = event->xTilt();
            mYT = event->yTilt();
            mZ = event->z();
            mPress = event->pressure();
            mTangential = event->tangentialPressure();
            mRot = event->rotation();
            mButton = event->button();
            mButtons = event->buttons();
            mModifiers = event->modifiers();
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
            mGPos = event->globalPosition();
            mTimestamp = event->timestamp();
            if (isVisible())
                update();
        }
        break;
    case QEvent::Wheel:
        ++mWheelEventCount;
        break;
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
        if (mDev.isNull()) {
            eventInfo << QString("Device info missing");
        } else {
            eventInfo << QString("Seat: %1").arg(mDev->seatName());
            eventInfo << QString("Name: %1").arg(mDev->name());
            eventInfo << QString("Device type: %1").arg(deviceTypeToString(mDev->type()));
            eventInfo << QString("Pointer type: %1").arg(pointerTypeToString(mDev->pointerType()));
            eventInfo << QString("Capabilities: %1").arg(pointerCapabilitiesToString(mDev->capabilities()));
            eventInfo << QString("Unique Id: %1").arg(QString::number(mDev->uniqueId().numericId(), 16));
        }
        eventInfo << QString("Button: %1 (0x%2)").arg(buttonToString(mButton)).arg(mButton, 0, 16);
        eventInfo << QString("Buttons currently pressed: %1 (0x%2)").arg(buttonsToString(mButtons)).arg(mButtons, 0, 16);
        eventInfo << QString("Keyboard modifiers: %1 (0x%2)").arg(modifiersToString(mModifiers)).arg(mModifiers, 0, 16);
        eventInfo << QString("Pressure: %1").arg(QString::number(mPress));
        eventInfo << QString("Tangential pressure: %1").arg(QString::number(mTangential));
        eventInfo << QString("Rotation: %1").arg(QString::number(mRot));
        eventInfo << QString("xTilt: %1").arg(QString::number(mXT));
        eventInfo << QString("yTilt: %1").arg(QString::number(mYT));
        eventInfo << QString("z: %1").arg(QString::number(mZ));


        eventInfo << QString("Total wheel events: %1").arg(QString::number(mWheelEventCount));
    }

    QString text = eventInfo.join("\n");
    painter.drawText(rect(), text);
}

const char *TabletWidget::deviceTypeToString(QInputDevice::DeviceType t)
{
    static int enumIdx = QInputDevice::staticMetaObject.indexOfEnumerator("DeviceType");
    return QPointingDevice::staticMetaObject.enumerator(enumIdx).valueToKey(int(t));
}

const char *TabletWidget::pointerTypeToString(QPointingDevice::PointerType t)
{
    static int enumIdx = QPointingDevice::staticMetaObject.indexOfEnumerator("PointerType");
    return QPointingDevice::staticMetaObject.enumerator(enumIdx).valueToKey(int(t));
}

QString TabletWidget::pointerCapabilitiesToString(QPointingDevice::Capabilities c)
{
    static int enumIdx = QPointingDevice::staticMetaObject.indexOfEnumerator("Capabilities");
    return QString::fromLatin1(QPointingDevice::staticMetaObject.enumerator(enumIdx).valueToKeys(c));
}

const char *TabletWidget::buttonToString(Qt::MouseButton b)
{
    static int enumIdx = QObject::staticMetaObject.indexOfEnumerator("MouseButtons");
    return QObject::staticMetaObject.enumerator(enumIdx).valueToKey(b);
}

QString TabletWidget::buttonsToString(Qt::MouseButtons bs)
{
    QStringList ret;
    for (int i = 0; (uint)(1 << i) <= Qt::MaxMouseButton; ++i) {
        Qt::MouseButton b = static_cast<Qt::MouseButton>(1 << i);
        if (bs.testFlag(b))
            ret << buttonToString(b);
    }
    return ret.join(QLatin1Char('|'));
}

QString TabletWidget::modifiersToString(Qt::KeyboardModifiers m)
{
    QStringList ret;
    if (m & Qt::ShiftModifier)
        ret << QLatin1String("Shift");
    if (m & Qt::ControlModifier)
        ret << QLatin1String("Control");
    if (m & Qt::AltModifier)
        ret << QLatin1String("Alt");
    if (m & Qt::MetaModifier)
        ret << QLatin1String("Meta");
    if (m & Qt::KeypadModifier)
        ret << QLatin1String("Keypad");
    if (m & Qt::GroupSwitchModifier)
        ret << QLatin1String("GroupSwitch");
    return ret.join(QLatin1Char('|'));
}

void TabletWidget::tabletEvent(QTabletEvent *event)
{
    event->accept();
}

