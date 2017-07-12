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

#include "qshadernodesloader_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

QShaderNodesLoader::QShaderNodesLoader() Q_DECL_NOTHROW
    : m_status(Null),
      m_device(nullptr)
{
}

QShaderNodesLoader::Status QShaderNodesLoader::status() const Q_DECL_NOTHROW
{
    return m_status;
}

QHash<QString, QShaderNode> QShaderNodesLoader::nodes() const Q_DECL_NOTHROW
{
    return m_nodes;
}

QIODevice *QShaderNodesLoader::device() const Q_DECL_NOTHROW
{
    return m_device;
}

void QShaderNodesLoader::setDevice(QIODevice *device) Q_DECL_NOTHROW
{
    m_device = device;
    m_nodes.clear();
    m_status = !m_device ? Null
             : (m_device->openMode() & QIODevice::ReadOnly) ? Waiting
             : Error;
}

void QShaderNodesLoader::load()
{
    if (m_status == Error)
        return;

    auto error = QJsonParseError();
    const auto document = QJsonDocument::fromJson(m_device->readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Invalid JSON document:" << error.errorString();
        m_status = Error;
        return;
    }

    if (document.isEmpty() || !document.isObject()) {
        qWarning() << "Invalid JSON document, root should be an object";
        m_status = Error;
        return;
    }

    const auto root = document.object();

    bool hasError = false;

    for (const auto &property : root.keys()) {
        const auto nodeValue = root.value(property);
        if (!nodeValue.isObject()) {
            qWarning() << "Invalid node found";
            hasError = true;
            break;
        }

        const auto nodeObject = nodeValue.toObject();

        auto node = QShaderNode();

        const auto inputsValue = nodeObject.value(QStringLiteral("inputs"));
        if (inputsValue.isArray()) {
            const auto inputsArray = inputsValue.toArray();
            for (const auto &inputValue : inputsArray) {
                if (!inputValue.isString()) {
                    qWarning() << "Non-string value in inputs";
                    hasError = true;
                    break;
                }

                auto input = QShaderNodePort();
                input.direction = QShaderNodePort::Input;
                input.name = inputValue.toString();
                node.addPort(input);
            }
        }

        const auto outputsValue = nodeObject.value(QStringLiteral("outputs"));
        if (outputsValue.isArray()) {
            const auto outputsArray = outputsValue.toArray();
            for (const auto &outputValue : outputsArray) {
                if (!outputValue.isString()) {
                    qWarning() << "Non-string value in outputs";
                    hasError = true;
                    break;
                }

                auto output = QShaderNodePort();
                output.direction = QShaderNodePort::Output;
                output.name = outputValue.toString();
                node.addPort(output);
            }
        }

        const auto parametersValue = nodeObject.value(QStringLiteral("parameters"));
        if (parametersValue.isObject()) {
            const auto parametersObject = parametersValue.toObject();
            for (const auto &parameterName : parametersObject.keys()) {
                const auto parameterValue = parametersObject.value(parameterName);
                if (parameterValue.isObject()) {
                    const auto parameterObject = parameterValue.toObject();
                    const auto type = parameterObject.value(QStringLiteral("type")).toString();
                    const auto typeId = QMetaType::type(type.toUtf8());

                    const auto value = parameterObject.value(QStringLiteral("value")).toString();
                    auto variant = QVariant(value);

                    if (QMetaType::typeFlags(typeId) & QMetaType::IsEnumeration) {
                        const auto metaObject = QMetaType::metaObjectForType(typeId);
                        const auto className = metaObject->className();
                        const auto enumName = type.mid(static_cast<int>(qstrlen(className)) + 2).toUtf8();
                        const auto metaEnum = metaObject->enumerator(metaObject->indexOfEnumerator(enumName));
                        const auto enumValue = metaEnum.keyToValue(value.toUtf8());
                        variant = QVariant(enumValue);
                        variant.convert(typeId);
                    } else {
                        variant.convert(typeId);
                    }
                    node.setParameter(parameterName, variant);
                } else {
                    node.setParameter(parameterName, parameterValue.toVariant());
                }
            }
        }

        const auto rulesValue = nodeObject.value(QStringLiteral("rules"));
        if (rulesValue.isArray()) {
            const auto rulesArray = rulesValue.toArray();
            for (const auto &ruleValue : rulesArray) {
                if (!ruleValue.isObject()) {
                    qWarning() << "Rules should be objects";
                    hasError = true;
                    break;
                }

                const auto ruleObject = ruleValue.toObject();

                const auto formatValue = ruleObject.value(QStringLiteral("format"));
                if (!formatValue.isObject()) {
                    qWarning() << "Format is mandatory in rules and should be an object";
                    hasError = true;
                    break;
                }

                const auto formatObject = formatValue.toObject();
                auto format = QShaderFormat();

                const auto apiValue = formatObject.value(QStringLiteral("api"));
                if (!apiValue.isString()) {
                    qWarning() << "Format API must be a string";
                    hasError = true;
                    break;
                }

                const auto api = apiValue.toString();
                format.setApi(api == QStringLiteral("OpenGLES") ? QShaderFormat::OpenGLES
                            : api == QStringLiteral("OpenGLNoProfile") ? QShaderFormat::OpenGLNoProfile
                            : api == QStringLiteral("OpenGLCoreProfile") ? QShaderFormat::OpenGLCoreProfile
                            : api == QStringLiteral("OpenGLCompatibilityProfile") ? QShaderFormat::OpenGLCompatibilityProfile
                            : QShaderFormat::NoApi);
                if (format.api() == QShaderFormat::NoApi) {
                    qWarning() << "Format API must be one of: OpenGLES, OpenGLNoProfile, OpenGLCoreProfile or OpenGLCompatibilityProfile";
                    hasError = true;
                    break;
                }

                const auto majorValue = formatObject.value(QStringLiteral("major"));
                const auto minorValue = formatObject.value(QStringLiteral("minor"));
                if (!majorValue.isDouble() || !minorValue.isDouble()) {
                    qWarning() << "Format major and minor version must be values";
                    hasError = true;
                    break;
                }
                format.setVersion(QVersionNumber(majorValue.toInt(), minorValue.toInt()));

                const auto extensionsValue = formatObject.value(QStringLiteral("extensions"));
                const auto extensionsArray = extensionsValue.toArray();
                auto extensions = QStringList();
                std::transform(extensionsArray.constBegin(), extensionsArray.constEnd(),
                               std::back_inserter(extensions),
                               [] (const QJsonValue &extensionValue) { return extensionValue.toString(); });
                format.setExtensions(extensions);

                const auto vendor = formatObject.value(QStringLiteral("vendor")).toString();
                format.setVendor(vendor);

                const auto substitutionValue = ruleObject.value(QStringLiteral("substitution"));
                if (!substitutionValue.isString()) {
                    qWarning() << "Substitution needs to be a string";
                    hasError = true;
                    break;
                }

                const auto substitution = substitutionValue.toString().toUtf8();

                const auto snippetsValue = ruleObject.value(QStringLiteral("headerSnippets"));
                const auto snippetsArray = snippetsValue.toArray();
                auto snippets = QByteArrayList();
                std::transform(snippetsArray.constBegin(), snippetsArray.constEnd(),
                               std::back_inserter(snippets),
                               [] (const QJsonValue &snippetValue) { return snippetValue.toString().toUtf8(); });

                node.addRule(format, QShaderNode::Rule(substitution, snippets));
            }
        }

        m_nodes.insert(property, node);
    }

    if (hasError) {
        m_status = Error;
        m_nodes.clear();
    } else {
        m_status = Ready;
    }
}

QT_END_NAMESPACE
