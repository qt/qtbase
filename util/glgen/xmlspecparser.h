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
