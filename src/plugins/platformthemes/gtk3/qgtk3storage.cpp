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

// Set a brush from a source and resolve recursions
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
                                              source.rec.appearance), map);

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

// Find source for a recursion and take dark/light/unknown into consideration
QGtk3Storage::Source QGtk3Storage::brush(const TargetBrush &b, const BrushMap &map) const
{
#define FIND(brush) if (map.contains(brush))\
                        return map.value(brush)

    // Return exact match
    FIND(b);

    // unknown appearance can find anything
    if (b.appearance == Qt::Appearance::Unknown) {
        FIND(TargetBrush(b, Qt::Appearance::Dark));
        FIND(TargetBrush(b, Qt::Appearance::Light));
    }

    // Color group All can always be found
    if (b.colorGroup != QPalette::All)
        return brush(TargetBrush(QPalette::All, b.colorRole, b.appearance), map);

    // Brush not found
    return Source();
#undef FIND
}

// Create a simple standard palette
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

// Deliver a palette styled according to the current GTK Theme
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
        // - theme and source appearance match
        // - or either of them is unknown
        const auto appSource = i.key().appearance;
        const auto appTheme = appearance();
        const bool setBrush = (appSource == appTheme) ||
                              (appSource == Qt::Appearance::Unknown) ||
                              (appTheme == Qt::Appearance::Unknown);

        if (setBrush) {
            p.setBrush(i.key().colorGroup, i.key().colorRole, brush(source, brushes));
        }
    }

    m_paletteCache[type].emplace(p);
    if (type == QPlatformTheme::SystemPalette)
        qCDebug(lcQGtk3Interface) << "System Palette defined" << themeName() << appearance() << p;

    return &m_paletteCache[type].value();
}

const QFont *QGtk3Storage::font(QPlatformTheme::Font type) const
{
    if (m_fontCache[type].has_value())
        return &m_fontCache[type].value();

    m_fontCache[type].emplace(m_interface->font(type));
    return &m_fontCache[type].value();
}

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

QIcon QGtk3Storage::fileIcon(const QFileInfo &fileInfo) const
{
    return m_interface ? m_interface->fileIcon(fileInfo) : QIcon();
}

void QGtk3Storage::clear()
{
    m_appearance = Qt::Appearance::Unknown;
    m_palettes.clear();
    for (auto &cache : m_paletteCache)
        cache.reset();

    for (auto &cache : m_fontCache)
        cache.reset();
}

void QGtk3Storage::handleThemeChange()
{
    clear();
    populateMap();
    QWindowSystemInterface::handleThemeChange();
}

void QGtk3Storage::populateMap()
{
    static QString m_themeName;

    // Distiguish initialization, theme change or call without theme change
    const QString newThemeName = themeName();
    if (m_themeName == newThemeName)
        return;

    clear();

    // Derive appearance from theme name
    m_appearance = newThemeName.contains("dark"_L1, Qt::CaseInsensitive)
                   ? Qt::Appearance::Dark : Qt::Appearance::Light;

    if (m_themeName.isEmpty()) {
        qCDebug(lcQGtk3Interface) << "GTK theme initialized:" << newThemeName << m_appearance;
    } else {
        qCDebug(lcQGtk3Interface) << "GTK theme changed to:" << newThemeName << m_appearance;
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

bool QGtk3Storage::save(const QString &filename, QJsonDocument::JsonFormat f) const
{
    return QGtk3Json::save(savePalettes(), filename, f);
}

QJsonDocument QGtk3Storage::save() const
{
    return QGtk3Json::save(savePalettes());
}

bool QGtk3Storage::load(const QString &filename)
{
    return QGtk3Json::load(m_palettes, filename);
}

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
                    Qt::Appearance::Unknown, lighter)
#define MODIFY(group, role, red, green, blue)\
    source = Source(QPalette::group, QPalette::role,\
                    Qt::Appearance::Unknown, red, green, blue)

    // Define fixed source
#define FIX(color) source = FixedSource(color);

    // Add the source to a target brush
    // Use default Qt::Appearance::Unknown, if no appearance was specified
#define ADD_2(group, role) map.insert(TargetBrush(QPalette::group, QPalette::role), source);
#define ADD_3(group, role, app) map.insert(TargetBrush(QPalette::group, QPalette::role,\
    Qt::Appearance::app), source);
#define ADD_X(x, group, role, app, FUNC, ...) FUNC
#define ADD(...) ADD_X(,##__VA_ARGS__, ADD_3(__VA_ARGS__), ADD_2(__VA_ARGS__))
    // Save target brushes to a palette type
#define SAVE(palette) m_palettes.insert(QPlatformTheme::palette, map)
    // Clear brushes to start next palette
