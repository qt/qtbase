/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QVSP2BLENDINGDEVICE_H
#define QVSP2BLENDINGDEVICE_H

#include <QtCore/QRect>
#include <QtCore/QVector>
#include <QtCore/qglobal.h>

#include "qlinuxmediadevice.h"

QT_BEGIN_NAMESPACE

class QSize;

class QVsp2BlendingDevice
{
public:
    QVsp2BlendingDevice(const QSize& screenSize); //TODO: add support for output format as well?
    bool enableInput(int i, const QRect &bufferGeometry, uint drmFormat, uint bytesPerLine);
    int enableInput(const QRect &bufferGeometry, uint drmFormat, uint bytesPerLine);
    bool disableInput(int i);
    bool setInputBuffer(int index, int dmabufFd);
    bool setInputPosition(int index, const QPoint &position);
    bool setInputAlpha(int index, qreal alpha);
    bool blend(int outputDmabufFd);
    int numInputs() const;
    bool isDirty() const { return m_dirty; }
    bool hasContinuousLayers() const;
private:
    bool streamOn();
    bool streamOff();
    bool setInputFormat(int i, const QRect &bufferGeometry, uint pixelFormat, uint bytesPerLine);
    QLinuxMediaDevice m_mediaDevice;
    QLinuxMediaDevice::CaptureSubDevice *m_wpfOutput = nullptr; // wpf output
    struct Input {
        bool enabled = false;
        QRect geometry;
        qreal alpha = 1;
        struct {
            int fd = -1;
            uint bytesUsed = 0;
            uint length = 0;
        } dmabuf;
        struct media_link *linkToBru = nullptr; //rpf.x:1 -> bru:x
        struct media_pad *inputFormatPad = nullptr; // rpf.x:0
        struct media_pad *outputFormatPad = nullptr; // rpf.x:1
        int outputFormatFd = -1; // rpf.x:1 (again, because v4l2_subdev_* doesn't have a way to set alpha)
        struct media_pad *bruInputFormatPad = nullptr; // bru:x
        QLinuxMediaDevice::OutputSubDevice *rpfInput = nullptr; // rpf.x input
    };
    QVector<struct Input> m_inputs;
    const QSize m_screenSize;
    bool m_dirty = true;
};

QT_END_NAMESPACE

#endif // QVSP2BLENDINGDEVICE
