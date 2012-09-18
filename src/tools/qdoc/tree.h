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
  tree.h
*/

#ifndef TREE_H
#define TREE_H

#include "node.h"
#include <qdom.h>
#include <qxmlstream.h>

QT_BEGIN_NAMESPACE

class Generator;
class QStringList;
class TreePrivate;
class QDocDatabase;

class Tree
{
 private:
    friend class QDocDatabase;

    enum FindFlag { SearchBaseClasses = 0x1,
                    SearchEnumValues = 0x2,
                    NonFunction = 0x4 };

    Tree(QDocDatabase* qdb);
    ~Tree();

    EnumNode* findEnumNode(const QStringList& path, Node* start = 0);
    ClassNode* findClassNode(const QStringList& path, Node* start = 0);
    QmlClassNode* findQmlTypeNode(const QStringList& path);
    NamespaceNode* findNamespaceNode(const QStringList& path);
    DocNode* findGroupNode(const QStringList& path, Node* start = 0);
    DocNode* findQmlModuleNode(const QStringList& path, Node* start = 0);

    Node* findNodeByNameAndType(const QStringList& path,
                                Node::Type type,
                                Node::SubType subtype,
                                Node* start,
                                bool acceptCollision = false);

    Node* findNodeRecursive(const QStringList& path,
                            int pathIndex,
                            Node* start,
                            Node::Type type,
                            Node::SubType subtype,
                            bool acceptCollision = false);

    const Node* findNode(const QStringList &path,
                         const Node* relative = 0,
                         int findFlags = 0,
                         const Node* self=0) const;

    const Node* findNode(const QStringList& path,
                         const Node* start,
                         int findFlags,
                         const Node* self,
                         bool qml) const;

    NameCollisionNode* checkForCollision(const QString& name) const;
    NameCollisionNode* findCollisionNode(const QString& name) const;
    FunctionNode *findFunctionNode(const QStringList &path,
                                   Node *relative = 0,
                                   int findFlags = 0);
    FunctionNode *findFunctionNode(const QStringList &parentPath,
                                   const FunctionNode *clone,
                                   Node *relative = 0,
                                   int findFlags = 0);
    void addBaseClass(ClassNode *subclass,
                      Node::Access access,
                      const QStringList &basePath,
                      const QString &dataTypeWithTemplateArgs,
                      InnerNode *parent);
    void addPropertyFunction(PropertyNode *property,
                             const QString &funcName,
                             PropertyNode::FunctionRole funcRole);
    void addToGroup(Node *node, const QString &group);
    void addToPublicGroup(Node *node, const QString &group);
    void addToQmlModule(Node* node);
    const NodeMultiMap& groups() const;
    QMultiMap<QString,QString> publicGroups() const;
    void resolveInheritance(NamespaceNode *rootNode = 0);
    void resolveProperties();
    void resolveGroups();
    void resolveTargets(InnerNode* root);
    void resolveCppToQmlLinks();
    void fixInheritance(NamespaceNode *rootNode = 0);
    void setVersion(const QString &version) { vers = version; }
    NamespaceNode *root() { return &root_; }
    QString version() const { return vers; }

    const FunctionNode *findFunctionNode(const QStringList &path,
                                         const Node *relative = 0,
                                         int findFlags = 0) const;
    const FunctionNode *findFunctionNode(const QStringList &parentPath,
                                         const FunctionNode *clone,
                                         const Node *relative = 0,
                                         int findFlags = 0) const;
    const DocNode *findDocNodeByTitle(const QString &title, const Node* relative = 0) const;
    const Node *findUnambiguousTarget(const QString &target, Atom *&atom, const Node* relative) const;
    Atom *findTarget(const QString &target, const Node *node) const;
    const NamespaceNode *root() const { return &root_; }
    void readIndexes(const QStringList &indexFiles);
    bool generateIndexSection(QXmlStreamWriter& writer, Node* node, bool generateInternalNodes = false);
    void generateIndexSections(QXmlStreamWriter& writer, Node* node, bool generateInternalNodes = false);
    void generateIndex(const QString &fileName,
                       const QString &url,
                       const QString &title,
                       Generator* g,
                       bool generateInternalNodes = false);
    void generateTagFileCompounds(QXmlStreamWriter &writer,
                                  const InnerNode *inner);
    void generateTagFileMembers(QXmlStreamWriter &writer,
                                const InnerNode *inner);
    void generateTagFile(const QString &fileName);
    void addExternalLink(const QString &url, const Node *relative);

    void resolveInheritance(int pass, ClassNode *classe);
    FunctionNode *findVirtualFunctionInBaseClasses(ClassNode *classe,
                                                   FunctionNode *clone);
    void fixPropertyUsingBaseClasses(ClassNode *classe, PropertyNode *property);
    NodeList allBaseClasses(const ClassNode *classe) const;
    void readIndexFile(const QString &path);
    void readIndexSection(const QDomElement &element, InnerNode *parent,
                          const QString &indexUrl);
    QString readIndexText(const QDomElement &element);
    void resolveIndex();

private:
    QDocDatabase* qdb_;
    NamespaceNode root_;
    QString vers;
    Generator* gen_;
    TreePrivate *priv;
};

QT_END_NAMESPACE

#endif
