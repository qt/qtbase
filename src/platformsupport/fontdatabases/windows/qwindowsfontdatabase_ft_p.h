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

#include <QtFontDatabaseSupport/private/qfreetypefontdatabase_p.h>
#include <QtCore/QSharedPointer>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class QWindowsFontDatabaseFT : public QFreeTypeFontDatabase
{
public:
    void populateFontDatabase() override;
    void populateFamily(const QString &familyName) override;
    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle) override;
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize,
                            QFont::HintingPreference hintingPreference) override;

    QStringList fallbacksForFamily(const QString &family, QFont::Style style,
                                   QFont::StyleHint styleHint,
                                   QChar::Script script) const override;

    QString fontDir() const override;
    QFont defaultFont() const override;
};

QT_END_NAMESPACE

#endif // QWINDOWSFONTDATABASEFT_H
