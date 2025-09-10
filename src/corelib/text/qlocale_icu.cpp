// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qglobal.h"
#include "qdebug.h"
#include "qlocale_p.h"
#include "qmutex.h"

#include "unicode/uloc.h"
#include "unicode/ustring.h"

QT_BEGIN_NAMESPACE

typedef int32_t (*Ptr_u_strToCase)(UChar *dest, int32_t destCapacity, const UChar *src, int32_t srcLength, const char *locale, UErrorCode *pErrorCode);

// caseFunc can either be u_strToUpper or u_strToLower
static bool qt_u_strToCase(const QString &str, QString *out, const char *localeID, Ptr_u_strToCase caseFunc)
{
    Q_ASSERT(out);

    int32_t size = str.size();
    size += size >> 2; // add 25% for possible expansions
    QString result(size, Qt::Uninitialized);

    UErrorCode status = U_ZERO_ERROR;

    size = caseFunc(reinterpret_cast<UChar *>(result.data()), result.size(),
            reinterpret_cast<const UChar *>(str.constData()), str.size(),
            localeID, &status);

    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR)
        return false;

    if (size < result.size()) {
        result.resize(size);
    } else if (size > result.size()) {
        // the resulting string is larger than our source string
        result.resize(size);

        status = U_ZERO_ERROR;
        size = caseFunc(reinterpret_cast<UChar *>(result.data()), result.size(),
            reinterpret_cast<const UChar *>(str.constData()), str.size(),
            localeID, &status);

        if (U_FAILURE(status))
            return false;

        // if the sizes don't match now, we give up.
        if (size != result.size())
            return false;
    }

    *out = result;
    return true;
}

QString QIcu::toUpper(const QByteArray &localeID, const QString &str, bool *ok)
{
    QString out;
    bool err = qt_u_strToCase(str, &out, localeID, u_strToUpper);
    if (ok)
        *ok = err;
    return out;
}

QString QIcu::toLower(const QByteArray &localeID, const QString &str, bool *ok)
{
    QString out;
    bool err = qt_u_strToCase(str, &out, localeID, u_strToLower);
    if (ok)
        *ok = err;
    return out;
}

QT_END_NAMESPACE
