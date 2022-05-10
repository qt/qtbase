// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "iconsizespinbox.h"

#include <QRegularExpression>

//! [0]
IconSizeSpinBox::IconSizeSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
}
//! [0]

//! [1]
int IconSizeSpinBox::valueFromText(const QString &text) const
{
    static const QRegularExpression regExp(tr("(\\d+)(\\s*[xx]\\s*\\d+)?"));
    Q_ASSERT(regExp.isValid());

    const QRegularExpressionMatch match = regExp.match(text);
    if (match.isValid())
        return match.captured(1).toInt();
    return 0;
}
//! [1]

//! [2]
QString IconSizeSpinBox::textFromValue(int value) const
{
    return tr("%1 x %1").arg(value);
}
//! [2]
