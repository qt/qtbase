// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "simplestyle.h"

void SimpleStyle::polish(QPalette &palette)
{
    palette.setBrush(QPalette::Text, Qt::red);
}
