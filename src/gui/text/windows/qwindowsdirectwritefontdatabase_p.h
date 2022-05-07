/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QWINDOWSDIRECTWRITEFONTDATABASE_P_H
#define QWINDOWSDIRECTWRITEFONTDATABASE_P_H

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

#include <QtGui/qtguiglobal.h>
#include <QtGui/private/qtgui-config_p.h>

QT_REQUIRE_CONFIG(directwrite3);

#include "qwindowsfontdatabasebase_p.h"
#include <QtCore/qloggingcategory.h>

struct IDWriteFactory;
struct IDWriteFont;
struct IDWriteFontFamily;
struct IDWriteLocalizedStrings;

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QWindowsDirectWriteFontDatabase : public QWindowsFontDatabaseBase
{
    Q_DISABLE_COPY_MOVE(QWindowsDirectWriteFontDatabase)
public:
    QWindowsDirectWriteFontDatabase();
    ~QWindowsDirectWriteFontDatabase() override;

    void populateFontDatabase() override;
    void populateFamily(const QString &familyName) override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const override;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *font = nullptr) override;
    void releaseHandle(void *handle) override;
    QFont defaultFont() const override;

    bool fontsAlwaysScalable() const override;
    bool isPrivateFontFamily(const QString &family) const override;

private:
    static QString localeString(IDWriteLocalizedStrings *names, wchar_t localeName[]);

    QHash<QString, IDWriteFontFamily *> m_populatedFonts;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECTWRITEFONTDATABASE_P_H
