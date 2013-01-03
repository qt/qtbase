/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

#define SimplifiedChineseCsbBit 18
#define TraditionalChineseCsbBit 20
#define JapaneseCsbBit 17
#define KoreanCsbBit 21

static int requiredUnicodeBits[QFontDatabase::WritingSystemsCount][2] = {
        // Any,
    { 127, 127 },
        // Latin,
    { 0, 127 },
        // Greek,
    { 7, 127 },
        // Cyrillic,
    { 9, 127 },
        // Armenian,
    { 10, 127 },
        // Hebrew,
    { 11, 127 },
        // Arabic,
    { 13, 127 },
        // Syriac,
    { 71, 127 },
    //Thaana,
    { 72, 127 },
    //Devanagari,
    { 15, 127 },
    //Bengali,
    { 16, 127 },
    //Gurmukhi,
    { 17, 127 },
    //Gujarati,
    { 18, 127 },
    //Oriya,
    { 19, 127 },
    //Tamil,
    { 20, 127 },
    //Telugu,
    { 21, 127 },
    //Kannada,
    { 22, 127 },
    //Malayalam,
    { 23, 127 },
    //Sinhala,
    { 73, 127 },
    //Thai,
    { 24, 127 },
    //Lao,
    { 25, 127 },
    //Tibetan,
    { 70, 127 },
    //Myanmar,
    { 74, 127 },
        // Georgian,
    { 26, 127 },
        // Khmer,
    { 80, 127 },
        // SimplifiedChinese,
    { 126, 127 },
        // TraditionalChinese,
    { 126, 127 },
        // Japanese,
    { 126, 127 },
        // Korean,
    { 56, 127 },
        // Vietnamese,
    { 0, 127 }, // same as latin1
        // Other,
    { 126, 127 },
        // Ogham,
    { 78, 127 },
        // Runic,
    { 79, 127 },
        // Nko,
    { 14, 127 },
};

typedef struct {
    quint16 majorVersion;
    quint16 minorVersion;
    quint16 numTables;
    quint16 searchRange;
    quint16 entrySelector;
    quint16 rangeShift;
} OFFSET_TABLE;

typedef struct {
    quint32 tag;
    quint32 checkSum;
    quint32 offset;
    quint32 length;
} TABLE_DIRECTORY;

typedef struct {
    quint16 fontSelector;
    quint16 nrCount;
    quint16 storageOffset;
} NAME_TABLE_HEADER;

typedef struct {
    quint16 platformID;
    quint16 encodingID;
    quint16 languageID;
    quint16 nameID;
    quint16 stringLength;
    quint16 stringOffset;
} NAME_RECORD;

QSupportedWritingSystems QBasicFontDatabase::determineWritingSystemsFromTrueTypeBits(quint32 unicodeRange[4], quint32 codePageRange[2])
{
    QSupportedWritingSystems writingSystems;
    bool hasScript = false;

    int i;
    for(i = 0; i < QFontDatabase::WritingSystemsCount; i++) {
        int bit = requiredUnicodeBits[i][0];
        int index = bit/32;
        int flag =  1 << (bit&31);
        if (bit != 126 && unicodeRange[index] & flag) {
            bit = requiredUnicodeBits[i][1];
            index = bit/32;

            flag =  1 << (bit&31);
            if (bit == 127 || unicodeRange[index] & flag) {
                writingSystems.setSupported(QFontDatabase::WritingSystem(i));
                hasScript = true;
                // qDebug("font %s: index=%d, flag=%8x supports script %d", familyName.latin1(), index, flag, i);
            }
        }
    }
    if(codePageRange[0] & (1 << SimplifiedChineseCsbBit)) {
        writingSystems.setSupported(QFontDatabase::SimplifiedChinese);
        hasScript = true;
        //qDebug("font %s supports Simplified Chinese", familyName.latin1());
    }
    if(codePageRange[0] & (1 << TraditionalChineseCsbBit)) {
        writingSystems.setSupported(QFontDatabase::TraditionalChinese);
        hasScript = true;
        //qDebug("font %s supports Traditional Chinese", familyName.latin1());
    }
    if(codePageRange[0] & (1 << JapaneseCsbBit)) {
        writingSystems.setSupported(QFontDatabase::Japanese);
        hasScript = true;
        //qDebug("font %s supports Japanese", familyName.latin1());
    }
    if(codePageRange[0] & (1 << KoreanCsbBit)) {
        writingSystems.setSupported(QFontDatabase::Korean);
        hasScript = true;
        //qDebug("font %s supports Korean", familyName.latin1());
    }
    if (!hasScript)
        writingSystems.setSupported(QFontDatabase::Symbol);

    return writingSystems;
}

