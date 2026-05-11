// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCORE_OHOS_PLATFORM_WINDOWIDSTRUCT_HACK_P_H
#define QCORE_OHOS_PLATFORM_WINDOWIDSTRUCT_HACK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <arkui/native_node.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

// NOTE - DO NOT REFACTOR/MODIFY this struct
// The layout is requested by the client and its purpose
// is to return it as a result of WinID call
struct WindowIdStruct
{
    ::ArkUI_NodeType nodeType;
    ::ArkUI_NodeHandle content;
    ::ArkUI_NodeHandle stack;
    char nodeOwner[8];
    void *nodePrivate;
};

}

QT_END_NAMESPACE

#endif // QCORE_OHOS_PLATFORM_WINDOWIDSTRUCT_HACK_P_H
