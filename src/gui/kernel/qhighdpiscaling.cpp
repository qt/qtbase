/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qhighdpiscaling_p.h"
#include "qguiapplication.h"
#include "qscreen.h"
#include "qplatformintegration.h"
#include "private/qscreen_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcScaling, "qt.scaling");

#ifndef QT_NO_HIGHDPISCALING
static const char legacyDevicePixelEnvVar[] = "QT_DEVICE_PIXEL_RATIO";
static const char scaleFactorEnvVar[] = "QT_SCALE_FACTOR";
static const char autoScreenEnvVar[] = "QT_AUTO_SCREEN_SCALE_FACTOR";
static const char screenFactorsEnvVar[] = "QT_SCREEN_SCALE_FACTORS";

static inline qreal initialGlobalScaleFactor()
{

    qreal result = 1;
    if (qEnvironmentVariableIsSet(scaleFactorEnvVar)) {
        bool ok;
        const qreal f = qgetenv(scaleFactorEnvVar).toDouble(&ok);
        if (ok && f > 0) {
            qCDebug(lcScaling) << "Apply " << scaleFactorEnvVar << f;
            result = f;
        }
    } else {
        if (qEnvironmentVariableIsSet(legacyDevicePixelEnvVar)) {
            qWarning("Warning: %s is deprecated. Instead use:\n"
                     "   %s to enable platform plugin controlled per-screen factors.\n"
                     "   %s to set per-screen factors.\n"
                     "   %s to set the application global scale factor.",
                     legacyDevicePixelEnvVar, autoScreenEnvVar, screenFactorsEnvVar, scaleFactorEnvVar);

            int dpr = qEnvironmentVariableIntValue(legacyDevicePixelEnvVar);
            if (dpr > 0)
                result = dpr;
        }
    }
    return result;
}

/*!
    \class QHighDpiScaling
    \since 5.6
    \internal
    \preliminary
    \ingroup qpa

    \brief Collection of utility functions for UI scaling.

    QHighDpiScaling implements utility functions for high-dpi scaling for use
    on operating systems that provide limited support for native scaling. In
    addition this functionality can be used for simulation and testing purposes.

    The functions support scaling between the device independent coordinate
    system used by Qt applications and the native coordinate system used by
    the platform plugins. Intended usage locations are the low level / platform
    plugin interfacing parts of QtGui, for example the QWindow, QScreen and
    QWindowSystemInterface implementation.

    There are now up to three active coordinate systems in Qt:

     ---------------------------------------------------
    |  Application            Device Independent Pixels |   devicePixelRatio
    |  Qt Widgets                                       |         =
    |  Qt Gui                                           |
    |---------------------------------------------------|   Qt Scale Factor
    |  Qt Gui QPlatform*      Native Pixels             |         *
    |  Qt platform plugin                               |
    |---------------------------------------------------|   OS Scale Factor
    |  Display                Device Pixels             |
    |  (Graphics Buffers)                               |
    -----------------------------------------------------

    This is an simplification and shows the main coordinate system. All layers
    may work with device pixels in specific cases: OpenGL, creating the backing
    store, and QPixmap management. The "Native Pixels" coordinate system is
    internal to Qt and should not be exposed to Qt users: Seen from the outside
    there are only two coordinate systems: device independent pixels and device
    pixels.

    The devicePixelRatio seen by applications is the product of the Qt scale
    factor and the OS scale factor. The value of the scale factors may be 1,
    in which case two or more of the coordinate systems are equivalent. Platforms
    that (may) have an OS scale factor include \macos, iOS and Wayland.

    Note that the functions in this file do not work with the OS scale factor
    directly and are limited to converting between device independent and native
    pixels. The OS scale factor is accunted for by QWindow::devicePixelRatio()
    and similar functions.

    Configuration Examples:

    'Classic': Device Independent Pixels = Native Pixels = Device Pixels
     ---------------------------------------------------    devicePixelRatio: 1
    |  Application / Qt Gui             100 x 100       |
    |                                                   |   Qt Scale Factor: 1
    |  Qt Platform / OS                 100 x 100       |
    |                                                   |   OS Scale Factor: 1
    |  Display                          100 x 100       |
    -----------------------------------------------------

    'Retina Device': Device Independent Pixels = Native Pixels
     ---------------------------------------------------    devicePixelRatio: 2
    |  Application / Qt Gui             100 x 100       |
    |                                                   |   Qt Scale Factor: 1
    |  Qt Platform / OS                 100 x 100       |
    |---------------------------------------------------|   OS Scale Factor: 2
    |  Display                          200 x 200       |
    -----------------------------------------------------

    '2x Qt Scaling': Native Pixels = Device Pixels
     ---------------------------------------------------    devicePixelRatio: 2
    |  Application / Qt Gui             100 x 100       |
    |---------------------------------------------------|   Qt Scale Factor: 2
    |  Qt Platform / OS                 200 x 200       |
    |                                                   |   OS Scale Factor: 1
    |  Display                          200 x 200       |
    -----------------------------------------------------

    The Qt Scale Factor is the product of two sub-scale factors, which
    are independently either set or determined by the platform plugin.
    Several APIs are offered for this, targeting both developers and
    end users. All scale factors are of type qreal.

    1) A global scale factor
        The QT_SCALE_FACTOR environment variable can be used to set
        a global scale factor for all windows in the processs. This
        is useful for testing and debugging (you can simulate any
        devicePixelRatio without needing access to special hardware),
        and perhaps also for targeting a specific application to
        a specific display type (embedded use cases).

    2) Per-screen scale factors
        Some platform plugins support providing a per-screen scale
        factor based on display density information. These platforms
        include X11, Windows, and Android.

        There are two APIs for enabling or disabling this behavior:
            - The QT_AUTO_SCREEN_SCALE_FACTOR environment variable.
            - The AA_EnableHighDpiScaling and AA_DisableHighDpiScaling
              application attributes

        Enabling either will make QHighDpiScaling call QPlatformScreen::pixelDensity()
        and use the value provided as the scale factor for the screen in
        question. Disabling is done on a 'veto' basis where either the
        environment or the application can disable the scaling. The intended use
        cases are 'My system is not providing correct display density
        information' and 'My application needs to work in display pixels',
        respectively.

        The QT_SCREEN_SCALE_FACTORS environment variable can be used to set the screen
        scale factors manually. Set this to a semicolon-separated
        list of scale factors (matching the order of QGuiApplications::screens()),
        or to a list of name=value pairs (where name matches QScreen::name()).

    Coordinate conversion functions must be used when writing code that passes
    geometry across the Qt Gui / Platform plugin boundary. The main conversion
    functions are:
        T toNativePixels(T, QWindow *)
        T fromNativePixels(T, QWindow*)

    The following classes in QtGui use native pixels, for the convenience of the
    plataform plugins:
        QPlatformWindow
        QPlatformScreen
        QWindowSystemInterface (API only - Events are in device independent pixels)

    As a special consideration platform plugin code should be careful about
    calling QtGui geometry accessor functions:
        QRect r = window->geometry();
    Here the returned geometry is in device independent pixels. Add a conversion call:
        QRect r = QHighDpi::toNativePixels(window->geometry());
    (Avoiding calling QWindow and instead using the QPlatformWindow geometry
     might be a better course of action in this case.)
*/

