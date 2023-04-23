// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <optional>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcHighDpi, "qt.highdpi");

#ifndef QT_NO_HIGHDPISCALING

static const char enableHighDpiScalingEnvVar[] = "QT_ENABLE_HIGHDPI_SCALING";
static const char scaleFactorEnvVar[] = "QT_SCALE_FACTOR";
static const char screenFactorsEnvVar[] = "QT_SCREEN_SCALE_FACTORS";
static const char scaleFactorRoundingPolicyEnvVar[] = "QT_SCALE_FACTOR_ROUNDING_POLICY";
static const char dpiAdjustmentPolicyEnvVar[] = "QT_DPI_ADJUSTMENT_POLICY";
static const char usePhysicalDpiEnvVar[] = "QT_USE_PHYSICAL_DPI";

static std::optional<QString> qEnvironmentVariableOptionalString(const char *name)
{
    if (!qEnvironmentVariableIsSet(name))
        return std::nullopt;

    return std::optional(qEnvironmentVariable(name));
}

static std::optional<QByteArray> qEnvironmentVariableOptionalByteArray(const char *name)
{
    if (!qEnvironmentVariableIsSet(name))
        return std::nullopt;

    return std::optional(qgetenv(name));
}

static std::optional<int> qEnvironmentVariableOptionalInt(const char *name)
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    auto opt = ok ? std::optional(value) : std::nullopt;
    return opt;
}

static std::optional<qreal> qEnvironmentVariableOptionalReal(const char *name)
{
    if (!qEnvironmentVariableIsSet(name))
        return std::nullopt;

    bool ok = false;
    const qreal value = qEnvironmentVariable(name).toDouble(&ok);
    return ok ? std::optional(value) : std::nullopt;
}

