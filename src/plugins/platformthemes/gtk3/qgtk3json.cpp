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
#include <QtCore/QFile>
#include <QMetaEnum>

QT_BEGIN_NAMESPACE

QLatin1String QGtk3Json::fromPalette(QPlatformTheme::Palette palette)
{
    return QLatin1String(QMetaEnum::fromType<QPlatformTheme::Palette>().valueToKey(static_cast<int>(palette)));
}

QLatin1String QGtk3Json::fromGtkState(GtkStateFlags state)
{
    return QGtk3Interface::fromGtkState(state);
}

QLatin1String fromColor(const QColor &color)
{
    return QLatin1String(QByteArray(color.name(QColor::HexRgb).toLatin1()));
}

QLatin1String QGtk3Json::fromColorRole(QPalette::ColorRole role)
{
    return QLatin1String(QMetaEnum::fromType<QPalette::ColorRole>().valueToKey(static_cast<int>(role)));
}

QLatin1String QGtk3Json::fromColorGroup(QPalette::ColorGroup group)
{
    return QLatin1String(QMetaEnum::fromType<QPalette::ColorGroup>().valueToKey(static_cast<int>(group)));
}

QLatin1String QGtk3Json::fromGdkSource(QGtk3Interface::QGtkColorSource source)
{
    return QLatin1String(QMetaEnum::fromType<QGtk3Interface::QGtkColorSource>().valueToKey(static_cast<int>(source)));
}

QLatin1String QGtk3Json::fromWidgetType(QGtk3Interface::QGtkWidget widgetType)
{
    return QLatin1String(QMetaEnum::fromType<QGtk3Interface::QGtkWidget>().valueToKey(static_cast<int>(widgetType)));
}

QLatin1String QGtk3Json::fromColorScheme(Qt::ColorScheme app)
{
    return QLatin1String(QMetaEnum::fromType<Qt::ColorScheme>().valueToKey(static_cast<int>(app)));
}

#define CONVERT(type, key, def)\
    bool ok;\
    const int intVal = QMetaEnum::fromType<type>().keyToValue(key.toLatin1().constData(), &ok);\
    return ok ? static_cast<type>(intVal) : type::def

Qt::ColorScheme QGtk3Json::toColorScheme(const QString &colorScheme)
{
    CONVERT(Qt::ColorScheme, colorScheme, Unknown);
}

QPlatformTheme::Palette QGtk3Json::toPalette(const QString &palette)
{
    CONVERT(QPlatformTheme::Palette, palette, NPalettes);
}

GtkStateFlags QGtk3Json::toGtkState(const QString &type)
{
    int i = QGtk3Interface::toGtkState(type);
    if (i < 0)
        return GTK_STATE_FLAG_NORMAL;
    return static_cast<GtkStateFlags>(i);
}

QColor toColor(const QStringView &color)
{
    return QColor::fromString(color);
}

QPalette::ColorRole QGtk3Json::toColorRole(const QString &role)
{
    CONVERT(QPalette::ColorRole, role, NColorRoles);
}

QPalette::ColorGroup QGtk3Json::toColorGroup(const QString &group)
{
    CONVERT(QPalette::ColorGroup, group, NColorGroups);
}

QGtk3Interface::QGtkColorSource QGtk3Json::toGdkSource(const QString &source)
{
    CONVERT(QGtk3Interface::QGtkColorSource, source, Background);
}

QLatin1String QGtk3Json::fromSourceType(QGtk3Storage::SourceType sourceType)
{
    return QLatin1String(QMetaEnum::fromType<QGtk3Storage::SourceType>().valueToKey(static_cast<int>(sourceType)));
}

QGtk3Storage::SourceType QGtk3Json::toSourceType(const QString &sourceType)
{
    CONVERT(QGtk3Storage::SourceType, sourceType, Invalid);
}

QGtk3Interface::QGtkWidget QGtk3Json::toWidgetType(const QString &widgetType)
{
    CONVERT(QGtk3Interface::QGtkWidget, widgetType, gtk_offscreen_window);
}

#undef CONVERT

bool QGtk3Json::save(const QGtk3Storage::PaletteMap &map, const QString &fileName,
                     QJsonDocument::JsonFormat format)
{
    QJsonDocument doc = save(map);
    if (doc.isEmpty()) {
        qWarning() << "Nothing to save to" << fileName;
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to open file" << fileName << "for writing.";
        return false;
    }

    if (!file.write(doc.toJson(format))) {
        qWarning() << "Unable to serialize Json document.";
        return false;
    }

    file.close();
    qInfo() << "Saved mapping data to" << fileName;
    return true;
}

