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

#ifndef QSHADERFORMAT_P_H
#define QSHADERFORMAT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QShaderFormat
{
public:
    enum Api : int {
        NoApi,
        OpenGLNoProfile,
        OpenGLCoreProfile,
        OpenGLCompatibilityProfile,
        OpenGLES,
        VulkanFlavoredGLSL
    };

    enum ShaderType : int {
        Vertex = 0,
        TessellationControl,
        TessellationEvaluation,
        Geometry,
        Fragment,
        Compute
    };

    Q_GUI_EXPORT QShaderFormat() noexcept;

    Q_GUI_EXPORT Api api() const noexcept;
    Q_GUI_EXPORT void setApi(Api api) noexcept;

    Q_GUI_EXPORT QVersionNumber version() const noexcept;
    Q_GUI_EXPORT void setVersion(const QVersionNumber &version) noexcept;

    Q_GUI_EXPORT QStringList extensions() const noexcept;
    Q_GUI_EXPORT void setExtensions(const QStringList &extensions) noexcept;

    Q_GUI_EXPORT QString vendor() const noexcept;
    Q_GUI_EXPORT void setVendor(const QString &vendor) noexcept;

    Q_GUI_EXPORT bool isValid() const noexcept;
    Q_GUI_EXPORT bool supports(const QShaderFormat &other) const noexcept;

    Q_GUI_EXPORT ShaderType shaderType() const Q_DECL_NOTHROW;
    Q_GUI_EXPORT void setShaderType(ShaderType shaderType) Q_DECL_NOTHROW;

private:
    Api m_api;
    QVersionNumber m_version;
    QStringList m_extensions;
    QString m_vendor;
    ShaderType m_shaderType;
};

Q_GUI_EXPORT bool operator==(const QShaderFormat &lhs, const QShaderFormat &rhs) noexcept;

inline bool operator!=(const QShaderFormat &lhs, const QShaderFormat &rhs) noexcept
{
    return !(lhs == rhs);
}

Q_DECLARE_TYPEINFO(QShaderFormat, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QShaderFormat)

#endif // QSHADERFORMAT_P_H
