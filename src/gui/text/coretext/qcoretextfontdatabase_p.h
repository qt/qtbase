// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCORETEXTFONTDATABASE_H
#define QCORETEXTFONTDATABASE_H

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

#include <qglobal.h>

#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformtheme.h>
#include <private/qcore_mac_p.h>

Q_FORWARD_DECLARE_CF_TYPE(CTFontDescriptor);
Q_FORWARD_DECLARE_CF_TYPE(CTFont);

QT_DECL_METATYPE_EXTERN_TAGGED(QCFType<CGFontRef>, QCFType_CGFontRef, Q_GUI_EXPORT)
QT_DECL_METATYPE_EXTERN_TAGGED(QCFType<CFURLRef>, QCFType_CFURLRef, Q_GUI_EXPORT)

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QCoreTextFontDatabase : public QPlatformFontDatabase
{
public:
    QCoreTextFontDatabase();
    ~QCoreTextFontDatabase();
    void populateFontDatabase() override;
    bool populateFamilyAliases(const QString &missingFamily) override;
    void populateFamily(const QString &familyName) override;
    void invalidate() override;

    QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const override;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *applicationFont = nullptr) override;
    void releaseHandle(void *handle) override;
    bool isPrivateFontFamily(const QString &family) const override;
    QFont defaultFont() const override;
    bool fontsAlwaysScalable() const override;
    QList<int> standardSizes() const override;
    bool supportsVariableApplicationFonts() const override;

    // For iOS and macOS platform themes
    QFont *themeFont(QPlatformTheme::Font) const;

private:
    void populateThemeFonts();
    void populateFromDescriptor(CTFontDescriptorRef font, const QString &familyName = QString(), QFontDatabasePrivate::ApplicationFont *applicationFont = nullptr);
    static CFArrayRef fallbacksForFamily(const QString &family);

    QHash<QPlatformTheme::Font, QFont *> m_themeFonts;
    QHash<QString, QList<QCFType<CTFontDescriptorRef>>> m_systemFontDescriptors;
    QHash<QChar::Script, QString> m_hardcodedFallbackFonts;
    mutable QSet<QString> m_privateFamilies;

    bool m_hasPopulatedAliases;

#if defined(Q_OS_MACOS)
    QMacNotificationObserver m_fontSetObserver;
#endif
};

// Split out into separate template class so that the compiler doesn't have
// to generate code for each override in QCoreTextFontDatabase for each T.

template <class T>
class Q_GUI_EXPORT QCoreTextFontDatabaseEngineFactory : public QCoreTextFontDatabase
{
public:
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference) override;
};

QT_END_NAMESPACE

#endif // QCORETEXTFONTDATABASE_H
