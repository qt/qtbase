/****************************************************************************
**
** Copyright (C) 2014-2015 Canonical, Ltd.
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


// local
#include "qmirclientscreen.h"
#include "qmirclientlogging.h"
#include "qmirclientorientationchangeevent_p.h"

#include <mir_toolkit/mir_client_library.h>

// Qt
#include <QCoreApplication>
#include <QtCore/qmath.h>
#include <QScreen>
#include <QThread>
#include <qpa/qwindowsysteminterface.h>
#include <QtEglSupport/private/qeglconvenience_p.h>

#include <memory>

static const int kSwapInterval = 1;

#if !defined(QT_NO_DEBUG)

static const char *orientationToStr(Qt::ScreenOrientation orientation) {
    switch (orientation) {
        case Qt::PrimaryOrientation:
            return "primary";
        case Qt::PortraitOrientation:
            return "portrait";
        case Qt::LandscapeOrientation:
            return "landscape";
        case Qt::InvertedPortraitOrientation:
            return "inverted portrait";
        case Qt::InvertedLandscapeOrientation:
            return "inverted landscape";
        default:
            return "INVALID!";
    }
}

static void printEglConfig(EGLDisplay display, EGLConfig config) {
  DASSERT(display != EGL_NO_DISPLAY);
  DASSERT(config != nullptr);
  static const struct { const EGLint attrib; const char* name; } kAttribs[] = {
    { EGL_BUFFER_SIZE, "EGL_BUFFER_SIZE" },
    { EGL_ALPHA_SIZE, "EGL_ALPHA_SIZE" },
    { EGL_BLUE_SIZE, "EGL_BLUE_SIZE" },
    { EGL_GREEN_SIZE, "EGL_GREEN_SIZE" },
    { EGL_RED_SIZE, "EGL_RED_SIZE" },
    { EGL_DEPTH_SIZE, "EGL_DEPTH_SIZE" },
    { EGL_STENCIL_SIZE, "EGL_STENCIL_SIZE" },
    { EGL_CONFIG_CAVEAT, "EGL_CONFIG_CAVEAT" },
    { EGL_CONFIG_ID, "EGL_CONFIG_ID" },
    { EGL_LEVEL, "EGL_LEVEL" },
    { EGL_MAX_PBUFFER_HEIGHT, "EGL_MAX_PBUFFER_HEIGHT" },
    { EGL_MAX_PBUFFER_PIXELS, "EGL_MAX_PBUFFER_PIXELS" },
    { EGL_MAX_PBUFFER_WIDTH, "EGL_MAX_PBUFFER_WIDTH" },
    { EGL_NATIVE_RENDERABLE, "EGL_NATIVE_RENDERABLE" },
    { EGL_NATIVE_VISUAL_ID, "EGL_NATIVE_VISUAL_ID" },
    { EGL_NATIVE_VISUAL_TYPE, "EGL_NATIVE_VISUAL_TYPE" },
    { EGL_SAMPLES, "EGL_SAMPLES" },
    { EGL_SAMPLE_BUFFERS, "EGL_SAMPLE_BUFFERS" },
    { EGL_SURFACE_TYPE, "EGL_SURFACE_TYPE" },
    { EGL_TRANSPARENT_TYPE, "EGL_TRANSPARENT_TYPE" },
    { EGL_TRANSPARENT_BLUE_VALUE, "EGL_TRANSPARENT_BLUE_VALUE" },
    { EGL_TRANSPARENT_GREEN_VALUE, "EGL_TRANSPARENT_GREEN_VALUE" },
    { EGL_TRANSPARENT_RED_VALUE, "EGL_TRANSPARENT_RED_VALUE" },
    { EGL_BIND_TO_TEXTURE_RGB, "EGL_BIND_TO_TEXTURE_RGB" },
    { EGL_BIND_TO_TEXTURE_RGBA, "EGL_BIND_TO_TEXTURE_RGBA" },
    { EGL_MIN_SWAP_INTERVAL, "EGL_MIN_SWAP_INTERVAL" },
    { EGL_MAX_SWAP_INTERVAL, "EGL_MAX_SWAP_INTERVAL" },
    { -1, NULL }
  };
  const char* string = eglQueryString(display, EGL_VENDOR);
  LOG("EGL vendor: %s", string);
  string = eglQueryString(display, EGL_VERSION);
  LOG("EGL version: %s", string);
  string = eglQueryString(display, EGL_EXTENSIONS);
  LOG("EGL extensions: %s", string);
  LOG("EGL configuration attibutes:");
  for (int index = 0; kAttribs[index].attrib != -1; index++) {
    EGLint value;
    if (eglGetConfigAttrib(display, config, kAttribs[index].attrib, &value))
      LOG("  %s: %d", kAttribs[index].name, static_cast<int>(value));
  }
}
#endif


const QEvent::Type OrientationChangeEvent::mType =
        static_cast<QEvent::Type>(QEvent::registerEventType());

static const MirDisplayOutput *find_active_output(
    const MirDisplayConfiguration *conf)
{
    const MirDisplayOutput *output = NULL;
    for (uint32_t d = 0; d < conf->num_outputs; d++)
    {
        const MirDisplayOutput *out = conf->outputs + d;

        if (out->used &&
            out->connected &&
            out->num_modes &&
            out->current_mode < out->num_modes)
        {
            output = out;
            break;
        }
    }

    return output;
}

QMirClientScreen::QMirClientScreen(MirConnection *connection)
    : mFormat(QImage::Format_RGB32)
    , mDepth(32)
    , mOutputId(0)
    , mSurfaceFormat()
    , mEglDisplay(EGL_NO_DISPLAY)
    , mEglConfig(nullptr)
    , mCursor(connection)
{
    // Initialize EGL.
    ASSERT(eglBindAPI(EGL_OPENGL_ES_API) == EGL_TRUE);

    mEglNativeDisplay = mir_connection_get_egl_native_display(connection);
    ASSERT((mEglDisplay = eglGetDisplay(mEglNativeDisplay)) != EGL_NO_DISPLAY);
    ASSERT(eglInitialize(mEglDisplay, nullptr, nullptr) == EGL_TRUE);

    // Configure EGL buffers format.
    mSurfaceFormat.setRedBufferSize(8);
    mSurfaceFormat.setGreenBufferSize(8);
    mSurfaceFormat.setBlueBufferSize(8);
    mSurfaceFormat.setAlphaBufferSize(8);
    mSurfaceFormat.setDepthBufferSize(24);
    mSurfaceFormat.setStencilBufferSize(8);
    if (!qEnvironmentVariableIsEmpty("QTUBUNTU_MULTISAMPLE")) {
        mSurfaceFormat.setSamples(4);
        DLOG("ubuntumirclient: setting MSAA to 4 samples");
    }
#ifdef QTUBUNTU_USE_OPENGL
    mSurfaceFormat.setRenderableType(QSurfaceFormat::OpenGL);
#else
    mSurfaceFormat.setRenderableType(QSurfaceFormat::OpenGLES);
#endif
    mEglConfig = q_configFromGLFormat(mEglDisplay, mSurfaceFormat, true);

    #if !defined(QT_NO_DEBUG)
    printEglConfig(mEglDisplay, mEglConfig);
    #endif

    // Set vblank swap interval.
    int swapInterval = kSwapInterval;
    QByteArray swapIntervalString = qgetenv("QTUBUNTU_SWAPINTERVAL");
    if (!swapIntervalString.isEmpty()) {
        bool ok;
        swapInterval = swapIntervalString.toInt(&ok);
        if (!ok)
            swapInterval = kSwapInterval;
    }
    DLOG("ubuntumirclient: setting swap interval to %d", swapInterval);
    eglSwapInterval(mEglDisplay, swapInterval);

    // Get screen resolution.
    auto configDeleter = [](MirDisplayConfiguration *config) { mir_display_config_destroy(config); };
    using configUp = std::unique_ptr<MirDisplayConfiguration, decltype(configDeleter)>;
    configUp displayConfig(mir_connection_create_display_config(connection), configDeleter);
    ASSERT(displayConfig != nullptr);

    auto const displayOutput = find_active_output(displayConfig.get());
    ASSERT(displayOutput != nullptr);

    mOutputId = displayOutput->output_id;

    mPhysicalSize = QSizeF(displayOutput->physical_width_mm, displayOutput->physical_height_mm);
    DLOG("ubuntumirclient: screen physical size: %.2fx%.2f", mPhysicalSize.width(), mPhysicalSize.height());

    const MirDisplayMode *mode = &displayOutput->modes[displayOutput->current_mode];
    const int kScreenWidth = mode->horizontal_resolution;
    const int kScreenHeight = mode->vertical_resolution;
    DASSERT(kScreenWidth > 0 && kScreenHeight > 0);

    DLOG("ubuntumirclient: screen resolution: %dx%d", kScreenWidth, kScreenHeight);

    mGeometry = QRect(0, 0, kScreenWidth, kScreenHeight);

    DLOG("QQMirClientScreen::QQMirClientScreen (this=%p)", this);

    // Set the default orientation based on the initial screen dimmensions.
    mNativeOrientation = (mGeometry.width() >= mGeometry.height()) ? Qt::LandscapeOrientation : Qt::PortraitOrientation;

    // If it's a landscape device (i.e. some tablets), start in landscape, otherwise portrait
    mCurrentOrientation = (mNativeOrientation == Qt::LandscapeOrientation) ? Qt::LandscapeOrientation : Qt::PortraitOrientation;
}

QMirClientScreen::~QMirClientScreen()
{
    eglTerminate(mEglDisplay);
}

void QMirClientScreen::customEvent(QEvent* event) {
    DASSERT(QThread::currentThread() == thread());

    OrientationChangeEvent* oReadingEvent = static_cast<OrientationChangeEvent*>(event);
    switch (oReadingEvent->mOrientation) {
        case OrientationChangeEvent::LeftUp: {
            mCurrentOrientation = (screen()->primaryOrientation() == Qt::LandscapeOrientation) ?
                        Qt::InvertedPortraitOrientation : Qt::LandscapeOrientation;
            break;
        }
        case OrientationChangeEvent::TopUp: {
            mCurrentOrientation = (screen()->primaryOrientation() == Qt::LandscapeOrientation) ?
                        Qt::LandscapeOrientation : Qt::PortraitOrientation;
            break;
        }
        case OrientationChangeEvent::RightUp: {
            mCurrentOrientation = (screen()->primaryOrientation() == Qt::LandscapeOrientation) ?
                        Qt::PortraitOrientation : Qt::InvertedLandscapeOrientation;
            break;
        }
        case OrientationChangeEvent::TopDown: {
            mCurrentOrientation = (screen()->primaryOrientation() == Qt::LandscapeOrientation) ?
                        Qt::InvertedLandscapeOrientation : Qt::InvertedPortraitOrientation;
            break;
        }
        default: {
            DLOG("QMirClientScreen::customEvent - Unknown orientation.");
            return;
        }
    }

    // Raise the event signal so that client apps know the orientation changed
    DLOG("QMirClientScreen::customEvent - handling orientation change to %s", orientationToStr(mCurrentOrientation));
    QWindowSystemInterface::handleScreenOrientationChange(screen(), mCurrentOrientation);
}

void QMirClientScreen::handleWindowSurfaceResize(int windowWidth, int windowHeight)
{
    if ((windowWidth > windowHeight && mGeometry.width() < mGeometry.height())
     || (windowWidth < windowHeight && mGeometry.width() > mGeometry.height())) {

        // The window aspect ratio differ's from the screen one. This means that
        // unity8 has rotated the window in its scene.
        // As there's no way to express window rotation in Qt's API, we have
        // Flip QScreen's dimensions so that orientation properties match
        // (primaryOrientation particularly).
        // FIXME: This assumes a phone scenario. Won't work, or make sense,
        //        on the desktop

        QRect currGeometry = mGeometry;
        mGeometry.setWidth(currGeometry.height());
        mGeometry.setHeight(currGeometry.width());

        DLOG("QMirClientScreen::handleWindowSurfaceResize - new screen geometry (w=%d, h=%d)",
            mGeometry.width(), mGeometry.height());
        QWindowSystemInterface::handleScreenGeometryChange(screen(),
                                                           mGeometry /* newGeometry */,
                                                           mGeometry /* newAvailableGeometry */);

        if (mGeometry.width() < mGeometry.height()) {
            mCurrentOrientation = Qt::PortraitOrientation;
        } else {
            mCurrentOrientation = Qt::LandscapeOrientation;
        }
        DLOG("QMirClientScreen::handleWindowSurfaceResize - new orientation %s",orientationToStr(mCurrentOrientation));
        QWindowSystemInterface::handleScreenOrientationChange(screen(), mCurrentOrientation);
    }
}
