// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgenericunixthemes_p.h"

#include <QPalette>
#include <QFont>
#include <QGuiApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QHash>
#include <QLoggingCategory>
#include <QVariant>
#include <QStandardPaths>
#include <QStringList>
#if QT_CONFIG(mimetype)
#include <QMimeDatabase>
#endif
#if QT_CONFIG(settings)
#include <QSettings>
#endif

#include <qpa/qplatformfontdatabase.h> // lcQpaFonts
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>
#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformtheme_p.h>

#include <private/qguiapplication_p.h>
#ifndef QT_NO_DBUS
#include <QDBusConnectionInterface>
#include <private/qdbusplatformmenu_p.h>
#include <private/qdbusmenubar_p.h>
#endif
#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
#include <private/qdbustrayicon_p.h>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE
#ifndef QT_NO_DBUS
Q_LOGGING_CATEGORY(lcQpaThemeDBus, "qt.qpa.theme.dbus")
#endif

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(qLcTray)

ResourceHelper::ResourceHelper()
{
    std::fill(palettes, palettes + QPlatformTheme::NPalettes, static_cast<QPalette *>(nullptr));
    std::fill(fonts, fonts + QPlatformTheme::NFonts, static_cast<QFont *>(nullptr));
}

void ResourceHelper::clear()
{
    qDeleteAll(palettes, palettes + QPlatformTheme::NPalettes);
    qDeleteAll(fonts, fonts + QPlatformTheme::NFonts);
    std::fill(palettes, palettes + QPlatformTheme::NPalettes, static_cast<QPalette *>(nullptr));
    std::fill(fonts, fonts + QPlatformTheme::NFonts, static_cast<QFont *>(nullptr));
}

const char *QGenericUnixTheme::name = "generic";

// Default system font, corresponding to the value returned by 4.8 for
// XRender/FontConfig which we can now assume as default.
static const char defaultSystemFontNameC[] = "Sans Serif";
static const char defaultFixedFontNameC[] = "monospace";
enum { defaultSystemFontSize = 9 };

#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
static bool isDBusTrayAvailable() {
    static bool dbusTrayAvailable = false;
    static bool dbusTrayAvailableKnown = false;
    if (!dbusTrayAvailableKnown) {
        QDBusMenuConnection conn;
        if (conn.isStatusNotifierHostRegistered())
            dbusTrayAvailable = true;
        dbusTrayAvailableKnown = true;
        qCDebug(qLcTray) << "D-Bus tray available:" << dbusTrayAvailable;
    }
    return dbusTrayAvailable;
}
#endif

#ifndef QT_NO_DBUS
static bool checkDBusGlobalMenuAvailable()
{
    const QDBusConnection connection = QDBusConnection::sessionBus();
    static const QString registrarService = QStringLiteral("com.canonical.AppMenu.Registrar");
    if (const auto iface = connection.interface())
        return iface->isServiceRegistered(registrarService);
    return false;
}

static bool isDBusGlobalMenuAvailable()
{
    static bool dbusGlobalMenuAvailable = checkDBusGlobalMenuAvailable();
    return dbusGlobalMenuAvailable;
}

/*!
 * \internal
 * The QGenericUnixThemeDBusListener class listens to the SettingChanged DBus signal
 * and translates it into the QDbusSettingType enum.
 * Upon construction, it logs success/failure of the DBus connection.
 *
 * The signal settingChanged delivers the normalized setting type and the new value as a string.
 * It is emitted on known setting types only.
 */

class QGenericUnixThemeDBusListener : public QObject
{
    Q_OBJECT

public:
    QGenericUnixThemeDBusListener(const QString &service, const QString &path, const QString &interface, const QString &signal);

    enum class SettingType {
        KdeGlobalTheme,
        KdeApplicationStyle,
        Unknown
    };
    Q_ENUM(SettingType)

    static SettingType toSettingType(const QString &location, const QString &key);

private Q_SLOTS:
    void onSettingChanged(const QString &location, const QString &key, const QDBusVariant &value);

Q_SIGNALS:
    void settingChanged(QGenericUnixThemeDBusListener::SettingType type, const QString &value);

};

QGenericUnixThemeDBusListener::QGenericUnixThemeDBusListener(const QString &service,
                               const QString &path, const QString &interface, const QString &signal)
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    const bool dBusRunning = dbus.isConnected();
    bool dBusSignalConnected = false;
#define LOG service << path << interface << signal;

    if (dBusRunning) {
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
        qCWarning(lcQpaThemeDBus) << "Application will not react to KDE setting changes.\n"
                             << "Check your DBus installation.";
    }
#undef LOG
}

