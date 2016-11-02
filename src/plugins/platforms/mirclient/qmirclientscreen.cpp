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


// local
#include "qmirclientscreen.h"
#include "qmirclientlogging.h"
#include "qmirclientorientationchangeevent_p.h"
#include "qmirclientnativeinterface.h"

#include <mir_toolkit/mir_client_library.h>

// Qt
#include <QGuiApplication>
#include <QtCore/qmath.h>
#include <QScreen>
#include <QThread>
#include <qpa/qwindowsysteminterface.h>
#include <QtEglSupport/private/qeglconvenience_p.h>

#include <memory>

static const int overrideDevicePixelRatio = qgetenv("QT_DEVICE_PIXEL_RATIO").toInt();

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
    }
    Q_UNREACHABLE();
}

const QEvent::Type OrientationChangeEvent::mType =
        static_cast<QEvent::Type>(QEvent::registerEventType());


QMirClientScreen::QMirClientScreen(const MirOutput *output, MirConnection *connection)
    : mDevicePixelRatio(1.0)
    , mFormat(QImage::Format_RGB32)
    , mDepth(32)
    , mDpi{0}
    , mFormFactor{mir_form_factor_unknown}
    , mScale{1.0}
    , mOutputId(0)
    , mCursor(connection)
{
    setMirOutput(output);
}

QMirClientScreen::~QMirClientScreen()
{
}

void QMirClientScreen::customEvent(QEvent* event) {
    Q_ASSERT(QThread::currentThread() == thread());

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
    }

    // Raise the event signal so that client apps know the orientation changed
    qCDebug(mirclient, "QMirClientScreen::customEvent - handling orientation change to %s", orientationToStr(mCurrentOrientation));
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

        qCDebug(mirclient, "QMirClientScreen::handleWindowSurfaceResize - new screen geometry (w=%d, h=%d)",
            mGeometry.width(), mGeometry.height());
        QWindowSystemInterface::handleScreenGeometryChange(screen(),
                                                           mGeometry /* newGeometry */,
                                                           mGeometry /* newAvailableGeometry */);

        if (mGeometry.width() < mGeometry.height()) {
            mCurrentOrientation = Qt::PortraitOrientation;
        } else {
            mCurrentOrientation = Qt::LandscapeOrientation;
        }
        qCDebug(mirclient, "QMirClientScreen::handleWindowSurfaceResize - new orientation %s",orientationToStr(mCurrentOrientation));
        QWindowSystemInterface::handleScreenOrientationChange(screen(), mCurrentOrientation);
    }
}

void QMirClientScreen::setMirOutput(const MirOutput *output)
{
    // Physical screen size (in mm)
    mPhysicalSize.setWidth(mir_output_get_physical_width_mm(output));
    mPhysicalSize.setHeight(mir_output_get_physical_height_mm(output));

    // Pixel Format
//    mFormat = qImageFormatFromMirPixelFormat(mir_output_get_current_pixel_format(output)); // GERRY: TODO

    // Pixel depth
    mDepth = 8 * MIR_BYTES_PER_PIXEL(mir_output_get_current_pixel_format(output));

    // Mode = Resolution & refresh rate
    const MirOutputMode *mode = mir_output_get_current_mode(output);
    mNativeGeometry.setX(mir_output_get_position_x(output));
    mNativeGeometry.setY(mir_output_get_position_y(output));
    mNativeGeometry.setWidth(mir_output_mode_get_width(mode));
    mNativeGeometry.setHeight(mir_output_mode_get_height(mode));

    mRefreshRate = mir_output_mode_get_refresh_rate(mode);

    // UI scale & DPR
    mScale = mir_output_get_scale_factor(output);
    if (overrideDevicePixelRatio > 0) {
        mDevicePixelRatio = overrideDevicePixelRatio;
    } else {
        mDevicePixelRatio = 1.0; // FIXME - need to determine suitable DPR for the specified scale
    }

    mFormFactor = mir_output_get_form_factor(output);

    mOutputId = mir_output_get_id(output);

    mGeometry.setX(mNativeGeometry.x());
    mGeometry.setY(mNativeGeometry.y());
    mGeometry.setWidth(mNativeGeometry.width());
    mGeometry.setHeight(mNativeGeometry.height());

    // Set the default orientation based on the initial screen dimensions.
    mNativeOrientation = (mGeometry.width() >= mGeometry.height()) ? Qt::LandscapeOrientation : Qt::PortraitOrientation;

    // If it's a landscape device (i.e. some tablets), start in landscape, otherwise portrait
    mCurrentOrientation = (mNativeOrientation == Qt::LandscapeOrientation) ? Qt::LandscapeOrientation : Qt::PortraitOrientation;
}

void QMirClientScreen::updateMirOutput(const MirOutput *output)
{
    auto oldRefreshRate = mRefreshRate;
    auto oldScale = mScale;
    auto oldFormFactor = mFormFactor;
    auto oldGeometry = mGeometry;

    setMirOutput(output);

    // Emit change signals in particular order
    if (oldGeometry != mGeometry) {
        QWindowSystemInterface::handleScreenGeometryChange(screen(),
                                                           mGeometry /* newGeometry */,
                                                           mGeometry /* newAvailableGeometry */);
    }

    if (!qFuzzyCompare(mRefreshRate, oldRefreshRate)) {
        QWindowSystemInterface::handleScreenRefreshRateChange(screen(), mRefreshRate);
    }

    auto nativeInterface = static_cast<QMirClientNativeInterface *>(qGuiApp->platformNativeInterface());
    if (!qFuzzyCompare(mScale, oldScale)) {
        nativeInterface->screenPropertyChanged(this, QStringLiteral("scale"));
    }
    if (mFormFactor != oldFormFactor) {
        nativeInterface->screenPropertyChanged(this, QStringLiteral("formFactor"));
    }
}

void QMirClientScreen::setAdditionalMirDisplayProperties(float scale, MirFormFactor formFactor, int dpi)
{
    if (mDpi != dpi) {
        mDpi = dpi;
        QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen(), dpi, dpi);
    }

    auto nativeInterface = static_cast<QMirClientNativeInterface *>(qGuiApp->platformNativeInterface());
    if (!qFuzzyCompare(mScale, scale)) {
        mScale = scale;
        nativeInterface->screenPropertyChanged(this, QStringLiteral("scale"));
    }
    if (mFormFactor != formFactor) {
        mFormFactor = formFactor;
        nativeInterface->screenPropertyChanged(this, QStringLiteral("formFactor"));
    }
}

QDpi QMirClientScreen::logicalDpi() const
{
    if (mDpi > 0) {
        return QDpi(mDpi, mDpi);
    } else {
        return QPlatformScreen::logicalDpi();
    }
}