const QJsonDocument QGtk3Json::save(const QGtk3Storage::PaletteMap &map)
{
    QJsonObject paletteObject;
    for (auto paletteIterator = map.constBegin(); paletteIterator != map.constEnd();
         ++paletteIterator) {
        const QGtk3Storage::BrushMap &bm = paletteIterator.value();
        QFlatMap<QPalette::ColorRole, QGtk3Storage::BrushMap> brushMaps;
        for (auto brushIterator = bm.constBegin(); brushIterator != bm.constEnd();
             ++brushIterator) {
            const QPalette::ColorRole role = brushIterator.key().colorRole;
            if (brushMaps.contains(role)) {
                brushMaps[role].insert(brushIterator.key(), brushIterator.value());
            } else {
                QGtk3Storage::BrushMap newMap;
                newMap.insert(brushIterator.key(), brushIterator.value());
                brushMaps.insert(role, newMap);
            }
        }

        QJsonObject brushArrayObject;
        for (auto brushMapIterator = brushMaps.constBegin();
             brushMapIterator != brushMaps.constEnd(); ++brushMapIterator) {

            QJsonArray brushArray;
            int brushIndex = 0;
            const QGtk3Storage::BrushMap &bm = brushMapIterator.value();
            for (auto brushIterator = bm.constBegin(); brushIterator != bm.constEnd();
                 ++brushIterator) {
                QJsonObject brushObject;
                const QGtk3Storage::TargetBrush tb = brushIterator.key();
                QGtk3Storage::Source s = brushIterator.value();
                brushObject.insert(ceColorGroup, fromColorGroup(tb.colorGroup));
                brushObject.insert(ceColorScheme, fromColorScheme(tb.colorScheme));
                brushObject.insert(ceSourceType, fromSourceType(s.sourceType));

                QJsonObject sourceObject;
                switch (s.sourceType) {
                case QGtk3Storage::SourceType::Gtk: {
                    sourceObject.insert(ceGtkWidget, fromWidgetType(s.gtk3.gtkWidgetType));
                    sourceObject.insert(ceGdkSource, fromGdkSource(s.gtk3.source));
                    sourceObject.insert(ceGtkState, fromGtkState(s.gtk3.state));
                    sourceObject.insert(ceWidth, s.gtk3.width);
                    sourceObject.insert(ceHeight, s.gtk3.height);
                }
                break;

                case QGtk3Storage::SourceType::Fixed: {
                        QJsonObject fixedObject;
                        fixedObject.insert(ceColor, s.fix.fixedBrush.color().name());
                        fixedObject.insert(ceWidth, s.fix.fixedBrush.texture().width());
                        fixedObject.insert(ceHeight, s.fix.fixedBrush.texture().height());
                        sourceObject.insert(ceBrush, fixedObject);
                    }
                    break;

                case QGtk3Storage::SourceType::Modified:{
                        sourceObject.insert(ceColorGroup, fromColorGroup(s.rec.colorGroup));
                        sourceObject.insert(ceColorRole, fromColorRole(s.rec.colorRole));
                        sourceObject.insert(ceColorScheme, fromColorScheme(s.rec.colorScheme));
                        sourceObject.insert(ceRed, s.rec.deltaRed);
                        sourceObject.insert(ceGreen, s.rec.deltaGreen);
                        sourceObject.insert(ceBlue, s.rec.deltaBlue);
                        sourceObject.insert(ceWidth, s.rec.width);
                        sourceObject.insert(ceHeight, s.rec.height);
                        sourceObject.insert(ceLighter, s.rec.lighter);
                    }
                    break;

                case QGtk3Storage::SourceType::Mixed: {
                        sourceObject.insert(ceColorGroup, fromColorGroup(s.mix.sourceGroup));
                        QJsonArray colorRoles;
                        colorRoles << fromColorRole(s.mix.colorRole1)
                                   << fromColorRole(s.mix.colorRole2);
                        sourceObject.insert(ceColorRole, colorRoles);
                    }
                    break;

                case QGtk3Storage::SourceType::Invalid:
                    break;
                }

                brushObject.insert(ceData, sourceObject);
                brushArray.insert(brushIndex, brushObject);
                ++brushIndex;
            }
            brushArrayObject.insert(fromColorRole(brushMapIterator.key()), brushArray);
        }
        paletteObject.insert(fromPalette(paletteIterator.key()), brushArrayObject);
    }

    QJsonObject top;
    top.insert(cePalettes, paletteObject);
    return paletteObject.keys().isEmpty() ? QJsonDocument() : QJsonDocument(top);
}

