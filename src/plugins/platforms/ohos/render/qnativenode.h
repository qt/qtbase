// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNATIVENODE_H
#define QNATIVENODE_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtGui/qcolor.h>
#include <QtGui/qimage.h>
#include <QtGui/qwindow.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <functional>
#include <memory>
#include <native_window/external_window.h>
#include <optional>
#include <qarkui/qqtembeddedwindownode.h>
#include <qohosforeignwindow.h>
#include <qohosjsscopedataowner.h>
#include <qohosplatformwindow.h>
#include <qohosplugincore.h>
#include <render/qohossurface.h>
#include <render/qxcomponent.h>
#include <vector>

QT_BEGIN_NAMESPACE

class QNativeNode : public QObject
{
    Q_OBJECT
public:
    using JsCallBatch = QtOhos::JsCallBatch;

    struct CreateInfo
    {
        QRect geometry;
        QWindow *window;
        std::optional<QNativeNode *> optParent;
        std::optional<QColor> backgroundColor;
        std::optional<::ArkUI_RenderFit> renderFitPolicyHint;
    };

    explicit QNativeNode(const CreateInfo &nativeNodeCreateInfo);

    void setParent(std::shared_ptr<QXComponentNode> xComponent, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setParent(QNativeNode &other);
    void detachFromParentIfPresent(JsCallBatch &jsCallBatch = JsCallBatch::immediate());

    void setSize(const QSizeF &size, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setSizeParentFillPercentNormalized(const QSizeF &size, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setPosition(QPoint position, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setVisibility(bool visible, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void fillToParent(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void raise(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void lower(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setFocused(bool focused, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setFocusable(bool focusable, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setBackgroundColor(const QColor &color, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setBrightness(int brightness, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setContrast(int contrast, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setSaturation(int saturation, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setTransparentForInput(bool transparentForInput, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void startDrag(
        const std::vector<QImage> &images, const QPointF &hotspot,
        const QMimeData &mimeData, QOhosConsumer<Qt::DropAction> dropActionConsumer,
        JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void enableDropDisallowedBadge(JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setNodeAreaChangeHandler(
        QOhosConsumer<QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo> areaChangeEventConsumer,
        JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setNodeFocusChangeHandler(
        QOhosConsumer<bool> focusedChangedConsumer, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    void setNodeVisibilityChangeHandler(
        QOhosConsumer<bool> visibilityChangedConsumer, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    QRect nodeScreenGeometryPixels() const;
    QRect nodeParentRelativeGeometryPixels() const;
    QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo nodeAreaInfo() const;

    void addForeignWindowChild(QOhosForeignWindow *foreignWindow);

    QRectF geometry() const;
    QOhosSurface *surfaceOrNull() const;
    WId windowId() const;

Q_SIGNALS:
    void surfaceStatusChanged(const std::optional<QSize> &optSurfaceSize);
    void externalContentClickDetected();

private:
    struct JsStateData
    {
        std::shared_ptr<QArkUi::QQtEmbeddedWindowNode> embeddedWindow;
    };

    void handleSurfaceEvent(
        QArkUi::QXComponentCallbackReceiver::SurfaceEventType eventType,
        ::OHNativeWindow *nativeWindow,
        const std::optional<QSize> &optSurfaceSize);

    QtOhos::JsScopeDataOwner<JsStateData> m_jsStateData;
    std::unique_ptr<QOhosSurface> m_optSurface;
    QRectF m_nodeGeometry;
    std::optional<QXComponentNode> m_nodeXComponentParent;
};

QT_END_NAMESPACE

#endif
