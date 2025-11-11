// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qkdetheme_p.h"
#include <qpa/qplatformtheme_p.h>
#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformdialoghelper.h>
#include <QPalette>
#include <qpa/qwindowsysteminterface.h>
#include "qdbuslistener_p.h"
#if QT_CONFIG(dbus) && QT_CONFIG(systemtrayicon)
#include <private/qdbustrayicon_p.h>
#endif
#include <private/qdbusplatformmenu_p.h>
#include <private/qdbusmenubar_p.h>
#include <QSettings>
#include <QStandardPaths>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcQpaThemeKde, "qt.qpa.theme.kde")

class QKdeThemePrivate : public QGenericUnixThemePrivate
{

public:
    enum class KdeSettingType {
        Root,
        KDE,
        Icons,
        ToolBarIcons,
        ToolBarStyle,
        Fonts,
        Colors,
    };

    enum class KdeSetting {
        WidgetStyle,
        ColorScheme,
        SingleClick,
        ShowIconsOnPushButtons,
        IconTheme,
        ToolBarIconSize,
        ToolButtonStyle,
        WheelScrollLines,
        DoubleClickInterval,
        StartDragDistance,
        StartDragTime,
        CursorBlinkRate,
        Font,
        Fixed,
        MenuFont,
        ToolBarFont,
        ButtonBackground,
        WindowBackground,
        ViewForeground,
        WindowForeground,
        ViewBackground,
        SelectionBackground,
        SelectionForeground,
        ViewBackgroundAlternate,
        ButtonForeground,
        ViewForegroundLink,
        ViewForegroundVisited,
        TooltipBackground,
        TooltipForeground,
    };

    QKdeThemePrivate(const QStringList &kdeDirs, int kdeVersion);
    ~QKdeThemePrivate() { clearResources(); }

    static QString kdeGlobals(const QString &kdeDir, int kdeVersion)
    {
        if (kdeVersion > 4)
            return kdeDir + "/kdeglobals"_L1;
        return kdeDir + "/share/config/kdeglobals"_L1;
    }

    void refresh();
    static QVariant readKdeSetting(KdeSetting s, const QStringList &kdeDirs, int kdeVersion, QHash<QString, QSettings*> &settings);
    QVariant readKdeSetting(KdeSetting s) const;
    void clearKdeSettings() const;
    static void readKdeSystemPalette(const QStringList &kdeDirs, int kdeVersion, QHash<QString, QSettings*> &kdeSettings, QPalette *pal);
    static QFont *kdeFont(const QVariant &fontValue);
    static QStringList kdeIconThemeSearchPaths(const QStringList &kdeDirs);

    const QStringList kdeDirs;
    const int kdeVersion;

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
    Qt::ColorScheme m_colorScheme = Qt::ColorScheme::Unknown;
    Qt::ColorScheme m_requestedColorScheme = Qt::ColorScheme::Unknown;
    std::unique_ptr<QPalette> systemPalette;
    QFont *fonts[QPlatformTheme::NFonts];
    void updateColorScheme(const QString &themeName);
    bool hasRequestedColorScheme() const { return m_requestedColorScheme != Qt::ColorScheme::Unknown
                                           && m_requestedColorScheme != m_colorScheme; }

private:
    mutable QHash<QString, QSettings *> kdeSettings;
#if QT_CONFIG(dbus)
    std::unique_ptr<QDBusListener> dbus;
    bool initDbus();
    void settingChangedHandler(QDBusListener::Provider provider,
                               QDBusListener::Setting setting,
                               const QVariant &value);
    Qt::ColorScheme colorSchemeFromPalette() const;
#endif // QT_CONFIG(dbus)
    void clearResources();
};

#if QT_CONFIG(dbus)
void QKdeThemePrivate::settingChangedHandler(QDBusListener::Provider provider,
                                             QDBusListener::Setting setting,
                                             const QVariant &value)
{
    if (provider != QDBusListener::Provider::Kde)
        return;

    switch (setting) {
    case QDBusListener::Setting::ColorScheme:
        qCDebug(lcQpaThemeKde) << "KDE color theme changed to:" << value.value<Qt::ColorScheme>();
        break;
    case QDBusListener::Setting::Theme:
        qCDebug(lcQpaThemeKde) << "KDE global theme changed to:" << value.toString();
        break;
    case QDBusListener::Setting::ApplicationStyle:
        qCDebug(lcQpaThemeKde) << "KDE application style changed to:" << value.toString();
        break;
    case QDBusListener::Setting::Contrast:
        qCDebug(lcQpaThemeKde) << "KDE contrast setting changed to: "
                               << value.value<Qt::ContrastPreference>();
        break;
    }

    refresh();
}

