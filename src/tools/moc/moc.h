/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOC_H
#define MOC_H

#include "parser.h"
#include <qstringlist.h>
#include <qmap.h>
#include <qpair.h>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <stdio.h>
#include <ctype.h>

QT_BEGIN_NAMESPACE

struct QMetaObject;

struct Type
{
    enum ReferenceType { NoReference, Reference, RValueReference, Pointer };

    inline Type() : isVolatile(false), isScoped(false), firstToken(NOTOKEN), referenceType(NoReference) {}
    inline explicit Type(const QByteArray &_name)
        : name(_name), rawName(name), isVolatile(false), isScoped(false), firstToken(NOTOKEN), referenceType(NoReference) {}
    QByteArray name;
    //When used as a return type, the type name may be modified to remove the references.
    // rawName is the type as found in the function signature
    QByteArray rawName;
    uint isVolatile : 1;
    uint isScoped : 1;
    Token firstToken;
    ReferenceType referenceType;
};
Q_DECLARE_TYPEINFO(Type, Q_MOVABLE_TYPE);

struct EnumDef
{
    QByteArray name;
    QList<QByteArray> values;
    bool isEnumClass; // c++11 enum class
    EnumDef() : isEnumClass(false) {}
};
Q_DECLARE_TYPEINFO(EnumDef, Q_MOVABLE_TYPE);

struct ArgumentDef
{
    ArgumentDef() : isDefault(false) {}
    Type type;
    QByteArray rightType, normalizedType, name;
    QByteArray typeNameForCast; // type name to be used in cast from void * in metacall
    bool isDefault;
};
Q_DECLARE_TYPEINFO(ArgumentDef, Q_MOVABLE_TYPE);

struct FunctionDef
{
    FunctionDef(): returnTypeIsVolatile(false), access(Private), isConst(false), isVirtual(false), isStatic(false),
                   inlineCode(false), wasCloned(false), isCompat(false), isInvokable(false),
                   isScriptable(false), isSlot(false), isSignal(false), isPrivateSignal(false),
                   isConstructor(false), isDestructor(false), isAbstract(false), revision(0) {}
    Type type;
    QByteArray normalizedType;
    QByteArray tag;
    QByteArray name;
    bool returnTypeIsVolatile;

    QVector<ArgumentDef> arguments;

    enum Access { Private, Protected, Public };
    Access access;
    bool isConst;
    bool isVirtual;
    bool isStatic;
    bool inlineCode;
    bool wasCloned;

    QByteArray inPrivateClass;
    bool isCompat;
    bool isInvokable;
    bool isScriptable;
    bool isSlot;
    bool isSignal;
    bool isPrivateSignal;
    bool isConstructor;
    bool isDestructor;
    bool isAbstract;

    int revision;
};
Q_DECLARE_TYPEINFO(FunctionDef, Q_MOVABLE_TYPE);

struct PropertyDef
{
    PropertyDef():notifyId(-1), constant(false), final(false), gspec(ValueSpec), revision(0){}
    QByteArray name, type, member, read, write, reset, designable, scriptable, editable, stored, user, notify, inPrivateClass;
    int notifyId; // -1 means no notifyId, >= 0 means signal defined in this class, < -1 means signal not defined in this class
    bool constant;
    bool final;
    enum Specification  { ValueSpec, ReferenceSpec, PointerSpec };
    Specification gspec;
    bool stdCppSet() const {
        QByteArray s("set");
        s += toupper(name[0]);
        s += name.mid(1);
        return (s == write);
    }
    int revision;
};
Q_DECLARE_TYPEINFO(PropertyDef, Q_MOVABLE_TYPE);


struct ClassInfoDef
{
    QByteArray name;
    QByteArray value;
};
Q_DECLARE_TYPEINFO(ClassInfoDef, Q_MOVABLE_TYPE);

struct BaseDef {
    QByteArray classname;
    QByteArray qualified;
    QVector<ClassInfoDef> classInfoList;
    QMap<QByteArray, bool> enumDeclarations;
    QVector<EnumDef> enumList;
    QMap<QByteArray, QByteArray> flagAliases;
    int begin = 0;
    int end = 0;
};

