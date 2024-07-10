// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdevicediscovery_static_p.h"

#include <QStringList>
#include <QCoreApplication>
#include <QObject>
#include <QHash>
#include <QDir>
#include <QLoggingCategory>
#include <QtCore/private/qcore_unix_p.h>

#ifdef Q_OS_FREEBSD
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif
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

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcDD, "qt.qpa.input")

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

    auto addDevices = [this, &devices](const char *path) {
        for (const auto &entry : QDirListing(QString::fromLatin1(path))) {
            QString absoluteFilePath = entry.absoluteFilePath();
            if (checkDeviceType(absoluteFilePath))
                devices.emplace_back(std::move(absoluteFilePath));
        }
    };

    // check for input devices
    if (m_types & Device_InputMask)
        addDevices(QT_EVDEV_DEVICE_PATH);

    // check for drm devices
    if (m_types & Device_VideoMask)
        addDevices(QT_DRM_DEVICE_PATH);

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

    if ((m_types & Device_DRM) && device.contains(QT_DRM_DEVICE_PREFIX ""_L1)) {
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
