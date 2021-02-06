/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QPLATFORMINTEGRATION_VKKHRDISPLAY_H
#define QPLATFORMINTEGRATION_VKKHRDISPLAY_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class QVkKhrDisplayScreen;
class QVkKhrDisplayVulkanInstance;
class QFbVtHandler;

class QVkKhrDisplayIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    explicit QVkKhrDisplayIntegration(const QStringList &parameters);
    ~QVkKhrDisplayIntegration();

    void initialize() override;

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformFontDatabase *fontDatabase() const override;
    QPlatformServices *services() const override;
    QPlatformInputContext *inputContext() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;
    QPlatformNativeInterface *nativeInterface() const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;

    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;

private:
    static void handleInstanceCreated(QVkKhrDisplayVulkanInstance *, void *);
    void createInputHandlers();

    mutable QPlatformFontDatabase *m_fontDatabase = nullptr;
    mutable QPlatformServices *m_services = nullptr;
    QPlatformInputContext *m_inputContext = nullptr;
    QFbVtHandler *m_vtHandler = nullptr;
    QVkKhrDisplayScreen *m_primaryScreen = nullptr;
};

QT_END_NAMESPACE

#endif
