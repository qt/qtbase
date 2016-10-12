/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgenericunixthemes_p.h"

#include "qpa/qplatformtheme_p.h"

#include <QtGui/QPalette>
#include <QtGui/QFont>
#include <QtGui/QGuiApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QMimeDatabase>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSettings>
#include <QtCore/QVariant>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>
#include <qpa/qplatformdialoghelper.h>
#ifndef QT_NO_DBUS
#include "qdbusplatformmenu_p.h"
#include "qdbusmenubar_p.h"
#endif
#if !defined(QT_NO_DBUS) && !defined(QT_NO_SYSTEMTRAYICON)
#include "qdbustrayicon_p.h"
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcTray)

ResourceHelper::ResourceHelper()
{
    std::fill(palettes, palettes + QPlatformTheme::NPalettes, static_cast<QPalette *>(0));
    std::fill(fonts, fonts + QPlatformTheme::NFonts, static_cast<QFont *>(0));
}

void ResourceHelper::clear()
{
    qDeleteAll(palettes, palettes + QPlatformTheme::NPalettes);
    qDeleteAll(fonts, fonts + QPlatformTheme::NFonts);
    std::fill(palettes, palettes + QPlatformTheme::NPalettes, static_cast<QPalette *>(0));
    std::fill(fonts, fonts + QPlatformTheme::NFonts, static_cast<QFont *>(0));
}

/*!
    \class QGenericX11ThemeQKdeTheme
    \brief QGenericX11Theme is a generic theme implementation for X11.
    \since 5.0
    \internal
    \ingroup qpa
*/

const char *QGenericUnixTheme::name = "generic";

// Default system font, corresponding to the value returned by 4.8 for
// XRender/FontConfig which we can now assume as default.
static const char defaultSystemFontNameC[] = "Sans Serif";
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
    QDBusConnection connection = QDBusConnection::sessionBus();
    QString registrarService = QStringLiteral("com.canonical.AppMenu.Registrar");
    return connection.interface()->isServiceRegistered(registrarService);
}

static bool isDBusGlobalMenuAvailable()
{
    static bool dbusGlobalMenuAvailable = checkDBusGlobalMenuAvailable();
    return dbusGlobalMenuAvailable;
}
#endif

class QGenericUnixThemePrivate : public QPlatformThemePrivate
{
public:
    QGenericUnixThemePrivate()
        : QPlatformThemePrivate()
        , systemFont(QLatin1String(defaultSystemFontNameC), defaultSystemFontSize)
        , fixedFont(QStringLiteral("monospace"), systemFont.pointSize())
    {
        fixedFont.setStyleHint(QFont::TypeWriter);
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
        return 0;
    }
}

// Helper to return the icon theme paths from XDG.
QStringList QGenericUnixTheme::xdgIconThemePaths()
{
    QStringList paths;
    // Add home directory first in search path
    const QFileInfo homeIconDir(QDir::homePath() + QLatin1String("/.icons"));
    if (homeIconDir.isDir())
        paths.prepend(homeIconDir.absoluteFilePath());

    QString xdgDirString = QFile::decodeName(qgetenv("XDG_DATA_DIRS"));
    if (xdgDirString.isEmpty())
        xdgDirString = QLatin1String("/usr/local/share/:/usr/share/");
    const auto xdgDirs = xdgDirString.splitRef(QLatin1Char(':'));
    for (const QStringRef &xdgDir : xdgDirs) {
        const QFileInfo xdgIconsDir(xdgDir + QLatin1String("/icons"));
        if (xdgIconsDir.isDir())
            paths.append(xdgIconsDir.absoluteFilePath());
    }

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
    return Q_NULLPTR;
}
#endif

QVariant QGenericUnixTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(QString(QStringLiteral("hicolor")));
    case QPlatformTheme::IconThemeSearchPaths:
        return xdgIconThemePaths();
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(true);
    case QPlatformTheme::StyleNames: {
        QStringList styleNames;
        styleNames << QStringLiteral("Fusion") << QStringLiteral("Windows");
        return QVariant(styleNames);
    }
    case QPlatformTheme::KeyboardScheme:
        return QVariant(int(X11KeyboardScheme));
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

