/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qevdevmousehandler_p.h"

#include <QSocketNotifier>
#include <QStringList>
#include <QPoint>
#include <QGuiApplication>
#include <QScreen>
#include <QLoggingCategory>
#include <qpa/qwindowsysteminterface.h>

#include <qplatformdefs.h>
#include <private/qcore_unix_p.h> // overrides QT_OPEN
#include <private/qhighdpiscaling_p.h>

#include <errno.h>

#ifdef Q_OS_FREEBSD
#include <dev/evdev/input.h>
#else
#include <linux/kd.h>
#include <linux/input.h>
#endif

#define TEST_BIT(array, bit)    (array[bit/8] & (1<<(bit%8)))

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEvdevMouse, "qt.qpa.input")

std::unique_ptr<QEvdevMouseHandler> QEvdevMouseHandler::create(const QString &device, const QString &specification)
{
    qCDebug(qLcEvdevMouse) << "create mouse handler for" << device << specification;

    bool compression = true;
    int jitterLimit = 0;
    int grab = 0;
    bool abs = false;

    const auto args = specification.splitRef(QLatin1Char(':'));
    for (const QStringRef &arg : args) {
        if (arg == QLatin1String("nocompress"))
            compression = false;
        else if (arg.startsWith(QLatin1String("dejitter=")))
            jitterLimit = arg.mid(9).toInt();
        else if (arg.startsWith(QLatin1String("grab=")))
            grab = arg.mid(5).toInt();
        else if (arg == QLatin1String("abs"))
            abs = true;
    }

    int fd;
    fd = qt_safe_open(device.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);
    if (fd >= 0) {
        ::ioctl(fd, EVIOCGRAB, grab);
        return std::unique_ptr<QEvdevMouseHandler>(new QEvdevMouseHandler(device, fd, abs, compression, jitterLimit));
    } else {
        qErrnoWarning(errno, "Cannot open mouse input device %s", qPrintable(device));
        return nullptr;
    }
}

QEvdevMouseHandler::QEvdevMouseHandler(const QString &device, int fd, bool abs, bool compression, int jitterLimit)
    : m_device(device), m_fd(fd), m_abs(abs), m_compression(compression)
{
    setObjectName(QLatin1String("Evdev Mouse Handler"));

    m_jitterLimitSquared = jitterLimit * jitterLimit;

    // Some touch screens present as mice with absolute coordinates.
    // These can not be differentiated from touchpads, so supplying abs to QT_QPA_EVDEV_MOUSE_PARAMETERS
    // will force qevdevmousehandler to treat the coordinates as absolute, scaled to the hardware maximums.
    // Turning this on will not affect mice as these do not report in absolute coordinates
    // but will make touchpads act like touch screens
    if (m_abs)
        m_abs = getHardwareMaximum();

    detectHiResWheelSupport();

    // socket notifier for events on the mouse device
    m_notify = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notify, &QSocketNotifier::activated,
            this, &QEvdevMouseHandler::readMouseData);
}

QEvdevMouseHandler::~QEvdevMouseHandler()
{
    if (m_fd >= 0)
        qt_safe_close(m_fd);
}

void QEvdevMouseHandler::detectHiResWheelSupport()
{
#if defined(REL_WHEEL_HI_RES) || defined(REL_HWHEEL_HI_RES)
    // Check if we can expect hires events as we will get both
    // legacy and hires event and needs to know if we should
    // ignore the legacy events.
    unsigned char relFeatures[(REL_MAX / 8) + 1]{};
    if (ioctl(m_fd, EVIOCGBIT(EV_REL, sizeof (relFeatures)), relFeatures) == -1)
        return;

#if defined(REL_WHEEL_HI_RES)
    m_hiResWheel = TEST_BIT(relFeatures, REL_WHEEL_HI_RES);
#endif
#if defined(REL_HWHEEL_HI_RES)
    m_hiResHWheel = TEST_BIT(relFeatures, REL_HWHEEL_HI_RES);
#endif
#endif
}

