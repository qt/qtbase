// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include "qgtk3json_p.h"
#include "qgtk3storage_p.h"
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

QGtk3Storage::QGtk3Storage()
{
    m_interface.reset(new QGtk3Interface(this));
    populateMap();
}

/*!
    \internal
    \enum QGtk3Storage::SourceType
    \brief This enum represents the type of a color source.

    \value Gtk Color is read from a GTK widget
    \value Fixed A fixed brush is specified
    \value Modified The color is a modification of another color (fixed or read from GTK)
    \omitvalue Invalid
 */

/*!
    \internal
    \brief Find a brush from a source.

    Returns a QBrush from a given \param source and a \param map of available brushes
    to search from.

    A null QBrush is returned, if no brush corresponding to the source has been found.
 */
QBrush QGtk3Storage::brush(const Source &source, const BrushMap &map) const
{
    switch (source.sourceType) {
    case SourceType::Gtk:
        return m_interface ? QBrush(m_interface->brush(source.gtk3.gtkWidgetType,
                                    source.gtk3.source, source.gtk3.state))
                           : QBrush();

    case SourceType::Modified: {
        // don't loop through modified sources, break if modified source not found
        Source recSource = brush(TargetBrush(source.rec.colorGroup, source.rec.colorRole,
                                              source.rec.colorScheme), map);

        if (!recSource.isValid() || (recSource.sourceType == SourceType::Modified))
            return QBrush();

        // Set brush and alter color
        QBrush b = brush(recSource, map);
        if (source.rec.width > 0 && source.rec.height > 0)
            b.setTexture(QPixmap(source.rec.width, source.rec.height));
        QColor c = b.color().lighter(source.rec.lighter);
        c = QColor((c.red() + source.rec.deltaRed),
                   (c.green() + source.rec.deltaGreen),
                   (c.blue() + source.rec.deltaBlue));
        b.setColor(c);
        return b;
    }

    case SourceType::Fixed:
        return source.fix.fixedBrush;

    case SourceType::Invalid:
        return QBrush();
    }

    // needed because of the scope after recursive
    Q_UNREACHABLE();
}

/*!
    \internal
    \brief Recurse to find a source brush for modification.

    Returns the source specified by the target brush \param b in the \param map of brushes.
    Takes dark/light/unknown into consideration.
    Returns an empty brush if no suitable one can be found.
 */
QGtk3Storage::Source QGtk3Storage::brush(const TargetBrush &b, const BrushMap &map) const
{
#define FIND(brush) if (map.contains(brush))\
                        return map.value(brush)

    // Return exact match
    FIND(b);

    // unknown color scheme can find anything
    if (b.colorScheme == Qt::ColorScheme::Unknown) {
        FIND(TargetBrush(b, Qt::ColorScheme::Dark));
        FIND(TargetBrush(b, Qt::ColorScheme::Light));
    }

    // Color group All can always be found
    if (b.colorGroup != QPalette::All)
        return brush(TargetBrush(QPalette::All, b.colorRole, b.colorScheme), map);

    // Brush not found
    return Source();
#undef FIND
}

/*!
    \internal
    \brief Returns a simple, hard coded base palette.

    Create a hard coded palette with default colors as a fallback for any color that can't be
    obtained from GTK.

    \note This palette will be used as a default baseline for the system palette, which then
    will be used as a default baseline for any other palette type.
 */
QPalette QGtk3Storage::standardPalette()
{
    QColor backgroundColor(0xd4, 0xd0, 0xc8);
    QColor lightColor(backgroundColor.lighter());
    QColor darkColor(backgroundColor.darker());
    const QBrush darkBrush(darkColor);
    QColor midColor(Qt::gray);
    QPalette palette(Qt::black, backgroundColor, lightColor, darkColor,
                     midColor, Qt::black, Qt::white);
    palette.setBrush(QPalette::Disabled, QPalette::WindowText, darkBrush);
    palette.setBrush(QPalette::Disabled, QPalette::Text, darkBrush);
    palette.setBrush(QPalette::Disabled, QPalette::ButtonText, darkBrush);
    palette.setBrush(QPalette::Disabled, QPalette::Base, QBrush(backgroundColor));
    return palette;
}

/*!
    \internal
    \brief Return a GTK styled QPalette.

    Returns the pointer to a (cached) QPalette for \param type, with its brushes
    populated according to the current GTK theme.
 */
