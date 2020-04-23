/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QWINDOWSFONTDATABASEBASE_P_H
#define QWINDOWSFONTDATABASEBASE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformfontdatabase.h>
#include <QtCore/QSharedPointer>
#include <QtCore/QLoggingCategory>
#include <QtCore/qt_windows.h>

#if !defined(QT_NO_DIRECTWRITE)
    struct IDWriteFactory;
    struct IDWriteGdiInterop;
    struct IDWriteFontFace;
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaFonts);

class QWindowsFontEngineData
{
    Q_DISABLE_COPY_MOVE(QWindowsFontEngineData)
public:
    QWindowsFontEngineData();
    ~QWindowsFontEngineData();

    uint pow_gamma[256];

    bool clearTypeEnabled = false;
    qreal fontSmoothingGamma;
    HDC hdc = 0;
#if !defined(QT_NO_DIRECTWRITE)
    IDWriteFactory *directWriteFactory = nullptr;
    IDWriteGdiInterop *directWriteGdiInterop = nullptr;
#endif
};

class QWindowsFontDatabaseBase : public QPlatformFontDatabase
{
public:
    QWindowsFontDatabaseBase();
    ~QWindowsFontDatabaseBase() override;

    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference) override;

    static int defaultVerticalDPI();
    static void setDefaultVerticalDPI(int d);

    static QSharedPointer<QWindowsFontEngineData> data();
#if !defined(QT_NO_DIRECTWRITE)
    static void createDirectWriteFactory(IDWriteFactory **factory);
#endif
    static QFont systemDefaultFont();
    static HFONT systemFont();
    static LOGFONT fontDefToLOGFONT(const QFontDef &fontDef, const QString &faceName);
    static QFont LOGFONT_to_QFont(const LOGFONT& lf, int verticalDPI = 0);

    static QString familyForStyleHint(QFont::StyleHint styleHint);
    static QStringList extraTryFontsForFamily(const QString &family);

    class FontTable{};
    class EmbeddedFont
    {
    public:
        EmbeddedFont(const QByteArray &fontData) : m_fontData(fontData) {}

        QString changeFamilyName(const QString &newFamilyName);
        QByteArray data() const { return m_fontData; }
        void updateFromOS2Table(QFontEngine *fontEngine);
        FontTable *tableDirectoryEntry(const QByteArray &tagName);
        QString familyName(FontTable *nameTableDirectory = nullptr);

    private:
        QByteArray m_fontData;
    };

protected:

#if !defined(QT_NO_DIRECTWRITE)
    IDWriteFontFace *createDirectWriteFace(const QByteArray &fontData) const;
#endif

private:
    static bool init(QSharedPointer<QWindowsFontEngineData> data);
};

QT_END_NAMESPACE

#endif // QWINDOWSFONTDATABASEBASE_P_H
