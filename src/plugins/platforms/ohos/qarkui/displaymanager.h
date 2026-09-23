// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QARKUI_DISPLAY_MANAGER_H
#define QARKUI_DISPLAY_MANAGER_H

#include <QtCore/private/qcore_ohos_p.h>
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
        QOhosConsumer<QOhosJsState &, std::vector<QOhosDisplayInfo>> displaysUpdatedCb;
        QOhosConsumer<QOhosJsState &, JsDisplayId, QRectF> displayAvailableAreaChangedCb;
    };

    static std::shared_ptr<QOhosDisplayManager> create(QOhosJsState &jsState, CreateInfo createInfo);

    std::vector<QOhosDisplayInfo> getRegisteredDisplayInfos();

private:
    QOhosDisplayManager(QOhosJsState &jsState, CreateInfo createInfo);

    void registerDisplayCallbackListener(
        QNapi::Object displayModule, const std::string &eventName,
        QOhosConsumer<QOhosJsState &, QOhosDisplayInfo::JsDisplayId> handleFunction);
    bool tryRegisterDisplay(QOhosJsState &jsState, JsDisplayId displayId);
    void rebuildRegisteredDisplayList(QOhosJsState &jsState);

    std::vector<QOhosDisplayInfo> m_registeredDisplayInfos;
    std::vector<std::shared_ptr<void>> m_destroyNotifiers;
    std::map<JsDisplayId, std::shared_ptr<void>> m_perDisplayDestroyNotifiers;
    QOhosConsumer<QOhosJsState &, JsDisplayId, QRectF> m_availableAreaChangedCb;
};

QPoint mapFromDisplayToGlobal(const QPoint &displayOffset, QOhosDisplayInfo::JsDisplayId jsDisplayId);

}

QT_END_NAMESPACE

#endif
