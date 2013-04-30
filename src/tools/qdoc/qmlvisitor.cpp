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

#include <qfileinfo.h>
#include <qstringlist.h>
#include <qglobal.h>
#include "qqmljsast_p.h"
#include "qqmljsastfwd_p.h"
#include "qqmljsengine_p.h"
#include <qdebug.h>
#include "node.h"
#include "codeparser.h"
#include "qmlvisitor.h"
#include "qdocdatabase.h"

QT_BEGIN_NAMESPACE

#define COMMAND_DEPRECATED              Doc::alias(QLatin1String("deprecated"))
#define COMMAND_INGROUP                 Doc::alias(QLatin1String("ingroup"))
#define COMMAND_INTERNAL                Doc::alias(QLatin1String("internal"))
#define COMMAND_OBSOLETE                Doc::alias(QLatin1String("obsolete"))
#define COMMAND_PAGEKEYWORDS            Doc::alias(QLatin1String("pagekeywords"))
#define COMMAND_PRELIMINARY             Doc::alias(QLatin1String("preliminary"))
#define COMMAND_SINCE                   Doc::alias(QLatin1String("since"))
#define COMMAND_WRAPPER                 Doc::alias(QLatin1String("wrapper"))

#define COMMAND_QMLABSTRACT             Doc::alias(QLatin1String("qmlabstract"))
#define COMMAND_QMLCLASS                Doc::alias(QLatin1String("qmlclass"))
#define COMMAND_QMLTYPE                 Doc::alias(QLatin1String("qmltype"))
#define COMMAND_QMLMODULE               Doc::alias(QLatin1String("qmlmodule"))
#define COMMAND_QMLPROPERTY             Doc::alias(QLatin1String("qmlproperty"))
#define COMMAND_QMLATTACHEDPROPERTY     Doc::alias(QLatin1String("qmlattachedproperty"))
#define COMMAND_QMLINHERITS             Doc::alias(QLatin1String("inherits"))
#define COMMAND_QMLINSTANTIATES         Doc::alias(QLatin1String("instantiates"))
#define COMMAND_INQMLMODULE             Doc::alias(QLatin1String("inqmlmodule"))
#define COMMAND_QMLSIGNAL               Doc::alias(QLatin1String("qmlsignal"))
#define COMMAND_QMLATTACHEDSIGNAL       Doc::alias(QLatin1String("qmlattachedsignal"))
#define COMMAND_QMLMETHOD               Doc::alias(QLatin1String("qmlmethod"))
#define COMMAND_QMLATTACHEDMETHOD       Doc::alias(QLatin1String("qmlattachedmethod"))
#define COMMAND_QMLDEFAULT              Doc::alias(QLatin1String("default"))
#define COMMAND_QMLREADONLY             Doc::alias(QLatin1String("readonly"))
#define COMMAND_QMLBASICTYPE            Doc::alias(QLatin1String("qmlbasictype"))

/*!
  The constructor stores all the parameters in local data members.
 */
QmlDocVisitor::QmlDocVisitor(const QString &filePath,
                             const QString &code,
                             QQmlJS::Engine *engine,
                             QSet<QString> &commands,
                             QSet<QString> &topics)
    : nestingLevel(0)
{
    lastEndOffset = 0;
    this->filePath = filePath;
    this->name = QFileInfo(filePath).baseName();
    document = code;
    this->engine = engine;
    this->commands = commands;
    this->topics = topics;
    current = QDocDatabase::qdocDB()->treeRoot();
}

/*!
  The destructor does nothing.
 */
QmlDocVisitor::~QmlDocVisitor()
{
    // nothing.
}

/*!
  Returns the location of the nearest comment above the \a offset.
 */
QQmlJS::AST::SourceLocation QmlDocVisitor::precedingComment(quint32 offset) const
{
    QListIterator<QQmlJS::AST::SourceLocation> it(engine->comments());
    it.toBack();

    while (it.hasPrevious()) {

        QQmlJS::AST::SourceLocation loc = it.previous();

        if (loc.begin() <= lastEndOffset) {
            // Return if we reach the end of the preceding structure.
            break;
        }
        else if (usedComments.contains(loc.begin())) {
            // Return if we encounter a previously used comment.
            break;
        }
        else if (loc.begin() > lastEndOffset && loc.end() < offset) {
            // Only examine multiline comments in order to avoid snippet markers.
            if (document.at(loc.offset - 1) == QLatin1Char('*')) {
                QString comment = document.mid(loc.offset, loc.length);
                if (comment.startsWith(QLatin1Char('!')) || comment.startsWith(QLatin1Char('*'))) {
                    return loc;
                }
            }
        }
    }

    return QQmlJS::AST::SourceLocation();
}

