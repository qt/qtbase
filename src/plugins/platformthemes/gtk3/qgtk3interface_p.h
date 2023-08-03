// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGTK3INTERFACE_H
#define QGTK3INTERFACE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QString>
#include <QtCore/QCache>
#include <private/qflatmap_p.h>
#include <QtCore/QObject>
#include <QtGui/QIcon>
#include <QtGui/QPalette>
#include <QtWidgets/QWidget>
#include <QtCore/QLoggingCategory>
#include <QtGui/QPixmap>
#include <qpa/qplatformtheme.h>

#undef signals // Collides with GTK symbols
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQGtk3Interface);

using namespace Qt::StringLiterals;

class QGtk3Storage;

/*!
    \internal
    \brief The QGtk3Interface class centralizes communication with the GTK3 library.

    By encapsulating all GTK version specific syntax and conversions, it makes Qt's GTK theme
    independent from GTK versions.

    \note
    Including GTK3 headers requires #undef signals, which disables Qt signal/slot handling.
 */

class QGtk3Interface
{
    Q_GADGET
public:
    QGtk3Interface(QGtk3Storage *);
    ~QGtk3Interface();

    /*!
     *  \internal
        \enum QGtk3Interface::QGtkWidget
        \brief Represents GTK widget types used to obtain color information.

        \note The enum value gtk_Default refers to the GTK default style, rather than to a specific widget.
     */
    enum class QGtkWidget {
        gtk_menu_bar,
        gtk_menu,
        gtk_button,
        gtk_button_box,
        gtk_check_button,
        gtk_radio_button,
        gtk_frame,
        gtk_statusbar,
        gtk_entry,
        gtk_popup,
        gtk_notebook,
        gtk_toolbar,
        gtk_tree_view,
        gtk_combo_box,
        gtk_combo_box_text,
        gtk_progress_bar,
        gtk_fixed,
        gtk_separator_menu_item,
        gtk_Default,
        gtk_offscreen_window
    };
    Q_ENUM(QGtkWidget)

    /*!
        \internal
        \enum QGtk3Interface::QGtkColorSource
        \brief The QGtkColorSource enum represents the source of a color within a GTK widgets style context.

        If the current GTK theme provides such a color for a given widget, the color can be read
        from the style context by passing the enum's key as a property name to the GTK method
        gtk_style_context_lookup_color. The method will return false, if no color has been found.
     */
    enum class QGtkColorSource {
        Foreground,
        Background,
        Text,
        Base,
        Border
    };
    Q_ENUM(QGtkColorSource)

    /*!
        \internal
        \enum QGtk3Interface::QGtkColorDefault
        \brief The QGtkColorDefault enum represents generic GTK colors.

        The GTK3 methods gtk_style_context_get_color, gtk_style_context_get_background_color, and
        gtk_style_context_get_foreground_color always return the respective colors with a widget's
        style context. Unless set as a property by the current GTK theme, GTK's default colors will
        be returned.
        These generic default colors, represented by the GtkColorDefault enum, are used as a
        back, if a specific color property is requested but not defined in the current GTK theme.
     */
    enum class QGtkColorDefault {
        Foreground,
        Background,
        Border
    };
    Q_ENUM(QGtkColorDefault)

    // Create a brush from GTK widget type, color source and color state
    QBrush brush(QGtkWidget wtype, QGtkColorSource source, GtkStateFlags state) const;

    // Font & icon getters
    QImage standardPixmap(QPlatformTheme::StandardPixmap standardPixmap) const;
    QFont font(QPlatformTheme::Font type) const;
    QIcon fileIcon(const QFileInfo &fileInfo) const;

    // Return current GTK theme name
    QString themeName() const;

    // Derive color scheme from default colors
    Qt::ColorScheme colorSchemeByColors() const;

    // Convert GTK state to/from string
    static int toGtkState(const QString &state);
    static const QLatin1String fromGtkState(GtkStateFlags state);

private:

    // Map colors to GTK property names and default to generic color getters
    struct ColorKey {
        QGtkColorSource colorSource = QGtkColorSource::Background;
        GtkStateFlags state = GTK_STATE_FLAG_NORMAL;

        // struct becomes key of a map, so operator< is needed
        bool operator<(const ColorKey& other) const {
           return std::tie(colorSource, state) <
                  std::tie(other.colorSource, other.state);
        }

        QDebug operator<<(QDebug dbg)
        {
            return dbg << "QGtk3Interface::ColorKey(colorSource=" << colorSource << ", GTK state=" << fromGtkState(state) << ")";
        }
    };

    struct ColorValue {
        QString propertyName = QString();
        QGtkColorDefault genericSource = QGtkColorDefault::Background;

        QDebug operator<<(QDebug dbg)
        {
            return dbg << "QGtk3Interface::ColorValue(propertyName=" << propertyName << ", genericSource=" << genericSource << ")";
        }
    };

    typedef QFlatMap<ColorKey, ColorValue> ColorMap;
    ColorMap gtkColorMap;
    void initColorMap();

    GdkRGBA genericColor(GtkStyleContext *con, GtkStateFlags state, QGtkColorDefault def) const;

    // Cache for GTK widgets
    mutable QFlatMap<QGtkWidget, GtkWidget *> cache;

    // Converters for GTK icon and GDK pixbuf
    QImage qt_gtk_get_icon(const char *iconName) const;
    QImage qt_convert_gdk_pixbuf(GdkPixbuf *buf) const;

    // Create new GTK widget object
    GtkWidget *qt_new_gtkWidget(QGtkWidget type) const;

    // Deliver GTK Widget from cache or create new
    GtkWidget *widget(QGtkWidget type) const;

    // Get a GTK widget's style context. Default settings style context if nullptr
    GtkStyleContext *context(GtkWidget *widget = nullptr) const;

    // Convert GTK color into QColor
    static inline QColor fromGdkColor (const GdkRGBA &c)
    { return QColor::fromRgbF(c.red, c.green, c.blue, c.alpha); }

    // get a QColor of a GTK widget (default settings style if nullptr)
    QColor color (GtkWidget *widget, QGtkColorSource source, GtkStateFlags state) const;

    // Mappings for GTK fonts
    inline static constexpr QGtkWidget toWidgetType(QPlatformTheme::Font);
    inline static constexpr QFont::Style toFontStyle(PangoStyle style);
    inline static constexpr int toFontWeight(PangoWeight weight);

};
QT_END_NAMESPACE
#endif // QGTK3INTERFACE_H
