// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    if (qFloatDistance(a, b) < (1 << 7)) {   // The last 7 bits are not
                                            // significant
        // precise enough
    }
//! [0]
