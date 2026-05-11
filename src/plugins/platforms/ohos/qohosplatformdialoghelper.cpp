// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformdialoghelper.h"

QT_BEGIN_NAMESPACE

namespace QOhosDialogs
{

QPlatformDialogHelper *createHelper(QPlatformTheme::DialogType type)
{
    QPlatformDialogHelper *helper = nullptr;

    if (type == QPlatformTheme::FileDialog)
        helper = makeQOhosPlatformFileDialogHelper();

    return helper;
}

}

QT_END_NAMESPACE
