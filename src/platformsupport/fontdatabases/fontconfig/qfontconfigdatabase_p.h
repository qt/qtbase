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

#ifndef QFONTCONFIGDATABASE_H
#define QFONTCONFIGDATABASE_H

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
#include <QtFontDatabaseSupport/private/qfreetypefontdatabase_p.h>

QT_BEGIN_NAMESPACE

class QFontEngineFT;

class QFontconfigDatabase : public QFreeTypeFontDatabase
{
public:
    void populateFontDatabase() override;
    void invalidate() override;
    QFontEngineMulti *fontEngineMulti(QFontEngine *fontEngine, QChar::Script script) override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference) override;
    QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const override;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName) override;
    QString resolveFontFamilyAlias(const QString &family) const override;
    QFont defaultFont() const override;

private:
    void setupFontEngine(QFontEngineFT *engine, const QFontDef &fontDef) const;
};

QT_END_NAMESPACE

#endif // QFONTCONFIGDATABASE_H
