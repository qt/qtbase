/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#include "qqnxnavigatoreventnotifier.h"

#include "qqnxnavigatoreventhandler.h"

#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QSocketNotifier>
#include <QtCore/private/qcore_unix_p.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(QQNXNAVIGATOREVENTNOTIFIER_DEBUG)
#define qNavigatorEventNotifierDebug qDebug
#else
#define qNavigatorEventNotifierDebug QT_NO_QDEBUG_MACRO
#endif

static const char *navigatorControlPath = "/pps/services/navigator/control";
static const int ppsBufferSize = 4096;

QT_BEGIN_NAMESPACE

QQnxNavigatorEventNotifier::QQnxNavigatorEventNotifier(QQnxNavigatorEventHandler *eventHandler, QObject *parent)
    : QObject(parent),
      m_fd(-1),
      m_readNotifier(0),
      m_eventHandler(eventHandler)
{
}

QQnxNavigatorEventNotifier::~QQnxNavigatorEventNotifier()
{
    delete m_readNotifier;

    // close connection to navigator
    if (m_fd != -1)
        close(m_fd);

    qNavigatorEventNotifierDebug("navigator event notifier stopped");
}

void QQnxNavigatorEventNotifier::start()
{
    qNavigatorEventNotifierDebug("navigator event notifier started");

    // open connection to navigator
    errno = 0;
    m_fd = open(navigatorControlPath, O_RDWR);
    if (m_fd == -1) {
        qNavigatorEventNotifierDebug("failed to open navigator pps: %s", strerror(errno));
        return;
    }

    m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read);
    connect(m_readNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(readData()));
}

void QQnxNavigatorEventNotifier::parsePPS(const QByteArray &ppsData, QByteArray &msg, QByteArray &dat, QByteArray &id)
{
    qNavigatorEventNotifierDebug() << "data=" << ppsData;

    // tokenize pps data into lines
    QList<QByteArray> lines = ppsData.split('\n');

    // validate pps object
    if (Q_UNLIKELY(lines.empty() || lines.at(0) != "@control"))
        qFatal("QQNX: unrecognized pps object, data=%s", ppsData.constData());

    // parse pps object attributes and extract values
    for (int i = 1; i < lines.size(); ++i) {

        // tokenize current attribute
        const QByteArray &attr = lines.at(i);
        qNavigatorEventNotifierDebug() << "attr=" << attr;

        int firstColon = attr.indexOf(':');
        if (firstColon == -1) {
            // abort - malformed attribute
            continue;
        }

        int secondColon = attr.indexOf(':', firstColon + 1);
        if (secondColon == -1) {
            // abort - malformed attribute
            continue;
        }

        QByteArray key = attr.left(firstColon);
        QByteArray value = attr.mid(secondColon + 1);

        qNavigatorEventNotifierDebug() << "key=" << key;
        qNavigatorEventNotifierDebug() << "val=" << value;

        // save attribute value
        if (key == "msg")
            msg = value;
        else if (key == "dat")
            dat = value;
        else if (key == "id")
            id = value;
        else
            qFatal("QQNX: unrecognized pps attribute, attr=%s", key.constData());
    }
}

void QQnxNavigatorEventNotifier::replyPPS(const QByteArray &res, const QByteArray &id, const QByteArray &dat)
{
    // construct pps message
    QByteArray ppsData = "res::";
    ppsData += res;
    ppsData += "\nid::";
    ppsData += id;
    if (!dat.isEmpty()) {
        ppsData += "\ndat::";
        ppsData += dat;
    }
    ppsData += "\n";

    qNavigatorEventNotifierDebug() << "reply=" << ppsData;

    // send pps message to navigator
    errno = 0;
    int bytes = write(m_fd, ppsData.constData(), ppsData.size());
    if (Q_UNLIKELY(bytes == -1))
        qFatal("QQNX: failed to write navigator pps, errno=%d", errno);
}

void QQnxNavigatorEventNotifier::handleMessage(const QByteArray &msg, const QByteArray &dat, const QByteArray &id)
{
    qNavigatorEventNotifierDebug() << "msg=" << msg << ", dat=" << dat << ", id=" << id;

    // check message type
    if (msg == "orientationCheck") {
        const bool response = m_eventHandler->handleOrientationCheck(dat.toInt());

        // reply to navigator that (any) orientation is acceptable
        replyPPS(msg, id, response ? "true" : "false");
    } else if (msg == "orientation") {
        m_eventHandler->handleOrientationChange(dat.toInt());
        replyPPS(msg, id, "");
    } else if (msg == "SWIPE_DOWN") {
        m_eventHandler->handleSwipeDown();
    } else if (msg == "exit") {
        m_eventHandler->handleExit();
    } else if (msg == "windowActive") {
        m_eventHandler->handleWindowGroupActivated(dat);
    } else if (msg == "windowInactive") {
        m_eventHandler->handleWindowGroupDeactivated(dat);
    }
}

void QQnxNavigatorEventNotifier::readData()
{
    qNavigatorEventNotifierDebug("reading navigator data");

    // allocate buffer for pps data
    char buffer[ppsBufferSize];

    // attempt to read pps data
    errno = 0;
    int bytes = qt_safe_read(m_fd, buffer, ppsBufferSize - 1);
    if (Q_UNLIKELY(bytes == -1))
        qFatal("QQNX: failed to read navigator pps, errno=%d", errno);

    // check if pps data was received
    if (bytes > 0) {

        // ensure data is null terminated
        buffer[bytes] = '\0';

        // process received message
        QByteArray ppsData(buffer);
        QByteArray msg;
        QByteArray dat;
        QByteArray id;
        parsePPS(ppsData, msg, dat, id);
        handleMessage(msg, dat, id);
    }
}

QT_END_NAMESPACE