/*!
  Finds the nearest unused qdoc comment above the QML entity
  represented by the \a node and processes the qdoc commands
  in that comment. The proceesed documentation is stored in
  the \a node.

  If a qdoc comment is found about \a location, true is returned.
  If a comment is not found there, false is returned.
 */
bool QmlDocVisitor::applyDocumentation(QQmlJS::AST::SourceLocation location, Node* node)
{
    QQmlJS::AST::SourceLocation loc = precedingComment(location.begin());

    if (loc.isValid()) {
        QString source = document.mid(loc.offset, loc.length);
        Location start(filePath);
        start.setLineNo(loc.startLine);
        start.setColumnNo(loc.startColumn);
        Location finish(filePath);
        finish.setLineNo(loc.startLine);
        finish.setColumnNo(loc.startColumn);

        Doc doc(start, finish, source.mid(1), commands, topics);
        node->setDoc(doc);
        applyMetacommands(loc, node, doc);
        usedComments.insert(loc.offset);
        if (doc.isEmpty()) {
            return false;
        }
        return true;
    }
    Location codeLoc(filePath);
    codeLoc.setLineNo(location.startLine);
    node->setLocation(codeLoc);
    return false;
}

/*!
  A QML property argument has the form...

  <type> <component>::<name>
  <type> <QML-module>::<component>::<name>

  This function splits the argument into one of those
  two forms. The three part form is the old form, which
  was used before the creation of QtQuick 2 and Qt
  Components. A <QML-module> is the QML equivalent of a
  C++ namespace. So this function splits \a arg on "::"
  and stores the parts in the \e {type}, \e {module},
  \e {component}, and \a {name}, fields of \a qpa. If it
  is successful, it returns true. If not enough parts
  are found, a qdoc warning is emitted and false is
  returned.
 */
bool QmlDocVisitor::splitQmlPropertyArg(const Doc& doc,
                                        const QString& arg,
                                        QmlPropArgs& qpa)
{
    qpa.clear();
    QStringList blankSplit = arg.split(QLatin1Char(' '));
    if (blankSplit.size() > 1) {
        qpa.type_ = blankSplit[0];
        QStringList colonSplit(blankSplit[1].split("::"));
        if (colonSplit.size() == 3) {
            qpa.module_ = colonSplit[0];
            qpa.component_ = colonSplit[1];
            qpa.name_ = colonSplit[2];
            return true;
        }
        else if (colonSplit.size() == 2) {
            qpa.component_ = colonSplit[0];
            qpa.name_ = colonSplit[1];
            return true;
        }
        else if (colonSplit.size() == 1) {
            qpa.name_ = colonSplit[0];
            return true;
        }
        QString msg = "Unrecognizable QML module/component qualifier for " + arg;
        doc.location().warning(tr(msg.toLatin1().data()));
    }
    else {
        QString msg = "Missing property type for " + arg;
        doc.location().warning(tr(msg.toLatin1().data()));
    }
    return false;
}

/*!
  Applies the metacommands found in the comment.
 */
