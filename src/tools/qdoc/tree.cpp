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
  tree.cpp
*/

#include <QDomDocument>
#include "atom.h"
#include "doc.h"
#include "htmlgenerator.h"
#include "location.h"
#include "node.h"
#include "text.h"
#include "tree.h"
#include <limits.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

struct InheritanceBound
{
    Node::Access access;
    QStringList basePath;
    QString dataTypeWithTemplateArgs;
    InnerNode* parent;

    InheritanceBound()
        : access(Node::Public) { }
    InheritanceBound(Node::Access access0,
                     const QStringList& basePath0,
                     const QString& dataTypeWithTemplateArgs0,
                     InnerNode* parent)
        : access(access0), basePath(basePath0),
          dataTypeWithTemplateArgs(dataTypeWithTemplateArgs0),
          parent(parent) { }
};

struct Target
{
    Node* node;
    Atom* atom;
    int priority;
};

typedef QMap<PropertyNode::FunctionRole, QString> RoleMap;
typedef QMap<PropertyNode*, RoleMap> PropertyMap;
typedef QMultiHash<QString, FakeNode*> FakeNodeHash;
typedef QMultiHash<QString, Target> TargetHash;

class TreePrivate
{
public:
    QMap<ClassNode* , QList<InheritanceBound> > unresolvedInheritanceMap;
    PropertyMap unresolvedPropertyMap;
    NodeMultiMap groupMap;
    QMultiMap<QString, QString> publicGroupMap;
    FakeNodeHash fakeNodesByTitle;
    TargetHash targetHash;
    QList<QPair<ClassNode*,QString> > basesList;
    QList<QPair<FunctionNode*,QString> > relatedList;
};

/*!
  \class Tree

  This class constructs and maintains a tree of instances of
  Node and its many subclasses.
 */

/*!
  The default constructor is the only constructor.
 */
Tree::Tree()
    : roo(0, "")
{
    priv = new TreePrivate;
}

/*!
  The destructor deletes the internal, private tree.
 */
Tree::~Tree()
{
    delete priv;
}

// 1 calls 2
/*!
  Searches the tree for a node that matches the \a path. The
  search begins at \a start but can move up the parent chain
  recursively if no match is found.
 */
const Node* Tree::findNode(const QStringList& path,
                           const Node* start,
                           int findFlags,
                           const Node* self) const
{
    const Node* current = start;
    if (!current)
        current = root();

    /*
      First, search for a node assuming we don't want a QML node.
      If that search fails, search again assuming we do want a
      QML node.
     */
    const Node* n = findNode(path,current,findFlags,self,false);
    if (!n) {
        n = findNode(path,current,findFlags,self,true);
    }
    return n;
}

// 2 is private; it is only called by 1.
/*!
  This overload function was extracted from the one above that has the
  same signature without the last bool parameter, \a qml. This version
  is called only by that other one. It is therefore private.  It can
  be called a second time by that other version, if the first call
  returns null. If \a qml is false, the search will only match a node
  that is not a QML node.  If \a qml is true, the search will only
  match a node that is a QML node.
*/
const Node* Tree::findNode(const QStringList& path,
                           const Node* start,
                           int findFlags,
                           const Node* self,
                           bool qml) const
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
        if (qml && path.size() >= 2) {
            QmlClassNode* qcn = QmlClassNode::lookupQmlTypeNode(path[0], path[1]);
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
            if ((node != self) && (node->subType() != Node::QmlPropertyGroup)) {
                if (node->subType() == Node::Collision) {
                    node = node->applyModuleIdentifier(start);
                }
                return node;
            }
        }
        current = current->parent();
    } while (current);

    return 0;
}

/*!
  Find the QML class node for the specified \a module and \a name
  identifiers. The \a module identifier may be empty. If the module
  identifier is empty, then begin by finding the FakeNode that has
  the specified \a name. If that FakeNode is a QML class, return it.
  If it is a collision node, return its current child, if the current
  child is a QML class. If the collision node does not have a child
  that is a QML class node, return 0.
 */
