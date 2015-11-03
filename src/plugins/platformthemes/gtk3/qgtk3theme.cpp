/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgtk3theme.h"
#include "qgtk3dialoghelpers.h"
#include <QVariant>

#undef signals
#include <gtk/gtk.h>

#include <X11/Xlib.h>

QT_BEGIN_NAMESPACE

const char *QGtk3Theme::name = "gtk3";

static QString gtkSetting(const gchar *propertyName)
{
    GtkSettings *settings = gtk_settings_get_default();
    gchararray value;
    g_object_get(settings, propertyName, &value, NULL);
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
    int (*oldErrorHandler)(Display *, XErrorEvent *) = XSetErrorHandler(NULL);

    gtk_init(0, 0);

    XSetErrorHandler(oldErrorHandler);

    /* Initialize some types here so that Gtk+ does not crash when reading
     * the treemodel for GtkFontChooser.
     */
    g_type_ensure(PANGO_TYPE_FONT_FAMILY);
    g_type_ensure(PANGO_TYPE_FONT_FACE);

    /* Use our custom log handler. */
    g_log_set_handler("Gtk", G_LOG_LEVEL_MESSAGE, gtkMessageHandler, NULL);
}

QVariant QGtk3Theme::themeHint(QPlatformTheme::ThemeHint hint) const
{
    switch (hint) {
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
        return true;
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
        return new QGtk3FileDialogHelper;
    case FontDialog:
        return new QGtk3FontDialogHelper;
    default:
        return 0;
    }
}

QT_END_NAMESPACE
