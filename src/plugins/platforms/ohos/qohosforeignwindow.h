// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSFOREIGNWINDOW_H
#define QOHOSFOREIGNWINDOW_H

#include <QtCore/qglobal.h>
#include <qarkui/qembeddedwindownode.h>
#include <qarkui/qnativenodeapi.h>
#include <qohosplatformwindow.h>
#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

class QOhosForeignWindow final : public QOhosPlatformWindow
{
public:
    explicit QOhosForeignWindow(QWindow *qWindow, WId windowId);

    void initialize() override;
    bool isForeignWindow() const override;
    void setGeometry(const QRect &unscaledGeometry) override;
    WId winId() const override;
    void setParent(const QPlatformWindow *window) override;
    void setVisible(bool visible) override;
    QMargins frameMargins() const override;

    QArkUi::QEmbeddedWindowNode &embeddedWindowNodeInJsThread();

private:
    struct JsStateData
    {
        std::unique_ptr<QArkUi::QEmbeddedWindowNode> embeddedWindow;
    };

    std::shared_ptr<JsStateData> m_jsStateData;
};

QT_END_NAMESPACE

#endif