/*!
    \class QHighDpiScaling
    \since 5.6
    \internal
    \preliminary
    \ingroup qpa

    \brief Collection of utility functions for UI scaling.

    QHighDpiScaling implements utility functions for high-dpi scaling for use
    on operating systems that provide limited support for native scaling, such
    as Windows, X11, and Android. In addition this functionality can be used
    for simulation and testing purposes.

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
    factor and the OS scale factor (see QWindow::devicePixelRatio()). The value
    of the scale factors may be 1, in which case two or more of the coordinate
    systems are equivalent. Platforms that (may) have an OS scale factor include
    macOS, iOS, Wayland, and Web(Assembly).

    Note that the API implemented in this file do use the OS scale factor, and
    is used for converting between device independent and native pixels only.

    Configuration Examples:

    'Classic': Device Independent Pixels = Native Pixels = Device Pixels
     ---------------------------------------------------    devicePixelRatio: 1
    |  Application / Qt Gui             100 x 100       |
    |                                                   |   Qt Scale Factor: 1
    |  Qt Platform / OS                 100 x 100       |
    |                                                   |   OS Scale Factor: 1
    |  Display                          100 x 100       |
    -----------------------------------------------------

    '2x Apple Device': Device Independent Pixels = Native Pixels
     ---------------------------------------------------    devicePixelRatio: 2
    |  Application / Qt Gui             100 x 100       |
    |                                                   |   Qt Scale Factor: 1
    |  Qt Platform / OS                 100 x 100       |
    |---------------------------------------------------|   OS Scale Factor: 2
    |  Display                          200 x 200       |
    -----------------------------------------------------

    'Windows at 200%': Native Pixels = Device Pixels
     ---------------------------------------------------    devicePixelRatio: 2
    |  Application / Qt Gui             100 x 100       |
    |---------------------------------------------------|   Qt Scale Factor: 2
    |  Qt Platform / OS                 200 x 200       |
    |                                                   |   OS Scale Factor: 1
    |  Display                          200 x 200       |
    -----------------------------------------------------

    * Configuration

    - Enabling: In Qt 6, high-dpi scaling (the functionality implemented in this file)
      is always enabled. The Qt scale factor value is typically determined by the
      QPlatformScreen implementation - see below.

      There is one environment variable based opt-out option: set QT_ENABLE_HIGHDPI_SCALING=0.
      Keep in mind that this does not affect the OS scale factor, which is controlled by
      the operating system.

    - Qt scale factor value: The Qt scale factor is the product of the screen scale
      factor and the global scale factor, which are independently either set or determined
      by the platform plugin. Several APIs are offered for this, targeting both developers
      and end users. All scale factors are of type qreal.

      1) Per-screen scale factors

        Per-screen scale factors are computed based on logical DPI provided by
        by the platform plugin.

        The platform plugin implements DPI accessor functions:
            QDpi QPlatformScreen::logicalDpi()
            QDpi QPlatformScreen::logicalBaseDpi()

        QHighDpiScaling then computes the per-screen scale factor as follows:

            factor = logicalDpi / logicalBaseDpi

        Alternatively, QT_SCREEN_SCALE_FACTORS can be used to set the screen
        scale factors.

      2) The global scale factor

        The QT_SCALE_FACTOR environment variable can be used to set a global scale
        factor which applies to all application windows. This allows developing and
        testing at any DPR, independently of available hardware and without changing
        global desktop settings.

    - Rounding

      Qt 6 does not round scale factors by default. Qt 5 rounds the screen scale factor
      to the nearest integer (except for Qt on Android which does not round).

      The rounding policy can be set by the application, or on the environment:

        Application (C++):    QGuiApplication::setHighDpiScaleFactorRoundingPolicy()
        User (environment):   QT_SCALE_FACTOR_ROUNDING_POLICY

      Note that the OS scale factor, and global scale factors set with QT_SCALE_FACTOR
      are never rounded by Qt.

    * C++ API Overview

    - Coordinate Conversion ("scaling")

      The QHighDpi namespace provides several functions for converting geometry
      between the device independent and native coordinate systems. These should
      be used when calling "QPlatform*" API from QtGui. Callers are responsible
      for selecting a function variant based on geometry type:

            Type                        From Native                              To Native
        local               :    QHighDpi::fromNativeLocalPosition()    QHighDpi::toNativeLocalPosition()
        global (screen)     :    QHighDpi::fromNativeGlobalPosition()   QHighDpi::toNativeGlobalPosition()
        QWindow::geometry() :    QHighDpi::fromNativeWindowGeometry()   QHighDpi::toNativeWindowGeometry()
        sizes, margins, etc :    QHighDpi::fromNativePixels()           QHighDpi::toNativePixels()

     The conversion functions take two arguments; the geometry and a context:

        QSize nativeSize = toNativePixels(deviceIndependentSize, window);

     The context is usually a QWindow instance, but can also be a QScreen instance,
     or the corresponding QPlatform classes.

    - Activation

      QHighDpiScaling::isActive() returns true iff
            Qt high-dpi scaling is enabled (e.g. with AA_EnableHighDpiScaling) AND
            there is a Qt scale factor != 1

      (the value of the OS scale factor does not affect this API)

    - Calling QtGui from the platform plugins

      Platform plugin code should be careful about calling QtGui geometry accessor
      functions like geometry():

         QRect r = window->geometry();

      In this case the returned geometry is in the wrong coordinate system (device independent
      instead of native pixels). Fix this by adding a conversion call:

         QRect r = QHighDpi::toNativeWindowGeometry(window->geometry());

      (Also consider if the call to QtGui is really needed - prefer calling QPlatform* API.)
*/

qreal QHighDpiScaling::m_factor = 1.0;
bool QHighDpiScaling::m_active = false; //"overall active" - is there any scale factor set.
bool QHighDpiScaling::m_usePlatformPluginDpi = false; // use scale factor based on platform plugin DPI
bool QHighDpiScaling::m_platformPluginDpiScalingActive  = false; // platform plugin DPI gives a scale factor > 1
bool QHighDpiScaling::m_globalScalingActive = false; // global scale factor is active
bool QHighDpiScaling::m_screenFactorSet = false; // QHighDpiScaling::setScreenFactor has been used
bool QHighDpiScaling::m_usePhysicalDpi = false;
QVector<QHighDpiScaling::ScreenFactor> QHighDpiScaling::m_screenFactors;
QHighDpiScaling::DpiAdjustmentPolicy QHighDpiScaling::m_dpiAdjustmentPolicy = QHighDpiScaling::DpiAdjustmentPolicy::Unset;
QHash<QString, qreal> QHighDpiScaling::m_namedScreenScaleFactors; // Per-screen scale factors (screen name -> factor)

