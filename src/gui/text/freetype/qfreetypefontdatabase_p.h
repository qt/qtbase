// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFREETYPEFONTDATABASE_H
#define QFREETYPEFONTDATABASE_H

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

#include <qpa/qplatformfontdatabase.h>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

struct FontFile
{
    QString fileName;
    int indexValue;
};

class Q_GUI_EXPORT QFreeTypeFontDatabase : public QPlatformFontDatabase
{
public:
    void populateFontDatabase() override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference) override;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *applicationFont = nullptr) override;
    void releaseHandle(void *handle) override;

    static QStringList addTTFile(const QByteArray &fontData, const QByteArray &file, QFontDatabasePrivate::ApplicationFont *applicationFont = nullptr);
};

QT_END_NAMESPACE

#endif // QFREETYPEFONTDATABASE_H
