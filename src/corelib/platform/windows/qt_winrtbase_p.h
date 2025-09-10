// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT_WINRTBASE_P_H
#define QT_WINRTBASE_P_H

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

#if QT_CONFIG(cpp_winrt)
#  include <winrt/base.h>
#  include <QtCore/private/qfactorycacheregistration_p.h>
// Workaround for Windows SDK bug.
// See https://github.com/microsoft/Windows.UI.Composition-Win32-Samples/issues/47
namespace winrt::impl
{
    template <typename Async>
    auto wait_for(Async const& async, Windows::Foundation::TimeSpan const& timeout);
}
// See https://learn.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/faq#how-do-i-resolve-ambiguities-with-getcurrenttime-and-or-try-
// for more workarounds.
#endif // QT_CONFIG(cpp/winrt)

#endif // QT_WINRTBASE_P_H
