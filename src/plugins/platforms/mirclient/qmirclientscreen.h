/****************************************************************************
**
** Copyright (C) 2014-2015 Canonical, Ltd.
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


#ifndef QMIRCLIENTSCREEN_H
#define QMIRCLIENTSCREEN_H

#include <qpa/qplatformscreen.h>
#include <QSurfaceFormat>
#include <EGL/egl.h>

#include "qmirclientcursor.h"

struct MirConnection;

class QMirClientScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT
public:
    QMirClientScreen(MirConnection *connection);
    virtual ~QMirClientScreen();

    // QPlatformScreen methods.
    QImage::Format format() const override { return mFormat; }
    int depth() const override { return mDepth; }
    QRect geometry() const override { return mGeometry; }
    QRect availableGeometry() const override { return mGeometry; }
    QSizeF physicalSize() const override { return mPhysicalSize; }
    Qt::ScreenOrientation nativeOrientation() const override { return mNativeOrientation; }
    Qt::ScreenOrientation orientation() const override { return mNativeOrientation; }
    QPlatformCursor *cursor() const override { return const_cast<QMirClientCursor*>(&mCursor); }

    // New methods.
    QSurfaceFormat surfaceFormat() const { return mSurfaceFormat; }
    EGLDisplay eglDisplay() const { return mEglDisplay; }
    EGLConfig eglConfig() const { return mEglConfig; }
    EGLNativeDisplayType eglNativeDisplay() const { return mEglNativeDisplay; }
    void handleWindowSurfaceResize(int width, int height);
    uint32_t mirOutputId() const { return mOutputId; }

    // QObject methods.
    void customEvent(QEvent* event) override;

private:
    QRect mGeometry;
    QSizeF mPhysicalSize;
    Qt::ScreenOrientation mNativeOrientation;
    Qt::ScreenOrientation mCurrentOrientation;
    QImage::Format mFormat;
    int mDepth;
    uint32_t mOutputId;
    QSurfaceFormat mSurfaceFormat;
    EGLDisplay mEglDisplay;
    EGLConfig mEglConfig;
    EGLNativeDisplayType mEglNativeDisplay;
    QMirClientCursor mCursor;
};

#endif // QMIRCLIENTSCREEN_H
