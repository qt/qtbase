// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QCache<int, Employee> cache;
//! [0]


//! [1]
Employee *employee = new Employee;
employee->setId(37);
employee->setName("Richard Schmit");
...
cache.insert(employee->id(), employee);
//! [1]


//! [2]
QCache<int, MyDataStructure> cache(5000);
//! [2]
