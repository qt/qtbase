/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdevicediscovery_static_p.h"

#include <QStringList>
#include <QCoreApplication>
#include <QObject>
#include <QHash>
#include <QDir>
#include <QLoggingCategory>
#include <QtCore/private/qcore_unix_p.h>

#include <linux/input.h>
#include <fcntl.h>

/* android (and perhaps some other linux-derived stuff) don't define everything
 * in linux/input.h, so we'll need to do that ourselves.
 */
#ifndef KEY_CNT
#define KEY_CNT                 (KEY_MAX+1)
#endif
#ifndef REL_CNT
#define REL_CNT                 (REL_MAX+1)
#endif
#ifndef ABS_CNT
#define ABS_CNT                 (ABS_MAX+1)
#endif
#ifndef ABS_MT_POSITION_X
#define ABS_MT_POSITION_X       0x35
#endif
#ifndef ABS_MT_POSITION_Y
#define ABS_MT_POSITION_Y       0x36
#endif

#define LONG_BITS (sizeof(long) * 8 )
#define LONG_FIELD_SIZE(bits) ((bits / LONG_BITS) + 1)

static bool testBit(long bit, const long *field)
{
    return (field[bit / LONG_BITS] >> bit % LONG_BITS) & 1;
}

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcDD, "qt.qpa.input")

QDeviceDiscovery *QDeviceDiscovery::create(QDeviceTypes types, QObject *parent)
{
    return new QDeviceDiscoveryStatic(types, parent);
}

QDeviceDiscoveryStatic::QDeviceDiscoveryStatic(QDeviceTypes types, QObject *parent)
    : QDeviceDiscovery(types, parent)
{
    qCDebug(lcDD) << "static device discovery for type" << types;
}

QStringList QDeviceDiscoveryStatic::scanConnectedDevices()
{
    QStringList devices;
    QDir dir;
    dir.setFilter(QDir::System);

    // check for input devices
    if (m_types & Device_InputMask) {
        dir.setPath(QString::fromLatin1(QT_EVDEV_DEVICE_PATH));
        const auto deviceFiles = dir.entryList();
        for (const QString &deviceFile : deviceFiles) {
            QString absoluteFilePath = dir.absolutePath() + QLatin1Char('/') + deviceFile;
            if (checkDeviceType(absoluteFilePath))
                devices << absoluteFilePath;
        }
    }

    // check for drm devices
    if (m_types & Device_VideoMask) {
        dir.setPath(QString::fromLatin1(QT_DRM_DEVICE_PATH));
        const auto deviceFiles = dir.entryList();
        for (const QString &deviceFile : deviceFiles) {
            QString absoluteFilePath = dir.absolutePath() + QLatin1Char('/') + deviceFile;
            if (checkDeviceType(absoluteFilePath))
                devices << absoluteFilePath;
        }
    }

    qCDebug(lcDD) << "Found matching devices" << devices;

    return devices;
}

bool QDeviceDiscoveryStatic::checkDeviceType(const QString &device)
{
    int fd = QT_OPEN(device.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);
    if (Q_UNLIKELY(fd == -1)) {
        qWarning() << "Device discovery cannot open device" << device;
        return false;
    }

    qCDebug(lcDD) << "doing static device discovery for " << device;

    if ((m_types & Device_DRM) && device.contains(QLatin1String(QT_DRM_DEVICE_PREFIX))) {
        QT_CLOSE(fd);
        return true;
    }

    long bitsAbs[LONG_FIELD_SIZE(ABS_CNT)];
    long bitsKey[LONG_FIELD_SIZE(KEY_CNT)];
    long bitsRel[LONG_FIELD_SIZE(REL_CNT)];
    memset(bitsAbs, 0, sizeof(bitsAbs));
    memset(bitsKey, 0, sizeof(bitsKey));
    memset(bitsRel, 0, sizeof(bitsRel));

    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(bitsAbs)), bitsAbs);
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bitsKey)), bitsKey);
    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(bitsRel)), bitsRel);

    QT_CLOSE(fd);

    if ((m_types & Device_Keyboard)) {
        if (testBit(KEY_Q, bitsKey)) {
            qCDebug(lcDD) << "Found keyboard at" << device;
            return true;
        }
    }

    if ((m_types & Device_Mouse)) {
        if (testBit(REL_X, bitsRel) && testBit(REL_Y, bitsRel) && testBit(BTN_MOUSE, bitsKey)) {
            qCDebug(lcDD) << "Found mouse at" << device;
            return true;
        }
    }

    if ((m_types & (Device_Touchpad | Device_Touchscreen))) {
        if (testBit(ABS_X, bitsAbs) && testBit(ABS_Y, bitsAbs)) {
            if ((m_types & Device_Touchpad) && testBit(BTN_TOOL_FINGER, bitsKey)) {
                qCDebug(lcDD) << "Found touchpad at" << device;
                return true;
            } else if ((m_types & Device_Touchscreen) && testBit(BTN_TOUCH, bitsKey)) {
                qCDebug(lcDD) << "Found touchscreen at" << device;
                return true;
            } else if ((m_types & Device_Tablet) && (testBit(BTN_STYLUS, bitsKey) || testBit(BTN_TOOL_PEN, bitsKey))) {
                qCDebug(lcDD) << "Found tablet at" << device;
                return true;
            }
        } else if (testBit(ABS_MT_POSITION_X, bitsAbs) &&
                   testBit(ABS_MT_POSITION_Y, bitsAbs)) {
            qCDebug(lcDD) << "Found new-style touchscreen at" << device;
            return true;
        }
    }

    if ((m_types & Device_Joystick)) {
        if (testBit(BTN_A, bitsKey) || testBit(BTN_TRIGGER, bitsKey) || testBit(ABS_RX, bitsAbs)) {
            qCDebug(lcDD) << "Found joystick/gamepad at" << device;
            return true;
        }
    }

    return false;
}

QT_END_NAMESPACE
