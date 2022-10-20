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