// Ask touch screen hardware for information on coordinate maximums
// If any ioctls fail, revert to non abs mode
bool QEvdevMouseHandler::getHardwareMaximum()
{
    unsigned char absFeatures[(ABS_MAX / 8) + 1];
    memset(absFeatures, '\0', sizeof (absFeatures));

    // test if ABS_X, ABS_Y are available
    if (ioctl(m_fd, EVIOCGBIT(EV_ABS, sizeof (absFeatures)), absFeatures) == -1)
        return false;

    if ((!TEST_BIT(absFeatures, ABS_X)) || (!TEST_BIT(absFeatures, ABS_Y)))
        return false;

    // ask hardware for minimum and maximum values
    struct input_absinfo absInfo;
    if (ioctl(m_fd, EVIOCGABS(ABS_X), &absInfo) == -1)
        return false;

    m_hardwareWidth = absInfo.maximum - absInfo.minimum;

    if (ioctl(m_fd, EVIOCGABS(ABS_Y), &absInfo) == -1)
        return false;

    m_hardwareHeight = absInfo.maximum - absInfo.minimum;

    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    QRect g = QHighDpi::toNativePixels(primaryScreen->virtualGeometry(), primaryScreen);
    m_hardwareScalerX = static_cast<qreal>(m_hardwareWidth) / (g.right() - g.left());
    m_hardwareScalerY = static_cast<qreal>(m_hardwareHeight) / (g.bottom() - g.top());

    qCDebug(qLcEvdevMouse) << "Absolute pointing device"
                           << "hardware max x" << m_hardwareWidth
                           << "hardware max y" << m_hardwareHeight
                           << "hardware scalers x" << m_hardwareScalerX << 'y' << m_hardwareScalerY;

    return true;
}

void QEvdevMouseHandler::sendMouseEvent()
{
    int x;
    int y;

    if (!m_abs) {
        x = m_x - m_prevx;
        y = m_y - m_prevy;
    }
    else {
        x = m_x / m_hardwareScalerX;
        y = m_y / m_hardwareScalerY;
    }

    if (m_prevInvalid) {
        x = y = 0;
        m_prevInvalid = false;
    }

    emit handleMouseEvent(x, y, m_abs, m_buttons, m_button, m_eventType);

    m_prevx = m_x;
    m_prevy = m_y;
}

