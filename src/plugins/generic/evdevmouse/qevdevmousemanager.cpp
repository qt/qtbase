/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qevdevmousemanager.h"

#include <QStringList>
#include <QGuiApplication>
#include <QScreen>
#include <QWindowSystemInterface>

#define QT_QPA_MOUSEMANAGER_DEBUG

#ifdef QT_QPA_MOUSEMANAGER_DEBUG
#include <QDebug>
#endif

QT_BEGIN_NAMESPACE

QEvdevMouseManager::QEvdevMouseManager(const QString &key, const QString &specification)
    : m_x(0), m_y(0), m_xoffset(0), m_yoffset(0)
{
    Q_UNUSED(key);

#ifndef QT_NO_LIBUDEV
    bool useUDev = true;
#else
    bool useUDev = false;
#endif // QT_NO_LIBUDEV
    QStringList args = specification.split(QLatin1Char(':'));
    QStringList devices;

    foreach (const QString &arg, args) {
        if (arg.startsWith("/dev/")) {
            // if device is specified try to use it
            devices.append(arg);
            args.removeAll(arg);
            useUDev = false;
        } else if (arg.startsWith("xoffset=")) {
            m_xoffset = arg.mid(8).toInt();
        } else if (arg.startsWith("yoffset=")) {
            m_yoffset = arg.mid(8).toInt();
        }
    }

    // build new specification without /dev/ elements
    m_spec = args.join(":");

    // add all mice for devices specified in the argument list
    foreach (const QString &device, devices)
        addMouse(device);

#ifdef QT_NO_LIBUDEV
    Q_UNUSED(useUDev)
#else
    if (useUDev) {
#ifdef QT_QPA_MOUSEMANAGER_DEBUG
        qWarning() << "Use UDev for device discovery";
#endif

        m_udeviceHelper = QUDeviceHelper::createUDeviceHelper(QUDeviceHelper::UDev_Mouse | QUDeviceHelper::UDev_Touchpad, this);
        if (m_udeviceHelper) {
            // scan and add already connected keyboards
            QStringList devices = m_udeviceHelper->scanConnectedDevices();
            foreach (QString device, devices) {
                addMouse(device);
            }

            connect(m_udeviceHelper, SIGNAL(deviceDetected(QString,QUDeviceTypes)), this, SLOT(addMouse(QString)));
            connect(m_udeviceHelper, SIGNAL(deviceRemoved(QString,QUDeviceTypes)), this, SLOT(removeMouse(QString)));
        }
    }
#endif // QT_NO_LIBUDEV
}

QEvdevMouseManager::~QEvdevMouseManager()
{
    qDeleteAll(m_mice);
    m_mice.clear();
}

void QEvdevMouseManager::handleMouseEvent(int x, int y, Qt::MouseButtons buttons)
{
    // update current absolute coordinates
    m_x += x;
    m_y += y;

    // clamp to screen geometry
    QRect g = QGuiApplication::primaryScreen()->virtualGeometry();
    if (m_x + m_xoffset < g.left())
        m_x = g.left() - m_xoffset;
    else if (m_x + m_xoffset > g.right())
        m_x = g.right() - m_xoffset;

    if (m_y + m_yoffset < g.top())
        m_y = g.top() - m_yoffset;
    else if (m_y + m_yoffset > g.bottom())
        m_y = g.bottom() - m_yoffset;

    QPoint pos(m_x + m_xoffset, m_y + m_yoffset);
    QWindowSystemInterface::handleMouseEvent(0, pos, pos, buttons);

#ifdef QT_QPA_MOUSEMANAGER_DEBUG
    qDebug("mouse event %d %d %d", pos.x(), pos.y(), int(buttons));
#endif
}

void QEvdevMouseManager::addMouse(const QString &deviceNode)
{
#ifdef QT_QPA_MOUSEMANAGER_DEBUG
    qWarning() << "Adding mouse at" << deviceNode;
#endif

    QString specification = m_spec;

    if (!deviceNode.isEmpty()) {
        specification.append(":");
        specification.append(deviceNode);
    }

    QEvdevMouseHandler *handler;
    handler = QEvdevMouseHandler::createLinuxInputMouseHandler("EvdevMouse", specification);
    if (handler) {
        connect(handler, SIGNAL(handleMouseEvent(int, int, Qt::MouseButtons)), this, SLOT(handleMouseEvent(int, int, Qt::MouseButtons)));
        m_mice.insert(deviceNode, handler);
    } else {
        qWarning("Failed to open mouse");
    }
}

void QEvdevMouseManager::removeMouse(const QString &deviceNode)
{
    if (m_mice.contains(deviceNode)) {
#ifdef QT_QPA_MOUSEMANAGER_DEBUG
        qWarning() << "Removing mouse at" << deviceNode;
#endif
        QEvdevMouseHandler *handler = m_mice.value(deviceNode);
        m_mice.remove(deviceNode);
        delete handler;
    }
}

QT_END_NAMESPACE
