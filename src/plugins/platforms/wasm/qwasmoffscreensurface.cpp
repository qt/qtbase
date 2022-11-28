// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmoffscreensurface.h"

QT_BEGIN_NAMESPACE

QWasmOffscreenSurface::QWasmOffscreenSurface(QOffscreenSurface *offscreenSurface)
    :QPlatformOffscreenSurface(offscreenSurface)
{
}

QWasmOffscreenSurface::~QWasmOffscreenSurface() = default;

QT_END_NAMESPACE
