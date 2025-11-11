// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qgnometheme_p.h"
#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformfontdatabase.h>
#if QT_CONFIG(dbus) && QT_CONFIG(systemtrayicon)
#  include <private/qdbustrayicon_p.h>
#endif
#if QT_CONFIG(dbus)
#  include <private/qdbusmenubar_p.h>
#endif
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

/*!
    \class QGnomeTheme
    \brief QGnomeTheme is a theme implementation for the Gnome desktop.
    \since 5.0
    \internal
    \ingroup qpa
*/
const char *QGnomeTheme::name = "gnome";

QGnomeThemePrivate::QGnomeThemePrivate()
{
#if QT_CONFIG(dbus)
    QObject::connect(&m_gnomePortal, &QGnomePortalInterface::themeNameChanged, &m_gnomePortal,
                     [this](const QString &themeName) { m_themeName = themeName; });
#endif // QT_CONFIG(dbus)
}

QGnomeThemePrivate::~QGnomeThemePrivate()
{
    if (systemFont)
        delete systemFont;
    if (fixedFont)
        delete fixedFont;
}

void QGnomeThemePrivate::configureFonts(const QString &gtkFontName) const
{
    Q_ASSERT(!systemFont);
    const int split = gtkFontName.lastIndexOf(QChar::Space);
    float size = QStringView{gtkFontName}.mid(split + 1).toFloat();
    QString fontName = gtkFontName.left(split);

    systemFont = new QFont(fontName, size);
    fixedFont = new QFont(QLatin1StringView(QGenericUnixTheme::defaultFixedFontNameC), systemFont->pointSize());
    fixedFont->setStyleHint(QFont::TypeWriter);
    qCDebug(lcQpaFonts) << "default fonts: system" << systemFont << "fixed" << fixedFont;
}

Qt::ColorScheme QGnomeThemePrivate::colorScheme() const
{
    if (hasRequestedColorScheme())
        return m_requestedColorScheme;

#if QT_CONFIG(dbus)
    if (Qt::ColorScheme colorScheme = m_gnomePortal.colorScheme();
        colorScheme != Qt::ColorScheme::Unknown)
        return colorScheme;

    // If the color scheme is set to Unknown by mistake or is not set at all,
    // then maybe the theme name contains a hint about the color scheme.
    // Let's hope the theme name does not include any accent color name
    // which contains "dark" or "light" in it (e.g. lightblue). At the moment they don't.
    if (m_themeName.contains(QLatin1StringView("light"), Qt::CaseInsensitive))
        return Qt::ColorScheme::Light;
    else if (m_themeName.contains(QLatin1StringView("dark"), Qt::CaseInsensitive))
        return Qt::ColorScheme::Dark;
#endif // QT_CONFIG(dbus)

    // Fallback to Unknown if no color scheme is set or detected
    return Qt::ColorScheme::Unknown;
}

bool QGnomeThemePrivate::hasRequestedColorScheme() const
{
    return m_requestedColorScheme != Qt::ColorScheme::Unknown;
}

QGnomeTheme::QGnomeTheme()
    : QGenericUnixTheme(new QGnomeThemePrivate())
{
#if QT_CONFIG(dbus)
    Q_D(QGnomeTheme);

    QGnomePortalInterface *portal = &d->m_gnomePortal;

    QObject::connect(portal, &QGnomePortalInterface::colorSchemeChanged, portal,
                     [this](Qt::ColorScheme colorScheme) { updateColorScheme(colorScheme); });

    QObject::connect(portal, &QGnomePortalInterface::contrastChanged, portal,
                     [this](Qt::ContrastPreference contrast) { updateHighContrast(contrast); });
#endif // QT_CONFIG(dbus)
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
        return QVariant(xdgIconThemePaths());
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
    return QStringLiteral("%1 %2").arg(QLatin1StringView(defaultSystemFontNameC))
                                  .arg(defaultSystemFontSize);
}

void QGnomeTheme::requestColorScheme(Qt::ColorScheme scheme)
{
    Q_D(QGnomeTheme);
    if (d->m_requestedColorScheme == scheme)
        return;
    QPlatformTheme::requestColorScheme(scheme);
    d->m_requestedColorScheme = scheme;
    QWindowSystemInterface::handleThemeChange();
}

Qt::ColorScheme QGnomeTheme::colorScheme() const
{
    Q_D(const QGnomeTheme);
    if (auto colorScheme = d->colorScheme(); colorScheme != Qt::ColorScheme::Unknown)
        return colorScheme;
    // If the color scheme is not set or detected, fall back to the default
    return QPlatformTheme::colorScheme();
}

#if QT_CONFIG(dbus)
void QGnomeTheme::updateColorScheme(Qt::ColorScheme colorScheme)
{
    Q_UNUSED(colorScheme);
    QWindowSystemInterface::handleThemeChange();
}

void QGnomeTheme::updateHighContrast(Qt::ContrastPreference contrast)
{
    Q_UNUSED(contrast);
    QWindowSystemInterface::handleThemeChange();
}

QPlatformMenuBar *QGnomeTheme::createPlatformMenuBar() const
{
    if (isDBusGlobalMenuAvailable())
        return new QDBusMenuBar();
    return nullptr;
}

Qt::ContrastPreference QGnomeTheme::contrastPreference() const
{
    Q_D(const QGnomeTheme);
    return d->m_gnomePortal.contrastPreference();
}

#  if QT_CONFIG(systemtrayicon)
QPlatformSystemTrayIcon *QGnomeTheme::createPlatformSystemTrayIcon() const
{
    if (shouldUseDBusTray())
        return new QDBusTrayIcon();
    return nullptr;
}
#  endif // QT_CONFIG(systemtrayicon)
#endif // QT_CONFIG(dbus)

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

QT_END_NAMESPACE
