// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifdef __cplusplus
#  error Must be compiled as C
#endif

#include <qarkui/qohosdrageventcshim.h>

int32_t qOhosSetArkUiDragEventDragResult(ArkUI_DragEvent *dragEvent, uint32_t dragResultValue)
{
    return OH_ArkUI_DragEvent_SetDragResult(dragEvent, (ArkUI_DragResult)dragResultValue);
}
