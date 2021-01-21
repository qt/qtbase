/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#ifndef QANDROIDPLATFORMFONTDATABASE_H
#define QANDROIDPLATFORMFONTDATABASE_H

#include <QtFontDatabaseSupport/private/qfreetypefontdatabase_p.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformFontDatabase: public QFreeTypeFontDatabase
{
public:
    QString fontDir() const override;
    void populateFontDatabase() override;
    QStringList fallbacksForFamily(const QString &family,
                                   QFont::Style style,
                                   QFont::StyleHint styleHint,
                                   QChar::Script script) const override;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMFONTDATABASE_H
