// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QRHIWIDGET_H
#define QRHIWIDGET_H

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QRhiWidgetPrivate;
class QRhi;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiRenderTarget;
class QRhiCommandBuffer;

class Q_WIDGETS_EXPORT QRhiWidget : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QRhiWidget)
    Q_PROPERTY(int sampleCount READ sampleCount WRITE setSampleCount NOTIFY sampleCountChanged)
    Q_PROPERTY(TextureFormat textureFormat READ textureFormat WRITE setTextureFormat NOTIFY textureFormatChanged)
    Q_PROPERTY(bool autoRenderTarget READ isAutoRenderTargetEnabled WRITE setAutoRenderTarget NOTIFY autoRenderTargetChanged)
    Q_PROPERTY(QSize explicitSize READ explicitSize WRITE setExplicitSize NOTIFY explicitSizeChanged)
    Q_PROPERTY(bool mirrorVertically READ isMirrorVerticallyEnabled WRITE setMirrorVertically NOTIFY mirrorVerticallyChanged)

public:
    QRhiWidget(QWidget *parent = nullptr, Qt::WindowFlags f = {});
    ~QRhiWidget();

    enum class Api {
        OpenGL,
        Metal,
        Vulkan,
        D3D11,
        D3D12,
        Null
    };
    Q_ENUM(Api)

    enum class TextureFormat {
        RGBA8,
        RGBA16F,
        RGBA32F,
        RGB10A2
    };
    Q_ENUM(TextureFormat)

    Api api() const;
    void setApi(Api api);

    bool isDebugLayerEnabled() const;
    void setDebugLayer(bool enable);

    int sampleCount() const;
    void setSampleCount(int samples);

    TextureFormat textureFormat() const;
    void setTextureFormat(TextureFormat format);

    QSize explicitSize() const;
    void setExplicitSize(const QSize &pixelSize);
    void setExplicitSize(int w, int h) { setExplicitSize(QSize(w, h)); }

    bool isAutoRenderTargetEnabled() const;
    void setAutoRenderTarget(bool enabled);

    bool isMirrorVerticallyEnabled() const;
    void setMirrorVertically(bool enabled);

    QImage grab();

    virtual void initialize(QRhiCommandBuffer *cb);
    virtual void render(QRhiCommandBuffer *cb);
    virtual void releaseResources();

    QRhi *rhi() const;
    QRhiTexture *colorTexture() const;
    QRhiRenderBuffer *msaaColorBuffer() const;
    QRhiTexture *resolveTexture() const;
    QRhiRenderBuffer *depthStencilBuffer() const;
    QRhiRenderTarget *renderTarget() const;

Q_SIGNALS:
    void frameSubmitted();
    void renderFailed();
    void sampleCountChanged(int samples);
    void textureFormatChanged(TextureFormat format);
    void autoRenderTargetChanged(bool enabled);
    void explicitSizeChanged(const QSize &pixelSize);
    void mirrorVerticallyChanged(bool enabled);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    bool event(QEvent *e) override;
};

QT_END_NAMESPACE

#endif
