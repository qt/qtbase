// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qgtk3theme.h"
#include "qgtk3dialoghelpers.h"
#include <QVariant>
#include <QGuiApplication>
#include <qpa/qwindowsysteminterface.h>

#undef signals
#include <gtk/gtk.h>

#if QT_CONFIG(xcb_xlib)
#include <X11/Xlib.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

const char *QGtk3Theme::name = "gtk3";

template <typename T>
static T gtkSetting(const gchar *propertyName)
{
    GtkSettings *settings = gtk_settings_get_default();
    T value;
    g_object_get(settings, propertyName, &value, NULL);
    return value;
}

static QString gtkSetting(const gchar *propertyName)
{
    gchararray value = gtkSetting<gchararray>(propertyName);
    QString str = QString::fromUtf8(value);
    g_free(value);
    return str;
}

void gtkMessageHandler(const gchar *log_domain,
                       GLogLevelFlags log_level,
                       const gchar *message,
                       gpointer unused_data) {
    /* Silence false-positive Gtk warnings (we are using Xlib to set
     * the WM_TRANSIENT_FOR hint).
     */
    if (g_strcmp0(message, "GtkDialog mapped without a transient parent. "
                           "This is discouraged.") != 0) {
        /* For other messages, call the default handler. */
        g_log_default_handler(log_domain, log_level, message, unused_data);
    }
}

QGtk3Theme::QGtk3Theme()
{
    // Ensure gtk uses the same windowing system, but let it
    // fallback in case GDK_BACKEND environment variable
    // filters the preferred one out
    if (QGuiApplication::platformName().startsWith("wayland"_L1))
        gdk_set_allowed_backends("wayland,x11");
    else if (QGuiApplication::platformName() == "xcb"_L1)
        gdk_set_allowed_backends("x11,wayland");

#if QT_CONFIG(xcb_xlib)
    // gtk_init will reset the Xlib error handler, and that causes
    // Qt applications to quit on X errors. Therefore, we need to manually restore it.
    int (*oldErrorHandler)(Display *, XErrorEvent *) = XSetErrorHandler(nullptr);
#endif

    gtk_init(nullptr, nullptr);

#if QT_CONFIG(xcb_xlib)
    XSetErrorHandler(oldErrorHandler);
#endif

    /* Initialize some types here so that Gtk+ does not crash when reading
     * the treemodel for GtkFontChooser.
     */
    g_type_ensure(PANGO_TYPE_FONT_FAMILY);
    g_type_ensure(PANGO_TYPE_FONT_FACE);

    /* Use our custom log handler. */
    g_log_set_handler("Gtk", G_LOG_LEVEL_MESSAGE, gtkMessageHandler, nullptr);

#define SETTING_CONNECT(setting) g_signal_connect(settings, "notify::" setting, G_CALLBACK(notifyThemeChanged), nullptr)
    auto notifyThemeChanged = [] {
        QWindowSystemInterface::handleThemeChange();
    };

    GtkSettings *settings = gtk_settings_get_default();
    SETTING_CONNECT("gtk-cursor-blink");
    SETTING_CONNECT("gtk-cursor-blink-time");
    SETTING_CONNECT("gtk-double-click-distance");
    SETTING_CONNECT("gtk-double-click-time");
    SETTING_CONNECT("gtk-long-press-time");
    SETTING_CONNECT("gtk-entry-password-hint-timeout");
    SETTING_CONNECT("gtk-dnd-drag-threshold");
    SETTING_CONNECT("gtk-icon-theme-name");
    SETTING_CONNECT("gtk-fallback-icon-theme");
    SETTING_CONNECT("gtk-font-name");
    SETTING_CONNECT("gtk-application-prefer-dark-theme");
    SETTING_CONNECT("gtk-theme-name");
    SETTING_CONNECT("gtk-cursor-theme-name");
    SETTING_CONNECT("gtk-cursor-theme-size");
#undef SETTING_CONNECT

    m_storage.reset(new QGtk3Storage);
}

static inline QVariant gtkGetLongPressTime()
{
    const char *gtk_long_press_time = "gtk-long-press-time";
    static bool found = g_object_class_find_property(G_OBJECT_GET_CLASS(gtk_settings_get_default()), gtk_long_press_time);
    if (!found)
        return QVariant();
    return QVariant(gtkSetting<guint>(gtk_long_press_time));  // Since 3.14, apparently we support >= 3.6
}

QVariant QGtk3Theme::themeHint(QPlatformTheme::ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::CursorFlashTime:
        if (gtkSetting<gboolean>("gtk-cursor-blink"))
            return QVariant(gtkSetting<gint>("gtk-cursor-blink-time"));
        else
            return 0;
    case QPlatformTheme::MouseDoubleClickDistance:
        return QVariant(gtkSetting<gint>("gtk-double-click-distance"));
    case QPlatformTheme::MouseDoubleClickInterval:
        return QVariant(gtkSetting<gint>("gtk-double-click-time"));
    case QPlatformTheme::MousePressAndHoldInterval: {
        QVariant v = gtkGetLongPressTime();
        if (!v.isValid())
            v = QGnomeTheme::themeHint(hint);
        return v;
    }
    case QPlatformTheme::PasswordMaskDelay:
        return QVariant(gtkSetting<guint>("gtk-entry-password-hint-timeout"));
    case QPlatformTheme::StartDragDistance:
        return QVariant(gtkSetting<gint>("gtk-dnd-drag-threshold"));
    case QPlatformTheme::SystemIconThemeName:
        return QVariant(gtkSetting("gtk-icon-theme-name"));
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(gtkSetting("gtk-fallback-icon-theme"));
    case QPlatformTheme::MouseCursorTheme:
        return QVariant(gtkSetting("gtk-cursor-theme-name"));
    case QPlatformTheme::MouseCursorSize: {
        int s = gtkSetting<gint>("gtk-cursor-theme-size");
        if (s > 0)
            return QVariant(QSize(s, s));
        return QGnomeTheme::themeHint(hint);
    }
    default:
        return QGnomeTheme::themeHint(hint);
    }
}