QGenericUnixThemeDBusListener::SettingType QGenericUnixThemeDBusListener::toSettingType(
        const QString &location, const QString &key)
{
    if (location == QLatin1StringView("org.kde.kdeglobals.KDE")
          && key == QLatin1StringView("widgetStyle"))
        return SettingType::KdeApplicationStyle;
    if (location == QLatin1StringView("org.kde.kdeglobals.General")
          && key == QLatin1StringView("ColorScheme"))
        return SettingType::KdeGlobalTheme;
    return SettingType::Unknown;
}

void QGenericUnixThemeDBusListener::onSettingChanged(const QString &location, const QString &key, const QDBusVariant &value)
{
    const SettingType type = toSettingType(location, key);
    if (type != SettingType::Unknown)
        emit settingChanged(type, value.variant().toString());
}

#endif //QT_NO_DBUS

class QGenericUnixThemePrivate : public QPlatformThemePrivate
{
public:
    QGenericUnixThemePrivate()
        : QPlatformThemePrivate()
        , systemFont(QLatin1StringView(defaultSystemFontNameC), defaultSystemFontSize)
        , fixedFont(QLatin1StringView(defaultFixedFontNameC), systemFont.pointSize())
    {
        fixedFont.setStyleHint(QFont::TypeWriter);
        qCDebug(lcQpaFonts) << "default fonts: system" << systemFont << "fixed" << fixedFont;
    }

    const QFont systemFont;
    QFont fixedFont;
};

QGenericUnixTheme::QGenericUnixTheme()
    : QPlatformTheme(new QGenericUnixThemePrivate())
{
}

const QFont *QGenericUnixTheme::font(Font type) const
{
    Q_D(const QGenericUnixTheme);
    switch (type) {
    case QPlatformTheme::SystemFont:
        return &d->systemFont;
    case QPlatformTheme::FixedFont:
        return &d->fixedFont;
    default:
        return nullptr;
    }
}

// Helper to return the icon theme paths from XDG.
QStringList QGenericUnixTheme::xdgIconThemePaths()
{
    QStringList paths;
    // Add home directory first in search path
    const QFileInfo homeIconDir(QDir::homePath() + "/.icons"_L1);
    if (homeIconDir.isDir())
        paths.prepend(homeIconDir.absoluteFilePath());

    paths.append(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                           QStringLiteral("icons"),
                                           QStandardPaths::LocateDirectory));

    return paths;
}

QStringList QGenericUnixTheme::iconFallbackPaths()
{
    QStringList paths;
    const QFileInfo pixmapsIconsDir(QStringLiteral("/usr/share/pixmaps"));
    if (pixmapsIconsDir.isDir())
        paths.append(pixmapsIconsDir.absoluteFilePath());

    return paths;
}

#ifndef QT_NO_DBUS
QPlatformMenuBar *QGenericUnixTheme::createPlatformMenuBar() const
{
    if (isDBusGlobalMenuAvailable())
        return new QDBusMenuBar();
    return nullptr;
}
#endif

#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
QPlatformSystemTrayIcon *QGenericUnixTheme::createPlatformSystemTrayIcon() const
{
    if (isDBusTrayAvailable())
        return new QDBusTrayIcon();
    return nullptr;
}
#endif

QVariant QGenericUnixTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(QString(QStringLiteral("hicolor")));
    case QPlatformTheme::IconThemeSearchPaths:
        return xdgIconThemePaths();
    case QPlatformTheme::IconFallbackSearchPaths:
        return iconFallbackPaths();
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(true);
    case QPlatformTheme::StyleNames: {
        QStringList styleNames;
        styleNames << QStringLiteral("Fusion") << QStringLiteral("Windows");
        return QVariant(styleNames);
    }
    case QPlatformTheme::KeyboardScheme:
        return QVariant(int(X11KeyboardScheme));
    case QPlatformTheme::UiEffects:
        return QVariant(int(HoverEffect));
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

// Helper functions for implementing QPlatformTheme::fileIcon() for XDG icon themes.
static QList<QSize> availableXdgFileIconSizes()
{
    return QIcon::fromTheme(QStringLiteral("inode-directory")).availableSizes();
}

#if QT_CONFIG(mimetype)
static QIcon xdgFileIcon(const QFileInfo &fileInfo)
{
    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo);
    if (!mimeType.isValid())
        return QIcon();
    const QString &iconName = mimeType.iconName();
    if (!iconName.isEmpty()) {
        const QIcon icon = QIcon::fromTheme(iconName);
        if (!icon.isNull())
            return icon;
    }
    const QString &genericIconName = mimeType.genericIconName();
    return genericIconName.isEmpty() ? QIcon() : QIcon::fromTheme(genericIconName);
}
#endif

#if QT_CONFIG(settings)
class QKdeThemePrivate : public QPlatformThemePrivate
{

public:
    QKdeThemePrivate(const QStringList &kdeDirs, int kdeVersion);

    static QString kdeGlobals(const QString &kdeDir, int kdeVersion)
    {
        if (kdeVersion > 4)
            return kdeDir + "/kdeglobals"_L1;
        return kdeDir + "/share/config/kdeglobals"_L1;
    }