qreal QHighDpiScaling::rawScaleFactor(const QPlatformScreen *screen)
{
    // Calculate scale factor beased on platform screen DPI values
    qreal factor;
    QDpi platformBaseDpi = screen->logicalBaseDpi();
    if (QHighDpiScaling::m_usePhysicalDpi) {
        QSize sz = screen->geometry().size();
        QSizeF psz = screen->physicalSize();
        qreal platformPhysicalDpi = ((sz.height() / psz.height()) + (sz.width() / psz.width())) * qreal(25.4 * 0.5);
        factor = qRound(platformPhysicalDpi) / qreal(platformBaseDpi.first);
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

    Qt::HighDpiScaleFactorRoundingPolicy scaleFactorRoundingPolicy =
        QGuiApplication::highDpiScaleFactorRoundingPolicy();

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

    // Clamp the minimum factor to 1. Qt does not currently render
    // correctly with factors less than 1.
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

    // Apply adjustment policy.
    const QDpi baseDpi = screen->logicalBaseDpi();
    const qreal dpiAdjustmentFactor = rawFactor / roundedFactor;

    // Return the base DPI for cases where there is no adjustment
    if (QHighDpiScaling::m_dpiAdjustmentPolicy == DpiAdjustmentPolicy::Disabled)
        return baseDpi;
    if (QHighDpiScaling::m_dpiAdjustmentPolicy == DpiAdjustmentPolicy::UpOnly && dpiAdjustmentFactor < 1)
        return baseDpi;

    return QDpi(baseDpi.first * dpiAdjustmentFactor, baseDpi.second * dpiAdjustmentFactor);
}

/*
    Determine and apply global/initial configuration which do not depend on
    having access to QScreen objects - this function is called before they
    have been created. Screen-dependent configuration happens later in
    updateHighDpiScaling().
*/
void QHighDpiScaling::initHighDpiScaling()
{
    qCDebug(lcHighDpi) << "Initializing high-DPI scaling";

    // Read environment variables
    static const char* envDebugStr = "environment variable set:";
    std::optional<int> envEnableHighDpiScaling = qEnvironmentVariableOptionalInt(enableHighDpiScalingEnvVar);
    if (envEnableHighDpiScaling.has_value())
        qCDebug(lcHighDpi) << envDebugStr << enableHighDpiScalingEnvVar << envEnableHighDpiScaling.value();

    std::optional<qreal> envScaleFactor = qEnvironmentVariableOptionalReal(scaleFactorEnvVar);
    if (envScaleFactor.has_value())
        qCDebug(lcHighDpi) << envDebugStr <<  scaleFactorEnvVar << envScaleFactor.value();

    std::optional<QString> envScreenFactors = qEnvironmentVariableOptionalString(screenFactorsEnvVar);
    if (envScreenFactors.has_value())
        qCDebug(lcHighDpi) << envDebugStr << screenFactorsEnvVar << envScreenFactors.value();

    std::optional<int> envUsePhysicalDpi = qEnvironmentVariableOptionalInt(usePhysicalDpiEnvVar);
    if (envUsePhysicalDpi.has_value())
        qCDebug(lcHighDpi) << envDebugStr << usePhysicalDpiEnvVar << envUsePhysicalDpi.value();

    std::optional<QByteArray> envScaleFactorRoundingPolicy = qEnvironmentVariableOptionalByteArray(scaleFactorRoundingPolicyEnvVar);
    if (envScaleFactorRoundingPolicy.has_value())
        qCDebug(lcHighDpi) << envDebugStr << scaleFactorRoundingPolicyEnvVar << envScaleFactorRoundingPolicy.value();

    std::optional<QByteArray> envDpiAdjustmentPolicy = qEnvironmentVariableOptionalByteArray(dpiAdjustmentPolicyEnvVar);
    if (envDpiAdjustmentPolicy.has_value())
        qCDebug(lcHighDpi) << envDebugStr << dpiAdjustmentPolicyEnvVar << envDpiAdjustmentPolicy.value();

    // High-dpi scaling is enabled by default; check for global disable.
    m_usePlatformPluginDpi = envEnableHighDpiScaling.value_or(1) > 0;
    m_platformPluginDpiScalingActive = false; // see updateHighDpiScaling()

    // Check for glabal scale factor (different from 1)
    m_factor = envScaleFactor.value_or(qreal(1));
    m_globalScalingActive = !qFuzzyCompare(m_factor, qreal(1));

    // Store the envScreenFactors string for later use. The string format
    // supports using screen names, which means that screen DPI cannot
    // be resolved at this point.
    QString screenFactorsSpec = envScreenFactors.value_or(QString());
    m_screenFactors = parseScreenScaleFactorsSpec(QStringView{screenFactorsSpec});
    m_namedScreenScaleFactors.clear();

    m_usePhysicalDpi = envUsePhysicalDpi.value_or(0) > 0;

    // Resolve HighDpiScaleFactorRoundingPolicy to QGuiApplication::highDpiScaleFactorRoundingPolicy
    if (envScaleFactorRoundingPolicy.has_value()) {
        QByteArray policyText = envScaleFactorRoundingPolicy.value();
        auto policyEnumValue = lookupScaleFactorRoundingPolicy(policyText);
        if (policyEnumValue != Qt::HighDpiScaleFactorRoundingPolicy::Unset) {
            QGuiApplication::setHighDpiScaleFactorRoundingPolicy(policyEnumValue);
        } else {
            auto values = joinEnumValues(std::begin(scaleFactorRoundingPolicyLookup),
                                         std::end(scaleFactorRoundingPolicyLookup));
            qWarning("Unknown scale factor rounding policy: %s. Supported values are: %s.",
                     policyText.constData(), values.constData());
        }
    }

    // Resolve DpiAdjustmentPolicy to m_dpiAdjustmentPolicy
    if (envDpiAdjustmentPolicy.has_value()) {
        QByteArray policyText = envDpiAdjustmentPolicy.value();
        auto policyEnumValue = lookupDpiAdjustmentPolicy(policyText);
        if (policyEnumValue != DpiAdjustmentPolicy::Unset) {
            QHighDpiScaling::m_dpiAdjustmentPolicy = policyEnumValue;
        } else {
            auto values = joinEnumValues(std::begin(dpiAdjustmentPolicyLookup),
                                         std::end(dpiAdjustmentPolicyLookup));
            qWarning("Unknown DPI adjustment policy: %s. Supported values are: %s.",
                     policyText.constData(), values.constData());
        }
    }

    // Set initial active state
    m_active = m_globalScalingActive || m_usePlatformPluginDpi;

    qCDebug(lcHighDpi) << "Initialization done, high-DPI scaling is"
                       << (m_active ? "active" : "inactive");
}

/*
    Update configuration based on available screens and screen properties.
    This function may be called whenever the screen configuration changed.
*/
void QHighDpiScaling::updateHighDpiScaling()
{
    qCDebug(lcHighDpi) << "Updating high-DPI scaling";

    // Apply screen factors from environment
    if (m_screenFactors.size() > 0) {
        qCDebug(lcHighDpi) << "Applying screen factors" << m_screenFactors;
        int i = -1;
        const auto screens = QGuiApplication::screens();
        for (const auto &[name, rawFactor]: m_screenFactors) {
            const qreal factor = roundScaleFactor(rawFactor);
            ++i;
            if (name.isNull()) {
                if (i < screens.size())
                    setScreenFactor(screens.at(i), factor);
            } else {
                for (QScreen *screen : screens) {
                    if (screen->name() == name) {
                        setScreenFactor(screen, factor);
                        break;
                    }
                }
            }
        }
    }

    // Check if any screens (now) has a scale factor != 1 and set
    // m_platformPluginDpiScalingActive if so.
    if (m_usePlatformPluginDpi && !m_platformPluginDpiScalingActive ) {
        const auto screens = QGuiApplication::screens();
        for (QScreen *screen : screens) {
            if (!qFuzzyCompare(screenSubfactor(screen->handle()), qreal(1))) {
                m_platformPluginDpiScalingActive  = true;
                break;
            }
        }
    }

    m_active = m_globalScalingActive || m_screenFactorSet || m_platformPluginDpiScalingActive;

    qCDebug(lcHighDpi) << "Update done, high-DPI scaling is"
                       << (m_active ? "active" : "inactive");
}

/*
    Sets the global scale factor which is applied to all windows.
*/
void QHighDpiScaling::setGlobalFactor(qreal factor)
{
    qCDebug(lcHighDpi) << "Setting global scale factor to" << factor;

    if (qFuzzyCompare(factor, m_factor))
        return;
    if (!QGuiApplication::allWindows().isEmpty())
        qWarning("QHighDpiScaling::setFactor: Should only be called when no windows exist.");

    const auto screens = QGuiApplication::screens();

    std::vector<QScreenPrivate::UpdateEmitter> updateEmitters;
    for (QScreen *screen : screens)
        updateEmitters.emplace_back(screen);

    m_globalScalingActive = !qFuzzyCompare(factor, qreal(1));
    m_factor = m_globalScalingActive ? factor : qreal(1);
    m_active = m_globalScalingActive || m_screenFactorSet || m_platformPluginDpiScalingActive ;
    for (QScreen *screen : screens)
        screen->d_func()->updateGeometry();
}

static const char scaleFactorProperty[] = "_q_scaleFactor";

/*
    Sets a per-screen scale factor.
*/
void QHighDpiScaling::setScreenFactor(QScreen *screen, qreal factor)
{
    qCDebug(lcHighDpi) << "Setting screen scale factor for" << screen << "to" << factor;

    if (!qFuzzyCompare(factor, qreal(1))) {
        m_screenFactorSet = true;
        m_active = true;
    }

    QScreenPrivate::UpdateEmitter updateEmitter(screen);

    // Prefer associating the factor with screen name over the object
    // since the screen object may be deleted on screen disconnects.
    const QString name = screen->name();
    if (name.isEmpty())
        screen->setProperty(scaleFactorProperty, QVariant(factor));
    else
        QHighDpiScaling::m_namedScreenScaleFactors.insert(name, factor);

    screen->d_func()->updateGeometry();
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
            auto byNameIt = QHighDpiScaling::m_namedScreenScaleFactors.constFind(screen->name());
            if ((screenPropertyUsed = byNameIt != QHighDpiScaling::m_namedScreenScaleFactors.cend()))
                factor = *byNameIt;
        }
    }

    if (!screenPropertyUsed && m_usePlatformPluginDpi)
        factor = roundScaleFactor(rawScaleFactor(screen));

    return factor;
}

