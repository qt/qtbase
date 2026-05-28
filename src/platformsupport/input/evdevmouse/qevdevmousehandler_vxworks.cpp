// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qevdevmousehandler_p.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QPoint>
#include <QScreen>
#include <QSocketNotifier>
#include <QStringList>
#include <private/qcore_unix_p.h> // overrides QT_OPEN
#include <qpa/qwindowsysteminterface.h>
#include <qplatformdefs.h>

#include <errno.h>
#include <evdevLib.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEvdevMouse, "qt.qpa.input")

std::unique_ptr<QEvdevMouseHandler> QEvdevMouseHandler::create(const QString &device, const QString &specification)
{
    qCDebug(qLcEvdevMouse) << "create mouse handler for" << device << specification;

    bool compression = true;
    int jitterLimit = 0;
    bool abs = false;

    QStringList args = specification.split(QLatin1Char(':'));
    for (const auto& arg : args) {
        if (arg == QLatin1String("abs"))
            abs = true;
    }

    int fd = qt_safe_open(device.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK, 0);
    if (fd >= 0) {
        return std::unique_ptr<QEvdevMouseHandler>(new QEvdevMouseHandler(device, fd, abs, compression, jitterLimit));
    } else {
        qCWarning(qLcEvdevMouse) << "Cannot open mouse input device" << qPrintable(device) << ":" << strerror(errno);
        return nullptr;
    }
}

QEvdevMouseHandler::QEvdevMouseHandler(const QString &device, int fd, bool abs, bool, int jitterLimit)
    : m_device(device), m_fd(fd), m_notify(0), m_x(0), m_y(0), m_prevx(0), m_prevy(0),
      m_abs(abs), m_buttons(0), m_button(Qt::NoButton), m_eventType(QEvent::None), m_prevInvalid(true)
{
    Q_UNUSED(jitterLimit)
    setObjectName(QLatin1String("Evdev Mouse Handler"));

    QSocketNotifier *notifier;
    notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated,
            this, &QEvdevMouseHandler::readMouseData);
}

QEvdevMouseHandler::~QEvdevMouseHandler()
{
    if (m_fd >= 0)
        qt_safe_close(m_fd);
}

void QEvdevMouseHandler::sendMouseEvent()
{
    int x = m_x - m_prevx;
    int y = m_y - m_prevy;

    if (m_prevInvalid) {
        x = 0;
        y = 0;
        m_prevInvalid = false;
    }

    if (m_eventType == QEvent::MouseMove)
        emit handleMouseEvent(x, y, m_abs, m_buttons, Qt::NoButton, m_eventType);
    else
        emit handleMouseEvent(x, y, m_abs, m_buttons, m_button, m_eventType);

    m_prevx = m_x;
    m_prevy = m_y;
}

void QEvdevMouseHandler::readMouseData()
{
    EV_DEV_EVENT buffer[32];
    int n = 0;
    bool posChanged = false, btnChanged = false;
    bool pendingMouseEvent = false;
    forever {
        int result = read(m_fd, reinterpret_cast<char *>(buffer) + n, sizeof(buffer) - n);

        if (result == 0) {
            qCWarning(qLcEvdevMouse)<<"evdevmouse: Got EOF from the input device";
            return;
        } else if (result < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                qCWarning(qLcEvdevMouse)<<"evdevmouse: Could not read from input device"<<errno;
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
        EV_DEV_EVENT *data = &buffer[i];
        if (data->type == EV_DEV_REL) {
            if (data->code == EV_DEV_PTR_REL_X) {
                m_x += data->value;
                posChanged = true;
            } else if (data->code == EV_DEV_PTR_REL_Y) {
                m_y += data->value;
                posChanged = true;
            }
        } else if (data->type == EV_DEV_KEY && data->code >= EV_DEV_PTR_BTN_LEFT) {
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
        } else if (data->type == EV_DEV_SYN) {
            if (btnChanged) {
                btnChanged = posChanged = false;
                sendMouseEvent();
                pendingMouseEvent = false;
            } else if (posChanged) {
                m_eventType = QEvent::MouseMove;
                posChanged = false;
                if (m_compression) {
                    pendingMouseEvent = true;
                } else {
                    sendMouseEvent();
                }
            }
        }
    }
    if (m_compression && pendingMouseEvent) {
        int distanceSquared = (m_x - m_prevx)*(m_x - m_prevx) + (m_y - m_prevy)*(m_y - m_prevy);
        if (distanceSquared > m_jitterLimitSquared)
            sendMouseEvent();
    }
}

QT_END_NAMESPACE
