// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMFONTDATABASE_H
#define QOHOSPLATFORMFONTDATABASE_H

#include <QtGui/private/qfontconfigdatabase_p.h>

QT_BEGIN_NAMESPACE

class QOhosPlatformFontDatabase: public QFontconfigDatabase
{
public:
    static void setOhosNoUiChildMode();

    void populateFontDatabase() override;
    QStringList fallbacksForFamily(const QString &family,
                                   QFont::Style style,
                                   QFont::StyleHint styleHint,
                                   QFontDatabasePrivate::ExtendedScript script) const override;
    QFont defaultFont() const override;
};

QT_END_NAMESPACE

#endif // QOHOSPLATFORMFONTDATABASE_H
