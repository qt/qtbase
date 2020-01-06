/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#ifndef QANDROIDPLATFORMINTERATION_H
#define QANDROIDPLATFORMINTERATION_H

#include <QtGui/qtguiglobal.h>

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformmenu.h>
#include <qpa/qplatformnativeinterface.h>

#include <EGL/egl.h>
#include <jni.h>
#include "qandroidinputcontext.h"

#include "qandroidplatformscreen.h"

#include <memory>

QT_BEGIN_NAMESPACE

class QDesktopWidget;
class QAndroidPlatformServices;
class QAndroidSystemLocale;
class QPlatformAccessibility;

struct AndroidStyle;
class QAndroidPlatformNativeInterface: public QPlatformNativeInterface
{
public:
    void *nativeResourceForIntegration(const QByteArray &resource) override;
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;
    std::shared_ptr<AndroidStyle> m_androidStyle;

protected:
    void customEvent(QEvent *event) override;
};

class QAndroidPlatformIntegration : public QPlatformIntegration
{
    friend class QAndroidPlatformScreen;

public:
    QAndroidPlatformIntegration(const QStringList &paramList);
    ~QAndroidPlatformIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;
    QAndroidPlatformScreen *screen() { return m_primaryScreen; }
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;

    virtual void setDesktopSize(int width, int height);
    virtual void setDisplayMetrics(int width, int height);
    void setScreenSize(int width, int height);
    bool isVirtualDesktop() { return true; }

    QPlatformFontDatabase *fontDatabase() const override;

#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *clipboard() const override;
#endif

    QPlatformInputContext *inputContext() const override;
    QPlatformNativeInterface *nativeInterface() const override;
    QPlatformServices *services() const override;

#ifndef QT_NO_ACCESSIBILITY
    virtual QPlatformAccessibility *accessibility() const override;
#endif

    QVariant styleHint(StyleHint hint) const override;
    Qt::WindowState defaultWindowState(Qt::WindowFlags flags) const override;

    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;

    static void setDefaultDisplayMetrics(int gw, int gh, int sw, int sh, int width, int height);
    static void setDefaultDesktopSize(int gw, int gh);
    static void setScreenOrientation(Qt::ScreenOrientation currentOrientation,
                                     Qt::ScreenOrientation nativeOrientation);

    static QSize defaultDesktopSize()
    {
        return QSize(m_defaultGeometryWidth, m_defaultGeometryHeight);
    }

    QTouchDevice *touchDevice() const { return m_touchDevice; }
    void setTouchDevice(QTouchDevice *touchDevice) { m_touchDevice = touchDevice; }

    void flushPendingUpdates();

#if QT_CONFIG(vulkan)
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;
#endif

private:
    EGLDisplay m_eglDisplay;
    QTouchDevice *m_touchDevice;

    QAndroidPlatformScreen *m_primaryScreen;

    QThread *m_mainThread;

    static int m_defaultGeometryWidth;
    static int m_defaultGeometryHeight;
    static int m_defaultPhysicalSizeWidth;
    static int m_defaultPhysicalSizeHeight;
    static int m_defaultScreenWidth;
    static int m_defaultScreenHeight;

    static Qt::ScreenOrientation m_orientation;
    static Qt::ScreenOrientation m_nativeOrientation;
    static bool m_showPasswordEnabled;

    QPlatformFontDatabase *m_androidFDB;
    QAndroidPlatformNativeInterface *m_androidPlatformNativeInterface;
    QAndroidPlatformServices *m_androidPlatformServices;

#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *m_androidPlatformClipboard;
#endif

    QAndroidSystemLocale *m_androidSystemLocale;
#ifndef QT_NO_ACCESSIBILITY
    mutable QPlatformAccessibility *m_accessibility;
#endif

    mutable QAndroidInputContext m_platformInputContext;
};

QT_END_NAMESPACE

#endif
