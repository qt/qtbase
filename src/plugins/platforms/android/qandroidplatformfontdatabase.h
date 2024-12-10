// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMFONTDATABASE_H
#define QANDROIDPLATFORMFONTDATABASE_H

#include <QtGui/private/qfreetypefontdatabase_p.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformFontDatabase: public QFreeTypeFontDatabase
{
public:
    QString fontDir() const override;
    void populateFontDatabase() override;
    QFont defaultFont() const override;
    QStringList fallbacksForFamily(const QString &family,
                                   QFont::Style style,
                                   QFont::StyleHint styleHint,
                                   QFontDatabasePrivate::ExtendedScript script) const override;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMFONTDATABASE_H
