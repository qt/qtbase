// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTUTIL_MACOS_H
#define QTESTUTIL_MACOS_H

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

#include <private/qglobal_p.h>
#import <objc/objc.h>

QT_BEGIN_NAMESPACE

namespace QTestPrivate {
    void disableWindowRestore();
    bool macCrashReporterWillShowDialog();

    class AppNapDisabler
    {
    public:
        AppNapDisabler();
        ~AppNapDisabler();
        AppNapDisabler(const AppNapDisabler&) = delete;
        AppNapDisabler& operator=(const AppNapDisabler&) = delete;
    private:
        id m_activity;
    };
}

QT_END_NAMESPACE

#endif
