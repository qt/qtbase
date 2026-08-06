// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSCREENMANAGER_H
#define QOHOSSCREENMANAGER_H

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <memory>
#include <optional>
#include <qarkui/displaymanager.h>
#include <qohosdisplayinfo.h>
#include <qohosplatformscreen.h>
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
        explicit QOhosPlatformScreenHolder(std::unique_ptr<QOhosPlatformScreen> platformScreen);

        QOhosPlatformScreenHolder(const QOhosPlatformScreenHolder &) = delete;
        QOhosPlatformScreenHolder(QOhosPlatformScreenHolder &&) = delete;
        QOhosPlatformScreenHolder &operator=(const QOhosPlatformScreenHolder &) = delete;
        QOhosPlatformScreenHolder &operator=(QOhosPlatformScreenHolder &&) = delete;

        ~QOhosPlatformScreenHolder();

        QOhosPlatformScreen *platformScreenOrNull() const;
        std::optional<QOhosDisplayInfo> displayInfoOrEmpty() const;
        std::optional<JsDisplayId> displayIdOrEmpty() const;

    private:
        QPointer<QOhosPlatformScreen> m_platformScreen;
    };

    void handleDisplayAvailableAreaChanged(JsDisplayId jsDisplayID, QRectF availableArea);

    QOhosPlatformScreenHolder *platformScreenHolderForDisplayIdOrNull(QOhosDisplayInfo::JsDisplayId displayId) const;
    void rebuildScreenList(std::vector<QOhosDisplayInfo> updatedScreenList);

    std::shared_ptr<QArkUi::QOhosDisplayManager> m_jsScopeData;
    std::vector<std::unique_ptr<QOhosPlatformScreenHolder>> m_displays;
};

QT_END_NAMESPACE

#endif
