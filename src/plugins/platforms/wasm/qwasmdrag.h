// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMDRAG_H
#define QWASMDRAG_H

#include <QtCore/qtconfigmacros.h>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

void dropEvent(emscripten::val event);

QT_END_NAMESPACE

#endif // QWASMDRAG_H