    void refresh();
    static QVariant readKdeSetting(const QString &key, const QStringList &kdeDirs, int kdeVersion, QHash<QString, QSettings*> &kdeSettings);
    static void readKdeSystemPalette(const QStringList &kdeDirs, int kdeVersion, QHash<QString, QSettings*> &kdeSettings, QPalette *pal);
    static QFont *kdeFont(const QVariant &fontValue);
    static QStringList kdeIconThemeSearchPaths(const QStringList &kdeDirs);

    const QStringList kdeDirs;
    const int kdeVersion;

    ResourceHelper resources;
    QString iconThemeName;
    QString iconFallbackThemeName;
    QStringList styleNames;
    int toolButtonStyle = Qt::ToolButtonTextBesideIcon;
    int toolBarIconSize = 0;
    bool singleClick = true;
    bool showIconsOnPushButtons = true;
    int wheelScrollLines = 3;
    int doubleClickInterval = 400;
    int startDragDist = 10;
    int startDragTime = 500;
    int cursorBlinkRate = 1000;

#ifndef QT_NO_DBUS
private:
    std::unique_ptr<QGenericUnixThemeDBusListener> dbus;
    bool initDbus();
    void settingChangedHandler(QGenericUnixThemeDBusListener::SettingType type, const QString &value);
#endif // QT_NO_DBUS
};

#ifndef QT_NO_DBUS
void QKdeThemePrivate::settingChangedHandler(QGenericUnixThemeDBusListener::SettingType type, const QString &value)
{
    switch (type) {
    case QGenericUnixThemeDBusListener::SettingType::KdeGlobalTheme:
        qCDebug(lcQpaThemeDBus) << "KDE global theme changed to:" << value;
        break;
    case QGenericUnixThemeDBusListener::SettingType::KdeApplicationStyle:
        qCDebug(lcQpaThemeDBus) << "KDE application style changed to:" << value;
        break;
    case QGenericUnixThemeDBusListener::SettingType::Unknown:
        Q_UNREACHABLE();
    }

    refresh();
}

bool QKdeThemePrivate::initDbus()
{
    constexpr QLatin1StringView service("");
    constexpr QLatin1StringView path("/org/freedesktop/portal/desktop");
    constexpr QLatin1StringView interface("org.freedesktop.portal.Settings");
    constexpr QLatin1StringView signal("SettingChanged");

    dbus.reset(new QGenericUnixThemeDBusListener(service, path, interface, signal));
    Q_ASSERT(dbus);

    // Wrap slot in a lambda to avoid inheriting QKdeThemePrivate from QObject
    auto wrapper = [this](QGenericUnixThemeDBusListener::SettingType type, const QString &value) {
        settingChangedHandler(type, value);
    };

    return QObject::connect(dbus.get(), &QGenericUnixThemeDBusListener::settingChanged, wrapper);
}
#endif // QT_NO_DBUS

QKdeThemePrivate::QKdeThemePrivate(const QStringList &kdeDirs, int kdeVersion)
    : kdeDirs(kdeDirs), kdeVersion(kdeVersion)
{
#ifndef QT_NO_DBUS
    initDbus();
#endif // QT_NO_DBUS
}

