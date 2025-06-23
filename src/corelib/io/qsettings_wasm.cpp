// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:trusted-data-only

#include "qsettings.h"
#ifndef QT_NO_SETTINGS

#include "qsettings_p.h"
#ifndef QT_NO_QOBJECT
#include "qcoreapplication.h"
#include <QFile>
#endif // QT_NO_QOBJECT
#include <QDebug>
#include <QtCore/private/qstdweb_p.h>
#include <QtCore/private/qwasmglobal_p.h>

#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QSet>

#include <emscripten.h>
#  include <emscripten/proxying.h>
#  include <emscripten/threading.h>
#  include <emscripten/val.h>

QT_BEGIN_NAMESPACE

using emscripten::val;
using namespace Qt::StringLiterals;

namespace {
QStringView keyNameFromPrefixedStorageName(QStringView prefix, QStringView prefixedStorageName)
{
    // Return the key slice after m_keyPrefix, or an empty string view if no match
    if (!prefixedStorageName.startsWith(prefix))
        return QStringView();
    return prefixedStorageName.sliced(prefix.length());
}
} // namespace

//
// Native settings implementation for WebAssembly using window.localStorage
// as the storage backend. localStorage is a key-value store with a synchronous
// API and a 5MB storage limit.
//
class QWasmLocalStorageSettingsPrivate final : public QSettingsPrivate
{
public:
    QWasmLocalStorageSettingsPrivate(QSettings::Scope scope, const QString &organization,
                                     const QString &application);
    ~QWasmLocalStorageSettingsPrivate() final = default;

    void remove(const QString &key) final;
    void set(const QString &key, const QVariant &value) final;
    std::optional<QVariant> get(const QString &key) const final;
    QStringList children(const QString &prefix, ChildSpec spec) const final;
    void clear() final;
    void sync() final;
    void flush() final;
    bool isWritable() const final;
    QString fileName() const final;

private:
    QStringList m_keyPrefixes;
};

QWasmLocalStorageSettingsPrivate::QWasmLocalStorageSettingsPrivate(QSettings::Scope scope,
                                                                   const QString &organization,
                                                                   const QString &application)
    : QSettingsPrivate(QSettings::NativeFormat, scope, organization, application)
{
    if (organization.isEmpty()) {
        setStatus(QSettings::AccessError);
        return;
    }

    // The key prefix contians "qt" to separate Qt keys from other keys on localStorage, a
    // version tag to allow for making changes to the key format in the future, the org
    // and app names.
    //
    // User code could could create separate settings object with different org and app names,
    // and would expect them to have separate settings. Also, different webassembly instances
    // on the page could write to the same window.localStorage. Add the org and app name
    // to the key prefix to differentiate, even if that leads to keys with redundant sections
    // for the common case of a single org and app name.
    //
    // Also, the common Qt mechanism for user/system scope and all-application settings are
    // implemented, using different prefixes.
    const QString allAppsSetting = QStringLiteral("all-apps");
    const QString systemSetting = QStringLiteral("sys-tem");

    const QLatin1String separator("-");
    const QLatin1String doubleSeparator("--");
    const QString escapedOrganization = QString(organization).replace(separator, doubleSeparator);
    const QString escapedApplication = QString(application).replace(separator, doubleSeparator);
    const QString prefix = "qt-v0-" + escapedOrganization + separator;
    if (scope == QSettings::Scope::UserScope) {
        if (!escapedApplication.isEmpty())
            m_keyPrefixes.push_back(prefix + escapedApplication + separator);
        m_keyPrefixes.push_back(prefix + allAppsSetting + separator);
    }
    if (!escapedApplication.isEmpty()) {
        m_keyPrefixes.push_back(prefix + escapedApplication + separator + systemSetting
                                + separator);
    }
    m_keyPrefixes.push_back(prefix + allAppsSetting + separator + systemSetting + separator);
}

void QWasmLocalStorageSettingsPrivate::remove(const QString &key)
{
    const std::string removed = QString(m_keyPrefixes.first() + key).toStdString();

    qwasmglobal::runTaskOnMainThread<void>([this, &removed, &key]() {
        std::vector<std::string> children = { removed };
        const int length = val::global("window")["localStorage"]["length"].as<int>();
        for (int i = 0; i < length; ++i) {
            const QString storedKeyWithPrefix = QString::fromStdString(
                    val::global("window")["localStorage"].call<val>("key", i).as<std::string>());

            const QStringView storedKey = keyNameFromPrefixedStorageName(
                    m_keyPrefixes.first(), QStringView(storedKeyWithPrefix));
            if (storedKey.isEmpty() || !storedKey.startsWith(key))
                continue;

            children.push_back(storedKeyWithPrefix.toStdString());
        }

        for (const auto &child : children)
            val::global("window")["localStorage"].call<val>("removeItem", child);
    });
}

