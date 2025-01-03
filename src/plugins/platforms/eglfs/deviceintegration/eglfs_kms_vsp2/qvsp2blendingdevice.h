// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVSP2BLENDINGDEVICE_H
#define QVSP2BLENDINGDEVICE_H

#include <QtCore/QList>
#include <QtCore/QRect>
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
    QList<struct Input> m_inputs;
    const QSize m_screenSize;
    bool m_dirty = true;
};

QT_END_NAMESPACE

#endif // QVSP2BLENDINGDEVICE