void QKdeThemePrivate::clearResources()
{
    qDeleteAll(fonts, fonts + QPlatformTheme::NFonts);
    std::fill(fonts, fonts + QPlatformTheme::NFonts, static_cast<QFont *>(nullptr));
    systemPalette.reset();
}

bool QKdeThemePrivate::initDbus()
{
    dbus.reset(new QDBusListener());
    Q_ASSERT(dbus);

    // Wrap slot in a lambda to avoid inheriting QKdeThemePrivate from QObject
    auto wrapper = [this](QDBusListener::Provider provider,
                          QDBusListener::Setting setting,
                          const QVariant &value) {
        settingChangedHandler(provider, setting, value);
    };

    return QObject::connect(dbus.get(), &QDBusListener::settingChanged, dbus.get(), wrapper);
}
#endif // QT_CONFIG(dbus)

QKdeThemePrivate::QKdeThemePrivate(const QStringList &kdeDirs, int kdeVersion)
    : kdeDirs(kdeDirs), kdeVersion(kdeVersion)
{
    std::fill(fonts, fonts + QPlatformTheme::NFonts, static_cast<QFont *>(nullptr));
#if QT_CONFIG(dbus)
    initDbus();
#endif // QT_CONFIG(dbus)
}

static constexpr QLatin1StringView settingsPrefix(QKdeThemePrivate::KdeSettingType type)
{
    switch (type) {
    case QKdeThemePrivate::KdeSettingType::Root:
        return QLatin1StringView();
    case QKdeThemePrivate::KdeSettingType::KDE:
        return QLatin1StringView("KDE/");
    case QKdeThemePrivate::KdeSettingType::Fonts:
        return QLatin1StringView();
    case QKdeThemePrivate::KdeSettingType::Colors:
        return QLatin1StringView("Colors:");
    case QKdeThemePrivate::KdeSettingType::Icons:
        return QLatin1StringView("Icons/");
    case QKdeThemePrivate::KdeSettingType::ToolBarIcons:
        return QLatin1StringView("ToolbarIcons/");
    case QKdeThemePrivate::KdeSettingType::ToolBarStyle:
        return QLatin1StringView("Toolbar style/");
    }
    // GCC 8.x does not treat __builtin_unreachable() as constexpr
#  if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
    // NOLINTNEXTLINE(qt-use-unreachable-return): Triggers on Clang, breaking GCC 8
    Q_UNREACHABLE();
#  endif
    return {};
}

static constexpr QKdeThemePrivate::KdeSettingType settingsType(QKdeThemePrivate::KdeSetting setting)
{
#define CASE(s, type) case QKdeThemePrivate::KdeSetting::s:\
        return QKdeThemePrivate::KdeSettingType::type

    switch (setting) {
    CASE(WidgetStyle, Root);
    CASE(ColorScheme, Root);
    CASE(SingleClick, KDE);
    CASE(ShowIconsOnPushButtons, KDE);
    CASE(IconTheme, Icons);
    CASE(ToolBarIconSize, ToolBarIcons);
    CASE(ToolButtonStyle, ToolBarStyle);
    CASE(WheelScrollLines, KDE);
    CASE(DoubleClickInterval, KDE);
    CASE(StartDragDistance, KDE);
    CASE(StartDragTime, KDE);
    CASE(CursorBlinkRate, KDE);
    CASE(Font, Root);
    CASE(Fixed, Root);
    CASE(MenuFont, Root);
    CASE(ToolBarFont, Root);
    CASE(ButtonBackground, Colors);
    CASE(WindowBackground, Colors);
    CASE(ViewForeground, Colors);
    CASE(WindowForeground, Colors);
    CASE(ViewBackground, Colors);
    CASE(SelectionBackground, Colors);
    CASE(SelectionForeground, Colors);
    CASE(ViewBackgroundAlternate, Colors);
    CASE(ButtonForeground, Colors);
    CASE(ViewForegroundLink, Colors);
    CASE(ViewForegroundVisited, Colors);
    CASE(TooltipBackground, Colors);
    CASE(TooltipForeground, Colors);
    };
    // GCC 8.x does not treat __builtin_unreachable() as constexpr
#  if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
    // NOLINTNEXTLINE(qt-use-unreachable-return): Triggers on Clang, breaking GCC 8
    Q_UNREACHABLE();
#  endif
    return QKdeThemePrivate::KdeSettingType::Root;
}
#undef CASE

