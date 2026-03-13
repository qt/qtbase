// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosforeignwindow.h>

#include <QtGui/private/qhighdpiscaling_p.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <qarkui/qembeddedwindownode.h>
#include <qohosplatformwindow.h>
#include <qohosplatformscreen.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>
#include <render/qohosview.h>

QT_BEGIN_NAMESPACE

QOhosForeignWindow::QOhosForeignWindow(QWindow *qWindow, WId windowId)
    : QOhosPlatformWindow(qWindow)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        auto windowIdStruct = std::unique_ptr<QtOhos::WindowIdStruct>(
            reinterpret_cast<QtOhos::WindowIdStruct *>(windowId));

        if (windowIdStruct->content == nullptr)
            qOhosReportFatalErrorAndAbort("Invalid WId data, content node is empty");

        bool windowIdHasStackNode = windowIdStruct->stack != nullptr;
        std::unique_ptr<QArkUi::Node> stackNode;
        std::unique_ptr<QArkUi::Node> embeddedComponentNode =
            QArkUi::Node::takeOwnershipOfExternalNode(windowIdStruct->content);

        if (windowIdHasStackNode) {
            stackNode = QArkUi::Node::takeOwnershipOfExternalNode(windowIdStruct->stack);
        } else {
            stackNode = QArkUi::Node::createOrFail(::ARKUI_NODE_STACK);
            stackNode->setAttributeOrFail(::NODE_ACCESSIBILITY_ROLE, ::ARKUI_NODE_STACK);
            stackNode->setAttributeOrFail(::NODE_STACK_ALIGN_CONTENT, ::ARKUI_ALIGNMENT_TOP_START);
            stackNode->setAttributeOrFail(::NODE_Z_INDEX, QArkUi::QEmbeddedWindowNode::minimumNodeZIndexValue);
            stackNode->addChildOrFail(*embeddedComponentNode);
            windowIdStruct->stack = stackNode->handle();
        }

        m_jsStateData = QtOhos::makeProxyWithJsThreadDeleter(
            std::make_shared<JsStateData>(JsStateData {
                .embeddedWindow = std::make_unique<QArkUi::QEmbeddedWindowNode>(
                    std::move(stackNode), std::move(embeddedComponentNode),
                    std::move(windowIdStruct)),
            }));
    });
}

void QOhosForeignWindow::initialize()
{
}

bool QOhosForeignWindow::isForeignWindow() const
{
    return true;
};

void QOhosForeignWindow::setGeometry(const QRect &unscaledGeometry)
{
    auto scaledGeometry = QHighDpi::fromNative(QRectF(unscaledGeometry),
                              static_cast<QOhosPlatformScreen *>(screen())->pixelScalingCoefficient());

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData->embeddedWindow->setSize(scaledGeometry.size());
        m_jsStateData->embeddedWindow->setPosition(scaledGeometry.topLeft());
    });

    setWindowGeometryFromOhos(unscaledGeometry);
}

WId QOhosForeignWindow::winId() const
{
    return QtOhos::evalInJsThread([this](QtOhos::JsState &) {
        return reinterpret_cast<WId>(m_jsStateData->embeddedWindow->qtWindowId());
    });
}

void QOhosForeignWindow::setParent(const QPlatformWindow *window)
{
    if (window != nullptr && window->isForeignWindow())
        qOhosReportFatalErrorAndAbort("Reparenting to foreign windows is not supported");

    if (window == nullptr) {
        QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
            m_jsStateData->embeddedWindow->detachFromParentIfPresent();
        });
        return;
    }

    const auto *ohosPlatformWindow = static_cast<const QOhosPlatformWindow *>(window);
    auto *view = ohosPlatformWindow->ownedViewOrNull();

    if (view == nullptr)
        qOhosReportFatalErrorAndAbort("view was null, but should not be");

    view->addForeignWindowChild(this);
}

void QOhosForeignWindow::setVisible(bool visible)
{
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &) {
        m_jsStateData->embeddedWindow->setNodeVisibility(visible);
    });

    setExposedFromOhos(visible);
}

QMargins QOhosForeignWindow::frameMargins() const
{
    return QMargins{};
}

QArkUi::QEmbeddedWindowNode &QOhosForeignWindow::embeddedWindowNodeInJsThread()
{
    return *m_jsStateData->embeddedWindow;
}

QT_END_NAMESPACE