QString QGtk3Theme::gtkFontName() const
{
    QString cfgFontName = gtkSetting("gtk-font-name");
    if (!cfgFontName.isEmpty())
        return cfgFontName;
    return QGnomeTheme::gtkFontName();
}

Qt::ColorScheme QGtk3Theme::colorScheme() const
{
    Q_ASSERT(m_storage);

    Q_D(const QGnomeTheme);
    const Qt::ColorScheme colorScheme = d->colorScheme();
    const bool hasRequestedColorScheme = d->hasRequestedColorScheme();

#ifdef QT_DEBUG
    if (hasRequestedColorScheme && colorScheme != m_storage->colorScheme()) {
        qCDebug(lcQGtk3Interface) << "Requested color scheme" << colorScheme
                                  << "differs from theme color scheme" << m_storage->colorScheme();
    }
#endif

    return hasRequestedColorScheme ? colorScheme : m_storage->colorScheme();
}

void QGtk3Theme::requestColorScheme(Qt::ColorScheme scheme)
{
    const Qt::ColorScheme oldColorScheme = colorScheme();
    QGnomeTheme::requestColorScheme(scheme);
    if (oldColorScheme == colorScheme())
        return;
    qCDebug(lcQGtk3Interface) << scheme << "has been requested. Theme supports color scheme:"
                              << m_storage->colorScheme();
    m_storage->handleThemeChange();
    QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::AllEvents);
}

bool QGtk3Theme::usePlatformNativeDialog(DialogType type) const
{
    switch (type) {
    case ColorDialog:
        return true;
    case FileDialog:
        return useNativeFileDialog();
    case FontDialog:
        return true;
    default:
        return false;
    }
}

QPlatformDialogHelper *QGtk3Theme::createPlatformDialogHelper(DialogType type) const
{
    switch (type) {
    case ColorDialog:
        return new QGtk3ColorDialogHelper;
    case FileDialog:
        if (!useNativeFileDialog())
            return nullptr;
        return new QGtk3FileDialogHelper;
    case FontDialog:
        return new QGtk3FontDialogHelper;
    default:
        return nullptr;
    }
}

bool QGtk3Theme::useNativeFileDialog()
{
    /* Require GTK3 >= 3.15.5 to avoid running into this bug:
     * https://bugzilla.gnome.org/show_bug.cgi?id=725164
     *
     * While this bug only occurs when using widget-based file dialogs
     * (native GTK3 dialogs are fine) we have to disable platform file
     * dialogs entirely since we can't avoid creation of a platform
     * dialog helper.
     */
    return gtk_check_version(3, 15, 5) == nullptr;
}

const QPalette *QGtk3Theme::palette(Palette type) const
{
    Q_ASSERT(m_storage);

    Q_D(const QGnomeTheme);
    const Qt::ColorScheme colorScheme = d->colorScheme();
    const bool hasRequestedColorScheme = d->hasRequestedColorScheme();

#ifdef QT_DEBUG
    if (hasRequestedColorScheme && colorScheme != m_storage->colorScheme()) {
        qCDebug(lcQGtk3Interface) << "Current KDE theme doesn't support requested color scheme"
                                  << colorScheme << "Falling back to fusion palette.";
        return QPlatformTheme::palette(type);
    }
#endif

    return (hasRequestedColorScheme && colorScheme != m_storage->colorScheme())
            ? QPlatformTheme::palette(type)
            : m_storage->palette(type);
}

QPixmap QGtk3Theme::standardPixmap(StandardPixmap sp, const QSizeF &size) const
{
    Q_ASSERT(m_storage);
    return m_storage->standardPixmap(sp, size);
}

const QFont *QGtk3Theme::font(Font type) const
{
    Q_ASSERT(m_storage);
    return m_storage->font(type);
}

QIcon QGtk3Theme::fileIcon(const QFileInfo &fileInfo,
                           QPlatformTheme::IconOptions iconOptions) const
{
    Q_UNUSED(iconOptions);
    Q_ASSERT(m_storage);
    return m_storage->fileIcon(fileInfo);
}

#if QT_CONFIG(dbus)
void QGtk3Theme::updateColorScheme(Qt::ColorScheme newColorScheme)
{
    if (newColorScheme == colorScheme())
        QGnomeTheme::updateColorScheme(newColorScheme);
    else
        m_storage->handleThemeChange();
}
#endif // QT_CONFIG(dbus)

QT_END_NAMESPACE
