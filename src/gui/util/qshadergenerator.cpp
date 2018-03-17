/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
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

#include "qshadergenerator_p.h"

#include "qshaderlanguage_p.h"

QT_BEGIN_NAMESPACE

namespace
{
    QByteArray toGlsl(QShaderLanguage::StorageQualifier qualifier, const QShaderFormat &format)
    {
        if (format.version().majorVersion() <= 2) {
            // Note we're assuming fragment shader only here, it'd be different
            // values for vertex shader, will need to be fixed properly at some
            // point but isn't necessary yet (this problem already exists in past
            // commits anyway)
            switch (qualifier) {
            case QShaderLanguage::Const:
                return "const";
            case QShaderLanguage::Input:
                return "varying";
            case QShaderLanguage::Output:
                return ""; // Although fragment shaders for <=2 only have fixed outputs
            case QShaderLanguage::Uniform:
                return "uniform";
            case QShaderLanguage::BuiltIn:
                return "//";
            }
        } else {
            switch (qualifier) {
            case QShaderLanguage::Const:
                return "const";
            case QShaderLanguage::Input:
                return "in";
            case QShaderLanguage::Output:
                return "out";
            case QShaderLanguage::Uniform:
                return "uniform";
            case QShaderLanguage::BuiltIn:
                return "//";
            }
        }

        Q_UNREACHABLE();
    }

