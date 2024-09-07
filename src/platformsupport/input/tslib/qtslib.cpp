// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtslib_p.h"

#include <QSocketNotifier>
#include <QStringList>
#include <QPoint>
#include <QLoggingCategory>

#include <qpa/qwindowsysteminterface.h>

#include <errno.h>
#include <tslib.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(qLcTsLib, "qt.qpa.input")

QTsLibMouseHandler::QTsLibMouseHandler(const QString &key,
                                       const QString &specification,
                                       QObject *parent)
    : QObject(parent),
    m_rawMode(!key.compare("TslibRaw"_L1, Qt::CaseInsensitive))
{
    qCDebug(qLcTsLib) << "Initializing tslib plugin" << key << specification;
    setObjectName("TSLib Mouse Handler"_L1);

    m_dev = ts_setup(nullptr, 1);
    if (!m_dev) {
        qErrnoWarning(errno, "ts_setup() failed");
        return;
    }

#ifdef TSLIB_VERSION_EVENTPATH /* also introduced in 1.15 */
    qCDebug(qLcTsLib) << "tslib device is" << ts_get_eventpath(m_dev);
#endif
    m_notify = new QSocketNotifier(ts_fd(m_dev), QSocketNotifier::Read, this);
    connect(m_notify, &QSocketNotifier::activated, this, &QTsLibMouseHandler::readMouseData);
}

QTsLibMouseHandler::~QTsLibMouseHandler()
{
    if (m_dev)
        ts_close(m_dev);
}

static bool get_sample(struct tsdev *dev, struct ts_sample *sample, bool rawMode)
{
    if (rawMode)
        return (ts_read_raw(dev, sample, 1) == 1);
    else
        return (ts_read(dev, sample, 1) == 1);
}

void QTsLibMouseHandler::readMouseData()
{
    ts_sample sample;

    while (get_sample(m_dev, &sample, m_rawMode)) {
        bool pressed = sample.pressure;
        int x = sample.x;
        int y = sample.y;

        // coordinates on release events can contain arbitrary values, just ignore them
        if (sample.pressure == 0) {
            x = m_x;
            y = m_y;
        }

        if (!m_rawMode) {
            //filtering: ignore movements of 2 pixels or less
            int dx = x - m_x;
            int dy = y - m_y;
            if (dx*dx <= 4 && dy*dy <= 4 && pressed == m_pressed)
                continue;
        }
        QPoint pos(x, y);

        Qt::MouseButton button = pressed ^ m_pressed ? Qt::LeftButton : Qt::NoButton;
        Qt::MouseButtons state = pressed ? Qt::LeftButton : Qt::NoButton;
        QEvent::Type type = pressed ? (m_pressed ? QEvent::MouseMove : QEvent::MouseButtonPress)
                                    : QEvent::MouseButtonRelease;

        QWindowSystemInterface::handleMouseEvent(nullptr, pos, pos, state, button, type);

        m_x = x;
        m_y = y;
        m_pressed = pressed;
    }
}

QT_END_NAMESPACE

#include "moc_qtslib_p.cpp"
