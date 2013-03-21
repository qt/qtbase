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
  cppcodemarker.cpp
*/

#include "atom.h"
#include "cppcodemarker.h"
#include "node.h"
#include "text.h"
#include "tree.h"
#include <qdebug.h>
#include <ctype.h>

QT_BEGIN_NAMESPACE

/*!
  The constructor does nothing.
 */
CppCodeMarker::CppCodeMarker()
{
    // nothing.
}

/*!
  The destructor does nothing.
 */
CppCodeMarker::~CppCodeMarker()
{
    // nothing.
}

/*!
  Returns true.
 */
bool CppCodeMarker::recognizeCode(const QString & /* code */)
{
    return true;
}

/*!
  Returns true if \a ext is any of a list of file extensions
  for the C++ language.
 */
bool CppCodeMarker::recognizeExtension(const QString& extension)
{
    QByteArray ext = extension.toLatin1();
    return ext == "c" ||
            ext == "c++" ||
            ext == "qdoc" ||
            ext == "qtt" ||
            ext == "qtx" ||
            ext == "cc" ||
            ext == "cpp" ||
            ext == "cxx" ||
            ext == "ch" ||
            ext == "h" ||
            ext == "h++" ||
            ext == "hh" ||
            ext == "hpp" ||
            ext == "hxx";
}

/*!
  Returns true if \a lang is either "C" or "Cpp".
 */
bool CppCodeMarker::recognizeLanguage(const QString &lang)
{
    return lang == QLatin1String("C") || lang == QLatin1String("Cpp");
}

/*!
  Returns the type of atom used to represent C++ code in the documentation.
*/
Atom::Type CppCodeMarker::atomType() const
{
    return Atom::Code;
}

QString CppCodeMarker::markedUpCode(const QString &code,
                                    const Node *relative,
                                    const Location &location)
{
    return addMarkUp(code, relative, location);
}

QString CppCodeMarker::markedUpSynopsis(const Node *node,
                                        const Node * /* relative */,
                                        SynopsisStyle style)
{
    const int MaxEnumValues = 6;
    const FunctionNode *func;
    const PropertyNode *property;
    const VariableNode *variable;
    const EnumNode *enume;
    const TypedefNode *typedeff;
    QString synopsis;
    QString extra;
    QString name;

    name = taggedNode(node);
    if (style != Detailed)
        name = linkTag(node, name);
    name = "<@name>" + name + "</@name>";

    if ((style == Detailed) && !node->parent()->name().isEmpty() &&
        (node->type() != Node::Property) && !node->isQmlNode())
        name.prepend(taggedNode(node->parent()) + "::");

    switch (node->type()) {
    case Node::Namespace:
        synopsis = "namespace " + name;
        break;
    case Node::Class:
        synopsis = "class " + name;
        break;
    case Node::Function:
    case Node::QmlSignal:
    case Node::QmlSignalHandler:
    case Node::QmlMethod:
        func = (const FunctionNode *) node;
        if (style != Subpage && !func->returnType().isEmpty())
            synopsis = typified(func->returnType()) + QLatin1Char(' ');
        synopsis += name;
        if (func->metaness() != FunctionNode::MacroWithoutParams) {
            synopsis += "(";
            if (!func->parameters().isEmpty()) {
                //synopsis += QLatin1Char(' ');
                QList<Parameter>::ConstIterator p = func->parameters().constBegin();
                while (p != func->parameters().constEnd()) {
                    if (p != func->parameters().constBegin())
                        synopsis += ", ";
                    synopsis += typified((*p).leftType());
                    if (style != Subpage && !(*p).name().isEmpty())
                        synopsis +=
                                "<@param>" + protect((*p).name()) + "</@param>";
                    synopsis += protect((*p).rightType());
                    if (style != Subpage && !(*p).defaultValue().isEmpty())
                        synopsis += " = " + protect((*p).defaultValue());
                    ++p;
                }
                //synopsis += QLatin1Char(' ');
            }
            synopsis += QLatin1Char(')');
        }
        if (func->isConst())
            synopsis += " const";

        if (style == Summary || style == Accessors) {
            if (func->virtualness() != FunctionNode::NonVirtual)
                synopsis.prepend("virtual ");
            if (func->virtualness() == FunctionNode::PureVirtual)
                synopsis.append(" = 0");
        }
        else if (style == Subpage) {
            if (!func->returnType().isEmpty() && func->returnType() != "void")
                synopsis += " : " + typified(func->returnType());
        }
        else {
            QStringList bracketed;
            if (func->isStatic()) {
                bracketed += "static";
            }
            else if (func->virtualness() != FunctionNode::NonVirtual) {
                if (func->virtualness() == FunctionNode::PureVirtual)
                    bracketed += "pure";
                bracketed += "virtual";
            }

            if (func->access() == Node::Protected) {
                bracketed += "protected";
            }
            else if (func->access() == Node::Private) {
                bracketed += "private";
            }

            if (func->metaness() == FunctionNode::Signal) {
                bracketed += "signal";
            }
            else if (func->metaness() == FunctionNode::Slot) {
                bracketed += "slot";
            }
            if (!bracketed.isEmpty())
                extra += " [" + bracketed.join(' ') + QLatin1Char(']');
        }
        break;
    case Node::Enum:
        enume = static_cast<const EnumNode *>(node);
        synopsis = "enum " + name;
        if (style == Summary) {
            synopsis += " { ";

            QStringList documentedItems = enume->doc().enumItemNames();
            if (documentedItems.isEmpty()) {
                foreach (const EnumItem &item, enume->items())
                    documentedItems << item.name();
            }
            QStringList omitItems = enume->doc().omitEnumItemNames();
            foreach (const QString &item, omitItems)
                documentedItems.removeAll(item);

            if (documentedItems.size() <= MaxEnumValues) {
                for (int i = 0; i < documentedItems.size(); ++i) {
                    if (i != 0)
                        synopsis += ", ";
                    synopsis += documentedItems.at(i);
                }
            }
            else {
                for (int i = 0; i < documentedItems.size(); ++i) {
                    if (i < MaxEnumValues-2 || i == documentedItems.size()-1) {
                        if (i != 0)
                            synopsis += ", ";
                        synopsis += documentedItems.at(i);
                    }
                    else if (i == MaxEnumValues - 1) {
                        synopsis += ", ...";
                    }
                }
            }
            if (!documentedItems.isEmpty())
                synopsis += QLatin1Char(' ');
            synopsis += QLatin1Char('}');
        }
        break;
    case Node::Typedef:
        typedeff = static_cast<const TypedefNode *>(node);
        if (typedeff->associatedEnum()) {
            synopsis = "flags " + name;
        }
        else {
            synopsis = "typedef " + name;
        }
        break;
    case Node::Property:
        property = static_cast<const PropertyNode *>(node);
        synopsis = name + " : " + typified(property->qualifiedDataType());
        break;
    case Node::Variable:
        variable = static_cast<const VariableNode *>(node);
        if (style == Subpage) {
            synopsis = name + " : " + typified(variable->dataType());
        }
        else {
            synopsis = typified(variable->leftType()) + QLatin1Char(' ') +
                    name + protect(variable->rightType());
        }
        break;
    default:
        synopsis = name;
    }

    if (style == Summary) {
        if (node->status() == Node::Preliminary) {
            extra += " (preliminary)";
        }
        else if (node->status() == Node::Deprecated) {
            extra += " (deprecated)";
        }
        else if (node->status() == Node::Obsolete) {
            extra += " (obsolete)";
        }
    }

    if (!extra.isEmpty()) {
        extra.prepend("<@extra>");
        extra.append("</@extra>");
    }
    return synopsis + extra;
}