#define CLEAR map.clear()

    /*
     *  Macro ussage:
     *
     *  1. Define a source
     *
     *  GTK(QGtkWidget, QGtkColorSource, GTK_STATE_FLAG)
     *  Fetch the color from a GtkWidget, related to a source and a state.
     *
     *  LIGHTER(ColorGroup, ColorROle, lighter)
     *  Use a color of the same QPalette related to ColorGroup and ColorRole.
     *  Make the color lighter (if lighter >100) or darker (if lighter < 100)
     *
     *  MODIFY(ColorGroup, ColorRole, red, green, blue)
     *  Use a color of the same QPalette related to ColorGroup and ColorRole.
     *  Modify it by adding red, green, blue.
     *
     *  FIX(const QBrush &)
     *  Use a fixed brush without querying GTK
     *
     *  2. Define the target
     *
     *  Use ADD(ColorGroup, ColorRole) to use the defined source for the
     *  color group / role in the current palette.
     *
     *  Use ADD(ColorGroup, ColorRole, Appearance) to use the defined source
     *  only for a specific appearance
     *
     *  3. Save mapping
     *  Save the defined mappings for a specific palette.
     *  If a mapping entry does not cover all color groups and roles of a palette,
     *  the system palette will be used for the remaining values.
     *  If the system palette does not have all combination of color groups and roles,
     *  the remaining ones will be populated by a hard coded fusion-style like palette.
     *
     *  4. Clear mapping
     *  Use CLEAR to clear the mapping and begin a new one.
     */


    // System palette
    // background color and calculate derivates
    GTK(Default, Background, INSENSITIVE);
    ADD(Normal, Window);
    ADD(Normal, Button);
    ADD(Normal, Base);
    ADD(Inactive, Base);
    ADD(Inactive, Window);
    LIGHTER(Normal, Window, 125);
    ADD(Normal, Light);
    LIGHTER(Normal, Window, 70);
    ADD(Normal, Shadow);
    LIGHTER(Normal, Window, 80);
    ADD(Normal, Dark);
    GTK(button, Foreground, ACTIVE);
    ADD(Normal, WindowText);
    ADD(Inactive, WindowText);
    LIGHTER(Normal, WindowText, 50);
    ADD(Disabled, Text);
    ADD(Disabled, WindowText);
    ADD(Inactive, ButtonText);
    GTK(button, Text, NORMAL);
    ADD(Disabled, ButtonText);
    // special background colors
    GTK(Default, Background, SELECTED);
    ADD(Disabled, Highlight);
    ADD(Normal, Highlight);
    GTK(entry, Foreground, SELECTED);
    ADD(Normal, HighlightedText);
    GTK(entry, Background, ACTIVE);
    ADD(Inactive, HighlightedText);
    // text color and friends
    GTK(entry, Text, NORMAL);
    ADD(Normal, ButtonText);
    ADD(Normal, WindowText);
    ADD(Disabled, WindowText);
    ADD(Disabled, HighlightedText);
    GTK(Default, Text, NORMAL);
    ADD(Normal, Text);
    ADD(Inactive, Text);
    ADD(Normal, HighlightedText);
    LIGHTER(Normal, Base, 93);
    ADD(All, AlternateBase);
    GTK(Default, Foreground, NORMAL);
    ADD(All, ToolTipText);
    MODIFY(Normal, Text, 100, 100, 100);
    ADD(All, PlaceholderText, Light);
    MODIFY(Normal, Text, -100, -100, -100);
    ADD(All, PlaceholderText, Dark);
    SAVE(SystemPalette);
    CLEAR;

    // Checkbox and Radio Button
    GTK(button, Text, ACTIVE);
    ADD(Normal, Base, Dark);
    GTK(Default, Background, NORMAL);
    ADD(All, Base);
    GTK(button, Text, NORMAL);
    ADD(Normal, Base, Light);
    SAVE(CheckBoxPalette);
    SAVE(RadioButtonPalette);
    CLEAR;

    // ComboBox, GroupBox, Frame
    GTK(combo_box, Text, NORMAL);
    ADD(Normal, ButtonText, Dark);
    ADD(Normal, Text, Dark);
    GTK(combo_box, Text, ACTIVE);
    ADD(Normal, ButtonText, Light);
    ADD(Normal, Text, Light);
    SAVE(ComboBoxPalette);
    SAVE(GroupBoxPalette);
    CLEAR;

    // Menu bar
    GTK(Default, Text, ACTIVE);
    ADD(Normal, ButtonText);
    SAVE(MenuPalette);
    CLEAR;

    // LineEdit
    GTK(Default, Background, NORMAL);
    ADD(All, Base);
    SAVE(TextLineEditPalette);
    CLEAR;

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
