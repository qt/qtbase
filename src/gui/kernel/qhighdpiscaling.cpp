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
#include "qplatformwindow.h"
#include "private/qscreen_p.h"
#include <private/qguiapplication_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qmetaobject.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcScaling, "qt.scaling");

#ifndef QT_NO_HIGHDPISCALING
static const char legacyDevicePixelEnvVar[] = "QT_DEVICE_PIXEL_RATIO";

// Note: QT_AUTO_SCREEN_SCALE_FACTOR is Done on X11, and should be kept
// working as-is. It's Deprecated on all other platforms.
static const char legacyAutoScreenEnvVar[] = "QT_AUTO_SCREEN_SCALE_FACTOR";

static const char enableHighDpiScalingEnvVar[] = "QT_ENABLE_HIGHDPI_SCALING";
static const char scaleFactorEnvVar[] = "QT_SCALE_FACTOR";
static const char screenFactorsEnvVar[] = "QT_SCREEN_SCALE_FACTORS";
static const char scaleFactorRoundingPolicyEnvVar[] = "QT_SCALE_FACTOR_ROUNDING_POLICY";
static const char dpiAdjustmentPolicyEnvVar[] = "QT_DPI_ADJUSTMENT_POLICY";
static const char usePhysicalDpiEnvVar[] = "QT_USE_PHYSICAL_DPI";

// Per-screen scale factors for named screens set with QT_SCREEN_SCALE_FACTORS
// are stored here. Use a global hash to keep the factor across screen
// disconnect/connect cycles where the screen object may be deleted.
typedef QHash<QString, qreal> QScreenScaleFactorHash;
Q_GLOBAL_STATIC(QScreenScaleFactorHash, qNamedScreenScaleFactors);

// Reads and interprets the given environment variable as a bool,
// returns the default value if not set.
static bool qEnvironmentVariableAsBool(const char *name, bool defaultValue)
{
    bool ok = false;
    int value = qEnvironmentVariableIntValue(name, &ok);
    return ok ? value > 0 : defaultValue;
}