QDpi QHighDpiScaling::logicalDpi(const QScreen *screen)
{
    // (Note: m_active test is performed at call site.)
    if (!screen || !screen->handle())
        return QDpi(96, 96);

    if (!m_usePlatformPluginDpi) {
        const qreal screenScaleFactor = screenSubfactor(screen->handle());
        const QDpi dpi = QPlatformScreen::overrideDpi(screen->handle()->logicalDpi());
        return QDpi{ dpi.first / screenScaleFactor, dpi.second / screenScaleFactor };
    }

    const qreal scaleFactor = rawScaleFactor(screen->handle());
    const qreal roundedScaleFactor = roundScaleFactor(scaleFactor);
    return effectiveLogicalDpi(screen->handle(), scaleFactor, roundedScaleFactor);
}

// Returns the screen containing \a position, using \a guess as a starting point
// for the search. \a guess might be nullptr. Returns nullptr if \a position is outside
// of all screens.
QScreen *QHighDpiScaling::screenForPosition(QHighDpiScaling::Point position, QScreen *guess)
{
    if (position.kind == QHighDpiScaling::Point::Invalid)
        return nullptr;

    auto getPlatformScreenGuess = [](QScreen *maybeScreen) -> QPlatformScreen * {
        if (maybeScreen)
            return maybeScreen->handle();
        if (QScreen *primary = QGuiApplication::primaryScreen())
            return primary->handle();
        return nullptr;
    };

    QPlatformScreen *platformGuess = getPlatformScreenGuess(guess);
    if (!platformGuess)
        return nullptr;

    auto onScreen = [](QHighDpiScaling::Point position, const QPlatformScreen *platformScreen) -> bool {
        return position.kind == Point::Native
          ?  platformScreen->geometry().contains(position.point)
          :  platformScreen->screen()->geometry().contains(position.point);
    };

    // is the guessed screen correct?
    if (onScreen(position, platformGuess))
        return platformGuess->screen();

    // search sibling screens
    const auto screens = platformGuess->virtualSiblings();
    for (const QPlatformScreen *screen : screens) {
        if (onScreen(position, screen))
            return screen->screen();
    }

    return nullptr;
}

