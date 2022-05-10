// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "simplestyleplugin.h"
#include "simplestyle.h"

//! [0]
QStringList SimpleStylePlugin::keys() const
{
    return {"SimpleStyle"};
}
//! [0]

//! [1]
QStyle *SimpleStylePlugin::create(const QString &key)
{
    if (key.toLower() == "simplestyle")
        return new SimpleStyle;
    return nullptr;
}
//! [1]
