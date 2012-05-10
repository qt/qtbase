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
  cppcodeparser.h
*/

#ifndef CPPCODEPARSER_H
#define CPPCODEPARSER_H

#include <qregexp.h>

#include "codeparser.h"

QT_BEGIN_NAMESPACE

class ClassNode;
class CodeChunk;
class CppCodeParserPrivate;
class FunctionNode;
class InnerNode;
class Tokenizer;

class CppCodeParser : public CodeParser
{
public:
    CppCodeParser();
    ~CppCodeParser();

    virtual void initializeParser(const Config& config);
    virtual void terminateParser();
    virtual QString language();
    virtual QStringList headerFileNameFilter();
    virtual QStringList sourceFileNameFilter();
    virtual void parseHeaderFile(const Location& location,
                                 const QString& filePath,
                                 Tree *tree);
    virtual void parseSourceFile(const Location& location,
                                 const QString& filePath,
                                 Tree *tree);
    virtual void doneParsingHeaderFiles(Tree *tree);
    virtual void doneParsingSourceFiles(Tree *tree);

    const FunctionNode *findFunctionNode(const QString& synopsis,
                                         Tree *tree,
                                         Node *relative = 0,
                                         bool fuzzy = false);

protected:
    virtual QSet<QString> topicCommands();
    virtual Node *processTopicCommand(const Doc& doc,
                                      const QString& command,
                                      const ArgLocPair& arg);
    virtual Node *processTopicCommandGroup(const QString& command, const ArgList& args);
    bool splitQmlPropertyArg(const QString& arg,
                             QString& type,
                             QString& module,
                             QString& element,
                             QString& name);
    bool splitQmlMethodArg(const QString& arg,
                           QString& type,
                           QString& module,
                           QString& element);
    virtual QSet<QString> otherMetaCommands();
    virtual void processOtherMetaCommand(const Doc& doc,
                                         const QString& command,
                                         const ArgLocPair& argLocPair,
                                         Node *node);
    void processOtherMetaCommands(const Doc& doc, Node *node);

private:
    void reset(Tree *tree);
    void readToken();
    const Location& location();
    QString previousLexeme();
    QString lexeme();
    bool match(int target);
    bool skipTo(int target);
    bool matchCompat();
    bool matchModuleQualifier(QString& name);
    bool matchTemplateAngles(CodeChunk *type = 0);
    bool matchTemplateHeader();
    bool matchDataType(CodeChunk *type, QString *var = 0);
    bool matchParameter(FunctionNode *func);
    bool matchFunctionDecl(InnerNode *parent,
                           QStringList *parentPathPtr = 0,
                           FunctionNode **funcPtr = 0,
                           const QString &templateStuff = QString(),
                           Node::Type type = Node::Function,
                           bool attached = false);
    bool matchBaseSpecifier(ClassNode *classe, bool isClass);
    bool matchBaseList(ClassNode *classe, bool isClass);
    bool matchClassDecl(InnerNode *parent,
                        const QString &templateStuff = QString());
    bool matchNamespaceDecl(InnerNode *parent);
    bool matchUsingDecl();
    bool matchEnumItem(InnerNode *parent, EnumNode *enume);
    bool matchEnumDecl(InnerNode *parent);
    bool matchTypedefDecl(InnerNode *parent);
    bool matchProperty(InnerNode *parent);
    bool matchDeclList(InnerNode *parent);
    bool matchDocsAndStuff();
    bool makeFunctionNode(const QString &synopsis,
                          QStringList *parentPathPtr,
                          FunctionNode **funcPtr,
                          InnerNode *root = 0,
                          Node::Type type = Node::Function,
                          bool attached = false);
    FunctionNode* makeFunctionNode(const Doc& doc,
                                   const QString& sig,
                                   InnerNode* parent,
                                   Node::Type type,
                                   bool attached,
                                   QString qdoctag);
    void parseQiteratorDotH(const Location &location, const QString &filePath);
    void instantiateIteratorMacro(const QString &container,
                                  const QString &includeFile,
                                  const QString &macroDef,
                                  Tree *tree);
    void createExampleFileNodes(FakeNode *fake);

    QMap<QString, Node::Type> nodeTypeMap;
    Tree* tree_;
    Tokenizer *tokenizer;
    int tok;
    Node::Access access;
    FunctionNode::Metaness metaness;
    QString moduleName;
    QStringList lastPath;
    QRegExp varComment;
    QRegExp sep;

    QString sequentialIteratorDefinition;
    QString mutableSequentialIteratorDefinition;
    QString associativeIteratorDefinition;
    QString mutableAssociativeIteratorDefinition;
    QSet<QString> activeNamespaces_;
    QMap<QString, QString> sequentialIteratorClasses;
    QMap<QString, QString> mutableSequentialIteratorClasses;
    QMap<QString, QString> associativeIteratorClasses;
    QMap<QString, QString> mutableAssociativeIteratorClasses;

    static QStringList exampleFiles;
    static QStringList exampleDirs;
    QString exampleNameFilter;
    QString exampleImageFilter;
};

QT_END_NAMESPACE

#endif
