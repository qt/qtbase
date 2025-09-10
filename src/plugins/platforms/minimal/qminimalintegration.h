// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINTEGRATION_MINIMAL_H
#define QPLATFORMINTEGRATION_MINIMAL_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>

#include <qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QMinimalScreen : public QPlatformScreen
{
public:
    QMinimalScreen()
        : mDepth(32), mFormat(QImage::Format_ARGB32_Premultiplied) {}

    QRect geometry() const override { return mGeometry; }
    int depth() const override { return mDepth; }
    QImage::Format format() const override { return mFormat; }

public:
    QRect mGeometry;
    int mDepth;
    QImage::Format mFormat;
    QSize mPhysicalSize;
};

class QMinimalIntegration : public QPlatformIntegration
{
public:
    enum Options { // Options to be passed on command line or determined from environment
        DebugBackingStore = 0x1,
        EnableFonts = 0x2,
        FreeTypeFontDatabase = 0x4,
        FontconfigDatabase = 0x8
    };

    explicit QMinimalIntegration(const QStringList &parameters);
    ~QMinimalIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformFontDatabase *fontDatabase() const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;

    QPlatformNativeInterface *nativeInterface() const override;

    unsigned options() const { return m_options; }

    static QMinimalIntegration *instance();

private:
    mutable QPlatformFontDatabase *m_fontDatabase;
    mutable QScopedPointer<QPlatformNativeInterface> m_nativeInterface;
    QMinimalScreen *m_primaryScreen;
    unsigned m_options;
};

QT_END_NAMESPACE

#endif
