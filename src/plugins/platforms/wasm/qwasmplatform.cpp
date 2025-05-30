// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmplatform.h"

QT_BEGIN_NAMESPACE

Platform platform()
{
    static const Platform qtDetectedPlatform = ([]() {
        // The Platform Detect: expand coverage as needed
        emscripten::val rawPlatform = emscripten::val::global("navigator")["platform"];

        if (rawPlatform.call<bool>("includes", emscripten::val("Mac")))
            return Platform::MacOS;
        if (rawPlatform.call<bool>("includes", emscripten::val("iPhone"))
        || rawPlatform.call<bool>("includes", emscripten::val("iPad")))
            return Platform::iOS;
        if (rawPlatform.call<bool>("includes", emscripten::val("Win32")))
            return Platform::Windows;
        if (rawPlatform.call<bool>("includes", emscripten::val("Linux"))) {
            emscripten::val uAgent = emscripten::val::global("navigator")["userAgent"];
            if (uAgent.call<bool>("includes", emscripten::val("Android")))
                return Platform::Android;
            return Platform::Linux;
        }
        return Platform::Generic;
    })();
    return qtDetectedPlatform;
}

QT_END_NAMESPACE
