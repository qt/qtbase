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

#include "doc.h"
#include "htmlgenerator.h"
#include "location.h"
#include "node.h"
#include "text.h"
#include "tree.h"
#include "qdocdatabase.h"
#include <limits.h>
#include <qdebug.h>

bool Tree::debug_ = false;

QT_BEGIN_NAMESPACE

/*!
  \class Tree

  This class constructs and maintains a tree of instances of
  the subclasses of Node.

  This class is now private. Only class QDocDatabase has access.
  Please don't change this. If you must access class Tree, do it
  though the pointer to the singleton QDocDatabase.
 */

/*!
  Constructs the singleton tree. \a qdb is the pointer to the
  qdoc database that is constructing the tree. This might not
  be necessary, and it might be removed later.
 */
Tree::Tree(QDocDatabase* qdb)
      : qdb_(qdb), root_(0, QString())
{
}

/*!
  Destroys the singleton Tree.
 */
Tree::~Tree()
{
}

/* API members */

/*!
  Find the C++ class node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only a C++ class node named \a path is
  acceptible. If one is not found, 0 is returned.
 */
ClassNode* Tree::findClassNode(const QStringList& path, Node* start) const
{
    if (!start)
        start = const_cast<NamespaceNode*>(root());
    return static_cast<ClassNode*>(findNodeRecursive(path, 0, start, Node::Class, Node::NoSubType));
}

/*!
  Find the Namespace node named \a path. Begin the search at
  the root of the tree. Only a Namespace node named \a path
  is acceptible. If one is not found, 0 is returned.
 */
NamespaceNode* Tree::findNamespaceNode(const QStringList& path) const
{
    Node* start = const_cast<NamespaceNode*>(root());
    return static_cast<NamespaceNode*>(findNodeRecursive(path, 0, start, Node::Namespace, Node::NoSubType));
}

/*!
  This function first ignores the \a clone node and searches
  for the parent node with \a parentPath. If that search is
  successful, it searches for a child node of the parent that
  matches the \a clone node. If it finds a node that is just
  like the \a clone, it returns a pointer to the found node.

  There should be a way to avoid creating the clone in the
  first place. Investigate when time allows.
 */
FunctionNode* Tree::findFunctionNode(const QStringList& parentPath, const FunctionNode* clone)
{
    const Node* parent = findNamespaceNode(parentPath);
    if (parent == 0)
        parent = findClassNode(parentPath, 0);
    if (parent == 0)
        parent = findNode(parentPath);
    if (parent == 0 || !parent->isInnerNode())
        return 0;
    return ((InnerNode*)parent)->findFunctionNode(clone);
}


/*!
  Find the Qml type node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only a Qml type node named <\a path is
  acceptible. If one is not found, 0 is returned.
 */
QmlClassNode* Tree::findQmlTypeNode(const QStringList& path)
{
    /*
      If the path contains one or two double colons ("::"),
      check first to see if the first two path strings refer
      to a QML element. If they do, path[0] will be the QML
      module identifier, and path[1] will be the QML type.
      If the anser is yes, the reference identifies a QML
      class node.
    */
    if (path.size() >= 2 && !path[0].isEmpty()) {
        QmlClassNode* qcn = qdb_->findQmlType(path[0], path[1]);
        if (qcn)
            return qcn;
    }
    return static_cast<QmlClassNode*>(findNodeRecursive(path, 0, root(), Node::Document, Node::QmlClass));
}

/*!
  First, search for a node with the specified \a name. If a matching
  node is found, if it is a collision node, another collision with
  this name has been found, so return the collision node. If the
  matching node is not a collision node, the first collision for this
  name has been found, so create a NameCollisionNode with the matching
  node as its first child, and return a pointer to the new
  NameCollisionNode. Otherwise return 0.
 */