static constexpr QLatin1StringView settingsKey(QKdeThemePrivate::KdeSetting setting)
{
    switch (setting) {
    case QKdeThemePrivate::KdeSetting::WidgetStyle:
        return QLatin1StringView("widgetStyle");
    case QKdeThemePrivate::KdeSetting::ColorScheme:
        return QLatin1StringView("ColorScheme");
    case QKdeThemePrivate::KdeSetting::SingleClick:
        return QLatin1StringView("SingleClick");
    case QKdeThemePrivate::KdeSetting::ShowIconsOnPushButtons:
        return QLatin1StringView("ShowIconsOnPushButtons");
    case QKdeThemePrivate::KdeSetting::IconTheme:
        return QLatin1StringView("Theme");
    case QKdeThemePrivate::KdeSetting::ToolBarIconSize:
        return QLatin1StringView("Size");
    case QKdeThemePrivate::KdeSetting::ToolButtonStyle:
        return QLatin1StringView("ToolButtonStyle");
    case QKdeThemePrivate::KdeSetting::WheelScrollLines:
        return QLatin1StringView("WheelScrollLines");
    case QKdeThemePrivate::KdeSetting::DoubleClickInterval:
        return QLatin1StringView("DoubleClickInterval");
    case QKdeThemePrivate::KdeSetting::StartDragDistance:
        return QLatin1StringView("StartDragDist");
    case QKdeThemePrivate::KdeSetting::StartDragTime:
        return QLatin1StringView("StartDragTime");
    case QKdeThemePrivate::KdeSetting::CursorBlinkRate:
        return QLatin1StringView("CursorBlinkRate");
    case QKdeThemePrivate::KdeSetting::Font:
        return QLatin1StringView("font");
    case QKdeThemePrivate::KdeSetting::Fixed:
        return QLatin1StringView("fixed");
    case QKdeThemePrivate::KdeSetting::MenuFont:
        return QLatin1StringView("menuFont");
    case QKdeThemePrivate::KdeSetting::ToolBarFont:
        return QLatin1StringView("toolBarFont");
    case QKdeThemePrivate::KdeSetting::ButtonBackground:
        return QLatin1StringView("Button/BackgroundNormal");
    case QKdeThemePrivate::KdeSetting::WindowBackground:
        return QLatin1StringView("Window/BackgroundNormal");
    case QKdeThemePrivate::KdeSetting::ViewForeground:
        return QLatin1StringView("View/ForegroundNormal");
    case QKdeThemePrivate::KdeSetting::WindowForeground:
        return QLatin1StringView("Window/ForegroundNormal");
    case QKdeThemePrivate::KdeSetting::ViewBackground:
        return QLatin1StringView("View/BackgroundNormal");
    case QKdeThemePrivate::KdeSetting::SelectionBackground:
        return QLatin1StringView("Selection/BackgroundNormal");
    case QKdeThemePrivate::KdeSetting::SelectionForeground:
        return QLatin1StringView("Selection/ForegroundNormal");
    case QKdeThemePrivate::KdeSetting::ViewBackgroundAlternate:
        return QLatin1StringView("View/BackgroundAlternate");
    case QKdeThemePrivate::KdeSetting::ButtonForeground:
        return QLatin1StringView("Button/ForegroundNormal");
    case QKdeThemePrivate::KdeSetting::ViewForegroundLink:
        return QLatin1StringView("View/ForegroundLink");
    case QKdeThemePrivate::KdeSetting::ViewForegroundVisited:
        return QLatin1StringView("View/ForegroundVisited");
    case QKdeThemePrivate::KdeSetting::TooltipBackground:
        return QLatin1StringView("Tooltip/BackgroundNormal");
    case QKdeThemePrivate::KdeSetting::TooltipForeground:
        return QLatin1StringView("Tooltip/ForegroundNormal");
    };
    // GCC 8.x does not treat __builtin_unreachable() as constexpr
#  if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
    // NOLINTNEXTLINE(qt-use-unreachable-return): Triggers on Clang, breaking GCC 8
    Q_UNREACHABLE();
#  endif
    return {};
}