qreal QHighDpiScaling::m_factor = 1.0;
bool QHighDpiScaling::m_active = false; //"overall active" - is there any scale factor set.
bool QHighDpiScaling::m_usePixelDensity = false; // use scale factor from platform plugin
bool QHighDpiScaling::m_pixelDensityScalingActive = false; // pixel density scale factor > 1
bool QHighDpiScaling::m_globalScalingActive = false; // global scale factor is active
bool QHighDpiScaling::m_screenFactorSet = false; // QHighDpiScaling::setScreenFactor has been used
QDpi QHighDpiScaling::m_logicalDpi = QDpi(-1,-1); // The scaled logical DPI of the primary screen

/*
    Initializes the QHighDpiScaling global variables. Called before the
    platform plugin is created.
*/

static inline bool usePixelDensity()
{
    // Determine if we should set a scale factor based on the pixel density
    // reported by the platform plugin. There are several enablers and several
    // disablers. A single disable may veto all other enablers.
    if (QCoreApplication::testAttribute(Qt::AA_DisableHighDpiScaling))
        return false;
    bool screenEnvValueOk;
    const int screenEnvValue = qEnvironmentVariableIntValue(autoScreenEnvVar, &screenEnvValueOk);
    if (screenEnvValueOk && screenEnvValue < 1)
        return false;
    return QCoreApplication::testAttribute(Qt::AA_EnableHighDpiScaling)
        || (screenEnvValueOk && screenEnvValue > 0)
        || (qEnvironmentVariableIsSet(legacyDevicePixelEnvVar) && qgetenv(legacyDevicePixelEnvVar).toLower() == "auto");
}

void QHighDpiScaling::initHighDpiScaling()
{
    // Determine if there is a global scale factor set.
    m_factor = initialGlobalScaleFactor();
    m_globalScalingActive = !qFuzzyCompare(m_factor, qreal(1));

    m_usePixelDensity = usePixelDensity();

    m_pixelDensityScalingActive = false; //set in updateHighDpiScaling below

    // we update m_active in updateHighDpiScaling, but while we create the
    // screens, we have to assume that m_usePixelDensity implies scaling
    m_active = m_globalScalingActive || m_usePixelDensity;
}