NameCollisionNode* Tree::checkForCollision(const QString& name) const
{
    Node* n = const_cast<Node*>(findNode(QStringList(name)));
    if (n) {
        if (n->subType() == Node::Collision) {
            NameCollisionNode* ncn = static_cast<NameCollisionNode*>(n);
            return ncn;
        }
        if (n->isInnerNode())
            return new NameCollisionNode(static_cast<InnerNode*>(n));
    }
    return 0;
}

/*!
  This function is like checkForCollision() in that it searches
  for a collision node with the specified \a name. But it doesn't
  create anything. If it finds a match, it returns the pointer.
  Otherwise it returns 0.
 */
NameCollisionNode* Tree::findCollisionNode(const QString& name) const
{
    Node* n = const_cast<Node*>(findNode(QStringList(name)));
    if (n) {
        if (n->subType() == Node::Collision) {
            NameCollisionNode* ncn = static_cast<NameCollisionNode*>(n);
            return ncn;
        }
    }
    return 0;
}

/*!
  This function begins searching the tree at \a relative for
  the \l {FunctionNode} {function node} identified by \a path.
  The \a findFlags are used to restrict the search. If a node
  that matches the \a path is found, it is returned. Otherwise,
  0 is returned. If \a relative is 0, the root of the tree is
  used as the starting point.
 */
const FunctionNode* Tree::findFunctionNode(const QStringList& path,
                                           const Node* relative,
                                           int findFlags) const
{
    if (!relative)
        relative = root();

    /*
      If the path contains two double colons ("::"), check
      first to see if it is a reference to a QML method. If
      it is a reference to a QML method, first look up the
      QML class node in the QML module map.
     */
    if (path.size() == 3 && !path[0].isEmpty()) {
        QmlClassNode* qcn = qdb_->findQmlType(path[0], path[1]);
        if (qcn) {
            return static_cast<const FunctionNode*>(qcn->findFunctionNode(path[2]));
        }
    }

    do {
        const Node* node = relative;
        int i;

        for (i = 0; i < path.size(); ++i) {
            if (node == 0 || !node->isInnerNode())
                break;

            const Node* next;
            if (i == path.size() - 1)
                next = ((InnerNode*) node)->findFunctionNode(path.at(i));
            else
                next = ((InnerNode*) node)->findChildNodeByName(path.at(i));

            if (!next && node->type() == Node::Class && (findFlags & SearchBaseClasses)) {
                NodeList baseClasses = allBaseClasses(static_cast<const ClassNode*>(node));
                foreach (const Node* baseClass, baseClasses) {
                    if (i == path.size() - 1)
                        next = static_cast<const InnerNode*>(baseClass)->findFunctionNode(path.at(i));
                    else
                        next = static_cast<const InnerNode*>(baseClass)->findChildNodeByName(path.at(i));

                    if (next)
                        break;
                }
            }

            node = next;
        }
        if (node && i == path.size() && node->isFunction()) {
            // CppCodeParser::processOtherMetaCommand ensures that reimplemented
            // functions are private.
            const FunctionNode* func = static_cast<const FunctionNode*>(node);
            while (func->access() == Node::Private) {
                const FunctionNode* from = func->reimplementedFrom();
                if (from != 0) {
                    if (from->access() != Node::Private)
                        return from;
                    else
                        func = from;
                }
                else
                    break;
            }
            return func;
        }
        relative = relative->parent();
    } while (relative);

    return 0;
}

static NodeTypeList t;
static const NodeTypeList& relatesTypes()
{
    if (t.isEmpty()) {
        t.reserve(3);
        t.append(NodeTypePair(Node::Class, Node::NoSubType));
        t.append(NodeTypePair(Node::Namespace, Node::NoSubType));
        t.append(NodeTypePair(Node::Document, Node::HeaderFile));
    }
    return t;
}