const QPalette *QGtk3Storage::palette(QPlatformTheme::Palette type) const
{
    if (type >= QPlatformTheme::NPalettes)
        return nullptr;

    if (m_paletteCache[type].has_value()) {
        qCDebug(lcQGtk3Interface) << "Returning palette from cache:"
                                  << QGtk3Json::fromPalette(type);

        return &m_paletteCache[type].value();
    }

    // Read system palette as a baseline first
    if (!m_paletteCache[QPlatformTheme::SystemPalette].has_value() && type != QPlatformTheme::SystemPalette)
        palette();

    // Fall back to system palette for unknown types
    if (!m_palettes.contains(type) &&  type != QPlatformTheme::SystemPalette) {
        qCDebug(lcQGtk3Interface) << "Returning system palette for unknown type"
                                  << QGtk3Json::fromPalette(type);
        return palette();
    }

    BrushMap brushes = m_palettes.value(type);

    // Standard palette is base for system palette. System palette is base for all others.
    QPalette p = QPalette( type == QPlatformTheme::SystemPalette ? standardPalette()
                                   : m_paletteCache[QPlatformTheme::SystemPalette].value());

    qCDebug(lcQGtk3Interface) << "Creating palette:" << QGtk3Json::fromPalette(type);
    for (auto i = brushes.begin(); i != brushes.end(); ++i) {
        Source source = i.value();

        // Brush is set if
        // - theme and source color scheme match
        // - or either of them is unknown
        const auto appSource = i.key().colorScheme;
        const auto appTheme = colorScheme();
        const bool setBrush = (appSource == appTheme) ||
                              (appSource == Qt::ColorScheme::Unknown) ||
                              (appTheme == Qt::ColorScheme::Unknown);

        if (setBrush) {
            p.setBrush(i.key().colorGroup, i.key().colorRole, brush(source, brushes));
        }
    }

    m_paletteCache[type].emplace(p);
    if (type == QPlatformTheme::SystemPalette)
        qCDebug(lcQGtk3Interface) << "System Palette defined" << themeName() << colorScheme() << p;

    return &m_paletteCache[type].value();
}

/*!
    \internal
    \brief Return a GTK styled font.

    Returns a QFont of \param type, styled according to the current GTK theme.
*/
const QFont *QGtk3Storage::font(QPlatformTheme::Font type) const
{
    if (m_fontCache[type].has_value())
        return &m_fontCache[type].value();

    m_fontCache[type].emplace(m_interface->font(type));
    return &m_fontCache[type].value();
}

/*!
    \internal
    \brief Return a GTK styled standard pixmap if available.

    Returns a pixmap specified by \param standardPixmap and \param size.
    Returns an empty pixmap if GTK doesn't support the requested one.
 */
QPixmap QGtk3Storage::standardPixmap(QPlatformTheme::StandardPixmap standardPixmap,
                                     const QSizeF &size) const
{
    if (m_pixmapCache.contains(standardPixmap))
        return QPixmap::fromImage(m_pixmapCache.object(standardPixmap)->scaled(size.toSize()));

    if (!m_interface)
        return QPixmap();

    QImage image = m_interface->standardPixmap(standardPixmap);
    if (image.isNull())
        return QPixmap();

    m_pixmapCache.insert(standardPixmap, new QImage(image));
    return QPixmap::fromImage(image.scaled(size.toSize()));
}

/*!
    \internal
    \brief Returns a GTK styled file icon corresponding to \param fileInfo.
 */
QIcon QGtk3Storage::fileIcon(const QFileInfo &fileInfo) const
{
    return m_interface ? m_interface->fileIcon(fileInfo) : QIcon();
}

/*!
    \internal
    \brief Clears all caches.
 */
void QGtk3Storage::clear()
{
    m_colorScheme = Qt::ColorScheme::Unknown;
    m_palettes.clear();
    for (auto &cache : m_paletteCache)
        cache.reset();

    for (auto &cache : m_fontCache)
        cache.reset();
}

/*!
    \internal
    \brief Handles a theme change at runtime.

    Clear all caches, re-populate with current GTK theme and notify the window system interface.
    This method is a callback for the theme change signal sent from GTK.
 */
void QGtk3Storage::handleThemeChange()
{
    clear();
    populateMap();
    QWindowSystemInterface::handleThemeChange();
}

