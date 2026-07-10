// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWINDOWHINTS_P_H
#define QOHOSWINDOWHINTS_P_H

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

#include <QtGui/qtguiglobal.h>

QT_BEGIN_NAMESPACE

namespace QOhosWindowHints {

// Stable dynamic-property keys set on a QWindow or QWidget and read by the platform plugin.
inline constexpr char privacyModeKey[] = "_q_platform_ohos_privacyMode";
inline constexpr char cornerRadiusKey[] = "_q_platform_ohos_cornerRadius";
inline constexpr char floatWindowKey[] = "_q_platform_ohos_floatWindow";
inline constexpr char surfaceBackgroundColorKey[] = "_q_platform_ohos_surfaceBackgroundColor";
inline constexpr char renderFitKey[] = "_q_platform_ohos_renderFit"; // int holding an ::ArkUI_RenderFit value
inline constexpr char keepScreenOnKey[] = "_q_platform_ohos_keepScreenOn";
inline constexpr char dragResizableKey[] = "_q_platform_ohos_dragResizable";
inline constexpr char brightnessKey[] = "_q_platform_ohos_brightness";
inline constexpr char contrastKey[] = "_q_platform_ohos_contrast";
inline constexpr char saturationKey[] = "_q_platform_ohos_saturation";

}

QT_END_NAMESPACE

#endif // QOHOSWINDOWHINTS_P_H
