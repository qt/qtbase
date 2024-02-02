// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef _WIDGETDUMP_
#define _WIDGETDUMP_

#include "qwindowdump.h"

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace QtDiag {

void dumpAllWidgets(FormatWindowOptions options = {}, const QWidget *root = nullptr);

} // namespace QtDiag

#endif
