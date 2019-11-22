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

#include "qgtk3theme.h"
#include "qgtk3dialoghelpers.h"
#include "qgtk3menu.h"
#include <QVariant>

#undef signals
#include <gtk/gtk.h>

#include <X11/Xlib.h>

QT_BEGIN_NAMESPACE

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
    // gtk_init will reset the Xlib error handler, and that causes
    // Qt applications to quit on X errors. Therefore, we need to manually restore it.
    int (*oldErrorHandler)(Display *, XErrorEvent *) = XSetErrorHandler(nullptr);

    gtk_init(nullptr, nullptr);

    XSetErrorHandler(oldErrorHandler);

    /* Initialize some types here so that Gtk+ does not crash when reading
     * the treemodel for GtkFontChooser.
     */
    g_type_ensure(PANGO_TYPE_FONT_FAMILY);
    g_type_ensure(PANGO_TYPE_FONT_FACE);

    /* Use our custom log handler. */
    g_log_set_handler("Gtk", G_LOG_LEVEL_MESSAGE, gtkMessageHandler, nullptr);
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
        return QVariant(gtkSetting<gint>("gtk-cursor-blink-time"));
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

QPlatformMenu* QGtk3Theme::createPlatformMenu() const
{
    return new QGtk3Menu;
}

QPlatformMenuItem* QGtk3Theme::createPlatformMenuItem() const
{
    return new QGtk3MenuItem;
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

QT_END_NAMESPACE
