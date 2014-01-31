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

#include "generator.h"
#include "atom.h"
#include "tree.h"
#include "qdocdatabase.h"
#include "qdoctagfiles.h"
#include "qdocindexfiles.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static NodeMap emptyNodeMap_;
static NodeMultiMap emptyNodeMultiMap_;

/*! \class QDocForest
  This class manages a collection of trees. Each tree is an
  instance of class Tree, which is a private class.

  The forest is populated as each index file is loaded.
  Each index file adds a tree to the forest. Each tree
  is named with the name of the module it represents.

  The search order is created by searchOrder(), if it has
  not already been created. The search order and module
  names arrays have parallel structure, i.e. modulNames_[i]
  is the module name of the Tree at searchOrder_[i].
 */

/*!
  Destroys the qdoc forest. This requires deleting
  each Tree in the forest. Note that the forest has
  been transferred into the search order array, so
  what is really being used to destroy the forest
  is the search order array.
 */
QDocForest::~QDocForest()
{
    for (int i=0; i<searchOrder_.size(); ++i)
        delete searchOrder_.at(i);
    forest_.clear();
    searchOrder_.clear();
    moduleNames_.clear();
    primaryTree_ = 0;
}

/*!
  Initializes the forest prior to a traversal and
  returns a pointer to the root node of the primary
  tree. If the forest is empty, it return 0
 */
NamespaceNode* QDocForest::firstRoot()
{
    currentIndex_ = 0;
    return (!searchOrder_.isEmpty() ? searchOrder_[0]->root() : 0);
}

/*!
  Increments the forest's current tree index. If the current
  tree index is still within the forest, the function returns
  the root node of the current tree. Otherwise it returns 0.
 */
NamespaceNode* QDocForest::nextRoot()
{
    ++currentIndex_;
    return (currentIndex_ < searchOrder_.size() ? searchOrder_[currentIndex_]->root() : 0);
}

/*!
  Initializes the forest prior to a traversal and
  returns a pointer to the primary tree. If the
  forest is empty, it returns 0.
 */
Tree* QDocForest::firstTree()
{
    currentIndex_ = 0;
    return (!searchOrder_.isEmpty() ? searchOrder_[0] : 0);
}

/*!
  Increments the forest's current tree index. If the current
  tree index is still within the forest, the function returns
  the pointer to the current tree. Otherwise it returns 0.
 */
Tree* QDocForest::nextTree()
{
    ++currentIndex_;
    return (currentIndex_ < searchOrder_.size() ? searchOrder_[currentIndex_] : 0);
}

/*!
  \fn Tree* QDocForest::primaryTree()

  Returns the pointer to the primary tree.
 */

/*!
  If the search order array is empty, create the search order.
  If the search order array is not empty, do nothing.
 */
void QDocForest::setSearchOrder()
{
    if (!searchOrder_.isEmpty())
        return;
    QString primaryName = primaryTree()->moduleName();
    searchOrder_.clear();
    searchOrder_.reserve(forest_.size()+1);
    moduleNames_.reserve(forest_.size()+1);
    searchOrder_.append(primaryTree_);
    moduleNames_.append(primaryName);
    QMap<QString, Tree*>::iterator i;
    if (primaryName != "QtCore") {
        i = forest_.find("QtCore");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtCore");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtGui") {
        i = forest_.find("QtGui");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtGui");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtNetwork") {
        i = forest_.find("QtNetwork");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtNetwork");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtOpenGL") {
        i = forest_.find("QtOpenGL");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtOpenGL");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtWidgets") {
        i = forest_.find("QtWidgets");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtWidgets");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtSql") {
        i = forest_.find("QtSql");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtSql");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtXml") {
        i = forest_.find("QtXml");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtXml");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtSvg") {
        i = forest_.find("QtSvg");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtSvg");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtDoc") {
        i = forest_.find("QtDoc");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtDoc");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtQuick") {
        i = forest_.find("QtQuick");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtQuick");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtQml") {
        i = forest_.find("QtQml");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtQml");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtPrintSupport") {
        i = forest_.find("QtPrintSupport");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtPrintSupport");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtGraphicalEffects") {
        i = forest_.find("QtGraphicalEffects");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtGraphicalEffects");
            forest_.erase(i);
        }
    }
    if (primaryName != "QtConcurrent") {
        i = forest_.find("QtConcurrent");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("QtConcurrent");
            forest_.erase(i);
        }
    }
#if 0
    if (primaryName != "zzz") {
        i = forest_.find("zzz");
        if (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append("zzz");
            forest_.erase(i);
        }
    }
#endif
    /*
      If any trees remain in the forest, just add them
      to the search order sequentially, because we don't
      know any better at this point.
     */
    if (!forest_.isEmpty()) {
        i = forest_.begin();
        while (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append(i.key());
            ++i;
        }
        forest_.clear();
    }
