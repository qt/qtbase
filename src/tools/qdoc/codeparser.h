/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  codeparser.h
*/

#ifndef CODEPARSER_H
#define CODEPARSER_H

#include <QSet>

#include "node.h"

QT_BEGIN_NAMESPACE

class Config;
class Node;
class QString;
class Tree;

class CodeParser
{
public:
    CodeParser();
    virtual ~CodeParser();

    virtual void initializeParser(const Config& config);
    virtual void terminateParser();
    virtual QString language() = 0;
    virtual QStringList headerFileNameFilter();
    virtual QStringList sourceFileNameFilter() = 0;
    virtual void parseHeaderFile(const Location& location,
                                 const QString& filePath, Tree *tree);
    virtual void parseSourceFile(const Location& location,
                                 const QString& filePath, Tree *tree) = 0;
    virtual void doneParsingHeaderFiles(Tree *tree);
    virtual void doneParsingSourceFiles(Tree *tree) = 0;

    void createOutputSubdirectory(const Location& location, const QString& filePath);

    static void initialize(const Config& config);
    static void terminate();
    static CodeParser *parserForLanguage(const QString& language);
    static CodeParser *parserForHeaderFile(const QString &filePath);
    static CodeParser *parserForSourceFile(const QString &filePath);
    static const QString titleFromName(const QString& name);
    static void setLink(Node* node, Node::LinkType linkType, const QString& arg);
    static const QString& currentOutputSubdirectory() { return currentSubDir_; }

protected:
    QSet<QString> commonMetaCommands();
    void processCommonMetaCommand(const Location& location,
                                  const QString& command, const QString& arg,
                                  Node *node, Tree *tree);
    static void extractPageLinkAndDesc(const QString& arg,
                                       QString* link,
                                       QString* desc);

private:
    static QString currentSubDir_;
    static QList<CodeParser *> parsers;
    static bool showInternal;
    static QMap<QString,QString> nameToTitle;
};

QT_END_NAMESPACE

#endif