/*!
 */
QString CppCodeMarker::markedUpQmlItem(const Node* node, bool summary)
{
    QString name = taggedQmlNode(node);
    if (summary)
        name = linkTag(node,name);
    else if (node->type() == Node::QmlProperty) {
        const QmlPropertyNode* pn = static_cast<const QmlPropertyNode*>(node);
        if (pn->isAttached())
            name.prepend(pn->element() + QLatin1Char('.'));
    }
    name = "<@name>" + name + "</@name>";
    QString synopsis;
    if (node->type() == Node::QmlProperty) {
        const QmlPropertyNode* pn = static_cast<const QmlPropertyNode*>(node);
        synopsis = name + " : " + typified(pn->dataType());
    }
    else if ((node->type() == Node::QmlMethod) ||
             (node->type() == Node::QmlSignal) ||
             (node->type() == Node::QmlSignalHandler)) {
        const FunctionNode* func = static_cast<const FunctionNode*>(node);
        if (!func->returnType().isEmpty())
            synopsis = typified(func->returnType()) + QLatin1Char(' ') + name;
        else
            synopsis = name;
        synopsis += QLatin1Char('(');
        if (!func->parameters().isEmpty()) {
            QList<Parameter>::ConstIterator p = func->parameters().constBegin();
            while (p != func->parameters().constEnd()) {
                if (p != func->parameters().constBegin())
                    synopsis += ", ";
                synopsis += typified((*p).leftType());
                if (!(*p).name().isEmpty()) {
                    if (!synopsis.endsWith(QLatin1Char('(')))
                        synopsis += QLatin1Char(' ');
                    synopsis += "<@param>" + protect((*p).name()) + "</@param>";
                }
                synopsis += protect((*p).rightType());
                ++p;
            }
        }
        synopsis += QLatin1Char(')');
    }
    else
        synopsis = name;

    QString extra;
    if (summary) {
        if (node->status() == Node::Preliminary) {
            extra += " (preliminary)";
        }
        else if (node->status() == Node::Deprecated) {
            extra += " (deprecated)";
        }
        else if (node->status() == Node::Obsolete) {
            extra += " (obsolete)";
        }
    }

    if (!extra.isEmpty()) {
        extra.prepend("<@extra>");
        extra.append("</@extra>");
    }
    return synopsis + extra;
}

