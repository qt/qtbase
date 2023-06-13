// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RHIWIDGET_H
#define RHIWIDGET_H

#include <QWidget>
#include <rhi/qrhi.h>

class QRhiWidgetPrivate;

class QRhiWidget : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QRhiWidget)
    Q_PROPERTY(QSize explicitSize READ explicitSize WRITE setExplicitSize NOTIFY explicitSizeChanged)

public:
    QRhiWidget(QWidget *parent = nullptr, Qt::WindowFlags f = {});
    ~QRhiWidget();

    enum Api {
        OpenGL,
        Metal,
        Vulkan,
        D3D11,
        Null
    };

    Api api() const;
    void setApi(Api api);

    bool isDebugLayerEnabled() const;
    void setDebugLayer(bool enable);

    QRhiTexture::Format textureFormat() const;
    void setTextureFormat(QRhiTexture::Format format);

    QSize explicitSize() const;
    void setExplicitSize(const QSize &pixelSize);

    virtual void initialize(QRhi *rhi, QRhiTexture *outputTexture);
    virtual void render(QRhiCommandBuffer *cb);

    QImage grabTexture();

Q_SIGNALS:
    void explicitSizeChanged(const QSize &pixelSize);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    bool event(QEvent *e) override;
};

#endif