void QWasmLocalStorageSettingsPrivate::set(const QString &key, const QVariant &value)
{
    qwasmglobal::runTaskOnMainThread<void>([this, &key, &value]() {
        const std::string keyString = QString(m_keyPrefixes.first() + key).toStdString();
        const std::string valueString = QSettingsPrivate::variantToString(value).toStdString();
        val::global("window")["localStorage"].call<void>("setItem", keyString, valueString);
    });
}

std::optional<QVariant> QWasmLocalStorageSettingsPrivate::get(const QString &key) const
{
    return qwasmglobal::runTaskOnMainThread<std::optional<QVariant>>(
            [this, &key]() -> std::optional<QVariant> {
                for (const auto &prefix : m_keyPrefixes) {
                    const std::string keyString = QString(prefix + key).toStdString();
                    const emscripten::val value =
                            val::global("window")["localStorage"].call<val>("getItem", keyString);
                    if (!value.isNull()) {
                        return QSettingsPrivate::stringToVariant(
                                QString::fromStdString(value.as<std::string>()));
                    }
                    if (!fallbacks) {
                        return std::nullopt;
                    }
                }
                return std::nullopt;
            });
}

QStringList QWasmLocalStorageSettingsPrivate::children(const QString &prefix, ChildSpec spec) const
{
    return qwasmglobal::runTaskOnMainThread<QStringList>([this, &prefix, &spec]() -> QStringList {
        QSet<QString> nodes;
        // Loop through all keys on window.localStorage, return Qt keys belonging to
        // this application, with the correct prefix, and according to ChildSpec.
        QStringList children;
        const int length = val::global("window")["localStorage"]["length"].as<int>();
        for (int i = 0; i < length; ++i) {
            for (const auto &storagePrefix : m_keyPrefixes) {
                const QString keyString =
                        QString::fromStdString(val::global("window")["localStorage"]
                                                       .call<val>("key", i)
                                                       .as<std::string>());

                const QStringView key =
                        keyNameFromPrefixedStorageName(storagePrefix, QStringView(keyString));
                if (!key.isEmpty() && key.startsWith(prefix)) {
                    QStringList children;
                    QSettingsPrivate::processChild(key.sliced(prefix.length()), spec, children);
                    if (!children.isEmpty())
                        nodes.insert(children.first());
                }
                if (!fallbacks)
                    break;
            }
        }

        return QStringList(nodes.begin(), nodes.end());
    });
}

void QWasmLocalStorageSettingsPrivate::clear()
{
    qwasmglobal::runTaskOnMainThread<void>([this]() {
        // Get all Qt keys from window.localStorage
        const int length = val::global("window")["localStorage"]["length"].as<int>();
        QStringList keys;
        keys.reserve(length);
        for (int i = 0; i < length; ++i)
            keys.append(QString::fromStdString(
                    (val::global("window")["localStorage"].call<val>("key", i).as<std::string>())));

        // Remove all Qt keys. Note that localStorage does not guarantee a stable
        // iteration order when the storage is mutated, which is why removal is done
        // in a second step after getting all keys.
        for (const QString &key : keys) {
            if (!keyNameFromPrefixedStorageName(m_keyPrefixes.first(), key).isEmpty())
                val::global("window")["localStorage"].call<val>("removeItem", key.toStdString());
        }
    });
}

void QWasmLocalStorageSettingsPrivate::sync() { }

void QWasmLocalStorageSettingsPrivate::flush() { }

bool QWasmLocalStorageSettingsPrivate::isWritable() const
{
    return true;
}

QString QWasmLocalStorageSettingsPrivate::fileName() const
{
    return QString();
}

//
// Native settings implementation for WebAssembly using the indexed database as
// the storage backend
//
class QWasmIDBSettingsPrivate : public QConfFileSettingsPrivate
{
public:
    QWasmIDBSettingsPrivate(QSettings::Scope scope, const QString &organization,
                            const QString &application);
    ~QWasmIDBSettingsPrivate();

    void clear() override;
    void sync() override;

private:
    bool writeSettingsToTemporaryFile(const QString &fileName, void *dataPtr, int size);
    void loadIndexedDBFiles();


    QString databaseName;
    QString id;
};

constexpr char DbName[] = "/home/web_user";

QWasmIDBSettingsPrivate::QWasmIDBSettingsPrivate(QSettings::Scope scope,
                                                 const QString &organization,
                                                 const QString &application)
    : QConfFileSettingsPrivate(QSettings::WebIndexedDBFormat, scope, organization, application)
{
    Q_ASSERT_X(qstdweb::haveJspi(), Q_FUNC_INFO, "QWasmIDBSettingsPrivate needs JSPI to work");

    if (organization.isEmpty()) {
        setStatus(QSettings::AccessError);
        return;
    }

    databaseName = organization;
    id = application;

    loadIndexedDBFiles();

    QConfFileSettingsPrivate::initAccess();
}

QWasmIDBSettingsPrivate::~QWasmIDBSettingsPrivate() = default;