QString CppCodeMarker::markedUpName(const Node *node)
{
    QString name = linkTag(node, taggedNode(node));
    if (node->type() == Node::Function)
        name += "()";
    return name;
}

QString CppCodeMarker::markedUpFullName(const Node *node, const Node *relative)
{
    if (node->name().isEmpty()) {
        return "global";
    }
    else {
        QString fullName;
        for (;;) {
            fullName.prepend(markedUpName(node));
            if (node->parent() == relative || node->parent()->name().isEmpty())
                break;
            fullName.prepend("<@op>::</@op>");
            node = node->parent();
        }
        return fullName;
    }
}

QString CppCodeMarker::markedUpEnumValue(const QString &enumValue, const Node *relative)
{
    const Node *node = relative->parent();
    QString fullName;
    while (node->parent()) {
        fullName.prepend(markedUpName(node));
        if (node->parent() == relative || node->parent()->name().isEmpty() ||
            node->parent()->isCollisionNode())
            break;
        fullName.prepend("<@op>::</@op>");
        node = node->parent();
    }
    if (!fullName.isEmpty())
        fullName.append("<@op>::</@op>");
    fullName.append(enumValue);
    return fullName;
}

QString CppCodeMarker::markedUpIncludes(const QStringList& includes)
{
    QString code;

    QStringList::ConstIterator inc = includes.constBegin();
    while (inc != includes.constEnd()) {
        code += "<@preprocessor>#include &lt;<@headerfile>" + *inc + "</@headerfile>&gt;</@preprocessor>\n";
        ++inc;
    }
    return code;
}

QString CppCodeMarker::functionBeginRegExp(const QString& funcName)
{
    return QLatin1Char('^') + QRegExp::escape(funcName) + QLatin1Char('$');

}

QString CppCodeMarker::functionEndRegExp(const QString& /* funcName */)
{
    return "^\\}$";
}

