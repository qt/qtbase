// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEMBEDDEDWINDOWNODE_H
#define QEMBEDDEDWINDOWNODE_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qrect.h>
#include <qohosplatformwindowidstruct_hack_p.h>
#include <arkui/drag_and_drop.h>
#include <arkui/native_gesture.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <arkui/ui_input_event.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <native_window/external_window.h>
#include <qarkui/input.h>
#include <qarkui/qnativenodeapi.h>
#include <qohosplugincore.h>
#include <qohosudmf.h>
#include <render/qxcomponent.h>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QArkUi {

struct NativeGestureInfo
{
    enum class GestureType {
        Pinch,
        Rotation,
    };

    GestureType type;
    ::ArkUI_GestureEvent *event;
};

class QEmbeddedWindowNode : public std::enable_shared_from_this<QEmbeddedWindowNode>
{
public:
    class ParentDescriptor
    {
    public:
        enum class Type
        {
            EmbeddedWindowNode,
            XComponentNode
        };
        explicit ParentDescriptor(QEmbeddedWindowNode &node);
        explicit ParentDescriptor(std::shared_ptr<QXComponentNode> node);

        ParentDescriptor(const ParentDescriptor &) = default;
        ParentDescriptor &operator=(const ParentDescriptor &) = default;

        ParentDescriptor(ParentDescriptor &&) = default;
        ParentDescriptor &operator=(ParentDescriptor &&) = default;

        Type type() const;
        std::shared_ptr<QEmbeddedWindowNode> embeddedWindowNodeOrNull() const;
        std::shared_ptr<QXComponentNode> xComponentNodeOrNull() const;
        bool operator==(const ParentDescriptor &) const;

    private:
        Type m_type;
        QOhosOptional<std::weak_ptr<QEmbeddedWindowNode>> m_embeddedWindowNode;
        QOhosOptional<std::weak_ptr<QXComponentNode>> m_xComponentNode;
    };

    static const std::int32_t minimumNodeZIndexValue;

    enum class SizePolicy
    {
        Points,
        PercentNormalized,
    };

    QEmbeddedWindowNode(
        std::unique_ptr<Node> stack,
        std::unique_ptr<Node> contentNode,
        std::unique_ptr<QtOhos::WindowIdStruct> windowId);

    QEmbeddedWindowNode(const QEmbeddedWindowNode &) = delete;
    QEmbeddedWindowNode &operator=(const QEmbeddedWindowNode &) = delete;

    QEmbeddedWindowNode(QEmbeddedWindowNode &&) = delete;
    QEmbeddedWindowNode &operator=(QEmbeddedWindowNode &&) = delete;

    void setNodeVisibility(bool visible);
    void setSize(const QSizeF &size);
    void setSizeParentFillPercentageNormalized(const QSizeF &size);
    void setPosition(const QPointF &position);
    void setFocused(bool focused);
    void setFocusable(bool focusable);
    void setBackgroundColor(const QColor &color);
    void setBrightness(int brightness);
    void setContrast(int contrast);
    void setSaturation(int saturation);
    void setGesturesHandler(QOhosConsumer<const NativeGestureInfo &> gesturesHandler);
    void setHitTestMode(::ArkUI_HitTestMode hitTestMode);
    void setDragEventsHandler(QOhosConsumer<::ArkUI_NodeEvent *> dragEventsHandler);
    void setKeyEventsHandler(QOhosConsumer<::ArkUI_UIInputEvent *> keyEventsHandler);
    void setHoverEventsHandler(QOhosConsumer<NativeNodeHoverEvent> hoverEventsHandler);
    void setAxisEventsHandler(QOhosConsumer<::ArkUI_UIInputEvent *> axisEventsHandler);
    void setMouseEventsHandler(QOhosConsumer<NativeNodeMouseEvent> mouseEventsHandler);
    void setCoastingAxisEventsHandler(QOhosConsumer<::ArkUI_UIInputEvent *> coastingAxisEventsHandler);
    std::shared_ptr<void> startDrag(
        std::vector<std::shared_ptr<::OH_PixelmapNative>> pixelMaps, const QPointF &hotspot,
        QOhosUdmfData udmfData, std::function<void(::ArkUI_DragAndDropInfo *)> statusListener);

    void raise();
    void lower();

    void *qtWindowId();

    void setParentOrReparent(ParentDescriptor parent);
    bool detachFromParentIfPresent();
    void handleGestureEvent(const NativeGestureInfo &nativeGestureInfo) const;

    virtual ~QEmbeddedWindowNode();

protected:
    Node &contentNode();
    const Node &contentNode() const;
    Node &stackNode();
    const Node &stackNode() const;

private:
    void setContentNodeEventHandler(
        ::ArkUI_NodeEventType eventType, const char *eventTypeName,
        QOhosConsumer<::ArkUI_UIInputEvent *> inputEventsHandler);

    std::unique_ptr<Node> m_stackNode;
    std::unique_ptr<Node> m_contentNode;
    std::shared_ptr<void> m_nodeEventDispatcherToken;
    QOhosOptional<ParentDescriptor> m_parentDescriptor;
    std::unique_ptr<QtOhos::WindowIdStruct> m_windowIdStruct;
    std::shared_ptr<void> m_nativeNodeDispatcherHandle;
    QOhosConsumer<const NativeGestureInfo &> m_gesturesHandler;
};

}

QT_END_NAMESPACE

#endif