static inline qreal initialGlobalScaleFactor()
{

    qreal result = 1;
    if (qEnvironmentVariableIsSet(scaleFactorEnvVar)) {
        bool ok;
        const qreal f = qEnvironmentVariable(scaleFactorEnvVar).toDouble(&ok);
        if (ok && f > 0) {
            qCDebug(lcScaling) << "Apply " << scaleFactorEnvVar << f;
            result = f;
        }
    } else {
        // Check for deprecated environment variables.
        if (qEnvironmentVariableIsSet(legacyDevicePixelEnvVar)) {
            qWarning("Warning: %s is deprecated. Instead use:\n"
                     "   %s to enable platform plugin controlled per-screen factors.\n"
                     "   %s to set per-screen DPI.\n"
                     "   %s to set the application global scale factor.",
                     legacyDevicePixelEnvVar, legacyAutoScreenEnvVar, screenFactorsEnvVar, scaleFactorEnvVar);

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
    pixels. The OS scale factor is accounted for by QWindow::devicePixelRatio()
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
        a global scale factor for all windows in the process. This
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
    platform plugins:
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

/*
    Initializes the QHighDpiScaling global variables. Called before the
    platform plugin is created.
*/

static inline bool usePixelDensity()
{
    // Determine if we should set a scale factor based on the pixel density
    // reported by the platform plugin. There are several enablers and several
    // disablers. A single disable may veto all other enablers.

    // First, check of there is an explicit disable.
    if (QCoreApplication::testAttribute(Qt::AA_DisableHighDpiScaling))
        return false;
    bool screenEnvValueOk;
    const int screenEnvValue = qEnvironmentVariableIntValue(legacyAutoScreenEnvVar, &screenEnvValueOk);
    if (screenEnvValueOk && screenEnvValue < 1)
        return false;
    bool enableEnvValueOk;
    const int enableEnvValue = qEnvironmentVariableIntValue(enableHighDpiScalingEnvVar, &enableEnvValueOk);
    if (enableEnvValueOk && enableEnvValue < 1)
        return false;

    // Then return if there was an enable.
    return QCoreApplication::testAttribute(Qt::AA_EnableHighDpiScaling)
        || (screenEnvValueOk && screenEnvValue > 0)
        || (enableEnvValueOk && enableEnvValue > 0)
        || (qEnvironmentVariableIsSet(legacyDevicePixelEnvVar)
            && qEnvironmentVariable(legacyDevicePixelEnvVar).compare(QLatin1String("auto"), Qt::CaseInsensitive) == 0);
}

qreal QHighDpiScaling::rawScaleFactor(const QPlatformScreen *screen)
{
    // Determine if physical DPI should be used
    static const bool usePhysicalDpi = qEnvironmentVariableAsBool(usePhysicalDpiEnvVar, false);

    // Calculate scale factor beased on platform screen DPI values
    qreal factor;
    QDpi platformBaseDpi = screen->logicalBaseDpi();
    if (usePhysicalDpi) {
        QSize sz = screen->geometry().size();
        QSizeF psz = screen->physicalSize();
        qreal platformPhysicalDpi = ((sz.height() / psz.height()) + (sz.width() / psz.width())) * qreal(25.4 * 0.5);
        factor = qreal(platformPhysicalDpi) / qreal(platformBaseDpi.first);
    } else {
        const QDpi platformLogicalDpi = QPlatformScreen::overrideDpi(screen->logicalDpi());
        factor = qreal(platformLogicalDpi.first) / qreal(platformBaseDpi.first);
    }

    return factor;
}

template <class EnumType>
struct EnumLookup
{
    const char *name;
    EnumType value;
};

template <class EnumType>
static bool operator==(const EnumLookup<EnumType> &e1, const EnumLookup<EnumType> &e2)
{
    return qstricmp(e1.name, e2.name) == 0;
}

template <class EnumType>
static QByteArray joinEnumValues(const EnumLookup<EnumType> *i1, const EnumLookup<EnumType> *i2)
{
    QByteArray result;
    for (; i1 < i2; ++i1) {
        if (!result.isEmpty())
            result += QByteArrayLiteral(", ");
        result += i1->name;
    }
    return result;
}

using ScaleFactorRoundingPolicyLookup = EnumLookup<Qt::HighDpiScaleFactorRoundingPolicy>;

static const ScaleFactorRoundingPolicyLookup scaleFactorRoundingPolicyLookup[] =
{
    {"Round", Qt::HighDpiScaleFactorRoundingPolicy::Round},
    {"Ceil", Qt::HighDpiScaleFactorRoundingPolicy::Ceil},
    {"Floor", Qt::HighDpiScaleFactorRoundingPolicy::Floor},
    {"RoundPreferFloor", Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor},
    {"PassThrough", Qt::HighDpiScaleFactorRoundingPolicy::PassThrough}
};

static Qt::HighDpiScaleFactorRoundingPolicy
    lookupScaleFactorRoundingPolicy(const QByteArray &v)
{
    auto end = std::end(scaleFactorRoundingPolicyLookup);
    auto it = std::find(std::begin(scaleFactorRoundingPolicyLookup), end,
                        ScaleFactorRoundingPolicyLookup{v.constData(), Qt::HighDpiScaleFactorRoundingPolicy::Unset});
    return it != end ? it->value : Qt::HighDpiScaleFactorRoundingPolicy::Unset;
}

using DpiAdjustmentPolicyLookup = EnumLookup<QHighDpiScaling::DpiAdjustmentPolicy>;

static const DpiAdjustmentPolicyLookup dpiAdjustmentPolicyLookup[] =
{
    {"AdjustDpi", QHighDpiScaling::DpiAdjustmentPolicy::Enabled},
    {"DontAdjustDpi", QHighDpiScaling::DpiAdjustmentPolicy::Disabled},
    {"AdjustUpOnly", QHighDpiScaling::DpiAdjustmentPolicy::UpOnly}
};

static QHighDpiScaling::DpiAdjustmentPolicy
    lookupDpiAdjustmentPolicy(const QByteArray &v)
{
    auto end = std::end(dpiAdjustmentPolicyLookup);
    auto it = std::find(std::begin(dpiAdjustmentPolicyLookup), end,
                        DpiAdjustmentPolicyLookup{v.constData(), QHighDpiScaling::DpiAdjustmentPolicy::Unset});
    return it != end ? it->value : QHighDpiScaling::DpiAdjustmentPolicy::Unset;
}

qreal QHighDpiScaling::roundScaleFactor(qreal rawFactor)
{
    // Apply scale factor rounding policy. Using mathematically correct rounding
    // may not give the most desirable visual results, especially for
    // critical fractions like .5. In general, rounding down results in visual
    // sizes that are smaller than the ideal size, and opposite for rounding up.
    // Rounding down is then preferable since "small UI" is a more acceptable
    // high-DPI experience than "large UI".
    static auto scaleFactorRoundingPolicy = Qt::HighDpiScaleFactorRoundingPolicy::Unset;

    // Determine rounding policy
    if (scaleFactorRoundingPolicy == Qt::HighDpiScaleFactorRoundingPolicy::Unset) {
        // Check environment
        if (qEnvironmentVariableIsSet(scaleFactorRoundingPolicyEnvVar)) {
            QByteArray policyText = qgetenv(scaleFactorRoundingPolicyEnvVar);
            auto policyEnumValue = lookupScaleFactorRoundingPolicy(policyText);
            if (policyEnumValue != Qt::HighDpiScaleFactorRoundingPolicy::Unset) {
                scaleFactorRoundingPolicy = policyEnumValue;
            } else {
                auto values = joinEnumValues(std::begin(scaleFactorRoundingPolicyLookup),
                                             std::end(scaleFactorRoundingPolicyLookup));
                qWarning("Unknown scale factor rounding policy: %s. Supported values are: %s.",
                         policyText.constData(), values.constData());
            }
        }

        // Check application object if no environment value was set.
        if (scaleFactorRoundingPolicy == Qt::HighDpiScaleFactorRoundingPolicy::Unset) {
            scaleFactorRoundingPolicy = QGuiApplication::highDpiScaleFactorRoundingPolicy();
        } else {
            // Make application setting reflect environment
            QGuiApplication::setHighDpiScaleFactorRoundingPolicy(scaleFactorRoundingPolicy);
        }
    }

    // Apply rounding policy.
    qreal roundedFactor = rawFactor;
    switch (scaleFactorRoundingPolicy) {
    case Qt::HighDpiScaleFactorRoundingPolicy::Round:
        roundedFactor = qRound(rawFactor);
        break;
    case Qt::HighDpiScaleFactorRoundingPolicy::Ceil:
        roundedFactor = qCeil(rawFactor);
        break;
    case Qt::HighDpiScaleFactorRoundingPolicy::Floor:
        roundedFactor = qFloor(rawFactor);
        break;
    case Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor:
        // Round up for .75 and higher. This favors "small UI" over "large UI".
        roundedFactor = rawFactor - qFloor(rawFactor) < 0.75
            ? qFloor(rawFactor) : qCeil(rawFactor);
        break;
    case Qt::HighDpiScaleFactorRoundingPolicy::PassThrough:
    case Qt::HighDpiScaleFactorRoundingPolicy::Unset:
        break;
    }

    // Don't round down to to zero; clamp the minimum (rounded) factor to 1.
    // This is not a common case but can happen if a display reports a very
    // low DPI.
    if (scaleFactorRoundingPolicy != Qt::HighDpiScaleFactorRoundingPolicy::PassThrough)
        roundedFactor = qMax(roundedFactor, qreal(1));

    return roundedFactor;
}

QDpi QHighDpiScaling::effectiveLogicalDpi(const QPlatformScreen *screen, qreal rawFactor, qreal roundedFactor)
{
    // Apply DPI adjustment policy, if needed. If enabled this will change the
    // reported logical DPI to account for the difference between the rounded
    // scale factor and the actual scale factor. The effect is that text size
    // will be correct for the screen dpi, but may be (slightly) out of sync
    // with the rest of the UI. The amount of out-of-synch-ness depends on how
    // well user code handles a non-standard DPI values, but since the
    // adjustment is small (typically +/- 48 max) this might be OK.
    static auto dpiAdjustmentPolicy = DpiAdjustmentPolicy::Unset;

    // Determine adjustment policy.
    if (dpiAdjustmentPolicy == DpiAdjustmentPolicy::Unset) {
        if (qEnvironmentVariableIsSet(dpiAdjustmentPolicyEnvVar)) {
            QByteArray policyText = qgetenv(dpiAdjustmentPolicyEnvVar);
            auto policyEnumValue = lookupDpiAdjustmentPolicy(policyText);
            if (policyEnumValue != DpiAdjustmentPolicy::Unset) {
                dpiAdjustmentPolicy = policyEnumValue;
            } else {
                auto values = joinEnumValues(std::begin(dpiAdjustmentPolicyLookup),
                                             std::end(dpiAdjustmentPolicyLookup));
                qWarning("Unknown DPI adjustment policy: %s. Supported values are: %s.",
                         policyText.constData(), values.constData());
            }
        }
        if (dpiAdjustmentPolicy == DpiAdjustmentPolicy::Unset)
            dpiAdjustmentPolicy = DpiAdjustmentPolicy::UpOnly;
    }

    // Apply adjustment policy.
    const QDpi baseDpi = screen->logicalBaseDpi();
    const qreal dpiAdjustmentFactor = rawFactor / roundedFactor;

    // Return the base DPI for cases where there is no adjustment
    if (dpiAdjustmentPolicy == DpiAdjustmentPolicy::Disabled)
        return baseDpi;
    if (dpiAdjustmentPolicy == DpiAdjustmentPolicy::UpOnly && dpiAdjustmentFactor < 1)
        return baseDpi;

    return QDpi(baseDpi.first * dpiAdjustmentFactor, baseDpi.second * dpiAdjustmentFactor);
}

void QHighDpiScaling::initHighDpiScaling()
{
    // Determine if there is a global scale factor set.
    m_factor = initialGlobalScaleFactor();
    m_globalScalingActive = !qFuzzyCompare(m_factor, qreal(1));

    m_usePixelDensity = usePixelDensity();

    m_pixelDensityScalingActive = false; //set in updateHighDpiScaling below

    m_active = m_globalScalingActive || m_usePixelDensity;
}

void QHighDpiScaling::updateHighDpiScaling()
{
    if (QCoreApplication::testAttribute(Qt::AA_DisableHighDpiScaling))
        return;

    m_usePixelDensity = usePixelDensity();

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
        const QString spec = qEnvironmentVariable(screenFactorsEnvVar);
        const auto specs = spec.splitRef(QLatin1Char(';'));
        for (const QStringRef &spec : specs) {
            int equalsPos = spec.lastIndexOf(QLatin1Char('='));
            qreal factor = 0;
            if (equalsPos > 0) {
                // support "name=factor"
                bool ok;
                const auto name = spec.left(equalsPos);
                factor = spec.mid(equalsPos + 1).toDouble(&ok);
                if (ok && factor > 0 ) {
                    const auto screens = QGuiApplication::screens();
                    for (QScreen *s : screens) {
                        if (s->name() == name) {
                            setScreenFactor(s, factor);
                            break;
                        }
                    }
                }
            } else {
                // listing screens in order
                bool ok;
                factor = spec.toDouble(&ok);
                if (ok && factor > 0 && i < QGuiApplication::screens().count()) {
                    QScreen *screen = QGuiApplication::screens().at(i);
                    setScreenFactor(screen, factor);
                }
            }
            ++i;
        }
    }
    m_active = m_globalScalingActive || m_screenFactorSet || m_pixelDensityScalingActive;
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

    // Prefer associating the factor with screen name over the object
    // since the screen object may be deleted on screen disconnects.
    const QString name = screen->name();
    if (name.isEmpty())
        screen->setProperty(scaleFactorProperty, QVariant(factor));
    else
        qNamedScreenScaleFactors()->insert(name, factor);

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

QPoint QHighDpiScaling::mapPositionToGlobal(const QPoint &pos, const QPoint &windowGlobalPosition, const QWindow *window)
{
    QPoint globalPosCandidate = pos + windowGlobalPosition;
    if (QGuiApplicationPrivate::screen_list.size() <= 1)
        return globalPosCandidate;

    // The global position may be outside device independent screen geometry
    // in cases where a window spans screens. Detect this case and map via
    // native coordinates to the correct screen.
    auto currentScreen = window->screen();
    if (currentScreen && !currentScreen->geometry().contains(globalPosCandidate)) {
        auto nativeGlobalPos = QHighDpi::toNativePixels(globalPosCandidate, currentScreen);
        if (auto actualPlatformScreen = currentScreen->handle()->screenForPosition(nativeGlobalPos))
            return QHighDpi::fromNativePixels(nativeGlobalPos, actualPlatformScreen->screen());
    }

    return globalPosCandidate;
}

QPoint QHighDpiScaling::mapPositionFromGlobal(const QPoint &pos, const QPoint &windowGlobalPosition, const QWindow *window)
{
    QPoint windowPosCandidate = pos - windowGlobalPosition;
    if (QGuiApplicationPrivate::screen_list.size() <= 1 || window->handle() == nullptr)
        return windowPosCandidate;

    // Device independent global (screen) space may discontiguous when high-dpi scaling
    // is active. This means that the normal subtracting of the window global position from the
    // position-to-be-mapped may not work in cases where a window spans multiple screens.
    // Map both positions to native global space (using the correct screens), subtract there,
    // and then map the difference back using the scale factor for the window.
    QScreen *posScreen = QGuiApplication::screenAt(pos);
    if (posScreen && posScreen != window->screen()) {
        QPoint nativePos = QHighDpi::toNativePixels(pos, posScreen);
        QPoint windowNativePos = window->handle()->geometry().topLeft();
        return QHighDpi::fromNativeLocalPosition(nativePos - windowNativePos, window);
    }

    return windowPosCandidate;
}

qreal QHighDpiScaling::screenSubfactor(const QPlatformScreen *screen)
{
    auto factor = qreal(1.0);
    if (!screen)
        return factor;

    // Unlike the other code where factors are combined by multiplication,
    // factors from QT_SCREEN_SCALE_FACTORS takes precedence over the factor
    // computed from platform plugin DPI. The rationale is that the user is
    // setting the factor to override erroneous DPI values.
    bool screenPropertyUsed = false;
    if (m_screenFactorSet) {
        // Check if there is a factor set on the screen object or associated
        // with the screen name. These are mutually exclusive, so checking
        // order is not significant.
        if (auto qScreen = screen->screen()) {
            auto screenFactor = qScreen->property(scaleFactorProperty).toReal(&screenPropertyUsed);
            if (screenPropertyUsed)
                factor = screenFactor;
        }

        if (!screenPropertyUsed) {
            auto byNameIt = qNamedScreenScaleFactors()->constFind(screen->name());
            if ((screenPropertyUsed = byNameIt != qNamedScreenScaleFactors()->cend()))
                factor = *byNameIt;
        }
    }

    if (!screenPropertyUsed && m_usePixelDensity)
        factor = roundScaleFactor(rawScaleFactor(screen));

    return factor;
}

QDpi QHighDpiScaling::logicalDpi(const QScreen *screen)
{
    // (Note: m_active test is performed at call site.)
    if (!screen || !screen->handle())
        return QDpi(96, 96);

    if (!m_usePixelDensity) {
        const qreal screenScaleFactor = screenSubfactor(screen->handle());
        const QDpi dpi = QPlatformScreen::overrideDpi(screen->handle()->logicalDpi());
        return QDpi{ dpi.first / screenScaleFactor, dpi.second / screenScaleFactor };
    }

    const qreal scaleFactor = rawScaleFactor(screen->handle());
    const qreal roundedScaleFactor = roundScaleFactor(scaleFactor);
    return effectiveLogicalDpi(screen->handle(), scaleFactor, roundedScaleFactor);
}

QHighDpiScaling::ScaleAndOrigin QHighDpiScaling::scaleAndOrigin(const QPlatformScreen *platformScreen, QPoint *nativePosition)
{
    if (!m_active)
        return { qreal(1), QPoint() };
    if (!platformScreen)
        return { m_factor, QPoint() }; // the global factor
    const QPlatformScreen *actualScreen = nativePosition ?
        platformScreen->screenForPosition(*nativePosition) : platformScreen;
    return { m_factor * screenSubfactor(actualScreen), actualScreen->geometry().topLeft() };
}

QHighDpiScaling::ScaleAndOrigin QHighDpiScaling::scaleAndOrigin(const QScreen *screen, QPoint *nativePosition)
{
    if (!m_active)
        return { qreal(1), QPoint() };
    if (!screen)
        return { m_factor, QPoint() }; // the global factor
    return scaleAndOrigin(screen->handle(), nativePosition);
}

QHighDpiScaling::ScaleAndOrigin QHighDpiScaling::scaleAndOrigin(const QWindow *window, QPoint *nativePosition)
{
    if (!m_active)
        return { qreal(1), QPoint() };

    QScreen *screen = window ? window->screen() : QGuiApplication::primaryScreen();
    const bool searchScreen = !window || window->isTopLevel();
    return scaleAndOrigin(screen, searchScreen ? nativePosition : nullptr);
}

#endif //QT_NO_HIGHDPISCALING
QT_END_NAMESPACE
