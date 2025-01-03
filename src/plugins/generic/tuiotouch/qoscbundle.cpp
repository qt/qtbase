// Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qoscbundle_p.h"
#include "qtuio_p.h"

#include <QtEndian>
#include <QDebug>
#include <QLoggingCategory>


QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTuioBundle, "qt.qpa.tuio.bundle")

QOscBundle::QOscBundle() {}

// TUIO packets are transmitted using the OSC protocol, located at:
//   http://opensoundcontrol.org/specification
// Snippets of this specification have been pasted into the source as a means of
// easily communicating requirements.

QOscBundle::QOscBundle(const QByteArray &data)
    : m_isValid(false)
    , m_immediate(false)
    , m_timeEpoch(0)
    , m_timePico(0)
{
    // 8  16 24 32 40 48 56 64
    // #  b  u  n  d  l  e  \0
    // 23 62 75 6e 64 6c 65 00 // OSC string bundle identifier
    // 00 00 00 00 00 00 00 01 // osc time-tag, "immediately"
    // 00 00 00 30 // element length
    //      => message or bundle(s), preceded by length each time
    qCDebug(lcTuioBundle) << data.toHex();
    quint32 parsedBytes = 0;

    // "An OSC Bundle consists of the OSC-string "#bundle""
    QByteArray identifier;
    if (!qt_readOscString(data, identifier, parsedBytes) || identifier != "#bundle")
        return;

    // "followed by an OSC Time
    // Tag, followed by zero or more OSC Bundle Elements. The OSC-timetag is a
    // 64-bit fixed point time tag whose semantics are described below."
    if (parsedBytes > (quint32)data.size() || data.size() - parsedBytes < qsizetype(sizeof(quint64)))
        return;

    // "Time tags are represented by a 64 bit fixed point number. The first 32
    // bits specify the number of seconds since midnight on January 1,  1900,
    // and the last 32 bits specify fractional parts of a second to a precision
    // of about 200 picoseconds. This is the representation used by Internet NTP
    // timestamps."
    //
    // (editor's note: one may wonder how a 64bit big-endian number can also be
    // two 32bit numbers, without specifying in which order they occur or
    // anything, and one may indeed continue to wonder.)
    quint32 oscTimeEpoch = qFromBigEndian<quint32>(data.constData() + parsedBytes);
    parsedBytes += sizeof(quint32);
    quint32 oscTimePico = qFromBigEndian<quint32>(data.constData() + parsedBytes);
    parsedBytes += sizeof(quint32);

    bool isImmediate = false;

    if (oscTimeEpoch == 0 && oscTimePico == 1) {
        // "The time tag value consisting of 63 zero bits followed by a
        // one in the least significant bit is a special case meaning
        // "immediately.""
        isImmediate = true;
    }

    while (parsedBytes < (quint32)data.size()) {
        // "An OSC Bundle Element consists of its size and its contents. The size is an
        // int32 representing the number of 8-bit bytes in the contents, and will
        // always be a multiple of 4."
        //
        // in practice, a bundle can contain multiple bundles or messages,
        // though, and each is prefixed by a size.
        if (data.size() - parsedBytes < qsizetype(sizeof(quint32)))
            return;

        quint32 size = qFromBigEndian<quint32>((const uchar*)data.constData() + parsedBytes);
        parsedBytes += sizeof(quint32);

        if (data.size() - parsedBytes < size)
            return;

        if (size == 0) {
            // empty bundle; these are valid, but should they be allowed? the
            // spec is unclear on this...
            qCWarning(lcTuioBundle, "Empty bundle?");
            m_isValid = true;
            m_immediate = isImmediate;
            m_timeEpoch = oscTimeEpoch;
            m_timePico = oscTimePico;
            return;
        }

        // "The contents are either an OSC Message or an OSC Bundle.
        // Note this recursive definition: bundle may contain bundles."
        QByteArray subdata = data.mid(parsedBytes, size);
        parsedBytes += size;

        // "The contents of an OSC packet must be either an OSC Message or an OSC Bundle.
        // The first byte of the packet's contents unambiguously distinguishes between
        // these two alternatives."
        //
        // we're not dealing with a packet here, but the same trick works just
        // the same.
        QByteArray bundleIdentifier = QByteArray("#bundle\0",  8);
        if (subdata.startsWith('/')) {
            // starts with / => address pattern => start of a message
            QOscMessage subMessage(subdata);
            if (subMessage.isValid()) {
                m_isValid = true;
                m_immediate = isImmediate;
                m_timeEpoch = oscTimeEpoch;
                m_timePico = oscTimePico;
                m_messages.append(subMessage);
            } else {
                qCWarning(lcTuioBundle, "Invalid sub-message");
                return;
            }
        } else if (subdata.startsWith(bundleIdentifier)) {
            // bundle identifier start => bundle
            QOscBundle subBundle(subdata);
            if (subBundle.isValid()) {
                m_isValid = true;
                m_immediate = isImmediate;
                m_timeEpoch = oscTimeEpoch;
                m_timePico = oscTimePico;
                m_bundles.append(subBundle);
            }
        } else {
            qCWarning(lcTuioBundle, "Malformed sub-data!");
            return;
        }
    }
}

QT_END_NAMESPACE

