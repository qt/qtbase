/****************************************************************************
**
** Copyright (C) 2014 Canonical, Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QMIRCLIENTNATIVEINTERFACE_H
#define QMIRCLIENTNATIVEINTERFACE_H

#include <qpa/qplatformnativeinterface.h>

class QMirClientNativeInterface : public QPlatformNativeInterface {
public:
    enum ResourceType { EglDisplay, EglContext, NativeOrientation, Display, MirConnection };

    QMirClientNativeInterface();
    ~QMirClientNativeInterface();

    // QPlatformNativeInterface methods.
    void* nativeResourceForIntegration(const QByteArray &resource) override;
    void* nativeResourceForContext(const QByteArray& resourceString,
                                   QOpenGLContext* context) override;
    void* nativeResourceForWindow(const QByteArray& resourceString,
                                  QWindow* window) override;
    void* nativeResourceForScreen(const QByteArray& resourceString,
                                  QScreen* screen) override;

    // New methods.
    const QByteArray& genericEventFilterType() const { return mGenericEventFilterType; }
    void setMirConnection(void *mirConnection) { mMirConnection = mirConnection; }

private:
    const QByteArray mGenericEventFilterType;
    Qt::ScreenOrientation* mNativeOrientation;
    void *mMirConnection;
};

#endif // QMIRCLIENTNATIVEINTERFACE_H
