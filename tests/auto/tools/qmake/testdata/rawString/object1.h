// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#define rawstring R"blah(lorem " ipsum /*)blah";
#include <QObject>

class Object1 : public QObject
{
    Q_OBJECT
};
