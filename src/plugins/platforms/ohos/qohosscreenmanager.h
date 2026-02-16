// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSCREENMANAGER_H
#define QOHOSSCREENMANAGER_H

#include <QObject>
#include <QPointer>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <memory>
#include <qarkui/displaymanager.h>
#include <qohosdisplayinfo.h>
#include <qohosplatformscreen.h>
#include <qohosplugincore.h>
#include <vector>

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
        QOhosOptional<QOhosDisplayInfo> displayInfoOrEmpty() const;
        QOhosOptional<JsDisplayId> displayIdOrEmpty() const;

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


    std::shared_ptr<QArkUi::QOhosDisplayManager> m_jsScopeData;
    JsDisplayId m_primaryDisplayId;
    std::vector<std::unique_ptr<QOhosPlatformScreenHolder>> m_displays;
};

QT_END_NAMESPACE

#endif
