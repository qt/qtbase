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

#include "tree.h"
#include "qdocdatabase.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static NodeMap emptyNodeMap_;
static NodeMultiMap emptyNodeMultiMap_;

/*! \class QDocDatabase
 */

QDocDatabase* QDocDatabase::qdocDB_ = NULL;

/*!
  Constructs the singleton qdoc database object.
  It constructs a singleton Tree object with this
  qdoc database pointer.
 */
QDocDatabase::QDocDatabase()
{
    tree_ = new Tree(this);
}

/*!
  Destroys the qdoc database object. This requires deleting
  the tree of nodes, which deletes each node.
 */
QDocDatabase::~QDocDatabase()
{
    masterMap_.clear();
    delete tree_;
}

/*! \fn Tree* QDocDatabase::tree()
  Returns the pointer to the tree. This function is for compatibility
  with the current qdoc. It will be removed when the QDocDatabase class
  replaces the current structures.
 */

/*!
  Creates the singleton. Allows only one instance of the class
  to be created. Returns a pointer to the singleton.
*/
QDocDatabase* QDocDatabase::qdocDB()
{
   if (!qdocDB_)
      qdocDB_ = new QDocDatabase;
   return qdocDB_;
}

/*!
  Destroys the singleton.
 */
void QDocDatabase::destroyQdocDB()
{
    if (qdocDB_) {
        delete qdocDB_;
        qdocDB_ = 0;
    }
}

/*!
  \fn const DocNodeMap& QDocDatabase::modules() const
  Returns a const reference to the collection of all
  module nodes.
*/

/*!
  \fn const DocNodeMap& QDocDatabase::qmlModules() const
  Returns a const reference to the collection of all
  QML module nodes.
*/

/*!
  Looks up the module node named \a name in the collection
  of all module nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new module node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned.
 */
DocNode* QDocDatabase::addModule(const QString& name)
{
    return findModule(name,true);
}

/*!
  Looks up the QML module node named \a name in the collection
  of all QML module nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new QML module node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned.
 */
DocNode* QDocDatabase::addQmlModule(const QString& name)
{
    return findQmlModule(name,true);
}

/*!
  Looks up the C++ module named \a moduleName. If it isn't
  there, create it. Then append \a node to the module's child
  list. The parent of \a node is not changed by this function.
  Returns the module node.
 */
DocNode* QDocDatabase::addToModule(const QString& moduleName, Node* node)
{
    DocNode* dn = findModule(moduleName,true);
    dn->addMember(node);
    node->setModuleName(moduleName);
    return dn;
}

/*!
  Looks up the QML module named \a qmlModuleName. If it isn't
  there, create it. Then append \a node to the module's child
  list. The parent of \a node is not changed by this function.
  Returns a pointer to the QML module node.
 */
DocNode* QDocDatabase::addToQmlModule(const QString& qmlModuleName, Node* node)
{
    DocNode* dn = findQmlModule(qmlModuleName,true);
    dn->addMember(node);
    node->setQmlModuleInfo(qmlModuleName);
    if (node->subType() == Node::QmlClass) {
        QString t = node->qmlModuleIdentifier() + "::" + node->name();
        QmlClassNode* n = static_cast<QmlClassNode*>(node);
        if (!qmlTypeMap_.contains(t))
            qmlTypeMap_.insert(t,n);
        if (!masterMap_.contains(t))
            masterMap_.insert(t,node);
        if (!masterMap_.contains(node->name(),node))
            masterMap_.insert(node->name(),node);
    }
    return dn;
}

/*!
  Find the module node named \a name and return a pointer
  to it. If a matching node is not found and \a addIfNotFound
  is true, add a new module node named \a name and return
  a pointer to that one. Otherwise, return 0.

  If a new module node is added, its parent is the tree root,
  but the new module node is not added to the child list of the
  tree root.
 */
DocNode* QDocDatabase::findModule(const QString& name, bool addIfNotFound)
{
    DocNodeMap::const_iterator i = modules_.find(name);
    if (i != modules_.end()) {
        return i.value();
    }
    if (addIfNotFound) {
        DocNode* dn = new DocNode(tree_->root(), name, Node::Module, Node::OverviewPage);
        modules_.insert(name,dn);
        if (!masterMap_.contains(name,dn))
            masterMap_.insert(name,dn);
        return dn;
    }
    return 0;
}

/*!
  Find the QML module node named \a name and return a pointer
  to it. If a matching node is not found and \a addIfNotFound
  is true, add a new QML module node named \a name and return
  a pointer to that one. Otherwise, return 0.

  If a new QML module node is added, its parent is the tree root,
  but the new QML module node is not added to the child list of
  the tree root.
 */
