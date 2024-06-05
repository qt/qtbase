// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    class EmployeeData;
//! [0]

//! [1]
    template<>
    EmployeeData *QSharedDataPointer<EmployeeData>::clone()
    {
        return d->clone();
    }
//! [1]
