// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTP2CONFIGURATION_P_H
#define QHTTP2CONFIGURATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the Network Access API. This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <private/http2protocol_p.h>
#include <private/qhttpheaderparser_p.h>
#include <private/qtnetworkglobal_p.h>

#include <QtNetwork/qhttp2configuration.h>

#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QHttp2ConfigurationPrivate : public QSharedData
{
public:
    unsigned sessionWindowSize = Http2::defaultSessionWindowSize;
    unsigned streamWindowSize = Http2::defaultSessionWindowSize;

    unsigned maxFrameSize = Http2::minPayloadLimit; // Initial (default) value of 16Kb.

    unsigned maxConcurrentStreams = Http2::maxConcurrentStreams;

    quint32 maxHeaderListSize = HeaderConstants::DEFAULT_MAX_TOTAL_HEADER_SIZE; // 256 KiB

    bool pushEnabled = false;
    // TODO: for now those two below are noop.
    bool huffmanCompressionEnabled = true;

    static QHttp2ConfigurationPrivate *get(QHttp2Configuration &config) { return config.d.data(); }
    static const QHttp2ConfigurationPrivate *get(const QHttp2Configuration &config)
    {
        return config.d.data();
    }
};

QT_END_NAMESPACE

#endif // QHTTP2CONFIGURATION_P_H
