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
*/


qreal QHighDpiScaling::m_factor;

bool QHighDpiScaling::m_active; //"overall active" - is there any scale factor set.
bool QHighDpiScaling::m_perScreenActive; // different screens may have different scale
bool QHighDpiScaling::m_usePixelDensity; // use scale factor from platform plugin

void QHighDpiScaling::initHighDPiScaling()
{
    m_factor = initialScaleFactor();
    bool usePlatformPluginPixelDensity = qEnvironmentVariableIsSet(autoScreenEnvVar)
                                         || qgetenv(legacyDevicePixelEnvVar).toLower() == "auto";

    // m_active below is "overall active" - is there any scale factor set.
    m_active = !qFuzzyCompare(m_factor, qreal(1)) || usePlatformPluginPixelDensity;
    m_usePixelDensity = usePlatformPluginPixelDensity;
    m_perScreenActive = m_usePixelDensity;
}

/*
    Sets the global scale factor which is applied to all windows.
*/
void QHighDpiScaling::setGlobalFactor(qreal factor)
{
    if (qFuzzyCompare(factor, QHighDpiScaling::m_factor))
        return;
    if (!QGuiApplication::allWindows().isEmpty()) {
        qWarning() << Q_FUNC_INFO << "QHighDpiScaling::setFactor: Should only be called when no windows exist.";
    }

    QHighDpiScaling::m_active = !qFuzzyCompare(factor, qreal(1));
    QHighDpiScaling::m_factor = QHighDpiScaling::m_active ? factor : qreal(1);
    Q_FOREACH (QScreen *screen, QGuiApplication::screens())
         screen->d_func()->updateHighDpi();
}

static const char *scaleFactorProperty = "_q_scaleFactor";

/*
    Sets a per-screen scale factor.
*/
void QHighDpiScaling::setScreenFactor(QScreen *screen, qreal factor)
{
    m_active = true;
    m_perScreenActive = true;
    screen->setProperty(scaleFactorProperty, QVariant(factor));
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
    if (m_perScreenActive && screen) {
        if (m_usePixelDensity)
            factor *= screen->pixelDensity();
        QVariant screenFactor = screen->screen()->property(scaleFactorProperty);
        if (screenFactor.isValid())
            factor *= screenFactor.toReal();
    }
    return factor;
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