DocNode* QDocDatabase::findQmlModule(const QString& name, bool addIfNotFound)
{
    QStringList dotSplit;
    QStringList blankSplit = name.split(QLatin1Char(' '));
    QString qmid = blankSplit[0];
    if (blankSplit.size() > 1) {
        dotSplit = blankSplit[1].split(QLatin1Char('.'));
        qmid += dotSplit[0];
    }
    DocNode* dn = 0;
    if (qmlModules_.contains(qmid))
        dn = qmlModules_.value(qmid);
    else if (addIfNotFound) {
        dn = new DocNode(tree_->root(), name, Node::QmlModule, Node::OverviewPage);
        dn->setQmlModuleInfo(name);
        qmlModules_.insert(qmid,dn);
        masterMap_.insert(qmid,dn);
        masterMap_.insert(dn->name(),dn);
    }
    return dn;
}

/*!
  Looks up the QML type node identified by the Qml module id
  \a qmid and QML type \a name and returns a pointer to the
  QML type node. The key is \a qmid + "::" + \a name.

  If the QML module id is empty, it looks up the QML type by
  \a name only.
 */
QmlClassNode* QDocDatabase::findQmlType(const QString& qmid, const QString& name) const
{
    if (!qmid.isEmpty())
        return qmlTypeMap_.value(qmid + "::" + name);

    QStringList path(name);
    Node* n = tree_->findNodeByNameAndType(path, Node::Document, Node::QmlClass, 0, true);
    if (n) {
        if (n->subType() == Node::QmlClass)
            return static_cast<QmlClassNode*>(n);
        else if (n->subType() == Node::Collision) {
            NameCollisionNode* ncn;
            ncn = static_cast<NameCollisionNode*>(n);
            return static_cast<QmlClassNode*>(ncn->findAny(Node::Document,Node::QmlClass));
        }
    }
    return 0;

}

/*!
  For debugging only.
 */
void QDocDatabase::printModules() const
{
    DocNodeMap::const_iterator i = modules_.begin();
    while (i != modules_.end()) {
        qDebug() << "  " << i.key();
        ++i;
    }
}

/*!
  For debugging only.
 */
void QDocDatabase::printQmlModules() const
{
    DocNodeMap::const_iterator i = qmlModules_.begin();
    while (i != qmlModules_.end()) {
        qDebug() << "  " << i.key();
        ++i;
    }
}

/*!
  Traverses the database to construct useful data structures
  for use when outputting certain significant collections of
  things, C++ classes, QML types, "since" lists, and other
  stuff.
 */
void QDocDatabase::buildCollections()
{
    nonCompatClasses_.clear();
    mainClasses_.clear();
    compatClasses_.clear();
    obsoleteClasses_.clear();
    funcIndex_.clear();
    legaleseTexts_.clear();
    serviceClasses_.clear();
    qmlClasses_.clear();

    findAllClasses(treeRoot());
    findAllFunctions(treeRoot());
    findAllLegaleseTexts(treeRoot());
    findAllNamespaces(treeRoot());
    findAllSince(treeRoot());
}

/*!
  Finds all the C++ class nodes and QML type nodes and
  sorts them into maps.
 */
void QDocDatabase::findAllClasses(const InnerNode* node)
{
    NodeList::const_iterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private && (*c)->url().isEmpty()) {
            if ((*c)->type() == Node::Class && !(*c)->doc().isEmpty()) {
                QString className = (*c)->name();
                if ((*c)->parent() &&
                        (*c)->parent()->type() == Node::Namespace &&
                        !(*c)->parent()->name().isEmpty())
                    className = (*c)->parent()->name()+"::"+className;

                if (!(static_cast<const ClassNode *>(*c))->hideFromMainList()) {
                    if ((*c)->status() == Node::Compat) {
                        compatClasses_.insert(className, *c);
                    }
                    else if ((*c)->status() == Node::Obsolete) {
                        obsoleteClasses_.insert(className, *c);
                    }
                    else {
                        nonCompatClasses_.insert(className, *c);
                        if ((*c)->status() == Node::Main)
                            mainClasses_.insert(className, *c);
                    }
                }

                QString serviceName = (static_cast<const ClassNode *>(*c))->serviceName();
                if (!serviceName.isEmpty())
                    serviceClasses_.insert(serviceName, *c);
            }
            else if ((*c)->type() == Node::Document &&
                     (*c)->subType() == Node::QmlClass &&
                     !(*c)->doc().isEmpty()) {
                QString qmlTypeName = (*c)->name();
                if (qmlTypeName.startsWith(QLatin1String("QML:")))
                    qmlClasses_.insert(qmlTypeName.mid(4),*c);
                else
                    qmlClasses_.insert(qmlTypeName,*c);
            }
            else if ((*c)->isInnerNode()) {
                findAllClasses(static_cast<InnerNode*>(*c));
            }
        }
        ++c;
    }
}