bool QGtk3Json::load(QGtk3Storage::PaletteMap &map, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcQGtk3Interface) << "Unable to open file:" << fileName;
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        qCWarning(lcQGtk3Interface) << "Unable to parse Json document from" << fileName
                                   << err.error << err.errorString();
        return false;
    }

    if (Q_LIKELY(load(map, doc))) {
        qInfo() << "GTK mapping successfully imported from" << fileName;
        return true;
    }

    qWarning() << "File" << fileName << "could not be loaded.";
    return false;
}

bool QGtk3Json::load(QGtk3Storage::PaletteMap &map, const QJsonDocument &doc)
{
#define GETSTR(obj, key)\
    if (!obj.contains(key)) {\
        qCInfo(lcQGtk3Interface) << key << "missing for palette" << paletteName\
                                  << ", Brush" << colorRoleName;\
        return false;\
    }\
    value = obj[key].toString()

#define GETINT(obj, key, var) GETSTR(obj, key);\
    if (!obj[key].isDouble()) {\
        qCInfo(lcQGtk3Interface) << key << "type mismatch" << value\
                                  << "is not an integer!"\
                                  << "(Palette" << paletteName << "), Brush" << colorRoleName;\
        return false;\
    }\
    const int var = obj[key].toInt()

    map.clear();
    const QJsonObject top(doc.object());
    if (doc.isEmpty() || top.isEmpty() || !top.contains(cePalettes)) {
        qCInfo(lcQGtk3Interface) << "Document does not contain Palettes.";
        return false;
    }

    const QStringList &paletteList = top[cePalettes].toObject().keys();
    for (const QString &paletteName : paletteList) {
        bool ok;
        const int intVal = QMetaEnum::fromType<QPlatformTheme::Palette>().keyToValue(paletteName
                                                                 .toLatin1().constData(), &ok);
        if (!ok) {
            qCInfo(lcQGtk3Interface) << "Invalid Palette name:" << paletteName;
            return false;
        }
        const QJsonObject &paletteObject = top[cePalettes][paletteName].toObject();
        const QStringList &brushList = paletteObject.keys();
        if (brushList.isEmpty()) {
            qCInfo(lcQGtk3Interface) << "Palette" << paletteName << "does not contain brushes";
            return false;
        }

        const QPlatformTheme::Palette paletteType = static_cast<QPlatformTheme::Palette>(intVal);
        QGtk3Storage::BrushMap brushes;
        const QStringList &colorRoles = paletteObject.keys();
        for (const QString &colorRoleName : colorRoles) {
            const int intVal = QMetaEnum::fromType<QPalette::ColorRole>().keyToValue(colorRoleName
                                                                    .toLatin1().constData(), &ok);
            if (!ok) {
                qCInfo(lcQGtk3Interface) << "Palette" << paletteName
                                          << "contains invalid color role" << colorRoleName;
                return false;
            }
            const QPalette::ColorRole colorRole = static_cast<QPalette::ColorRole>(intVal);
            const QJsonArray &brushArray = paletteObject[colorRoleName].toArray();
            for (int brushIndex = 0; brushIndex < brushArray.size(); ++brushIndex) {
                const QJsonObject brushObject = brushArray.at(brushIndex).toObject();
                if (brushObject.isEmpty()) {
                    qCInfo(lcQGtk3Interface) << "Brush specification missing at for palette"
                                              << paletteName << ", Brush" << colorRoleName;
                    return false;
                }

                QString value;
                GETSTR(brushObject, ceSourceType);
                const QGtk3Storage::SourceType sourceType = toSourceType(value);
                GETSTR(brushObject, ceColorGroup);
                const QPalette::ColorGroup colorGroup = toColorGroup(value);
                GETSTR(brushObject, ceColorScheme);
                const Qt::ColorScheme colorScheme = toColorScheme(value);
                QGtk3Storage::TargetBrush tb(colorGroup, colorRole, colorScheme);
                QGtk3Storage::Source s;

                if (!brushObject.contains(ceData) || !brushObject[ceData].isObject()) {
                    qCInfo(lcQGtk3Interface) << "Source specification missing for palette" << paletteName
                                                  << "Brush" << colorRoleName;
                    return false;
                }
                const QJsonObject &sourceObject = brushObject[ceData].toObject();

                switch (sourceType) {
                case QGtk3Storage::SourceType::Gtk: {
                        GETSTR(sourceObject, ceGdkSource);
                        const QGtk3Interface::QGtkColorSource gtkSource = toGdkSource(value);
                        GETSTR(sourceObject, ceGtkState);
                        const GtkStateFlags gtkState = toGtkState(value);
                        GETSTR(sourceObject, ceGtkWidget);
                        const QGtk3Interface::QGtkWidget widgetType = toWidgetType(value);
                        GETINT(sourceObject, ceHeight, height);
                        GETINT(sourceObject, ceWidth, width);
                        s = QGtk3Storage::Source(widgetType, gtkSource, gtkState, width, height);
                    }
                    break;

                case QGtk3Storage::SourceType::Fixed: {
                        if (!sourceObject.contains(ceBrush)) {
                            qCInfo(lcQGtk3Interface) << "Fixed brush specification missing for palette" << paletteName
                                                      << "Brush" << colorRoleName;
                            return false;
                        }
                        const QJsonObject &fixedSource = sourceObject[ceBrush].toObject();
                        GETINT(fixedSource, ceWidth, width);
                        GETINT(fixedSource, ceHeight, height);
                        GETSTR(fixedSource, ceColor);
                        const QColor color(value);
                        if (!color.isValid()) {
                            qCInfo(lcQGtk3Interface) << "Color" << value << "can't be parsed for:" << paletteName
                                                      << "Brush" << colorRoleName;
                            return false;
                        }
                        const QBrush fixedBrush = (width < 0 && height < 0)
                                                  ? QBrush(color, QPixmap(width, height))
                                                  : QBrush(color);
                        s = QGtk3Storage::Source(fixedBrush);
                    }
                    break;

                case QGtk3Storage::SourceType::Modified: {
                        GETSTR(sourceObject, ceColorGroup);
                        const QPalette::ColorGroup colorGroup = toColorGroup(value);
                        GETSTR(sourceObject, ceColorRole);
                        const QPalette::ColorRole colorRole = toColorRole(value);
                        GETSTR(sourceObject, ceColorScheme);
                        const Qt::ColorScheme colorScheme = toColorScheme(value);
                        GETINT(sourceObject, ceLighter, lighter);
                        GETINT(sourceObject, ceRed, red);
                        GETINT(sourceObject, ceBlue, blue);
                        GETINT(sourceObject, ceGreen, green);
                        s = QGtk3Storage::Source(colorGroup, colorRole, colorScheme,
                                                 lighter, red, green, blue);
                    }
                    break;

                case QGtk3Storage::SourceType::Mixed: {
                        if (!sourceObject[ceColorRole].isArray()) {
                            qCInfo(lcQGtk3Interface) << "Mixed brush missing the array of color roles for palette:" << paletteName
                                                     << "Brush" << colorRoleName;
                            return false;
                        }
                        QJsonArray colorRoles = sourceObject[ceColorRole].toArray();
                        if (colorRoles.size() < 2) {
                            qCInfo(lcQGtk3Interface) << "Mixed brush missing enough color roles for palette" << paletteName
                                                     << "Brush" << colorRoleName;
                            return false;
                        }
                        const QPalette::ColorRole colorRole1 = toColorRole(colorRoles[0].toString());
                        const QPalette::ColorRole colorRole2 = toColorRole(colorRoles[1].toString());
                        GETSTR(sourceObject, ceColorGroup);
                        const QPalette::ColorGroup sourceGroup = toColorGroup(value);
                        s = QGtk3Storage::Source(sourceGroup, colorRole1, colorRole2);
                    }
                    break;

                case QGtk3Storage::SourceType::Invalid:
                    qCInfo(lcQGtk3Interface) << "Invalid source type for palette" << paletteName
                                              << "Brush." << colorRoleName;
                    return false;
                }
                brushes.insert(tb, s);
            }
        }
        map.insert(paletteType, brushes);
    }
    return true;
}

QT_END_NAMESPACE

