// Copyright (C) 2011 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

QT_BEGIN_NAMESPACE

// Q_LOGGING_CATEGORY(lcQpaQnxNavigatorEvents, "qt.qpa.qnx.navigator.events");

const char *QQnxNavigatorEventNotifier::navigatorControlPath = "/pps/services/navigator/control";
const size_t QQnxNavigatorEventNotifier::ppsBufferSize = 4096;

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

    qCDebug(lcQpaQnxNavigatorEvents) << "Navigator event notifier stopped";
}

void QQnxNavigatorEventNotifier::start()
{
    qCDebug(lcQpaQnxNavigatorEvents) << "Navigator event notifier started";

    // open connection to navigator
    errno = 0;
    m_fd = open(navigatorControlPath, O_RDWR);
    if (m_fd == -1) {
        qCDebug(lcQpaQnxNavigatorEvents, "Failed to open navigator pps: %s", strerror(errno));
        return;
    }

    m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read);
    connect(m_readNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(readData()));
}

void QQnxNavigatorEventNotifier::parsePPS(const QByteArray &ppsData, QByteArray &msg, QByteArray &dat, QByteArray &id)
{
    qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO << "data=" << ppsData;

    // tokenize pps data into lines
    QList<QByteArray> lines = ppsData.split('\n');

    // validate pps object
    if (Q_UNLIKELY(lines.empty() || lines.at(0) != "@control"))
        qFatal("QQNX: unrecognized pps object, data=%s", ppsData.constData());

    // parse pps object attributes and extract values
    for (int i = 1; i < lines.size(); ++i) {

        // tokenize current attribute
        const QByteArray &attr = lines.at(i);
        qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO << "attr=" << attr;

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

        qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO << "key =" << key << "value =" << value;

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

    qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO << "reply=" << ppsData;

    // send pps message to navigator
    errno = 0;
    int bytes = write(m_fd, ppsData.constData(), ppsData.size());
    if (Q_UNLIKELY(bytes == -1))
        qFatal("QQNX: failed to write navigator pps, errno=%d", errno);
}

void QQnxNavigatorEventNotifier::handleMessage(const QByteArray &msg, const QByteArray &dat, const QByteArray &id)
{
    qCDebug(lcQpaQnxNavigatorEvents) << Q_FUNC_INFO << "msg=" << msg << ", dat=" << dat << ", id=" << id;

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
    qCDebug(lcQpaQnxNavigatorEvents) << "Reading navigator data";

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
