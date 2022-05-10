// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINTEGRATION_DIRECTFB_H
#define QPLATFORMINTEGRATION_DIRECTFB_H

#include "qdirectfbinput.h"
#include "qdirectfbscreen.h"

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <directfb.h>
#include <directfb_version.h>

QT_BEGIN_NAMESPACE

class QThread;
class QAbstractEventDispatcher;

class QDirectFbIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    QDirectFbIntegration();
    ~QDirectFbIntegration();

    void connectToDirectFb();

    bool hasCapability(Capability cap) const;
    QPlatformPixmap *createPlatformPixmap(QPlatformPixmap::PixelType type) const;
    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;
    QAbstractEventDispatcher *createEventDispatcher() const;

    QPlatformFontDatabase *fontDatabase() const;
    QPlatformServices *services() const;
    QPlatformInputContext *inputContext() const { return m_inputContext; }
    QPlatformNativeInterface *nativeInterface() const;

protected:
    virtual void initializeDirectFB();
    virtual void initializeScreen();
    virtual void initializeInput();

protected:
    QDirectFBPointer<IDirectFB> m_dfb;
    QScopedPointer<QDirectFbScreen> m_primaryScreen;
    QScopedPointer<QDirectFbInput> m_input;
    QScopedPointer<QThread> m_inputRunner;
    QScopedPointer<QPlatformFontDatabase> m_fontDb;
    QScopedPointer<QPlatformServices> m_services;
    QPlatformInputContext *m_inputContext;
};

QT_END_NAMESPACE

#endif
