// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qdbuslistener_p.h"
#include "qdbussettings_p.h"
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>
#include <private/qdbustrayicon_p.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>

QT_BEGIN_NAMESPACE
using namespace Qt::StringLiterals;
Q_STATIC_LOGGING_CATEGORY(lcQpaThemeDBus, "qt.qpa.theme.dbus")

/*!
    \internal
    The QDBusListener class listens to the SettingChanged DBus signal
    and translates it into combinations of the enums \c Provider and \c Setting.
    Upon construction, it logs success/failure of the DBus connection.

    The signal settingChanged delivers the normalized setting type and the new value as a string.
    It is emitted on known setting types only.
 */
QDBusListener::QDBusListener(const QString &service,
                               const QString &path, const QString &interface, const QString &signal)
{
    init (service, path, interface, signal);
}

QDBusListener::QDBusListener()
{
    const auto service = u""_s;
    const auto path = u"/org/freedesktop/portal/desktop"_s;
    const auto interface = u"org.freedesktop.portal.Settings"_s;
    const auto signal = u"SettingChanged"_s;

    init (service, path, interface, signal);
}

namespace {
namespace JsonKeys {
constexpr auto dbusLocation() { return "DBusLocation"_L1; }
constexpr auto dbusKey() { return "DBusKey"_L1; }
constexpr auto provider() { return "Provider"_L1; }
constexpr auto setting() { return "Setting"_L1; }
constexpr auto dbusSignals() { return "DbusSignals"_L1; }
constexpr auto root() { return "Q_L1.qpa.DBusSignals"_L1; }
} // namespace JsonKeys
} // namespace

void QDBusListener::init(const QString &service, const QString &path,
          const QString &interface, const QString &signal)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    const bool dBusRunning = dbus.isConnected();
    bool dBusSignalConnected = false;
#define LOG service << path << interface << signal;

    if (dBusRunning) {
        populateSignalMap();
        qRegisterMetaType<QDBusVariant>();
        dBusSignalConnected = dbus.connect(service, path, interface, signal, this,
                              SLOT(onSettingChanged(QString,QString,QDBusVariant)));
    }

    if (dBusSignalConnected) {
        // Connection successful
        qCDebug(lcQpaThemeDBus) << LOG;
    } else {
        if (dBusRunning) {
            // DBus running, but connection failed
            qCWarning(lcQpaThemeDBus) << "DBus connection failed:" << LOG;
        } else {
            // DBus not running
            qCWarning(lcQpaThemeDBus) << "Session DBus not running.";
        }
        qCWarning(lcQpaThemeDBus) << "Application will not react to setting changes.\n"
                                  << "Check your DBus installation.";
    }
#undef LOG
}

void QDBusListener::loadJson(const QString &fileName)
{
    Q_ASSERT(!fileName.isEmpty());
#define CHECK(cond, warning)\
    if (!cond) {\
        qCWarning(lcQpaThemeDBus) << fileName << warning << "Falling back to default.";\
        return;\
    }

#define PARSE(var, enumeration, string)\
    enumeration var;\
    {\
        bool success;\
        const int val = QMetaEnum::fromType<enumeration>().keyToValue(string.toLatin1(), &success);\
        CHECK(success, "Parse Error: Invalid value" << string << "for" << #var);\
        var = static_cast<enumeration>(val);\
    }

    QFile file(fileName);
    CHECK(file.exists(), fileName << "doesn't exist.");
    CHECK(file.open(QIODevice::ReadOnly), "could not be opened for reading.");

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    CHECK((error.error == QJsonParseError::NoError), error.errorString());
    CHECK(doc.isObject(), "Parse Error: Expected root object" << JsonKeys::root());

    const QJsonObject &root = doc.object();
    CHECK(root.contains(JsonKeys::root()), "Parse Error: Expectned root object" << JsonKeys::root());
    CHECK(root[JsonKeys::root()][JsonKeys::dbusSignals()].isArray(), "Parse Error: Expected array" << JsonKeys::dbusSignals());

    const QJsonArray &sigs = root[JsonKeys::root()][JsonKeys::dbusSignals()].toArray();
    CHECK((sigs.count() > 0), "Parse Error: Found empty array" << JsonKeys::dbusSignals());

    for (auto sig = sigs.constBegin(); sig != sigs.constEnd(); ++sig) {
        CHECK(sig->isObject(), "Parse Error: Expected object array" << JsonKeys::dbusSignals());
        const QJsonObject &obj = sig->toObject();
        CHECK(obj.contains(JsonKeys::dbusLocation()), "Parse Error: Expected key" << JsonKeys::dbusLocation());
        CHECK(obj.contains(JsonKeys::dbusKey()), "Parse Error: Expected key" << JsonKeys::dbusKey());
        CHECK(obj.contains(JsonKeys::provider()), "Parse Error: Expected key" << JsonKeys::provider());
        CHECK(obj.contains(JsonKeys::setting()), "Parse Error: Expected key" << JsonKeys::setting());
        const QString &location = obj[JsonKeys::dbusLocation()].toString();
        const QString &key = obj[JsonKeys::dbusKey()].toString();
        const QString &providerString = obj[JsonKeys::provider()].toString();
        const QString &settingString = obj[JsonKeys::setting()].toString();
        PARSE(provider, Provider, providerString);
        PARSE(setting, Setting, settingString);
        const DBusKey dkey(location, key);
        CHECK (!m_signalMap.contains(dkey), "Duplicate key" << location << key);
        m_signalMap.insert(dkey, ChangeSignal(provider, setting));
    }
#undef PARSE
#undef CHECK

    if (m_signalMap.count() > 0)
        qCInfo(lcQpaThemeDBus) << "Successfully imported" << fileName;
    else
        qCWarning(lcQpaThemeDBus) << "No data imported from" << fileName << "falling back to default.";

#ifdef QT_DEBUG
    const int count = m_signalMap.count();
    if (count == 0)
        return;

    qCDebug(lcQpaThemeDBus) << "Listening to" << count << "signals:";
    for (auto it = m_signalMap.constBegin(); it != m_signalMap.constEnd(); ++it) {
        qDebug() << it.key().key << it.key().location << "mapped to"
                 << it.value().provider << it.value().setting;
    }

#endif
}

void QDBusListener::saveJson(const QString &fileName) const
{
    Q_ASSERT(!m_signalMap.isEmpty());
    Q_ASSERT(!fileName.isEmpty());
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(lcQpaThemeDBus) << fileName << "could not be opened for writing.";
        return;
    }

    QJsonArray sigs;
    for (auto sig = m_signalMap.constBegin(); sig != m_signalMap.constEnd(); ++sig) {
        const DBusKey &dkey = sig.key();
        const ChangeSignal &csig = sig.value();
        QJsonObject obj;
        obj[JsonKeys::dbusLocation()] = dkey.location;
        obj[JsonKeys::dbusKey()] = dkey.key;
        obj[JsonKeys::provider()] = QLatin1StringView(QMetaEnum::fromType<Provider>()
                                            .valueToKey(static_cast<int>(csig.provider)));
        obj[JsonKeys::setting()] = QLatin1StringView(QMetaEnum::fromType<Setting>()
                                           .valueToKey(static_cast<int>(csig.setting)));
        sigs.append(obj);
    }
    QJsonObject obj;
    obj[JsonKeys::dbusSignals()] = sigs;
    QJsonObject root;
    root[JsonKeys::root()] = obj;
    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
}