/*!
  This function searches for the node specified by \a path.
  The matching node can be one of several different types
  including a C++ class, a C++ namespace, or a C++ header
  file.

  I'm not sure if it can be a QML type, but if that is a
  possibility, the code can easily accommodate it.

  If a matching node is found, a pointer to it is returned.
  Otherwise 0 is returned.
 */
InnerNode* Tree::findRelatesNode(const QStringList& path)
{
    Node* n = findNodeRecursive(path, 0, root(), relatesTypes());
    return ((n && n->isInnerNode()) ? static_cast<InnerNode*>(n) : 0);
}

/*!
 */
void Tree::addPropertyFunction(PropertyNode* property,
                               const QString& funcName,
                               PropertyNode::FunctionRole funcRole)
{
    unresolvedPropertyMap[property].insert(funcRole, funcName);
}

/*!
  This function resolves C++ inheritance and reimplementation
  settings for each C++ class node found in the tree beginning
  at \a n. It also calls itself recursively for each C++ class
  node or namespace node it encounters. For each child of \a n
  that is a class node, it calls resolveInheritanceHelper().

  This function does not resolve QML inheritance.
 */
void Tree::resolveInheritance(InnerNode* n)
{
    if (!n)
        n = root();

    for (int pass = 0; pass < 2; pass++) {
        NodeList::ConstIterator c = n->childNodes().constBegin();
        while (c != n->childNodes().constEnd()) {
            if ((*c)->type() == Node::Class) {
                resolveInheritanceHelper(pass, (ClassNode*)*c);
                resolveInheritance((ClassNode*)*c);
            }
            else if ((*c)->type() == Node::Namespace) {
                NamespaceNode* ns = static_cast<NamespaceNode*>(*c);
                resolveInheritance(ns);
            }
            ++c;
        }
    }
}

/*!
  This function is run twice for eachclass node \a cn in the
  tree. First it is run with \a pass set to 0 for each
 class node \a cn. Then it is run with \a pass set to 1 for
  eachclass node \a cn.

  In \a pass 0, all the base classes ofclass node \a cn are
  found and added to the base class list forclass node \a cn.

  In \a pass 1, each child ofclass node \a cn that is a function
  that is reimplemented from one of the base classes is marked
  as being reimplemented from that class.

  Some property node fixing up is also done in \a pass 1.
 */
void Tree::resolveInheritanceHelper(int pass, ClassNode* cn)
{
    if (pass == 0) {
        QList<RelatedClass>& bases = cn->baseClasses();
        QList<RelatedClass>::iterator b = bases.begin();
        while (b != bases.end()) {
            if (!(*b).node_) {
                InnerNode* parent = cn->parent();
                Node* n = findClassNode((*b).path_);
                /*
                  If the node for the base class was not found,
                  the reason might be that the subclass is in a
                  namespace and the base class is in the same
                  namespace, but the base class name was not
                  qualified with the namespace name. That is the
                  case most of the time. Then restart the search
                  at the parent of the subclass node (the namespace
                  node) using the unqualified base class name.
                 */
                if (!n)
                    n = findClassNode((*b).path_, parent);
                if (n)
                    (*b).node_ = static_cast<ClassNode*>(n);
            }
            ++b;
        }
    }
    else {
        NodeList::ConstIterator c = cn->childNodes().constBegin();
        while (c != cn->childNodes().constEnd()) {
            if ((*c)->type() == Node::Function) {
                FunctionNode* func = (FunctionNode*)* c;
                FunctionNode* from = findVirtualFunctionInBaseClasses(cn, func);
                if (from != 0) {
                    if (func->virtualness() == FunctionNode::NonVirtual)
                        func->setVirtualness(FunctionNode::ImpureVirtual);
                    func->setReimplementedFrom(from);
                }
            }
            else if ((*c)->type() == Node::Property)
                cn->fixPropertyUsingBaseClasses(static_cast<PropertyNode*>(*c));
            ++c;
        }
    }
}

/*!
 */
