/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

static const char legacyDevicePixelEnvVar[] = "QT_DEVICE_PIXEL_RATIO";
static const char scaleFactorEnvVar[] = "QT_SCALE_FACTOR";
static const char autoScreenEnvVar[] = "QT_AUTO_SCREEN_SCALE_FACTOR";
static const char screenFactorsEnvVar[] = "QT_SCREEN_SCALE_FACTORS";

static inline qreal initialScaleFactor()
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
            qWarning() << "Warning:" << legacyDevicePixelEnvVar << "is deprecated. Instead use:";
            qWarning() << "   " << scaleFactorEnvVar << "to set the application global scale factor.";
            qWarning() << "   " << autoScreenEnvVar << "to enable platform plugin controlled per-screen factors.";

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

    The coordinate system scaling is enabled by setting one or more scale
    factors. These will then be factored into the value returned by the
    devicePixelRatio() accessors (any native scale factor will also be
    included in this value). Several setters are available:

    - A process-global scale factor
        - QT_SCALE_FACTOR (environment variable)
        - QHighDpiScaling::setGlobalFactor()

    - A per-screen scale factor
        - QT_AUTO_SCALE_FACTOR (environment variable)
          Setting this to a true-ish value will make QHighDpiScaling
          call QPlatformScreen::pixelDensity()
        - QHighDpiScaling::setScreenFactor(screen, factor);
        - QT_SCREEN_SCALE_FACTORS (environment variable)
        Set this to a semicolon-separated list of scale factors
        (matching the order of QGuiApplications::screens()),
        or to a list of name=value pairs (where name matches
        QScreen::name()).

    All scale factors are of type qreal.

    The main scaling functions for use in QtGui are:
        T toNativePixels(T, QWindow *)
        T fromNativePixels(T, QWindow*)
    Where T is QPoint, QSize, QRect etc.
*/

qreal QHighDpiScaling::m_factor;
bool QHighDpiScaling::m_active; //"overall active" - is there any scale factor set.
bool QHighDpiScaling::m_usePixelDensity; // use scale factor from platform plugin
bool QHighDpiScaling::m_pixelDensityScalingActive; // pixel density scale factor > 1
bool QHighDpiScaling::m_globalScalingActive; // global scale factor is active
bool QHighDpiScaling::m_screenFactorSet; // QHighDpiScaling::setScreenFactor has been used
QDpi QHighDpiScaling::m_logicalDpi; // The scaled logical DPI of the primary screen

/*
    Initializes the QHighDpiScaling global variables. Called before the
    platform plugin is created.
*/
void QHighDpiScaling::initHighDpiScaling()
{
    if (QCoreApplication::testAttribute(Qt::AA_NoHighDpiScaling)) {
        m_factor = 1;
        m_active = false;
        return;
    }
    m_factor = initialScaleFactor();
    bool usePlatformPluginPixelDensity = qEnvironmentVariableIsSet(autoScreenEnvVar)
                                         || qgetenv(legacyDevicePixelEnvVar).toLower() == "auto";

    m_globalScalingActive = !qFuzzyCompare(m_factor, qreal(1));
    m_usePixelDensity = usePlatformPluginPixelDensity;
    m_pixelDensityScalingActive = false; //set in updateHighDpiScaling below

    // we update m_active in updateHighDpiScaling, but while we create the
    // screens, we have to assume that m_usePixelDensity implies scaling
    m_active = m_globalScalingActive || m_usePixelDensity;
}

void QHighDpiScaling::updateHighDpiScaling()
{
    if (QCoreApplication::testAttribute(Qt::AA_NoHighDpiScaling))
        return;

    if (m_usePixelDensity && !m_pixelDensityScalingActive) {
        Q_FOREACH (QScreen *screen, QGuiApplication::screens()) {
            if (!qFuzzyCompare(screenSubfactor(screen->handle()), qreal(1))) {
                m_pixelDensityScalingActive = true;
                break;
            }
        }
    }
    if (qEnvironmentVariableIsSet(screenFactorsEnvVar)) {
        int i = 0;
        Q_FOREACH (QByteArray spec, qgetenv(screenFactorsEnvVar).split(';')) {
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
                    Q_FOREACH (QScreen *s, QGuiApplication::screens()) {
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
    if (!QGuiApplication::allWindows().isEmpty()) {
        qWarning() << Q_FUNC_INFO << "QHighDpiScaling::setFactor: Should only be called when no windows exist.";
    }

    m_globalScalingActive = !qFuzzyCompare(factor, qreal(1));
    m_factor = m_globalScalingActive ? factor : qreal(1);
    m_active = m_globalScalingActive || m_screenFactorSet || m_pixelDensityScalingActive;
    Q_FOREACH (QScreen *screen, QGuiApplication::screens())
         screen->d_func()->updateHighDpi();
}

static const char *scaleFactorProperty = "_q_scaleFactor";

/*
    Sets a per-screen scale factor.
*/
void QHighDpiScaling::setScreenFactor(QScreen *screen, qreal factor)
{
    m_screenFactorSet = true;
    m_active = true;
    screen->setProperty(scaleFactorProperty, QVariant(factor));

    //### dirty hack to force re-evaluation of screen geometry
    if (screen->handle())
        screen->d_func()->setPlatformScreen(screen->handle()); // update geometries based on scale factor
}

/*

QPoint QXcbScreen::mapToNative(const QPoint &pos) const
{
    const int dpr = int(devicePixelRatio());
    return (pos - m_geometry.topLeft()) * dpr + m_nativeGeometry.topLeft();
}

QPoint QXcbScreen::mapFromNative(const QPoint &pos) const
{
    const int dpr = int(devicePixelRatio());
    return (pos - m_nativeGeometry.topLeft()) / dpr + m_geometry.topLeft();
}


 */


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
    if (!m_active || !window)
        return qreal(1.0);

    return factor(window->screen());
}

QPoint QHighDpiScaling::origin(const QScreen *screen)
{
    return screen->geometry().topLeft();
}

QPoint QHighDpiScaling::origin(const QPlatformScreen *platformScreen)
{
    return platformScreen->geometry().topLeft();
}

QT_END_NAMESPACE