/*!
  Finds all the function nodes
 */
void QDocDatabase::findAllFunctions(const InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode() && (*c)->url().isEmpty()) {
                findAllFunctions(static_cast<const InnerNode*>(*c));
            }
            else if ((*c)->type() == Node::Function) {
                const FunctionNode* func = static_cast<const FunctionNode*>(*c);
                if ((func->status() > Node::Obsolete) &&
                        !func->isInternal() &&
                        (func->metaness() != FunctionNode::Ctor) &&
                        (func->metaness() != FunctionNode::Dtor)) {
                    funcIndex_[(*c)->name()].insert((*c)->parent()->fullDocumentName(), *c);
                }
            }
        }
        ++c;
    }
}

/*!
  Finds all the nodes containing legalese text and puts them
  in a map.
 */
void QDocDatabase::findAllLegaleseTexts(const InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if (!(*c)->doc().legaleseText().isEmpty())
                legaleseTexts_.insertMulti((*c)->doc().legaleseText(), *c);
            if ((*c)->isInnerNode())
                findAllLegaleseTexts(static_cast<const InnerNode *>(*c));
        }
        ++c;
    }
}

/*!
  Finds all the namespace nodes and puts them in an index.
 */
void QDocDatabase::findAllNamespaces(const InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode() && (*c)->url().isEmpty()) {
                findAllNamespaces(static_cast<const InnerNode *>(*c));
                if ((*c)->type() == Node::Namespace) {
                    const NamespaceNode* nspace = static_cast<const NamespaceNode *>(*c);
                    // Ensure that the namespace's name is not empty (the root
                    // namespace has no name).
                    if (!nspace->name().isEmpty()) {
                        namespaceIndex_.insert(nspace->name(), *c);
                    }
                }
            }
        }
        ++c;
    }
}

/*!
  Finds all the nodes where a \e{since} command appeared in the
  qdoc comment and sorts them into maps according to the kind of
  node.

  This function is used for generating the "New Classes... in x.y"
  section on the \e{What's New in Qt x.y} page.
 */
void QDocDatabase::findAllSince(const InnerNode* node)
{
    NodeList::const_iterator child = node->childNodes().constBegin();
    while (child != node->childNodes().constEnd()) {
        QString sinceString = (*child)->since();
        // Insert a new entry into each map for each new since string found.
        if (((*child)->access() != Node::Private) && !sinceString.isEmpty()) {
            NodeMultiMapMap::iterator nsmap = newSinceMaps_.find(sinceString);
            if (nsmap == newSinceMaps_.end())
                nsmap = newSinceMaps_.insert(sinceString,NodeMultiMap());

            NodeMapMap::iterator ncmap = newClassMaps_.find(sinceString);
            if (ncmap == newClassMaps_.end())
                ncmap = newClassMaps_.insert(sinceString,NodeMap());

            NodeMapMap::iterator nqcmap = newQmlTypeMaps_.find(sinceString);
            if (nqcmap == newQmlTypeMaps_.end())
                nqcmap = newQmlTypeMaps_.insert(sinceString,NodeMap());

            if ((*child)->type() == Node::Function) {
                // Insert functions into the general since map.
                FunctionNode *func = static_cast<FunctionNode *>(*child);
                if ((func->status() > Node::Obsolete) &&
                    (func->metaness() != FunctionNode::Ctor) &&
                    (func->metaness() != FunctionNode::Dtor)) {
                    nsmap.value().insert(func->name(),(*child));
                }
            }
            else if ((*child)->url().isEmpty()) {
                if ((*child)->type() == Node::Class && !(*child)->doc().isEmpty()) {
                    // Insert classes into the since and class maps.
                    QString className = (*child)->name();
                    if ((*child)->parent() && (*child)->parent()->type() == Node::Namespace &&
                        !(*child)->parent()->name().isEmpty()) {
                        className = (*child)->parent()->name()+"::"+className;
                    }
                    nsmap.value().insert(className,(*child));
                    ncmap.value().insert(className,(*child));
                }
                else if ((*child)->subType() == Node::QmlClass) {
                    // Insert QML elements into the since and element maps.
                    QString className = (*child)->name();
                    if ((*child)->parent() && (*child)->parent()->type() == Node::Namespace &&
                        !(*child)->parent()->name().isEmpty()) {
                        className = (*child)->parent()->name()+"::"+className;
                    }
                    nsmap.value().insert(className,(*child));
                    nqcmap.value().insert(className,(*child));
                }
                else if ((*child)->type() == Node::QmlProperty) {
                    // Insert QML properties into the since map.
                    QString propertyName = (*child)->name();
                    nsmap.value().insert(propertyName,(*child));
                }
            }
            else {
                // Insert external documents into the general since map.
                QString name = (*child)->name();
                if ((*child)->parent() && (*child)->parent()->type() == Node::Namespace &&
                    !(*child)->parent()->name().isEmpty()) {
                    name = (*child)->parent()->name()+"::"+name;
                }
                nsmap.value().insert(name,(*child));
            }

            // Recursively find child nodes with since commands.
            if ((*child)->isInnerNode()) {
                findAllSince(static_cast<InnerNode *>(*child));
            }
        }
        ++child;
    }
}

