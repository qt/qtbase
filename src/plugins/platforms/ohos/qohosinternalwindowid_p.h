// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCORE_OHOS_INTERNALWINDOWID_P_H
#define QCORE_OHOS_INTERNALWINDOWID_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qnapi_p.h>
#include <napi.h>
#include <string>

QT_BEGIN_NAMESPACE

namespace QtOhos {

class Q_CORE_EXPORT InternalWindowId
{
    enum {
        InvalidWindowId = 1,
        MainWindowId = 2,
    };

public:
    static InternalWindowId generate();
    static InternalWindowId invalidWindowId();
    static InternalWindowId fromNapiValue(QNapi::Number napiWindowId);

    constexpr InternalWindowId() = default;

    constexpr InternalWindowId(const InternalWindowId &) = default;
    constexpr InternalWindowId(InternalWindowId &&) = default;
    constexpr InternalWindowId &operator=(const InternalWindowId &) = default;
    constexpr InternalWindowId &operator=(InternalWindowId &&) = default;

    bool isMainWindowId() const;
    bool isValid() const;

    bool operator==(const InternalWindowId &other) const;
    bool operator!=(const InternalWindowId &other) const;

    bool operator<(const InternalWindowId &other) const;

    QNapi::Number toNapiValue(napi_env env) const;
    QString toString() const;
    std::string toStdString() const;

private:
    constexpr explicit InternalWindowId(int value);

    int m_value = InvalidWindowId;
};

}

QT_END_NAMESPACE

#endif
