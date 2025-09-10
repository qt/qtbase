// Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef LEGACYSPECPARSER_H
#define LEGACYSPECPARSER_H

#include "specparser.h"

#include <QStringList>
#include <QVariant>

class QTextStream;

class LegacySpecParser : public SpecParser
{
public:
    virtual QList<Version> versions() const {return m_versions;}

    virtual bool parse();

protected:
    const QMultiHash<VersionProfile, Function> &versionFunctions() const { return m_functions; }
    const QMultiMap<QString, FunctionProfile> &extensionFunctions() const { return m_extensionFunctions; }

private:
    QMap<QString, QString> m_typeMap;
    QMultiHash<VersionProfile, Function> m_functions;

    QList<Version> m_versions;

    // Extension support
    QMultiMap<QString, FunctionProfile> m_extensionFunctions;

    bool parseTypeMap();
    void parseEnums();
    void parseFunctions(QTextStream &stream);
    bool inDeprecationException(const QString &functionName) const;
};

#endif // LEGACYSPECPARSER_H
