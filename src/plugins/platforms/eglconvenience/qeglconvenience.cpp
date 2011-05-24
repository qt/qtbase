/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglconvenience.h"

QT_BEGIN_NAMESPACE

QVector<EGLint> q_createConfigAttributesFromFormat(const QPlatformWindowFormat &format)
{
    int redSize     = format.redBufferSize();
    int greenSize   = format.greenBufferSize();
    int blueSize    = format.blueBufferSize();
    int alphaSize   = format.alphaBufferSize();
    int depthSize   = format.depthBufferSize();
    int stencilSize = format.stencilBufferSize();
    int sampleCount = format.samples();

    // QPlatformWindowFormat uses a magic value of -1 to indicate "don't care", even when a buffer of that
    // type has been requested. So we must check QPlatformWindowFormat's booleans too if size is -1:
    if (format.alpha() && alphaSize <= 0)
        alphaSize = 1;
    if (format.depth() && depthSize <= 0)
        depthSize = 1;
    if (format.stencil() && stencilSize <= 0)
        stencilSize = 1;
    if (format.sampleBuffers() && sampleCount <= 0)
        sampleCount = 1;

    // We want to make sure 16-bit configs are chosen over 32-bit configs as they will provide
    // the best performance. The EGL config selection algorithm is a bit stange in this regard:
    // The selection criteria for EGL_BUFFER_SIZE is "AtLeast", so we can't use it to discard
    // 32-bit configs completely from the selection. So it then comes to the sorting algorithm.
    // The red/green/blue sizes have a sort priority of 3, so they are sorted by first. The sort
    // order is special and described as "by larger _total_ number of color bits.". So EGL will
    // put 32-bit configs in the list before the 16-bit configs. However, the spec also goes on
    // to say "If the requested number of bits in attrib_list for a particular component is 0,
    // then the number of bits for that component is not considered". This part of the spec also
    // seems to imply that setting the red/green/blue bits to zero means none of the components
    // are considered and EGL disregards the entire sorting rule. It then looks to the next
    // highest priority rule, which is EGL_BUFFER_SIZE. Despite the selection criteria being
    // "AtLeast" for EGL_BUFFER_SIZE, it's sort order is "smaller" meaning 16-bit configs are
    // put in the list before 32-bit configs. So, to make sure 16-bit is preffered over 32-bit,
    // we must set the red/green/blue sizes to zero. This has an unfortunate consequence that
    // if the application sets the red/green/blue size to 5/6/5 on the QPlatformWindowFormat,
    // they will probably get a 32-bit config, even when there's an RGB565 config available.

    // Now normalize the values so -1 becomes 0
    redSize   = redSize   > 0 ? redSize   : 0;
    greenSize = greenSize > 0 ? greenSize : 0;
    blueSize  = blueSize  > 0 ? blueSize  : 0;
    alphaSize = alphaSize > 0 ? alphaSize : 0;
    depthSize = depthSize > 0 ? depthSize : 0;
    stencilSize = stencilSize > 0 ? stencilSize : 0;
    sampleCount = sampleCount > 0 ? sampleCount : 0;

    QVector<EGLint> configAttributes;

    configAttributes.append(EGL_RED_SIZE);
    configAttributes.append(redSize);

    configAttributes.append(EGL_GREEN_SIZE);
    configAttributes.append(greenSize);

    configAttributes.append(EGL_BLUE_SIZE);
    configAttributes.append(blueSize);

    configAttributes.append(EGL_ALPHA_SIZE);
    configAttributes.append(alphaSize);

    configAttributes.append(EGL_DEPTH_SIZE);
    configAttributes.append(depthSize);

    configAttributes.append(EGL_STENCIL_SIZE);
    configAttributes.append(stencilSize);

    configAttributes.append(EGL_SAMPLES);
    configAttributes.append(sampleCount);

    configAttributes.append(EGL_SAMPLE_BUFFERS);
    configAttributes.append(sampleCount? 1:0);

    return configAttributes;
}

