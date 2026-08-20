// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef HDC_H
#define HDC_H

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class Hdc
{
public:
    Hdc(QString hdcPath, QString connectKey);

    QString run(const QStringList &args, bool printOnFailure = false) const;

    QString program() const;
    QStringList arguments(const QStringList &args) const;

    QString connectKey() const;

private:
    QString m_hdcPath;
    QString m_connectKey;
};

QT_END_NAMESPACE

#endif // HDC_H