QVector<QHighDpiScaling::ScreenFactor> QHighDpiScaling::parseScreenScaleFactorsSpec(const QStringView &screenScaleFactors)
{
    QVector<QHighDpiScaling::ScreenFactor> screenFactors;

    // The spec is _either_
    // - a semicolon-separated ordered factor list: "1.5;2;3"
    // - a semicolon-separated name=factor list: "foo=1.5;bar=2;baz=3"
    const auto specs = screenScaleFactors.split(u';');
    for (const auto &spec : specs) {
        const qsizetype equalsPos = spec.lastIndexOf(u'=');
        if (equalsPos == -1) {
            // screens in order
            bool ok;
            const qreal factor = spec.toDouble(&ok);
            if (ok && factor > 0) {
                screenFactors.append(QHighDpiScaling::ScreenFactor(QString(), factor));
            }
        } else {
            // "name=factor"
            bool ok;
            const qreal factor = spec.mid(equalsPos + 1).toDouble(&ok);
            if (ok && factor > 0) {
                screenFactors.append(QHighDpiScaling::ScreenFactor(spec.left(equalsPos).toString(), factor));
            }
        }
    } // for (specs)

    return screenFactors;
}

QHighDpiScaling::ScaleAndOrigin QHighDpiScaling::scaleAndOrigin(const QPlatformScreen *platformScreen, QHighDpiScaling::Point position)
{
    Q_UNUSED(position)
    if (!m_active)
        return { qreal(1), QPoint() };
    if (!platformScreen)
        return { m_factor, QPoint() }; // the global factor
    return { m_factor * screenSubfactor(platformScreen), platformScreen->geometry().topLeft() };
}

