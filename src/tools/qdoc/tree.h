/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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

#include <QtCore/qstack.h>
#include "node.h"

QT_BEGIN_NAMESPACE

class QStringList;
class QDocDatabase;

struct TargetRec
{
  public:
    enum Type { Unknown, Target, Keyword, Contents, Class, Function, Page, Subtitle };
    TargetRec() : node_(0), priority_(INT_MAX), type_(Unknown) { }
    bool isEmpty() const { return ref_.isEmpty(); }
    Node* node_;
    QString ref_;
    int priority_;
    Type type_;
};

typedef QMultiMap<QString, TargetRec> TargetMap;
typedef QMultiMap<QString, DocNode*> DocNodeMultiMap;
typedef QMap<QString, QmlClassNode*> QmlTypeMap;
typedef QMultiMap<QString, const ExampleNode*> ExampleNodeMap;

class Tree
{
 private:
    friend class QDocForest;
    friend class QDocDatabase;

    typedef QMap<PropertyNode::FunctionRole, QString> RoleMap;
    typedef QMap<PropertyNode*, RoleMap> PropertyMap;

    Tree(const QString& module, QDocDatabase* qdb);
    ~Tree();

    ClassNode* findClassNode(const QStringList& path, Node* start = 0) const;
    NamespaceNode* findNamespaceNode(const QStringList& path) const;
    FunctionNode* findFunctionNode(const QStringList& parentPath, const FunctionNode* clone);
    const Node* resolveFunctionTarget(const QString& target, const Node* relative);

    Node* findNodeRecursive(const QStringList& path,
                            int pathIndex,
                            const Node* start,
                            Node::Type type,
                            Node::SubType subtype,
                            bool acceptCollision = false) const;
    Node* findNodeRecursive(const QStringList& path,
                            int pathIndex,
                            Node* start,
                            const NodeTypeList& types) const;

    const Node* findNode(const QStringList &path,
                         const Node* relative = 0,
                         int findFlags = 0) const;

    const Node* findNode(const QStringList& path,
                         const Node* start,
                         int findFlags,
                         bool qml) const;

    QmlClassNode* findQmlTypeNode(const QStringList& path);

    Node* findNodeByNameAndType(const QStringList& path,
                                Node::Type type,
                                Node::SubType subtype,
                                bool acceptCollision = false) const;


    InnerNode* findRelatesNode(const QStringList& path);
    NameCollisionNode* checkForCollision(const QString& name);
    NameCollisionNode* findCollisionNode(const QString& name) const;
    QString findTarget(const QString& target, const Node* node) const;
    void insertTarget(const QString& name, TargetRec::Type type, Node* node, int priority);
    void resolveTargets(InnerNode* root);
    const Node* findUnambiguousTarget(const QString& target, QString& ref);
    const DocNode* findDocNodeByTitle(const QString& title) const;

    void addPropertyFunction(PropertyNode *property,
                             const QString &funcName,
                             PropertyNode::FunctionRole funcRole);
    void resolveInheritance(InnerNode* n = 0);
    void resolveInheritanceHelper(int pass, ClassNode* cn);
    void resolveProperties();
    void resolveCppToQmlLinks();
    void fixInheritance(NamespaceNode *rootNode = 0);
    NamespaceNode *root() { return &root_; }

    const FunctionNode *findFunctionNode(const QStringList &path,
                                         const Node *relative = 0,
                                         int findFlags = 0) const;
    const NamespaceNode *root() const { return &root_; }

    FunctionNode *findVirtualFunctionInBaseClasses(ClassNode *classe,
                                                   FunctionNode *clone);
    NodeList allBaseClasses(const ClassNode *classe) const;
    QString refForAtom(const Atom* atom);

    const CNMap& groups() const { return groups_; }
    const CNMap& modules() const { return modules_; }
    const CNMap& qmlModules() const { return qmlModules_; }
    const CNMap& getCollections(Node::Type t) const {
        if (t == Node::Group)
            return groups_;
        if (t == Node::Module)
            return modules_;
        return qmlModules_;
    }

    CollectionNode* getCorrespondingCollection(CollectionNode* cn);

    GroupNode* getGroup(const QString& name);
    ModuleNode* getModule(const QString& name);
    QmlModuleNode* getQmlModule(const QString& name);

    GroupNode* findGroup(const QString& name);
    ModuleNode* findModule(const QString& name);
    QmlModuleNode* findQmlModule(const QString& name);

    GroupNode* addGroup(const QString& name);
    ModuleNode* addModule(const QString& name);
    QmlModuleNode* addQmlModule(const QString& name);

    GroupNode* addToGroup(const QString& name, Node* node);
    ModuleNode* addToModule(const QString& name, Node* node);
    QmlModuleNode* addToQmlModule(const QString& name, Node* node);

    QmlClassNode* lookupQmlType(const QString& name) const { return qmlTypeMap_.value(name); }
    void insertQmlType(const QString& key, QmlClassNode* n);
    void addExampleNode(ExampleNode* n) { exampleNodeMap_.insert(n->title(), n); }
    ExampleNodeMap& exampleNodeMap() { return exampleNodeMap_; }

 public:
    const QString& moduleName() const { return module_; }

private:
    QString module_;
    QDocDatabase* qdb_;
    NamespaceNode root_;
    PropertyMap unresolvedPropertyMap;
    DocNodeMultiMap         docNodesByTitle_;
    TargetMap               nodesByTarget_;
    CNMap                   groups_;
    CNMap                   modules_;
    CNMap                   qmlModules_;
    QmlTypeMap              qmlTypeMap_;
    ExampleNodeMap          exampleNodeMap_;
};

QT_END_NAMESPACE

#endif
