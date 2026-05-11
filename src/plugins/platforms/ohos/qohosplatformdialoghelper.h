// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMDIALOGHELPER_H
#define QOHOSPLATFORMDIALOGHELPER_H

#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

namespace QOhosDialogs
{
    QPlatformDialogHelper *createHelper(QPlatformTheme::DialogType type);
}

QPlatformFileDialogHelper *makeQOhosPlatformFileDialogHelper();

QT_END_NAMESPACE

#endif // QOHOSPLATFORMDIALOGHELPER_H
