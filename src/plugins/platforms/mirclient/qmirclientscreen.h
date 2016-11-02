/****************************************************************************
**
** Copyright (C) 2014-2016 Canonical, Ltd.
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


#ifndef QMIRCLIENTSCREEN_H
#define QMIRCLIENTSCREEN_H

#include <qpa/qplatformscreen.h>
#include <QSurfaceFormat>

#include <mir_toolkit/common.h> // just for MirFormFactor enum

#include "qmirclientcursor.h"

struct MirConnection;
struct MirOutput;

class QMirClientScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT
public:
    QMirClientScreen(const MirOutput *output, MirConnection *connection);
    virtual ~QMirClientScreen();

    // QPlatformScreen methods.
    QImage::Format format() const override { return mFormat; }
    int depth() const override { return mDepth; }
    QRect geometry() const override { return mGeometry; }
    QRect availableGeometry() const override { return mGeometry; }
    QSizeF physicalSize() const override { return mPhysicalSize; }
    qreal devicePixelRatio() const override { return mDevicePixelRatio; }
    QDpi logicalDpi() const override;
    Qt::ScreenOrientation nativeOrientation() const override { return mNativeOrientation; }
    Qt::ScreenOrientation orientation() const override { return mNativeOrientation; }
    QPlatformCursor *cursor() const override { return const_cast<QMirClientCursor*>(&mCursor); }

    // Additional Screen properties from Mir
    int mirOutputId() const { return mOutputId; }
    MirFormFactor formFactor() const { return mFormFactor; }
    float scale() const { return mScale; }

    // Internally used methods
    void updateMirOutput(const MirOutput *output);
    void setAdditionalMirDisplayProperties(float scale, MirFormFactor formFactor, int dpi);
    void handleWindowSurfaceResize(int width, int height);

    // QObject methods.
    void customEvent(QEvent* event) override;

private:
    void setMirOutput(const MirOutput *output);

    QRect mGeometry, mNativeGeometry;
    QSizeF mPhysicalSize;
    qreal mDevicePixelRatio;
    Qt::ScreenOrientation mNativeOrientation;
    Qt::ScreenOrientation mCurrentOrientation;
    QImage::Format mFormat;
    int mDepth;
    int mDpi;
    qreal mRefreshRate;
    MirFormFactor mFormFactor;
    float mScale;
    int mOutputId;
    QMirClientCursor mCursor;

    friend class QMirClientNativeInterface;
};

#endif // QMIRCLIENTSCREEN_H
