/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformfontdatabase_qpa.h"
#include <QtGui/private/qfontengine_p.h>
#include <QtGui/private/qfontengine_qpa_p.h>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

extern void qt_registerFont(const QString &familyname, const QString &foundryname, int weight,
                                         QFont::Style style, int stretch, bool antialiased,bool scalable, int pixelSize,
                                         const QSupportedWritingSystems &writingSystems, void *hanlde);

void QPlatformFontDatabase::registerQPF2Font(const QByteArray &dataArray, void *handle)
{
    if (dataArray.size() == 0)
        return;

    const uchar *data = reinterpret_cast<const uchar *>(dataArray.constData());
    if (QFontEngineQPA::verifyHeader(data, dataArray.size())) {
        QString fontName = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_FontName).toString();
        int pixelSize = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_PixelSize).toInt();
        QVariant weight = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_Weight);
        QVariant style = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_Style);
        QByteArray writingSystemBits = QFontEngineQPA::extractHeaderField(data, QFontEngineQPA::Tag_WritingSystems).toByteArray();

        if (!fontName.isEmpty() && pixelSize) {
            QFont::Weight fontWeight = QFont::Normal;
            if (weight.type() == QVariant::Int || weight.type() == QVariant::UInt)
                fontWeight = QFont::Weight(weight.toInt());

            QFont::Style fontStyle = static_cast<QFont::Style>(style.toInt());

            QSupportedWritingSystems writingSystems;
            for (int i = 0; i < writingSystemBits.count(); ++i) {
                uchar currentByte = writingSystemBits.at(i);
                for (int j = 0; j < 8; ++j) {
                    if (currentByte & 1)
                        writingSystems.setSupported(QFontDatabase::WritingSystem(i * 8 + j));
                    currentByte >>= 1;
                }
            }
            QFont::Stretch stretch = QFont::Unstretched;
            registerFont(fontName,QString(),fontWeight,fontStyle,stretch,true,false,pixelSize,writingSystems,handle);
        }
    } else {
        qDebug() << "header verification of QPF2 font failed. maybe it is corrupt?";
    }
}

void QPlatformFontDatabase::registerFont(const QString &familyname, const QString &foundryname, QFont::Weight weight,
                                         QFont::Style style, QFont::Stretch stretch, bool antialiased, bool scalable, int pixelSize,
                                         const QSupportedWritingSystems &writingSystems, void *usrPtr)
{
    if (scalable)
        pixelSize = 0;
    qt_registerFont(familyname,foundryname,weight,style,stretch,antialiased,scalable,pixelSize,writingSystems,usrPtr);
}

class QWritingSystemsPrivate
{
public:
    QWritingSystemsPrivate()
        : ref(1)
        , vector(QFontDatabase::WritingSystemsCount,false)
    {
    }

    QWritingSystemsPrivate(const QWritingSystemsPrivate *other)
        : ref(1)
        , vector(other->vector)
    {
    }

    QAtomicInt ref;
    QVector<bool> vector;
};

QSupportedWritingSystems::QSupportedWritingSystems()
{
    d = new QWritingSystemsPrivate;
}

QSupportedWritingSystems::QSupportedWritingSystems(const QSupportedWritingSystems &other)
{
    d = other.d;
    d->ref.ref();
}

QSupportedWritingSystems &QSupportedWritingSystems::operator=(const QSupportedWritingSystems &other)
{
    if (d != other.d) {
        other.d->ref.ref();
        if (!d->ref.deref())
            delete d;
        d = other.d;
    }
    return *this;
}

QSupportedWritingSystems::~QSupportedWritingSystems()
{
    if (!d->ref.deref())
        delete d;
}

void QSupportedWritingSystems::detach()
{
    if (d->ref != 1) {
        QWritingSystemsPrivate *newd = new QWritingSystemsPrivate(d);
        if (!d->ref.deref())
            delete d;
        d = newd;
    }
}

void QSupportedWritingSystems::setSupported(QFontDatabase::WritingSystem writingSystem, bool support)
{
    detach();
    d->vector[writingSystem] = support;
}

