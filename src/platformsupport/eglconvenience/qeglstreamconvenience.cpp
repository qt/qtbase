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

#include "qeglstreamconvenience_p.h"
#include <string.h>

QT_BEGIN_NAMESPACE

QEGLStreamConvenience::QEGLStreamConvenience()
    : initialized(false),
      has_egl_platform_device(false),
      has_egl_device_base(false),
      has_egl_stream(false),
      has_egl_stream_producer_eglsurface(false),
      has_egl_stream_consumer_egloutput(false),
      has_egl_output_drm(false),
      has_egl_output_base(false),
      has_egl_stream_cross_process_fd(false),
      has_egl_stream_consumer_gltexture(false)
{
    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (!extensions) {
        qWarning("Failed to query EGL extensions");
        return;
    }

    query_devices = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(eglGetProcAddress("eglQueryDevicesEXT"));
    query_device_string = reinterpret_cast<PFNEGLQUERYDEVICESTRINGEXTPROC>(eglGetProcAddress("eglQueryDeviceStringEXT"));
    get_platform_display = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));

    has_egl_device_base = strstr(extensions, "EGL_EXT_device_base");
    has_egl_platform_device = strstr(extensions, "EGL_EXT_platform_device");
}

void QEGLStreamConvenience::initialize(EGLDisplay dpy)
{
    if (initialized)
        return;

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        qWarning("Failed to bind OpenGL ES API");
        return;
    }

    const char *extensions = eglQueryString(dpy, EGL_EXTENSIONS);
    if (!extensions) {
        qWarning("Failed to query EGL extensions");
        return;
    }

    create_stream = reinterpret_cast<PFNEGLCREATESTREAMKHRPROC>(eglGetProcAddress("eglCreateStreamKHR"));
    destroy_stream = reinterpret_cast<PFNEGLDESTROYSTREAMKHRPROC>(eglGetProcAddress("eglDestroyStreamKHR"));
    stream_attrib = reinterpret_cast<PFNEGLSTREAMATTRIBKHRPROC>(eglGetProcAddress("eglStreamAttribKHR"));
    query_stream = reinterpret_cast<PFNEGLQUERYSTREAMKHRPROC>(eglGetProcAddress("eglQueryStreamKHR"));
    query_stream_u64 = reinterpret_cast<PFNEGLQUERYSTREAMU64KHRPROC>(eglGetProcAddress("eglQueryStreamu64KHR"));
    create_stream_producer_surface = reinterpret_cast<PFNEGLCREATESTREAMPRODUCERSURFACEKHRPROC>(eglGetProcAddress("eglCreateStreamProducerSurfaceKHR"));
    stream_consumer_output = reinterpret_cast<PFNEGLSTREAMCONSUMEROUTPUTEXTPROC>(eglGetProcAddress("eglStreamConsumerOutputEXT"));
    get_output_layers = reinterpret_cast<PFNEGLGETOUTPUTLAYERSEXTPROC>(eglGetProcAddress("eglGetOutputLayersEXT"));
    get_output_ports = reinterpret_cast<PFNEGLGETOUTPUTPORTSEXTPROC>(eglGetProcAddress("eglGetOutputPortsEXT"));
    output_layer_attrib = reinterpret_cast<PFNEGLOUTPUTLAYERATTRIBEXTPROC>(eglGetProcAddress("eglOutputLayerAttribEXT"));
    query_output_layer_attrib = reinterpret_cast<PFNEGLQUERYOUTPUTLAYERATTRIBEXTPROC>(eglGetProcAddress("eglQueryOutputLayerAttribEXT"));
    query_output_layer_string = reinterpret_cast<PFNEGLQUERYOUTPUTLAYERSTRINGEXTPROC>(eglGetProcAddress("eglQueryOutputLayerStringEXT"));
    query_output_port_attrib = reinterpret_cast<PFNEGLQUERYOUTPUTPORTATTRIBEXTPROC>(eglGetProcAddress("eglQueryOutputPortAttribEXT"));
    query_output_port_string = reinterpret_cast<PFNEGLQUERYOUTPUTPORTSTRINGEXTPROC>(eglGetProcAddress("eglQueryOutputPortStringEXT"));
    get_stream_file_descriptor = reinterpret_cast<PFNEGLGETSTREAMFILEDESCRIPTORKHRPROC>(eglGetProcAddress("eglGetStreamFileDescriptorKHR"));
    create_stream_from_file_descriptor = reinterpret_cast<PFNEGLCREATESTREAMFROMFILEDESCRIPTORKHRPROC>(eglGetProcAddress("eglCreateStreamFromFileDescriptorKHR"));
    stream_consumer_gltexture = reinterpret_cast<PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALKHRPROC>(eglGetProcAddress("eglStreamConsumerGLTextureExternalKHR"));
    stream_consumer_acquire = reinterpret_cast<PFNEGLSTREAMCONSUMERACQUIREKHRPROC>(eglGetProcAddress("eglStreamConsumerAcquireKHR"));
    stream_consumer_release = reinterpret_cast<PFNEGLSTREAMCONSUMERRELEASEKHRPROC>(eglGetProcAddress("eglStreamConsumerReleaseKHR"));
    create_stream_attrib_nv = reinterpret_cast<PFNEGLCREATESTREAMATTRIBNVPROC>(eglGetProcAddress("eglCreateStreamAttribNV"));
    set_stream_attrib_nv = reinterpret_cast<PFNEGLSETSTREAMATTRIBNVPROC>(eglGetProcAddress("eglSetStreamAttribNV"));
    query_stream_attrib_nv = reinterpret_cast<PFNEGLQUERYSTREAMATTRIBNVPROC>(eglGetProcAddress("eglQueryStreamAttribNV"));
    acquire_stream_attrib_nv = reinterpret_cast<PFNEGLSTREAMCONSUMERACQUIREATTRIBNVPROC>(eglGetProcAddress("eglStreamConsumerAcquireAttribNV"));
    release_stream_attrib_nv = reinterpret_cast<PFNEGLSTREAMCONSUMERRELEASEATTRIBNVPROC>(eglGetProcAddress("eglStreamConsumerReleaseAttribNV"));

    has_egl_stream = strstr(extensions, "EGL_KHR_stream");
    has_egl_stream_producer_eglsurface = strstr(extensions, "EGL_KHR_stream_producer_eglsurface");
    has_egl_stream_consumer_egloutput = strstr(extensions, "EGL_EXT_stream_consumer_egloutput");
    has_egl_output_drm = strstr(extensions, "EGL_EXT_output_drm");
    has_egl_output_base = strstr(extensions, "EGL_EXT_output_base");
    has_egl_stream_cross_process_fd = strstr(extensions, "EGL_KHR_stream_cross_process_fd");
    has_egl_stream_consumer_gltexture = strstr(extensions, "EGL_KHR_stream_consumer_gltexture");

    initialized = true;
}

QT_END_NAMESPACE
