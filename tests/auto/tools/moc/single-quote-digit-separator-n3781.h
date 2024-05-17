// Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SINGLE_QUOTE_DIGIT_SEPARATOR_N3781_H
#define SINGLE_QUOTE_DIGIT_SEPARATOR_N3781_H

#include <QObject>

class KDAB : public QObject
{
    Q_OBJECT
public:
    // C++1y allows use of single quote as a digit separator, useful for large
    // numbers. http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3781.pdf
    // Ensure that moc does not get confused with this.
    enum Salaries {
        Steve
#ifdef Q_MOC_RUN
        = 1'234'567
#endif
    };
    Q_ENUMS(Salaries)
};
#endif // SINGLE_QUOTE_DIGIT_SEPARATOR_N3781_H
