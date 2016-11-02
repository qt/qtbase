/****************************************************************************
**
** Copyright (C) 2016 Canonical, Ltd.
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


#ifndef QMIRCLIENTNATIVEINTERFACE_H
#define QMIRCLIENTNATIVEINTERFACE_H

#include <qpa/qplatformnativeinterface.h>

#include "qmirclientintegration.h"

class QPlatformScreen;

class QMirClientNativeInterface : public QPlatformNativeInterface {
    Q_OBJECT
public:
    enum ResourceType { EglDisplay, EglContext, NativeOrientation, Display, MirConnection, MirSurface, Scale, FormFactor };

    QMirClientNativeInterface(const QMirClientClientIntegration *integration);
    ~QMirClientNativeInterface();

    // QPlatformNativeInterface methods.
    void* nativeResourceForIntegration(const QByteArray &resource) override;
    void* nativeResourceForContext(const QByteArray& resourceString,
                                   QOpenGLContext* context) override;
    void* nativeResourceForWindow(const QByteArray& resourceString,
                                  QWindow* window) override;
    void* nativeResourceForScreen(const QByteArray& resourceString,
                                  QScreen* screen) override;

    QVariantMap windowProperties(QPlatformWindow *window) const override;
    QVariant windowProperty(QPlatformWindow *window, const QString &name) const override;
    QVariant windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const override;

    // New methods.
    const QByteArray& genericEventFilterType() const { return mGenericEventFilterType; }

Q_SIGNALS: // New signals
    void screenPropertyChanged(QPlatformScreen *screen, const QString &propertyName);

private:
    const QMirClientClientIntegration *mIntegration;
    const QByteArray mGenericEventFilterType;
    Qt::ScreenOrientation* mNativeOrientation;
};

#endif // QMIRCLIENTNATIVEINTERFACE_H