    QByteArray toGlsl(QShaderLanguage::VariableType type)
    {
        switch (type) {
        case QShaderLanguage::Bool:
            return "bool";
        case QShaderLanguage::Int:
            return "int";
        case QShaderLanguage::Uint:
            return "uint";
        case QShaderLanguage::Float:
            return "float";
        case QShaderLanguage::Double:
            return "double";
        case QShaderLanguage::Vec2:
            return "vec2";
        case QShaderLanguage::Vec3:
            return "vec3";
        case QShaderLanguage::Vec4:
            return "vec4";
        case QShaderLanguage::DVec2:
            return "dvec2";
        case QShaderLanguage::DVec3:
            return "dvec3";
        case QShaderLanguage::DVec4:
            return "dvec4";
        case QShaderLanguage::BVec2:
            return "bvec2";
        case QShaderLanguage::BVec3:
            return "bvec3";
        case QShaderLanguage::BVec4:
            return "bvec4";
        case QShaderLanguage::IVec2:
            return "ivec2";
        case QShaderLanguage::IVec3:
            return "ivec3";
        case QShaderLanguage::IVec4:
            return "ivec4";
        case QShaderLanguage::UVec2:
            return "uvec2";
        case QShaderLanguage::UVec3:
            return "uvec3";
        case QShaderLanguage::UVec4:
            return "uvec4";
        case QShaderLanguage::Mat2:
            return "mat2";
        case QShaderLanguage::Mat3:
            return "mat3";
        case QShaderLanguage::Mat4:
            return "mat4";
        case QShaderLanguage::Mat2x2:
            return "mat2x2";
        case QShaderLanguage::Mat2x3:
            return "mat2x3";
        case QShaderLanguage::Mat2x4:
            return "mat2x4";
        case QShaderLanguage::Mat3x2:
            return "mat3x2";
        case QShaderLanguage::Mat3x3:
            return "mat3x3";
        case QShaderLanguage::Mat3x4:
            return "mat3x4";
        case QShaderLanguage::Mat4x2:
            return "mat4x2";
        case QShaderLanguage::Mat4x3:
            return "mat4x3";
        case QShaderLanguage::Mat4x4:
            return "mat4x4";
        case QShaderLanguage::DMat2:
            return "dmat2";
        case QShaderLanguage::DMat3:
            return "dmat3";
        case QShaderLanguage::DMat4:
            return "dmat4";
        case QShaderLanguage::DMat2x2:
            return "dmat2x2";
        case QShaderLanguage::DMat2x3:
            return "dmat2x3";
        case QShaderLanguage::DMat2x4:
            return "dmat2x4";
        case QShaderLanguage::DMat3x2:
            return "dmat3x2";
        case QShaderLanguage::DMat3x3:
            return "dmat3x3";
        case QShaderLanguage::DMat3x4:
            return "dmat3x4";
        case QShaderLanguage::DMat4x2:
            return "dmat4x2";
        case QShaderLanguage::DMat4x3:
            return "dmat4x3";
        case QShaderLanguage::DMat4x4:
            return "dmat4x4";
        case QShaderLanguage::Sampler1D:
            return "sampler1D";
        case QShaderLanguage::Sampler2D:
            return "sampler2D";
        case QShaderLanguage::Sampler3D:
            return "sampler3D";
        case QShaderLanguage::SamplerCube:
            return "samplerCube";
        case QShaderLanguage::Sampler2DRect:
            return "sampler2DRect";
        case QShaderLanguage::Sampler2DMs:
            return "sampler2DMS";
        case QShaderLanguage::SamplerBuffer:
            return "samplerBuffer";
        case QShaderLanguage::Sampler1DArray:
            return "sampler1DArray";
        case QShaderLanguage::Sampler2DArray:
            return "sampler2DArray";
        case QShaderLanguage::Sampler2DMsArray:
            return "sampler2DMSArray";
        case QShaderLanguage::SamplerCubeArray:
            return "samplerCubeArray";
        case QShaderLanguage::Sampler1DShadow:
            return "sampler1DShadow";
        case QShaderLanguage::Sampler2DShadow:
            return "sampler2DShadow";
        case QShaderLanguage::Sampler2DRectShadow:
            return "sampler2DRectShadow";
        case QShaderLanguage::Sampler1DArrayShadow:
            return "sampler1DArrayShadow";
        case QShaderLanguage::Sampler2DArrayShadow:
            return "sample2DArrayShadow";
        case QShaderLanguage::SamplerCubeShadow:
            return "samplerCubeShadow";
        case QShaderLanguage::SamplerCubeArrayShadow:
            return "samplerCubeArrayShadow";
        case QShaderLanguage::ISampler1D:
            return "isampler1D";
        case QShaderLanguage::ISampler2D:
            return "isampler2D";
        case QShaderLanguage::ISampler3D:
            return "isampler3D";
        case QShaderLanguage::ISamplerCube:
            return "isamplerCube";
        case QShaderLanguage::ISampler2DRect:
            return "isampler2DRect";
        case QShaderLanguage::ISampler2DMs:
            return "isampler2DMS";
        case QShaderLanguage::ISamplerBuffer:
            return "isamplerBuffer";
        case QShaderLanguage::ISampler1DArray:
            return "isampler1DArray";
        case QShaderLanguage::ISampler2DArray:
            return "isampler2DArray";
        case QShaderLanguage::ISampler2DMsArray:
            return "isampler2DMSArray";
        case QShaderLanguage::ISamplerCubeArray:
            return "isamplerCubeArray";
        case QShaderLanguage::USampler1D:
            return "usampler1D";
        case QShaderLanguage::USampler2D:
            return "usampler2D";
        case QShaderLanguage::USampler3D:
            return "usampler3D";
        case QShaderLanguage::USamplerCube:
            return "usamplerCube";
        case QShaderLanguage::USampler2DRect:
            return "usampler2DRect";
        case QShaderLanguage::USampler2DMs:
            return "usampler2DMS";
        case QShaderLanguage::USamplerBuffer:
            return "usamplerBuffer";
        case QShaderLanguage::USampler1DArray:
            return "usampler1DArray";
        case QShaderLanguage::USampler2DArray:
            return "usampler2DArray";
        case QShaderLanguage::USampler2DMsArray:
            return "usampler2DMSArray";
        case QShaderLanguage::USamplerCubeArray:
            return "usamplerCubeArray";
        }

        Q_UNREACHABLE();
    }

