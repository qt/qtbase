/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qshaderformat_p.h"

QT_BEGIN_NAMESPACE

QShaderFormat::QShaderFormat() noexcept
    : m_api(NoApi)
    , m_shaderType(Fragment)
{
}

QShaderFormat::Api QShaderFormat::api() const noexcept
{
    return m_api;
}

void QShaderFormat::setApi(QShaderFormat::Api api) noexcept
{
    m_api = api;
}

QVersionNumber QShaderFormat::version() const noexcept
{
    return m_version;
}

void QShaderFormat::setVersion(const QVersionNumber &version) noexcept
{
    m_version = version;
}

QStringList QShaderFormat::extensions() const noexcept
{
    return m_extensions;
}

void QShaderFormat::setExtensions(const QStringList &extensions) noexcept
{
    m_extensions = extensions;
    m_extensions.sort();
}

QString QShaderFormat::vendor() const noexcept
{
    return m_vendor;
}

void QShaderFormat::setVendor(const QString &vendor) noexcept
{
    m_vendor = vendor;
}

bool QShaderFormat::isValid() const noexcept
{
    return m_api != NoApi && m_version.majorVersion() > 0;
}

bool QShaderFormat::supports(const QShaderFormat &other) const noexcept
{
    if (!isValid() || !other.isValid())
        return false;

    if (m_api == OpenGLES && m_api != other.m_api)
        return false;

    if (m_api == OpenGLCoreProfile && m_api != other.m_api)
        return false;

    if (m_version < other.m_version)
        return false;

    if (m_shaderType != other.m_shaderType)
        return false;

    const auto containsAllExtensionsFromOther = std::includes(m_extensions.constBegin(),
                                                              m_extensions.constEnd(),
                                                              other.m_extensions.constBegin(),
                                                              other.m_extensions.constEnd());
    if (!containsAllExtensionsFromOther)
        return false;

    if (!other.m_vendor.isEmpty() && m_vendor != other.m_vendor)
        return false;

    return true;
}

QShaderFormat::ShaderType QShaderFormat::shaderType() const Q_DECL_NOTHROW
{
    return m_shaderType;
}

void QShaderFormat::setShaderType(QShaderFormat::ShaderType shaderType) Q_DECL_NOTHROW
{
    m_shaderType = shaderType;
}

bool operator==(const QShaderFormat &lhs, const QShaderFormat &rhs) noexcept
{
    return lhs.api() == rhs.api()
        && lhs.version() == rhs.version()
        && lhs.extensions() == rhs.extensions()
        && lhs.vendor() == rhs.vendor()
        && lhs.shaderType() == rhs.shaderType();
}

QT_END_NAMESPACE
