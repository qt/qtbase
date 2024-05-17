// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DIR_IN_INCLUDE_PATH_H
#define DIR_IN_INCLUDE_PATH_H
#include <Plugin>

class DirInIncludePath : public QObject, public MyInterface
{
    Q_OBJECT
    Q_INTERFACES(MyInterface)
};
#endif // DIR_IN_INCLUDE_PATH_H
