// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDSYSTEMLOCALE_H
#define QANDROIDSYSTEMLOCALE_H

#include "private/qlocale_p.h"
#include <QtCore/qreadwritelock.h>

QT_BEGIN_NAMESPACE

class QAndroidSystemLocale : public QSystemLocale
{
public:
    QAndroidSystemLocale();

    QVariant query(QueryType type, QVariant in) const override;
    QLocale fallbackLocale() const override;

private:
    void getLocaleFromJava() const;

    mutable QLocale m_locale;
    mutable QReadWriteLock m_lock;
};

QT_END_NAMESPACE

#endif // QANDROIDSYSTEMLOCALE_H
