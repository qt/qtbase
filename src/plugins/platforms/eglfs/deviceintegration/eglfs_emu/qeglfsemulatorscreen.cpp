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

#include "qeglfsemulatorscreen.h"

QT_BEGIN_NAMESPACE

QEglFSEmulatorScreen::QEglFSEmulatorScreen(const QJsonObject &screenDescription)
    : QEglFSScreen(eglGetDisplay(EGL_DEFAULT_DISPLAY))
    , m_id(0)
{
    initFromJsonObject(screenDescription);
}

QRect QEglFSEmulatorScreen::geometry() const
{
    return m_geometry;
}

QRect QEglFSEmulatorScreen::rawGeometry() const
{
    return QRect(QPoint(0, 0), m_geometry.size());
}

int QEglFSEmulatorScreen::depth() const
{
    return m_depth;
}

QImage::Format QEglFSEmulatorScreen::format() const
{
    return m_format;
}

QSizeF QEglFSEmulatorScreen::physicalSize() const
{
    return m_physicalSize;
}

QDpi QEglFSEmulatorScreen::logicalDpi() const
{
    const QSizeF ps = m_physicalSize;
    const QSize s = m_geometry.size();

    if (!ps.isEmpty() && !s.isEmpty())
        return QDpi(25.4 * s.width() / ps.width(),
                    25.4 * s.height() / ps.height());
    else
        return QDpi(100, 100);
}

qreal QEglFSEmulatorScreen::pixelDensity() const
{
    return m_pixelDensity;
}

qreal QEglFSEmulatorScreen::refreshRate() const
{
    return m_refreshRate;
}

Qt::ScreenOrientation QEglFSEmulatorScreen::nativeOrientation() const
{
    return m_nativeOrientation;
}

Qt::ScreenOrientation QEglFSEmulatorScreen::orientation() const
{
    return m_orientation;
}

uint QEglFSEmulatorScreen::id() const
{
    return m_id;
}

QString QEglFSEmulatorScreen::name() const
{
    return m_description;
}

void QEglFSEmulatorScreen::initFromJsonObject(const QJsonObject &description)
{
    QJsonValue value;

    value = description.value(QLatin1String("id"));
    if (!value.isUndefined() && value.isDouble())
        m_id = value.toInt();

    value = description.value(QLatin1String("description"));
    if (!value.isUndefined() && value.isString())
        m_description = value.toString();

    value = description.value(QLatin1String("geometry"));
    if (!value.isUndefined() && value.isObject()) {
        QJsonObject geometryObject = value.toObject();
        value = geometryObject.value(QLatin1String("x"));
        if (!value.isUndefined() && value.isDouble())
            m_geometry.setX(value.toInt());
        value = geometryObject.value(QLatin1String("y"));
        if (!value.isUndefined() && value.isDouble())
            m_geometry.setY(value.toInt());
        value = geometryObject.value(QLatin1String("width"));
        if (!value.isUndefined() && value.isDouble())
            m_geometry.setWidth(value.toInt());
        value = geometryObject.value(QLatin1String("height"));
        if (!value.isUndefined() && value.isDouble())
            m_geometry.setHeight(value.toInt());
    }

    value = description.value(QLatin1String("depth"));
    if (!value.isUndefined() && value.isDouble())
        m_depth = value.toInt();

    value = description.value(QLatin1String("format"));
    if (!value.isUndefined() && value.isDouble())
        m_format = static_cast<QImage::Format>(value.toInt());

    value = description.value(QLatin1String("physicalSize"));
    if (!value.isUndefined() && value.isObject()) {
        QJsonObject physicalSizeObject = value.toObject();
        value = physicalSizeObject.value(QLatin1String("width"));
        if (!value.isUndefined() && value.isDouble())
            m_physicalSize.setWidth(value.toInt());
        value = physicalSizeObject.value(QLatin1String("height"));
        if (!value.isUndefined() && value.isDouble())
            m_physicalSize.setHeight(value.toInt());
    }

    value = description.value(QLatin1String("pixelDensity"));
    if (!value.isUndefined() && value.isDouble())
        m_pixelDensity = value.toDouble();

    value = description.value(QLatin1String("refreshRate"));
    if (!value.isUndefined() && value.isDouble())
        m_refreshRate = value.toDouble();

    value = description.value(QLatin1String("nativeOrientation"));
    if (!value.isUndefined() && value.isDouble())
        m_nativeOrientation = static_cast<Qt::ScreenOrientation>(value.toInt());

    value = description.value(QLatin1String("orientation"));
    if (!value.isUndefined() && value.isDouble())
        m_orientation = static_cast<Qt::ScreenOrientation>(value.toInt());
}

QT_END_NAMESPACE
