// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
sb->setPrefix("$");
//! [0]


//! [1]
sb->setSuffix(" km");
//! [1]


//! [2]
setRange(minimum, maximum);
//! [2]


//! [3]
setMinimum(minimum);
setMaximum(maximum);
//! [3]


//! [4]
spinbox->setPrefix("$");
//! [4]


//! [5]
spinbox->setSuffix(" km");
//! [5]


//! [6]
setRange(minimum, maximum);
//! [6]


//! [7]
setMinimum(minimum);
setMaximum(maximum);
//! [7]

//! [8]
int IconSizeSpinBox::valueFromText(const QString &text) const
{
    static const QRegularExpression regExp(tr("(\\d+)(\\s*[xx]\\s*\\d+)?"));
    Q_ASSERT(regExp.isValid());

    const QRegularExpressionMatch match = regExp.match(text);
    if (match.isValid())
        return match.captured(1).toInt();
    return 0;
}
//! [8]

//! [9]
QString IconSizeSpinBox::textFromValue(int value) const
{
    return tr("%1 x %1").arg(value);
}
//! [9]