void QDBusListener::populateSignalMap()
{
    m_signalMap.clear();
    const QString &loadJsonFile = qEnvironmentVariable("QT_QPA_DBUS_SIGNALS");
    if (!loadJsonFile.isEmpty())
        loadJson(loadJsonFile);
    if (!m_signalMap.isEmpty())
        return;

    m_signalMap.insert(DBusKey("org.kde.kdeglobals.KDE"_L1, "widgetStyle"_L1),
                       ChangeSignal(Provider::Kde, Setting::ApplicationStyle));

    m_signalMap.insert(DBusKey("org.kde.kdeglobals.General"_L1, "ColorScheme"_L1),
                       ChangeSignal(Provider::Kde, Setting::Theme));

    m_signalMap.insert(DBusKey("org.gnome.desktop.interface"_L1, "gtk-theme"_L1),
                       ChangeSignal(Provider::Gtk, Setting::Theme));

    using namespace QDBusSettings;
    m_signalMap.insert(DBusKey(XdgSettings::AppearanceNamespace, XdgSettings::ColorSchemeKey),
                       ChangeSignal(Provider::Gnome, Setting::ColorScheme));

    m_signalMap.insert(DBusKey(XdgSettings::AppearanceNamespace, XdgSettings::ContrastKey),
                       ChangeSignal(Provider::Gnome, Setting::Contrast));
    // Alternative solution if XDG desktop portal setting is not accessible,
    // e.g. when using the XDG portal version 1.
    m_signalMap.insert(DBusKey(GnomeSettings::AllyNamespace, GnomeSettings::ContrastKey),
                       ChangeSignal(Provider::Gnome, Setting::Contrast));

    const QString &saveJsonFile = qEnvironmentVariable("QT_QPA_DBUS_SIGNALS_SAVE");
    if (!saveJsonFile.isEmpty())
        saveJson(saveJsonFile);
}

std::optional<QDBusListener::ChangeSignal>
    QDBusListener::findSignal(const QString &location, const QString &key) const
{
    const DBusKey dkey(location, key);
    std::optional<QDBusListener::ChangeSignal> ret;
    const auto it = m_signalMap.find(dkey);
    if (it != m_signalMap.cend())
        ret.emplace(it.value());

    return ret;
}

void QDBusListener::onSettingChanged(const QString &location, const QString &key, const QDBusVariant &value)
{
    auto sig = findSignal(location, key);
    if (!sig.has_value())
        return;

    const Setting setting = sig.value().setting;
    QVariant settingValue = value.variant();

    switch (setting) {
    case Setting::ColorScheme:
        settingValue.setValue(QDBusSettings::XdgSettings::convertColorScheme(settingValue));
        break;
    case Setting::Contrast:
        using namespace QDBusSettings;
        // To unify the value, it's necessary to convert the DBus value to Qt::ContrastPreference.
        // Then the users of the value don't need to parse the raw value.
        if (key == XdgSettings::ContrastKey)
            settingValue.setValue(XdgSettings::convertContrastPreference(settingValue));
        else if (key == GnomeSettings::ContrastKey)
            settingValue.setValue(GnomeSettings::convertContrastPreference(settingValue));
        else
            Q_UNREACHABLE_IMPL();
        break;
    default:
        break;
    }

    emit settingChanged(sig.value().provider, setting, settingValue);
}
QT_END_NAMESPACE