QList<Section> CppCodeMarker::sections(const InnerNode *inner,
                                       SynopsisStyle style,
                                       Status status)
{
    QList<Section> sections;

    if (inner->type() == Node::Class) {
        const ClassNode *classNode = static_cast<const ClassNode *>(inner);

        if (style == Summary) {
            FastSection privateFunctions(classNode,
                                         "Private Functions",
                                         QString(),
                                         "private function",
                                         "private functions");
            FastSection privateSlots(classNode, "Private Slots", QString(), "private slot", "private slots");
            FastSection privateTypes(classNode, "Private Types", QString(), "private type", "private types");
            FastSection protectedFunctions(classNode,
                                           "Protected Functions",
                                           QString(),
                                           "protected function",
                                           "protected functions");
            FastSection protectedSlots(classNode,
                                       "Protected Slots",
                                       QString(),
                                       "protected slot",
                                       "protected slots");
            FastSection protectedTypes(classNode,
                                       "Protected Types",
                                       QString(),
                                       "protected type",
                                       "protected types");
            FastSection protectedVariables(classNode,
                                           "Protected Variables",
                                           QString(),
                                           "protected type",
                                           "protected variables");
            FastSection publicFunctions(classNode,
                                        "Public Functions",
                                        QString(),
                                        "public function",
                                        "public functions");
            FastSection publicSignals(classNode, "Signals", QString(), "signal", "signals");
            FastSection publicSlots(classNode, "Public Slots", QString(), "public slot", "public slots");
            FastSection publicTypes(classNode, "Public Types", QString(), "public type", "public types");
            FastSection publicVariables(classNode,
                                        "Public Variables",
                                        QString(),
                                        "public variable",
                                        "public variables");
            FastSection properties(classNode, "Properties", QString(), "property", "properties");
            FastSection relatedNonMembers(classNode,
                                          "Related Non-Members",
                                          QString(),
                                          "related non-member",
                                          "related non-members");
            FastSection staticPrivateMembers(classNode,
                                             "Static Private Members",
                                             QString(),
                                             "static private member",
                                             "static private members");
            FastSection staticProtectedMembers(classNode,
                                               "Static Protected Members",
                                               QString(),
                                               "static protected member",
                                               "static protected members");
            FastSection staticPublicMembers(classNode,
                                            "Static Public Members",
                                            QString(),
                                            "static public member",
                                            "static public members");
            FastSection macros(inner, "Macros", QString(), "macro", "macros");

            NodeList::ConstIterator r = classNode->relatedNodes().constBegin();
            while (r != classNode->relatedNodes().constEnd()) {
                if ((*r)->type() == Node::Function) {
                    FunctionNode *func = static_cast<FunctionNode *>(*r);
                    if (func->isMacro())
                        insert(macros, *r, style, status);
                    else
                        insert(relatedNonMembers, *r, style, status);
                }
                else {
                    insert(relatedNonMembers, *r, style, status);
                }
                ++r;
            }

            QStack<const ClassNode *> stack;
            stack.push(classNode);
            while (!stack.isEmpty()) {
                const ClassNode *ancestorClass = stack.pop();

                NodeList::ConstIterator c = ancestorClass->childNodes().constBegin();
                while (c != ancestorClass->childNodes().constEnd()) {
                    bool isSlot = false;
                    bool isSignal = false;
                    bool isStatic = false;
                    if ((*c)->type() == Node::Function) {
                        const FunctionNode *func = (const FunctionNode *) *c;
                        isSlot = (func->metaness() == FunctionNode::Slot);
                        isSignal = (func->metaness() == FunctionNode::Signal);
                        isStatic = func->isStatic();
                        if (func->associatedProperty()) {
                            if (func->associatedProperty()->status() == Node::Obsolete) {
                                ++c;
                                continue;
                            }
                        }
                    }
                    else if ((*c)->type() == Node::Variable) {
                        const VariableNode *var = static_cast<const VariableNode *>(*c);
                        isStatic = var->isStatic();
                    }

                    switch ((*c)->access()) {
                    case Node::Public:
                        if (isSlot) {
                            insert(publicSlots, *c, style, status);
                        }
                        else if (isSignal) {
                            insert(publicSignals, *c, style, status);
                        }
                        else if (isStatic) {
                            if ((*c)->type() != Node::Variable || !(*c)->doc().isEmpty())
                                insert(staticPublicMembers,*c,style,status);
                        }
                        else if ((*c)->type() == Node::Property) {
                            insert(properties, *c, style, status);
                        }
                        else if ((*c)->type() == Node::Variable) {
                            if (!(*c)->doc().isEmpty())
                                insert(publicVariables, *c, style, status);
                        }
                        else if ((*c)->type() == Node::Function) {
                            if (!insertReimpFunc(publicFunctions,*c,status)) {
                                insert(publicFunctions, *c, style, status);
                            }
                        }
                        else {
                            insert(publicTypes, *c, style, status);
                        }
                        break;
                    case Node::Protected:
                        if (isSlot) {
                            insert(protectedSlots, *c, style, status);
                        }
                        else if (isStatic) {
                            if ((*c)->type() != Node::Variable || !(*c)->doc().isEmpty())
                                insert(staticProtectedMembers,*c,style,status);
                        }
                        else if ((*c)->type() == Node::Variable) {
                            if (!(*c)->doc().isEmpty())
                                insert(protectedVariables,*c,style,status);
                        }
                        else if ((*c)->type() == Node::Function) {
                            if (!insertReimpFunc(protectedFunctions,*c,status)) {
                                insert(protectedFunctions, *c, style, status);
                            }
                        }
                        else {
                            insert(protectedTypes, *c, style, status);
                        }
                        break;
                    case Node::Private:
                        if (isSlot) {
                            insert(privateSlots, *c, style, status);
                        }
                        else if (isStatic) {
                            if ((*c)->type() != Node::Variable || !(*c)->doc().isEmpty())
                                insert(staticPrivateMembers,*c,style,status);
                        }
                        else if ((*c)->type() == Node::Function) {
                            if (!insertReimpFunc(privateFunctions,*c,status)) {
                                insert(privateFunctions, *c, style, status);
                            }
                        }
                        else {
                            insert(privateTypes,*c,style,status);
                        }
                    }
                    ++c;
                }

                QList<RelatedClass>::ConstIterator r =
                        ancestorClass->baseClasses().constBegin();
                while (r != ancestorClass->baseClasses().constEnd()) {
                    stack.prepend((*r).node);
                    ++r;
                }
            }

            append(sections, publicTypes);
            append(sections, properties);
            append(sections, publicFunctions);
            append(sections, publicSlots);
            append(sections, publicSignals);
            append(sections, publicVariables);
            append(sections, staticPublicMembers);
            append(sections, protectedTypes);
            append(sections, protectedFunctions);
            append(sections, protectedSlots);
            append(sections, protectedVariables);
            append(sections, staticProtectedMembers);
            append(sections, privateTypes);
            append(sections, privateFunctions);
            append(sections, privateSlots);
            append(sections, staticPrivateMembers);
            append(sections, relatedNonMembers);
            append(sections, macros);
        }
        else if (style == Detailed) {
            FastSection memberFunctions(classNode,"Member Function Documentation","func","member","members");
            FastSection memberTypes(classNode,"Member Type Documentation","types","member","members");
            FastSection memberVariables(classNode,"Member Variable Documentation","vars","member","members");
            FastSection properties(classNode,"Property Documentation","prop","member","members");
            FastSection relatedNonMembers(classNode,"Related Non-Members","relnonmem","member","members");
            FastSection macros(classNode,"Macro Documentation","macros","member","members");

            NodeList::ConstIterator r = classNode->relatedNodes().constBegin();
            while (r != classNode->relatedNodes().constEnd()) {
                if ((*r)->type() == Node::Function) {
                    FunctionNode *func = static_cast<FunctionNode *>(*r);
                    if (func->isMacro())
                        insert(macros, *r, style, status);
                    else
                        insert(relatedNonMembers, *r, style, status);
                }
                else {
                    insert(relatedNonMembers, *r, style, status);
                }
                ++r;
            }

            NodeList::ConstIterator c = classNode->childNodes().constBegin();
            while (c != classNode->childNodes().constEnd()) {
                if ((*c)->type() == Node::Enum ||
                        (*c)->type() == Node::Typedef) {
                    insert(memberTypes, *c, style, status);
                }
                else if ((*c)->type() == Node::Property) {
                    insert(properties, *c, style, status);
                }
                else if ((*c)->type() == Node::Variable) {
                    if (!(*c)->doc().isEmpty())
                        insert(memberVariables, *c, style, status);
                }
                else if ((*c)->type() == Node::Function) {
                    FunctionNode *function = static_cast<FunctionNode *>(*c);
                    if (!function->associatedProperty())
                        insert(memberFunctions, function, style, status);
                }
                ++c;
            }

            append(sections, memberTypes);
            append(sections, properties);
            append(sections, memberFunctions);
            append(sections, memberVariables);
            append(sections, relatedNonMembers);
            append(sections, macros);
        }
        else {
            FastSection all(classNode,QString(),QString(),"member","members");

            QStack<const ClassNode *> stack;
            stack.push(classNode);

            while (!stack.isEmpty()) {
                const ClassNode *ancestorClass = stack.pop();

                NodeList::ConstIterator c = ancestorClass->childNodes().constBegin();
                while (c != ancestorClass->childNodes().constEnd()) {
                    if ((*c)->access() != Node::Private &&
                            (*c)->type() != Node::Property)
                        insert(all, *c, style, status);
                    ++c;
                }

                QList<RelatedClass>::ConstIterator r =
                        ancestorClass->baseClasses().constBegin();
                while (r != ancestorClass->baseClasses().constEnd()) {
                    stack.prepend((*r).node);
                    ++r;
                }
            }
            append(sections, all);
        }
    }
    else {
        if (style == Summary || style == Detailed) {
            FastSection namespaces(inner,
                                   "Namespaces",
                                   style == Detailed ? "nmspace" : QString(),
                                   "namespace",
                                   "namespaces");
            FastSection classes(inner,
                                "Classes",
                                style == Detailed ? "classes" : QString(),
                                "class",
                                "classes");
            FastSection types(inner,
                              style == Summary ? "Types" : "Type Documentation",
                              style == Detailed ? "types" : QString(),
                              "type",
                              "types");
            FastSection functions(inner,
                                  style == Summary ?
                                      "Functions" : "Function Documentation",
                                  style == Detailed ? "func" : QString(),
                                  "function",
                                  "functions");
            FastSection macros(inner,
                               style == Summary ?
                                   "Macros" : "Macro Documentation",
                               style == Detailed ? "macros" : QString(),
                               "macro",
                               "macros");

            NodeList nodeList = inner->childNodes();
            nodeList += inner->relatedNodes();

            NodeList::ConstIterator n = nodeList.constBegin();
            while (n != nodeList.constEnd()) {
                switch ((*n)->type()) {
                case Node::Namespace:
                    insert(namespaces, *n, style, status);
                    break;
                case Node::Class:
                    insert(classes, *n, style, status);
                    break;
                case Node::Enum:
                case Node::Typedef:
                    insert(types, *n, style, status);
                    break;
                case Node::Function:
                {
                    FunctionNode *func = static_cast<FunctionNode *>(*n);
                    if (func->isMacro())
                        insert(macros, *n, style, status);
                    else
                        insert(functions, *n, style, status);
                }
                    break;
                default:
                    ;
                }
                ++n;
            }
            append(sections, namespaces);
            append(sections, classes);
            append(sections, types);
            append(sections, functions);
            append(sections, macros);
        }
    }

    return sections;
}

