// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSTATUSBARMENU_H
#define QOHOSSTATUSBARMENU_H

#include <QtGui/qpa/qplatformmenu.h>
#include <functional>
#include <memory>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

class QOhosStatusBarMenu : public QPlatformMenu
{
public:
    virtual std::function<QNapi::Array(QtOhos::JsState &)> makeJsStatusBarGroupMenusFactory() const = 0;

protected:
    QOhosStatusBarMenu();
};

std::unique_ptr<QOhosStatusBarMenu> makeQOhosStatusBarMenu();

QT_END_NAMESPACE

#endif // QOHOSSTATUSBARMENU_H
