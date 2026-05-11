// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSYSTEMLOCALE_H
#define QOHOSSYSTEMLOCALE_H

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <private/qlocale_p.h>

QT_BEGIN_NAMESPACE

class QOhosSystemLocale : public QSystemLocale
{
public:
    QOhosSystemLocale(const QString &systemLocaleId, const QStringList &preferredLanguages);

    QVariant query(QueryType type, QVariant &&in = QVariant()) const override;
    QLocale fallbackLocale() const override;

private:
    QString m_systemLocaleId;
    QStringList m_preferredLanguages;
};

QT_END_NAMESPACE

#endif // QOHOSSYSTEMLOCALE_H
