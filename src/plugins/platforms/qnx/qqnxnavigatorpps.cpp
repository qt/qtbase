// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxnavigatorpps.h"

#include <QDebug>
#include <QHash>
#include <QByteArray>
#include <private/qcore_unix_p.h>

#if defined(QQNXNAVIGATOR_DEBUG)
#define qNavigatorDebug qDebug
#else
#define qNavigatorDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

const char *QQnxNavigatorPps::navigatorControlPath = "/pps/services/navigator/control";
const size_t QQnxNavigatorPps::ppsBufferSize = 4096;

QQnxNavigatorPps::QQnxNavigatorPps(QObject *parent)
    : QQnxAbstractNavigator(parent)
    , m_fd(-1)
{
}

QQnxNavigatorPps::~QQnxNavigatorPps()
{
    // close connection to navigator
    if (m_fd != -1)
        qt_safe_close(m_fd);
}

bool QQnxNavigatorPps::openPpsConnection()
{
    if (m_fd != -1)
        return true;

    // open connection to navigator
    errno = 0;
    m_fd = qt_safe_open(navigatorControlPath, O_RDWR);
    if (m_fd == -1) {
        qWarning("QQNX: failed to open navigator pps, errno=%d", errno);
        return false;
    }

    qNavigatorDebug("successfully connected to Navigator. fd=%d", m_fd);

    return true;
}

bool QQnxNavigatorPps::requestInvokeUrl(const QByteArray &encodedUrl)
{
    if (!openPpsConnection())
        return false;

    return sendPpsMessage("invoke", encodedUrl);
}

bool QQnxNavigatorPps::sendPpsMessage(const QByteArray &message, const QByteArray &data)
{
    QByteArray ppsMessage = "msg::" + message;

    if (!data.isEmpty())
        ppsMessage += "\ndat::" + data;

    ppsMessage += "\n";

    qNavigatorDebug() << "sending PPS message:\n" << ppsMessage;

    // send pps message to navigator
    errno = 0;
    int bytes = qt_safe_write(m_fd, ppsMessage.constData(), ppsMessage.size());
    if (Q_UNLIKELY(bytes == -1))
        qFatal("QQNX: failed to write navigator pps, errno=%d", errno);

    // allocate buffer for pps data
    char buffer[ppsBufferSize];

    // attempt to read pps data
    do {
        errno = 0;
        bytes = qt_safe_read(m_fd, buffer, ppsBufferSize - 1);
        if (Q_UNLIKELY(bytes == -1))
            qFatal("QQNX: failed to read navigator pps, errno=%d", errno);
    } while (bytes == 0);

    // ensure data is null terminated
    buffer[bytes] = '\0';

    qNavigatorDebug() << "received PPS message:\n" << buffer;

    // process received message
    QByteArray ppsData(buffer);
    QHash<QByteArray, QByteArray> responseFields;
    parsePPS(ppsData, responseFields);

    if (responseFields.contains("res") && responseFields.value("res") == message) {
        if (Q_UNLIKELY(responseFields.contains("err"))) {
            qCritical() << "navigator responded with error: " << responseFields.value("err");
            return false;
        }
    }

    return true;
}

void QQnxNavigatorPps::parsePPS(const QByteArray &ppsData, QHash<QByteArray, QByteArray> &messageFields)
{
    qNavigatorDebug() << "data=" << ppsData;

    // tokenize pps data into lines
    QList<QByteArray> lines = ppsData.split('\n');

    // validate pps object
    if (Q_UNLIKELY(lines.empty() || lines.at(0) != "@control"))
        qFatal("QQNX: unrecognized pps object, data=%s", ppsData.constData());

    // parse pps object attributes and extract values
    for (int i = 1; i < lines.size(); i++) {

        // tokenize current attribute
        const QByteArray &attr = lines.at(i);

        qNavigatorDebug() << "attr=" << attr;

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

        qNavigatorDebug() << "key=" << key;
        qNavigatorDebug() << "val=" << value;
        messageFields[key] = value;
    }
}

QT_END_NAMESPACE
