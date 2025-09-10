// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QGTK3JSON_P_H
#define QGTK3JSON_P_H

#include <QtCore/QCache>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtGui/QGuiApplication>
#include <QtGui/QPalette>

#include <qpa/qplatformtheme.h>
#include "qgtk3interface_p.h"
#include "qgtk3storage_p.h"

#undef signals // Collides with GTK symbols
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

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

QT_BEGIN_NAMESPACE

class QGtk3Json
{
    Q_GADGET
private:
    QGtk3Json() {}

public:
    // Convert enums to strings
    static QLatin1String fromPalette(QPlatformTheme::Palette palette);
    static QLatin1String fromGtkState(GtkStateFlags type);
    static QLatin1String fromColor(const QColor &Color);
    static QLatin1String fromColorRole(QPalette::ColorRole role);
    static QLatin1String fromColorGroup(QPalette::ColorGroup group);
    static QLatin1String fromGdkSource(QGtk3Interface::QGtkColorSource source);
    static QLatin1String fromSourceType(QGtk3Storage::SourceType sourceType);
    static QLatin1String fromWidgetType(QGtk3Interface::QGtkWidget widgetType);
    static QLatin1String fromColorScheme(Qt::ColorScheme colorScheme);

    // Convert strings to enums
    static QPlatformTheme::Palette toPalette(const QString &palette);
    static GtkStateFlags toGtkState(const QString &type);
    static QColor toColor(const QString &Color);
    static QPalette::ColorRole toColorRole(const QString &role);
    static QPalette::ColorGroup toColorGroup(const QString &group);
    static QGtk3Interface::QGtkColorSource toGdkSource(const QString &source);
    static QGtk3Storage::SourceType toSourceType(const QString &sourceType);
    static QGtk3Interface::QGtkWidget toWidgetType(const QString &widgetType);
    static Qt::ColorScheme toColorScheme(const QString &colorScheme);

    // Json keys
    static constexpr QLatin1StringView cePalettes = "QtGtk3Palettes"_L1;
    static constexpr QLatin1StringView cePalette = "PaletteType"_L1;
    static constexpr QLatin1StringView ceGtkState = "GtkStateType"_L1;
    static constexpr QLatin1StringView ceGtkWidget = "GtkWidgetType"_L1;
    static constexpr QLatin1StringView ceColor = "Color"_L1;
    static constexpr QLatin1StringView ceColorRole = "ColorRole"_L1;
    static constexpr QLatin1StringView ceColorGroup = "ColorGroup"_L1;
    static constexpr QLatin1StringView ceGdkSource = "GdkSource"_L1;
    static constexpr QLatin1StringView ceSourceType = "SourceType"_L1;
    static constexpr QLatin1StringView ceLighter = "Lighter"_L1;
    static constexpr QLatin1StringView ceRed = "DeltaRed"_L1;
    static constexpr QLatin1StringView ceGreen = "DeltaGreen"_L1;
    static constexpr QLatin1StringView ceBlue =  "DeltaBlue"_L1;
    static constexpr QLatin1StringView ceWidth = "Width"_L1;
    static constexpr QLatin1StringView ceHeight = "Height"_L1;
    static constexpr QLatin1StringView ceBrush = "FixedBrush"_L1;
    static constexpr QLatin1StringView ceData = "SourceData"_L1;
    static constexpr QLatin1StringView ceBrushes = "Brushes"_L1;
    static constexpr QLatin1StringView ceColorScheme = "ColorScheme"_L1;

    // Save to a file
    static bool save(const QGtk3Storage::PaletteMap &map, const QString &fileName,
              QJsonDocument::JsonFormat format = QJsonDocument::Indented);

    // Save to a Json document
    static const QJsonDocument save(const QGtk3Storage::PaletteMap &map);

    // Load from a file
    static bool load(QGtk3Storage::PaletteMap &map, const QString &fileName);

    // Load from a Json document
    static bool load(QGtk3Storage::PaletteMap &map, const QJsonDocument &doc);
};

QT_END_NAMESPACE
#endif // QGTK3JSON_P_H
