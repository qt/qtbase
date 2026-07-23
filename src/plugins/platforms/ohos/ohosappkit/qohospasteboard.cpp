// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohospasteboard.h"
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

/*!
    \namespace QtOhosAppKit::Pasteboard
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The Pasteboard class is to manage native pasteboard.
*/
namespace Pasteboard {

/*!
    \fn void QtOhosAppKit::Pasteboard::setInAppOnlyPasteboardShareOption(bool shareInAppOnly)

    According to \a shareInAppOnly sets if pasteboard data content can be used only in the same
    application or across applications of a device.
*/
void setInAppOnlyPasteboardShareOption(bool shareInAppOnly)
{
    constexpr const char *platformFunctionName = "setInAppOnlyPasteboardShareOption";

    auto *fn = reinterpret_cast<void(*)(bool)>(qApp->platformFunction(platformFunctionName));
    if (fn == nullptr)
        qFatal("failed to load function: %s", platformFunctionName);
    (*fn)(shareInAppOnly);
}

}

}

QT_END_NAMESPACE
