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

    bool compression = false;
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

    emit handleMouseEvent(x, y, m_abs, m_buttons, m_button, m_eventType);

    m_prevx = m_x;
    m_prevy = m_y;
}

void QEvdevMouseHandler::readMouseData()
{
    bool posChanged = false;
    bool btnChanged = false;

    Q_FOREVER {
        EV_DEV_EVENT ev;
        size_t n = read(m_fd, (char *)(&ev), sizeof(EV_DEV_EVENT));
        if (n < sizeof(EV_DEV_EVENT)) {
            sendMouseEvent();
            return;
        }
        switch (ev.type) {
        case EV_DEV_KEY: {
            Qt::MouseButton buttons = Qt::NoButton;
            switch (ev.code) {
            case EV_DEV_PTR_BTN_LEFT:
                buttons = Qt::LeftButton;
                break;
            case EV_DEV_PTR_BTN_RIGHT:
                buttons = Qt::RightButton;
                break;
            case EV_DEV_PTR_BTN_MIDDLE:
                buttons = Qt::MiddleButton;
                break;
            }
            if (ev.value)
                m_buttons |= buttons;
            else
                m_buttons &= ~buttons;
            m_button = buttons;
            m_eventType = ev.value != 0 ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease;
            btnChanged = true;
            break;
            }
        case EV_DEV_REL:
            switch (ev.code) {
            case EV_DEV_PTR_REL_X:
                m_x += ev.value;
                m_eventType = QEvent::MouseMove;
                posChanged = true;
                break;
            case EV_DEV_PTR_REL_Y:
                m_y += ev.value;
                m_eventType = QEvent::MouseMove;
                posChanged = true;
                break;
            }
            break;
        case EV_DEV_ABS:
            switch (ev.code) {
            case EV_DEV_PTR_ABS_X:
                m_x = ev.value;
                m_eventType = QEvent::MouseMove;
                posChanged = true;
                break;
            case EV_DEV_PTR_ABS_Y:
                m_y = ev.value;
                m_eventType = QEvent::MouseMove;
                posChanged = true;
                break;
            }
            break;
        }

        if (btnChanged) {
            btnChanged = false;
            posChanged = false;
            sendMouseEvent();
        } else if (posChanged) {
            posChanged = false;
            sendMouseEvent();
        }
    }
}

QT_END_NAMESPACE