void QKdeThemePrivate::refresh()
{
    clearResources();
    clearKdeSettings();

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

    systemPalette.reset(new QPalette(QPalette()));
    readKdeSystemPalette(kdeDirs, kdeVersion, kdeSettings, systemPalette.get());

    const QVariant styleValue = readKdeSetting(KdeSetting::WidgetStyle);
    if (styleValue.isValid()) {
        const QString style = styleValue.toString();
        if (style != styleNames.front())
            styleNames.push_front(style);
    }

    const QVariant colorScheme = readKdeSetting(KdeSetting::ColorScheme);

    updateColorScheme(colorScheme.toString());

    const QVariant singleClickValue = readKdeSetting(KdeSetting::SingleClick);
    if (singleClickValue.isValid())
        singleClick = singleClickValue.toBool();
    else if (kdeVersion >= 6) // Plasma 6 defaults to double-click
        singleClick = false;
    else // earlier version to single-click
        singleClick = true;

    const QVariant showIconsOnPushButtonsValue = readKdeSetting(KdeSetting::ShowIconsOnPushButtons);
    if (showIconsOnPushButtonsValue.isValid())
        showIconsOnPushButtons = showIconsOnPushButtonsValue.toBool();

    const QVariant themeValue = readKdeSetting(KdeSetting::IconTheme);
    if (themeValue.isValid())
        iconThemeName = themeValue.toString();

    const QVariant toolBarIconSizeValue = readKdeSetting(KdeSetting::ToolBarIconSize);
    if (toolBarIconSizeValue.isValid())
        toolBarIconSize = toolBarIconSizeValue.toInt();

    const QVariant toolbarStyleValue = readKdeSetting(KdeSetting::ToolButtonStyle);
    if (toolbarStyleValue.isValid()) {
        const QString toolBarStyle = toolbarStyleValue.toString();
        if (toolBarStyle == "TextBesideIcon"_L1)
            toolButtonStyle =  Qt::ToolButtonTextBesideIcon;
        else if (toolBarStyle == "TextOnly"_L1)
            toolButtonStyle = Qt::ToolButtonTextOnly;
        else if (toolBarStyle == "TextUnderIcon"_L1)
            toolButtonStyle = Qt::ToolButtonTextUnderIcon;
    }

    const QVariant wheelScrollLinesValue = readKdeSetting(KdeSetting::WheelScrollLines);
    if (wheelScrollLinesValue.isValid())
        wheelScrollLines = wheelScrollLinesValue.toInt();

    const QVariant doubleClickIntervalValue = readKdeSetting(KdeSetting::DoubleClickInterval);
    if (doubleClickIntervalValue.isValid())
        doubleClickInterval = doubleClickIntervalValue.toInt();

    const QVariant startDragDistValue = readKdeSetting(KdeSetting::StartDragDistance);
    if (startDragDistValue.isValid())
        startDragDist = startDragDistValue.toInt();

    const QVariant startDragTimeValue = readKdeSetting(KdeSetting::StartDragTime);
    if (startDragTimeValue.isValid())
        startDragTime = startDragTimeValue.toInt();

    const QVariant cursorBlinkRateValue = readKdeSetting(KdeSetting::CursorBlinkRate);
    if (cursorBlinkRateValue.isValid()) {
        cursorBlinkRate = cursorBlinkRateValue.toInt();
        cursorBlinkRate = cursorBlinkRate > 0 ? qBound(200, cursorBlinkRate, 2000) : 0;
    }

    // Read system font, ignore 'smallestReadableFont'
    if (QFont *systemFont = kdeFont(readKdeSetting(KdeSetting::Font)))
        fonts[QPlatformTheme::SystemFont] = systemFont;
    else
        fonts[QPlatformTheme::SystemFont] = new QFont(QLatin1StringView(QGenericUnixTheme::defaultSystemFontNameC),
                                                      QGenericUnixTheme::defaultSystemFontSize);

    if (QFont *fixedFont = kdeFont(readKdeSetting(KdeSetting::Fixed))) {
        fonts[QPlatformTheme::FixedFont] = fixedFont;
    } else {
        fixedFont = new QFont(QLatin1StringView(QGenericUnixTheme::defaultFixedFontNameC),
                              QGenericUnixTheme::defaultSystemFontSize);
        fixedFont->setStyleHint(QFont::TypeWriter);
        fonts[QPlatformTheme::FixedFont] = fixedFont;
    }

    if (QFont *menuFont = kdeFont(readKdeSetting(KdeSetting::MenuFont))) {
        fonts[QPlatformTheme::MenuFont] = menuFont;
        fonts[QPlatformTheme::MenuBarFont] = new QFont(*menuFont);
    }

    if (QFont *toolBarFont = kdeFont(readKdeSetting(KdeSetting::ToolBarFont)))
        fonts[QPlatformTheme::ToolButtonFont] = toolBarFont;

    QWindowSystemInterface::handleThemeChange();

    qCDebug(lcQpaFonts) << "default fonts: system" << fonts[QPlatformTheme::SystemFont]
                        << "fixed" << fonts[QPlatformTheme::FixedFont];
    qDeleteAll(kdeSettings);
}