static inline bool scriptRequiresOpenType(int script)
{
    return ((script >= QChar::Script_Syriac && script <= QChar::Script_Sinhala)
            || script == QChar::Script_Khmer || script == QChar::Script_Nko);
}

void QBasicFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();

    if(!QFile::exists(fontpath)) {
        qFatal("QFontDatabase: Cannot find font directory %s - is Qt installed correctly?",
               qPrintable(fontpath));
    }

    QDir dir(fontpath);
    dir.setNameFilters(QStringList() << QLatin1String("*.ttf")
                       << QLatin1String("*.ttc") << QLatin1String("*.pfa")
                       << QLatin1String("*.pfb"));
    dir.refresh();
    for (int i = 0; i < int(dir.count()); ++i) {
        const QByteArray file = QFile::encodeName(dir.absoluteFilePath(dir[i]));
//        qDebug() << "looking at" << file;
        addTTFile(QByteArray(), file);
    }
}

QFontEngine *QBasicFontDatabase::fontEngine(const QFontDef &fontDef, QChar::Script script, void *usrPtr)
{
    QFontEngineFT *engine;
    FontFile *fontfile = static_cast<FontFile *> (usrPtr);
    QFontEngine::FaceId fid;
    fid.filename = fontfile->fileName.toLocal8Bit();
    fid.index = fontfile->indexValue;
    engine = new QFontEngineFT(fontDef);

    bool antialias = !(fontDef.styleStrategy & QFont::NoAntialias);
    QFontEngineFT::GlyphFormat format = antialias? QFontEngineFT::Format_A8 : QFontEngineFT::Format_Mono;
    if (!engine->init(fid,antialias,format)) {
        delete engine;
        engine = 0;
        return engine;
    }
    if (engine->invalid()) {
        delete engine;
        engine = 0;
    } else if (scriptRequiresOpenType(script)) {
        HB_Face hbFace = engine->initializedHarfbuzzFace();
        if (!hbFace || !hbFace->supported_scripts[script]) {
            delete engine;
            engine = 0;
        }
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

    QFontEngineFTRawData *fe = new QFontEngineFTRawData(fontDef);
    if (!fe->initFromData(fontData)) {
        delete fe;
        return 0;
    }

    fe->updateFamilyNameAndStyle();

    switch (hintingPreference) {
    case QFont::PreferNoHinting:
        fe->setDefaultHintStyle(QFontEngineFT::HintNone);
        break;
    case QFont::PreferFullHinting:
        fe->setDefaultHintStyle(QFontEngineFT::HintFull);
        break;
    case QFont::PreferVerticalHinting:
        fe->setDefaultHintStyle(QFontEngineFT::HintLight);
        break;
    default:
        // Leave it as it is
        break;
    }

    return fe;
}

QStringList QBasicFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    Q_UNUSED(family);
    Q_UNUSED(style);
    Q_UNUSED(script);
    Q_UNUSED(styleHint);
    return QStringList();
}

