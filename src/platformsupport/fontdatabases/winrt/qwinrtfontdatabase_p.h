/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QWINRTFONTDATABASE_H
#define QWINRTFONTDATABASE_H

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

#include <QtFontDatabaseSupport/private/qfreetypefontdatabase_p.h>
#include <QtCore/QLoggingCategory>

struct IDWriteFontFile;
struct IDWriteFontFamily;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaFonts)

struct FontDescription
{
    quint32 index;
    QByteArray uuid;
};

class QWinRTFontDatabase : public QFreeTypeFontDatabase
{
public:
    ~QWinRTFontDatabase();
    QString fontDir() const override;
    QFont defaultFont() const override;
    bool fontsAlwaysScalable() const override;
    void populateFontDatabase() override;
    void populateFamily(const QString &familyName) override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QStringList fallbacksForFamily(const QString &family, QFont::Style style,
                                   QFont::StyleHint styleHint, QChar::Script script) const override;
    void releaseHandle(void *handle) override;

    static QString familyForStyleHint(QFont::StyleHint styleHint);
private:
    QHash<IDWriteFontFile *, FontDescription> m_fonts;
    QHash<QString, IDWriteFontFamily *> m_fontFamilies;
};

QT_END_NAMESPACE

#endif // QWINRTFONTDATABASE_H
