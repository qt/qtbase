/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QBACKINGSTORERHISUPPORT_P_H
#define QBACKINGSTORERHISUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qwindow.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/private/qrhi_p.h>
#include <qpa/qplatformbackingstore.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QBackingStoreRhiSupport
{
public:
    ~QBackingStoreRhiSupport();

    void reset();

    void setFormat(const QSurfaceFormat &format) { m_format = format; }
    void setWindow(QWindow *window) { m_window = window; }
    void setConfig(const QPlatformBackingStoreRhiConfig &config) { m_config = config; }

    bool create();

    QRhiSwapChain *swapChainForWindow(QWindow *window);

    static QSurface::SurfaceType surfaceTypeForConfig(const QPlatformBackingStoreRhiConfig &config);

    static bool checkForceRhi(QPlatformBackingStoreRhiConfig *outConfig, QSurface::SurfaceType *outType);

    static QRhi::Implementation apiToRhiBackend(QPlatformBackingStoreRhiConfig::Api api);
    static const char *apiName(QPlatformBackingStoreRhiConfig::Api api);

    QRhi *rhi() const { return m_rhi; }

private:
    QSurfaceFormat m_format;
    QWindow *m_window = nullptr;
    QPlatformBackingStoreRhiConfig m_config;
    QRhi *m_rhi = nullptr;
    QOffscreenSurface *m_openGLFallbackSurface = nullptr;
    struct SwapchainData {
        QRhiSwapChain *swapchain = nullptr;
        QRhiRenderPassDescriptor *renderPassDescriptor = nullptr;
        QObject *windowWatcher = nullptr;
        void reset();
    };
    QHash<QWindow *, SwapchainData> m_swapchains;
    friend class QBackingStoreRhiSupportWindowWatcher;
};

class QBackingStoreRhiSupportWindowWatcher : public QObject
{
public:
    QBackingStoreRhiSupportWindowWatcher(QBackingStoreRhiSupport *rhiSupport) : m_rhiSupport(rhiSupport) { }
    bool eventFilter(QObject *obj, QEvent *ev) override;
private:
    QBackingStoreRhiSupport *m_rhiSupport;
};

QT_END_NAMESPACE

#endif // QBACKINGSTORERHISUPPORT_P_H
