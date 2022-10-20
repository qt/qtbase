/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
