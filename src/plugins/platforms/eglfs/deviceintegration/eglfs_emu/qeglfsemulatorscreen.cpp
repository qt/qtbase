// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfsemulatorscreen.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
    return logicalBaseDpi();
}

QDpi QEglFSEmulatorScreen::logicalBaseDpi() const
{
    return QDpi(100, 100);
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

    value = description.value("id"_L1);
    if (!value.isUndefined() && value.isDouble())
        m_id = value.toInt();

    value = description.value("description"_L1);
    if (!value.isUndefined() && value.isString())
        m_description = value.toString();

    value = description.value("geometry"_L1);
    if (!value.isUndefined() && value.isObject()) {
        QJsonObject geometryObject = value.toObject();
        value = geometryObject.value("x"_L1);
        if (!value.isUndefined() && value.isDouble())
            m_geometry.setX(value.toInt());
        value = geometryObject.value("y"_L1);
        if (!value.isUndefined() && value.isDouble())
            m_geometry.setY(value.toInt());
        value = geometryObject.value("width"_L1);
        if (!value.isUndefined() && value.isDouble())
            m_geometry.setWidth(value.toInt());
        value = geometryObject.value("height"_L1);
        if (!value.isUndefined() && value.isDouble())
            m_geometry.setHeight(value.toInt());
    }

    value = description.value("depth"_L1);
    if (!value.isUndefined() && value.isDouble())
        m_depth = value.toInt();

    value = description.value("format"_L1);
    if (!value.isUndefined() && value.isDouble())
        m_format = static_cast<QImage::Format>(value.toInt());

    value = description.value("physicalSize"_L1);
    if (!value.isUndefined() && value.isObject()) {
        QJsonObject physicalSizeObject = value.toObject();
        value = physicalSizeObject.value("width"_L1);
        if (!value.isUndefined() && value.isDouble())
            m_physicalSize.setWidth(value.toInt());
        value = physicalSizeObject.value("height"_L1);
        if (!value.isUndefined() && value.isDouble())
            m_physicalSize.setHeight(value.toInt());
    }


    value = description.value("refreshRate"_L1);
    if (!value.isUndefined() && value.isDouble())
        m_refreshRate = value.toDouble();

    value = description.value("nativeOrientation"_L1);
    if (!value.isUndefined() && value.isDouble())
        m_nativeOrientation = static_cast<Qt::ScreenOrientation>(value.toInt());

    value = description.value("orientation"_L1);
    if (!value.isUndefined() && value.isDouble())
        m_orientation = static_cast<Qt::ScreenOrientation>(value.toInt());
}

QT_END_NAMESPACE