QStringList QBasicFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
    return addTTFile(fontData,fileName.toLocal8Bit());
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
            qDebug() << "FT_New_Face failed with index" << index << ":" << hex << error;
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

            writingSystems = determineWritingSystemsFromTrueTypeBits(unicodeRange, codePageRange);

            if (os2->usWeightClass == 0)
                ;
            else if (os2->usWeightClass < 350)
                weight = QFont::Light;
            else if (os2->usWeightClass < 450)
                weight = QFont::Normal;
            else if (os2->usWeightClass < 650)
                weight = QFont::DemiBold;
            else if (os2->usWeightClass < 750)
                weight = QFont::Bold;
            else if (os2->usWeightClass < 1000)
                weight = QFont::Black;

            if (os2->panose[2] >= 2) {
                int w = os2->panose[2];
                if (w <= 3)
                    weight = QFont::Light;
                else if (w <= 5)
                    weight = QFont::Normal;
                else if (w <= 7)
                    weight = QFont::DemiBold;
                else if (w <= 8)
                    weight = QFont::Bold;
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

QString QBasicFontDatabase::fontNameFromTTFile(const QString &filename)
{
    QFile f(filename);
    QString retVal;
    qint64 bytesRead;
    qint64 bytesToRead;

    if (f.open(QIODevice::ReadOnly)) {
        OFFSET_TABLE ttOffsetTable;
        bytesToRead = sizeof(OFFSET_TABLE);
        bytesRead = f.read((char*)&ttOffsetTable, bytesToRead);
        if (bytesToRead != bytesRead)
            return retVal;
        ttOffsetTable.numTables = qFromBigEndian(ttOffsetTable.numTables);
        ttOffsetTable.majorVersion = qFromBigEndian(ttOffsetTable.majorVersion);
        ttOffsetTable.minorVersion = qFromBigEndian(ttOffsetTable.minorVersion);

        if (ttOffsetTable.majorVersion != 1 || ttOffsetTable.minorVersion != 0)
            return retVal;

        TABLE_DIRECTORY tblDir;
        bool found = false;

        for (int i = 0; i < ttOffsetTable.numTables; i++) {
            bytesToRead = sizeof(TABLE_DIRECTORY);
            bytesRead = f.read((char*)&tblDir, bytesToRead);
            if (bytesToRead != bytesRead)
                return retVal;
            if (qFromBigEndian(tblDir.tag) == MAKE_TAG('n', 'a', 'm', 'e')) {
                found = true;
                tblDir.length = qFromBigEndian(tblDir.length);
                tblDir.offset = qFromBigEndian(tblDir.offset);
                break;
            }
        }

        if (found) {
            f.seek(tblDir.offset);
            NAME_TABLE_HEADER ttNTHeader;
            bytesToRead = sizeof(NAME_TABLE_HEADER);
            bytesRead = f.read((char*)&ttNTHeader, bytesToRead);
            if (bytesToRead != bytesRead)
                return retVal;
            ttNTHeader.nrCount = qFromBigEndian(ttNTHeader.nrCount);
            ttNTHeader.storageOffset = qFromBigEndian(ttNTHeader.storageOffset);
            NAME_RECORD ttRecord;
            found = false;

            for (int i = 0; i < ttNTHeader.nrCount; i++) {
                bytesToRead = sizeof(NAME_RECORD);
                bytesRead = f.read((char*)&ttRecord, bytesToRead);
                if (bytesToRead != bytesRead)
                    return retVal;
                ttRecord.nameID = qFromBigEndian(ttRecord.nameID);
                if (ttRecord.nameID == 1) {
                    ttRecord.stringLength = qFromBigEndian(ttRecord.stringLength);
                    ttRecord.stringOffset = qFromBigEndian(ttRecord.stringOffset);
                    int nPos = f.pos();
                    f.seek(tblDir.offset + ttRecord.stringOffset + ttNTHeader.storageOffset);

                    QByteArray nameByteArray = f.read(ttRecord.stringLength);
                    if (!nameByteArray.isEmpty()) {
                        if (ttRecord.encodingID == 256 || ttRecord.encodingID == 768) {
                            //This is UTF-16 in big endian
                            int stringLength = ttRecord.stringLength / 2;
                            retVal.resize(stringLength);
                            QChar *data = retVal.data();
                            const ushort *srcData = (const ushort *)nameByteArray.data();
                            for (int i = 0; i < stringLength; ++i)
                                data[i] = qFromBigEndian(srcData[i]);
                            return retVal;
                        } else if (ttRecord.encodingID == 0) {
                            //This is Latin1
                            retVal = QString::fromLatin1(nameByteArray);
                        } else {
                            qWarning("Could not retrieve Font name from file: %s", qPrintable(QDir::toNativeSeparators(filename)));
                        }
                        break;
                    }
                    f.seek(nPos);
                }
            }
        }
        f.close();
    }
    return retVal;
}

QT_END_NAMESPACE
