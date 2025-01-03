// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QHash>
#include <QDataStream>
#include <QSet>

#include "configfile.h"

#if 1
typedef int InputType;

enum SpecialInputType {
    DigitInput,
    SpaceInput,
    Letter
};

#else

enum SpecialInputType {
    NoSpecialInput = 0,
    DigitInput,
    SpaceInput,
    LetterOrNumberInput
};

struct InputType
{
    inline InputType() : val(0), specialInput(NoSpecialInput) {}
    inline InputType(const int &val) : val(val), specialInput(NoSpecialInput) {}

    inline operator int() const { return val; }

    inline bool operator==(const InputType &other) const
    { return val == other.val; }
    inline bool operator!=(const InputType &other) const
    { return val != other.val; }

    int val;
    SpecialInputType specialInput;
};

inline int qHash(const InputType &t) { return qHash(t.val); }

inline QDataStream &operator<<(QDataStream &stream, const InputType &i)
{
    return stream << i;
}

inline QDataStream &operator>>(QDataStream &stream, InputType &i)
{
    return stream >> i;
}

#endif

const InputType Epsilon = -1;

struct Config
{
    inline Config() : caseSensitivity(Qt::CaseSensitive), debug(false), cache(false) {}
    QSet<InputType> maxInputSet;
    Qt::CaseSensitivity caseSensitivity;
    QString className;
    bool debug;
    bool cache;
    QString ruleFile;
    ConfigFile::SectionMap configSections;
};

#endif // GLOBAL_H
