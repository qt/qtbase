/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSFONTDATABASE_H
#define QWINDOWSFONTDATABASE_H

#include <qpa/qplatformfontdatabase.h>
#include <QtCore/QSharedPointer>
#include "qtwindows_additional.h"

#if !defined(QT_NO_DIRECTWRITE)
    struct IDWriteFactory;
    struct IDWriteGdiInterop;
#endif

QT_BEGIN_NAMESPACE

class QWindowsFontEngineData
{
    Q_DISABLE_COPY(QWindowsFontEngineData)
public:
    QWindowsFontEngineData();
    ~QWindowsFontEngineData();

    uint pow_gamma[256];

    bool clearTypeEnabled;
    qreal fontSmoothingGamma;
    HDC hdc;
#if !defined(QT_NO_DIRECTWRITE)
    IDWriteFactory *directWriteFactory;
    IDWriteGdiInterop *directWriteGdiInterop;
#endif
};

class QWindowsFontDatabase : public QPlatformFontDatabase
{
public:
    QWindowsFontDatabase();
    ~QWindowsFontDatabase();

    void populateFontDatabase() Q_DECL_OVERRIDE;
    void populateFamily(const QString &familyName) Q_DECL_OVERRIDE;
    QFontEngineMulti *fontEngineMulti(QFontEngine *fontEngine, QChar::Script script) Q_DECL_OVERRIDE;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) Q_DECL_OVERRIDE;
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference) Q_DECL_OVERRIDE;
    QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const Q_DECL_OVERRIDE;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName) Q_DECL_OVERRIDE;
    void releaseHandle(void *handle) Q_DECL_OVERRIDE;
    QString fontDir() const Q_DECL_OVERRIDE;

    QFont defaultFont() const  Q_DECL_OVERRIDE { return systemDefaultFont(); }
    bool fontsAlwaysScalable() const Q_DECL_OVERRIDE;
    void derefUniqueFont(const QString &uniqueFont);
    void refUniqueFont(const QString &uniqueFont);

    static QFont systemDefaultFont();

    static QFontEngine *createEngine(const QFontDef &request,
                                     int dpi,
                                     const QSharedPointer<QWindowsFontEngineData> &data);

    static HFONT systemFont();
    static QFont LOGFONT_to_QFont(const LOGFONT& lf, int verticalDPI = 0);

    static qreal fontSmoothingGamma();
    static LOGFONT fontDefToLOGFONT(const QFontDef &fontDef);

    static QStringList extraTryFontsForFamily(const QString &family);
    static QString familyForStyleHint(QFont::StyleHint styleHint);

private:
    void populateFamily(const QString &familyName, bool registerAlias);
    void removeApplicationFonts();

    struct WinApplicationFont {
        HANDLE handle;
        QString fileName;
    };

    QList<WinApplicationFont> m_applicationFonts;

    struct UniqueFontData {
        HANDLE handle;
        QAtomicInt refCount;
    };

    QMap<QString, UniqueFontData> m_uniqueFontData;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug, const QFontDef &def);
#endif

QT_END_NAMESPACE

#endif // QWINDOWSFONTDATABASE_H
