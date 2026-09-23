// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSDRAGEVENTCSHIM_H
#define QOHOSDRAGEVENTCSHIM_H

// Must precede the ArkUI header: older SDK versions use bool in C without including it.
#include <stdbool.h>

#include <arkui/drag_and_drop.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compiled as C: converting an integer outside an enum's C++ value range to the enum is UB (C++17 [expr.static.cast]/10);
// in C it is defined for every value the enum's compatible integer type represents (C11 6.7.2.2p4, 6.3.1.3p1).
int32_t qOhosSetArkUiDragEventDragResult(ArkUI_DragEvent *dragEvent, uint32_t dragResultValue);

#ifdef __cplusplus
}
#endif

#endif
