/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qtslib.h"


#include <QSocketNotifier>
#include <QStringList>
#include <QPoint>
#include <QWindowSystemInterface>

#include <Qt>

#include <errno.h>
#include <tslib.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

QTsLibMouseHandler::QTsLibMouseHandler(const QString &key,
                                                 const QString &specification)
    : m_notify(0), m_x(0), m_y(0), m_pressed(0), m_rawMode(false)
{
    qDebug() << "QTsLibMouseHandler" << key << specification;
    setObjectName(QLatin1String("TSLib Mouse Handler"));

    QByteArray device = "/dev/input/event1";
    if (specification.startsWith("/dev/"))
        device = specification.toLocal8Bit();

    m_dev =  ts_open(device.constData(), 1);

    if (ts_config(m_dev)) {
        perror("Error configuring\n");
    }


    m_rawMode =  !key.compare(QLatin1String("TslibRaw"), Qt::CaseInsensitive);

    int fd = ts_fd(m_dev);
    if (fd >= 0) {
        m_notify = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(m_notify, SIGNAL(activated(int)), this, SLOT(readMouseData()));
    } else {
        qWarning("Cannot open mouse input device '%s': %s", device.constData(), strerror(errno));
        return;
    }
}


QTsLibMouseHandler::~QTsLibMouseHandler()
{
    if (m_dev)
        ts_close(m_dev);
}


static bool get_sample(struct tsdev *dev, struct ts_sample *sample, bool rawMode)
{
    if (rawMode) {
        return (ts_read_raw(dev, sample, 1) == 1);
    } else {
        int ret = ts_read(dev, sample, 1);
        return ( ret == 1);
    }
}


void QTsLibMouseHandler::readMouseData()
{
    ts_sample sample;
    while (get_sample(m_dev, &sample, m_rawMode)) {

        bool pressed = sample.pressure;
        int x = sample.x;
        int y = sample.y;


        if (!m_rawMode) {
            //filtering: ignore movements of 2 pixels or less
            int dx = x - m_x;
            int dy = y - m_y;
            if (dx*dx <= 4 && dy*dy <= 4 && pressed == m_pressed)
                continue;
        } else {
            // work around missing coordinates on mouse release in raw mode
            if (sample.pressure == 0 && sample.x == 0 && sample.y == 0) {
                x = m_x;
                y = m_y;
            }
        }
        QPoint pos(x, y);

        //printf("handleMouseEvent %d %d %d %ld\n", m_x, m_y, pressed, sample.tv.tv_usec);

        QWindowSystemInterface::handleMouseEvent(0, pos, pos, pressed ? Qt::LeftButton : Qt::NoButton);

        m_x = x;
        m_y = y;
        m_pressed = pressed;
    }
}

QT_END_NAMESPACE