bool q_reduceConfigAttributes(QVector<EGLint> *configAttributes)
{
    int i = -1;
    // Reduce the complexity of a configuration request to ask for less
    // because the previous request did not result in success.  Returns
    // true if the complexity was reduced, or false if no further
    // reductions in complexity are possible.

    i = configAttributes->indexOf(EGL_SWAP_BEHAVIOR);
    if (i >= 0) {
        configAttributes->remove(i,2);
    }

#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
    // For OpenVG, we sometimes try to create a surface using a pre-multiplied format. If we can't
    // find a config which supports pre-multiplied formats, remove the flag on the surface type:

    i = configAttributes->indexOf(EGL_SURFACE_TYPE);
    if (i >= 0) {
        EGLint surfaceType = configAttributes->at(i +1);
        if (surfaceType & EGL_VG_ALPHA_FORMAT_PRE_BIT) {
            surfaceType ^= EGL_VG_ALPHA_FORMAT_PRE_BIT;
            configAttributes->replace(i+1,surfaceType);
            return true;
        }
    }
#endif

    // EGL chooses configs with the highest color depth over
    // those with smaller (but faster) lower color depths. One
    // way around this is to set EGL_BUFFER_SIZE to 16, which
    // trumps the others. Of course, there may not be a 16-bit
    // config available, so it's the first restraint we remove.
    i = configAttributes->indexOf(EGL_BUFFER_SIZE);
    if (i >= 0) {
        if (configAttributes->at(i+1) == 16) {
            configAttributes->remove(i,2);
            return true;
        }
    }

    i = configAttributes->indexOf(EGL_SAMPLE_BUFFERS);
    if (i >= 0) {
        configAttributes->remove(i,2);
        i = configAttributes->indexOf(EGL_SAMPLES);
        if (i >= 0) {
            configAttributes->remove(i,2);
        }
        return true;
    }

    i = configAttributes->indexOf(EGL_ALPHA_SIZE);
    if (i >= 0) {
        configAttributes->remove(i,2);
#if defined(EGL_BIND_TO_TEXTURE_RGBA) && defined(EGL_BIND_TO_TEXTURE_RGB)
        i = configAttributes->indexOf(EGL_BIND_TO_TEXTURE_RGBA);
        if (i >= 0) {
            configAttributes->replace(i,EGL_BIND_TO_TEXTURE_RGB);
            configAttributes->replace(i+1,TRUE);

        }
#endif
        return true;
    }

    i = configAttributes->indexOf(EGL_STENCIL_SIZE);
    if (i >= 0) {
        configAttributes->remove(i,2);
        return true;
    }
    i = configAttributes->indexOf(EGL_DEPTH_SIZE);
    if (i >= 0) {
        configAttributes->remove(i,2);
        return true;
    }
#ifdef EGL_BIND_TO_TEXTURE_RGB
    i = configAttributes->indexOf(EGL_BIND_TO_TEXTURE_RGB);
    if (i >= 0) {
        configAttributes->remove(i,2);
        return true;
    }
#endif

    return false;
}

