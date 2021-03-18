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
#include <QRegularExpression>

#include <cctype>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(ShaderGenerator, "ShaderGenerator", QtWarningMsg)

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
                if (format.shaderType() == QShaderFormat::Vertex)
                    return "attribute";
                else
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
        QByteArray result = original;

        const QStringList parameterNames = node.parameterNames();
        for (const QString &parameterName : parameterNames) {
            const QByteArray placeholder = QByteArray(QByteArrayLiteral("$") + parameterName.toUtf8());
            const QVariant parameter = node.parameter(parameterName);
            if (parameter.userType() == qMetaTypeId<QShaderLanguage::StorageQualifier>()) {
                const QShaderLanguage::StorageQualifier qualifier = qvariant_cast<QShaderLanguage::StorageQualifier>(parameter);
                const QByteArray value = toGlsl(qualifier, format);
                result.replace(placeholder, value);
            } else if (parameter.userType() == qMetaTypeId<QShaderLanguage::VariableType>()) {
                const QShaderLanguage::VariableType type = qvariant_cast<QShaderLanguage::VariableType>(parameter);
                const QByteArray value = toGlsl(type);
                result.replace(placeholder, value);
            } else {
                const QByteArray value = parameter.toString().toUtf8();
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
        const bool isGLES = format.api() == QShaderFormat::OpenGLES;
        const int major = format.version().majorVersion();
        const int minor = format.version().minorVersion();

        const int version = major == 2 && isGLES ? 100
                          : major == 3 && isGLES ? 300
                          : major == 2 ? 100 + 10 * (minor + 1)
                          : major == 3 && minor <= 2 ? 100 + 10 * (minor + 3)
                          : major * 100 + minor * 10;

        const QByteArray profile = isGLES && version > 100 ? QByteArrayLiteral(" es")
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

    QVector<QString> globalInputVariables;
    const QRegularExpression globalInputExtractRegExp(QStringLiteral("^.*\\s+(\\w+).*;$"));

    const QVector<QShaderNode> nodes = graph.nodes();
    for (const QShaderNode &node : nodes) {
        if (intersectsEnabledLayers(node.layers())) {
            const QByteArrayList headerSnippets = node.rule(format).headerSnippets;
            for (const QByteArray &snippet : headerSnippets) {
                code << replaceParameters(snippet, node, format);

                // If node is an input, record the variable name into the globalInputVariables vector
                if (node.type() == QShaderNode::Input) {
                    const QRegularExpressionMatch match = globalInputExtractRegExp.match(QString::fromUtf8(code.last()));
                    if (match.hasMatch())
                        globalInputVariables.push_back(match.captured(1));
                }
            }
        }
    }

    code << QByteArray();
    code << QByteArrayLiteral("void main()");
    code << QByteArrayLiteral("{");

    const QRegularExpression temporaryVariableToAssignmentRegExp(QStringLiteral("([^;]*\\s+(v\\d+))\\s*=\\s*([^;]*);"));
    const QRegularExpression temporaryVariableInAssignmentRegExp(QStringLiteral("\\W*(v\\d+)\\W*"));
    const QRegularExpression statementRegExp(QStringLiteral("\\s*(\\w+)\\s*=\\s*([^;]*);"));

    struct Variable;

    struct Assignment
    {
        QString expression;
        QVector<Variable *> referencedVariables;
    };

    struct Variable
    {
        enum Type {
            GlobalInput,
            TemporaryAssignment,
            Output
        };

        QString name;
        QString declaration;
        int referenceCount = 0;
        Assignment assignment;
        Type type = TemporaryAssignment;
        bool substituted = false;

        static void substitute(Variable *v)
        {
            if (v->substituted)
                return;

            qCDebug(ShaderGenerator) << "Begin Substituting " << v->name << " = " << v->assignment.expression;
            for (Variable *ref : qAsConst(v->assignment.referencedVariables)) {
                // Recursively substitute
                Variable::substitute(ref);

                // Replace all variables referenced only once in the assignment
                // by their actual expression
                if (ref->referenceCount == 1 || ref->type == Variable::GlobalInput) {
                    const QRegularExpression r(QStringLiteral("(.*\\b)(%1)(\\b.*)").arg(ref->name));
                    if (v->assignment.referencedVariables.size() == 1)
                        v->assignment.expression.replace(r,
                                                         QStringLiteral("\\1%2\\3").arg(ref->assignment.expression));
                    else
                        v->assignment.expression.replace(r,
                                                         QStringLiteral("(\\1%2\\3)").arg(ref->assignment.expression));
                }
            }
            qCDebug(ShaderGenerator) << "Done Substituting " << v->name << " = " << v->assignment.expression;
            v->substituted = true;
        }
    };

    struct LineContent
    {
        QByteArray rawContent;
        Variable *var = nullptr;
    };

    // Table to store temporary variables that should be replaced:
    // - If variable references a a global variables
    //   -> we will use the global variable directly
    // - If variable references a function results
    //   -> will be kept only if variable is referenced more than once.
    // This avoids having vec3 v56 = vertexPosition; when we could
    // just use vertexPosition directly.
    // The added benefit is when having arrays, we don't try to create
    // mat4 v38 = skinningPalelette[100] which would be invalid
    QVector<Variable> temporaryVariables;
    // Reserve more than enough space to ensure no reallocation will take place
    temporaryVariables.reserve(nodes.size() * 8);

    QVector<LineContent> lines;

    auto createVariable = [&] () -> Variable * {
        Q_ASSERT(temporaryVariables.capacity() > 0);
        temporaryVariables.resize(temporaryVariables.size() + 1);
        return &temporaryVariables.last();
    };

    auto findVariable = [&] (const QString &name) -> Variable * {
        const auto end = temporaryVariables.end();
        auto it = std::find_if(temporaryVariables.begin(), end,
                               [=] (const Variable &a) { return a.name == name; });
        if (it != end)
            return &(*it);
        return nullptr;
    };

    auto gatherTemporaryVariablesFromAssignment = [&] (Variable *v, const QString &assignmentContent) {
        QRegularExpressionMatchIterator subMatchIt = temporaryVariableInAssignmentRegExp.globalMatch(assignmentContent);
        while (subMatchIt.hasNext()) {
            const QRegularExpressionMatch subMatch = subMatchIt.next();
            const QString variableName = subMatch.captured(1);

            // Variable we care about should already exists -> an expression cannot reference a variable that hasn't been defined
            Variable *u = findVariable(variableName);
            Q_ASSERT(u);

            // Increase reference count for u
            ++u->referenceCount;
            // Insert u as reference for variable v
            v->assignment.referencedVariables.push_back(u);
        }
    };

    for (const QShaderGraph::Statement &statement : graph.createStatements(enabledLayers)) {
        const QShaderNode node = statement.node;
        QByteArray line = node.rule(format).substitution;
        const QVector<QShaderNodePort> ports = node.ports();

        struct VariableReplacement {
            QByteArray placeholder;
            QByteArray variable;
        };

        QVector<VariableReplacement> variableReplacements;

        // Generate temporary variable names vN
        for (const QShaderNodePort &port : ports) {
            const QString portName = port.name;
            const QShaderNodePort::Direction portDirection = port.direction;
            const bool isInput = port.direction == QShaderNodePort::Input;

            const int portIndex = statement.portIndex(portDirection, portName);

            Q_ASSERT(portIndex >= 0);

            const int variableIndex = isInput ? statement.inputs.at(portIndex)
                                              : statement.outputs.at(portIndex);
            if (variableIndex < 0)
                continue;

            VariableReplacement replacement;
            replacement.placeholder = QByteArrayLiteral("$") + portName.toUtf8();
            replacement.variable = QByteArrayLiteral("v") + QByteArray::number(variableIndex);

            variableReplacements.append(std::move(replacement));
        }

        int begin = 0;
        while ((begin = line.indexOf('$', begin)) != -1) {
            int end = begin + 1;
            char endChar = line.at(end);
            const int size = line.size();
            while (end < size && (std::isalnum(endChar) || endChar == '_')) {
                ++end;
                endChar = line.at(end);
            }

            const int placeholderLength = end - begin;

            const QByteArray variableName = line.mid(begin, placeholderLength);
            const auto replacementIt = std::find_if(variableReplacements.cbegin(), variableReplacements.cend(),
                                              [&variableName](const VariableReplacement &replacement) {
                return variableName == replacement.placeholder;
            });

            if (replacementIt != variableReplacements.cend()) {
                line.replace(begin, placeholderLength, replacementIt->variable);
                begin += replacementIt->variable.length();
            } else {
                begin = end;
            }
        }

        // Substitute variable names by generated vN variable names
        const QByteArray substitutionedLine = replaceParameters(line, node, format);

        QRegularExpressionMatchIterator matches;

        switch (node.type()) {
        case QShaderNode::Input:
        case QShaderNode::Output:
            matches = statementRegExp.globalMatch(QString::fromUtf8(substitutionedLine));
            break;
        case QShaderNode::Function:
            matches = temporaryVariableToAssignmentRegExp.globalMatch(QString::fromUtf8(substitutionedLine));
            break;
        case QShaderNode::Invalid:
            break;
        }

        while (matches.hasNext()) {
            QRegularExpressionMatch match = matches.next();

            Variable *v = nullptr;

            switch (node.type()) {
            // Record name of temporary variable that possibly references a global input
            // We will replace the temporary variables by the matching global variables later
            case QShaderNode::Input: {
                const QString localVariable = match.captured(1);
                const QString globalVariable = match.captured(2);

                v = createVariable();
                v->name = localVariable;
                v->type = Variable::GlobalInput;

                Assignment assignment;
                assignment.expression = globalVariable;
                v->assignment = assignment;
                break;
            }

            case QShaderNode::Function: {
                const QString localVariableDeclaration = match.captured(1);
                const QString localVariableName = match.captured(2);
                const QString assignmentContent = match.captured(3);

                // Add new variable -> it cannot exist already
                v = createVariable();
                v->name = localVariableName;
                v->declaration = localVariableDeclaration;
                v->assignment.expression = assignmentContent;

                // Find variables that may be referenced in the assignment
                gatherTemporaryVariablesFromAssignment(v, assignmentContent);
                break;
            }

            case QShaderNode::Output: {
                const QString outputDeclaration = match.captured(1);
                const QString assignmentContent = match.captured(2);

                v = createVariable();
                v->name = outputDeclaration;
                v->declaration = outputDeclaration;
                v->type = Variable::Output;

                Assignment assignment;
                assignment.expression = assignmentContent;
                v->assignment = assignment;

                // Find variables that may be referenced in the assignment
                gatherTemporaryVariablesFromAssignment(v, assignmentContent);
                break;
            }
            case QShaderNode::Invalid:
                break;
            }

            LineContent lineContent;
            lineContent.rawContent = QByteArray(QByteArrayLiteral("    ") + substitutionedLine);
            lineContent.var = v;
            lines << lineContent;
        }
    }

    // Go through all lines
    // Perform substitution of line with temporary variables substitution
    for (LineContent &lineContent : lines) {
        Variable *v = lineContent.var;
        qCDebug(ShaderGenerator) << lineContent.rawContent;
        if (v != nullptr) {
            Variable::substitute(v);

            qCDebug(ShaderGenerator) << "Line " << lineContent.rawContent << "is assigned to temporary" << v->name;

            // Check number of occurrences a temporary variable is referenced
            if (v->referenceCount == 1 || v->type == Variable::GlobalInput) {
                // If it is referenced only once, no point in creating a temporary
                // Clear content for current line
                lineContent.rawContent.clear();
                // We assume expression that were referencing vN will have vN properly substituted
            } else {
                lineContent.rawContent = QStringLiteral("    %1 = %2;").arg(v->declaration)
                                                                       .arg(v->assignment.expression)
                                                                       .toUtf8();
            }

            qCDebug(ShaderGenerator) << "Updated Line is " << lineContent.rawContent;
        }
    }

    // Go throug all lines and insert content
    for (const LineContent &lineContent : qAsConst(lines)) {
        if (!lineContent.rawContent.isEmpty()) {
            code << lineContent.rawContent;
        }
    }

    code << QByteArrayLiteral("}");
    code << QByteArray();

    return code.join('\n');
}

QT_END_NAMESPACE
