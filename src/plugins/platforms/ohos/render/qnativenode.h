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
#include <qarkui/qqtembeddedwindownode.h>
#include <qohosforeignwindow.h>
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
    struct CreateInfo
    {
        QRect geometry;
        QWindow *window;
        QOhosOptional<QNativeNode *> optParent;
        QOhosOptional<QColor> backgroundColor;
        QOhosOptional<QOhosPlatformWindow::NativeNodeRenderFitPolicy> renderFitPolicyHint;
    };

    explicit QNativeNode(const CreateInfo &nativeNodeCreateInfo);

    void setParent(std::shared_ptr<QXComponentNode> xComponent);
    void setParent(QNativeNode &other);
    void detachFromParentIfPresent();

    void setSize(const QSizeF &size);
    void setSizeParentFillPercentNormalized(const QSizeF &size);
    void setPosition(QPoint position);
    void setVisibility(bool visible);
    void fillToParent(const QSize &surfaceResolution);
    void raise();
    void lower();
    void setFocused(bool focused);
    void setFocusable(bool focusable);
    void setBackgroundColor(const QColor &color);
    void setBrightness(int brightness);
    void setContrast(int contrast);
    void setSaturation(int saturation);
    void setTransparentForInput(bool transparentForInput);
    void startDrag(
        const std::vector<QImage> &images, const QPointF &hotspot,
        const QMimeData &mimeData, QOhosConsumer<Qt::DropAction> dropActionConsumer);
    void setNodeAreaChangeHandler(QOhosConsumer<QArkUi::QQtEmbeddedWindowNode::NodeAreaInfo> areaChangeEventConsumer);
    void setNodeFocusChangeHandler(QOhosConsumer<bool> focusedChangedConsumer);
    void setNodeVisibilityChangeHandler(QOhosConsumer<bool> visibilityChangedConsumer);
    QRect nodeScreenGeometryPixels() const;
    QRect nodeParentRelativeGeometryPixels() const;

    void addForeignWindowChild(QOhosForeignWindow *foreignWindow);

    QRectF geometry() const;
    QOhosSurface *surfaceOrNull() const;
    WId windowId() const;

Q_SIGNALS:
    void surfaceStatusChanged(const QOhosOptional<QSize> &optSurfaceSize);
    void externalContentClickDetected();

private:
    struct JsStateData
    {
        std::shared_ptr<QArkUi::QQtEmbeddedWindowNode> embeddedWindow;
    };

    void handleSurfaceEvent(
        QArkUi::QXComponentCallbackReceiver::SurfaceEventType eventType,
        ::OHNativeWindow *nativeWindow,
        const QOhosOptional<QSize> &optSurfaceSize);

    std::shared_ptr<JsStateData> m_jsStateData;
    std::unique_ptr<QOhosSurface> m_optSurface;
    QRectF m_nodeGeometry;
    QOhosOptional<QXComponentNode> m_nodeXComponentParent;
};

QT_END_NAMESPACE

#endif