void QmlDocVisitor::applyMetacommands(QQmlJS::AST::SourceLocation,
                                      Node* node,
                                      Doc& doc)
{
    QDocDatabase* qdb = QDocDatabase::qdocDB();

    const TopicList& topicsUsed = doc.topicsUsed();
    if (topicsUsed.size() > 0) {
        if (node->type() == Node::QmlProperty) {
            QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
            for (int i=0; i<topicsUsed.size(); ++i) {
                if (topicsUsed.at(i).topic == "qmlproperty") {
                    QmlPropArgs qpa;
                    if (splitQmlPropertyArg(doc, topicsUsed.at(i).args, qpa)) {
                        QmlPropertyNode* n = new QmlPropertyNode(qpn, qpa.name_, qpa.type_, false);
                        n->setLocation(doc.location());
                        qpn->appendQmlPropNode(n);
                        n->setReadOnly(qpn->isReadOnly());
                        if (qpn->isDefault())
                            n->setDefault();
                    }
                    else
                        qDebug() << "  FAILED TO PARSE QML PROPERTY:"
                                 << topicsUsed.at(i).topic << topicsUsed.at(i).args;
                }
            }
        }
    }
    QSet<QString> metacommands = doc.metaCommandsUsed();
    if (metacommands.count() > 0) {
        QString topic;
        ArgList args;
        QSet<QString>::iterator i = metacommands.begin();
        while (i != metacommands.end()) {
            if (topics.contains(*i)) {
                topic = *i;
                break;
            }
            ++i;
        }
        if (!topic.isEmpty()) {
            args = doc.metaCommandArgs(topic);
            if ((topic == COMMAND_QMLCLASS) || (topic == COMMAND_QMLTYPE)) {
                // do nothing.
            }
            else if (topic == COMMAND_QMLPROPERTY) {
                if (node->type() == Node::QmlProperty) {
                    QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
                    qpn->setReadOnly(0);
                    if (qpn->dataType() == "alias") {
                        QStringList part = args[0].first.split(QLatin1Char(' '));
                        qpn->setDataType(part[0]);
                    }
                }
            }
            else if (topic == COMMAND_QMLMODULE) {
            }
            else if (topic == COMMAND_QMLATTACHEDPROPERTY) {
                if (node->type() == Node::QmlProperty) {
                    QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
                    qpn->setReadOnly(0);
                }
            }
            else if (topic == COMMAND_QMLSIGNAL) {
            }
            else if (topic == COMMAND_QMLATTACHEDSIGNAL) {
            }
            else if (topic == COMMAND_QMLMETHOD) {
            }
            else if (topic == COMMAND_QMLATTACHEDMETHOD) {
            }
            else if (topic == COMMAND_QMLBASICTYPE) {
            }
        }
        metacommands.subtract(topics);
        i = metacommands.begin();
        while (i != metacommands.end()) {
            QString command = *i;
            args = doc.metaCommandArgs(command);
            if (command == COMMAND_QMLABSTRACT) {
                if ((node->type() == Node::Document) && (node->subType() == Node::QmlClass)) {
                    node->setAbstract(true);
                }
            }
            else if (command == COMMAND_DEPRECATED) {
                node->setStatus(Node::Obsolete);
            }
            else if (command == COMMAND_INQMLMODULE) {
                qdb->addToQmlModule(args[0].first,node);
            }
            else if (command == COMMAND_QMLINHERITS) {
                if (node->name() == args[0].first)
                    doc.location().warning(tr("%1 tries to inherit itself").arg(args[0].first));
                else if (node->subType() == Node::QmlClass) {
                    QmlClassNode *qmlClass = static_cast<QmlClassNode*>(node);
                    qmlClass->setQmlBaseName(args[0].first);
                    QmlClassNode::addInheritedBy(args[0].first,node);
                }
            }
            else if (command == COMMAND_QMLDEFAULT) {
                if (node->type() == Node::QmlProperty) {
                    QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
                    qpn->setDefault();
                }
            }
            else if (command == COMMAND_QMLREADONLY) {
                if (node->type() == Node::QmlProperty) {
                    QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
                    qpn->setReadOnly(1);
                }
            }
            else if ((command == COMMAND_INGROUP) && !args.isEmpty()) {
                ArgList::ConstIterator argsIter = args.constBegin();
                while (argsIter != args.constEnd()) {
                    QDocDatabase::qdocDB()->addToGroup(argsIter->first, node);
                    ++argsIter;
                }
            }
            else if (command == COMMAND_INTERNAL) {
                node->setAccess(Node::Private);
                node->setStatus(Node::Internal);
            }
            else if (command == COMMAND_OBSOLETE) {
                if (node->status() != Node::Compat)
                    node->setStatus(Node::Obsolete);
            }
            else if (command == COMMAND_PAGEKEYWORDS) {
                // Not done yet. Do we need this?
            }
            else if (command == COMMAND_PRELIMINARY) {
                node->setStatus(Node::Preliminary);
            }
            else if (command == COMMAND_SINCE) {
                QString arg = args[0].first; //.join(' ');
                node->setSince(arg);
            }
            else if (command == COMMAND_WRAPPER) {
                node->setWrapper();
            }
            else {
                doc.location().warning(tr("The \\%1 command is ignored in QML files").arg(command));
            }
            ++i;
        }
    }
}