bool QSupportedWritingSystems::supported(QFontDatabase::WritingSystem writingSystem) const
{
    return d->vector.at(writingSystem);
}

/*!
    \class QSupportedWritingSystems
    \brief The QSupportedWritingSystems class is used when registering fonts with the internal Qt
    fontdatabase
    \ingroup painting

    Its to provide an easy to use interface for indicating what writing systems a specific font
    supports.

*/

/*!
  This function is called once at startup by Qts internal fontdatabase. Reimplement this function
  in a subclass for a convenient place to initialise the internal fontdatabase.

  The default implementation looks in the fontDir() location and registers all qpf2 fonts.
*/
void QPlatformFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();

    if(!QFile::exists(fontpath)) {
        qFatal("QFontDatabase: Cannot find font directory %s - is Qt installed correctly?",
               qPrintable(fontpath));
    }

    QDir dir(fontpath);
    dir.setNameFilters(QStringList() << QLatin1String("*.qpf2"));
    dir.refresh();
    for (int i = 0; i < int(dir.count()); ++i) {
        const QByteArray fileName = QFile::encodeName(dir.absoluteFilePath(dir[i]));
        QFile file(QString::fromLocal8Bit(fileName));
        if (file.open(QFile::ReadOnly)) {
            const QByteArray fileData = file.readAll();
            QByteArray *fileDataPtr = new QByteArray(fileData);
            registerQPF2Font(fileData, fileDataPtr);
        }
    }
}

/*!

*/
QFontEngine *QPlatformFontDatabase::fontEngine(const QFontDef &fontDef, QUnicodeTables::Script script, void *handle)
{
    Q_UNUSED(script);
    Q_UNUSED(handle);
    QByteArray *fileDataPtr = static_cast<QByteArray *>(handle);
    QFontEngineQPA *engine = new QFontEngineQPA(fontDef,*fileDataPtr);
    //qDebug() << fontDef.pixelSize << fontDef.weight << fontDef.style << fontDef.stretch << fontDef.styleHint << fontDef.styleStrategy << fontDef.family << script;
    return engine;
}

/*!

*/
QStringList QPlatformFontDatabase::fallbacksForFamily(const QString family, const QFont::Style &style, const QFont::StyleHint &styleHint, const QUnicodeTables::Script &script) const
{
    Q_UNUSED(family);
    Q_UNUSED(style);
    Q_UNUSED(styleHint);
    Q_UNUSED(script);
    return QStringList();
}

/*!
    Adds an application font. Returns a list of family names, or an empty list if the font could
    not be added
*/
QStringList QPlatformFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
    Q_UNUSED(fontData);
    Q_UNUSED(fileName);

    qWarning("This plugin does not support application fonts");
    return QStringList();
}

/*!

*/
void QPlatformFontDatabase::releaseHandle(void *handle)
{
    QByteArray *fileDataPtr = static_cast<QByteArray *>(handle);
    delete fileDataPtr;
}

/*!

*/
QString QPlatformFontDatabase::fontDir() const
{
    QString fontpath = QString::fromLocal8Bit(qgetenv("QT_QPA_FONTDIR"));
    if (fontpath.isEmpty()) {
#ifndef QT_NO_SETTINGS
        fontpath = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
        fontpath += QLatin1String("/fonts");
#endif
    }

    return fontpath;
}

/*!
    \class QPlatformFontDatabase
    \brief The QPlatformFontDatabase makes it possible to customize how fonts are picked up, and
    and how they are rendered

    \ingroup painting

    QPlatformFontDatabase is the superclass which is intended to let platform implementations use
    native font handling.

    Qt has its internal fontdatabase which it uses to pick up available fonts. To be able
    to populate this database subclass this class, and reimplement populateFontDatabase().

    Use the function registerFont to populate the internal fontdatabase.

    Sometimes a specified font does not have the required glyphs, then the fallbackForFamily
    function is called.

    \sa QSupportedWritingSystems
*/
QT_END_NAMESPACE