void Tree::resolveProperties()
{
    PropertyMap::ConstIterator propEntry;

    propEntry = unresolvedPropertyMap.constBegin();
    while (propEntry != unresolvedPropertyMap.constEnd()) {
        PropertyNode* property = propEntry.key();
        InnerNode* parent = property->parent();
        QString getterName = (*propEntry)[PropertyNode::Getter];
        QString setterName = (*propEntry)[PropertyNode::Setter];
        QString resetterName = (*propEntry)[PropertyNode::Resetter];
        QString notifierName = (*propEntry)[PropertyNode::Notifier];

        NodeList::ConstIterator c = parent->childNodes().constBegin();
        while (c != parent->childNodes().constEnd()) {
            if ((*c)->type() == Node::Function) {
                FunctionNode* function = static_cast<FunctionNode*>(*c);
                if (function->access() == property->access() &&
                        (function->status() == property->status() ||
                         function->doc().isEmpty())) {
                    if (function->name() == getterName) {
                        property->addFunction(function, PropertyNode::Getter);
                    }
                    else if (function->name() == setterName) {
                        property->addFunction(function, PropertyNode::Setter);
                    }
                    else if (function->name() == resetterName) {
                        property->addFunction(function, PropertyNode::Resetter);
                    }
                    else if (function->name() == notifierName) {
                        property->addSignal(function, PropertyNode::Notifier);
                    }
                }
            }
            ++c;
        }
        ++propEntry;
    }

    propEntry = unresolvedPropertyMap.constBegin();
    while (propEntry != unresolvedPropertyMap.constEnd()) {
        PropertyNode* property = propEntry.key();
        // redo it to set the property functions
        if (property->overriddenFrom())
            property->setOverriddenFrom(property->overriddenFrom());
        ++propEntry;
    }

    unresolvedPropertyMap.clear();
}

/*!
  For each QML class node that points to a C++ class node,
  follow its C++ class node pointer and set the C++ class
  node's QML class node pointer back to the QML class node.
 */
void Tree::resolveCppToQmlLinks()
{

    foreach (Node* child, root_.childNodes()) {
        if (child->type() == Node::Document && child->subType() == Node::QmlClass) {
            QmlClassNode* qcn = static_cast<QmlClassNode*>(child);
            ClassNode* cn = const_cast<ClassNode*>(qcn->classNode());
            if (cn)
                cn->setQmlElement(qcn);
        }
    }
}

/*!
 */
void Tree::fixInheritance(NamespaceNode* rootNode)
{
    if (!rootNode)
        rootNode = root();

    NodeList::ConstIterator c = rootNode->childNodes().constBegin();
    while (c != rootNode->childNodes().constEnd()) {
        if ((*c)->type() == Node::Class)
            static_cast<ClassNode*>(*c)->fixBaseClasses();
        else if ((*c)->type() == Node::Namespace) {
            NamespaceNode* ns = static_cast<NamespaceNode*>(*c);
            fixInheritance(ns);
        }
        ++c;
    }
}

/*!
 */
FunctionNode* Tree::findVirtualFunctionInBaseClasses(ClassNode* cn, FunctionNode* clone)
{
    const QList<RelatedClass>& rc = cn->baseClasses();
    QList<RelatedClass>::ConstIterator r = rc.constBegin();
    while (r != rc.constEnd()) {
        FunctionNode* func;
        if ((*r).node_) {
            if (((func = findVirtualFunctionInBaseClasses((*r).node_, clone)) != 0 ||
                 (func = (*r).node_->findFunctionNode(clone)) != 0)) {
                if (func->virtualness() != FunctionNode::NonVirtual)
                    return func;
            }
        }
        ++r;
    }
    return 0;
}

/*!
 */
NodeList Tree::allBaseClasses(const ClassNode* classNode) const
{
    NodeList result;
    foreach (const RelatedClass& r, classNode->baseClasses()) {
        if (r.node_) {
            result += r.node_;
            result += allBaseClasses(r.node_);
        }
    }
    return result;
}

