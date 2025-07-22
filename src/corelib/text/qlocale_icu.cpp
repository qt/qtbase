// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qglobal.h"
#include "qdebug.h"
#include "qlocale_p.h"
#include "qmutex.h"

#include "unicode/uloc.h"
#include "unicode/ustring.h"

QT_BEGIN_NAMESPACE

static_assert(std::is_same_v<UChar, char16_t>,
              "Hmm... in C++ mode, ICU's UChar ought to be char16_t");

namespace QtIcuPrivate {

enum class CaseConversion : bool { Upper, Lower };

static bool qt_u_strToCase(const QString &str, QString *out, const char *localeID,
                           CaseConversion conv)
{
    Q_ASSERT(out);

    int32_t size = str.size();
    size += size >> 2; // add 25% for possible expansions
    QString result(size, Qt::Uninitialized);

    UErrorCode status = U_ZERO_ERROR;

    const auto caseFunc = [conv] (auto&&...args) {
            // try to be a completely transparent wrapper:
            using R [[maybe_unused]] = decltype(u_strToUpper(std::forward<decltype(args)>(args)...));
            switch (conv) {
            case CaseConversion::Upper:
                return u_strToUpper(std::forward<decltype(args)>(args)...);
            case CaseConversion::Lower:
                return u_strToLower(std::forward<decltype(args)>(args)...);
            };
            Q_UNREACHABLE_RETURN(R{0});
        };

    size = caseFunc(result.data_ptr().data(), result.size(),
            str.data_ptr().data(), str.size(),
            localeID, &status);

    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR)
        return false;

    if (size < result.size()) {
        result.resize(size);
    } else if (size > result.size()) {
        // the resulting string is larger than our source string
        result.resize(size);

        status = U_ZERO_ERROR;
        size = caseFunc(result.data_ptr().data(), result.size(),
            str.data_ptr().data(), str.size(),
            localeID, &status);

        if (U_FAILURE(status))
            return false;

        // if the sizes don't match now, we give up.
        if (size != result.size())
            return false;
    }

    *out = std::move(result);
    return true;
}

} // namespace QtIcuPrivate

QString QLocalePrivate::toUpper(const QString &str, bool *ok) const
{
    Q_ASSERT(ok);
    using namespace QtIcuPrivate;
    QString out;
    *ok = qt_u_strToCase(str, &out, bcp47Name('_'), CaseConversion::Upper);
    return out;
}

QString QLocalePrivate::toLower(const QString &str, bool *ok) const
{
    Q_ASSERT(ok);
    using namespace QtIcuPrivate;
    QString out;
    *ok = qt_u_strToCase(str, &out, bcp47Name('_'), CaseConversion::Lower);
    return out;
}

QT_END_NAMESPACE
