// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMFONTDATABASE_H
#define QWASMFONTDATABASE_H

#include <QtGui/private/qfreetypefontdatabase_p.h>

QT_BEGIN_NAMESPACE

class QWasmFontDatabase : public QFreeTypeFontDatabase
{
public:
    void populateFontDatabase() override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QStringList fallbacksForFamily(const QString &family, QFont::Style style,
                                   QFont::StyleHint styleHint,
                                   QChar::Script script) const override;
    void releaseHandle(void *handle) override;
    QFont defaultFont() const override;

    void populateLocalfonts();

    static void notifyFontsChanged();
};
QT_END_NAMESPACE
#endif
