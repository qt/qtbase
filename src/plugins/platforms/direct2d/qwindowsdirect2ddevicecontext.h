// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSDIRECT2DDEVICECONTEXT_H
#define QWINDOWSDIRECT2DDEVICECONTEXT_H

#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

/*
 * Convenience class for handling device contexts. We have to call BeginDraw
 * before anything can happen, and EndDraw once we're done, for every frame and
 * pretty much any kind of operation.
 *
 * Unfortunately, these calls cannot be interleaved, and there is no way to check
 * what state a device context is in.
 *
 * The end result is that the following throws an error if we don't track it:
 *      QPixmap pmap;
 *      QPainter painter(&pmap);
 *      pmap.clear();
 *
 * Here BeginDraw would first be called through the paint device, then when we clear
 * the pixmap we would have to call it again. There is no way to know what state
 * the device context is in when performing the clear, and activating the dc is an
 * error. Bummer.
 *
 * Hence we keep a reference count here and only activate/deactivate the device
 * if the refcount is zero.
 *
 * In a nutshell: Do not call BeginDraw/EndDraw yourself on the device pointer, do
 * so through the begin/end members below.
 */

class QWindowsDirect2DDeviceContextPrivate;
class QWindowsDirect2DDeviceContext
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DDeviceContext)
    friend class QWindowsDirect2DDeviceContextSuspender;
public:
    QWindowsDirect2DDeviceContext(ID2D1DeviceContext *dc);
    ~QWindowsDirect2DDeviceContext();

    ID2D1DeviceContext *get() const;

    void begin();
    bool end();

private:
    void suspend();
    void resume();

    QScopedPointer<QWindowsDirect2DDeviceContextPrivate> d_ptr;
};

class QWindowsDirect2DDeviceContextSuspender {
    Q_DISABLE_COPY_MOVE(QWindowsDirect2DDeviceContextSuspender)

    QWindowsDirect2DDeviceContext *m_dc;
public:
    QWindowsDirect2DDeviceContextSuspender(QWindowsDirect2DDeviceContext *dc);
    ~QWindowsDirect2DDeviceContextSuspender();

    void resume();
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DDEVICECONTEXT_H