#if 0
    qDebug() << "  SEARCH ORDER:";
    for (int i=0; i<moduleNames_.size(); ++i)
        qDebug() << "    " << i+1 << "." << moduleNames_.at(i);
#endif
}

/*!
  Returns an ordered array of Tree pointers that represents
  the order in which the trees should be searched. The first
  Tree in the array is the tree for the current module, i.e.
  the module for which qdoc is generating documentation.

  The other Tree pointers in the array represent the index
  files that were loaded in preparation for generating this
  module's documentation. Each Tree pointer represents one
  index file. The index file Tree points have been ordered
  heuristically to, hopefully, minimize searching. Thr order
  will probably be changed.

  If the search order array is empty, this function calls
  setSearchOrder() to create the search order.
 */
const QVector<Tree*>& QDocForest::searchOrder()
{
    if (searchOrder_.isEmpty())
        setSearchOrder();
    return searchOrder_;
}

/*!
  Create a new Tree for the index file for the specified
  \a module and add it to the forest. Return the pointer
  to its root.
 */
NamespaceNode* QDocForest::newIndexTree(const QString& module)
{
    primaryTree_ = new Tree(module, qdb_);
    forest_.insert(module, primaryTree_);
    return primaryTree_->root();
}

/*!
  Create a new Tree for use as the primary tree. This tree
  will represent the primary module.
 */
void QDocForest::newPrimaryTree(const QString& module)
{
    primaryTree_ = new Tree(module, qdb_);
}

/*!
  Searches the Tree \a t for a node named \a target and returns
  a pointer to it if found. The \a relative node is the starting
  point, but it only makes sense in the primary tree. Therefore,
  when this function is called with \a t being an index tree,
  \a relative is 0. When relative is 0, the root node of \a t is
  the starting point.
 */
const Node* QDocForest::resolveTargetHelper(const QString& target,
                                            const Node* relative,
                                            Tree* t)
{
    const Node* node = 0;
    if (target.endsWith("()")) {
        QString funcName = target;
        funcName.chop(2);
        QStringList path = funcName.split("::");
        const FunctionNode* fn = t->findFunctionNode(path, relative, SearchBaseClasses);
        if (fn && fn->metaness() != FunctionNode::MacroWithoutParams)
            node = fn;
    }
    else {
        QStringList path = target.split("::");
        int flags = SearchBaseClasses | SearchEnumValues | NonFunction;
        node = t->findNode(path, relative, flags);
        if (!node) {
            QStringList path = target.split("::");
            const FunctionNode* fn = t->findFunctionNode(path, relative, SearchBaseClasses);
            if (fn && fn->metaness() != FunctionNode::MacroWithoutParams)
                node = fn;
        }
    }
    return node;
}

/*!
  Searches the Tree \a t for a type node named by the \a path
  and returns a pointer to it if found. The \a relative node
  is the starting point, but it only makes sense when searching
  the primary tree. Therefore, when this function is called with
  \a t being an index tree, \a relative is 0. When relative is 0,
  the root node of \a t is the starting point.
 */
const Node* QDocForest::resolveTypeHelper(const QStringList& path, const Node* relative, Tree* t)
{
    int flags = SearchBaseClasses | SearchEnumValues | NonFunction;
    return t->findNode(path, relative, flags);
}

/*! \class QDocDatabase
  This class provides exclusive access to the qdoc database,
  which consists of a forrest of trees and a lot of maps and
  other useful data structures.
 */

QDocDatabase* QDocDatabase::qdocDB_ = NULL;
NodeMap QDocDatabase::typeNodeMap_;

/*!
  Constructs the singleton qdoc database object. The singleton
  constructs the \a forest_ object, which is also a singleton.
  \a showInternal_ is normally false. If it is true, qdoc will
  write documentation for nodes marked \c internal.
 */
QDocDatabase::QDocDatabase() : showInternal_(false), forest_(this)
{
    // nothing
}

/*!
  Destroys the qdoc database object. This requires destroying
  the forest object, which contains an array of tree pointers.
  Each tree is deleted.
 */
QDocDatabase::~QDocDatabase()
{
    masterMap_.clear();
}