    QByteArray replaceParameters(const QByteArray &original, const QShaderNode &node, const QShaderFormat &format)
    {
        auto result = original;

        for (const auto &parameterName : node.parameterNames()) {
            const auto placeholder = QByteArray(QByteArrayLiteral("$") + parameterName.toUtf8());
            const auto parameter = node.parameter(parameterName);
            if (parameter.userType() == qMetaTypeId<QShaderLanguage::StorageQualifier>()) {
                const auto qualifier = parameter.value<QShaderLanguage::StorageQualifier>();
                const auto value = toGlsl(qualifier, format);
                result.replace(placeholder, value);
            } else if (parameter.userType() == qMetaTypeId<QShaderLanguage::VariableType>()) {
                const auto type = parameter.value<QShaderLanguage::VariableType>();
                const auto value = toGlsl(type);
                result.replace(placeholder, value);
            } else {
                const auto value = parameter.toString().toUtf8();
                result.replace(placeholder, value);
            }
        }

        return result;
    }
}

QByteArray QShaderGenerator::createShaderCode(const QStringList &enabledLayers) const
{
    auto code = QByteArrayList();

    if (format.isValid()) {
        const auto isGLES = format.api() == QShaderFormat::OpenGLES;
        const auto major = format.version().majorVersion();
        const auto minor = format.version().minorVersion();

        const auto version = major == 2 && isGLES ? 100
                           : major == 3 && isGLES ? 300
                           : major == 2 ? 100 + 10 * (minor + 1)
                           : major == 3 && minor <= 2 ? 100 + 10 * (minor + 3)
                           : major * 100 + minor * 10;

        const auto profile = isGLES && version > 100 ? QByteArrayLiteral(" es")
                           : version >= 150 && format.api() == QShaderFormat::OpenGLCoreProfile ? QByteArrayLiteral(" core")
                           : version >= 150 && format.api() == QShaderFormat::OpenGLCompatibilityProfile ? QByteArrayLiteral(" compatibility")
                           : QByteArray();

        code << (QByteArrayLiteral("#version ") + QByteArray::number(version) + profile);
        code << QByteArray();
    }

    const auto intersectsEnabledLayers = [enabledLayers] (const QStringList &layers) {
        return layers.isEmpty()
            || std::any_of(layers.cbegin(), layers.cend(),
                           [enabledLayers] (const QString &s) { return enabledLayers.contains(s); });
    };

    for (const auto &node : graph.nodes()) {
        if (intersectsEnabledLayers(node.layers())) {
            for (const auto &snippet : node.rule(format).headerSnippets) {
                code << replaceParameters(snippet, node, format);
            }
        }
    }

    code << QByteArray();
    code << QByteArrayLiteral("void main()");
    code << QByteArrayLiteral("{");

    for (const auto &statement : graph.createStatements(enabledLayers)) {
        const auto node = statement.node;
        auto line = node.rule(format).substitution;
        for (const auto &port : node.ports()) {
            const auto portName = port.name;
            const auto portDirection = port.direction;
            const auto isInput = port.direction == QShaderNodePort::Input;

            const auto portIndex = statement.portIndex(portDirection, portName);
            const auto variableIndex = isInput ? statement.inputs.at(portIndex)
                                               : statement.outputs.at(portIndex);
            if (variableIndex < 0)
                continue;

            const auto placeholder = QByteArray(QByteArrayLiteral("$") + portName.toUtf8());
            const auto variable = QByteArray(QByteArrayLiteral("v") + QByteArray::number(variableIndex));
            line.replace(placeholder, variable);
        }

        code << QByteArrayLiteral("    ") + replaceParameters(line, node, format);
    }

    code << QByteArrayLiteral("}");
    code << QByteArray();
    return code.join('\n');
}

QT_END_NAMESPACE
