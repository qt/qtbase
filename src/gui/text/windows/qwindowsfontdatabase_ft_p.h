// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSFONTDATABASEFT_H
#define QWINDOWSFONTDATABASEFT_H

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

#include <QtGui/private/qfreetypefontdatabase_p.h>
#include <QtCore/QSharedPointer>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QWindowsFontDatabaseFT : public QFreeTypeFontDatabase
{
public:
    void populateFontDatabase() override;
    bool populateFamilyAliases(const QString &familyName) override;
    void populateFamily(const QString &familyName) override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize,
                            QFont::HintingPreference hintingPreference) override;

    QStringList fallbacksForFamily(const QString &family, QFont::Style style,
                                   QFont::StyleHint styleHint,
                                   QChar::Script script) const override;

    QString fontDir() const override;
    QFont defaultFont() const override;

    bool m_hasPopulatedAliases = false;
};

QT_END_NAMESPACE

#endif // QWINDOWSFONTDATABASEFT_H
