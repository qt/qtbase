/***************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: http://www.qt.io/licensing/
**
** This file is part of the utilities of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "legacyspecparser.h"

#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>

#ifdef SPECPARSER_DEBUG
#define qLegacySpecParserDebug qDebug
#else
#define qLegacySpecParserDebug QT_NO_QDEBUG_MACRO
#endif

bool LegacySpecParser::parse()
{
    // Get the mapping form generic types to specific types suitable for use in C-headers
    if (!parseTypeMap())
        return false;

    // Open up a stream on the actual OpenGL function spec file
    QFile file(specFileName());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open spec file:" << specFileName() << "Aborting";
        return false;
    }

    QTextStream stream(&file);

    // Extract the info that we need
    parseFunctions(stream);
    return true;
}

bool LegacySpecParser::parseTypeMap()
{
    QFile file(typeMapFileName());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open type file:" << typeMapFileName() << "Aborting";
        return false;
    }

    QTextStream stream(&file);

    static QRegExp typeMapRegExp("([^,]+)\\W+([^,]+)");

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        if (line.startsWith(QLatin1Char('#')))
            continue;

        if (typeMapRegExp.indexIn(line) != -1) {
            QString key = typeMapRegExp.cap(1).simplified();
            QString value = typeMapRegExp.cap(2).simplified();

            // Special case for void
            if (value == QLatin1String("*"))
                value = QStringLiteral("void");

            m_typeMap.insert(key, value);
            qLegacySpecParserDebug() << "Found type mapping from" << key << "=>" << value;
        }
    }

    return true;
}

void LegacySpecParser::parseEnums()
{
}

void LegacySpecParser::parseFunctions(QTextStream &stream)
{
    static QRegExp functionRegExp("^(\\w+)\\(.*\\)");
    static QRegExp returnRegExp("^\\treturn\\s+(\\S+)");
    static QRegExp argumentRegExp("param\\s+(\\S+)\\s+(\\S+) (\\S+) (\\S+)");
    static QRegExp versionRegExp("^\\tversion\\s+(\\S+)");
    static QRegExp deprecatedRegExp("^\\tdeprecated\\s+(\\S+)");
    static QRegExp categoryRegExp("^\\tcategory\\s+(\\S+)");
    static QRegExp categoryVersionRegExp("VERSION_(\\d)_(\\d)");
    static QRegExp extToCoreVersionRegExp("passthru:\\s/\\*\\sOpenGL\\s(\\d)\\.(\\d)\\s.*\\sextensions:");
    static QRegExp extToCoreRegExp("passthru:\\s/\\*\\s(ARB_\\S*)\\s.*\\*/");

    Function currentFunction;
    VersionProfile currentVersionProfile;
    QString currentCategory;
    bool haveVersionInfo = false;
    bool acceptCurrentFunctionInCore = false;
    bool acceptCurrentFunctionInExtension = false;

    QHash<QString, Version> extensionsNowInCore;
    Version extToCoreCurrentVersion;
    int functionCount = 0;

    QSet<Version> versions;

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.startsWith("#"))
            continue;

        if (functionRegExp.indexIn(line) != -1) {

            if (!currentFunction.name.isEmpty()) {

                // NB - Special handling!
                // Versions 4.2 and 4.3 (and probably newer) add functionality by
                // subsuming extensions such as ARB_texture_storage. However, some extensions
                // also include functions to interact with the EXT_direct_state_access
                // extension. These functions should be added to the DSA extension rather
                // than the core functionality. The core will already contain non-DSA
                // versions of these functions.
                if (acceptCurrentFunctionInCore && currentFunction.name.endsWith(QLatin1String("EXT"))) {
                    acceptCurrentFunctionInCore = false;
                    acceptCurrentFunctionInExtension = true;
                    currentCategory = QStringLiteral("EXT_direct_state_access");
                }

                // Finish off previous function (if any) by inserting it into the core
                // functionality or extension functionality (or both)
                if (acceptCurrentFunctionInCore) {
                    m_functions.insert(currentVersionProfile, currentFunction);
                    versions.insert(currentVersionProfile.version);
                }

                if (acceptCurrentFunctionInExtension) {
                    FunctionProfile fp;
                    fp.profile = currentVersionProfile.profile;
                    fp.function = currentFunction;
                    m_extensionFunctions.insert(currentCategory, fp);
                }
            }

            // Start a new function
            ++functionCount;
            haveVersionInfo = false;
            acceptCurrentFunctionInCore = true;
            acceptCurrentFunctionInExtension = false;
            currentCategory = QString();
            currentFunction = Function();

            // We assume a core function unless we find a deprecated flag (see below)
            currentVersionProfile = VersionProfile();
            currentVersionProfile.profile = VersionProfile::CoreProfile;

            // Extract the function name
            QString functionName = functionRegExp.cap(1);
            currentFunction.name = functionName;
            qLegacySpecParserDebug() << "Found function:" << functionName;

        } else if (argumentRegExp.indexIn(line) != -1) {
            // Extract info about this function argument
            Argument arg;
            arg.name = argumentRegExp.cap(1);

            QString type = argumentRegExp.cap(2); // Lookup in type map
            arg.type = m_typeMap.value(type);

            QString direction = argumentRegExp.cap(3);
            if (direction == QLatin1String("in")) {
                arg.direction = Argument::In;
            } else if (direction == QLatin1String("out")) {
                arg.direction = Argument::Out;
            } else {
                qWarning() << "Invalid argument direction found:" << direction;
                acceptCurrentFunctionInCore = false;
            }

            QString mode = argumentRegExp.cap(4);
            if (mode == QLatin1String("value")) {
                arg.mode = Argument::Value;
            } else if (mode == QLatin1String("array")) {
                arg.mode = Argument::Array;
            } else if (mode == QLatin1String("reference")) {
                arg.mode = Argument::Reference;
            } else {
                qWarning() << "Invalid argument mode found:" << mode;
                acceptCurrentFunctionInCore = false;
            }

            qLegacySpecParserDebug() << "    argument:" << arg.type << arg.name;
            currentFunction.arguments.append(arg);

        } else if (returnRegExp.indexIn(line) != -1) {
            // Lookup the return type from the typemap
            QString returnTypeKey = returnRegExp.cap(1).simplified();
            if (!m_typeMap.contains(returnTypeKey)) {
                qWarning() << "Unknown return type found:" << returnTypeKey;
                acceptCurrentFunctionInCore = false;
            }
            QString returnType = m_typeMap.value(returnTypeKey);
            qLegacySpecParserDebug() << "    return type:" << returnType;
            currentFunction.returnType = returnType;

        } else if (versionRegExp.indexIn(line) != -1 && !haveVersionInfo) { // Only use version line if no other source
            // Extract the OpenGL version in which this function was introduced
            QString version = versionRegExp.cap(1);
            qLegacySpecParserDebug() << "    version:" << version;
            QStringList parts = version.split(QLatin1Char('.'));
            if (parts.size() != 2) {
                qWarning() << "Found invalid version number";
                continue;
            }
            int majorVersion = parts.first().toInt();
            int minorVersion = parts.last().toInt();
            Version v;
            v.major = majorVersion;
            v.minor = minorVersion;
            currentVersionProfile.version = v;

        } else if (deprecatedRegExp.indexIn(line) != -1) {
            // Extract the OpenGL version in which this function was deprecated.
            // If it is OpenGL 3.1 then it must be a compatibility profile function
            QString deprecatedVersion = deprecatedRegExp.cap(1).simplified();
            if (deprecatedVersion == QLatin1String("3.1") && !inDeprecationException(currentFunction.name))
                currentVersionProfile.profile = VersionProfile::CompatibilityProfile;

        } else if (categoryRegExp.indexIn(line) != -1) {
            // Extract the category for this function
            QString category = categoryRegExp.cap(1).simplified();
            qLegacySpecParserDebug() << "    category:" << category;

            if (categoryVersionRegExp.indexIn(category) != -1) {
                // Use the version info in the category in preference to the version
                // entry as this is more applicable and consistent
                int majorVersion = categoryVersionRegExp.cap(1).toInt();
                int minorVersion = categoryVersionRegExp.cap(2).toInt();

                Version v;
                v.major = majorVersion;
                v.minor = minorVersion;
                currentVersionProfile.version = v;
                haveVersionInfo = true;

            } else {
                // Make a note of the extension name and tag this function as being part of an extension
                qLegacySpecParserDebug() << "Found category =" << category;
                currentCategory = category;
                acceptCurrentFunctionInExtension = true;

                // See if this category (extension) is in our set of extensions that
                // have now been folded into the core feature set
                if (extensionsNowInCore.contains(category)) {
                    currentVersionProfile.version = extensionsNowInCore.value(category);
                    haveVersionInfo = true;
                } else {
                    acceptCurrentFunctionInCore = false;
                }
            }

        } else if (extToCoreVersionRegExp.indexIn(line) != -1) {
            qLegacySpecParserDebug() << line;
            int majorVersion = extToCoreVersionRegExp.cap(1).toInt();
            int minorVersion = extToCoreVersionRegExp.cap(2).toInt();
            extToCoreCurrentVersion.major = majorVersion;
            extToCoreCurrentVersion.minor = minorVersion;

        } else if (extToCoreRegExp.indexIn(line) != -1) {
            QString extension = extToCoreRegExp.cap(1);
            extensionsNowInCore.insert(extension, extToCoreCurrentVersion);
        }
    }

    m_versions = versions.toList();
    qSort(m_versions);
}

bool LegacySpecParser::inDeprecationException(const QString &functionName) const
{
    return functionName == QLatin1String("TexImage3D");
}
