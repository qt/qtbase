// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSCREENMANAGER_H
#define QOHOSSCREENMANAGER_H

#include <QObject>
#include <QPointer>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <map>
#include <memory>
#include <qohosdisplayinfo.h>
#include <qohosplatformscreen.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

class QOhosScreenManager final : public QObject
{
    Q_OBJECT

public:
    QOhosScreenManager();

    QOhosPlatformScreen *platformScreenForDisplayIdOrNull(QOhosDisplayInfo::JsDisplayId displayId) const;
    QOhosPlatformScreen *platformScreenForDisplayIdOrFail(QOhosDisplayInfo::JsDisplayId displayId) const;

private:
    using JsDisplayId = QOhosDisplayInfo::JsDisplayId;

    class QOhosPlatformScreenHolder
    {
    public:
        explicit QOhosPlatformScreenHolder(const QOhosDisplayInfo &displayInfo);

        QOhosPlatformScreenHolder(const QOhosPlatformScreenHolder &) = delete;
        QOhosPlatformScreenHolder(QOhosPlatformScreenHolder &&) = delete;
        QOhosPlatformScreenHolder &operator=(const QOhosPlatformScreenHolder &) = delete;
        QOhosPlatformScreenHolder &operator=(QOhosPlatformScreenHolder &&) = delete;

        ~QOhosPlatformScreenHolder();

        QOhosPlatformScreen *platformScreenOrNull() const;

    private:
        QPointer<QOhosPlatformScreen> m_platformScreen;
    };

    void handleDisplayChangedCallbackInQtThread(const QOhosDisplayInfo &displayInfo);
    void handleDisplayAdded(const QOhosDisplayInfo &displayInfo);
    void handleDisplayRemoved(QOhosDisplayInfo::JsDisplayId displayId);
    void handleDisplayAvailableAreaChanged(JsDisplayId jsDisplayID, QRectF availableArea);

    QOhosPlatformScreenHolder *platformScreenHolderForDisplayIdOrNull(QOhosDisplayInfo::JsDisplayId displayId) const;
    void addScreen(QOhosDisplayInfo displayInfo);
    void removeScreenIfExists(QOhosDisplayInfo::JsDisplayId displayId);
    void updatePrimaryPlatformScreenIfNeeded();

    class QOhosDisplayManager : public std::enable_shared_from_this<QOhosDisplayManager>
    {
    public:
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
        QOhosDisplayManager(QtOhos::JsState &);

        void initialize(QtOhos::JsState &jsState, CreateInfo createInfo);
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

    std::shared_ptr<QOhosDisplayManager> m_jsScopeData;
    JsDisplayId m_primaryDisplayId;
    std::map<JsDisplayId, std::unique_ptr<QOhosPlatformScreenHolder>> m_displays;
};

QT_END_NAMESPACE

#endif