/*!
  Creates the singleton. Allows only one instance of the class
  to be created. Returns a pointer to the singleton.
*/
QDocDatabase* QDocDatabase::qdocDB()
{
    if (!qdocDB_) {
      qdocDB_ = new QDocDatabase;
      initializeDB();
    }
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
  Initialize data structures in the singleton qdoc database.

  In particular, the type node map is initialized with a lot
  type names that don't refer to documented types. For example,
  the C++ standard types are included. These might be documented
  here at some point, but for now they are not. Other examples
  include \c array and \c data, which are just generic names
  used as place holders in function signatures that appear in
  the documentation.
 */
void QDocDatabase::initializeDB()
{
    typeNodeMap_.insert( "accepted", 0);
    typeNodeMap_.insert( "actionPerformed", 0);
    typeNodeMap_.insert( "activated", 0);
    typeNodeMap_.insert( "alias", 0);
    typeNodeMap_.insert( "anchors", 0);
    typeNodeMap_.insert( "any", 0);
    typeNodeMap_.insert( "array", 0);
    typeNodeMap_.insert( "autoSearch", 0);
    typeNodeMap_.insert( "axis", 0);
    typeNodeMap_.insert( "backClicked", 0);
    typeNodeMap_.insert( "bool", 0);
    typeNodeMap_.insert( "boomTime", 0);
    typeNodeMap_.insert( "border", 0);
    typeNodeMap_.insert( "buttonClicked", 0);
    typeNodeMap_.insert( "callback", 0);
    typeNodeMap_.insert( "char", 0);
    typeNodeMap_.insert( "clicked", 0);
    typeNodeMap_.insert( "close", 0);
    typeNodeMap_.insert( "closed", 0);
    typeNodeMap_.insert( "color", 0);
    typeNodeMap_.insert( "cond", 0);
    typeNodeMap_.insert( "data", 0);
    typeNodeMap_.insert( "dataReady", 0);
    typeNodeMap_.insert( "dateString", 0);
    typeNodeMap_.insert( "dateTimeString", 0);
    typeNodeMap_.insert( "datetime", 0);
    typeNodeMap_.insert( "day", 0);
    typeNodeMap_.insert( "deactivated", 0);
    typeNodeMap_.insert( "double", 0);
    typeNodeMap_.insert( "drag", 0);
    typeNodeMap_.insert( "easing", 0);
    typeNodeMap_.insert( "enumeration", 0);
    typeNodeMap_.insert( "error", 0);
    typeNodeMap_.insert( "exposure", 0);
    typeNodeMap_.insert( "fatalError", 0);
    typeNodeMap_.insert( "fileSelected", 0);
    typeNodeMap_.insert( "flags", 0);
    typeNodeMap_.insert( "float", 0);
    typeNodeMap_.insert( "focus", 0);
    typeNodeMap_.insert( "focusZone", 0);
    typeNodeMap_.insert( "format", 0);
    typeNodeMap_.insert( "framePainted", 0);
    typeNodeMap_.insert( "from", 0);
    typeNodeMap_.insert( "frontClicked", 0);
    typeNodeMap_.insert( "function", 0);
    typeNodeMap_.insert( "hasOpened", 0);
    typeNodeMap_.insert( "hovered", 0);
    typeNodeMap_.insert( "hoveredTitle", 0);
    typeNodeMap_.insert( "hoveredUrl", 0);
    typeNodeMap_.insert( "imageCapture", 0);
    typeNodeMap_.insert( "imageProcessing", 0);
    typeNodeMap_.insert( "index", 0);
    typeNodeMap_.insert( "initialized", 0);
    typeNodeMap_.insert( "int", 0);
    typeNodeMap_.insert( "isLoaded", 0);
    typeNodeMap_.insert( "item", 0);
    typeNodeMap_.insert( "jsdict", 0);
    typeNodeMap_.insert( "jsobject", 0);
    typeNodeMap_.insert( "key", 0);
    typeNodeMap_.insert( "keysequence", 0);
    typeNodeMap_.insert( "list", 0);
    typeNodeMap_.insert( "listViewClicked", 0);
    typeNodeMap_.insert( "loadRequest", 0);
    typeNodeMap_.insert( "locale", 0);
    typeNodeMap_.insert( "location", 0);
    typeNodeMap_.insert( "long", 0);
    typeNodeMap_.insert( "message", 0);
    typeNodeMap_.insert( "messageReceived", 0);
    typeNodeMap_.insert( "mode", 0);
    typeNodeMap_.insert( "month", 0);
    typeNodeMap_.insert( "name", 0);
    typeNodeMap_.insert( "number", 0);
    typeNodeMap_.insert( "object", 0);
    typeNodeMap_.insert( "offset", 0);
    typeNodeMap_.insert( "ok", 0);
    typeNodeMap_.insert( "openCamera", 0);
    typeNodeMap_.insert( "openImage", 0);
    typeNodeMap_.insert( "openVideo", 0);
    typeNodeMap_.insert( "padding", 0);
    typeNodeMap_.insert( "parent", 0);
    typeNodeMap_.insert( "path", 0);
    typeNodeMap_.insert( "photoModeSelected", 0);
    typeNodeMap_.insert( "position", 0);
    typeNodeMap_.insert( "precision", 0);
    typeNodeMap_.insert( "presetClicked", 0);
    typeNodeMap_.insert( "preview", 0);
    typeNodeMap_.insert( "previewSelected", 0);
    typeNodeMap_.insert( "progress", 0);
    typeNodeMap_.insert( "puzzleLost", 0);
    typeNodeMap_.insert( "qmlSignal", 0);
    typeNodeMap_.insert( "real", 0);
    typeNodeMap_.insert( "rectangle", 0);
    typeNodeMap_.insert( "request", 0);
    typeNodeMap_.insert( "requestId", 0);
    typeNodeMap_.insert( "section", 0);
    typeNodeMap_.insert( "selected", 0);
    typeNodeMap_.insert( "send", 0);
    typeNodeMap_.insert( "settingsClicked", 0);
    typeNodeMap_.insert( "shoe", 0);
    typeNodeMap_.insert( "short", 0);
    typeNodeMap_.insert( "signed", 0);
    typeNodeMap_.insert( "sizeChanged", 0);
    typeNodeMap_.insert( "size_t", 0);
    typeNodeMap_.insert( "sockaddr", 0);
    typeNodeMap_.insert( "someOtherSignal", 0);
    typeNodeMap_.insert( "sourceSize", 0);
    typeNodeMap_.insert( "startButtonClicked", 0);
    typeNodeMap_.insert( "state", 0);
    typeNodeMap_.insert( "std::initializer_list", 0);
    typeNodeMap_.insert( "std::list", 0);
    typeNodeMap_.insert( "std::map", 0);
    typeNodeMap_.insert( "std::pair", 0);
    typeNodeMap_.insert( "std::string", 0);
    typeNodeMap_.insert( "std::vector", 0);
    typeNodeMap_.insert( "string", 0);
    typeNodeMap_.insert( "stringlist", 0);
    typeNodeMap_.insert( "swapPlayers", 0);
    typeNodeMap_.insert( "symbol", 0);
    typeNodeMap_.insert( "t", 0);
    typeNodeMap_.insert( "T", 0);
    typeNodeMap_.insert( "tagChanged", 0);
    typeNodeMap_.insert( "timeString", 0);
    typeNodeMap_.insert( "timeout", 0);
    typeNodeMap_.insert( "to", 0);
    typeNodeMap_.insert( "toggled", 0);
    typeNodeMap_.insert( "type", 0);
    typeNodeMap_.insert( "unsigned", 0);
    typeNodeMap_.insert( "urllist", 0);
    typeNodeMap_.insert( "va_list", 0);
    typeNodeMap_.insert( "value", 0);
    typeNodeMap_.insert( "valueEmitted", 0);
    typeNodeMap_.insert( "videoFramePainted", 0);
    typeNodeMap_.insert( "videoModeSelected", 0);
    typeNodeMap_.insert( "videoRecorder", 0);
    typeNodeMap_.insert( "void", 0);
    typeNodeMap_.insert( "volatile", 0);
    typeNodeMap_.insert( "wchar_t", 0);
    typeNodeMap_.insert( "x", 0);
    typeNodeMap_.insert( "y", 0);
    typeNodeMap_.insert( "zoom", 0);
    typeNodeMap_.insert( "zoomTo", 0);
}

/*! \fn NamespaceNode* QDocDatabase::primaryTreeRoot()
  Returns a pointer to the root node of the primary tree.
 */

/*!
  \fn const DocNodeMap& QDocDatabase::groups() const
  Returns a const reference to the collection of all
  group nodes.
*/

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
  Find the group node named \a name and return a pointer
  to it. If a matching node is not found, return 0.
 */
DocNode* QDocDatabase::getGroup(const QString& name)
{
    DocNodeMap::const_iterator i = groups_.find(name);
    if (i != groups_.end())
        return i.value();
    return 0;
}

/*!
  Find the group node named \a name and return a pointer
  to it. If a matching node is not found, add a new group
  node named \a name and return a pointer to that one.

  If a new group node is added, its parent is the tree root,
  and the new group node is marked \e{not seen}.
 */
DocNode* QDocDatabase::findGroup(const QString& name)
{
    DocNodeMap::const_iterator i = groups_.find(name);
    if (i != groups_.end())
        return i.value();
    DocNode* dn = new DocNode(primaryTreeRoot(), name, Node::Group, Node::OverviewPage);
    dn->markNotSeen();
    groups_.insert(name,dn);
    if (!masterMap_.contains(name,dn))
        masterMap_.insert(name,dn);
    return dn;
}

/*!
  Find the module node named \a name and return a pointer
  to it. If a matching node is not found, add a new module
  node named \a name and return a pointer to that one.

  If a new module node is added, its parent is the tree root,
  and the new module node is marked \e{not seen}.
 */
DocNode* QDocDatabase::findModule(const QString& name)
{
    DocNodeMap::const_iterator i = modules_.find(name);
    if (i != modules_.end())
        return i.value();
    DocNode* dn = new DocNode(primaryTreeRoot(), name, Node::Module, Node::OverviewPage);
    dn->markNotSeen();
    modules_.insert(name,dn);
    if (!masterMap_.contains(name,dn))
        masterMap_.insert(name,dn);
    return dn;
}

/*!
  Find the QML module node named \a name and return a pointer
  to it. If a matching node is not found, add a new QML module
  node named \a name and return a pointer to that one.

  If a new QML module node is added, its parent is the tree root,
  and the new QML module node is marked \e{not seen}.
 */
QmlModuleNode* QDocDatabase::findQmlModule(const QString& name)
{
    if (qmlModules_.contains(name))
        return static_cast<QmlModuleNode*>(qmlModules_.value(name));

    QmlModuleNode* qmn = new QmlModuleNode(primaryTreeRoot(), name);
    qmn->markNotSeen();
    qmn->setQmlModuleInfo(name);
    qmlModules_.insert(name, qmn);
    masterMap_.insert(name, qmn);
    return qmn;
}

/*!
  Looks up the group node named \a name in the collection
  of all group nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new group node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned. The group node is marked \e{seen}
  in either case.
 */
DocNode* QDocDatabase::addGroup(const QString& name)
{
    DocNode* group = findGroup(name);
    group->markSeen();
    return group;
}

/*!
  Looks up the module node named \a name in the collection
  of all module nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new module node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned. The module node is marked \e{seen}
  in either case.
 */
DocNode* QDocDatabase::addModule(const QString& name)
{
    DocNode* module = findModule(name);
    module->markSeen();
    return module;
}

/*!
  Looks up the QML module node named \a name in the collection
  of all QML module nodes. If a match is found, a pointer to the
  node is returned. Otherwise, a new QML module node named \a name
  is created and inserted into the collection, and the pointer
  to that node is returned. The QML module node is marked \e{seen}
  in either case.
 */
QmlModuleNode* QDocDatabase::addQmlModule(const QString& name)
{
    QStringList blankSplit = name.split(QLatin1Char(' '));
    QmlModuleNode* qmn = findQmlModule(blankSplit[0]);
    qmn->setQmlModuleInfo(name);
    qmn->markSeen();
    //masterMap_.insert(qmn->qmlModuleIdentifier(),qmn);
    return qmn;
}

/*!
  Looks up the group node named \a name in the collection
  of all group nodes. If a match is not found, a new group
  node named \a name is created and inserted into the collection.
  Then append \a node to the group's members list, and append the
  group node to the member list of the \a node. The parent of the
  \a node is not changed by this function. Returns a pointer to
  the group node.
 */
DocNode* QDocDatabase::addToGroup(const QString& name, Node* node)
{
    DocNode* dn = findGroup(name);
    dn->addMember(node);
    node->addMember(dn);
    return dn;
}

/*!
  Looks up the module node named \a name in the collection
  of all module nodes. If a match is not found, a new module
  node named \a name is created and inserted into the collection.
  Then append \a node to the module's members list. The parent of
  \a node is not changed by this function. Returns the module node.
 */
DocNode* QDocDatabase::addToModule(const QString& name, Node* node)
{
    DocNode* dn = findModule(name);
    dn->addMember(node);
    node->setModuleName(name);
    return dn;
}

/*!
  Looks up the QML module named \a name. If it isn't there,
  create it. Then append \a node to the QML module's member
  list. The parent of \a node is not changed by this function.
 */
void QDocDatabase::addToQmlModule(const QString& name, Node* node)
{
    QStringList qmid;
    QStringList dotSplit;
    QStringList blankSplit = name.split(QLatin1Char(' '));
    qmid.append(blankSplit[0]);
    if (blankSplit.size() > 1) {
        qmid.append(blankSplit[0] + blankSplit[1]);
        dotSplit = blankSplit[1].split(QLatin1Char('.'));
        qmid.append(blankSplit[0] + dotSplit[0]);
    }

    QmlModuleNode* qmn = findQmlModule(blankSplit[0]);
    qmn->addMember(node);
    node->setQmlModule(qmn);

    if (node->subType() == Node::QmlClass) {
        QmlClassNode* n = static_cast<QmlClassNode*>(node);
        for (int i=0; i<qmid.size(); ++i) {
            QString key = qmid[i] + "::" + node->name();
            if (!qmlTypeMap_.contains(key))
                qmlTypeMap_.insert(key,n);
            if (!masterMap_.contains(key))
                masterMap_.insert(key,node);
        }
        if (!masterMap_.contains(node->name(),node))
            masterMap_.insert(node->name(),node);
    }
}

/*!
  Looks up the QML type node identified by the Qml module id
  \a qmid and QML type \a name and returns a pointer to the
  QML type node. The key is \a qmid + "::" + \a name.

  If the QML module id is empty, it looks up the QML type by
  \a name only.
 */
QmlClassNode* QDocDatabase::findQmlType(const QString& qmid, const QString& name)
{
    if (!qmid.isEmpty())
        return qmlTypeMap_.value(qmid + "::" + name);

    QStringList path(name);
    Node* n = forest_.findNodeByNameAndType(path, Node::Document, Node::QmlClass, true);
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
  Looks up the QML type node identified by the Qml module id
  constructed from the strings in the \a import record and the
  QML type \a name and returns a pointer to the QML type node.
  If a QML type node is not found, 0 is returned.
 */
QmlClassNode* QDocDatabase::findQmlType(const ImportRec& import, const QString& name) const
{
    if (!import.isEmpty()) {
        QStringList dotSplit;
        dotSplit = name.split(QLatin1Char('.'));
        QString qmName;
        if (import.importUri_.isEmpty())
            qmName = import.name_;
        else
            qmName = import.importUri_;
        for (int i=0; i<dotSplit.size(); ++i) {
            QString qualifiedName = qmName + "::" + dotSplit[i];
            QmlClassNode* qcn = qmlTypeMap_.value(qualifiedName);
            if (qcn)
                return qcn;
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
  This function calls \a func for each tree in the forest.
 */
void QDocDatabase::processForest(void (QDocDatabase::*func) (InnerNode*))
{
    Tree* t = forest_.firstTree();
    while (t) {
        (this->*(func))(t->root());
        t = forest_.nextTree();
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

    /*
    findAllClasses(treeRoot());
    findAllFunctions(treeRoot());
    findAllLegaleseTexts(treeRoot());
    findAllNamespaces(treeRoot());
    findAllSince(treeRoot());
    findAllObsoleteThings(treeRoot());
    */
    processForest(&QDocDatabase::findAllClasses);
    processForest(&QDocDatabase::findAllFunctions);
    processForest(&QDocDatabase::findAllLegaleseTexts);
    processForest(&QDocDatabase::findAllNamespaces);
    processForest(&QDocDatabase::findAllSince);
    processForest(&QDocDatabase::findAllObsoleteThings);
}

/*!
  Finds all the C++ class nodes and QML type nodes and
  sorts them into maps.
 */
void QDocDatabase::findAllClasses(InnerNode* node)
{
    NodeList::const_iterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private && (!(*c)->isInternal() || showInternal_)) {
            if ((*c)->type() == Node::Class && !(*c)->doc().isEmpty()) {
                QString className = (*c)->name();
                if ((*c)->parent() &&
                        (*c)->parent()->type() == Node::Namespace &&
                        !(*c)->parent()->name().isEmpty())
                    className = (*c)->parent()->name()+"::"+className;

                if ((*c)->status() == Node::Compat) {
                    compatClasses_.insert(className, *c);
                }
                else {
                    nonCompatClasses_.insert(className, *c);
                    if ((*c)->status() == Node::Main)
                        mainClasses_.insert(className, *c);
                }

                QString serviceName = (static_cast<const ClassNode *>(*c))->serviceName();
                if (!serviceName.isEmpty()) {
                    serviceClasses_.insert(serviceName, *c);
                }
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
void QDocDatabase::findAllFunctions(InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode()) {
                findAllFunctions(static_cast<InnerNode*>(*c));
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
void QDocDatabase::findAllLegaleseTexts(InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if (!(*c)->doc().legaleseText().isEmpty())
                legaleseTexts_.insertMulti((*c)->doc().legaleseText(), *c);
            if ((*c)->isInnerNode())
                findAllLegaleseTexts(static_cast<InnerNode *>(*c));
        }
        ++c;
    }
}

/*!
  Finds all the namespace nodes and puts them in an index.
 */
void QDocDatabase::findAllNamespaces(InnerNode* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isInnerNode()) {
                findAllNamespaces(static_cast<InnerNode *>(*c));
                if ((*c)->type() == Node::Namespace) {
                    // Ensure that the namespace's name is not empty (the root
                    // namespace has no name).
                    if (!(*c)->name().isEmpty())
                        namespaceIndex_.insert((*c)->name(), *c);
                }
            }
        }
        ++c;
    }
}

/*!
  Finds all nodes with status = Obsolete and sorts them into
  maps. They can be C++ classes, QML types, or they can be
  functions, enum types, typedefs, methods, etc.
 */
void QDocDatabase::findAllObsoleteThings(InnerNode* node)
{
    NodeList::const_iterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            QString name = (*c)->name();
            if ((*c)->status() == Node::Obsolete) {
                if ((*c)->type() == Node::Class) {
                    if ((*c)->parent() && (*c)->parent()->type() == Node::Namespace &&
                        !(*c)->parent()->name().isEmpty())
                        name = (*c)->parent()->name() + "::" + name;
                    obsoleteClasses_.insert(name, *c);
                }
                else if ((*c)->type() == Node::Document && (*c)->subType() == Node::QmlClass) {
                    if (name.startsWith(QLatin1String("QML:")))
                        name = name.mid(4);
                    name = (*c)->qmlModuleName() + "::" + name;
                    obsoleteQmlTypes_.insert(name,*c);
                }
            }
            else if ((*c)->type() == Node::Class) {
                InnerNode* n = static_cast<InnerNode*>(*c);
                bool inserted = false;
                NodeList::const_iterator p = n->childNodes().constBegin();
                while (p != n->childNodes().constEnd()) {
                    if ((*p)->access() != Node::Private) {
                        switch ((*p)->type()) {
                        case Node::Enum:
                        case Node::Typedef:
                        case Node::Function:
                        case Node::Property:
                        case Node::Variable:
                            if ((*p)->status() == Node::Obsolete) {
                                if ((*c)->parent() && (*c)->parent()->type() == Node::Namespace &&
                                    !(*c)->parent()->name().isEmpty())
                                    name = (*c)->parent()->name() + "::" + name;
                                classesWithObsoleteMembers_.insert(name, *c);
                                inserted = true;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    if (inserted)
                        break;
                    ++p;
                }
            }
            else if ((*c)->type() == Node::Document && (*c)->subType() == Node::QmlClass) {
                InnerNode* n = static_cast<InnerNode*>(*c);
                bool inserted = false;
                NodeList::const_iterator p = n->childNodes().constBegin();
                while (p != n->childNodes().constEnd()) {
                    if ((*p)->access() != Node::Private) {
                        switch ((*c)->type()) {
                        case Node::QmlProperty:
                        case Node::QmlSignal:
                        case Node::QmlSignalHandler:
                        case Node::QmlMethod:
                            if ((*c)->parent()) {
                                Node* parent = (*c)->parent();
                                if (parent->type() == Node::QmlPropertyGroup && parent->parent())
                                    parent = parent->parent();
                                if (parent && parent->subType() == Node::QmlClass && !parent->name().isEmpty())
                                    name = parent->name() + "::" + name;
                            }
                            qmlTypesWithObsoleteMembers_.insert(name,*c);
                            inserted = true;
                            break;
                        default:
                            break;
                        }
                    }
                    if (inserted)
                        break;
                    ++p;
                }
            }
            else if ((*c)->isInnerNode()) {
                findAllObsoleteThings(static_cast<InnerNode*>(*c));
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
void QDocDatabase::findAllSince(InnerNode* node)
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
            else {
                if ((*child)->type() == Node::Class) {
                    // Insert classes into the since and class maps.
                    QString className = (*child)->name();
                    if ((*child)->parent() && !(*child)->parent()->name().isEmpty()) {
                        className = (*child)->parent()->name()+"::"+className;
                    }
                    nsmap.value().insert(className,(*child));
                    ncmap.value().insert(className,(*child));
                }
                else if ((*child)->subType() == Node::QmlClass) {
                    // Insert QML elements into the since and element maps.
                    QString className = (*child)->name();
                    if ((*child)->parent() && !(*child)->parent()->name().isEmpty()) {
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
                else {
                    // Insert external documents into the general since map.
                    QString name = (*child)->name();
                    if ((*child)->parent() && !(*child)->parent()->name().isEmpty()) {
                        name = (*child)->parent()->name()+"::"+name;
                    }
                    nsmap.value().insert(name,(*child));
                }
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
    resolveQmlInheritance(primaryTreeRoot());
    resolveTargets();
    primaryTree()->resolveCppToQmlLinks();
}

/*!
  This function is called for autolinking to a \a type,
  which could be a function return type or a parameter
  type. The tree node that represents the \a type is
  returned. All the trees are searched until a match is
  found. When searching the primary tree, the search
  begins at \a relative and proceeds up the parent chain.
  When searching the index trees, the search begins at the
  root.
 */
const Node* QDocDatabase::resolveType(const QString& type, const Node* relative)
{
    QStringList path = type.split("::");
    if ((path.size() == 1) && (path.at(0)[0].isLower() || path.at(0) == QString("T"))) {
        NodeMap::iterator i = typeNodeMap_.find(path.at(0));
        if (i != typeNodeMap_.end())
            return i.value();
    }
    return forest_.resolveType(path, relative);
}

/*!
  Finds the node that will generate the documentation that
  contains the \a target and returns a pointer to it.

  Can this be improved by using the target map in Tree?
 */
const Node* QDocDatabase::findNodeForTarget(const QString& target, const Node* relative)
{
    const Node* node = 0;
    if (target.isEmpty())
        node = relative;
    else if (target.endsWith(".html")) {
        node = findNodeByNameAndType(QStringList(target), Node::Document, Node::NoSubType);
    }
    else {
        node = resolveTarget(target, relative);
        if (!node)
            node = findDocNodeByTitle(target, relative);
    }
    return node;
}

/*!
  For each QML Type node in the tree beginning at \a root,
  if it has a QML base type name but its QML base type node
  pointer is 0, use the QML base type name to look up the
  base type node. If the node is found in the tree, set the
  node's QML base type node pointer.
 */
void QDocDatabase::resolveQmlInheritance(InnerNode* root)
{
    // Do we need recursion?
    foreach (Node* child, root->childNodes()) {
        if (child->type() == Node::Document && child->subType() == Node::QmlClass) {
            QmlClassNode* qcn = static_cast<QmlClassNode*>(child);
            if ((qcn->qmlBaseNode() == 0) && !qcn->qmlBaseName().isEmpty()) {
                QmlClassNode* bqcn = 0;
                if (qcn->qmlBaseName().contains("::")) {
                    bqcn =  qmlTypeMap_.value(qcn->qmlBaseName());
                }
                else {
                    const ImportList& imports = qcn->importList();
                    for (int i=0; i<imports.size(); ++i) {
                        bqcn = findQmlType(imports[i], qcn->qmlBaseName());
                        if (bqcn)
                            break;
                    }
                }
                if (bqcn == 0) {
                    bqcn = findQmlType(QString(), qcn->qmlBaseName());
                }
                if (bqcn) {
                    qcn->setQmlBaseNode(bqcn);
                }
#if 0
                else {
                    qDebug() << "Temporary error message (ignore): UNABLE to resolve QML base type:"
                             << qcn->qmlBaseName() << "for QML type:" << qcn->name();
                }
#endif
            }
        }
    }
}

/*!
  Generates a tag file and writes it to \a name.
 */
void QDocDatabase::generateTagFile(const QString& name, Generator* g)
{
    if (!name.isEmpty()) {
        QDocTagFiles::qdocTagFiles()->generateTagFile(name, g);
        QDocTagFiles::destroyQDocTagFiles();
    }
}

/*!
  Reads and parses the qdoc index files listed in \a indexFiles.
 */
void QDocDatabase::readIndexes(const QStringList& indexFiles)
{
    QDocIndexFiles::qdocIndexFiles()->readIndexes(indexFiles);
    QDocIndexFiles::destroyQDocIndexFiles();
}

/*!
  Generates a qdoc index file and write it to \a fileName. The
  index file is generated with the parameters \a url, \a title,
  \a g, and \a generateInternalNodes.
 */
void QDocDatabase::generateIndex(const QString& fileName,
                                 const QString& url,
                                 const QString& title,
                                 Generator* g,
                                 bool generateInternalNodes)
{
    QDocIndexFiles::qdocIndexFiles()->generateIndex(fileName, url, title, g, generateInternalNodes);
    QDocIndexFiles::destroyQDocIndexFiles();
}

/*!
  If there are open namespaces, search for the function node
  having the same function name as the \a clone node in each
  open namespace. The \a parentPath is a portion of the path
  name provided with the function name at the point of
  reference. \a parentPath is usually a class name. Return
  the pointer to the function node if one is found in an
  open namespace. Otherwise return 0.

  This open namespace concept is of dubious value and might
  be removed.
 */
FunctionNode* QDocDatabase::findNodeInOpenNamespace(const QStringList& parentPath,
                                                    const FunctionNode* clone)
{
    FunctionNode* fn = 0;
    if (!openNamespaces_.isEmpty()) {
        foreach (const QString& t, openNamespaces_) {
            QStringList path = t.split("::") + parentPath;
            fn = findFunctionNode(path, clone);
            if (fn)
                break;
        }
    }
    return fn;
}

/*!
  Find a node of the specified \a type and \a subtype that is
  reached with the specified \a path qualified with the name
  of one of the open namespaces (might not be any open ones).
  If the node is found in an open namespace, prefix \a path
  with the name of the open namespace and "::" and return a
  pointer to the node. Othewrwise return 0.

  This function only searches in the current primary tree.
 */
Node* QDocDatabase::findNodeInOpenNamespace(QStringList& path,
                                            Node::Type type,
                                            Node::SubType subtype)
{
    if (path.isEmpty())
        return 0;
    Node* n = 0;
    if (!openNamespaces_.isEmpty()) {
        foreach (const QString& t, openNamespaces_) {
            QStringList p;
            if (t != path[0])
                p = t.split("::") + path;
            else
                p = path;
            n = primaryTree()->findNodeByNameAndType(p, type, subtype);
            if (n) {
                path = p;
                break;
            }
        }
    }
    return n;
}

QT_END_NAMESPACE
