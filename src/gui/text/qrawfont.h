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

#ifndef QRAWFONT_H
#define QRAWFONT_H

#include <QtCore/qstring.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtGui/qfont.h>
#include <QtGui/qtransform.h>
#include <QtGui/qfontdatabase.h>

#if !defined(QT_NO_RAWFONT)

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QRawFontPrivate;
class Q_GUI_EXPORT QRawFont
{
public:
    enum AntialiasingType {
        PixelAntialiasing,
        SubPixelAntialiasing
    };

    QRawFont();
    QRawFont(const QString &fileName,
             int pixelSize,
             QFont::HintingPreference hintingPreference = QFont::PreferDefaultHinting);
    QRawFont(const QByteArray &fontData,
             int pixelSize,
             QFont::HintingPreference hintingPreference = QFont::PreferDefaultHinting);
    QRawFont(const QRawFont &other);
    ~QRawFont();

    bool isValid() const;

    QRawFont &operator=(const QRawFont &other);
    bool operator==(const QRawFont &other) const;

    QString familyName() const;

    QFont::Style style() const;
    int weight() const;

    QVector<quint32> glyphIndexesForString(const QString &text) const;
    QVector<QPointF> advancesForGlyphIndexes(const QVector<quint32> &glyphIndexes) const;

    QImage alphaMapForGlyph(quint32 glyphIndex,
                            AntialiasingType antialiasingType = SubPixelAntialiasing,
                            const QTransform &transform = QTransform()) const;
    QPainterPath pathForGlyph(quint32 glyphIndex) const;

    void setPixelSize(int pixelSize);
    int pixelSize() const;

    QFont::HintingPreference hintingPreference() const;

    qreal ascent() const;
    qreal descent() const;

    qreal unitsPerEm() const;

    void loadFromFile(const QString &fileName,
                      int pixelSize,
                      QFont::HintingPreference hintingPreference);

    void loadFromData(const QByteArray &fontData,
                      int pixelSize,
                      QFont::HintingPreference hintingPreference);

    bool supportsCharacter(quint32 ucs4) const;
    bool supportsCharacter(const QChar &character) const;
    QList<QFontDatabase::WritingSystem> supportedWritingSystems() const;

    QByteArray fontTable(const char *tagName) const;

    static QRawFont fromFont(const QFont &font,
                             QFontDatabase::WritingSystem writingSystem = QFontDatabase::Any);

private:
    friend class QRawFontPrivate;

    void detach();

    QExplicitlySharedDataPointer<QRawFontPrivate> d;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_RAWFONT

#endif // QRAWFONT_H
