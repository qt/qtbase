// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qhttpheadershelper_p.h"

#include <QtNetwork/qhttpheaders.h>

QT_BEGIN_NAMESPACE

bool QHttpHeadersHelper::compareStrict(const QHttpHeaders &left, const QHttpHeaders &right)
{
    if (left.size() != right.size())
        return false;

    for (qsizetype i = 0; i < left.size(); ++i) {
        if (left.nameAt(i) != right.nameAt(i))
            return false;
        if (left.valueAt(i) != right.valueAt(i))
            return false;
    }

    return true;
}

QT_END_NAMESPACE
