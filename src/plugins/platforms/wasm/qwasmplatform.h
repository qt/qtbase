// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMPLATFORM_H
#define QWASMPLATFORM_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>

#include <QPoint>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

enum class Platform {
    Generic,
    MacOS,
    Windows,
    Linux,
    Android,
    iOS
};

Platform platform();

QT_END_NAMESPACE

#endif  // QWASMPLATFORM_H