/*!
    \internal
    \brief Populates a map with information about how to locate colors in GTK.

    This method creates a data structure to locate color information for each brush of a QPalette
    within GTK. The structure can hold mapping information for each QPlatformTheme::Palette
    enum value. If no specific mapping is stored for an enum value, the system palette is returned
    instead of a specific one. If no mapping is stored for the system palette, it will fall back to
    QGtk3Storage::standardPalette.

    The method will populate the data structure with a standard mapping, covering the following
    palette types:
    \list
    \li QPlatformTheme::SystemPalette
    \li QPlatformTheme::CheckBoxPalette
    \li QPlatformTheme::RadioButtonPalette
    \li QPlatformTheme::ComboBoxPalette
    \li QPlatformTheme::GroupBoxPalette
    \li QPlatformTheme::MenuPalette
    \li QPlatformTheme::TextLineEditPalette
    \endlist

    The method will check the environment variable {{QT_GUI_GTK_JSON_SAVE}}. If it points to a
    valid path with write access, it will write the standard mapping into a Json file.
    That Json file can be modified and/or extended.
    The Json syntax is
    - "QGtk3Palettes" (top level value)
        - QPlatformTheme::Palette
            - QPalette::ColorRole
                - Qt::ColorScheme
                - Qt::ColorGroup
                - Source data
                    - Source Type
                        - [source data]

    If the environment variable {{QT_GUI_GTK_JSON_HARDCODED}} contains the keyword \c true,
    all sources are converted to fixed sources. In that case, they contain the hard coded HexRGBA
    values read from GTK.

    The method will also check the environment variable {{QT_GUI_GTK_JSON}}. If it points to a valid
    Json file with read access, it will be parsed instead of creating a standard mapping.
    Parsing errors will be printed out with qCInfo if the logging category {{qt.qpa.gtk}} is activated.
    In case of a parsing error, the method will fall back to creating a standard mapping.

    \note
    If a Json file contains only fixed brushes (e.g. exported with {{QT_GUI_GTK_JSON_HARDCODED=true}}),
    no colors will be imported from GTK.
 */
void QGtk3Storage::populateMap()
{
    static QString m_themeName;

    // Distiguish initialization, theme change or call without theme change
    const QString newThemeName = themeName();
    if (m_themeName == newThemeName)
        return;

    clear();

    // Derive color scheme from theme name
    m_colorScheme = newThemeName.contains("dark"_L1, Qt::CaseInsensitive)
                   ? Qt::ColorScheme::Dark : m_interface->colorSchemeByColors();

    if (m_themeName.isEmpty()) {
        qCDebug(lcQGtk3Interface) << "GTK theme initialized:" << newThemeName << m_colorScheme;
    } else {
        qCDebug(lcQGtk3Interface) << "GTK theme changed to:" << newThemeName << m_colorScheme;
    }
    m_themeName = newThemeName;

    // create standard mapping or load from Json file?
    const QString jsonInput = qEnvironmentVariable("QT_GUI_GTK_JSON");
    if (!jsonInput.isEmpty()) {
        if (load(jsonInput)) {
            return;
        } else {
            qWarning() << "Falling back to standard GTK mapping.";
        }
    }

    createMapping();

    const QString jsonOutput = qEnvironmentVariable("QT_GUI_GTK_JSON_SAVE");
    if (!jsonOutput.isEmpty() && !save(jsonOutput))
        qWarning() << "File" << jsonOutput << "could not be saved.\n";
}

/*!
    \internal
    \brief Return a palette map for saving.

    This method returns the existing palette map, if the environment variable
    {{QT_GUI_GTK_JSON_HARDCODED}} is not set or does not contain the keyword \c true.
    If it contains the keyword \c true, it returns a palette map with all brush
    sources converted to fixed sources.
 */
const QGtk3Storage::PaletteMap QGtk3Storage::savePalettes() const
{
    const QString hard = qEnvironmentVariable("QT_GUI_GTK_JSON_HARDCODED");
    if (!hard.contains("true"_L1, Qt::CaseInsensitive))
        return m_palettes;

    // Json output is supposed to be readable without GTK connection
    // convert palette map into hard coded brushes
    PaletteMap map = m_palettes;
    for (auto paletteIterator = map.begin(); paletteIterator != map.end();
         ++paletteIterator) {
        QGtk3Storage::BrushMap &bm = paletteIterator.value();
        for (auto brushIterator = bm.begin(); brushIterator != bm.end();
             ++brushIterator) {
            QGtk3Storage::Source &s = brushIterator.value();
            switch (s.sourceType) {

            // Read the brush and convert it into a fixed brush
            case SourceType::Gtk: {
                const QBrush fixedBrush = brush(s, bm);
                s.fix.fixedBrush = fixedBrush;
                s.sourceType = SourceType::Fixed;
            }
                break;
            case SourceType::Fixed:
            case SourceType::Modified:
            case SourceType::Invalid:
                break;
            }
        }
    }
    return map;
}