void QKdeThemePrivate::refresh()
{
    resources.clear();

    toolButtonStyle = Qt::ToolButtonTextBesideIcon;
    toolBarIconSize = 0;
    styleNames.clear();
    if (kdeVersion >= 5)
        styleNames << QStringLiteral("breeze");
    styleNames << QStringLiteral("Oxygen") << QStringLiteral("Fusion") << QStringLiteral("windows");
    if (kdeVersion >= 5)
        iconFallbackThemeName = iconThemeName = QStringLiteral("breeze");
    else
        iconFallbackThemeName = iconThemeName = QStringLiteral("oxygen");

    QHash<QString, QSettings*> kdeSettings;

    QPalette systemPalette = QPalette();
    readKdeSystemPalette(kdeDirs, kdeVersion, kdeSettings, &systemPalette);
    resources.palettes[QPlatformTheme::SystemPalette] = new QPalette(systemPalette);
    //## TODO tooltip color

    const QVariant styleValue = readKdeSetting(QStringLiteral("widgetStyle"), kdeDirs, kdeVersion, kdeSettings);
    if (styleValue.isValid()) {
        const QString style = styleValue.toString();
        if (style != styleNames.front())
            styleNames.push_front(style);
    }

    const QVariant singleClickValue = readKdeSetting(QStringLiteral("KDE/SingleClick"), kdeDirs, kdeVersion, kdeSettings);
    if (singleClickValue.isValid())
        singleClick = singleClickValue.toBool();

    const QVariant showIconsOnPushButtonsValue = readKdeSetting(QStringLiteral("KDE/ShowIconsOnPushButtons"), kdeDirs, kdeVersion, kdeSettings);
    if (showIconsOnPushButtonsValue.isValid())
        showIconsOnPushButtons = showIconsOnPushButtonsValue.toBool();

    const QVariant themeValue = readKdeSetting(QStringLiteral("Icons/Theme"), kdeDirs, kdeVersion, kdeSettings);
    if (themeValue.isValid())
        iconThemeName = themeValue.toString();

    const QVariant toolBarIconSizeValue = readKdeSetting(QStringLiteral("ToolbarIcons/Size"), kdeDirs, kdeVersion, kdeSettings);
    if (toolBarIconSizeValue.isValid())
        toolBarIconSize = toolBarIconSizeValue.toInt();

    const QVariant toolbarStyleValue = readKdeSetting(QStringLiteral("Toolbar style/ToolButtonStyle"), kdeDirs, kdeVersion, kdeSettings);
    if (toolbarStyleValue.isValid()) {
        const QString toolBarStyle = toolbarStyleValue.toString();
        if (toolBarStyle == "TextBesideIcon"_L1)
            toolButtonStyle =  Qt::ToolButtonTextBesideIcon;
        else if (toolBarStyle == "TextOnly"_L1)
            toolButtonStyle = Qt::ToolButtonTextOnly;
        else if (toolBarStyle == "TextUnderIcon"_L1)
            toolButtonStyle = Qt::ToolButtonTextUnderIcon;
    }

    const QVariant wheelScrollLinesValue = readKdeSetting(QStringLiteral("KDE/WheelScrollLines"), kdeDirs, kdeVersion, kdeSettings);
    if (wheelScrollLinesValue.isValid())
        wheelScrollLines = wheelScrollLinesValue.toInt();

    const QVariant doubleClickIntervalValue = readKdeSetting(QStringLiteral("KDE/DoubleClickInterval"), kdeDirs, kdeVersion, kdeSettings);
    if (doubleClickIntervalValue.isValid())
        doubleClickInterval = doubleClickIntervalValue.toInt();

    const QVariant startDragDistValue = readKdeSetting(QStringLiteral("KDE/StartDragDist"), kdeDirs, kdeVersion, kdeSettings);
    if (startDragDistValue.isValid())
        startDragDist = startDragDistValue.toInt();

    const QVariant startDragTimeValue = readKdeSetting(QStringLiteral("KDE/StartDragTime"), kdeDirs, kdeVersion, kdeSettings);
    if (startDragTimeValue.isValid())
        startDragTime = startDragTimeValue.toInt();

    const QVariant cursorBlinkRateValue = readKdeSetting(QStringLiteral("KDE/CursorBlinkRate"), kdeDirs, kdeVersion, kdeSettings);
    if (cursorBlinkRateValue.isValid()) {
        cursorBlinkRate = cursorBlinkRateValue.toInt();
        cursorBlinkRate = cursorBlinkRate > 0 ? qBound(200, cursorBlinkRate, 2000) : 0;
    }

    // Read system font, ignore 'smallestReadableFont'
    if (QFont *systemFont = kdeFont(readKdeSetting(QStringLiteral("font"), kdeDirs, kdeVersion, kdeSettings)))
        resources.fonts[QPlatformTheme::SystemFont] = systemFont;
    else
        resources.fonts[QPlatformTheme::SystemFont] = new QFont(QLatin1StringView(defaultSystemFontNameC), defaultSystemFontSize);

    if (QFont *fixedFont = kdeFont(readKdeSetting(QStringLiteral("fixed"), kdeDirs, kdeVersion, kdeSettings))) {
        resources.fonts[QPlatformTheme::FixedFont] = fixedFont;
    } else {
        fixedFont = new QFont(QLatin1StringView(defaultFixedFontNameC), defaultSystemFontSize);
        fixedFont->setStyleHint(QFont::TypeWriter);
        resources.fonts[QPlatformTheme::FixedFont] = fixedFont;
    }

    if (QFont *menuFont = kdeFont(readKdeSetting(QStringLiteral("menuFont"), kdeDirs, kdeVersion, kdeSettings))) {
        resources.fonts[QPlatformTheme::MenuFont] = menuFont;
        resources.fonts[QPlatformTheme::MenuBarFont] = new QFont(*menuFont);
    }

    if (QFont *toolBarFont = kdeFont(readKdeSetting(QStringLiteral("toolBarFont"), kdeDirs, kdeVersion, kdeSettings)))
        resources.fonts[QPlatformTheme::ToolButtonFont] = toolBarFont;

    QWindowSystemInterface::handleThemeChange();

    qCDebug(lcQpaFonts) << "default fonts: system" << resources.fonts[QPlatformTheme::SystemFont]
                        << "fixed" << resources.fonts[QPlatformTheme::FixedFont];
    qDeleteAll(kdeSettings);
}

