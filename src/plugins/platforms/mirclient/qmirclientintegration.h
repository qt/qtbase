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


#ifndef QMIRCLIENTINTEGRATION_H
#define QMIRCLIENTINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <QSharedPointer>

#include "qmirclientplatformservices.h"

// platform-api
#include <ubuntu/application/description.h>
#include <ubuntu/application/instance.h>

class QMirClientClipboard;
class QMirClientInput;
class QMirClientNativeInterface;
class QMirClientScreen;

class QMirClientClientIntegration : public QPlatformIntegration {
public:
    QMirClientClientIntegration();
    virtual ~QMirClientClientIntegration();

    // QPlatformIntegration methods.
    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;
    QPlatformNativeInterface* nativeInterface() const override;
    QPlatformBackingStore* createPlatformBackingStore(QWindow* window) const override;
    QPlatformOpenGLContext* createPlatformOpenGLContext(QOpenGLContext* context) const override;
    QPlatformFontDatabase* fontDatabase() const override { return mFontDb; }
    QStringList themeNames() const override;
    QPlatformTheme* createPlatformTheme(const QString& name) const override;
    QVariant styleHint(StyleHint hint) const override;
    QPlatformServices *services() const override;
    QPlatformWindow* createPlatformWindow(QWindow* window) const override;
    QPlatformInputContext* inputContext() const override { return mInputContext; }
    QPlatformClipboard* clipboard() const override;

    QPlatformOpenGLContext* createPlatformOpenGLContext(QOpenGLContext* context);
    QPlatformWindow* createPlatformWindow(QWindow* window);
    QMirClientScreen* screen() const;

private:
    void setupOptions();
    void setupDescription();

    QMirClientNativeInterface* mNativeInterface;
    QPlatformFontDatabase* mFontDb;

    QMirClientPlatformServices* mServices;

    QMirClientInput* mInput;
    QPlatformInputContext* mInputContext;
    QSharedPointer<QMirClientClipboard> mClipboard;
    qreal mScaleFactor;

    // Platform API stuff
    UApplicationOptions* mOptions;
    UApplicationDescription* mDesc;
    UApplicationInstance* mInstance;
};

#endif // QMIRCLIENTINTEGRATION_H