void QHighDpiScaling::updateHighDpiScaling()
{
    if (QCoreApplication::testAttribute(Qt::AA_DisableHighDpiScaling))
        return;

    if (m_usePixelDensity && !m_pixelDensityScalingActive) {
        const auto screens = QGuiApplication::screens();
        for (QScreen *screen : screens) {
            if (!qFuzzyCompare(screenSubfactor(screen->handle()), qreal(1))) {
                m_pixelDensityScalingActive = true;
                break;
            }
        }
    }
    if (qEnvironmentVariableIsSet(screenFactorsEnvVar)) {
        int i = 0;
        const auto specs = qgetenv(screenFactorsEnvVar).split(';');
        for (const QByteArray &spec : specs) {
            QScreen *screen = 0;
            int equalsPos = spec.lastIndexOf('=');
            double factor = 0;
            if (equalsPos > 0) {
                // support "name=factor"
                QByteArray name = spec.mid(0, equalsPos);
                QByteArray f = spec.mid(equalsPos + 1);
                bool ok;
                factor = f.toDouble(&ok);
                if (ok) {
                    const auto screens = QGuiApplication::screens();
                    for (QScreen *s : screens) {
                        if (s->name() == QString::fromLocal8Bit(name)) {
                            screen = s;
                            break;
                        }
                    }
                }
            } else {
                // listing screens in order
                bool ok;
                factor = spec.toDouble(&ok);
                if (ok && i < QGuiApplication::screens().count())
                    screen = QGuiApplication::screens().at(i);
            }
            if (screen)
                setScreenFactor(screen, factor);
            ++i;
        }
    }
    m_active = m_globalScalingActive || m_screenFactorSet || m_pixelDensityScalingActive;

    QPlatformScreen *primaryScreen = QGuiApplication::primaryScreen()->handle();
    qreal sf = screenSubfactor(primaryScreen);
    QDpi primaryDpi = primaryScreen->logicalDpi();
    m_logicalDpi = QDpi(primaryDpi.first / sf, primaryDpi.second / sf);
}

/*
    Sets the global scale factor which is applied to all windows.
*/
void QHighDpiScaling::setGlobalFactor(qreal factor)
{
    if (qFuzzyCompare(factor, m_factor))
        return;
    if (!QGuiApplication::allWindows().isEmpty())
        qWarning("QHighDpiScaling::setFactor: Should only be called when no windows exist.");

    m_globalScalingActive = !qFuzzyCompare(factor, qreal(1));
    m_factor = m_globalScalingActive ? factor : qreal(1);
    m_active = m_globalScalingActive || m_screenFactorSet || m_pixelDensityScalingActive;
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens)
         screen->d_func()->updateHighDpi();
}

static const char scaleFactorProperty[] = "_q_scaleFactor";

/*
    Sets a per-screen scale factor.
*/
void QHighDpiScaling::setScreenFactor(QScreen *screen, qreal factor)
{
    if (!qFuzzyCompare(factor, qreal(1))) {
        m_screenFactorSet = true;
        m_active = true;
    }
    screen->setProperty(scaleFactorProperty, QVariant(factor));

    // hack to force re-evaluation of screen geometry
    if (screen->handle())
        screen->d_func()->setPlatformScreen(screen->handle()); // updates geometries based on scale factor
}

QPoint QHighDpiScaling::mapPositionToNative(const QPoint &pos, const QPlatformScreen *platformScreen)
{
    if (!platformScreen)
        return pos;
    const qreal scaleFactor = factor(platformScreen);
    const QPoint topLeft = platformScreen->geometry().topLeft();
    return (pos - topLeft) * scaleFactor + topLeft;
}

QPoint QHighDpiScaling::mapPositionFromNative(const QPoint &pos, const QPlatformScreen *platformScreen)
{
    if (!platformScreen)
        return pos;
    const qreal scaleFactor = factor(platformScreen);
    const QPoint topLeft = platformScreen->geometry().topLeft();
    return (pos - topLeft) / scaleFactor + topLeft;
}

qreal QHighDpiScaling::screenSubfactor(const QPlatformScreen *screen)
{
    qreal factor = qreal(1.0);
    if (screen) {
        if (m_usePixelDensity)
            factor *= screen->pixelDensity();
        if (m_screenFactorSet) {
            QVariant screenFactor = screen->screen()->property(scaleFactorProperty);
            if (screenFactor.isValid())
                factor *= screenFactor.toReal();
        }
    }
    return factor;
}

QDpi QHighDpiScaling::logicalDpi()
{
    return m_logicalDpi;
}

qreal QHighDpiScaling::factor(const QScreen *screen)
{
    // Fast path for when scaling in Qt is not used at all.
    if (!m_active)
        return qreal(1.0);

    // The effective factor for a given screen is the product of the
    // screen and global sub-factors
    qreal factor = m_factor;
    if (screen)
        factor *= screenSubfactor(screen->handle());
    return factor;
}

qreal QHighDpiScaling::factor(const QPlatformScreen *platformScreen)
{
    if (!m_active)
        return qreal(1.0);

    return m_factor * screenSubfactor(platformScreen);
}

qreal QHighDpiScaling::factor(const QWindow *window)
{
    if (!m_active)
        return qreal(1.0);

    return factor(window ? window->screen() : QGuiApplication::primaryScreen());
}

QPoint QHighDpiScaling::origin(const QScreen *screen)
{
    return screen->geometry().topLeft();
}

QPoint QHighDpiScaling::origin(const QPlatformScreen *platformScreen)
{
    return platformScreen->geometry().topLeft();
}

#endif //QT_NO_HIGHDPISCALING
QT_END_NAMESPACE