bool QWasmIDBSettingsPrivate::writeSettingsToTemporaryFile(const QString &fileName, void *dataPtr,
                                                           int size)
{
    QFile file(fileName);
    QFileInfo fileInfo(fileName);
    QDir dir(fileInfo.path());
    if (!dir.exists())
        dir.mkpath(fileInfo.path());

    if (!file.open(QFile::WriteOnly))
        return false;

    return size == file.write(reinterpret_cast<char *>(dataPtr), size);
}

void QWasmIDBSettingsPrivate::clear()
{
    QConfFileSettingsPrivate::clear();

    int error = 0;
    emscripten_idb_delete(DbName, fileName().toLocal8Bit(), &error);
    setStatus(!!error ? QSettings::AccessError : QSettings::NoError);
}

void QWasmIDBSettingsPrivate::sync()
{
    // Reload the files, in case there were any changes in IndexedDB, and flush them to disk.
    // Thanks to this, QConfFileSettingsPrivate::sync will handle key merging correctly.
    loadIndexedDBFiles();

    QConfFileSettingsPrivate::sync();

    QFile file(fileName());
    if (file.open(QFile::ReadOnly)) {
        QByteArray dataPointer = file.readAll();

        int error = 0;
        emscripten_idb_store(DbName, fileName().toLocal8Bit(),
                             reinterpret_cast<void *>(dataPointer.data()), dataPointer.length(),
                             &error);
        setStatus(!!error ? QSettings::AccessError : QSettings::NoError);
    }
}

void QWasmIDBSettingsPrivate::loadIndexedDBFiles()
{
    for (const auto *confFile : getConfFiles()) {
        int exists = 0;
        int error = 0;
        emscripten_idb_exists(DbName, confFile->name.toLocal8Bit(), &exists, &error);
        if (error) {
            setStatus(QSettings::AccessError);
            return;
        }
        if (exists) {
            void *contents;
            int size;
            emscripten_idb_load(DbName, confFile->name.toLocal8Bit(), &contents, &size, &error);
            if (error || !writeSettingsToTemporaryFile(confFile->name, contents, size)) {
                setStatus(QSettings::AccessError);
                return;
            }
        }
    }
}

QSettingsPrivate *QSettingsPrivate::create(QSettings::Format format, QSettings::Scope scope,
                                           const QString &organization, const QString &application)
{
    // Make WebLocalStorageFormat the default native format
    if (format == QSettings::NativeFormat)
        format = QSettings::WebLocalStorageFormat;

    // Check if cookies are enabled (required for using persistent storage)

    const bool cookiesEnabled = qwasmglobal::runTaskOnMainThread<bool>(
            []() { return val::global("navigator")["cookieEnabled"].as<bool>(); });

    constexpr QLatin1StringView cookiesWarningMessage(
            "QSettings::%1 requires cookies, falling back to IniFormat with temporary file");
    if (!cookiesEnabled) {
        if (format == QSettings::WebLocalStorageFormat) {
            qWarning() << cookiesWarningMessage.arg("WebLocalStorageFormat");
            format = QSettings::IniFormat;
        } else if (format == QSettings::WebIndexedDBFormat) {
            qWarning() << cookiesWarningMessage.arg("WebIndexedDBFormat");
            format = QSettings::IniFormat;
        }
    }
    if (format == QSettings::WebIndexedDBFormat && !qstdweb::haveJspi()) {
        qWarning() << "QSettings::WebIndexedDBFormat requires JSPI, falling back to IniFormat with "
                      "temporary file";
        format = QSettings::IniFormat;
    }

    // Create settings backend according to selected format
    switch (format) {
    case QSettings::Format::WebLocalStorageFormat:
        return new QWasmLocalStorageSettingsPrivate(scope, organization, application);
    case QSettings::Format::WebIndexedDBFormat:
        return new QWasmIDBSettingsPrivate(scope, organization, application);
    case QSettings::Format::IniFormat:
    case QSettings::Format::CustomFormat1:
    case QSettings::Format::CustomFormat2:
    case QSettings::Format::CustomFormat3:
    case QSettings::Format::CustomFormat4:
    case QSettings::Format::CustomFormat5:
    case QSettings::Format::CustomFormat6:
    case QSettings::Format::CustomFormat7:
    case QSettings::Format::CustomFormat8:
    case QSettings::Format::CustomFormat9:
    case QSettings::Format::CustomFormat10:
    case QSettings::Format::CustomFormat11:
    case QSettings::Format::CustomFormat12:
    case QSettings::Format::CustomFormat13:
    case QSettings::Format::CustomFormat14:
    case QSettings::Format::CustomFormat15:
    case QSettings::Format::CustomFormat16:
        return new QConfFileSettingsPrivate(format, scope, organization, application);
    case QSettings::Format::InvalidFormat:
        return nullptr;
    case QSettings::Format::NativeFormat:
        Q_UNREACHABLE();
        break;
    }
}

QT_END_NAMESPACE
#endif // QT_NO_SETTINGS