/*!
    \internal
    \brief Saves current palette mapping to a \param filename with Json format \param f.

    Saves the current palette mapping into a QJson file,
    taking {{QT_GUI_GTK_JSON_HARDCODED}} into consideration.
    Returns \c true if saving was successful and \c false otherwise.
 */
bool QGtk3Storage::save(const QString &filename, QJsonDocument::JsonFormat f) const
{
    return QGtk3Json::save(savePalettes(), filename, f);
}

/*!
    \internal
    \brief Returns a QJsonDocument with current palette mapping.

    Saves the current palette mapping into a QJsonDocument,
    taking {{QT_GUI_GTK_JSON_HARDCODED}} into consideration.
    Returns \c true if saving was successful and \c false otherwise.
 */
QJsonDocument QGtk3Storage::save() const
{
    return QGtk3Json::save(savePalettes());
}

/*!
    \internal
    \brief Loads palette mapping from Json file \param filename.

    Returns \c true if the file was successfully parsed and \c false otherwise.
 */
bool QGtk3Storage::load(const QString &filename)
{
    return QGtk3Json::load(m_palettes, filename);
}

/*!
    \internal
    \brief Creates a standard palette mapping.

    The method creates a hard coded standard mapping, used if no external Json file
    containing a valid mapping has been specified in the environment variable {{QT_GUI_GTK_JSON}}.
 */
void QGtk3Storage::createMapping()
{
    // Hard code standard mapping
    BrushMap map;
    Source source;

    // Define a GTK source
#define GTK(wtype, colorSource, state)\
    source = Source(QGtk3Interface::QGtkWidget::gtk_ ##wtype,\
                    QGtk3Interface::QGtkColorSource::colorSource, GTK_STATE_FLAG_ ##state)

    // Define a modified source
#define LIGHTER(group, role, lighter)\
    source = Source(QPalette::group, QPalette::role,\
                    Qt::ColorScheme::Unknown, lighter)
#define MODIFY(group, role, red, green, blue)\
    source = Source(QPalette::group, QPalette::role,\
                    Qt::ColorScheme::Unknown, red, green, blue)

    // Define fixed source
#define FIX(color) source = FixedSource(color);

    // Add the source to a target brush
    // Use default Qt::ColorScheme::Unknown, if no color scheme was specified
#define ADD_2(group, role) map.insert(TargetBrush(QPalette::group, QPalette::role), source);
#define ADD_3(group, role, app) map.insert(TargetBrush(QPalette::group, QPalette::role,\
    Qt::ColorScheme::app), source);
#define ADD_X(x, group, role, app, FUNC, ...) FUNC
#define ADD(...) ADD_X(,##__VA_ARGS__, ADD_3(__VA_ARGS__), ADD_2(__VA_ARGS__))
    // Save target brushes to a palette type
#define SAVE(palette) m_palettes.insert(QPlatformTheme::palette, map)
    // Clear brushes to start next palette
