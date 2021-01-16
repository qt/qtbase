/***************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utilities of the Qt Toolkit.
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
****************************************************************************/

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
