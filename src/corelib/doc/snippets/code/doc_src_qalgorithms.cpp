// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
QList<Employee *> list;
list.append(new Employee("Blackpool", "Stephen"));
list.append(new Employee("Twist", "Oliver"));

qDeleteAll(list.begin(), list.end());
list.clear();
//! [1]
