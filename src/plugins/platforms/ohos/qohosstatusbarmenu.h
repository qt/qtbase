// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSTATUSBARMENU_H
#define QOHOSSTATUSBARMENU_H

#include <QtCore/private/qcore_ohos_p.h>
#include <QtGui/qpa/qplatformmenu.h>
#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

class QOhosStatusBarMenu : public QPlatformMenu
{
public:
    virtual std::function<QNapi::Array(QOhosJsState &)> makeJsStatusBarGroupMenusFactory() const = 0;

protected:
    QOhosStatusBarMenu();
};

std::unique_ptr<QOhosStatusBarMenu> makeQOhosStatusBarMenu();

QT_END_NAMESPACE

#endif // QOHOSSTATUSBARMENU_H
