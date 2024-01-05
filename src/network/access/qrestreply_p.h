// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRESTREPLY_P_H
#define QRESTREPLY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "private/qobject_p.h"
#include <QtNetwork/qnetworkreply.h>
#include <QtCore/qjsondocument.h>

#include <optional>

QT_BEGIN_NAMESPACE

class QStringDecoder;

class QRestReplyPrivate : public QObjectPrivate
{
public:
    QRestReplyPrivate();
    ~QRestReplyPrivate() override;

    QNetworkReply *networkReply = nullptr;
    std::optional<QStringDecoder> decoder;

    QByteArray contentCharset() const;
    bool hasNonHttpError() const;
};

QT_END_NAMESPACE

#endif