/*!
  Reconstruct the qualified \a id using dot notation
  and return the fully qualified string.
 */
QString QmlDocVisitor::getFullyQualifiedId(QQmlJS::AST::UiQualifiedId *id)
{
    QString result;
    if (id) {
        result = id->name.toString();
        id = id->next;
        while (id != 0) {
            result += QChar('.') + id->name.toString();
            id = id->next;
        }
    }
    return result;
}

/*!
  Begin the visit of the object \a definition, recording it in the
  qdoc database. Increment the object nesting level, which is used
  to test whether we are at the public API level. The public level
  is level 1.
*/
bool QmlDocVisitor::visit(QQmlJS::AST::UiObjectDefinition *definition)
{
    QString type = getFullyQualifiedId(definition->qualifiedTypeNameId);
    nestingLevel++;

    if (current->type() == Node::Namespace) {
        QmlClassNode *component = new QmlClassNode(current, name);
        component->setTitle(name);
        component->setImportList(importList);
        if (applyDocumentation(definition->firstSourceLocation(), component)) {
            QmlClassNode::addInheritedBy(type, component);
            component->setQmlBaseName(type);
        }
        current = component;
    }

    return true;
}

/*!
  End the visit of the object \a definition. In particular,
  decrement the object nesting level, which is used to test
  whether we are at the public API level. The public API
  level is level 1. It won't decrement below 0.
 */
void QmlDocVisitor::endVisit(QQmlJS::AST::UiObjectDefinition *definition)
{
    if (nestingLevel > 0) {
        --nestingLevel;
    }
    lastEndOffset = definition->lastSourceLocation().end();
}

/*!
  Note that the imports list can be traversed by iteration to obtain
  all the imports in the document at once, having found just one:

  *it = imports; it; it = it->next

 */
bool QmlDocVisitor::visit(QQmlJS::AST::UiImportList *imports)
{
    while (imports != 0) {
        QQmlJS::AST::UiImport* imp = imports->import;

        QString name = document.mid(imp->fileNameToken.offset, imp->fileNameToken.length);
        if (name[0] == '\"')
            name = name.mid(1, name.length()-2);
        QString version = document.mid(imp->versionToken.offset, imp->versionToken.length);
        QString importId = document.mid(imp->importIdToken.offset, imp->importIdToken.length);
        QString importUri = getFullyQualifiedId(imp->importUri);
        importList.append(ImportRec(name, version, importId, importUri));
        imports = imports->next;
    }
    return true;
}

/*!
  End the visit of the imports list.
 */
void QmlDocVisitor::endVisit(QQmlJS::AST::UiImportList *definition)
{
    lastEndOffset = definition->lastSourceLocation().end();
}

bool QmlDocVisitor::visit(QQmlJS::AST::UiObjectBinding *)
{
    ++nestingLevel;
    return true;
}

void QmlDocVisitor::endVisit(QQmlJS::AST::UiObjectBinding *)
{
    --nestingLevel;
}

bool QmlDocVisitor::visit(QQmlJS::AST::UiArrayBinding *)
{
    return true;
}

void QmlDocVisitor::endVisit(QQmlJS::AST::UiArrayBinding *)
{
}

/*!
    Visits the public \a member declaration, which can be a
    signal or a property. It is a custom signal or property.
    Only visit the \a member if the nestingLevel is 1.
*/
bool QmlDocVisitor::visit(QQmlJS::AST::UiPublicMember *member)
{
    if (nestingLevel > 1) {
        return true;
    }
    switch (member->type) {
    case QQmlJS::AST::UiPublicMember::Signal:
    {
        if (current->type() == Node::Document) {
            QmlClassNode *qmlClass = static_cast<QmlClassNode *>(current);
            if (qmlClass) {

                QString name = member->name.toString();
                FunctionNode *qmlSignal = new FunctionNode(Node::QmlSignal, current, name, false);

                QList<Parameter> parameters;
                for (QQmlJS::AST::UiParameterList *it = member->parameters; it; it = it->next) {
                    if (!it->type.isEmpty() && !it->name.isEmpty())
                        parameters.append(Parameter(it->type.toString(), QString(), it->name.toString()));
                }

                qmlSignal->setParameters(parameters);
                applyDocumentation(member->firstSourceLocation(), qmlSignal);
            }
        }
        break;
    }
    case QQmlJS::AST::UiPublicMember::Property:
    {
        QString type = member->memberType.toString();
        QString name = member->name.toString();
        if (current->type() == Node::Document) {
            QmlClassNode *qmlClass = static_cast<QmlClassNode *>(current);
            if (qmlClass) {
                QString name = member->name.toString();
                QmlPropertyNode *qmlPropNode = new QmlPropertyNode(qmlClass, name, type, false);
                qmlPropNode->setReadOnly(member->isReadonlyMember);
                if (member->isDefaultMember)
                    qmlPropNode->setDefault();
                applyDocumentation(member->firstSourceLocation(), qmlPropNode);
            }
        }
        break;
    }
    default:
        return false;
    }

    return true;
}