static const char * const typeTable[] = {
    "bool", "char", "double", "float", "int", "long", "short",
    "signed", "unsigned", "uint", "ulong", "ushort", "uchar", "void",
    "qlonglong", "qulonglong",
    "qint", "qint8", "qint16", "qint32", "qint64",
    "quint", "quint8", "quint16", "quint32", "quint64",
    "qreal", "cond", 0
};

static const char * const keywordTable[] = {
    "and", "and_eq", "asm", "auto", "bitand", "bitor", "break",
    "case", "catch", "class", "compl", "const", "const_cast",
    "continue", "default", "delete", "do", "dynamic_cast", "else",
    "enum", "explicit", "export", "extern", "false", "for", "friend",
    "goto", "if", "include", "inline", "monitor", "mutable", "namespace",
    "new", "not", "not_eq", "operator", "or", "or_eq", "private", "protected",
    "public", "register", "reinterpret_cast", "return", "sizeof",
    "static", "static_cast", "struct", "switch", "template", "this",
    "throw", "true", "try", "typedef", "typeid", "typename", "union",
    "using", "virtual", "volatile", "wchar_t", "while", "xor",
    "xor_eq", "synchronized",
    // Qt specific
    "signals", "slots", "emit", 0
};

/*
    @char
    @class
    @comment
    @function
    @keyword
    @number
    @op
    @preprocessor
    @string
    @type
*/

