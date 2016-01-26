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

#include "qbasicfontdatabase_p.h"

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformscreen.h>

#include <QtCore/QFile>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDir>
#include <QtCore/QUuid>
#include <QtCore/QtEndian>

#undef QT_NO_FREETYPE
#include <QtGui/private/qfontengine_ft_p.h>
#include <QtGui/private/qfontengine_p.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H
#include FT_ERRORS_H

QT_BEGIN_NAMESPACE

void QBasicFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();
    QDir dir(fontpath);

    if (!dir.exists()) {
        qWarning("QFontDatabase: Cannot find font directory %s.\n"
                 "Note that Qt no longer ships fonts. Deploy some (from http://dejavu-fonts.org for example) or switch to fontconfig.",
                 qPrintable(fontpath));
        return;
    }

    QStringList nameFilters;
    nameFilters << QLatin1String("*.ttf")
                << QLatin1String("*.ttc")
                << QLatin1String("*.pfa")
                << QLatin1String("*.pfb")
                << QLatin1String("*.otf");

    foreach (const QFileInfo &fi, dir.entryInfoList(nameFilters, QDir::Files)) {
        const QByteArray file = QFile::encodeName(fi.absoluteFilePath());
        QBasicFontDatabase::addTTFile(QByteArray(), file);
    }
}

QFontEngine *QBasicFontDatabase::fontEngine(const QFontDef &fontDef, void *usrPtr)
{
    FontFile *fontfile = static_cast<FontFile *> (usrPtr);
    QFontEngine::FaceId fid;
    fid.filename = QFile::encodeName(fontfile->fileName);
    fid.index = fontfile->indexValue;

    bool antialias = !(fontDef.styleStrategy & QFont::NoAntialias);
    QFontEngineFT *engine = new QFontEngineFT(fontDef);
    QFontEngineFT::GlyphFormat format = QFontEngineFT::Format_Mono;
    if (antialias) {
        QFontEngine::SubpixelAntialiasingType subpixelType = subpixelAntialiasingTypeHint();
        if (subpixelType == QFontEngine::Subpixel_None || (fontDef.styleStrategy & QFont::NoSubpixelAntialias)) {
            format = QFontEngineFT::Format_A8;
            engine->subpixelType = QFontEngine::Subpixel_None;
        } else {
            format = QFontEngineFT::Format_A32;
            engine->subpixelType = subpixelType;
        }
    }

    if (!engine->init(fid, antialias, format) || engine->invalid()) {
        delete engine;
        engine = 0;
    } else {
        engine->setQtDefaultHintStyle(static_cast<QFont::HintingPreference>(fontDef.hintingPreference));
    }

    return engine;
}

namespace {

    class QFontEngineFTRawData: public QFontEngineFT
    {
    public:
        QFontEngineFTRawData(const QFontDef &fontDef) : QFontEngineFT(fontDef)
        {
        }

        void updateFamilyNameAndStyle()
        {
            fontDef.family = QString::fromLatin1(freetype->face->family_name);

            if (freetype->face->style_flags & FT_STYLE_FLAG_ITALIC)
                fontDef.style = QFont::StyleItalic;

            if (freetype->face->style_flags & FT_STYLE_FLAG_BOLD)
                fontDef.weight = QFont::Bold;
        }

        bool initFromData(const QByteArray &fontData)
        {
            FaceId faceId;
            faceId.filename = "";
            faceId.index = 0;
            faceId.uuid = QUuid::createUuid().toByteArray();

            return init(faceId, true, Format_None, fontData);
        }
    };

}

QFontEngine *QBasicFontDatabase::fontEngine(const QByteArray &fontData, qreal pixelSize,
                                                QFont::HintingPreference hintingPreference)
{
    QFontDef fontDef;
    fontDef.pixelSize = pixelSize;
    fontDef.hintingPreference = hintingPreference;

    QFontEngineFTRawData *fe = new QFontEngineFTRawData(fontDef);
    if (!fe->initFromData(fontData)) {
        delete fe;
        return 0;
    }

    fe->updateFamilyNameAndStyle();
    fe->setQtDefaultHintStyle(static_cast<QFont::HintingPreference>(fontDef.hintingPreference));

    return fe;
}

QStringList QBasicFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
    return QBasicFontDatabase::addTTFile(fontData, fileName.toLocal8Bit());
}

void QBasicFontDatabase::releaseHandle(void *handle)
{
    FontFile *file = static_cast<FontFile *>(handle);
    delete file;
}

extern FT_Library qt_getFreetype();

QStringList QBasicFontDatabase::addTTFile(const QByteArray &fontData, const QByteArray &file)
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
            qDebug() << "FT_New_Face failed with index" << index << ':' << hex << error;
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
                weight = QPlatformFontDatabase::weightFromInteger(os2->usWeightClass);
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
        }

        QString family = QString::fromLatin1(face->family_name);
        FontFile *fontFile = new FontFile;
        fontFile->fileName = QFile::decodeName(file);
        fontFile->indexValue = index;

        QFont::Stretch stretch = QFont::Unstretched;

        registerFont(family,QString::fromLatin1(face->style_name),QString(),weight,style,stretch,true,true,0,fixedPitch,writingSystems,fontFile);

        families.append(family);

        FT_Done_Face(face);
        ++index;
    } while (index < numFaces);
    return families;
}

QT_END_NAMESPACE
