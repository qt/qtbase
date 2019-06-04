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

#include "qeglfsopenwfdintegration.h"

#include "wfd.h"
#include "wfdext2.h"

QT_BEGIN_NAMESPACE

typedef struct wfd_buffer {
    WFD_EGLImageType*   image;
    WFDSource           source;
} wfd_buffer_t;

typedef struct wfd_window {
    WFDDevice       dev;
    WFDPort         port;
    WFDPipeline     pipeline;
    int             numBuffers;
    wfd_buffer_t    buffers[MAX_NUM_OF_WFD_BUFFERS];
} wfd_window_t;

void QEglFSOpenWFDIntegration::platformInit()
{
    QEglFSDeviceIntegration::platformInit();

    mNativeDisplay = EGL_DEFAULT_DISPLAY;

    // Get device list
    WFDint numDevs = wfdEnumerateDevices(nullptr, 0, nullptr);
    WFDint devIds[MAX_NUM_OF_WFD_DEVICES];

    if (numDevs > 0)
        wfdEnumerateDevices(devIds, numDevs, nullptr);

    // Create device
    WFDint dev_attribs[3] = {WFD_DEVICE_CLIENT_TYPE,
                             WFD_CLIENT_ID_CLUSTER,
                             WFD_NONE};

    bool ok;
    WFDint clientType = qgetenv("QT_OPENWFD_CLIENT_ID").toInt(&ok, 16);

    if (ok)
        dev_attribs[1] = clientType;

    mDevice = wfdCreateDevice(WFD_DEFAULT_DEVICE_ID, dev_attribs);

    if (WFD_INVALID_HANDLE == mDevice)
        qFatal( "Failed to create wfd device");

    // Get port list
    WFDint portIds[MAX_NUM_OF_WFD_PORTS];
    WFDint numPorts = wfdEnumeratePorts(mDevice, nullptr, 0, nullptr);
    wfdEnumeratePorts(mDevice, portIds, numPorts, nullptr);

    // Create port
    mPort = wfdCreatePort(mDevice, portIds[0], nullptr);

    if (WFD_INVALID_HANDLE == mPort)
        qFatal("Failed to create wfd port");

    // Get port modes
    WFDint numPortModes = wfdGetPortModes(mDevice, mPort, nullptr, 0);
    WFDPortMode portModes[MAX_NUM_OF_WFD_PORT_MODES];
    wfdGetPortModes(mDevice, mPort, portModes, numPortModes);

    // Get width and height
    mScreenSize.setWidth(wfdGetPortModeAttribi(mDevice, mPort, portModes[0], WFD_PORT_MODE_WIDTH));
    mScreenSize.setHeight(wfdGetPortModeAttribi(mDevice, mPort, portModes[0], WFD_PORT_MODE_HEIGHT));

    // Set port mode
    wfdSetPortMode(mDevice, mPort, portModes[0]);
    WFDErrorCode eError = wfdGetError(mDevice);
    if (WFD_ERROR_NONE != eError)
        qFatal("Failed to set wfd port mode");

    // Power on
    wfdSetPortAttribi(mDevice, mPort, WFD_PORT_POWER_MODE, WFD_POWER_MODE_ON);
    eError = wfdGetError(mDevice);
    if (WFD_ERROR_NONE != eError)
        qFatal("Failed to power on wfd port");

    // Get list of pipelines
    WFDint numPipelines = wfdEnumeratePipelines(mDevice, nullptr, 0, nullptr);
    WFDint pipelineIds[MAX_NUM_OF_WFD_PIPELINES];
    wfdEnumeratePipelines(mDevice, pipelineIds, numPipelines, nullptr);

    WFDint pipelineId = qgetenv("QT_OPENWFD_PIPELINE_ID").toInt(&ok);

    if (!ok)
        pipelineId = pipelineIds[0];

    mPipeline = wfdCreatePipeline(mDevice, pipelineId, nullptr);
    if (WFD_INVALID_HANDLE == mPipeline)
        qFatal("Failed to create wfd pipeline");

    wfdSetPipelineAttribi(mDevice, mPipeline, WFD_PIPELINE_TRANSPARENCY_ENABLE,
                          (WFD_TRANSPARENCY_SOURCE_ALPHA|WFD_TRANSPARENCY_GLOBAL_ALPHA));

    eError = wfdGetError(mDevice);
    if (WFD_ERROR_NONE != eError)
        qFatal("Failed to set WFD_PIPELINE_TRANSPARENCY_ENABLE");

    wfdSetPipelineAttribi(mDevice, mPipeline, WFD_PIPELINE_GLOBAL_ALPHA, 255);
    eError = wfdGetError(mDevice);
    if (WFD_ERROR_NONE != eError)
        qFatal("Failed to set WFD_PIPELINE_GLOBAL_ALPHA");

    wfdBindPipelineToPort(mDevice, mPort, mPipeline);
    eError = wfdGetError(mDevice);
    if (WFD_ERROR_NONE != eError)
        qFatal("Failed to bind port to pipeline");


    for (int i = 0; i < MAX_NUM_OF_WFD_BUFFERS; i++) {
        wfdCreateWFDEGLImages(mDevice, mScreenSize.width(), mScreenSize.height(),
                              WFD_FORMAT_RGBA8888, WFD_USAGE_OPENGL_ES2 | WFD_USAGE_DISPLAY,
                              1, &(mEGLImageHandles[i]), 0);

        mWFDEglImages[i] = (WFD_EGLImageType *)(mEGLImageHandles[i]);
        if (WFD_INVALID_HANDLE == mWFDEglImages[i])
            qFatal("Failed to create WDFEGLImages");

        mSources[i] = wfdCreateSourceFromImage(mDevice, mPipeline, mEGLImageHandles[i], nullptr);
        if (WFD_INVALID_HANDLE == mSources[i])
            qFatal("Failed to create source from EGLImage");
    }
}