QString CppCodeMarker::addMarkUp(const QString &in,
                                 const Node * /* relative */,
                                 const Location & /* location */)
{
#define readChar() \
    ch = (i < (int)code.length()) ? code[i++].cell() : EOF

    QString code = in;

    QMap<QString, int> types;
    QMap<QString, int> keywords;
    int j = 0;
    while (typeTable[j] != 0) {
        types.insert(QString(typeTable[j]), 0);
        j++;
    }
    j = 0;
    while (keywordTable[j] != 0) {
        keywords.insert(QString(keywordTable[j]), 0);
        j++;
    }

    QString out;
    int braceDepth = 0;
    int parenDepth = 0;
    int i = 0;
    int start = 0;
    int finish = 0;
    QChar ch;
    QRegExp classRegExp("Qt?(?:[A-Z3]+[a-z][A-Za-z]*|t)");
    QRegExp functionRegExp("q([A-Z][a-z]+)+");

    readChar();

    while (ch != EOF) {
        QString tag;
        bool target = false;

        if (ch.isLetter() || ch == '_') {
            QString ident;
            do {
                ident += ch;
                finish = i;
                readChar();
            } while (ch.isLetterOrNumber() || ch == '_');

            if (classRegExp.exactMatch(ident)) {
                tag = QLatin1String("type");
            } else if (functionRegExp.exactMatch(ident)) {
                tag = QLatin1String("func");
                target = true;
            } else if (types.contains(ident)) {
                tag = QLatin1String("type");
            } else if (keywords.contains(ident)) {
                tag = QLatin1String("keyword");
            } else if (braceDepth == 0 && parenDepth == 0) {
                if (QString(code.unicode() + i - 1, code.length() - (i - 1))
                        .indexOf(QRegExp(QLatin1String("^\\s*\\("))) == 0)
                    tag = QLatin1String("func");
                target = true;
            }
        } else if (ch.isDigit()) {
            do {
                finish = i;
                readChar();
            } while (ch.isLetterOrNumber() || ch == '.');
            tag = QLatin1String("number");
        } else {
            switch (ch.unicode()) {
            case '+':
            case '-':
            case '!':
            case '%':
            case '^':
            case '&':
            case '*':
            case ',':
            case '.':
            case '<':
            case '=':
            case '>':
            case '?':
            case '[':
            case ']':
            case '|':
            case '~':
                finish = i;
                readChar();
                tag = QLatin1String("op");
                break;
            case '"':
                finish = i;
                readChar();

                while (ch != EOF && ch != '"') {
                    if (ch == '\\')
                        readChar();
                    readChar();
                }
                finish = i;
                readChar();
                tag = QLatin1String("string");
                break;
            case '#':
                finish = i;
                readChar();
                while (ch != EOF && ch != '\n') {
                    if (ch == '\\')
                        readChar();
                    finish = i;
                    readChar();
                }
                tag = QLatin1String("preprocessor");
                break;
            case '\'':
                finish = i;
                readChar();

                while (ch != EOF && ch != '\'') {
                    if (ch == '\\')
                        readChar();
                    readChar();
                }
                finish = i;
                readChar();
                tag = QLatin1String("char");
                break;
            case '(':
                finish = i;
                readChar();
                parenDepth++;
                break;
            case ')':
                finish = i;
                readChar();
                parenDepth--;
                break;
            case ':':
                finish = i;
                readChar();
                if (ch == ':') {
                    finish = i;
                    readChar();
                    tag = QLatin1String("op");
                }
                break;
            case '/':
                finish = i;
                readChar();
                if (ch == '/') {
                    do {
                        finish = i;
                        readChar();
                    } while (ch != EOF && ch != '\n');
                    tag = QLatin1String("comment");
                } else if (ch == '*') {
                    bool metAster = false;
                    bool metAsterSlash = false;

                    finish = i;
                    readChar();

                    while (!metAsterSlash) {
                        if (ch == EOF)
                            break;

                        if (ch == '*')
                            metAster = true;
                        else if (metAster && ch == '/')
                            metAsterSlash = true;
                        else
                            metAster = false;
                        finish = i;
                        readChar();
                    }
                    tag = QLatin1String("comment");
                } else {
                    tag = QLatin1String("op");
                }
                break;
            case '{':
                finish = i;
                readChar();
                braceDepth++;
                break;
            case '}':
                finish = i;
                readChar();
                braceDepth--;
                break;
            default:
                finish = i;
                readChar();
            }
        }

        QString text;
        text = code.mid(start, finish - start);
        start = finish;

        if (!tag.isEmpty()) {
            out += QLatin1String("<@") + tag;
            if (target)
                out += QLatin1String(" target=\"") + text + QLatin1String("()\"");
            out += QLatin1Char('>');
        }

        out += protect(text);

        if (!tag.isEmpty())
            out += QLatin1String("</@") + tag + QLatin1Char('>');
    }

    if (start < code.length()) {
        out += protect(code.mid(start));
    }

    return out;
}

