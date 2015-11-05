/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qevdevmousemanager_p.h"

#include <QStringList>
#include <QGuiApplication>
#include <QScreen>
#include <QLoggingCategory>
#include <qpa/qwindowsysteminterface.h>
#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <private/qguiapplication_p.h>
#include <private/qinputdevicemanager_p_p.h>
#include <private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEvdevMouse)

QEvdevMouseManager::QEvdevMouseManager(const QString &key, const QString &specification, QObject *parent)
    : QObject(parent), m_x(0), m_y(0), m_xoffset(0), m_yoffset(0)
{
    Q_UNUSED(key);

    QString spec = QString::fromLocal8Bit(qgetenv("QT_QPA_EVDEV_MOUSE_PARAMETERS"));

    if (spec.isEmpty())
        spec = specification;

    QStringList args = spec.split(QLatin1Char(':'));
    QStringList devices;

    foreach (const QString &arg, args) {
        if (arg.startsWith(QLatin1String("/dev/"))) {
            // if device is specified try to use it
            devices.append(arg);
            args.removeAll(arg);
        } else if (arg.startsWith(QLatin1String("xoffset="))) {
            m_xoffset = arg.mid(8).toInt();
        } else if (arg.startsWith(QLatin1String("yoffset="))) {
            m_yoffset = arg.mid(8).toInt();
        }
    }

    // build new specification without /dev/ elements
    m_spec = args.join(QLatin1Char(':'));

    // add all mice for devices specified in the argument list
    foreach (const QString &device, devices)
        addMouse(device);

    if (devices.isEmpty()) {
        qCDebug(qLcEvdevMouse) << "evdevmouse: Using device discovery";
        m_deviceDiscovery = QDeviceDiscovery::create(QDeviceDiscovery::Device_Mouse | QDeviceDiscovery::Device_Touchpad, this);
        if (m_deviceDiscovery) {
            // scan and add already connected keyboards
            QStringList devices = m_deviceDiscovery->scanConnectedDevices();
            foreach (const QString &device, devices) {
                addMouse(device);
            }

            connect(m_deviceDiscovery, SIGNAL(deviceDetected(QString)), this, SLOT(addMouse(QString)));
            connect(m_deviceDiscovery, SIGNAL(deviceRemoved(QString)), this, SLOT(removeMouse(QString)));
        }
    }

    connect(QGuiApplicationPrivate::inputDeviceManager(), SIGNAL(cursorPositionChangeRequested(QPoint)),
            this, SLOT(handleCursorPositionChange(QPoint)));
}

QEvdevMouseManager::~QEvdevMouseManager()
{
    qDeleteAll(m_mice);
    m_mice.clear();
}

void QEvdevMouseManager::clampPosition()
{
    // clamp to screen geometry
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    QRect g = QHighDpi::toNativePixels(primaryScreen->virtualGeometry(), primaryScreen);
    if (m_x + m_xoffset < g.left())
        m_x = g.left() - m_xoffset;
    else if (m_x + m_xoffset > g.right())
        m_x = g.right() - m_xoffset;

    if (m_y + m_yoffset < g.top())
        m_y = g.top() - m_yoffset;
    else if (m_y + m_yoffset > g.bottom())
        m_y = g.bottom() - m_yoffset;
}

void QEvdevMouseManager::handleMouseEvent(int x, int y, bool abs, Qt::MouseButtons buttons)
{
    // update current absolute coordinates
    if (!abs) {
        m_x += x;
        m_y += y;
    } else {
        m_x = x;
        m_y = y;
    }

    clampPosition();

    QPoint pos(m_x + m_xoffset, m_y + m_yoffset);
    // Cannot track the keyboard modifiers ourselves here. Instead, report the
    // modifiers from the last key event that has been seen by QGuiApplication.
    QWindowSystemInterface::handleMouseEvent(0, pos, pos, buttons, QGuiApplication::keyboardModifiers());
}

void QEvdevMouseManager::handleWheelEvent(int delta, Qt::Orientation orientation)
{
    QPoint pos(m_x + m_xoffset, m_y + m_yoffset);
    QWindowSystemInterface::handleWheelEvent(0, pos, pos, delta, orientation, QGuiApplication::keyboardModifiers());
}

void QEvdevMouseManager::addMouse(const QString &deviceNode)
{
    qCDebug(qLcEvdevMouse) << "Adding mouse at" << deviceNode;
    QEvdevMouseHandler *handler;
    handler = QEvdevMouseHandler::create(deviceNode, m_spec);
    if (handler) {
        connect(handler, SIGNAL(handleMouseEvent(int,int,bool,Qt::MouseButtons)), this, SLOT(handleMouseEvent(int,int,bool,Qt::MouseButtons)));
        connect(handler, SIGNAL(handleWheelEvent(int,Qt::Orientation)), this, SLOT(handleWheelEvent(int,Qt::Orientation)));
        m_mice.insert(deviceNode, handler);
        QInputDeviceManagerPrivate::get(QGuiApplicationPrivate::inputDeviceManager())->setDeviceCount(
            QInputDeviceManager::DeviceTypePointer, m_mice.count());
    } else {
        qWarning("evdevmouse: Failed to open mouse device %s", qPrintable(deviceNode));
    }
}

void QEvdevMouseManager::removeMouse(const QString &deviceNode)
{
    if (m_mice.contains(deviceNode)) {
        qCDebug(qLcEvdevMouse) << "Removing mouse at" << deviceNode;
        QEvdevMouseHandler *handler = m_mice.value(deviceNode);
        m_mice.remove(deviceNode);
        QInputDeviceManagerPrivate::get(QGuiApplicationPrivate::inputDeviceManager())->setDeviceCount(
            QInputDeviceManager::DeviceTypePointer, m_mice.count());
        delete handler;
    }
}

void QEvdevMouseManager::handleCursorPositionChange(const QPoint &pos)
{
    m_x = pos.x();
    m_y = pos.y();
    clampPosition();
}

QT_END_NAMESPACE