QVariant QKdeThemePrivate::readKdeSetting(const QString &key, const QStringList &kdeDirs, int kdeVersion, QHash<QString, QSettings*> &kdeSettings)
{
    for (const QString &kdeDir : kdeDirs) {
        QSettings *settings = kdeSettings.value(kdeDir);
        if (!settings) {
            const QString kdeGlobalsPath = kdeGlobals(kdeDir, kdeVersion);
            if (QFileInfo(kdeGlobalsPath).isReadable()) {
                settings = new QSettings(kdeGlobalsPath, QSettings::IniFormat);
                kdeSettings.insert(kdeDir, settings);
            }
        }
        if (settings) {
            const QVariant value = settings->value(key);
            if (value.isValid())
                return value;
        }
    }
    return QVariant();
}

// Reads the color from the KDE configuration, and store it in the
// palette with the given color role if found.
static inline bool kdeColor(QPalette *pal, QPalette::ColorRole role, const QVariant &value)
{
    if (!value.isValid())
        return false;
    const QStringList values = value.toStringList();
    if (values.size() != 3)
        return false;
    pal->setBrush(role, QColor(values.at(0).toInt(), values.at(1).toInt(), values.at(2).toInt()));
    return true;
}

void QKdeThemePrivate::readKdeSystemPalette(const QStringList &kdeDirs, int kdeVersion, QHash<QString, QSettings*> &kdeSettings, QPalette *pal)
{
    if (!kdeColor(pal, QPalette::Button, readKdeSetting(QStringLiteral("Colors:Button/BackgroundNormal"), kdeDirs, kdeVersion, kdeSettings))) {
        // kcolorscheme.cpp: SetDefaultColors
        const QColor defaultWindowBackground(214, 210, 208);
        const QColor defaultButtonBackground(223, 220, 217);
        *pal = QPalette(defaultButtonBackground, defaultWindowBackground);
        return;
    }

    kdeColor(pal, QPalette::Window, readKdeSetting(QStringLiteral("Colors:Window/BackgroundNormal"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::Text, readKdeSetting(QStringLiteral("Colors:View/ForegroundNormal"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::WindowText, readKdeSetting(QStringLiteral("Colors:Window/ForegroundNormal"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::Base, readKdeSetting(QStringLiteral("Colors:View/BackgroundNormal"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::Highlight, readKdeSetting(QStringLiteral("Colors:Selection/BackgroundNormal"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::HighlightedText, readKdeSetting(QStringLiteral("Colors:Selection/ForegroundNormal"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::AlternateBase, readKdeSetting(QStringLiteral("Colors:View/BackgroundAlternate"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::ButtonText, readKdeSetting(QStringLiteral("Colors:Button/ForegroundNormal"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::Link, readKdeSetting(QStringLiteral("Colors:View/ForegroundLink"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::LinkVisited, readKdeSetting(QStringLiteral("Colors:View/ForegroundVisited"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::ToolTipBase, readKdeSetting(QStringLiteral("Colors:Tooltip/BackgroundNormal"), kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::ToolTipText, readKdeSetting(QStringLiteral("Colors:Tooltip/ForegroundNormal"), kdeDirs, kdeVersion, kdeSettings));

    // The above code sets _all_ color roles to "normal" colors. In KDE, the disabled
    // color roles are calculated by applying various effects described in kdeglobals.
    // We use a bit simpler approach here, similar logic than in qt_palette_from_color().
    const QColor button = pal->color(QPalette::Button);
    int h, s, v;
    button.getHsv(&h, &s, &v);

    const QBrush whiteBrush = QBrush(Qt::white);
    const QBrush buttonBrush = QBrush(button);
    const QBrush buttonBrushDark = QBrush(button.darker(v > 128 ? 200 : 50));
    const QBrush buttonBrushDark150 = QBrush(button.darker(v > 128 ? 150 : 75));
    const QBrush buttonBrushLight150 = QBrush(button.lighter(v > 128 ? 150 : 75));
    const QBrush buttonBrushLight = QBrush(button.lighter(v > 128 ? 200 : 50));

    pal->setBrush(QPalette::Disabled, QPalette::WindowText, buttonBrushDark);
    pal->setBrush(QPalette::Disabled, QPalette::ButtonText, buttonBrushDark);
    pal->setBrush(QPalette::Disabled, QPalette::Button, buttonBrush);
    pal->setBrush(QPalette::Disabled, QPalette::Text, buttonBrushDark);
    pal->setBrush(QPalette::Disabled, QPalette::BrightText, whiteBrush);
    pal->setBrush(QPalette::Disabled, QPalette::Base, buttonBrush);
    pal->setBrush(QPalette::Disabled, QPalette::Window, buttonBrush);
    pal->setBrush(QPalette::Disabled, QPalette::Highlight, buttonBrushDark150);
    pal->setBrush(QPalette::Disabled, QPalette::HighlightedText, buttonBrushLight150);

    // set calculated colors for all groups
    pal->setBrush(QPalette::Light, buttonBrushLight);
    pal->setBrush(QPalette::Midlight, buttonBrushLight150);
    pal->setBrush(QPalette::Mid, buttonBrushDark150);
    pal->setBrush(QPalette::Dark, buttonBrushDark);
}

/*!
    \class QKdeTheme
    \brief QKdeTheme is a theme implementation for the KDE desktop (version 4 or higher).
    \since 5.0
    \internal
    \ingroup qpa
*/

const char *QKdeTheme::name = "kde";

QKdeTheme::QKdeTheme(const QStringList& kdeDirs, int kdeVersion)
    : QPlatformTheme(new QKdeThemePrivate(kdeDirs,kdeVersion))
{
    d_func()->refresh();
}

QFont *QKdeThemePrivate::kdeFont(const QVariant &fontValue)
{
    if (fontValue.isValid()) {
        // Read font value: Might be a QStringList as KDE stores fonts without quotes.
        // Also retrieve the family for the constructor since we cannot use the
        // default constructor of QFont, which accesses QGuiApplication::systemFont()
        // causing recursion.
        QString fontDescription;
        QString fontFamily;
        if (fontValue.userType() == QMetaType::QStringList) {
            const QStringList list = fontValue.toStringList();
            if (!list.isEmpty()) {
                fontFamily = list.first();
                fontDescription = list.join(u',');
            }
        } else {
            fontDescription = fontFamily = fontValue.toString();
        }
        if (!fontDescription.isEmpty()) {
            QFont font(fontFamily);
            if (font.fromString(fontDescription))
                return new QFont(font);
        }
    }
    return nullptr;
}


QStringList QKdeThemePrivate::kdeIconThemeSearchPaths(const QStringList &kdeDirs)
{
    QStringList paths = QGenericUnixTheme::xdgIconThemePaths();
    const QString iconPath = QStringLiteral("/share/icons");
    for (const QString &candidate : kdeDirs) {
        const QFileInfo fi(candidate + iconPath);
        if (fi.isDir())
            paths.append(fi.absoluteFilePath());
    }
    return paths;
}

QVariant QKdeTheme::themeHint(QPlatformTheme::ThemeHint hint) const
{
    Q_D(const QKdeTheme);
    switch (hint) {
    case QPlatformTheme::UseFullScreenForPopupMenu:
        return QVariant(true);
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(d->showIconsOnPushButtons);
    case QPlatformTheme::DialogButtonBoxLayout:
        return QVariant(QPlatformDialogHelper::KdeLayout);
    case QPlatformTheme::ToolButtonStyle:
        return QVariant(d->toolButtonStyle);
    case QPlatformTheme::ToolBarIconSize:
        return QVariant(d->toolBarIconSize);
    case QPlatformTheme::SystemIconThemeName:
        return QVariant(d->iconThemeName);
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(d->iconFallbackThemeName);
    case QPlatformTheme::IconThemeSearchPaths:
        return QVariant(d->kdeIconThemeSearchPaths(d->kdeDirs));
    case QPlatformTheme::IconPixmapSizes:
        return QVariant::fromValue(availableXdgFileIconSizes());
    case QPlatformTheme::StyleNames:
        return QVariant(d->styleNames);
    case QPlatformTheme::KeyboardScheme:
        return QVariant(int(KdeKeyboardScheme));
    case QPlatformTheme::ItemViewActivateItemOnSingleClick:
        return QVariant(d->singleClick);
    case QPlatformTheme::WheelScrollLines:
        return QVariant(d->wheelScrollLines);
    case QPlatformTheme::MouseDoubleClickInterval:
        return QVariant(d->doubleClickInterval);
    case QPlatformTheme::StartDragTime:
        return QVariant(d->startDragTime);
    case QPlatformTheme::StartDragDistance:
        return QVariant(d->startDragDist);
    case QPlatformTheme::CursorFlashTime:
        return QVariant(d->cursorBlinkRate);
    case QPlatformTheme::UiEffects:
        return QVariant(int(HoverEffect));
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

QIcon QKdeTheme::fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions) const
{
#if QT_CONFIG(mimetype)
    return xdgFileIcon(fileInfo);
#else
    Q_UNUSED(fileInfo);
    return QIcon();
#endif
}

const QPalette *QKdeTheme::palette(Palette type) const
{
    Q_D(const QKdeTheme);
    return d->resources.palettes[type];
}

const QFont *QKdeTheme::font(Font type) const
{
    Q_D(const QKdeTheme);
    return d->resources.fonts[type];
}

QPlatformTheme *QKdeTheme::createKdeTheme()
{
    const QByteArray kdeVersionBA = qgetenv("KDE_SESSION_VERSION");
    const int kdeVersion = kdeVersionBA.toInt();
    if (kdeVersion < 4)
        return nullptr;

    if (kdeVersion > 4)
        // Plasma 5 follows XDG spec
        // but uses the same config file format:
        return new QKdeTheme(QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation), kdeVersion);

    // Determine KDE prefixes in the following priority order:
    // - KDEHOME and KDEDIRS environment variables
    // - ~/.kde(<version>)
    // - read prefixes from /etc/kde<version>rc
    // - fallback to /etc/kde<version>

    QStringList kdeDirs;
    const QString kdeHomePathVar = QFile::decodeName(qgetenv("KDEHOME"));
    if (!kdeHomePathVar.isEmpty())
        kdeDirs += kdeHomePathVar;

    const QString kdeDirsVar = QFile::decodeName(qgetenv("KDEDIRS"));
    if (!kdeDirsVar.isEmpty())
        kdeDirs += kdeDirsVar.split(u':', Qt::SkipEmptyParts);

    const QString kdeVersionHomePath = QDir::homePath() + "/.kde"_L1 + QLatin1StringView(kdeVersionBA);
    if (QFileInfo(kdeVersionHomePath).isDir())
        kdeDirs += kdeVersionHomePath;

    const QString kdeHomePath = QDir::homePath() + "/.kde"_L1;
    if (QFileInfo(kdeHomePath).isDir())
        kdeDirs += kdeHomePath;

    const QString kdeRcPath = "/etc/kde"_L1 + QLatin1StringView(kdeVersionBA) + "rc"_L1;
    if (QFileInfo(kdeRcPath).isReadable()) {
        QSettings kdeSettings(kdeRcPath, QSettings::IniFormat);
        kdeSettings.beginGroup(QStringLiteral("Directories-default"));
        kdeDirs += kdeSettings.value(QStringLiteral("prefixes")).toStringList();
    }

    const QString kdeVersionPrefix = "/etc/kde"_L1 + QLatin1StringView(kdeVersionBA);
    if (QFileInfo(kdeVersionPrefix).isDir())
        kdeDirs += kdeVersionPrefix;

    kdeDirs.removeDuplicates();
    if (kdeDirs.isEmpty()) {
        qWarning("Unable to determine KDE dirs");
        return nullptr;
    }

    return new QKdeTheme(kdeDirs, kdeVersion);
}

#ifndef QT_NO_DBUS
QPlatformMenuBar *QKdeTheme::createPlatformMenuBar() const
{
    if (isDBusGlobalMenuAvailable())
        return new QDBusMenuBar();
    return nullptr;
}
#endif

#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
QPlatformSystemTrayIcon *QKdeTheme::createPlatformSystemTrayIcon() const
{
    if (isDBusTrayAvailable())
        return new QDBusTrayIcon();
    return nullptr;
}
#endif

#endif // settings

/*!
    \class QGnomeTheme
    \brief QGnomeTheme is a theme implementation for the Gnome desktop.
    \since 5.0
    \internal
    \ingroup qpa
*/

const char *QGnomeTheme::name = "gnome";

class QGnomeThemePrivate : public QPlatformThemePrivate
{
public:
    QGnomeThemePrivate() : systemFont(nullptr), fixedFont(nullptr) {}
    ~QGnomeThemePrivate() { delete systemFont; delete fixedFont; }

    void configureFonts(const QString &gtkFontName) const
    {
        Q_ASSERT(!systemFont);
        const int split = gtkFontName.lastIndexOf(QChar::Space);
        float size = QStringView{gtkFontName}.mid(split + 1).toFloat();
        QString fontName = gtkFontName.left(split);

        systemFont = new QFont(fontName, size);
        fixedFont = new QFont(QLatin1StringView(defaultFixedFontNameC), systemFont->pointSize());
        fixedFont->setStyleHint(QFont::TypeWriter);
        qCDebug(lcQpaFonts) << "default fonts: system" << systemFont << "fixed" << fixedFont;
    }

    mutable QFont *systemFont;
    mutable QFont *fixedFont;
};

QGnomeTheme::QGnomeTheme()
    : QPlatformTheme(new QGnomeThemePrivate())
{
}

QVariant QGnomeTheme::themeHint(QPlatformTheme::ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(true);
    case QPlatformTheme::DialogButtonBoxLayout:
        return QVariant(QPlatformDialogHelper::GnomeLayout);
    case QPlatformTheme::SystemIconThemeName:
        return QVariant(QStringLiteral("Adwaita"));
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(QStringLiteral("gnome"));
    case QPlatformTheme::IconThemeSearchPaths:
        return QVariant(QGenericUnixTheme::xdgIconThemePaths());
    case QPlatformTheme::IconPixmapSizes:
        return QVariant::fromValue(availableXdgFileIconSizes());
    case QPlatformTheme::StyleNames: {
        QStringList styleNames;
        styleNames << QStringLiteral("Fusion") << QStringLiteral("windows");
        return QVariant(styleNames);
    }
    case QPlatformTheme::KeyboardScheme:
        return QVariant(int(GnomeKeyboardScheme));
    case QPlatformTheme::PasswordMaskCharacter:
        return QVariant(QChar(0x2022));
    case QPlatformTheme::UiEffects:
        return QVariant(int(HoverEffect));
    case QPlatformTheme::ButtonPressKeys:
        return QVariant::fromValue(
                QList<Qt::Key>({ Qt::Key_Space, Qt::Key_Return, Qt::Key_Enter, Qt::Key_Select }));
    case QPlatformTheme::PreselectFirstFileInDirectory:
        return true;
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

QIcon QGnomeTheme::fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions) const
{
#if QT_CONFIG(mimetype)
    return xdgFileIcon(fileInfo);
#else
    Q_UNUSED(fileInfo);
    return QIcon();
#endif
}

const QFont *QGnomeTheme::font(Font type) const
{
    Q_D(const QGnomeTheme);
    if (!d->systemFont)
        d->configureFonts(gtkFontName());
    switch (type) {
    case QPlatformTheme::SystemFont:
        return d->systemFont;
    case QPlatformTheme::FixedFont:
        return d->fixedFont;
    default:
        return nullptr;
    }
}

QString QGnomeTheme::gtkFontName() const
{
    return QStringLiteral("%1 %2").arg(QLatin1StringView(defaultSystemFontNameC)).arg(defaultSystemFontSize);
}

#ifndef QT_NO_DBUS
QPlatformMenuBar *QGnomeTheme::createPlatformMenuBar() const
{
    if (isDBusGlobalMenuAvailable())
        return new QDBusMenuBar();
    return nullptr;
}
#endif

#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
QPlatformSystemTrayIcon *QGnomeTheme::createPlatformSystemTrayIcon() const
{
    if (isDBusTrayAvailable())
        return new QDBusTrayIcon();
    return nullptr;
}
#endif

QString QGnomeTheme::standardButtonText(int button) const
{
    switch (button) {
    case QPlatformDialogHelper::Ok:
        return QCoreApplication::translate("QGnomeTheme", "&OK");
    case QPlatformDialogHelper::Save:
        return QCoreApplication::translate("QGnomeTheme", "&Save");
    case QPlatformDialogHelper::Cancel:
        return QCoreApplication::translate("QGnomeTheme", "&Cancel");
    case QPlatformDialogHelper::Close:
        return QCoreApplication::translate("QGnomeTheme", "&Close");
    case QPlatformDialogHelper::Discard:
        return QCoreApplication::translate("QGnomeTheme", "Close without Saving");
    default:
        break;
    }
    return QPlatformTheme::standardButtonText(button);
}

/*!
    \brief Creates a UNIX theme according to the detected desktop environment.
*/

QPlatformTheme *QGenericUnixTheme::createUnixTheme(const QString &name)
{
    if (name == QLatin1StringView(QGenericUnixTheme::name))
        return new QGenericUnixTheme;
#if QT_CONFIG(settings)
    if (name == QLatin1StringView(QKdeTheme::name))
        if (QPlatformTheme *kdeTheme = QKdeTheme::createKdeTheme())
            return kdeTheme;
#endif
    if (name == QLatin1StringView(QGnomeTheme::name))
        return new QGnomeTheme;
    return nullptr;
}

QStringList QGenericUnixTheme::themeNames()
{
    QStringList result;
    if (QGuiApplication::desktopSettingsAware()) {
        const QByteArray desktopEnvironment = QGuiApplicationPrivate::platformIntegration()->services()->desktopEnvironment();
        QList<QByteArray> gtkBasedEnvironments;
        gtkBasedEnvironments << "GNOME"
                             << "X-CINNAMON"
                             << "UNITY"
                             << "MATE"
                             << "XFCE"
                             << "LXDE";
        const QList<QByteArray> desktopNames = desktopEnvironment.split(':');
        for (const QByteArray &desktopName : desktopNames) {
            if (desktopEnvironment == "KDE") {
#if QT_CONFIG(settings)
                result.push_back(QLatin1StringView(QKdeTheme::name));
#endif
            } else if (gtkBasedEnvironments.contains(desktopName)) {
                // prefer the GTK3 theme implementation with native dialogs etc.
                result.push_back(QStringLiteral("gtk3"));
                // fallback to the generic Gnome theme if loading the GTK3 theme fails
                result.push_back(QLatin1StringView(QGnomeTheme::name));
            } else {
                // unknown, but lowercase the name (our standard practice) and
                // remove any "x-" prefix
                QString s = QString::fromLatin1(desktopName.toLower());
                result.push_back(s.startsWith("x-"_L1) ? s.mid(2) : s);
            }
        }
    } // desktopSettingsAware
    result.append(QLatin1StringView(QGenericUnixTheme::name));
    return result;
}

QT_END_NAMESPACE

#ifndef QT_NO_DBUS
#include "qgenericunixthemes.moc"
#endif // QT_NO_DBUS