/*!
  End the visit of the \a member.
 */
void QmlDocVisitor::endVisit(QQmlJS::AST::UiPublicMember* member)
{
    lastEndOffset = member->lastSourceLocation().end();
}

bool QmlDocVisitor::visit(QQmlJS::AST::IdentifierPropertyName *)
{
    return true;
}

/*!
  Begin the visit of the function declaration \a fd, but only
  if the nesting level is 1.
 */
bool QmlDocVisitor::visit(QQmlJS::AST::FunctionDeclaration* fd)
{
    if (nestingLevel > 1) {
        return true;
    }
    if (current->type() == Node::Document) {
        QmlClassNode* qmlClass = static_cast<QmlClassNode*>(current);
        if (qmlClass) {
            QString name = fd->name.toString();
            FunctionNode* qmlMethod = new FunctionNode(Node::QmlMethod, current, name, false);
            int overloads = 0;
            NodeList::ConstIterator overloadIterator = current->childNodes().constBegin();
            while (overloadIterator != current->childNodes().constEnd()) {
                if ((*overloadIterator)->name() == name)
                    overloads++;
                overloadIterator++;
            }
            if (overloads > 1)
                qmlMethod->setOverload(true);
            QList<Parameter> parameters;
            QQmlJS::AST::FormalParameterList* formals = fd->formals;
            if (formals) {
                QQmlJS::AST::FormalParameterList* fpl = formals;
                do {
                    parameters.append(Parameter(QString(), QString(), fpl->name.toString()));
                    fpl = fpl->next;
                } while (fpl && fpl != formals);
                qmlMethod->setParameters(parameters);
            }
            applyDocumentation(fd->firstSourceLocation(), qmlMethod);
        }
    }
    return true;
}

/*!
  End the visit of the function declaration, \a fd.
 */
void QmlDocVisitor::endVisit(QQmlJS::AST::FunctionDeclaration* fd)
{
    lastEndOffset = fd->lastSourceLocation().end();
}

/*!
  Begin the visit of the signal handler declaration \a sb, but only
  if the nesting level is 1.

  This visit is now deprecated. It has been decided to document
  public signals. If a signal handler must be discussed in the
  documentation, that discussion must take place in the comment
  for the signal.
 */
bool QmlDocVisitor::visit(QQmlJS::AST::UiScriptBinding* )
{
#if 0
    if (nestingLevel > 1) {
        return true;
    }
    if (current->type() == Node::Document) {
        QString handler = sb->qualifiedId->name.toString();
        if (handler.length() > 2 && handler.startsWith("on") && handler.at(2).isUpper()) {
            QmlClassNode* qmlClass = static_cast<QmlClassNode*>(current);
            if (qmlClass) {
                FunctionNode* qmlSH = new FunctionNode(Node::QmlSignalHandler,current,handler,false);
                applyDocumentation(sb->firstSourceLocation(), qmlSH);
            }
        }
    }
#endif
    return true;
}

void QmlDocVisitor::endVisit(QQmlJS::AST::UiScriptBinding* sb)
{
    lastEndOffset = sb->lastSourceLocation().end();
}

bool QmlDocVisitor::visit(QQmlJS::AST::UiQualifiedId* )
{
    return true;
}

void QmlDocVisitor::endVisit(QQmlJS::AST::UiQualifiedId* )
{
    // nothing.
}

QT_END_NAMESPACE