/*!
  Find the \a key in the map of new class maps, and return a
  reference to the value, which is a NodeMap. If \a key is not
  found, return a reference to an empty NodeMap.
 */
const NodeMap& QDocDatabase::getClassMap(const QString& key) const
{
    NodeMapMap::const_iterator i = newClassMaps_.constFind(key);
    if (i != newClassMaps_.constEnd())
        return i.value();
    return emptyNodeMap_;
}

/*!
  Find the \a key in the map of new QML type maps, and return a
  reference to the value, which is a NodeMap. If the \a key is not
  found, return a reference to an empty NodeMap.
 */
const NodeMap& QDocDatabase::getQmlTypeMap(const QString& key) const
{
    NodeMapMap::const_iterator i = newQmlTypeMaps_.constFind(key);
    if (i != newQmlTypeMaps_.constEnd())
        return i.value();
    return emptyNodeMap_;
}

/*!
  Find the \a key in the map of new \e {since} maps, and return
  a reference to the value, which is a NodeMultiMap. If \a key
  is not found, return a reference to an empty NodeMultiMap.
 */
const NodeMultiMap& QDocDatabase::getSinceMap(const QString& key) const
{
    NodeMultiMapMap::const_iterator i = newSinceMaps_.constFind(key);
    if (i != newSinceMaps_.constEnd())
        return i.value();
    return emptyNodeMultiMap_;
}

/*!
  Performs several housekeeping algorithms that create
  certain data structures and resolve lots of links, prior
  to generating documentation.
 */
void QDocDatabase::resolveIssues() {
    tree_->resolveGroups();
    tree_->resolveTargets(tree_->root());
    tree_->resolveCppToQmlLinks();
}

/*!
  Look up group \a name in the map of groups. If found, populate
  the node map \a group with the classes in the group that are
  not marked internal or private.
 */
void QDocDatabase::getGroup(const QString& name, NodeMap& group) const
{
    group.clear();
    NodeList values = tree_->groups().values(name);
    for (int i=0; i<values.size(); ++i) {
        const Node* n = values.at(i);
        if ((n->status() != Node::Internal) && (n->access() != Node::Private)) {
            group.insert(n->nameForLists(),n);
        }
    }
}

/*!
  Searches the \a database for a node named \a target and returns
  a pointer to it if found.
 */
const Node* QDocDatabase::resolveTarget(const QString& target,
                                        const Node* relative,
                                        const Node* self)
{
    const Node* node = 0;
    if (target.endsWith("()")) {
        QString funcName = target;
        funcName.chop(2);
        QStringList path = funcName.split("::");
        const FunctionNode* fn = tree_->findFunctionNode(path, relative, Tree::SearchBaseClasses);
        if (fn) {
            /*
              Why is this case not accepted?
             */
            if (fn->metaness() != FunctionNode::MacroWithoutParams)
                node = fn;
        }
    }
    else if (target.contains(QLatin1Char('#'))) {
        // This error message is never printed; I think we can remove the case.
        qDebug() << "qdoc: target case not handled:" << target;
    }
    else {
        QStringList path = target.split("::");
        int flags = Tree::SearchBaseClasses | Tree::SearchEnumValues | Tree::NonFunction;
        node = tree_->findNode(path, relative, flags, self);
    }
    return node;
}

/*!
  zzz
  Is the atom necessary?
 */
const Node* QDocDatabase::findNodeForTarget(const QString& target,
                                            const Node* relative,
                                            const Atom* atom)
{
    const Node* node = 0;
    if (target.isEmpty())
        node = relative;
    else if (target.endsWith(".html"))
        node = tree_->root()->findChildNodeByNameAndType(target, Node::Document);
    else {
        node = resolveTarget(target, relative);
        if (!node)
            node = tree_->findDocNodeByTitle(target, relative);
        if (!node && atom) {
            qDebug() << "USING ATOM!";
            node = tree_->findUnambiguousTarget(target, *const_cast<Atom**>(&atom), relative);
        }
    }
    return node;
}

QT_END_NAMESPACE
