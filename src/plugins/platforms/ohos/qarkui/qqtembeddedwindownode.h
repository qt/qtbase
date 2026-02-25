// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQTEMBEDDEDWINDOWNODE_H
#define QQTEMBEDDEDWINDOWNODE_H

#include <QtCore/qglobal.h>
#include <qarkui/qembeddedwindownode.h>
#include <qohosplugincore.h>
#include <QtCore/qpoint.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {

class QXComponentCallbackReceiver
{
public:
    enum class SurfaceEventType
    {
        SurfaceCreated,
        SurfaceChanged,
        SurfaceDestroyed,
    };

    enum class InputEventType
    {
        Mouse,
        Touch,
        Keyboard,
    };

    virtual void onSurfaceEvent(SurfaceEventType surfaceEventType, ::OHNativeWindow *nativeWindow) = 0;
    virtual void onInputEvent(InputEventType inputEventType, ::OHNativeWindow *nativeWindow) = 0;
    virtual void onHoverEvent(bool isHover) = 0;
    virtual ~QXComponentCallbackReceiver();
};

class QQtEmbeddedWindowNode final
    : public QEmbeddedWindowNode
    , private QXComponentCallbackReceiver
{
public:
    struct CreateInfo
    {
        QPointF offset;
        QSizeF size;
        std::int32_t zIndex = minimumNodeZIndexValue;
        QXComponentId xComponentId;
        ::ArkUI_XComponentType xComponentType = ::ArkUI_XComponentType::ARKUI_XCOMPONENT_TYPE_SURFACE;
        bool focusOnTouch = false;
        bool focusable = false;
        QOhosOptional<ParentDescriptor> optParent;
        SizePolicy sizePolicy = SizePolicy::Points;
        ::ArkUI_RenderFit renderFit = ::ARKUI_RENDER_FIT_TOP_LEFT;
        QOhosOptional<QColor> backgroundColor;
    };

    struct NodeAreaInfo
    {
        QRect screenGeometryPixels;
        QPoint windowRelativeOffsetPixels;
        QPoint parentRelativeOffsetPixels;
    };

    static std::shared_ptr<QQtEmbeddedWindowNode> createOrFail(const CreateInfo &createInfo);

    QXComponentRender renderXComponent() const;
    ::OHNativeWindow *nativeWindowOrNull() const;
    void setCallbackReceiver(std::unique_ptr<QXComponentCallbackReceiver> callbackReceiver);
    void setSurfaceResolution(std::uint32_t width, std::uint32_t height);
    void setAreaChangeReceiver(QOhosConsumer<NodeAreaInfo> areaChangeReceiver);
    void setFocusedChangeReceiver(QOhosConsumer<bool> focus);
    void setVisibilityChangeReceiver(QOhosConsumer<bool> visibilityChangedReceiver);
    void setTouchInterceptReceiver(QOhosConsumer<const ::ArkUI_UIInputEvent *> touchInterceptReceiver);
    QRect nodeScreenGeometryPixels() const;
    QPoint windowRelativeOffsetPixels() const;
    QPoint parentRelativeOffsetPixels() const;
    bool hasNonQtManagedChildren() const;
    NodeAreaInfo nodeAreaInfo() const;

    ~QQtEmbeddedWindowNode() override;

private:
    QQtEmbeddedWindowNode(
        std::unique_ptr<Node> stackNode,
        std::unique_ptr<Node> xComponentNode,
        std::unique_ptr<QtOhos::WindowIdStruct> windowId);

    void onSurfaceEvent(SurfaceEventType surfaceEventType, ::OHNativeWindow *nativeWindow) override;
    void onInputEvent(InputEventType inputEventType, ::OHNativeWindow *nativeWindow) override;
    void onHoverEvent(bool isHover) override;

    std::shared_ptr<void> m_xComponentCallbackDispatcherToken;
    std::unique_ptr<QXComponentCallbackReceiver> m_callbackReceiver;
    ::OHNativeWindow *m_nativeWindow = nullptr;
    QOhosConsumer<NodeAreaInfo> m_optAreaChangedReceiver;
    QOhosConsumer<bool> m_optFocusedChangedReceiver;
    QOhosConsumer<bool> m_optVisibilityChangedReceiver;
    QOhosConsumer<const ::ArkUI_UIInputEvent *> m_optTouchInterceptReceiver;
};

}

QT_END_NAMESPACE

#endif