struct ClassDef : BaseDef {
    QVector<QPair<QByteArray, FunctionDef::Access> > superclassList;

    struct Interface
    {
        Interface() {} // for QVector, don't use
        inline explicit Interface(const QByteArray &_className)
            : className(_className) {}
        QByteArray className;
        QByteArray interfaceId;
    };
    QVector<QVector<Interface> >interfaceList;

    bool hasQObject = false;
    bool hasQGadget = false;

    struct PluginData {
        QByteArray iid;
        QMap<QString, QJsonArray> metaArgs;
        QJsonDocument metaData;
    } pluginData;

    QVector<FunctionDef> constructorList;
    QVector<FunctionDef> signalList, slotList, methodList, publicList;
    QVector<QByteArray> nonClassSignalList;
    int notifyableProperties = 0;
    QVector<PropertyDef> propertyList;
    int revisionedMethods = 0;
    int revisionedProperties = 0;

};
Q_DECLARE_TYPEINFO(ClassDef, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(ClassDef::Interface, Q_MOVABLE_TYPE);

struct NamespaceDef : BaseDef {
    bool hasQNamespace = false;
};
Q_DECLARE_TYPEINFO(NamespaceDef, Q_MOVABLE_TYPE);

class Moc : public Parser
{
public:
    Moc()
        : noInclude(false), mustIncludeQPluginH(false)
        {}

    QByteArray filename;

    bool noInclude;
    bool mustIncludeQPluginH;
    QByteArray includePath;
    QList<QByteArray> includeFiles;
    QVector<ClassDef> classList;
    QMap<QByteArray, QByteArray> interface2IdMap;
    QList<QByteArray> metaTypes;
    // map from class name to fully qualified name
    QHash<QByteArray, QByteArray> knownQObjectClasses;
    QHash<QByteArray, QByteArray> knownGadgets;
    QMap<QString, QJsonArray> metaArgs;

    void parse();
    void generate(FILE *out);

    bool parseClassHead(ClassDef *def);
    inline bool inClass(const ClassDef *def) const {
        return index > def->begin && index < def->end - 1;
    }

    inline bool inNamespace(const NamespaceDef *def) const {
        return index > def->begin && index < def->end - 1;
    }

    Type parseType();

    bool parseEnum(EnumDef *def);

    bool parseFunction(FunctionDef *def, bool inMacro = false);
    bool parseMaybeFunction(const ClassDef *cdef, FunctionDef *def);

    void parseSlots(ClassDef *def, FunctionDef::Access access);
    void parseSignals(ClassDef *def);
    void parseProperty(ClassDef *def);
    void parsePluginData(ClassDef *def);
    void createPropertyDef(PropertyDef &def);
    void parseEnumOrFlag(BaseDef *def, bool isFlag);
    void parseFlag(BaseDef *def);
    void parseClassInfo(BaseDef *def);
    void parseInterfaces(ClassDef *def);
    void parseDeclareInterface();
    void parseDeclareMetatype();
    void parseSlotInPrivate(ClassDef *def, FunctionDef::Access access);
    void parsePrivateProperty(ClassDef *def);

    void parseFunctionArguments(FunctionDef *def);

    QByteArray lexemUntil(Token);
    bool until(Token);

    // test for Q_INVOCABLE, Q_SCRIPTABLE, etc. and set the flags
    // in FunctionDef accordingly
    bool testFunctionAttribute(FunctionDef *def);
    bool testFunctionAttribute(Token tok, FunctionDef *def);
    bool testFunctionRevision(FunctionDef *def);

    void checkSuperClasses(ClassDef *def);
    void checkProperties(ClassDef* cdef);
};

inline QByteArray noRef(const QByteArray &type)
{
    if (type.endsWith('&')) {
        if (type.endsWith("&&"))
            return type.left(type.length()-2);
        return type.left(type.length()-1);
    }
    return type;
}

QT_END_NAMESPACE

#endif // MOC_H
