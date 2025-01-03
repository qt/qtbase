// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfreetypefontdatabase_p.h"

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformscreen.h>

#include <QtCore/QFile>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDir>
#include <QtCore/QtEndian>

#undef QT_NO_FREETYPE
#include "qfontengine_ft_p.h"

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H
#include FT_ERRORS_H

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

void QFreeTypeFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();
    QDir dir(fontpath);

    if (!dir.exists()) {
        qWarning("QFontDatabase: Cannot find font directory %s.\n"
                 "Note that Qt no longer ships fonts. Deploy some (from https://dejavu-fonts.github.io/ for example) or switch to fontconfig.",
                 qPrintable(fontpath));
        return;
    }

    static const QString nameFilters[] = {
        u"*.ttf"_s,
        u"*.pfa"_s,
        u"*.pfb"_s,
        u"*.otf"_s,
    };

    const auto fis = dir.entryInfoList(QStringList::fromReadOnlyData(nameFilters), QDir::Files);
    for (const QFileInfo &fi : fis) {
        const QByteArray file = QFile::encodeName(fi.absoluteFilePath());
        QFreeTypeFontDatabase::addTTFile(QByteArray(), file);
    }
}

QFontEngine *QFreeTypeFontDatabase::fontEngine(const QFontDef &fontDef, void *usrPtr)
{
    FontFile *fontfile = static_cast<FontFile *>(usrPtr);
    QFontEngine::FaceId faceId;
    faceId.filename = QFile::encodeName(fontfile->fileName);
    faceId.index = fontfile->indexValue;

    return QFontEngineFT::create(fontDef, faceId, fontfile->data);
}

QFontEngine *QFreeTypeFontDatabase::fontEngine(const QByteArray &fontData, qreal pixelSize,
                                                QFont::HintingPreference hintingPreference)
{
    return QFontEngineFT::create(fontData, pixelSize, hintingPreference);
}

QStringList QFreeTypeFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *applicationFont)
{
    return QFreeTypeFontDatabase::addTTFile(fontData, fileName.toLocal8Bit(), applicationFont);
}

void QFreeTypeFontDatabase::releaseHandle(void *handle)
{
    FontFile *file = static_cast<FontFile *>(handle);
    delete file;
}

extern FT_Library qt_getFreetype();

QStringList QFreeTypeFontDatabase::addTTFile(const QByteArray &fontData, const QByteArray &file, QFontDatabasePrivate::ApplicationFont *applicationFont)
{
    FT_Library library = qt_getFreetype();

    int index = 0;
    int numFaces = 0;
    QStringList families;
    do {
        FT_Face face;
        FT_Error error;
        if (!fontData.isEmpty()) {
            error = FT_New_Memory_Face(library, (const FT_Byte *)fontData.constData(), fontData.size(), index, &face);
        } else {
            error = FT_New_Face(library, file.constData(), index, &face);
        }
        if (error != FT_Err_Ok) {
            qDebug() << "FT_New_Face failed with index" << index << ':' << Qt::hex << error;
            break;
        }
        numFaces = face->num_faces;

        QFont::Weight weight = QFont::Normal;

        QFont::Style style = QFont::StyleNormal;
        if (face->style_flags & FT_STYLE_FLAG_ITALIC)
            style = QFont::StyleItalic;

        if (face->style_flags & FT_STYLE_FLAG_BOLD)
            weight = QFont::Bold;

        bool fixedPitch = (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH);
        QSupportedWritingSystems writingSystems;
        // detect symbol fonts
        for (int i = 0; i < face->num_charmaps; ++i) {
            FT_CharMap cm = face->charmaps[i];
            if (cm->encoding == FT_ENCODING_ADOBE_CUSTOM
                    || cm->encoding == FT_ENCODING_MS_SYMBOL) {
                writingSystems.setSupported(QFontDatabase::Symbol);
                break;
            }
        }

        QFont::Stretch stretch = QFont::Unstretched;
        TT_OS2 *os2 = (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2);
        if (os2) {
            quint32 unicodeRange[4] = {
                quint32(os2->ulUnicodeRange1),
                quint32(os2->ulUnicodeRange2),
                quint32(os2->ulUnicodeRange3),
                quint32(os2->ulUnicodeRange4)
            };
            quint32 codePageRange[2] = {
                quint32(os2->ulCodePageRange1),
                quint32(os2->ulCodePageRange2)
            };

            writingSystems = QPlatformFontDatabase::writingSystemsFromTrueTypeBits(unicodeRange, codePageRange);

            if (os2->usWeightClass) {
                weight = static_cast<QFont::Weight>(os2->usWeightClass);
            } else if (os2->panose[2]) {
                int w = os2->panose[2];
                if (w <= 1)
                    weight = QFont::Thin;
                else if (w <= 2)
                    weight = QFont::ExtraLight;
                else if (w <= 3)
                    weight = QFont::Light;
                else if (w <= 5)
                    weight = QFont::Normal;
                else if (w <= 6)
                    weight = QFont::Medium;
                else if (w <= 7)
                    weight = QFont::DemiBold;
                else if (w <= 8)
                    weight = QFont::Bold;
                else if (w <= 9)
                    weight = QFont::ExtraBold;
                else if (w <= 10)
                    weight = QFont::Black;
            }

            switch (os2->usWidthClass) {
            case 1:
                stretch = QFont::UltraCondensed;
                break;
            case 2:
                stretch = QFont::ExtraCondensed;
                break;
            case 3:
                stretch = QFont::Condensed;
                break;
            case 4:
                stretch = QFont::SemiCondensed;
                break;
            case 5:
                stretch = QFont::Unstretched;
                break;
            case 6:
                stretch = QFont::SemiExpanded;
                break;
            case 7:
                stretch = QFont::Expanded;
                break;
            case 8:
                stretch = QFont::ExtraExpanded;
                break;
            case 9:
                stretch = QFont::UltraExpanded;
                break;
            }
        }

        QString family = QString::fromLatin1(face->family_name);
        FontFile *fontFile = new FontFile{
            QFile::decodeName(file),
            index,
            fontData
        };

        QString styleName = QString::fromLatin1(face->style_name);

        if (applicationFont != nullptr) {
            QFontDatabasePrivate::ApplicationFont::Properties properties;
            properties.familyName = family;
            properties.styleName = styleName;
            properties.weight = weight;
            properties.stretch = stretch;
            properties.style = style;

            applicationFont->properties.append(properties);
        }

        registerFont(family, styleName, QString(), weight, style, stretch, true, true, 0, fixedPitch, writingSystems, fontFile);
        families.append(family);

        FT_Done_Face(face);
        ++index;
    } while (index < numFaces);
    return families;
}

QT_END_NAMESPACE