EGLConfig q_configFromQPlatformWindowFormat(EGLDisplay display, const QPlatformWindowFormat &format, bool highestPixelFormat, int surfaceType)
{
    EGLConfig cfg = 0;
    QVector<EGLint> configureAttributes = q_createConfigAttributesFromFormat(format);
    configureAttributes.append(EGL_SURFACE_TYPE); //we only support eglconfigs for windows for now
    configureAttributes.append(surfaceType);

    configureAttributes.append(EGL_RENDERABLE_TYPE);
    if (format.windowApi() == QPlatformWindowFormat::OpenVG) {
        configureAttributes.append(EGL_OPENVG_BIT);        
    } else {
        configureAttributes.append(EGL_OPENGL_ES2_BIT);
    }
    configureAttributes.append(EGL_NONE);

    do {
        // Get the number of matching configurations for this set of properties.
        EGLint matching = 0;
        if (!eglChooseConfig(display, configureAttributes.constData(), 0, 0, &matching) || !matching)
            continue;

        // If we want the best pixel format, then return the first
        // matching configuration.
        if (highestPixelFormat) {
            eglChooseConfig(display, configureAttributes.constData(), &cfg, 1, &matching);
            if (matching < 1)
                continue;
            return cfg;
        }

        // Fetch all of the matching configurations and find the
        // first that matches the pixel format we wanted.
        int i = configureAttributes.indexOf(EGL_RED_SIZE);
        int confAttrRed = configureAttributes.at(i+1);
        i = configureAttributes.indexOf(EGL_GREEN_SIZE);
        int confAttrGreen = configureAttributes.at(i+1);
        i = configureAttributes.indexOf(EGL_BLUE_SIZE);
        int confAttrBlue = configureAttributes.at(i+1);
        i = configureAttributes.indexOf(EGL_ALPHA_SIZE);
        int confAttrAlpha = configureAttributes.at(i+1);

        EGLint size = matching;
        EGLConfig *configs = new EGLConfig [size];
        eglChooseConfig(display, configureAttributes.constData(), configs, size, &matching);
        for (EGLint index = 0; index < size; ++index) {
            EGLint red, green, blue, alpha;
            eglGetConfigAttrib(display, configs[index], EGL_RED_SIZE, &red);
            eglGetConfigAttrib(display, configs[index], EGL_GREEN_SIZE, &green);
            eglGetConfigAttrib(display, configs[index], EGL_BLUE_SIZE, &blue);
            eglGetConfigAttrib(display, configs[index], EGL_ALPHA_SIZE, &alpha);
            if (red == confAttrRed &&
                    green == confAttrGreen &&
                    blue == confAttrBlue &&
                    (confAttrAlpha == 0 ||
                     alpha == confAttrAlpha)) {
                cfg = configs[index];
                delete [] configs;
                return cfg;
            }
        }
        delete [] configs;
    } while (q_reduceConfigAttributes(&configureAttributes));
    qWarning("Cant find EGLConfig, returning null config");
    return 0;
}

QPlatformWindowFormat qt_qPlatformWindowFormatFromConfig(EGLDisplay display, const EGLConfig config)
{
    QPlatformWindowFormat format;
    EGLint redSize     = 0;
    EGLint greenSize   = 0;
    EGLint blueSize    = 0;
    EGLint alphaSize   = 0;
    EGLint depthSize   = 0;
    EGLint stencilSize = 0;
    EGLint sampleCount = 0;
    EGLint level       = 0;

    eglGetConfigAttrib(display, config, EGL_RED_SIZE,     &redSize);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE,   &greenSize);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE,    &blueSize);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE,   &alphaSize);
    eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE,   &depthSize);
    eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &stencilSize);
    eglGetConfigAttrib(display, config, EGL_SAMPLES,      &sampleCount);
    eglGetConfigAttrib(display, config, EGL_LEVEL,        &level);

    format.setRedBufferSize(redSize);
    format.setGreenBufferSize(greenSize);
    format.setBlueBufferSize(blueSize);
    format.setAlphaBufferSize(alphaSize);
    format.setDepthBufferSize(depthSize);
    format.setStencilBufferSize(stencilSize);
    format.setSamples(sampleCount);
    format.setDirectRendering(true); // All EGL contexts are direct-rendered
    format.setRgba(true);            // EGL doesn't support colour index rendering
    format.setStereo(false);         // EGL doesn't support stereo buffers
    format.setAccumBufferSize(0);    // EGL doesn't support accululation buffers

    // Clear the EGL error state because some of the above may
    // have errored out because the attribute is not applicable
    // to the surface type.  Such errors don't matter.
    eglGetError();

    return format;
}

bool q_hasEglExtension(EGLDisplay display, const char* extensionName)
{
    QList<QByteArray> extensions =
        QByteArray(reinterpret_cast<const char *>
            (eglQueryString(display, EGL_EXTENSIONS))).split(' ');
    return extensions.contains(extensionName);
}

QT_END_NAMESPACE
