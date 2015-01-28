/***************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB)
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

#include "xmlspecparser.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QXmlStreamReader>

#ifdef SPECPARSER_DEBUG
#define qXmlSpecParserDebug qDebug
#else
#define qXmlSpecParserDebug QT_NO_QDEBUG_MACRO
#endif

bool XmlSpecParser::parse()
{
    // Open up a stream on the actual OpenGL function spec file
    QFile file(specFileName());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open spec file:" << specFileName() << "Aborting";
        return false;
    }

    QXmlStreamReader stream(&file);

    // Extract the info that we need
    parseFunctions(stream);
    return true;
}

void XmlSpecParser::parseParam(QXmlStreamReader &stream, Function &func)
{
    Argument arg;
    arg.type = QString();

    while (!stream.isEndDocument()) {
        stream.readNext();

        if (stream.isStartElement()) {
            QString tag = stream.name().toString();

            if (tag == "ptype") {
                if (stream.readNext() == QXmlStreamReader::Characters)
                    arg.type.append(stream.text().toString());
            }

            else if (tag == "name") {
                if (stream.readNext() == QXmlStreamReader::Characters)
                    arg.name = stream.text().toString().trimmed();
            }
        } else if (stream.isCharacters()) {
            arg.type.append(stream.text().toString());
        } else if (stream.isEndElement()) {
            QString tag = stream.name().toString();

            if (tag == "param") {
                // compatibility with old spec
                QRegularExpression typeRegExp("(const )?(.+)(?<!\\*)((?:(?!\\*$)\\*)*)(\\*)?");

                // remove extra whitespace
                arg.type = arg.type.trimmed();

                // set default
                arg.direction = Argument::In;
                arg.mode = Argument::Value;

                QRegularExpressionMatch exp = typeRegExp.match(arg.type);
                if (exp.hasMatch()) {
                    if (!exp.captured(4).isEmpty()) {
                        arg.mode = Argument::Reference;

                        if (exp.captured(1).isEmpty())
                            arg.direction = Argument::Out;
                    }

                    arg.type = exp.captured(2) + exp.captured(3);
                }

                break;
            }
        }
    }

    // remove any excess whitespace
    arg.type = arg.type.trimmed();
    arg.name = arg.name.trimmed();

    // maybe some checks?
    func.arguments.append(arg);
}

void XmlSpecParser::parseCommand(QXmlStreamReader &stream)
{
    Function func;

    while (!stream.isEndDocument()) {
        stream.readNext();

        if (stream.isStartElement()) {
            QString tag = stream.name().toString();

            if (tag == "proto") {
                while (!stream.isEndDocument()) {
                    stream.readNext();
                    if (stream.isStartElement() && (stream.name().toString() == "name")) {
                        if (stream.readNext() == QXmlStreamReader::Characters)
                            func.name = stream.text().toString();
                    } else if (stream.isCharacters()) {
                        func.returnType.append(stream.text().toString());
                    } else if (stream.isEndElement() && (stream.name().toString() == "proto")) {
                        break;
                    }
                }
            }

            if (tag == "param")
                parseParam(stream, func);
        }

        else if (stream.isEndElement()) {
            QString tag = stream.name().toString();

            if (tag == "command")
                break;
        }
    }

    // maybe checks?
    func.returnType = func.returnType.trimmed();

    // for compatibility with old spec
    if (func.name.startsWith("gl"))
        func.name.remove(0, 2);

    m_functionList.insert(func.name, func);
}

void XmlSpecParser::parseCommands(QXmlStreamReader &stream)
{
    while (!stream.isEndDocument()) {
        stream.readNext();

        if (stream.isStartElement()) {
            QString tag = stream.name().toString();

            if (tag == "command")
                parseCommand(stream);
        }

        else if (stream.isEndElement()) {
            QString tag = stream.name().toString();

            if (tag == "commands")
                break;
        }
    }
}

void XmlSpecParser::parseRequire(QXmlStreamReader &stream, FunctionList &funcs)
{
    while (!stream.isEndDocument()) {
        stream.readNext();

        if (stream.isStartElement()) {
            QString tag = stream.name().toString();

            if (tag == "command") {
                QString func = stream.attributes().value("name").toString();

                // for compatibility with old spec
                if (func.startsWith("gl"))
                    func.remove(0, 2);

                funcs.append(m_functionList[func]);
            }
        } else if (stream.isEndElement()) {
            QString tag = stream.name().toString();

            if (tag == "require")
                break;
        }
    }
}

void XmlSpecParser::parseRemoveCore(QXmlStreamReader &stream)
{
    while (!stream.isEndDocument()) {
        stream.readNext();

        if (stream.isStartElement()) {
            QString tag = stream.name().toString();

            if (tag == "command") {
                QString func = stream.attributes().value("name").toString();

                // for compatibility with old spec
                if (func.startsWith("gl"))
                    func.remove(0, 2);

                // go through list of version and mark as incompatible
                Q_FOREACH (const Version& v, m_versions) {
                    // Combine version and profile for this subset of functions
                    VersionProfile version;
                    version.version = v;
                    version.profile = VersionProfile::CoreProfile;

                    // Fetch the functions and add to collection for this class
                    Q_FOREACH (const Function& f, m_functions.values(version)) {
                        if (f.name == func) {
                            VersionProfile newVersion;
                            newVersion.version = v;
                            newVersion.profile = VersionProfile::CompatibilityProfile;

                            m_functions.insert(newVersion, f);
                            m_functions.remove(version, f);
                        }
                    }
                }
            }
        } else if (stream.isEndElement()) {
            QString tag = stream.name().toString();

            if (tag == "remove")
                break;
        }
    }
}

void XmlSpecParser::parseFeature(QXmlStreamReader &stream)
{
    QRegularExpression versionRegExp("(\\d).(\\d)");
    QXmlStreamAttributes attributes = stream.attributes();

    QRegularExpressionMatch match = versionRegExp.match(attributes.value("number").toString());

    if (!match.hasMatch()) {
        qWarning() << "Malformed version indicator";
        return;
    }

    if (attributes.value("api").toString() != "gl")
        return;

    int majorVersion = match.captured(1).toInt();
    int minorVersion = match.captured(2).toInt();

    Version v;
    VersionProfile core, compat;

    v.major = majorVersion;
    v.minor = minorVersion;
    core.version = compat.version = v;
    core.profile = VersionProfile::CoreProfile;
    compat.profile = VersionProfile::CompatibilityProfile;

    while (!stream.isEndDocument()) {
        stream.readNext();

        if (stream.isStartElement()) {
            QString tag = stream.name().toString();

            if (tag == "require") {
                FunctionList funcs;

                if (stream.attributes().value("profile").toString() == "compatibility") {
                    parseRequire(stream, funcs);
                    Q_FOREACH (const Function& f, funcs) {
                        m_functions.insert(compat, f);
                    }
                } else {
                    parseRequire(stream, funcs);
                    Q_FOREACH (const Function& f, funcs) {
                        m_functions.insert(core, f);
                    }
                }
            } else if (tag == "remove") {
                if (stream.attributes().value("profile").toString() == "core")
                    parseRemoveCore(stream);
            }
        } else if (stream.isEndElement()) {
            QString tag = stream.name().toString();

            if (tag == "feature")
                break;
        }
    }

    m_versions.append(v);
}

void XmlSpecParser::parseExtension(QXmlStreamReader &stream)
{
    QXmlStreamAttributes attributes = stream.attributes();
    QString name = attributes.value("name").toString();

    while (!stream.isEndDocument()) {
        stream.readNext();

        if (stream.isStartElement()) {
            QString tag = stream.name().toString();

            if (tag == "require") {
                if (stream.attributes().value("profile").toString() == "compatibility") {
                    FunctionList funcs;
                    parseRequire(stream, funcs);

                    Q_FOREACH (const Function& f, funcs) {
                        FunctionProfile fp;
                        fp.function = f;
                        fp.profile = VersionProfile::CompatibilityProfile;
                        m_extensionFunctions.insert(name, fp);
                    }
                } else {
                    FunctionList funcs;
                    parseRequire(stream, funcs);
                    Q_FOREACH (const Function& f, funcs) {
                        FunctionProfile fp;
                        fp.function = f;
                        fp.profile = VersionProfile::CoreProfile;
                        m_extensionFunctions.insert(name, fp);
                    }
                }


            }
        } else if (stream.isEndElement()) {
            QString tag = stream.name().toString();

            if (tag == "extension")
                break;
        }
    }
}

void XmlSpecParser::parseFunctions(QXmlStreamReader &stream)
{
    while (!stream.isEndDocument()) {
        stream.readNext();

        if (stream.isStartElement()) {
            QString tag = stream.name().toString();

            if (tag == "feature") {
                parseFeature(stream);
            } else if (tag == "commands") {
                parseCommands(stream);
            } else if (tag == "extension") {
                parseExtension(stream);
            }
        } else if (stream.isEndElement()) {
            stream.readNext();
        }
    }

    // hack - add GL_ARB_imaging to every version after 1.2 inclusive
    Version versionThreshold;
    versionThreshold.major = 1;
    versionThreshold.minor = 2;
    QList<FunctionProfile> funcs = m_extensionFunctions.values("GL_ARB_imaging");

    VersionProfile vp;
    vp.version = versionThreshold;

    Q_FOREACH (const FunctionProfile& fp, funcs) {
        vp.profile = fp.profile;
        m_functions.insert(vp, fp.function);
    }

    // now we will prune any duplicates
    QSet<QString> funcset;

    Q_FOREACH (const Version& v, m_versions) {
        // check compatibility first
        VersionProfile vp;
        vp.version = v;

        vp.profile = VersionProfile::CompatibilityProfile;

        Q_FOREACH (const Function& f, m_functions.values(vp)) {
            // remove duplicate
            if (funcset.contains(f.name))
                m_functions.remove(vp, f);

            funcset.insert(f.name);
        }

        vp.profile = VersionProfile::CoreProfile;

        Q_FOREACH (const Function& f, m_functions.values(vp)) {

            // remove duplicate
            if (funcset.contains(f.name))
                m_functions.remove(vp, f);

            funcset.insert(f.name);
        }
    }
}