/*!
  Find the node with the specified \a path name that is of
  the specified \a type and \a subtype. Begin the search at
  the \a start node. If the \a start node is 0, begin the
  search at the tree root. \a subtype is not used unless
  \a type is \c{Document}.
 */
Node* Tree::findNodeByNameAndType(const QStringList& path,
                                  Node::Type type,
                                  Node::SubType subtype,
                                  Node* start,
                                  bool acceptCollision)
{
    if (!start)
        start = const_cast<NamespaceNode*>(root());
    Node* result = findNodeRecursive(path, 0, start, type, subtype, acceptCollision);
    return result;
}

/* internal members */

/*!
  Recursive search for a node identified by \a path. Each
  path element is a name. \a pathIndex specifies the index
  of the name in \a path to try to match. \a start is the
  node whose children shoulod be searched for one that has
  that name. Each time a match is found, increment the
  \a pathIndex and call this function recursively.

  If the end of the path is reached (i.e. if a matching
  node is found for each name in the \a path), the \a type
  must match the type of the last matching node, and if the
  type is \e{Document}, the \a subtype must match as well.

  If the algorithm is successful, the pointer to the final
  node is returned. Otherwise 0 is returned.
 */
Node* Tree::findNodeRecursive(const QStringList& path,
                              int pathIndex,
                              Node* start,
                              Node::Type type,
                              Node::SubType subtype,
                              bool acceptCollision) const
{
    if (!start || path.isEmpty())
        return 0; // no place to start, or nothing to search for.
    if (start->isLeaf()) {
        if (pathIndex >= path.size())
            return start; // found a match.
        return 0; // premature leaf
    }
    if (pathIndex >= path.size())
        return 0; // end of search path.

    InnerNode* current = static_cast<InnerNode*>(start);
    const NodeList& children = current->childNodes();
    const QString& name = path.at(pathIndex);
    for (int i=0; i<children.size(); ++i) {
        Node* n = children.at(i);
        if (!n)
            continue;
        if (n->isQmlPropertyGroup()) {
            if (type == Node::QmlProperty) {
                n = findNodeRecursive(path, pathIndex, n, type, subtype);
                if (n)
                    return n;
            }
        }
        else if (n->name() == name) {
            if (pathIndex+1 >= path.size()) {
                if (n->type() == type) {
                    if (type == Node::Document) {
                        if (n->subType() == subtype)
                            return n;
                        else if (n->subType() == Node::Collision) {
                            if (acceptCollision)
                                return n;
                            return n->disambiguate(type, subtype);
                        }
                        else if (subtype == Node::NoSubType)
                            return n;
                        continue;
                    }
                    return n;
                }
                else if (n->isCollisionNode()) {
                    if (acceptCollision)
                        return n;
                    return findNodeRecursive(path, pathIndex, n, type, subtype);
                }
                else {
                    continue;
                }
            }
            else { // Search the children of n for the next name in the path.
                n = findNodeRecursive(path, pathIndex+1, n, type, subtype);
                if (n)
                    return n;
            }
        }
    }
    return 0;
}

/*!
  Recursive search for a node identified by \a path. Each
  path element is a name. \a pathIndex specifies the index
  of the name in \a path to try to match. \a start is the
  node whose children shoulod be searched for one that has
  that name. Each time a name match is found, increment the
  \a pathIndex and call this function recursively.

  If the end of the path is reached (i.e. if a matching
  node is found for each name in the \a path), test the
  matching node's type and subtype values against the ones
  listed in \a types. If a match is found there, return the
  pointer to the final node. Otherwise return 0.
 */
