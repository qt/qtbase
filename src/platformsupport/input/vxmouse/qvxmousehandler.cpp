
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvxmousehandler_p.h"

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
#include <evdevLib.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(qLcVxMouse, "qt.qpa.input")

std::unique_ptr<QVxMouseHandler> QVxMouseHandler::create(const QString &device, const QString &specification)
{
    qCDebug(qLcVxMouse) << "create mouse handler for" << device << specification;

    bool compression = true;
    int jitterLimit = 0;
    int grab = 0;
    bool abs = false;

    const auto args = QStringView{specification}.split(u':');
    for (const auto &arg : args) {
        if (arg == "nocompress"_L1)
            compression = false;
        else if (arg.startsWith("dejitter="_L1))
            jitterLimit = arg.mid(9).toInt();
        else if (arg.startsWith("grab="_L1))
            grab = arg.mid(5).toInt();
    }

    int fd = qt_safe_open(device.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK, 0);
    if (fd >= 0) {
        return std::unique_ptr<QVxMouseHandler>(new QVxMouseHandler(device, fd, compression, jitterLimit));
    } else {
        qCWarning(qLcVxMouse) << "Cannot open mouse input device" << qPrintable(device) << ":" << strerror(errno);
        return nullptr;
    }
}

QVxMouseHandler::QVxMouseHandler(const QString &device, int fd, bool compression, int jitterLimit)
    : m_device(device), m_fd(fd), m_compression(compression)
{
    setObjectName("Evdev Mouse Handler"_L1);

    m_jitterLimitSquared = jitterLimit * jitterLimit;

    // socket notifier for events on the mouse device
    m_notify = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notify, &QSocketNotifier::activated,
            this, &QVxMouseHandler::readMouseData);
}

QVxMouseHandler::~QVxMouseHandler()
{
    if (m_fd >= 0)
        qt_safe_close(m_fd);
}

void QVxMouseHandler::sendMouseEvent()
{
    int x;
    int y;

    x = m_x - m_prevx;
    y = m_y - m_prevy;
    if (m_prevInvalid) {
        x = y = 0;
        m_prevInvalid = false;
    }

    if (m_eventType == QEvent::MouseMove)
        emit handleMouseEvent(x, y, m_buttons, Qt::NoButton, m_eventType);
    else
        emit handleMouseEvent(x, y, m_buttons, m_button, m_eventType);

    m_prevx = m_x;
    m_prevy = m_y;
}

void QVxMouseHandler::readMouseData()
{
    EV_DEV_EVENT buffer[32];
    int n = 0;
    bool posChanged = false, btnChanged = false;
    bool pendingMouseEvent = false;
    forever {
        int result = QT_READ(m_fd, reinterpret_cast<char *>(buffer) + n, sizeof(buffer) - n);

        if (result == 0) {
            qCWarning(qLcVxMouse)<<"evdevmouse: Got EOF from the input device";
            return;
        } else if (result < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                qCWarning(qLcVxMouse)<<"evdevmouse: Could not read from input device"<<errno;
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

#include "moc_qvxmousehandler_p.cpp"
