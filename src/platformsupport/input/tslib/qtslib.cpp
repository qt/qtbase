/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#include "qtslib_p.h"

#include <QSocketNotifier>
#include <QStringList>
#include <QPoint>
#include <QLoggingCategory>

#include <qpa/qwindowsysteminterface.h>

#include <errno.h>
#include <tslib.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcTsLib, "qt.qpa.input")

QTsLibMouseHandler::QTsLibMouseHandler(const QString &key,
                                       const QString &specification,
                                       QObject *parent)
    : QObject(parent),
    m_notify(0), m_x(0), m_y(0), m_pressed(0), m_rawMode(false)
{
    qCDebug(qLcTsLib) << "Initializing tslib plugin" << key << specification;
    setObjectName(QLatin1String("TSLib Mouse Handler"));

    QByteArray device = qgetenv("TSLIB_TSDEVICE");

    if (specification.startsWith(QStringLiteral("/dev/")))
        device = specification.toLocal8Bit();

    if (device.isEmpty())
        device = QByteArrayLiteral("/dev/input/event1");

    m_dev = ts_open(device.constData(), 1);
    if (!m_dev) {
        qErrnoWarning(errno, "ts_open() failed");
        return;
    }

    if (ts_config(m_dev))
        qErrnoWarning(errno, "ts_config() failed");

    m_rawMode = !key.compare(QLatin1String("TslibRaw"), Qt::CaseInsensitive);

    int fd = ts_fd(m_dev);
    if (fd >= 0) {
        qCDebug(qLcTsLib) << "tslib device is" << device;
        m_notify = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(m_notify, SIGNAL(activated(int)), this, SLOT(readMouseData()));
    } else {
        qErrnoWarning(errno, "tslib: Cannot open input device %s", device.constData());
    }
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

        // work around missing coordinates on mouse release
        if (sample.pressure == 0 && sample.x == 0 && sample.y == 0) {
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

        QWindowSystemInterface::handleMouseEvent(0, pos, pos, pressed ? Qt::LeftButton : Qt::NoButton);

        m_x = x;
        m_y = y;
        m_pressed = pressed;
    }
}

QT_END_NAMESPACE
