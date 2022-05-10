// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef TEST_QMAKE_RAWSTRING_OBJECT2_H
#define TEST_QMAKE_RAWSTRING_OBJECT2_H

#define Lu8UR "land"
inline char opener(int i) {
    const char text[] = Lu8UR"blah( not a raw string; just juxtaposed";
    return text[i];
}

#include <QObject>

class Object2 : public QObject
{
    Q_OBJECT
};

inline char closer(int i) {
    const char text[] = "pretend to close it, all the same )blah";
    return text[i];
}

#endif // TEST_QMAKE_RAWSTRING_OBJECT2_H
