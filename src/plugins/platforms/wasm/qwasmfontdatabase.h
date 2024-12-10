// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMFONTDATABASE_H
#define QWASMFONTDATABASE_H

#include <QtGui/private/qfreetypefontdatabase_p.h>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

class QWasmFontDatabase : public QFreeTypeFontDatabase
{
public:
    QWasmFontDatabase();
    static QWasmFontDatabase *get();

    void populateFontDatabase() override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QStringList fallbacksForFamily(const QString &family, QFont::Style style,
                                   QFont::StyleHint styleHint,
                                   QFontDatabasePrivate::ExtendedScript script) const override;
    void releaseHandle(void *handle) override;
    QFont defaultFont() const override;

    void populateLocalfonts();
    void populateLocalFontFamilies(emscripten::val families);
    void populateLocalFontFamilies(const QStringList &famliies, bool allFamilies);

    static void beginFontDatabaseStartupTask();
    static void endFontDatabaseStartupTask();
    static void refFontFileLoading();
    static void derefFontFileLoading();
    static void endAllFontFileLoading();

private:
    bool m_localFontsApiSupported = false;
    bool m_queryLocalFontsPermission = false;
    enum FontFamilyLoadSet {
        NoFontFamilies,
        DefaultFontFamilies,
        AllFontFamilies,
    };
    FontFamilyLoadSet m_localFontFamilyLoadSet;
    QStringList m_extraLocalFontFamilies;
};
QT_END_NAMESPACE
#endif
