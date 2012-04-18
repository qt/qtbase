/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QUDEVICEHELPER_H
#define QUDEVICEHELPER_H

#include <QObject>
#include <QSocketNotifier>

#include <libudev.h>

QT_BEGIN_NAMESPACE

class QUDeviceHelper : public QObject
{
    Q_OBJECT
    Q_ENUMS(QUDeviceType)

public:
    enum QUDeviceType {
        UDev_Unknown = 0x00,
        UDev_Mouse = 0x01,
        UDev_Touchpad = 0x02,
        UDev_Touchscreen = 0x04,
        UDev_Keyboard = 0x08,
        UDev_DRM = 0x10,
        UDev_InputMask = UDev_Mouse | UDev_Touchpad | UDev_Touchscreen | UDev_Keyboard,
        UDev_VideoMask = UDev_DRM
    };
    Q_DECLARE_FLAGS(QUDeviceTypes, QUDeviceType)

    static QUDeviceHelper *createUDeviceHelper(QUDeviceTypes type, QObject *parent);
    ~QUDeviceHelper();

    QStringList scanConnectedDevices();

signals:
    void deviceDetected(const QString &deviceNode, QUDeviceTypes types);
    void deviceRemoved(const QString &deviceNode, QUDeviceTypes types);

private slots:
    void handleUDevNotification();

private:
    QUDeviceHelper(QUDeviceTypes types, struct udev *udev, QObject *parent = 0);

    void startWatching();
    void stopWatching();

    QUDeviceTypes checkDeviceType(struct udev_device *dev);

    struct udev *m_udev;
    QUDeviceTypes m_types;
    struct udev_monitor *m_udevMonitor;
    int m_udevMonitorFileDescriptor;
    QSocketNotifier *m_udevSocketNotifier;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QUDeviceHelper::QUDeviceTypes)

QT_END_NAMESPACE

#endif // QUDEVICEHELPER_H
