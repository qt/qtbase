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
    QLocale fallbackUiLocale() const override;

private:
    void getLocaleFromJava() const;

    mutable QLocale m_locale;
    mutable QReadWriteLock m_lock;
};

QT_END_NAMESPACE

#endif // QANDROIDSYSTEMLOCALE_H