Node* Tree::findNodeRecursive(const QStringList& path,
                              int pathIndex,
                              Node* start,
                              const NodeTypeList& types) const
{
    /*
      Safety checks
    */
    if (!start || path.isEmpty())
        return 0;
    if (start->isLeaf())
        return ((pathIndex >= path.size()) ? start : 0);
    if (pathIndex >= path.size())
        return 0;

    InnerNode* current = static_cast<InnerNode*>(start);
    const NodeList& children = current->childNodes();
    for (int i=0; i<children.size(); ++i) {
        Node* n = children.at(i);
        if (n && n->name() == path.at(pathIndex)) {
            if (pathIndex+1 >= path.size()) {
                if (n->match(types))
                    return n;
            }
            else if (!n->isLeaf()) {
                n = findNodeRecursive(path, pathIndex+1, n, types);
                if (n)
                    return n;
            }
        }
    }
    return 0;
}

/*!
  Searches the tree for a node that matches the \a path. The
  search begins at \a start but can move up the parent chain
  recursively if no match is found.

  This findNode() callse the other findNode(), which is not
  called anywhere else.
 */
const Node* Tree::findNode(const QStringList& path, const Node* start, int findFlags) const
{
    const Node* current = start;
    if (!current)
        current = root();

    /*
      First, search for a node assuming we don't want a QML node.
      If that search fails, search again assuming we do want a
      QML node.
     */
    const Node* n = findNode(path, current, findFlags, false);
    return (n ? n : findNode(path, current, findFlags, true));
}

/*!
  This overload function was extracted from the one above that has the
  same signature without the last bool parameter, \a qml. This version
  is called only by that other one. It is therefore private.  It can
  be called a second time by that other version, if the first call
  returns null. If \a qml is false, the search will only match a node
  that is not a QML node.  If \a qml is true, the search will only
  match a node that is a QML node.

  This findNode() is only called by the other findNode().
*/
const Node* Tree::findNode(const QStringList& path, const Node* start, int findFlags, bool qml) const
{
    const Node* current = start;
    do {
        const Node* node = current;
        int i;
        int start_idx = 0;

        /*
          If the path contains one or two double colons ("::"),
          check first to see if the first two path strings refer
          to a QML element. If they do, path[0] will be the QML
          module identifier, and path[1] will be the QML type.
          If the anser is yes, the reference identifies a QML
          class node.
        */
        if (qml && path.size() >= 2 && !path[0].isEmpty()) {
            QmlClassNode* qcn = qdb_->findQmlType(path[0], path[1]);
            if (qcn) {
                node = qcn;
                if (path.size() == 2)
                    return node;
                start_idx = 2;
            }
        }

        for (i = start_idx; i < path.size(); ++i) {
            if (node == 0 || !node->isInnerNode())
                break;

            const Node* next = static_cast<const InnerNode*>(node)->findChildNodeByName(path.at(i), qml);
            if (!next && (findFlags & SearchEnumValues) && i == path.size()-1)
                next = static_cast<const InnerNode*>(node)->findEnumNodeForValue(path.at(i));

            if (!next && !qml && node->type() == Node::Class && (findFlags & SearchBaseClasses)) {
                NodeList baseClasses = allBaseClasses(static_cast<const ClassNode*>(node));
                foreach (const Node* baseClass, baseClasses) {
                    next = static_cast<const InnerNode*>(baseClass)->findChildNodeByName(path.at(i));
                    if (!next && (findFlags & SearchEnumValues) && i == path.size() - 1)
                        next = static_cast<const InnerNode*>(baseClass)->findEnumNodeForValue(path.at(i));
                    if (next)
                        break;
                }
            }
            node = next;
        }
        if (node && i == path.size()
                && (!(findFlags & NonFunction) || node->type() != Node::Function
                    || ((FunctionNode*)node)->metaness() == FunctionNode::MacroWithoutParams)) {
            if (!node->isQmlPropertyGroup()) {
                if (node->isCollisionNode())
                    node = node->applyModuleName(start);
                return node;
            }
        }
        current = current->parent();
    } while (current);

    return 0;
}

QT_END_NAMESPACE
