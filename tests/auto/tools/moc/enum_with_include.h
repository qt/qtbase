// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef ENUM_WITH_INCLUDE_H
#define ENUM_WITH_INCLUDE_H
#include <QtCore/QObject>

class Foo : public QObject {
    enum en {
       #include <enum_inc.h>
    };

    enum class en2 {
       #include <enum_inc.h>
       reference = 42
    };
    Q_OBJECT
};
#endif
