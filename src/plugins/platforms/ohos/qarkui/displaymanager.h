// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QARKUI_DISPLAY_MANAGER_H
#define QARKUI_DISPLAY_MANAGER_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <qohosdisplayinfo.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {

class QOhosDisplayManager : public std::enable_shared_from_this<QOhosDisplayManager>
{
public:
    using JsDisplayId = QOhosDisplayInfo::JsDisplayId;

    struct CreateInfo
    {
        std::vector<QOhosDisplayInfo> displayInfos;
        QOhosConsumer<QtOhos::JsState &, JsDisplayId> displayChangedCb;
        QOhosConsumer<QtOhos::JsState &, JsDisplayId> displayAddedCb;
        QOhosConsumer<QtOhos::JsState &, JsDisplayId> displayRemovedCb;
        QOhosConsumer<QtOhos::JsState &, JsDisplayId, QRectF> displayAvailableAreaChangedCb;
    };

    static std::shared_ptr<QOhosDisplayManager> create(QtOhos::JsState &jsState, CreateInfo createInfo);

    std::vector<QOhosDisplayInfo> getRegisteredDisplayInfos();

private:
    QOhosDisplayManager(QtOhos::JsState &jsState, CreateInfo createInfo);

    void registerDisplayCallbackListener(
        QNapi::Object displayModule, const std::string &eventName,
        QOhosConsumer<QtOhos::JsState &, QOhosDisplayInfo::JsDisplayId> handleFunction);
    bool tryRegisterDisplay(QtOhos::JsState &jsState, JsDisplayId displayId);
    void unregisterDisplay(JsDisplayId displayId);

    std::vector<QOhosDisplayInfo> m_registeredDisplayInfos;
    std::vector<std::shared_ptr<void>> m_destroyNotifiers;
    std::map<JsDisplayId, std::shared_ptr<void>> m_perDisplayDestroyNotifiers;
    QOhosConsumer<QtOhos::JsState &, JsDisplayId, QRectF> m_availableAreaChangedCb;
};

}

QT_END_NAMESPACE

#endif
