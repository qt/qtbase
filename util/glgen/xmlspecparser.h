// Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef XMLSPECPARSER_H
#define XMLSPECPARSER_H

#include "specparser.h"

#include <QStringList>
#include <QVariant>

class QXmlStreamReader;

class XmlSpecParser : public SpecParser
{
public:
    virtual QList<Version> versions() const { return m_versions; }

    virtual bool parse();

protected:
    const QMultiHash<VersionProfile, Function> &versionFunctions() const { return m_functions; }
    const QMultiMap<QString, FunctionProfile> &extensionFunctions() const { return m_extensionFunctions; }

private:
    void parseFunctions(QXmlStreamReader &stream);
    void parseCommands(QXmlStreamReader &stream);
    void parseCommand(QXmlStreamReader &stream);
    void parseParam(QXmlStreamReader &stream, Function &func);
    void parseFeature(QXmlStreamReader &stream);
    void parseExtension(QXmlStreamReader &stream);
    void parseRequire(QXmlStreamReader &stream, FunctionList& profile);
    void parseRemoveCore(QXmlStreamReader &stream);

    QMultiHash<VersionProfile, Function> m_functions;

    QList<Version> m_versions;

    // Extension support
    QMultiMap<QString, FunctionProfile> m_extensionFunctions;

    QMap<QString, Function> m_functionList;
};

#endif // XMLSPECPARSER_H
