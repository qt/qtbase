/***************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utilities of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
