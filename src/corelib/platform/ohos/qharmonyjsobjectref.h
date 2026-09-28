// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHARMONYJSOBJECTREF_H
#define QHARMONYJSOBJECTREF_H

#include <QtCore/qglobal.h>

#if defined(Q_QDOC) || defined(Q_OS_HARMONY)

#include <QtCore/qcompare.h>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qhashfunctions.h>
#include <cstdint>
#include <memory>
#include <napi/native_api.h>

QT_BEGIN_NAMESPACE

class QHarmonyJsObjectRef;
Q_CORE_EXPORT size_t qHash(const QHarmonyJsObjectRef &key, size_t seed = 0) noexcept;

class Q_CORE_EXPORT QHarmonyJsObjectRef
{
public:
    QHarmonyJsObjectRef();
    QHarmonyJsObjectRef(const QHarmonyJsObjectRef &other);
    QHarmonyJsObjectRef(QHarmonyJsObjectRef &&other) noexcept;
    QHarmonyJsObjectRef &operator=(const QHarmonyJsObjectRef &other);
    QHarmonyJsObjectRef &operator=(QHarmonyJsObjectRef &&other) noexcept;
    ~QHarmonyJsObjectRef();

    static QHarmonyJsObjectRef fromValue(QHarmonyJsContext &jsContext, napi_value value);

    napi_value value(QHarmonyJsContext &jsContext) const;
    bool isNull() const;

private:
    friend Q_CORE_EXPORT bool comparesEqual(const QHarmonyJsObjectRef &lhs, const QHarmonyJsObjectRef &rhs) noexcept;
    Q_DECLARE_EQUALITY_COMPARABLE(QHarmonyJsObjectRef)
    friend Q_CORE_EXPORT size_t qHash(const QHarmonyJsObjectRef &key, size_t seed) noexcept;

    std::int64_t identity() const noexcept;

    class Private;
    std::shared_ptr<Private> d;
};

QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QHarmonyJsObjectRef)

QT_END_NAMESPACE

#endif // Q_QDOC || Q_OS_HARMONY

#endif // QHARMONYJSOBJECTREF_H