QmlClassNode* Tree::findQmlClassNode(const QString& module, const QString& name)
{
    if (module.isEmpty()) {
        Node* n = findQmlClassNode(QStringList(name));
        if (n) {
            if (n->subType() == Node::QmlClass)
                return static_cast<QmlClassNode*>(n);
            else if (n->subType() == Node::Collision) {
                NameCollisionNode* ncn;
                ncn = static_cast<NameCollisionNode*>(n);
                return static_cast<QmlClassNode*>(ncn->findAny(Node::Fake,Node::QmlClass));
            }
        }
        return 0;
    }
    return QmlClassNode::lookupQmlTypeNode(module, name);
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
  This function just calls the const version of the same function
  and returns the function node.
 */
FunctionNode* Tree::findFunctionNode(const QStringList& path,
                                     Node* relative,
                                     int findFlags)
{
    return const_cast<FunctionNode*>
            (const_cast<const Tree*>(this)->findFunctionNode(path,relative,findFlags));
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
    if (path.size() == 3) {
        QmlClassNode* qcn = QmlClassNode::lookupQmlTypeNode(path[0], path[1]);
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

/*!
  This function just calls the const version of itself and
  returns the result.
 */
FunctionNode* Tree::findFunctionNode(const QStringList& parentPath,
                                     const FunctionNode* clone,
                                     Node* relative,
                                     int findFlags)
{
    return const_cast<FunctionNode*>(
                const_cast<const Tree*>(this)->findFunctionNode(parentPath,
                                                                clone,
                                                                relative,
                                                                findFlags));
}

/*!
  This function first ignores the \a clone node and searches
  for the node having the \a parentPath by calling the main
  findFunction(\a {parentPath}, \a {relative}, \a {findFlags}).
  If that search is successful, then it searches for the \a clone
  in the found parent node.
 */
const FunctionNode* Tree::findFunctionNode(const QStringList& parentPath,
                                           const FunctionNode* clone,
                                           const Node* relative,
                                           int findFlags) const
{
    const Node* parent = findNode(parentPath, relative, findFlags);
    if (parent == 0 || !parent->isInnerNode()) {
        return 0;
    }
    else {
        return ((InnerNode*)parent)->findFunctionNode(clone);
    }
}
//findNode(parameter.leftType().split("::"), 0, SearchBaseClasses|NonFunction);

static const int NumSuffixes = 3;
static const char*  const suffixes[NumSuffixes] = { "", "s", "es" };

/*!
  This function searches for a node with the specified \a title.
  If \a relative node is provided, it is used to disambiguate if
  it has a QML module identifier.
 */
const FakeNode* Tree::findFakeNodeByTitle(const QString& title, const Node* relative ) const
{
    for (int pass = 0; pass < NumSuffixes; ++pass) {
        FakeNodeHash::const_iterator i = priv->fakeNodesByTitle.constFind(Doc::canonicalTitle(title + suffixes[pass]));
        if (i != priv->fakeNodesByTitle.constEnd()) {
            if (relative && !relative->qmlModuleIdentifier().isEmpty()) {
                const FakeNode* fn = i.value();
                InnerNode* parent = fn->parent();
                if (parent && parent->type() == Node::Fake && parent->subType() == Node::Collision) {
                    const NodeList& nl = parent->childNodes();
                    NodeList::ConstIterator it = nl.constBegin();
                    while (it != nl.constEnd()) {
                        if ((*it)->qmlModuleIdentifier() == relative->qmlModuleIdentifier()) {
                            /*
                              By returning here, we avoid printing all the duplicate
                              header warnings, which are not really duplicates now,
                              because of the QML module identifier being used as a
                              namespace qualifier.
                             */
                            fn = static_cast<const FakeNode*>(*it);
                            return fn;
                        }
                        ++it;
                    }
                }
            }
            /*
              Reporting all these duplicate section titles is probably
              overkill. We should report the duplicate file and let
              that suffice.
             */
            FakeNodeHash::const_iterator j = i;
            ++j;
            if (j != priv->fakeNodesByTitle.constEnd() && j.key() == i.key()) {
                QList<Location> internalLocations;
                while (j != priv->fakeNodesByTitle.constEnd()) {
                    if (j.key() == i.key() && j.value()->url().isEmpty())
                        internalLocations.append(j.value()->location());
                    ++j;
                }
                if (internalLocations.size() > 0) {
                    i.value()->location().warning(tr("This page exists in more than one file: \"%1\"").arg(title));
                    foreach (const Location &location, internalLocations)
                        location.warning(tr("[It also exists here]"));
                }
            }
            return i.value();
        }
    }
    return 0;
}

/*!
  This function searches for a \a target anchor node. If it
  finds one, it sets \a atom from the found node and returns
  the found node.
 */
const Node*
Tree::findUnambiguousTarget(const QString& target, Atom *&atom, const Node* relative) const
{
    Target bestTarget = {0, 0, INT_MAX};
    int numBestTargets = 0;
    QList<Target> bestTargetList;

    for (int pass = 0; pass < NumSuffixes; ++pass) {
        TargetHash::const_iterator i = priv->targetHash.constFind(Doc::canonicalTitle(target + suffixes[pass]));
        if (i != priv->targetHash.constEnd()) {
            TargetHash::const_iterator j = i;
            do {
                const Target& candidate = j.value();
                if (candidate.priority < bestTarget.priority) {
                    bestTarget = candidate;
                    bestTargetList.clear();
                    bestTargetList.append(candidate);
                    numBestTargets = 1;
                } else if (candidate.priority == bestTarget.priority) {
                    bestTargetList.append(candidate);
                    ++numBestTargets;
                }
                ++j;
            } while (j != priv->targetHash.constEnd() && j.key() == i.key());

            if (numBestTargets == 1) {
                atom = bestTarget.atom;
                return bestTarget.node;
            }
            else if (bestTargetList.size() > 1) {
                if (relative && !relative->qmlModuleIdentifier().isEmpty()) {
                    for (int i=0; i<bestTargetList.size(); ++i) {
                        const Node* n = bestTargetList.at(i).node;
                        if (relative->qmlModuleIdentifier() == n->qmlModuleIdentifier()) {
                            atom = bestTargetList.at(i).atom;
                            return n;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/*!
  This function searches for a node with a canonical title
  constructed from \a target and each of the possible suffixes.
  If the node it finds is \a node, it returns the Atom from that
  node. Otherwise it returns null.
 */
Atom* Tree::findTarget(const QString& target, const Node* node) const
{
    for (int pass = 0; pass < NumSuffixes; ++pass) {
        QString key = Doc::canonicalTitle(target + suffixes[pass]);
        TargetHash::const_iterator i = priv->targetHash.constFind(key);

        if (i != priv->targetHash.constEnd()) {
            do {
                if (i.value().node == node)
                    return i.value().atom;
                ++i;
            } while (i != priv->targetHash.constEnd() && i.key() == key);
        }
    }
    return 0;
}

/*!
 */
void Tree::addBaseClass(ClassNode* subclass, Node::Access access,
                        const QStringList& basePath,
                        const QString& dataTypeWithTemplateArgs,
                        InnerNode* parent)
{
    priv->unresolvedInheritanceMap[subclass].append(
                InheritanceBound(access,
                                 basePath,
                                 dataTypeWithTemplateArgs,
                                 parent)
                );
}

/*!
 */
void Tree::addPropertyFunction(PropertyNode* property,
                               const QString& funcName,
                               PropertyNode::FunctionRole funcRole)
{
    priv->unresolvedPropertyMap[property].insert(funcRole, funcName);
}

/*!
  This function adds the \a node to the \a group. The group
  can be listed anywhere using the \e{annotated list} command.
 */
void Tree::addToGroup(Node* node, const QString& group)
{
    priv->groupMap.insert(group, node);
}

/*!
  Returns the group map.
 */
NodeMultiMap Tree::groups() const
{
    return priv->groupMap;
}

/*!
  This function adds the \a group name to the list of groups
  for the \a node name. It also adds the \a node to the \a group.
 */
void Tree::addToPublicGroup(Node* node, const QString& group)
{
    priv->publicGroupMap.insert(node->name(), group);
    addToGroup(node, group);
}

/*!
  Returns the public group map.
 */
QMultiMap<QString, QString> Tree::publicGroups() const
{
    return priv->publicGroupMap;
}

/*!
 */
void Tree::resolveInheritance(NamespaceNode* rootNode)
{
    if (!rootNode)
        rootNode = root();

    for (int pass = 0; pass < 2; pass++) {
        NodeList::ConstIterator c = rootNode->childNodes().constBegin();
        while (c != rootNode->childNodes().constEnd()) {
            if ((*c)->type() == Node::Class) {
                resolveInheritance(pass, (ClassNode*)* c);
            }
            else if ((*c)->type() == Node::Namespace) {
                NamespaceNode* ns = static_cast<NamespaceNode*>(*c);
                resolveInheritance(ns);
            }
            ++c;
        }
        if (rootNode == root())
            priv->unresolvedInheritanceMap.clear();
    }
}

/*!
 */
void Tree::resolveProperties()
{
    PropertyMap::ConstIterator propEntry;

    propEntry = priv->unresolvedPropertyMap.constBegin();
    while (propEntry != priv->unresolvedPropertyMap.constEnd()) {
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

    propEntry = priv->unresolvedPropertyMap.constBegin();
    while (propEntry != priv->unresolvedPropertyMap.constEnd()) {
        PropertyNode* property = propEntry.key();
        // redo it to set the property functions
        if (property->overriddenFrom())
            property->setOverriddenFrom(property->overriddenFrom());
        ++propEntry;
    }

    priv->unresolvedPropertyMap.clear();
}

/*!
 */
void Tree::resolveInheritance(int pass, ClassNode* classe)
{
    if (pass == 0) {
        QList<InheritanceBound> bounds = priv->unresolvedInheritanceMap[classe];
        QList<InheritanceBound>::ConstIterator b = bounds.constBegin();
        while (b != bounds.constEnd()) {
            Node* n = findClassNode((*b).basePath);
            if (!n && (*b).parent) {
                n = findClassNode((*b).basePath, (*b).parent);
            }
            if (n) {
                classe->addBaseClass((*b).access, static_cast<ClassNode*>(n), (*b).dataTypeWithTemplateArgs);
            }
            ++b;
        }
    }
    else {
        NodeList::ConstIterator c = classe->childNodes().constBegin();
        while (c != classe->childNodes().constEnd()) {
            if ((*c)->type() == Node::Function) {
                FunctionNode* func = (FunctionNode*)* c;
                FunctionNode* from = findVirtualFunctionInBaseClasses(classe, func);
                if (from != 0) {
                    if (func->virtualness() == FunctionNode::NonVirtual)
                        func->setVirtualness(FunctionNode::ImpureVirtual);
                    func->setReimplementedFrom(from);
                }
            }
            else if ((*c)->type() == Node::Property) {
                fixPropertyUsingBaseClasses(classe, static_cast<PropertyNode*>(*c));
            }
            ++c;
        }
    }
}

/*!
  For each node in the group map, add the node to the appropriate
  group node.
 */
void Tree::resolveGroups()
{
    NodeMultiMap::const_iterator i;
    for (i = priv->groupMap.constBegin(); i != priv->groupMap.constEnd(); ++i) {
        if (i.value()->access() == Node::Private)
            continue;

        Node* n = findGroupNode(QStringList(i.key()));
        if (n)
            n->addGroupMember(i.value());
    }
}

/*!
 */
void Tree::resolveTargets(InnerNode* root)
{
    // need recursion

    foreach (Node* child, root->childNodes()) {
        if (child->type() == Node::Fake) {
            FakeNode* node = static_cast<FakeNode*>(child);
            if (!node->title().isEmpty())
                priv->fakeNodesByTitle.insert(Doc::canonicalTitle(node->title()), node);
            if (node->subType() == Node::Collision) {
                resolveTargets(node);
            }
        }

        if (child->doc().hasTableOfContents()) {
            const QList<Atom*>& toc = child->doc().tableOfContents();
            Target target;
            target.node = child;
            target.priority = 3;

            for (int i = 0; i < toc.size(); ++i) {
                target.atom = toc.at(i);
                QString title = Text::sectionHeading(target.atom).toString();
                if (!title.isEmpty())
                    priv->targetHash.insert(Doc::canonicalTitle(title), target);
            }
        }
        if (child->doc().hasKeywords()) {
            const QList<Atom*>& keywords = child->doc().keywords();
            Target target;
            target.node = child;
            target.priority = 1;

            for (int i = 0; i < keywords.size(); ++i) {
                target.atom = keywords.at(i);
                priv->targetHash.insert(Doc::canonicalTitle(target.atom->string()), target);
            }
        }
        if (child->doc().hasTargets()) {
            const QList<Atom*>& toc = child->doc().targets();
            Target target;
            target.node = child;
            target.priority = 2;

            for (int i = 0; i < toc.size(); ++i) {
                target.atom = toc.at(i);
                priv->targetHash.insert(Doc::canonicalTitle(target.atom->string()), target);
            }
        }
    }
}

/*!
  For each QML class node that points to a C++ class node,
  follow its C++ class node pointer and set the C++ class
  node's QML class node pointer back to the QML class node.
 */
void Tree::resolveCppToQmlLinks()
{

    foreach (Node* child, roo.childNodes()) {
        if (child->type() == Node::Fake && child->subType() == Node::QmlClass) {
            QmlClassNode* qcn = static_cast<QmlClassNode*>(child);
            ClassNode* cn = const_cast<ClassNode*>(qcn->classNode());
            if (cn)
                cn->setQmlElement(qcn);
        }
    }
}

/*!
  For each QML class node in the tree, determine whether
  it inherits a QML base class and, if so, which one, and
  store that pointer in the QML class node's state.
 */
void Tree::resolveQmlInheritance()
{

    foreach (Node* child, roo.childNodes()) {
        if (child->type() == Node::Fake) {
            if (child->subType() == Node::QmlClass) {
                QmlClassNode* qcn = static_cast<QmlClassNode*>(child);
                qcn->resolveInheritance(this);
            }
            else if (child->subType() == Node::Collision) {
                NameCollisionNode* ncn = static_cast<NameCollisionNode*>(child);
                foreach (Node* child, ncn->childNodes()) {
                    if (child->type() == Node::Fake) {
                        if (child->subType() == Node::QmlClass) {
                            QmlClassNode* qcn = static_cast<QmlClassNode*>(child);
                            qcn->resolveInheritance(this);
                        }
                    }
                }
            }
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
FunctionNode* Tree::findVirtualFunctionInBaseClasses(ClassNode* classe,
                                                     FunctionNode* clone)
{
    QList<RelatedClass>::ConstIterator r = classe->baseClasses().constBegin();
    while (r != classe->baseClasses().constEnd()) {
        FunctionNode* func;
        if (((func = findVirtualFunctionInBaseClasses((*r).node, clone)) != 0 ||
             (func = (*r).node->findFunctionNode(clone)) != 0)) {
            if (func->virtualness() != FunctionNode::NonVirtual)
                return func;
        }
        ++r;
    }
    return 0;
}

/*!
 */
void Tree::fixPropertyUsingBaseClasses(ClassNode* classe, PropertyNode* property)
{
    QList<RelatedClass>::const_iterator r = classe->baseClasses().constBegin();
    while (r != classe->baseClasses().constEnd()) {
        Node* n = r->node->findChildNodeByNameAndType(property->name(), Node::Property);
        if (n) {
            PropertyNode* baseProperty = static_cast<PropertyNode*>(n);
            fixPropertyUsingBaseClasses(r->node, baseProperty);
            property->setOverriddenFrom(baseProperty);
        }
        else {
            fixPropertyUsingBaseClasses(r->node, property);
        }
        ++r;
    }
}

/*!
 */
NodeList Tree::allBaseClasses(const ClassNode* classe) const
{
    NodeList result;
    foreach (const RelatedClass& r, classe->baseClasses()) {
        result += r.node;
        result += allBaseClasses(r.node);
    }
    return result;
}

/*!
 */
void Tree::readIndexes(const QStringList& indexFiles)
{
    foreach (const QString& indexFile, indexFiles)
        readIndexFile(indexFile);
}

/*!
  Read the QDomDocument at \a path and get the index from it.
 */
void Tree::readIndexFile(const QString& path)
{
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        QDomDocument document;
        document.setContent(&file);
        file.close();

        QDomElement indexElement = document.documentElement();

        // Generate a relative URL between the install dir and the index file
        // when the -installdir command line option is set.
        QString indexUrl;
        if (Config::installDir.isEmpty()) {
            indexUrl = indexElement.attribute("url", "");
        }
        else {
            // Use a fake directory, since we will copy the output to a sub directory of
            // installDir when using "make install". This is just for a proper relative path.
            QDir installDir(Config::installDir + "/outputdir");
            indexUrl = installDir.relativeFilePath(path).section('/', 0, -2);
        }

        priv->basesList.clear();
        priv->relatedList.clear();

        // Scan all elements in the XML file, constructing a map that contains
        // base classes for each class found.

        QDomElement child = indexElement.firstChildElement();
        while (!child.isNull()) {
            readIndexSection(child, root(), indexUrl);
            child = child.nextSiblingElement();
        }

        // Now that all the base classes have been found for this index,
        // arrange them into an inheritance hierarchy.

        resolveIndex();
    }
}

/*!
  Read a <section> element from the index file and create the
  appropriate node(s).
 */
void Tree::readIndexSection(const QDomElement& element,
                            InnerNode* parent,
                            const QString& indexUrl)
{
    QString name = element.attribute("name");
    QString href = element.attribute("href");

    Node* section;
    Location location;

    if (element.nodeName() == "namespace") {
        section = new NamespaceNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name.toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(name.toLower() + ".html");

    }
    else if (element.nodeName() == "class") {
        section = new ClassNode(parent, name);
        priv->basesList.append(QPair<ClassNode*,QString>(
                                   static_cast<ClassNode*>(section), element.attribute("bases")));

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name.toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(name.toLower() + ".html");
    }
    else if ((element.nodeName() == "qmlclass") ||
             ((element.nodeName() == "page") && (element.attribute("subtype") == "qmlclass"))) {
        QmlClassNode* qcn = new QmlClassNode(parent, name, 0);
        qcn->setTitle(element.attribute("title"));
        if (element.hasAttribute("location"))
            name = element.attribute("location", "");
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        section = qcn;
    }
    else if (element.nodeName() == "qmlbasictype") {
        QmlBasicTypeNode* qbtn = new QmlBasicTypeNode(parent, name);
        qbtn->setTitle(element.attribute("title"));
        if (element.hasAttribute("location"))
            name = element.attribute("location", "");
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        section = qbtn;
    }
    else if (element.nodeName() == "page") {
        Node::SubType subtype;
        Node::PageType ptype = Node::NoPageType;
        if (element.attribute("subtype") == "example") {
            subtype = Node::Example;
            ptype = Node::ExamplePage;
        }
        else if (element.attribute("subtype") == "header") {
            subtype = Node::HeaderFile;
            ptype = Node::ApiPage;
        }
        else if (element.attribute("subtype") == "file") {
            subtype = Node::File;
            ptype = Node::NoPageType;
        }
        else if (element.attribute("subtype") == "group") {
            subtype = Node::Group;
            ptype = Node::OverviewPage;
        }
        else if (element.attribute("subtype") == "module") {
            subtype = Node::Module;
            ptype = Node::OverviewPage;
        }
        else if (element.attribute("subtype") == "page") {
            subtype = Node::Page;
            ptype = Node::ArticlePage;
        }
        else if (element.attribute("subtype") == "externalpage") {
            subtype = Node::ExternalPage;
            ptype = Node::ArticlePage;
        }
        else if (element.attribute("subtype") == "qmlclass") {
            subtype = Node::QmlClass;
            ptype = Node::ApiPage;
        }
        else if (element.attribute("subtype") == "qmlpropertygroup") {
            subtype = Node::QmlPropertyGroup;
            ptype = Node::ApiPage;
        }
        else if (element.attribute("subtype") == "qmlbasictype") {
            subtype = Node::QmlBasicType;
            ptype = Node::ApiPage;
        }
        else
            return;

        FakeNode* fakeNode = new FakeNode(parent, name, subtype, ptype);
        fakeNode->setTitle(element.attribute("title"));

        if (element.hasAttribute("location"))
            name = element.attribute("location", "");

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);

        section = fakeNode;

    }
    else if (element.nodeName() == "enum") {
        EnumNode* enumNode = new EnumNode(parent, name);

        if (!indexUrl.isEmpty())
            location =
                    Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

        QDomElement child = element.firstChildElement("value");
        while (!child.isNull()) {
            EnumItem item(child.attribute("name"), child.attribute("value"));
            enumNode->addItem(item);
            child = child.nextSiblingElement("value");
        }

        section = enumNode;

    } else if (element.nodeName() == "typedef") {
        section = new TypedefNode(parent, name);

        if (!indexUrl.isEmpty())
            location =
                    Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    }
    else if (element.nodeName() == "property") {
        section = new PropertyNode(parent, name);

        if (!indexUrl.isEmpty())
            location =
                    Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    } else if (element.nodeName() == "function") {
        FunctionNode::Virtualness virt;
        if (element.attribute("virtual") == "non")
            virt = FunctionNode::NonVirtual;
        else if (element.attribute("virtual") == "impure")
            virt = FunctionNode::ImpureVirtual;
        else if (element.attribute("virtual") == "pure")
            virt = FunctionNode::PureVirtual;
        else
            return;

        FunctionNode::Metaness meta;
        if (element.attribute("meta") == "plain")
            meta = FunctionNode::Plain;
        else if (element.attribute("meta") == "signal")
            meta = FunctionNode::Signal;
        else if (element.attribute("meta") == "slot")
            meta = FunctionNode::Slot;
        else if (element.attribute("meta") == "constructor")
            meta = FunctionNode::Ctor;
        else if (element.attribute("meta") == "destructor")
            meta = FunctionNode::Dtor;
        else if (element.attribute("meta") == "macro")
            meta = FunctionNode::MacroWithParams;
        else if (element.attribute("meta") == "macrowithparams")
            meta = FunctionNode::MacroWithParams;
        else if (element.attribute("meta") == "macrowithoutparams")
            meta = FunctionNode::MacroWithoutParams;
        else
            return;

        FunctionNode* functionNode = new FunctionNode(parent, name);
        functionNode->setReturnType(element.attribute("return"));
        functionNode->setVirtualness(virt);
        functionNode->setMetaness(meta);
        functionNode->setConst(element.attribute("const") == "true");
        functionNode->setStatic(element.attribute("static") == "true");
        functionNode->setOverload(element.attribute("overload") == "true");

        if (element.hasAttribute("relates")
                && element.attribute("relates") != parent->name()) {
            priv->relatedList.append(
                        QPair<FunctionNode*,QString>(functionNode,
                                                     element.attribute("relates")));
        }

        QDomElement child = element.firstChildElement("parameter");
        while (!child.isNull()) {
            // Do not use the default value for the parameter; it is not
            // required, and has been known to cause problems.
            Parameter parameter(child.attribute("left"),
                                child.attribute("right"),
                                child.attribute("name"),
                                ""); // child.attribute("default")
            functionNode->addParameter(parameter);
            child = child.nextSiblingElement("parameter");
        }

        section = functionNode;

        if (!indexUrl.isEmpty())
            location =
                    Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    }
    else if (element.nodeName() == "variable") {
        section = new VariableNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    }
    else if (element.nodeName() == "keyword") {
        Target target;
        target.node = parent;
        target.priority = 1;
        target.atom = new Atom(Atom::Target, name);
        priv->targetHash.insert(name, target);
        return;

    }
    else if (element.nodeName() == "target") {
        Target target;
        target.node = parent;
        target.priority = 2;
        target.atom = new Atom(Atom::Target, name);
        priv->targetHash.insert(name, target);
        return;

    }
    else if (element.nodeName() == "contents") {
        Target target;
        target.node = parent;
        target.priority = 3;
        target.atom = new Atom(Atom::Target, name);
        priv->targetHash.insert(name, target);
        return;

    }
    else
        return;

    QString access = element.attribute("access");
    if (access == "public")
        section->setAccess(Node::Public);
    else if (access == "protected")
        section->setAccess(Node::Protected);
    else if (access == "private")
        section->setAccess(Node::Private);
    else
        section->setAccess(Node::Public);

    if ((element.nodeName() != "page") &&
            (element.nodeName() != "qmlclass") &&
            (element.nodeName() != "qmlbasictype")) {
        QString threadSafety = element.attribute("threadsafety");
        if (threadSafety == "non-reentrant")
            section->setThreadSafeness(Node::NonReentrant);
        else if (threadSafety == "reentrant")
            section->setThreadSafeness(Node::Reentrant);
        else if (threadSafety == "thread safe")
            section->setThreadSafeness(Node::ThreadSafe);
        else
            section->setThreadSafeness(Node::UnspecifiedSafeness);
    }
    else
        section->setThreadSafeness(Node::UnspecifiedSafeness);

    QString status = element.attribute("status");
    if (status == "compat")
        section->setStatus(Node::Compat);
    else if (status == "obsolete")
        section->setStatus(Node::Obsolete);
    else if (status == "deprecated")
        section->setStatus(Node::Deprecated);
    else if (status == "preliminary")
        section->setStatus(Node::Preliminary);
    else if (status == "commendable")
        section->setStatus(Node::Commendable);
    else if (status == "internal")
        section->setStatus(Node::Internal);
    else if (status == "main")
        section->setStatus(Node::Main);
    else
        section->setStatus(Node::Commendable);

    section->setModuleName(element.attribute("module"));
    if (!indexUrl.isEmpty()) {
        section->setUrl(indexUrl + QLatin1Char('/') + href);
    }

    // Create some content for the node.
    QSet<QString> emptySet;

    Doc doc(location, location, " ", emptySet); // placeholder
    section->setDoc(doc);
    section->setIndexNodeFlag();

    if (section->isInnerNode()) {
        InnerNode* inner = static_cast<InnerNode*>(section);
        if (inner) {
            QDomElement child = element.firstChildElement();

            while (!child.isNull()) {
                if (element.nodeName() == "class")
                    readIndexSection(child, inner, indexUrl);
                else if (element.nodeName() == "qmlclass")
                    readIndexSection(child, inner, indexUrl);
                else if (element.nodeName() == "page")
                    readIndexSection(child, inner, indexUrl);
                else if (element.nodeName() == "namespace" && !name.isEmpty())
                    // The root node in the index is a namespace with an empty name.
                    readIndexSection(child, inner, indexUrl);
                else
                    readIndexSection(child, parent, indexUrl);

                child = child.nextSiblingElement();
            }
        }
    }
}

/*!
 */
QString Tree::readIndexText(const QDomElement& element)
{
    QString text;
    QDomNode child = element.firstChild();
    while (!child.isNull()) {
        if (child.isText())
            text += child.toText().nodeValue();
        child = child.nextSibling();
    }
    return text;
}

/*!
 */
void Tree::resolveIndex()
{
    QPair<ClassNode*,QString> pair;

    foreach (pair, priv->basesList) {
        foreach (const QString& base, pair.second.split(QLatin1Char(','))) {
            Node* n = root()->findChildNodeByNameAndType(base, Node::Class);
            if (n) {
                pair.first->addBaseClass(Node::Public, static_cast<ClassNode*>(n));
            }
        }
    }

    QPair<FunctionNode*,QString> relatedPair;

    foreach (relatedPair, priv->relatedList) {
        Node* n = root()->findChildNodeByNameAndType(relatedPair.second, Node::Class);
        if (n)
            relatedPair.first->setRelates(static_cast<ClassNode*>(n));
    }
}

/*!
  Generate the index section with the given \a writer for the \a node
  specified, returning true if an element was written; otherwise returns
  false.
 */
bool Tree::generateIndexSection(QXmlStreamWriter& writer,
                                Node* node,
                                bool generateInternalNodes)
{
    if (!node->url().isEmpty())
        return false;

    QString nodeName;
    switch (node->type()) {
    case Node::Namespace:
        nodeName = "namespace";
        break;
    case Node::Class:
        nodeName = "class";
        break;
    case Node::Fake:
        nodeName = "page";
        if (node->subType() == Node::QmlClass)
            nodeName = "qmlclass";
        else if (node->subType() == Node::QmlBasicType)
            nodeName = "qmlbasictype";
        break;
    case Node::Enum:
        nodeName = "enum";
        break;
    case Node::Typedef:
        nodeName = "typedef";
        break;
    case Node::Property:
        nodeName = "property";
        break;
    case Node::Function:
        nodeName = "function";
        break;
    case Node::Variable:
        nodeName = "variable";
        break;
    case Node::QmlProperty:
        nodeName = "qmlproperty";
        break;
    case Node::QmlSignal:
        nodeName = "qmlsignal";
        break;
    case Node::QmlSignalHandler:
        nodeName = "qmlsignalhandler";
        break;
    case Node::QmlMethod:
        nodeName = "qmlmethod";
        break;
    default:
        return false;
    }

    QString access;
    switch (node->access()) {
    case Node::Public:
        access = "public";
        break;
    case Node::Protected:
        access = "protected";
        break;
    case Node::Private:
        // Do not include private non-internal nodes in the index.
        // (Internal public and protected nodes are marked as private
        // by qdoc. We can check their internal status to determine
        // whether they were really private to begin with.)
        if (node->status() == Node::Internal && generateInternalNodes)
            access = "internal";
        else
            return false;
        break;
    default:
        return false;
    }

    QString objName = node->name();

    // Special case: only the root node should have an empty name.
    if (objName.isEmpty() && node != root())
        return false;

    writer.writeStartElement(nodeName);

    QXmlStreamAttributes attributes;
    writer.writeAttribute("access", access);

    if (node->type() != Node::Fake) {
        QString threadSafety;
        switch (node->threadSafeness()) {
        case Node::NonReentrant:
            threadSafety = "non-reentrant";
            break;
        case Node::Reentrant:
            threadSafety = "reentrant";
            break;
        case Node::ThreadSafe:
            threadSafety = "thread safe";
            break;
        case Node::UnspecifiedSafeness:
        default:
            threadSafety = "unspecified";
            break;
        }
        writer.writeAttribute("threadsafety", threadSafety);
    }

    QString status;
    switch (node->status()) {
    case Node::Compat:
        status = "compat";
        break;
    case Node::Obsolete:
        status = "obsolete";
        break;
    case Node::Deprecated:
        status = "deprecated";
        break;
    case Node::Preliminary:
        status = "preliminary";
        break;
    case Node::Commendable:
        status = "commendable";
        break;
    case Node::Internal:
        status = "internal";
        break;
    case Node::Main:
    default:
        status = "main";
        break;
    }
    writer.writeAttribute("status", status);

    writer.writeAttribute("name", objName);
    QString fullName = node->fullDocumentName();
    if (fullName != objName)
        writer.writeAttribute("fullname", fullName);
    QString href = node->outputSubdirectory();
    if (!href.isEmpty())
        href.append(QLatin1Char('/'));
    href.append(Generator::fullDocumentLocation(node));
    writer.writeAttribute("href", href);
    if ((node->type() != Node::Fake) && (!node->isQmlNode()))
        writer.writeAttribute("location", node->location().fileName());

    switch (node->type()) {

    case Node::Class:
    {
        // Classes contain information about their base classes.

        const ClassNode* classNode = static_cast<const ClassNode*>(node);
        QList<RelatedClass> bases = classNode->baseClasses();
        QSet<QString> baseStrings;
        foreach (const RelatedClass& related, bases) {
            ClassNode* baseClassNode = related.node;
            baseStrings.insert(baseClassNode->name());
        }
        writer.writeAttribute("bases", QStringList(baseStrings.toList()).join(","));
        writer.writeAttribute("module", node->moduleName());
    }
        break;

    case Node::Namespace:
        writer.writeAttribute("module", node->moduleName());
        break;

    case Node::Fake:
    {
        /*
              Fake nodes (such as manual pages) contain subtypes,
              titles and other attributes.
            */

        const FakeNode* fakeNode = static_cast<const FakeNode*>(node);
        switch (fakeNode->subType()) {
        case Node::Example:
            writer.writeAttribute("subtype", "example");
            break;
        case Node::HeaderFile:
            writer.writeAttribute("subtype", "header");
            break;
        case Node::File:
            writer.writeAttribute("subtype", "file");
            break;
        case Node::Group:
            writer.writeAttribute("subtype", "group");
            break;
        case Node::Module:
            writer.writeAttribute("subtype", "module");
            break;
        case Node::Page:
            writer.writeAttribute("subtype", "page");
            break;
        case Node::ExternalPage:
            writer.writeAttribute("subtype", "externalpage");
            break;
        case Node::QmlClass:
            //writer.writeAttribute("subtype", "qmlclass");
            break;
        case Node::QmlBasicType:
            //writer.writeAttribute("subtype", "qmlbasictype");
            break;
        default:
            break;
        }
        writer.writeAttribute("title", fakeNode->title());
        writer.writeAttribute("fulltitle", fakeNode->fullTitle());
        writer.writeAttribute("subtitle", fakeNode->subTitle());
        writer.writeAttribute("location", fakeNode->doc().location().fileName());
    }
        break;

    case Node::Function:
    {
        /*
              Function nodes contain information about the type of
              function being described.
            */

        const FunctionNode* functionNode =
                static_cast<const FunctionNode*>(node);

        switch (functionNode->virtualness()) {
        case FunctionNode::NonVirtual:
            writer.writeAttribute("virtual", "non");
            break;
        case FunctionNode::ImpureVirtual:
            writer.writeAttribute("virtual", "impure");
            break;
        case FunctionNode::PureVirtual:
            writer.writeAttribute("virtual", "pure");
            break;
        default:
            break;
        }
        switch (functionNode->metaness()) {
        case FunctionNode::Plain:
            writer.writeAttribute("meta", "plain");
            break;
        case FunctionNode::Signal:
            writer.writeAttribute("meta", "signal");
            break;
        case FunctionNode::Slot:
            writer.writeAttribute("meta", "slot");
            break;
        case FunctionNode::Ctor:
            writer.writeAttribute("meta", "constructor");
            break;
        case FunctionNode::Dtor:
            writer.writeAttribute("meta", "destructor");
            break;
        case FunctionNode::MacroWithParams:
            writer.writeAttribute("meta", "macrowithparams");
            break;
        case FunctionNode::MacroWithoutParams:
            writer.writeAttribute("meta", "macrowithoutparams");
            break;
        default:
            break;
        }
        writer.writeAttribute("const", functionNode->isConst()?"true":"false");
        writer.writeAttribute("static", functionNode->isStatic()?"true":"false");
        writer.writeAttribute("overload", functionNode->isOverload()?"true":"false");
        if (functionNode->isOverload())
            writer.writeAttribute("overload-number", QString::number(functionNode->overloadNumber()));
        if (functionNode->relates())
            writer.writeAttribute("relates", functionNode->relates()->name());
        const PropertyNode* propertyNode = functionNode->associatedProperty();
        if (propertyNode)
            writer.writeAttribute("associated-property", propertyNode->name());
        writer.writeAttribute("type", functionNode->returnType());
    }
        break;

    case Node::QmlProperty:
    {
        QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
        writer.writeAttribute("type", qpn->dataType());
        writer.writeAttribute("attached", qpn->isAttached() ? "true" : "false");
        writer.writeAttribute("writable", qpn->isWritable(this) ? "true" : "false");
    }
        break;
    case Node::Property:
    {
        const PropertyNode* propertyNode = static_cast<const PropertyNode*>(node);
        writer.writeAttribute("type", propertyNode->dataType());
        foreach (const Node* fnNode, propertyNode->getters()) {
            if (fnNode) {
                const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                writer.writeStartElement("getter");
                writer.writeAttribute("name", functionNode->name());
                writer.writeEndElement(); // getter
            }
        }
        foreach (const Node* fnNode, propertyNode->setters()) {
            if (fnNode) {
                const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                writer.writeStartElement("setter");
                writer.writeAttribute("name", functionNode->name());
                writer.writeEndElement(); // setter
            }
        }
        foreach (const Node* fnNode, propertyNode->resetters()) {
            if (fnNode) {
                const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                writer.writeStartElement("resetter");
                writer.writeAttribute("name", functionNode->name());
                writer.writeEndElement(); // resetter
            }
        }
        foreach (const Node* fnNode, propertyNode->notifiers()) {
            if (fnNode) {
                const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                writer.writeStartElement("notifier");
                writer.writeAttribute("name", functionNode->name());
                writer.writeEndElement(); // notifier
            }
        }
    }
        break;

    case Node::Variable:
    {
        const VariableNode* variableNode =
                static_cast<const VariableNode*>(node);
        writer.writeAttribute("type", variableNode->dataType());
        writer.writeAttribute("static",
                              variableNode->isStatic() ? "true" : "false");
    }
        break;
    default:
        break;
    }

    // Inner nodes and function nodes contain child nodes of some sort, either
    // actual child nodes or function parameters. For these, we close the
    // opening tag, create child elements, then add a closing tag for the
    // element. Elements for all other nodes are closed in the opening tag.

    if (node->isInnerNode()) {

        const InnerNode* inner = static_cast<const InnerNode*>(node);

        // For internal pages, we canonicalize the target, keyword and content
        // item names so that they can be used by qdoc for other sets of
        // documentation.
        // The reason we do this here is that we don't want to ruin
        // externally composed indexes, containing non-qdoc-style target names
        // when reading in indexes.

        if (inner->doc().hasTargets()) {
            bool external = false;
            if (inner->type() == Node::Fake) {
                const FakeNode* fakeNode = static_cast<const FakeNode*>(inner);
                if (fakeNode->subType() == Node::ExternalPage)
                    external = true;
            }

            foreach (const Atom* target, inner->doc().targets()) {
                QString targetName = target->string();
                if (!external)
                    targetName = Doc::canonicalTitle(targetName);

                writer.writeStartElement("target");
                writer.writeAttribute("name", targetName);
                writer.writeEndElement(); // target
            }
        }
        if (inner->doc().hasKeywords()) {
            foreach (const Atom* keyword, inner->doc().keywords()) {
                writer.writeStartElement("keyword");
                writer.writeAttribute("name",
                                      Doc::canonicalTitle(keyword->string()));
                writer.writeEndElement(); // keyword
            }
        }
        if (inner->doc().hasTableOfContents()) {
            for (int i = 0; i < inner->doc().tableOfContents().size(); ++i) {
                Atom* item = inner->doc().tableOfContents()[i];
                int level = inner->doc().tableOfContentsLevels()[i];

                QString title = Text::sectionHeading(item).toString();
                writer.writeStartElement("contents");
                writer.writeAttribute("name", Doc::canonicalTitle(title));
                writer.writeAttribute("title", title);
                writer.writeAttribute("level", QString::number(level));
                writer.writeEndElement(); // contents
            }
        }

    }
    else if (node->type() == Node::Function) {

        const FunctionNode* functionNode = static_cast<const FunctionNode*>(node);
        // Write a signature attribute for convenience.
        QStringList signatureList;
        QStringList resolvedParameters;

        foreach (const Parameter& parameter, functionNode->parameters()) {
            QString leftType = parameter.leftType();
            const Node* leftNode = const_cast<Tree*>(this)->findNode(parameter.leftType().split("::"),
                                                      0, SearchBaseClasses|NonFunction);
            if (!leftNode || leftNode->type() != Node::Typedef) {
                leftNode = const_cast<Tree*>(this)->findNode(parameter.leftType().split("::"),
                            node->parent(), SearchBaseClasses|NonFunction);
            }
            if (leftNode && leftNode->type() == Node::Typedef) {
                if (leftNode->type() == Node::Typedef) {
                    const TypedefNode* typedefNode =
                            static_cast<const TypedefNode*>(leftNode);
                    if (typedefNode->associatedEnum()) {
                        leftType = "QFlags<" + typedefNode->associatedEnum()->fullDocumentName() + QLatin1Char('>');
                    }
                }
                else
                    leftType = leftNode->fullDocumentName();
            }
            resolvedParameters.append(leftType);
            signatureList.append(leftType + QLatin1Char(' ') + parameter.name());
        }

        QString signature = functionNode->name()+QLatin1Char('(')+signatureList.join(", ")+QLatin1Char(')');
        if (functionNode->isConst())
            signature += " const";
        writer.writeAttribute("signature", signature);

        for (int i = 0; i < functionNode->parameters().size(); ++i) {
            Parameter parameter = functionNode->parameters()[i];
            writer.writeStartElement("parameter");
            writer.writeAttribute("left", resolvedParameters[i]);
            writer.writeAttribute("right", parameter.rightType());
            writer.writeAttribute("name", parameter.name());
            writer.writeAttribute("default", parameter.defaultValue());
            writer.writeEndElement(); // parameter
        }

    }
    else if (node->type() == Node::Enum) {

        const EnumNode* enumNode = static_cast<const EnumNode*>(node);
        if (enumNode->flagsType()) {
            writer.writeAttribute("typedef",enumNode->flagsType()->fullDocumentName());
        }
        foreach (const EnumItem& item, enumNode->items()) {
            writer.writeStartElement("value");
            writer.writeAttribute("name", item.name());
            writer.writeAttribute("value", item.value());
            writer.writeEndElement(); // value
        }

    }
    else if (node->type() == Node::Typedef) {

        const TypedefNode* typedefNode = static_cast<const TypedefNode*>(node);
        if (typedefNode->associatedEnum()) {
            writer.writeAttribute("enum",typedefNode->associatedEnum()->fullDocumentName());
        }
    }

    return true;
}


/*!
    Returns true if the node \a n1 is less than node \a n2.
    The comparison is performed by comparing properties of the nodes in order
    of increasing complexity.
*/
bool compareNodes(const Node* n1, const Node* n2)
{
    // Private nodes can occur in any order since they won't normally be
    // written to the index.
    if (n1->access() == Node::Private && n2->access() == Node::Private)
        return true;

    if (n1->location().filePath() < n2->location().filePath())
        return true;
    else if (n1->location().filePath() > n2->location().filePath())
        return false;

    if (n1->type() < n2->type())
        return true;
    else if (n1->type() > n2->type())
        return false;

    if (n1->name() < n2->name())
        return true;
    else if (n1->name() > n2->name())
        return false;

    if (n1->access() < n2->access())
        return true;
    else if (n1->access() > n2->access())
        return false;

    if (n1->type() == Node::Function && n2->type() == Node::Function) {
        const FunctionNode* f1 = static_cast<const FunctionNode*>(n1);
        const FunctionNode* f2 = static_cast<const FunctionNode*>(n2);

        if (f1->isConst() < f2->isConst())
            return true;
        else if (f1->isConst() > f2->isConst())
            return false;

        if (f1->signature() < f2->signature())
            return true;
        else if (f1->signature() > f2->signature())
            return false;
    }

    if (n1->type() == Node::Fake && n2->type() == Node::Fake) {
        const FakeNode* f1 = static_cast<const FakeNode*>(n1);
        const FakeNode* f2 = static_cast<const FakeNode*>(n2);
        if (f1->fullTitle() < f2->fullTitle())
            return true;
        else if (f1->fullTitle() > f2->fullTitle())
            return false;
    }

    return false;
}

/*!
    Generate index sections for the child nodes of the given \a node
    using the \a writer specified. If \a generateInternalNodes is true,
    nodes marked as internal will be included in the index; otherwise,
    they will be omitted.
*/
void Tree::generateIndexSections(QXmlStreamWriter& writer,
                                 Node* node,
                                 bool generateInternalNodes)
{
    if (generateIndexSection(writer, node, generateInternalNodes)) {

        if (node->isInnerNode()) {
            const InnerNode* inner = static_cast<const InnerNode*>(node);

            NodeList cnodes = inner->childNodes();
            qSort(cnodes.begin(), cnodes.end(), compareNodes);

            foreach (Node* child, cnodes) {
                /*
                  Don't generate anything for a QML property group node.
                  It is just a place holder for a collection of QML property
                  nodes. Recurse to its children, which are the QML property
                  nodes.
                 */
                if (child->subType() == Node::QmlPropertyGroup) {
                    const InnerNode* pgn = static_cast<const InnerNode*>(child);
                    foreach (Node* c, pgn->childNodes()) {
                        generateIndexSections(writer, c, generateInternalNodes);
                    }
                }
                else
                    generateIndexSections(writer, child, generateInternalNodes);
            }

            /*
            foreach (const Node* child, inner->relatedNodes()) {
                QDomElement childElement = generateIndexSections(document, child);
                element.appendChild(childElement);
            }
*/
        }
        writer.writeEndElement();
    }
}

/*!
  Outputs an index file.
 */
void Tree::generateIndex(const QString& fileName,
                         const QString& url,
                         const QString& title,
                         bool generateInternalNodes)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return ;

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeDTD("<!DOCTYPE QDOCINDEX>");

    writer.writeStartElement("INDEX");
    writer.writeAttribute("url", url);
    writer.writeAttribute("title", title);
    writer.writeAttribute("version", version());

    generateIndexSections(writer, root(), generateInternalNodes);

    writer.writeEndElement(); // INDEX
    writer.writeEndElement(); // QDOCINDEX
    writer.writeEndDocument();
    file.close();
}

/*!
  Generate the tag file section with the given \a writer for the \a node
  specified, returning true if an element was written; otherwise returns
  false.
 */
void Tree::generateTagFileCompounds(QXmlStreamWriter& writer, const InnerNode* inner)
{
    foreach (const Node* node, inner->childNodes()) {

        if (!node->url().isEmpty())
            continue;

        QString kind;
        switch (node->type()) {
        case Node::Namespace:
            kind = "namespace";
            break;
        case Node::Class:
            kind = "class";
            break;
        case Node::Enum:
        case Node::Typedef:
        case Node::Property:
        case Node::Function:
        case Node::Variable:
        default:
            continue;
        }

        QString access;
        switch (node->access()) {
        case Node::Public:
            access = "public";
            break;
        case Node::Protected:
            access = "protected";
            break;
        case Node::Private:
        default:
            continue;
        }

        QString objName = node->name();

        // Special case: only the root node should have an empty name.
        if (objName.isEmpty() && node != root())
            continue;

        // *** Write the starting tag for the element here. ***
        writer.writeStartElement("compound");
        writer.writeAttribute("kind", kind);

        if (node->type() == Node::Class) {
            writer.writeTextElement("name", node->fullDocumentName());
            writer.writeTextElement("filename", Generator::fullDocumentLocation(node,true));

            // Classes contain information about their base classes.
            const ClassNode* classNode = static_cast<const ClassNode*>(node);
            QList<RelatedClass> bases = classNode->baseClasses();
            foreach (const RelatedClass& related, bases) {
                ClassNode* baseClassNode = related.node;
                writer.writeTextElement("base", baseClassNode->name());
            }

            // Recurse to write all members.
            generateTagFileMembers(writer, static_cast<const InnerNode*>(node));
            writer.writeEndElement();

            // Recurse to write all compounds.
            generateTagFileCompounds(writer, static_cast<const InnerNode*>(node));
        } else {
            writer.writeTextElement("name", node->fullDocumentName());
            writer.writeTextElement("filename", Generator::fullDocumentLocation(node,true));

            // Recurse to write all members.
            generateTagFileMembers(writer, static_cast<const InnerNode*>(node));
            writer.writeEndElement();

            // Recurse to write all compounds.
            generateTagFileCompounds(writer, static_cast<const InnerNode*>(node));
        }
    }
}

/*!
 */
void Tree::generateTagFileMembers(QXmlStreamWriter& writer, const InnerNode* inner)
{
    foreach (const Node* node, inner->childNodes()) {

        if (!node->url().isEmpty())
            continue;

        QString nodeName;
        QString kind;
        switch (node->type()) {
        case Node::Enum:
            nodeName = "member";
            kind = "enum";
            break;
        case Node::Typedef:
            nodeName = "member";
            kind = "typedef";
            break;
        case Node::Property:
            nodeName = "member";
            kind = "property";
            break;
        case Node::Function:
            nodeName = "member";
            kind = "function";
            break;
        case Node::Namespace:
            nodeName = "namespace";
            break;
        case Node::Class:
            nodeName = "class";
            break;
        case Node::Variable:
        default:
            continue;
        }

        QString access;
        switch (node->access()) {
        case Node::Public:
            access = "public";
            break;
        case Node::Protected:
            access = "protected";
            break;
        case Node::Private:
        default:
            continue;
        }

        QString objName = node->name();

        // Special case: only the root node should have an empty name.
        if (objName.isEmpty() && node != root())
            continue;

        // *** Write the starting tag for the element here. ***
        writer.writeStartElement(nodeName);
        if (!kind.isEmpty())
            writer.writeAttribute("kind", kind);

        switch (node->type()) {

        case Node::Class:
            writer.writeCharacters(node->fullDocumentName());
            writer.writeEndElement();
            break;
        case Node::Namespace:
            writer.writeCharacters(node->fullDocumentName());
            writer.writeEndElement();
            break;
        case Node::Function:
        {
            /*
                  Function nodes contain information about
                  the type of function being described.
                */

            const FunctionNode* functionNode =
                    static_cast<const FunctionNode*>(node);
            writer.writeAttribute("protection", access);

            switch (functionNode->virtualness()) {
            case FunctionNode::NonVirtual:
                writer.writeAttribute("virtualness", "non");
                break;
            case FunctionNode::ImpureVirtual:
                writer.writeAttribute("virtualness", "virtual");
                break;
            case FunctionNode::PureVirtual:
                writer.writeAttribute("virtual", "pure");
                break;
            default:
                break;
            }
            writer.writeAttribute("static",
                                  functionNode->isStatic() ? "yes" : "no");

            if (functionNode->virtualness() == FunctionNode::NonVirtual)
                writer.writeTextElement("type", functionNode->returnType());
            else
                writer.writeTextElement("type",
                                        "virtual " + functionNode->returnType());

            writer.writeTextElement("name", objName);
            QStringList pieces = Generator::fullDocumentLocation(node,true).split(QLatin1Char('#'));
            writer.writeTextElement("anchorfile", pieces[0]);
            writer.writeTextElement("anchor", pieces[1]);

            // Write a signature attribute for convenience.
            QStringList signatureList;

            foreach (const Parameter& parameter, functionNode->parameters()) {
                QString leftType = parameter.leftType();
                const Node* leftNode = const_cast<Tree*>(this)->findNode(parameter.leftType().split("::"),
                                                                         0, SearchBaseClasses|NonFunction);
                if (!leftNode || leftNode->type() != Node::Typedef) {
                    leftNode = const_cast<Tree*>(this)->findNode(parameter.leftType().split("::"),
                                                      node->parent(), SearchBaseClasses|NonFunction);
                }
                if (leftNode && leftNode->type() == Node::Typedef) {
                    const TypedefNode* typedefNode = static_cast<const TypedefNode*>(leftNode);
                    if (typedefNode->associatedEnum()) {
                        leftType = "QFlags<" + typedefNode->associatedEnum()->fullDocumentName() + QLatin1Char('>');
                    }
                }
                signatureList.append(leftType + QLatin1Char(' ') + parameter.name());
            }

            QString signature = QLatin1Char('(')+signatureList.join(", ")+QLatin1Char(')');
            if (functionNode->isConst())
                signature += " const";
            if (functionNode->virtualness() == FunctionNode::PureVirtual)
                signature += " = 0";
            writer.writeTextElement("arglist", signature);
        }
            writer.writeEndElement(); // member
            break;

        case Node::Property:
        {
            const PropertyNode* propertyNode = static_cast<const PropertyNode*>(node);
            writer.writeAttribute("type", propertyNode->dataType());
            writer.writeTextElement("name", objName);
            QStringList pieces = Generator::fullDocumentLocation(node,true).split(QLatin1Char('#'));
            writer.writeTextElement("anchorfile", pieces[0]);
            writer.writeTextElement("anchor", pieces[1]);
            writer.writeTextElement("arglist", "");
        }
            writer.writeEndElement(); // member
            break;

        case Node::Enum:
        {
            const EnumNode* enumNode = static_cast<const EnumNode*>(node);
            writer.writeTextElement("name", objName);
            QStringList pieces = Generator::fullDocumentLocation(node).split(QLatin1Char('#'));
            writer.writeTextElement("anchor", pieces[1]);
            writer.writeTextElement("arglist", "");
            writer.writeEndElement(); // member

            for (int i = 0; i < enumNode->items().size(); ++i) {
                EnumItem item = enumNode->items().value(i);
                writer.writeStartElement("member");
                writer.writeAttribute("name", item.name());
                writer.writeTextElement("anchor", pieces[1]);
                writer.writeTextElement("arglist", "");
                writer.writeEndElement(); // member
            }
        }
            break;

        case Node::Typedef:
        {
            const TypedefNode* typedefNode = static_cast<const TypedefNode*>(node);
            if (typedefNode->associatedEnum())
                writer.writeAttribute("type", typedefNode->associatedEnum()->fullDocumentName());
            else
                writer.writeAttribute("type", "");
            writer.writeTextElement("name", objName);
            QStringList pieces = Generator::fullDocumentLocation(node,true).split(QLatin1Char('#'));
            writer.writeTextElement("anchorfile", pieces[0]);
            writer.writeTextElement("anchor", pieces[1]);
            writer.writeTextElement("arglist", "");
        }
            writer.writeEndElement(); // member
            break;

        case Node::Variable:
        default:
            break;
        }
    }
}

/*!
  Writes a tag file named \a fileName.
 */
void Tree::generateTagFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return ;

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();

    writer.writeStartElement("tagfile");

    generateTagFileCompounds(writer, root());

    writer.writeEndElement(); // tagfile
    writer.writeEndDocument();
    file.close();
}

/*!
 */
void Tree::addExternalLink(const QString& url, const Node* relative)
{
    FakeNode* fakeNode = new FakeNode(root(), url, Node::ExternalPage, Node::ArticlePage);
    fakeNode->setAccess(Node::Public);

    // Create some content for the node.
    QSet<QString> emptySet;
    Location location(relative->doc().location());
    Doc doc(location, location, " ", emptySet); // placeholder
    fakeNode->setDoc(doc);
}

/*!
  Find the node with the specified \a path name that is of
  the specified \a type and \a subtype. Begin the search at
  the \a start node. If the \a start node is 0, begin the
  search at the tree root. \a subtype is not used unless
  \a type is \c{Fake}.
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
  type is \e{Fake}, the \a subtype must match as well.

  If the algorithm is successful, the pointer to the final
  node is returned. Otherwise 0 is returned.
 */
Node* Tree::findNodeRecursive(const QStringList& path,
                              int pathIndex,
                              Node* start,
                              Node::Type type,
                              Node::SubType subtype,
                              bool acceptCollision)
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
                    if (type == Node::Fake) {
                        if (n->subType() == subtype)
                            return n;
                        else if (n->subType() == Node::Collision && acceptCollision)
                            return n;
                        else if (subtype == Node::NoSubType)
                            return n; // don't care what subtype is.
                        return 0;
                    }
                    else
                        return n;
                }
                else if (n->isCollisionNode()) {
                    if (acceptCollision)
                        return n;
                    return findNodeRecursive(path, pathIndex, n, type, subtype);
                }
                else
                    return 0;
            }
            else { // Not at the end of the path.
                n = findNodeRecursive(path, pathIndex+1, n, type, subtype);
                if (n)
                    return n;
            }
        }
    }
    return 0;
}

/*!
  Find the Enum type node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only an Enum type node named \a path is
  acceptible. If one is not found, 0 is returned.
 */
EnumNode* Tree::findEnumNode(const QStringList& path, Node* start)
{
    if (!start)
        start = const_cast<NamespaceNode*>(root());
    return static_cast<EnumNode*>(findNodeRecursive(path, 0, start, Node::Enum, Node::NoSubType));
}

/*!
  Find the C++ class node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only a C++ class node named \a path is
  acceptible. If one is not found, 0 is returned.
 */
ClassNode* Tree::findClassNode(const QStringList& path, Node* start)
{
    if (!start)
        start = const_cast<NamespaceNode*>(root());
    return static_cast<ClassNode*>(findNodeRecursive(path, 0, start, Node::Class, Node::NoSubType));
}

/*!
  Find the Qml class node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only a Qml class node named \a path is
  acceptible. If one is not found, 0 is returned.
 */
QmlClassNode* Tree::findQmlClassNode(const QStringList& path, Node* start)
{
    /*
      If the path contains one or two double colons ("::"),
      check first to see if the first two path strings refer
      to a QML element. If they do, path[0] will be the QML
      module identifier, and path[1] will be the QML type.
      If the anser is yes, the reference identifies a QML
      class node.
    */
    if (path.size() >= 2) {
        QmlClassNode* qcn = QmlClassNode::lookupQmlTypeNode(path[0], path[1]);
        if (qcn)
            return qcn;
    }

    if (!start)
        start = const_cast<NamespaceNode*>(root());
    return static_cast<QmlClassNode*>(findNodeRecursive(path, 0, start, Node::Fake, Node::QmlClass));
}

/*!
  Find the Namespace node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only a Namespace node named \a path is
  acceptible. If one is not found, 0 is returned.
 */
NamespaceNode* Tree::findNamespaceNode(const QStringList& path, Node* start)
{
    if (!start)
        start = const_cast<NamespaceNode*>(root());
    return static_cast<NamespaceNode*>(findNodeRecursive(path, 0, start, Node::Namespace, Node::NoSubType));
}

/*!
  Find the Group node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only a Group node named \a path is
  acceptible. If one is not found, 0 is returned.
 */
FakeNode* Tree::findGroupNode(const QStringList& path, Node* start)
{
    if (!start)
        start = const_cast<NamespaceNode*>(root());
    return static_cast<FakeNode*>(findNodeRecursive(path, 0, start, Node::Fake, Node::Group));
}

/*!
  Find the Qml module node named \a path. Begin the search at the
  \a start node. If the \a start node is 0, begin the search
  at the root of the tree. Only a Qml module node named \a path is
  acceptible. If one is not found, 0 is returned.
 */
FakeNode* Tree::findQmlModuleNode(const QStringList& path, Node* start)
{
    if (!start)
        start = const_cast<NamespaceNode*>(root());
    return static_cast<FakeNode*>(findNodeRecursive(path, 0, start, Node::Fake, Node::QmlModule));
}

QT_END_NAMESPACE