QSize QEglFSOpenWFDIntegration::screenSize() const
{
    return mScreenSize;
}

EGLNativeDisplayType QEglFSOpenWFDIntegration::platformDisplay() const
{
    return mNativeDisplay;
}

EGLNativeWindowType QEglFSOpenWFDIntegration::createNativeWindow(QPlatformWindow *window,
                                                                 const QSize &size,
                                                                 const QSurfaceFormat &format)
{
    Q_UNUSED(window);
    Q_UNUSED(format);

    // Create native window
    wfd_window_t* nativeWindow = (wfd_window_t*)malloc(sizeof(wfd_window_t));
    if (nullptr == nativeWindow)
        qFatal("Failed to allocate memory for native window");

    nativeWindow->dev = mDevice;
    nativeWindow->port = mPort;
    nativeWindow->pipeline = mPipeline;
    nativeWindow->numBuffers = MAX_NUM_OF_WFD_BUFFERS;

    WFDErrorCode eError;

    for (int i = 0; i < MAX_NUM_OF_WFD_BUFFERS; i++) {
        nativeWindow->buffers[i].image  = mWFDEglImages[i];
        nativeWindow->buffers[i].source = mSources[i];
        wfdBindSourceToPipeline(nativeWindow->dev, nativeWindow->pipeline, nativeWindow->buffers[i].source , WFD_TRANSITION_AT_VSYNC, nullptr);
        eError = wfdGetError(nativeWindow->dev);
        if (WFD_ERROR_NONE != eError)
        {
            qFatal("wfdBindSourceToPipeline eError=0x%08x", eError);
        }
    }

    // Commit port
    wfdDeviceCommit(mDevice, WFD_COMMIT_ENTIRE_PORT, mPort);
    eError = wfdGetError(mDevice);
    if (WFD_ERROR_NONE != eError)
        qFatal("Failed to commit port");

    return (EGLNativeWindowType)nativeWindow;
}

QSurfaceFormat QEglFSOpenWFDIntegration::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat format = inputFormat;
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setAlphaBufferSize(8);
    return format;
}

void QEglFSOpenWFDIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    wfdBindSourceToPipeline(mDevice, ((wfd_window_t*)window)->pipeline, nullptr, WFD_TRANSITION_IMMEDIATE, nullptr);

    for (int i = 0; i < MAX_NUM_OF_WFD_BUFFERS; i++) {
        wfdDestroySource(mDevice, mSources[i]);
        wfdDestroyWFDEGLImages(mDevice, 1, &mEGLImageHandles[i]);
    }

    free((void*)window);
}

QT_END_NAMESPACE