QVariant QKdeThemePrivate::readKdeSetting(KdeSetting s, const QStringList &kdeDirs, int kdeVersion, QHash<QString, QSettings*> &kdeSettings)
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
            const QString key = settingsPrefix(settingsType(s)) + settingsKey(s);
            const QVariant value = settings->value(key);
            if (value.isValid())
                return value;
        }
    }
    return QVariant();
}

QVariant QKdeThemePrivate::readKdeSetting(KdeSetting s) const
{
    return readKdeSetting(s, kdeDirs, kdeVersion, kdeSettings);
}

void QKdeThemePrivate::clearKdeSettings() const
{
    kdeSettings.clear();
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
    if (!kdeColor(pal, QPalette::Button, readKdeSetting(KdeSetting::ButtonBackground, kdeDirs, kdeVersion, kdeSettings))) {
        // kcolorscheme.cpp: SetDefaultColors
        const QColor defaultWindowBackground(214, 210, 208);
        const QColor defaultButtonBackground(223, 220, 217);
        *pal = QPalette(defaultButtonBackground, defaultWindowBackground);
        return;
    }

    kdeColor(pal, QPalette::Window, readKdeSetting(KdeSetting::WindowBackground, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::Text, readKdeSetting(KdeSetting::ViewForeground, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::WindowText, readKdeSetting(KdeSetting::WindowForeground, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::Base, readKdeSetting(KdeSetting::ViewBackground, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::Highlight, readKdeSetting(KdeSetting::SelectionBackground, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::HighlightedText, readKdeSetting(KdeSetting::SelectionForeground, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::AlternateBase, readKdeSetting(KdeSetting::ViewBackgroundAlternate, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::ButtonText, readKdeSetting(KdeSetting::ButtonForeground, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::Link, readKdeSetting(KdeSetting::ViewForegroundLink, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::LinkVisited, readKdeSetting(KdeSetting::ViewForegroundVisited, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::ToolTipBase, readKdeSetting(KdeSetting::TooltipBackground, kdeDirs, kdeVersion, kdeSettings));
    kdeColor(pal, QPalette::ToolTipText, readKdeSetting(KdeSetting::TooltipForeground, kdeDirs, kdeVersion, kdeSettings));

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
    : QGenericUnixTheme(new QKdeThemePrivate(kdeDirs, kdeVersion))
{
    d_func()->refresh();
}

QKdeTheme::~QKdeTheme()
    = default;

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
    case QPlatformTheme::MouseCursorTheme:
        return QVariant(mouseCursorTheme());
    case QPlatformTheme::MouseCursorSize:
        return QVariant(mouseCursorSize());
    case QPlatformTheme::PreferFileIconFromTheme:
        return true;
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

/*!
    \internal
    \reimp
    \brief QKdeTheme::requestColorScheme Programmatically request a color scheme
    If \a scheme is \c Dark or \c Light, \a scheme is applied to the application,
    independently from the current KDE theme.
    If \a scheme is \c Unknown, the current KDE theme's color scheme will be applied instead.
    This is the default behavior.

    \note
    A KDE theme is considered either \c Dark or \c Light. When the requested color scheme
    doesn't match the current KDE theme, a default \c Dark or \c Light fusion palette
    is used instead.
    \sa QKdeThemePrivate::hasRequestedColorScheme
*/

/*!
    \internal
    \brief QKdeThemePrivate::hasRequestedColorScheme Check if fusion palette fallback is necessary.
    This internal helper function returns true, if
    \list
    \li a color scheme has been programmatically requested, and
    \li the requested color scheme differs from the current KDE theme's color scheme.
    \endlist
    \sa QKdeTheme:requestColorScheme
*/

void QKdeTheme::requestColorScheme(Qt::ColorScheme scheme)
{
    Q_D(QKdeTheme);
    if (d->m_requestedColorScheme == scheme)
        return;
    qCDebug(lcQpaThemeKde) << scheme << "has been requested. Theme supports color scheme:"
                           << d->m_colorScheme;
    d->m_requestedColorScheme = scheme;
    d->refresh();
}

Qt::ColorScheme QKdeTheme::colorScheme() const
{
    Q_D(const QKdeTheme);
#ifdef QT_DEBUG
    if (d->hasRequestedColorScheme()) {
        qCDebug(lcQpaThemeKde) << "Reuqested color scheme" << d->m_requestedColorScheme
                               << "differs from theme color scheme" << d->m_colorScheme;
    }
#endif
    return d->hasRequestedColorScheme() ? d->m_requestedColorScheme
                                        : d->m_colorScheme;
}

/*!
    \internal
    \brief QKdeTheme::updateColorScheme - guess and set a color scheme for unix themes.
    KDE themes do not have a color scheme property.
    The key words "dark" or "light" are usually part of the theme name.
    This is, however, not a mandatory convention.

    If \param themeName contains a valid key word, the respective color scheme is set.
    If it doesn't, the color scheme is heuristically determined by comparing text and base color
    of the system palette.
 */
void QKdeThemePrivate::updateColorScheme(const QString &themeName)
{
    if (themeName.contains(QLatin1StringView("light"), Qt::CaseInsensitive)) {
        m_colorScheme = Qt::ColorScheme::Light;
        return;
    }
    if (themeName.contains(QLatin1StringView("dark"), Qt::CaseInsensitive)) {
        m_colorScheme = Qt::ColorScheme::Dark;
        return;
    }

    m_colorScheme = colorSchemeFromPalette();
}

Qt::ColorScheme QKdeThemePrivate::colorSchemeFromPalette() const
{
    if (!systemPalette)
        return Qt::ColorScheme::Unknown;
    if (systemPalette->text().color().lightness() < systemPalette->base().color().lightness())
        return Qt::ColorScheme::Light;
    if (systemPalette->text().color().lightness() > systemPalette->base().color().lightness())
        return Qt::ColorScheme::Dark;
    return Qt::ColorScheme::Unknown;
}

const QPalette *QKdeTheme::palette(Palette type) const
{
    Q_D(const QKdeTheme);
    if (d->hasRequestedColorScheme()) {
        qCDebug(lcQpaThemeKde) << "Current KDE theme doesn't support reuqested color scheme"
                               << d->m_requestedColorScheme << "Falling back to fusion palette:"
                               << type;
        return QPlatformTheme::palette(type);
    }

    return d->systemPalette.get();
}

const QFont *QKdeTheme::font(Font type) const
{
    Q_D(const QKdeTheme);
    return d->fonts[type];
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
    const QString kdeHomePathVar = qEnvironmentVariable("KDEHOME");
    if (!kdeHomePathVar.isEmpty())
        kdeDirs += kdeHomePathVar;

    const QString kdeDirsVar = qEnvironmentVariable("KDEDIRS");
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

#if QT_CONFIG(dbus)
QPlatformMenuBar *QKdeTheme::createPlatformMenuBar() const
{
    if (isDBusGlobalMenuAvailable())
        return new QDBusMenuBar();
    return nullptr;
}
#endif

#if QT_CONFIG(dbus) && QT_CONFIG(systemtrayicon)
QPlatformSystemTrayIcon *QKdeTheme::createPlatformSystemTrayIcon() const
{
    if (shouldUseDBusTray())
        return new QDBusTrayIcon();
    return nullptr;
}
#endif

QT_END_NAMESPACE
