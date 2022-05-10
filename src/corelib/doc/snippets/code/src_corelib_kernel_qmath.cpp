// Copyright (C) 2016 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
float degrees = 180.0f
float radians = qDegreesToRadians(degrees)
//! [0]


//! [1]
double degrees = 180.0
double radians = qDegreesToRadians(degrees)
//! [1]


//! [2]
float radians = float(M_PI)
float degrees = qRadiansToDegrees(radians)
//! [2]


//! [3]
double radians = M_PI
double degrees = qRadiansToDegrees(radians)
//! [3]