/*!
  This function is for documenting QML properties. It returns
  the list of documentation sections for the children of the
  \a qmlClassNode.
 */
QList<Section> CppCodeMarker::qmlSections(const QmlClassNode* qmlClassNode, SynopsisStyle style)
{
    QList<Section> sections;
    if (qmlClassNode) {
        if (style == Summary) {
            FastSection qmlproperties(qmlClassNode,
                                      "Properties",
                                      QString(),
                                      "property",
                                      "properties");
            FastSection qmlattachedproperties(qmlClassNode,
                                              "Attached Properties",
                                              QString(),
                                              "property",
                                              "properties");
            FastSection qmlsignals(qmlClassNode,
                                   "Signals",
                                   QString(),
                                   "signal",
                                   "signals");
            FastSection qmlsignalhandlers(qmlClassNode,
                                          "Signal Handlers",
                                          QString(),
                                          "signal handler",
                                          "signal handlers");
            FastSection qmlattachedsignals(qmlClassNode,
                                           "Attached Signals",
                                           QString(),
                                           "signal",
                                           "signals");
            FastSection qmlmethods(qmlClassNode,
                                   "Methods",
                                   QString(),
                                   "method",
                                   "methods");
            FastSection qmlattachedmethods(qmlClassNode,
                                           "Attached Methods",
                                           QString(),
                                           "method",
                                           "methods");

            const QmlClassNode* qcn = qmlClassNode;
            while (qcn != 0) {
                NodeList::ConstIterator c = qcn->childNodes().constBegin();
                while (c != qcn->childNodes().constEnd()) {
                    if ((*c)->subType() == Node::QmlPropertyGroup) {
                        const QmlPropGroupNode* qpgn = static_cast<const QmlPropGroupNode*>(*c);
                        NodeList::ConstIterator p = qpgn->childNodes().constBegin();
                        while (p != qpgn->childNodes().constEnd()) {
                            if ((*p)->type() == Node::QmlProperty) {
                                const QmlPropertyNode* pn = static_cast<const QmlPropertyNode*>(*p);
                                if (pn->isAttached())
                                    insert(qmlattachedproperties,*p,style,Okay);
                                else
                                    insert(qmlproperties,*p,style,Okay);
                            }
                            ++p;
                        }
                    }
                    else if ((*c)->type() == Node::QmlProperty) {
                        const QmlPropertyNode* pn = static_cast<const QmlPropertyNode*>(*c);
                        if (pn->qmlPropNodes().isEmpty()) {
                            if (pn->isAttached())
                                insert(qmlattachedproperties,*c,style,Okay);
                            else
                                insert(qmlproperties,*c,style,Okay);
                        }
                        else {
                            NodeList::ConstIterator p = pn->qmlPropNodes().constBegin();
                            while (p != pn->qmlPropNodes().constEnd()) {
                                if ((*p)->type() == Node::QmlProperty) {
                                    const QmlPropertyNode* pn = static_cast<const QmlPropertyNode*>(*p);
                                    if (pn->isAttached())
                                        insert(qmlattachedproperties,*p,style,Okay);
                                    else
                                        insert(qmlproperties,*p,style,Okay);
                                }
                                ++p;
                            }
                        }
                    }
                    else if ((*c)->type() == Node::QmlSignal) {
                        const FunctionNode* sn = static_cast<const FunctionNode*>(*c);
                        if (sn->isAttached())
                            insert(qmlattachedsignals,*c,style,Okay);
                        else
                            insert(qmlsignals,*c,style,Okay);
                    }
                    else if ((*c)->type() == Node::QmlSignalHandler) {
                        insert(qmlsignalhandlers,*c,style,Okay);
                    }
                    else if ((*c)->type() == Node::QmlMethod) {
                        const FunctionNode* mn = static_cast<const FunctionNode*>(*c);
                        if (mn->isAttached())
                            insert(qmlattachedmethods,*c,style,Okay);
                        else
                            insert(qmlmethods,*c,style,Okay);
                    }
                    ++c;
                }
                if (qcn->qmlBaseNode() != 0) {
                    qcn = static_cast<const QmlClassNode*>(qcn->qmlBaseNode());
                    if (!qcn->isAbstract())
                        qcn = 0;
                }
                else
                    qcn = 0;
            }
            append(sections,qmlproperties);
            append(sections,qmlattachedproperties);
            append(sections,qmlsignals);
            append(sections,qmlsignalhandlers);
            append(sections,qmlattachedsignals);
            append(sections,qmlmethods);
            append(sections,qmlattachedmethods);
        }
        else if (style == Detailed) {
            FastSection qmlproperties(qmlClassNode, "Property Documentation","qmlprop","member","members");
            FastSection qmlattachedproperties(qmlClassNode,"Attached Property Documentation","qmlattprop",
                                              "member","members");
            FastSection qmlsignals(qmlClassNode,"Signal Documentation","qmlsig","signal","signals");
            FastSection qmlsignalhandlers(qmlClassNode,"Signal Handler Documentation","qmlsighan","signal handler","signal handlers");
            FastSection qmlattachedsignals(qmlClassNode,"Attached Signal Documentation","qmlattsig",
                                           "signal","signals");
            FastSection qmlmethods(qmlClassNode,"Method Documentation","qmlmeth","member","members");
            FastSection qmlattachedmethods(qmlClassNode,"Attached Method Documentation","qmlattmeth",
                                           "member","members");
            const QmlClassNode* qcn = qmlClassNode;
            while (qcn != 0) {
                NodeList::ConstIterator c = qcn->childNodes().constBegin();
                while (c != qcn->childNodes().constEnd()) {
                    if ((*c)->subType() == Node::QmlPropertyGroup) {
                        bool attached = false;
                        const QmlPropGroupNode* pgn = static_cast<const QmlPropGroupNode*>(*c);
                        NodeList::ConstIterator C = pgn->childNodes().constBegin();
                        while (C != pgn->childNodes().constEnd()) {
                            if ((*C)->type() == Node::QmlProperty) {
                                const QmlPropertyNode* pn = static_cast<const QmlPropertyNode*>(*C);
                                if (pn->isAttached()) {
                                    attached = true;
                                    break;
                                }
                            }
                            ++C;
                        }
                        if (attached)
                            insert(qmlattachedproperties,*c,style,Okay);
                        else
                            insert(qmlproperties,*c,style,Okay);
                    }
                    else if ((*c)->type() == Node::QmlProperty) {
                        const QmlPropertyNode* pn = static_cast<const QmlPropertyNode*>(*c);
                        if (pn->isAttached())
                            insert(qmlattachedproperties,*c,style,Okay);
                        else
                            insert(qmlproperties,*c,style,Okay);
                    }
                    else if ((*c)->type() == Node::QmlSignal) {
                        const FunctionNode* sn = static_cast<const FunctionNode*>(*c);
                        if (sn->isAttached())
                            insert(qmlattachedsignals,*c,style,Okay);
                        else
                            insert(qmlsignals,*c,style,Okay);
                    }
                    else if ((*c)->type() == Node::QmlSignalHandler) {
                        insert(qmlsignalhandlers,*c,style,Okay);
                    }
                    else if ((*c)->type() == Node::QmlMethod) {
                        const FunctionNode* mn = static_cast<const FunctionNode*>(*c);
                        if (mn->isAttached())
                            insert(qmlattachedmethods,*c,style,Okay);
                        else
                            insert(qmlmethods,*c,style,Okay);
                    }
                    ++c;
                }
                if (qcn->qmlBaseNode() != 0) {
                    qcn = static_cast<const QmlClassNode*>(qcn->qmlBaseNode());
                    if (!qcn->isAbstract())
                        qcn = 0;
                }
                else
                    qcn = 0;
            }
            append(sections,qmlproperties);
            append(sections,qmlattachedproperties);
            append(sections,qmlsignals);
            append(sections,qmlsignalhandlers);
            append(sections,qmlattachedsignals);
            append(sections,qmlmethods);
            append(sections,qmlattachedmethods);
        }
        else {
            /*
              This is where the list of all members including inherited
              members is prepared.
             */
            ClassMap* classMap = 0;
            FastSection all(qmlClassNode,QString(),QString(),"member","members");
            const QmlClassNode* current = qmlClassNode;
            while (current != 0) {
                /*
                  If the QML type is abstract, do not create
                  a new entry in the list for it. Instead,
                  add its members to the current entry.
                 */
                if (!current->isAbstract()) {
                    classMap = new ClassMap;
                    classMap->first = current;
                    all.classMapList_.append(classMap);
                }
                NodeList::ConstIterator c = current->childNodes().constBegin();
                while (c != current->childNodes().constEnd()) {
                    if ((*c)->subType() == Node::QmlPropertyGroup) {
                        const QmlPropGroupNode* qpgn = static_cast<const QmlPropGroupNode*>(*c);
                        NodeList::ConstIterator p = qpgn->childNodes().constBegin();
                        while (p != qpgn->childNodes().constEnd()) {
                            if ((*p)->type() == Node::QmlProperty) {
                                QString key = (*p)->name();
                                key = sortName(*p, &key);
                                all.memberMap.insert(key,*p);
                                classMap->second.insert(key,*p);
                            }
                            ++p;
                        }
                    }
                    else {
                        QString key = (*c)->name();
                        key = sortName(*c, &key);
                        all.memberMap.insert(key,*c);
                        classMap->second.insert(key,*c);
                    }
                    ++c;
                }
                current = current->qmlBaseNode();
                while (current) {
                    if (current->isAbstract())
                        break;
                    if (current->isInternal())
                        current = current->qmlBaseNode();
                    else
                        break;
                }
            }
            append(sections, all, true);
        }
    }

    return sections;
}

QT_END_NAMESPACE