#ifndef QT_NO_SETTINGS
class QKdeThemePrivate : public QPlatformThemePrivate
{
public:
    QKdeThemePrivate(const QStringList &kdeDirs, int kdeVersion)
        : kdeDirs(kdeDirs)
        , kdeVersion(kdeVersion)
        , toolButtonStyle(Qt::ToolButtonTextBesideIcon)
        , toolBarIconSize(0)
        , singleClick(true)
        , wheelScrollLines(3)
    { }

    static QString kdeGlobals(const QString &kdeDir, int kdeVersion)
    {
        if (kdeVersion > 4)
            return kdeDir + QLatin1String("/kdeglobals");
        return kdeDir + QLatin1String("/share/config/kdeglobals");
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
    int toolButtonStyle;
    int toolBarIconSize;
    bool singleClick;
    int wheelScrollLines;
};

void QKdeThemePrivate::refresh()
{
    resources.clear();

    toolButtonStyle = Qt::ToolButtonTextBesideIcon;
    toolBarIconSize = 0;
    styleNames.clear();
    if (kdeVersion >= 5)
        styleNames << QStringLiteral("breeze");
    styleNames << QStringLiteral("Oxygen") << QStringLiteral("fusion") << QStringLiteral("windows");
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

    const QVariant themeValue = readKdeSetting(QStringLiteral("Icons/Theme"), kdeDirs, kdeVersion, kdeSettings);
    if (themeValue.isValid())
        iconThemeName = themeValue.toString();

    const QVariant toolBarIconSizeValue = readKdeSetting(QStringLiteral("ToolbarIcons/Size"), kdeDirs, kdeVersion, kdeSettings);
    if (toolBarIconSizeValue.isValid())
        toolBarIconSize = toolBarIconSizeValue.toInt();

    const QVariant toolbarStyleValue = readKdeSetting(QStringLiteral("Toolbar style/ToolButtonStyle"), kdeDirs, kdeVersion, kdeSettings);
    if (toolbarStyleValue.isValid()) {
        const QString toolBarStyle = toolbarStyleValue.toString();
        if (toolBarStyle == QLatin1String("TextBesideIcon"))
            toolButtonStyle =  Qt::ToolButtonTextBesideIcon;
        else if (toolBarStyle == QLatin1String("TextOnly"))
            toolButtonStyle = Qt::ToolButtonTextOnly;
        else if (toolBarStyle == QLatin1String("TextUnderIcon"))
            toolButtonStyle = Qt::ToolButtonTextUnderIcon;
    }

    const QVariant wheelScrollLinesValue = readKdeSetting(QStringLiteral("KDE/WheelScrollLines"), kdeDirs, kdeVersion, kdeSettings);
    if (wheelScrollLinesValue.isValid())
        wheelScrollLines = wheelScrollLinesValue.toInt();

    // Read system font, ignore 'smallestReadableFont'
    if (QFont *systemFont = kdeFont(readKdeSetting(QStringLiteral("font"), kdeDirs, kdeVersion, kdeSettings)))
        resources.fonts[QPlatformTheme::SystemFont] = systemFont;
    else
        resources.fonts[QPlatformTheme::SystemFont] = new QFont(QLatin1String(defaultSystemFontNameC), defaultSystemFontSize);

    if (QFont *fixedFont = kdeFont(readKdeSetting(QStringLiteral("fixed"), kdeDirs, kdeVersion, kdeSettings))) {
        resources.fonts[QPlatformTheme::FixedFont] = fixedFont;
    } else {
        fixedFont = new QFont(QLatin1String(defaultSystemFontNameC), defaultSystemFontSize);
        fixedFont->setStyleHint(QFont::TypeWriter);
        resources.fonts[QPlatformTheme::FixedFont] = fixedFont;
    }

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
        if (fontValue.type() == QVariant::StringList) {
            const QStringList list = fontValue.toStringList();
            if (!list.isEmpty()) {
                fontFamily = list.first();
                fontDescription = list.join(QLatin1Char(','));
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
    return 0;
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
        return QVariant(true);
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
        return 0;

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
        kdeDirs += kdeDirsVar.split(QLatin1Char(':'), QString::SkipEmptyParts);

    const QString kdeVersionHomePath = QDir::homePath() + QLatin1String("/.kde") + QLatin1String(kdeVersionBA);
    if (QFileInfo(kdeVersionHomePath).isDir())
        kdeDirs += kdeVersionHomePath;

    const QString kdeHomePath = QDir::homePath() + QLatin1String("/.kde");
    if (QFileInfo(kdeHomePath).isDir())
        kdeDirs += kdeHomePath;

    const QString kdeRcPath = QLatin1String("/etc/kde") + QLatin1String(kdeVersionBA) + QLatin1String("rc");
    if (QFileInfo(kdeRcPath).isReadable()) {
        QSettings kdeSettings(kdeRcPath, QSettings::IniFormat);
        kdeSettings.beginGroup(QStringLiteral("Directories-default"));
        kdeDirs += kdeSettings.value(QStringLiteral("prefixes")).toStringList();
    }

    const QString kdeVersionPrefix = QLatin1String("/etc/kde") + QLatin1String(kdeVersionBA);
    if (QFileInfo(kdeVersionPrefix).isDir())
        kdeDirs += kdeVersionPrefix;

    kdeDirs.removeDuplicates();
    if (kdeDirs.isEmpty()) {
        qWarning("Unable to determine KDE dirs");
        return 0;
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
    return Q_NULLPTR;
}
#endif

#endif // QT_NO_SETTINGS

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
    QGnomeThemePrivate() : systemFont(Q_NULLPTR), fixedFont(Q_NULLPTR) {}
    ~QGnomeThemePrivate() { delete systemFont; delete fixedFont; }

    void configureFonts(const QString &gtkFontName) const
    {
        Q_ASSERT(!systemFont);
        const int split = gtkFontName.lastIndexOf(QChar::Space);
        float size = gtkFontName.midRef(split + 1).toFloat();
        QString fontName = gtkFontName.left(split);

        systemFont = new QFont(fontName, size);
        fixedFont = new QFont(QLatin1String("monospace"), systemFont->pointSize());
        fixedFont->setStyleHint(QFont::TypeWriter);
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
        styleNames << QStringLiteral("fusion") << QStringLiteral("windows");
        return QVariant(styleNames);
    }
    case QPlatformTheme::KeyboardScheme:
        return QVariant(int(GnomeKeyboardScheme));
    case QPlatformTheme::PasswordMaskCharacter:
        return QVariant(QChar(0x2022));
    case QPlatformTheme::UiEffects:
        return QVariant(int(HoverEffect));
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
        return 0;
    }
}

QString QGnomeTheme::gtkFontName() const
{
    return QStringLiteral("%1 %2").arg(QLatin1String(defaultSystemFontNameC)).arg(defaultSystemFontSize);
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
    return Q_NULLPTR;
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
    if (name == QLatin1String(QGenericUnixTheme::name))
        return new QGenericUnixTheme;
#ifndef QT_NO_SETTINGS
    if (name == QLatin1String(QKdeTheme::name))
        if (QPlatformTheme *kdeTheme = QKdeTheme::createKdeTheme())
            return kdeTheme;
#endif
    if (name == QLatin1String(QGnomeTheme::name))
        return new QGnomeTheme;
    return Q_NULLPTR;
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
#ifndef QT_NO_SETTINGS
                result.push_back(QLatin1String(QKdeTheme::name));
#endif
            } else if (gtkBasedEnvironments.contains(desktopName)) {
                // prefer the GTK3 theme implementation with native dialogs etc.
                result.push_back(QStringLiteral("gtk3"));
                // fallback to the generic Gnome theme if loading the GTK3 theme fails
                result.push_back(QLatin1String(QGnomeTheme::name));
            }
        }
        const QString session = QString::fromLocal8Bit(qgetenv("DESKTOP_SESSION"));
        if (!session.isEmpty() && session != QLatin1String("default") && !result.contains(session))
            result.push_back(session);
    } // desktopSettingsAware
    result.append(QLatin1String(QGenericUnixTheme::name));
    return result;
}

QT_END_NAMESPACE