#define CLEAR map.clear()

    /*
       Macro usage:

       1. Define a source
       GTK(QGtkWidget, QGtkColorSource, GTK_STATE_FLAG)
       Fetch the color from a GtkWidget, related to a source and a state.

       LIGHTER(ColorGroup, ColorROle, lighter)
       Use a color of the same QPalette related to ColorGroup and ColorRole.
       Make the color lighter (if lighter >100) or darker (if lighter < 100)

       MODIFY(ColorGroup, ColorRole, red, green, blue)
       Use a color of the same QPalette related to ColorGroup and ColorRole.
       Modify it by adding red, green, blue.

       FIX(const QBrush &)
       Use a fixed brush without querying GTK

       2. Define the target
       Use ADD(ColorGroup, ColorRole) to use the defined source for the
       color group / role in the current palette.

       Use ADD(ColorGroup, ColorRole, ColorScheme) to use the defined source
       only for a specific color scheme

       3. Save mapping
       Save the defined mappings for a specific palette.
       If a mapping entry does not cover all color groups and roles of a palette,
       the system palette will be used for the remaining values.
       If the system palette does not have all combination of color groups and roles,
       the remaining ones will be populated by a hard coded fusion-style like palette.

       4. Clear mapping
       Use CLEAR to clear the mapping and begin a new one.
     */


    // System palette
    {
        // background color and calculate derivates
        GTK(Default, Background, INSENSITIVE);
        ADD(All, Window);
        ADD(All, Button);
        ADD(All, Base);
        LIGHTER(Normal, Window, 125);
        ADD(Normal, Light);
        ADD(Inactive, Light);
        LIGHTER(Normal, Window, 70);
        ADD(Normal, Shadow);
        LIGHTER(Normal, Window, 80);
        ADD(Normal, Dark);
        ADD(Inactive, Dark)

        GTK(button, Foreground, ACTIVE);
        ADD(Inactive, WindowText);
        LIGHTER(Normal, WindowText, 50);
        ADD(Disabled, Text);
        ADD(Disabled, WindowText);
        ADD(Disabled, ButtonText);

        GTK(button, Text, NORMAL);
        ADD(Inactive, ButtonText);

        // special background colors
        GTK(Default, Background, SELECTED);
        ADD(Disabled, Highlight);
        ADD(Normal, Highlight);
        ADD(Inactive, Highlight);

        GTK(entry, Foreground, SELECTED);
        ADD(Normal, HighlightedText);
        ADD(Inactive, HighlightedText);

        // text color and friends
        GTK(entry, Text, NORMAL);
        ADD(Normal, ButtonText);
        ADD(Normal, WindowText);
        ADD(Disabled, HighlightedText);

        GTK(Default, Text, NORMAL);
        ADD(Normal, Text);
        ADD(Inactive, Text);
        ADD(Normal, HighlightedText);
        LIGHTER(Normal, Base, 93);
        ADD(All, AlternateBase);

        GTK(Default, Foreground, NORMAL);
        MODIFY(Normal, Text, 100, 100, 100);
        ADD(All, PlaceholderText, Light);
        MODIFY(Normal, Text, -100, -100, -100);
        ADD(All, PlaceholderText, Dark);

        // Light, midlight, dark, mid, shadow colors
        LIGHTER(Normal, Button, 125);
        ADD(All, Light)
        LIGHTER(Normal, Button, 113);
        ADD(All, Midlight)
        LIGHTER(Normal, Button, 113);
        ADD(All, Mid)
        LIGHTER(Normal, Button, 87);
        ADD(All, Dark)
        LIGHTER(Normal, Button, 5);
        ADD(All, Shadow)

        SAVE(SystemPalette);
        CLEAR;
    }

    // Label and TabBar Palette
    {
        GTK(entry, Text, NORMAL);
        ADD(Normal, WindowText);
        ADD(Inactive, WindowText);

        SAVE(LabelPalette);
        SAVE(TabBarPalette);
        CLEAR;
    }

    // Checkbox and RadioButton Palette
    {
        GTK(button, Text, ACTIVE);
        ADD(Normal, Base, Dark);
        ADD(Inactive, WindowText, Dark);

        GTK(Default, Foreground, NORMAL);
        ADD(All, Text);

        GTK(Default, Background, NORMAL);
        ADD(All, Base);

        GTK(button, Text, NORMAL);
        ADD(Normal, Base, Light);
        ADD(Inactive, WindowText, Light);

        SAVE(CheckBoxPalette);
        SAVE(RadioButtonPalette);
        CLEAR;
    }

    // ComboBox, GroupBox & Frame Palette
    {
        GTK(combo_box, Text, NORMAL);
        ADD(Normal, ButtonText, Dark);
        ADD(Normal, Text, Dark);
        ADD(Inactive, WindowText, Dark);

        GTK(combo_box, Text, ACTIVE);
        ADD(Normal, ButtonText, Light);
        ADD(Normal, Text, Light);
        ADD(Inactive, WindowText, Light);

        SAVE(ComboBoxPalette);
        SAVE(GroupBoxPalette);
        CLEAR;
    }

    // MenuBar Palette
    {
        GTK(Default, Text, ACTIVE);
        ADD(Normal, ButtonText);
        SAVE(MenuPalette);
        CLEAR;
    }

    // LineEdit Palette
    {
        GTK(Default, Background, NORMAL);
        ADD(All, Base);
        SAVE(TextLineEditPalette);
        CLEAR;
    }

#undef GTK
#undef REC
#undef FIX
#undef ADD
#undef ADD_2
#undef ADD_3
#undef ADD_X
#undef SAVE
#undef LOAD
}

QT_END_NAMESPACE