QHighDpiScaling::ScaleAndOrigin QHighDpiScaling::scaleAndOrigin(const QScreen *screen, QHighDpiScaling::Point position)
{
    Q_UNUSED(position)
    if (!m_active)
        return { qreal(1), QPoint() };
    if (!screen)
        return { m_factor, QPoint() }; // the global factor
    return scaleAndOrigin(screen->handle(), position);
}

QHighDpiScaling::ScaleAndOrigin QHighDpiScaling::scaleAndOrigin(const QWindow *window, QHighDpiScaling::Point position)
{
    if (!m_active)
        return { qreal(1), QPoint() };

    // Determine correct screen; use the screen which contains the given
    // position if a valid position is passed.
    QScreen *screen = window ? window->screen() : QGuiApplication::primaryScreen();
    QScreen *overrideScreen = QHighDpiScaling::screenForPosition(position, screen);
    QScreen *targetScreen = overrideScreen ? overrideScreen : screen;
    return scaleAndOrigin(targetScreen, position);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QHighDpiScaling::ScreenFactor &factor)
{
    const QDebugStateSaver saver(debug);
    debug.nospace();
    if (!factor.name.isEmpty())
        debug << factor.name << "=";
    debug << factor.factor;
    return debug;
}
#endif

#else // QT_NO_HIGHDPISCALING

QHighDpiScaling::ScaleAndOrigin QHighDpiScaling::scaleAndOrigin(const QPlatformScreen *, QPoint *)
{
    return { qreal(1), QPoint() };
}

QHighDpiScaling::ScaleAndOrigin QHighDpiScaling::scaleAndOrigin(const QScreen *, QPoint *)
{
    return { qreal(1), QPoint() };
}

QHighDpiScaling::ScaleAndOrigin QHighDpiScaling::scaleAndOrigin(const QWindow *, QPoint *)
{
    return { qreal(1), QPoint() };
}

#endif // QT_NO_HIGHDPISCALING

QT_END_NAMESPACE

#include "moc_qhighdpiscaling_p.cpp"
