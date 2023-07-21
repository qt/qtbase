// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [qt_wrap_cpp_4]
// myapp.cpp
#include "myapp.h"
#include <QObject>

class MyApp : public QObject {
    Q_OBJECT
public:
    MyApp() = default;
};

#include "myapp.moc"
//! [qt_wrap_cpp_4]
