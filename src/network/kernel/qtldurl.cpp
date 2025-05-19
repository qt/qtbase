// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <QtNetwork/private/qtnetworkglobal_p.h>

#if QT_CONFIG(topleveldomain)

#include "QtCore/qfile.h"
#include "QtCore/qloggingcategory.h"
#include "QtCore/qstandardpaths.h"
#include "QtCore/qstring.h"

#if !QT_CONFIG(publicsuffix_qt) && !QT_CONFIG(publicsuffix_system)
#   error Enable at least one feature: publicsuffix-qt, publicsuffix-system
#endif

#if QT_CONFIG(publicsuffix_qt)
#   include "psl_data.cpp"
#endif

// Defined in src/3rdparty/libpsl/src/lookup_string_in_fixed_set.c
extern "C" int LookupStringInFixedSet(const unsigned char *graph, std::size_t length,
                                      const char *key, std::size_t key_length);

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(publicsuffix_system)
Q_STATIC_LOGGING_CATEGORY(lcTld, "qt.network.tld")
#endif

static constexpr int PSL_NOT_FOUND = -1;
static constexpr int PSL_FLAG_EXCEPTION = 1 << 0;
static constexpr int PSL_FLAG_WILDCARD = 1 << 1;

class QPublicSuffixDatabase final
{
public:
#if QT_CONFIG(publicsuffix_system)
    QPublicSuffixDatabase();
#endif // QT_CONFIG(publicsuffix_system)

    int lookupDomain(QByteArrayView domain) const;

private:
    QByteArrayView m_data
#if QT_CONFIG(publicsuffix_qt)
    {
        kDafsa, sizeof(kDafsa)
    }
#endif // QT_CONFIG(publicsuffix_qt)
    ;

#if QT_CONFIG(publicsuffix_system)
    std::unique_ptr<QFile> m_dev;
    QByteArray m_storage;
    bool loadFile(const QString &fileName);
#endif // QT_CONFIG(publicsuffix_system)
};

int QPublicSuffixDatabase::lookupDomain(QByteArrayView domain) const
{
    return LookupStringInFixedSet(reinterpret_cast<const unsigned char *>(m_data.constData()),
                                     m_data.size(), domain.data(), domain.size());
}

#if QT_CONFIG(publicsuffix_system)

static QStringList locatePublicSuffixFiles()
{
    return QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                     u"publicsuffix/public_suffix_list.dafsa"_s);
}

QPublicSuffixDatabase::QPublicSuffixDatabase()
{
    for (auto &&fileName : locatePublicSuffixFiles()) {
        if (loadFile(fileName))
            return;
    }

#if QT_CONFIG(publicsuffix_qt)
    qCDebug(lcTld, "Using builtin publicsuffix list");
#else
    qCWarning(lcTld, "No usable publicsuffix file found");
#endif
}

bool QPublicSuffixDatabase::loadFile(const QString &fileName)
{
    static const QByteArrayView DafsaFileHeader = ".DAFSA@PSL_0   \n";

    qCDebug(lcTld, "Loading publicsuffix file: %s", qUtf8Printable(fileName));

    auto systemFile = std::make_unique<QFile>(fileName);

    if (!systemFile->open(QIODevice::ReadOnly)) {
        qCDebug(lcTld, "Failed to open publicsuffix file: %s",
                qUtf8Printable(systemFile->errorString()));
        return false;
    }

    auto fileSize = systemFile->size();
    // Check if there is enough data for header, version byte and some data
    if (fileSize < DafsaFileHeader.size() + 2) {
        qCWarning(lcTld, "publicsuffix file is too small: %zu", std::size_t(fileSize));
        return false;
    }

    auto header = systemFile->read(DafsaFileHeader.size());
    if (header != DafsaFileHeader) {
        qCWarning(lcTld, "Invalid publicsuffix file header: %s", header.toHex().constData());
        return false;
    }

    // Check if the file is UTF-8 compatible
    if (!systemFile->seek(fileSize - 1)) {
        qCWarning(lcTld, "Failed to seek to the end of file: %s",
                  qUtf8Printable(systemFile->errorString()));
        return false;
    }

    char version;
    if (systemFile->read(&version, 1) != 1) {
        qCWarning(lcTld, "Failed to read publicsuffix version");
        return false;
    }

    if (version != 0x01) {
        qCWarning(lcTld, "Unsupported publicsuffix version: %d", int(version));
        return false;
    }

    const auto dataSize = fileSize - DafsaFileHeader.size() - 1;
    // Try to map the file first
    auto mappedData = systemFile->map(DafsaFileHeader.size(), dataSize);
    if (mappedData) {
        qCDebug(lcTld, "Using mapped system publicsuffix data");
        systemFile->close();
        m_data = QByteArrayView(mappedData, dataSize);
        m_dev = std::move(systemFile);
        return true;
    }

    qCDebug(lcTld, "Failed to map publicsuffix file: %s",
            qUtf8Printable(systemFile->errorString()));

    systemFile->seek(DafsaFileHeader.size());
    m_storage = systemFile->read(dataSize);
    if (m_storage.size() != dataSize) {
        qCWarning(lcTld, "Failed to read publicsuffix file");
        m_storage.clear();
        return false;
    }

    qCDebug(lcTld, "Using system publicsuffix data");
    m_data = m_storage;

    return true;
}

Q_GLOBAL_STATIC(QPublicSuffixDatabase, publicSuffix);

#else

static const QPublicSuffixDatabase m_publicSuffix;

#endif // QT_CONFIG(publicsuffix_system)

/*!
    \internal

    Return true if \a domain is a top-level-domain per Qt's copy of the Mozilla public suffix list.

    The \a domain must be in lower-case format (as per QString::toLower()).
*/

Q_NETWORK_EXPORT bool qIsEffectiveTLD(QStringView domain)
{
    // for domain 'foo.bar.com':
    // 1. return false if TLD table contains '!foo.bar.com'
    // 2. return true if TLD table contains 'foo.bar.com'
    // 3. return true if the table contains '*.bar.com'

    QByteArray decodedDomain = domain.toUtf8();
    QByteArrayView domainView(decodedDomain);

#if QT_CONFIG(publicsuffix_system)
    if (publicSuffix.isDestroyed())
        return false;
#else
    auto publicSuffix = &m_publicSuffix;
#endif // QT_CONFIG(publicsuffix_system)

    auto ret = publicSuffix->lookupDomain(domainView);
    if (ret != PSL_NOT_FOUND) {
        if (ret & PSL_FLAG_EXCEPTION) // 1
            return false;
        if ((ret & PSL_FLAG_WILDCARD) == 0) // 2
            return true;
    }

    const auto dot = domainView.indexOf('.');
    if (dot < 0) // Actual TLD: may be effective if the subject of a wildcard rule:
        return ret != PSL_NOT_FOUND;
    ret = publicSuffix->lookupDomain(domainView.sliced(dot + 1)); // 3
    if (ret == PSL_NOT_FOUND)
        return false;
    return (ret & PSL_FLAG_WILDCARD) != 0;
}

QT_END_NAMESPACE

#endif // QT_CONFIG(topleveldomain)