void QEvdevMouseHandler::readMouseData()
{
    struct ::input_event buffer[32];
    int n = 0;
    bool posChanged = false, btnChanged = false;
    bool pendingMouseEvent = false;
    int eventCompressCount = 0;
    forever {
        int result = QT_READ(m_fd, reinterpret_cast<char *>(buffer) + n, sizeof(buffer) - n);

        if (result == 0) {
            qWarning("evdevmouse: Got EOF from the input device");
            return;
        } else if (result < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                qErrnoWarning(errno, "evdevmouse: Could not read from input device");
                // If the device got disconnected, stop reading, otherwise we get flooded
                // by the above error over and over again.
                if (errno == ENODEV) {
                    delete m_notify;
                    m_notify = nullptr;
                    qt_safe_close(m_fd);
                    m_fd = -1;
                }
                return;
            }
        } else {
            n += result;
            if (n % sizeof(buffer[0]) == 0)
                break;
        }
    }

    n /= sizeof(buffer[0]);

    for (int i = 0; i < n; ++i) {
        struct ::input_event *data = &buffer[i];
        if (data->type == EV_ABS) {
            // Touchpads: store the absolute position for now, will calculate a relative one later.
            if (data->code == ABS_X && m_x != data->value) {
                m_x = data->value;
                posChanged = true;
            } else if (data->code == ABS_Y && m_y != data->value) {
                m_y = data->value;
                posChanged = true;
            }
        } else if (data->type == EV_REL) {
            QPoint delta;
            if (data->code == REL_X) {
                m_x += data->value;
                posChanged = true;
            } else if (data->code == REL_Y) {
                m_y += data->value;
                posChanged = true;
            } else if (!m_hiResWheel && data->code == REL_WHEEL) {
                // data->value: positive == up, negative == down
                delta.setY(120 * data->value);
                emit handleWheelEvent(delta);
#ifdef REL_WHEEL_HI_RES
            } else if (data->code == REL_WHEEL_HI_RES) {
                delta.setY(data->value);
                emit handleWheelEvent(delta);
#endif
            } else if (!m_hiResHWheel && data->code == REL_HWHEEL) {
                // data->value: positive == right, negative == left
                delta.setX(-120 * data->value);
                emit handleWheelEvent(delta);
#ifdef REL_HWHEEL_HI_RES
            } else if (data->code == REL_HWHEEL_HI_RES) {
                delta.setX(-data->value);
                emit handleWheelEvent(delta);
#endif
            }
        } else if (data->type == EV_KEY && data->code == BTN_TOUCH) {
            // We care about touchpads only, not touchscreens -> don't map to button press.
            // Need to invalidate prevx/y however to get proper relative pos.
            m_prevInvalid = true;
        } else if (data->type == EV_KEY && data->code >= BTN_LEFT && data->code <= BTN_JOYSTICK) {
            Qt::MouseButton button = Qt::NoButton;
            // BTN_LEFT == 0x110 in kernel's input.h
            // The range of possible mouse buttons ends just before BTN_JOYSTICK, value 0x120.
            switch (data->code) {
            case 0x110: button = Qt::LeftButton; break;    // BTN_LEFT
            case 0x111: button = Qt::RightButton; break;
            case 0x112: button = Qt::MiddleButton; break;
            case 0x113: button = Qt::ExtraButton1; break;  // AKA Qt::BackButton
            case 0x114: button = Qt::ExtraButton2; break;  // AKA Qt::ForwardButton
            case 0x115: button = Qt::ExtraButton3; break;  // AKA Qt::TaskButton
            case 0x116: button = Qt::ExtraButton4; break;
            case 0x117: button = Qt::ExtraButton5; break;
            case 0x118: button = Qt::ExtraButton6; break;
            case 0x119: button = Qt::ExtraButton7; break;
            case 0x11a: button = Qt::ExtraButton8; break;
            case 0x11b: button = Qt::ExtraButton9; break;
            case 0x11c: button = Qt::ExtraButton10; break;
            case 0x11d: button = Qt::ExtraButton11; break;
            case 0x11e: button = Qt::ExtraButton12; break;
            case 0x11f: button = Qt::ExtraButton13; break;
            }
            m_buttons.setFlag(button, data->value);
            m_button = button;
            m_eventType = data->value == 0 ? QEvent::MouseButtonRelease : QEvent::MouseButtonPress;
            btnChanged = true;
        } else if (data->type == EV_SYN && data->code == SYN_REPORT) {
            if (btnChanged) {
                btnChanged = posChanged = false;
                sendMouseEvent();
                pendingMouseEvent = false;
            } else if (posChanged) {
                m_eventType = QEvent::MouseMove;
                posChanged = false;
                if (m_compression) {
                    pendingMouseEvent = true;
                    eventCompressCount++;
                } else {
                    sendMouseEvent();
                }
            }
        } else if (data->type == EV_MSC && data->code == MSC_SCAN) {
            // kernel encountered an unmapped key - just ignore it
            continue;
        }
    }
    if (m_compression && pendingMouseEvent) {
        int distanceSquared = (m_x - m_prevx)*(m_x - m_prevx) + (m_y - m_prevy)*(m_y - m_prevy);
        if (distanceSquared > m_jitterLimitSquared)
            sendMouseEvent();
    }
}

QT_END_NAMESPACE
