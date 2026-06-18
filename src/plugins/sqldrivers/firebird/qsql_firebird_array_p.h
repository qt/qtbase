// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2026 Andreas Bacher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QSQL_FIREBIRD_ARRAY_P_H
#define QSQL_FIREBIRD_ARRAY_P_H

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

#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

#include <ibase.h>               // ISC_QUAD, ISC_ARRAY_DESC
#include <firebird/Interface.h>

QT_BEGIN_NAMESPACE

//  Firebird ARRAY column support (qsql_firebird_array.cpp). The descriptor
//  lookup, slice sizing and (de)serialisation internals are file-static there;
//  only the two whole-column entry points are exposed.

// Identifies the array column an ISC_ARRAY_DESC was resolved for
struct ArrayFieldKey
{
    QByteArray relation;
    QByteArray field;
};

inline bool operator==(const ArrayFieldKey &lhs, const ArrayFieldKey &rhs) noexcept
{
    return lhs.relation == rhs.relation && lhs.field == rhs.field;
}

inline size_t qHash(const ArrayFieldKey &key, size_t seed = 0) noexcept
{
    return qHashMulti(seed, key.relation, key.field);
}

using ArrayDescCache = QHash<ArrayFieldKey, ISC_ARRAY_DESC>;

// Fetch array data via OO API (IAttachment::getSlice)
QVariant fetchArray(Firebird::IAttachment *att, Firebird::ITransaction *tra,
                    Firebird::IStatus *iStatus, Firebird::IMaster *master,
                    ArrayDescCache &descCache,
                    const ISC_QUAD &arrayId, const QString &relation,
                    const QString &field);

// Write array data via OO API (IAttachment::putSlice)
bool writeArray(Firebird::IAttachment *att, Firebird::ITransaction *tra,
                Firebird::IStatus *iStatus, Firebird::IMaster *master,
                ArrayDescCache &descCache,
                ISC_QUAD *arrayId, const QString &relation,
                const QString &field, const QList<QVariant> &list);

QT_END_NAMESPACE

#endif // QSQL_FIREBIRD_ARRAY_P_H
