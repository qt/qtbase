// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MOC_H
#define MOC_H

#include "parser.h"
#include <qstringlist.h>
#include <qmap.h>
#include <qpair.h>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qversionnumber.h>
#include <stdio.h>

#include <private/qtools_p.h>

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
Q_DECLARE_TYPEINFO(Type, Q_RELOCATABLE_TYPE);

struct ClassDef;
struct EnumDef
{
    QByteArray name;
    QByteArray enumName;
    QByteArray type;
    QList<QByteArray> values;
    bool isEnumClass; // c++11 enum class
    EnumDef() : isEnumClass(false) {}
    QJsonObject toJson(const ClassDef &cdef) const;
    QByteArray qualifiedType(const ClassDef *cdef) const;
};
Q_DECLARE_TYPEINFO(EnumDef, Q_RELOCATABLE_TYPE);

struct ArgumentDef
{
    ArgumentDef() : isDefault(false) {}
    Type type;
    QByteArray rightType, normalizedType, name;
    QByteArray typeNameForCast; // type name to be used in cast from void * in metacall
    bool isDefault;

    QJsonObject toJson() const;
};
Q_DECLARE_TYPEINFO(ArgumentDef, Q_RELOCATABLE_TYPE);

struct FunctionDef
{
    Type type;
    QList<ArgumentDef> arguments;
    QByteArray normalizedType;
    QByteArray tag;
    QByteArray name;
    QByteArray inPrivateClass;

    enum Access { Private, Protected, Public };
    Access access = Private;
    int revision = 0;

    bool isConst = false;
    bool isVirtual = false;
    bool isStatic = false;
    bool inlineCode = false;
    bool wasCloned = false;

    bool returnTypeIsVolatile = false;

    bool isCompat = false;
    bool isInvokable = false;
    bool isScriptable = false;
    bool isSlot = false;
    bool isSignal = false;
    bool isPrivateSignal = false;
    bool isConstructor = false;
    bool isDestructor = false;
    bool isAbstract = false;
    bool isRawSlot = false;

    QJsonObject toJson() const;
    static void accessToJson(QJsonObject *obj, Access acs);
};
Q_DECLARE_TYPEINFO(FunctionDef, Q_RELOCATABLE_TYPE);

struct PropertyDef
{
    bool stdCppSet() const {
        if (name.isEmpty())
            return false;
        QByteArray s("set");
        s += QtMiscUtils::toAsciiUpper(name[0]);
        s += name.mid(1);
        return (s == write);
    }

    QByteArray name, type, member, read, write, bind, reset, designable, scriptable, stored, user, notify, inPrivateClass;
    int notifyId = -1; // -1 means no notifyId, >= 0 means signal defined in this class, < -1 means signal not defined in this class
    enum Specification  { ValueSpec, ReferenceSpec, PointerSpec };
    Specification gspec = ValueSpec;
    int revision = 0;
    bool constant = false;
    bool final = false;
    bool required = false;
    int relativeIndex = -1; // property index in current metaobject

    qsizetype location = -1; // token index, used for error reporting

    QJsonObject toJson() const;
};
Q_DECLARE_TYPEINFO(PropertyDef, Q_RELOCATABLE_TYPE);

struct PrivateQPropertyDef
{
    Type type;
    QByteArray name;
    QByteArray setter;
    QByteArray accessor;
    QByteArray storage;
};
Q_DECLARE_TYPEINFO(PrivateQPropertyDef, Q_RELOCATABLE_TYPE);

struct ClassInfoDef
{
    QByteArray name;
    QByteArray value;
};
Q_DECLARE_TYPEINFO(ClassInfoDef, Q_RELOCATABLE_TYPE);

struct BaseDef {
    QByteArray classname;
    QByteArray qualified;
    QList<ClassInfoDef> classInfoList;
    QMap<QByteArray, bool> enumDeclarations;
    QList<EnumDef> enumList;
    QMap<QByteArray, QByteArray> flagAliases;
    qsizetype begin = 0;
    qsizetype end = 0;
};

struct ClassDef : BaseDef {
    QList<QPair<QByteArray, FunctionDef::Access>> superclassList;

    struct Interface
    {
        Interface() { } // for QList, don't use
        inline explicit Interface(const QByteArray &_className)
            : className(_className) {}
        QByteArray className;
        QByteArray interfaceId;
    };
    QList<QList<Interface>> interfaceList;

    struct PluginData {
        QByteArray iid;
        QByteArray uri;
        QMap<QString, QJsonArray> metaArgs;
        QJsonDocument metaData;
    } pluginData;

    QList<FunctionDef> constructorList;
    QList<FunctionDef> signalList, slotList, methodList, publicList;
    QList<QByteArray> nonClassSignalList;
    QList<PropertyDef> propertyList;
    int revisionedMethods = 0;

    bool hasQObject = false;
    bool hasQGadget = false;
    bool hasQNamespace = false;
    bool requireCompleteMethodTypes = false;

    QJsonObject toJson() const;
};
Q_DECLARE_TYPEINFO(ClassDef, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(ClassDef::Interface, Q_RELOCATABLE_TYPE);

struct NamespaceDef : BaseDef {
    bool hasQNamespace = false;
    bool doGenerate = false;
};
Q_DECLARE_TYPEINFO(NamespaceDef, Q_RELOCATABLE_TYPE);

class Moc : public Parser
{
public:
    enum PropertyMode { Named, Anonymous };

    Moc()
        : noInclude(false), mustIncludeQPluginH(false), requireCompleteTypes(false)
        {}

    QByteArray filename;

    bool noInclude;
    bool mustIncludeQPluginH;
    bool requireCompleteTypes;
    QByteArray includePath;
    QList<QByteArray> includeFiles;
    QList<ClassDef> classList;
    QMap<QByteArray, QByteArray> interface2IdMap;
    QList<QByteArray> metaTypes;
    // map from class name to fully qualified name
    QHash<QByteArray, QByteArray> knownQObjectClasses;
    QHash<QByteArray, QByteArray> knownGadgets;
    QMap<QString, QJsonArray> metaArgs;
    QList<QString> parsedPluginMetadataFiles;

    void parse();
    void generate(FILE *out, FILE *jsonOutput);

    bool parseClassHead(ClassDef *def);
    inline bool inClass(const ClassDef *def) const {
        return index > def->begin && index < def->end - 1;
    }

    inline bool inNamespace(const NamespaceDef *def) const {
        return index > def->begin && index < def->end - 1;
    }

    void prependNamespaces(BaseDef &def, const QList<NamespaceDef> &namespaceList) const;

    Type parseType();

    bool parseEnum(EnumDef *def);

    bool parseFunction(FunctionDef *def, bool inMacro = false);
    bool parseMaybeFunction(const ClassDef *cdef, FunctionDef *def);

    void parseSlots(ClassDef *def, FunctionDef::Access access);
    void parseSignals(ClassDef *def);
    void parseProperty(ClassDef *def, PropertyMode mode);
    void parsePluginData(ClassDef *def);

    void createPropertyDef(PropertyDef &def, int propertyIndex, PropertyMode mode);

    void parsePropertyAttributes(PropertyDef &propDef);
    void parseEnumOrFlag(BaseDef *def, bool isFlag);
    void parseFlag(BaseDef *def);
    enum class EncounteredQmlMacro {Yes, No};
    EncounteredQmlMacro parseClassInfo(BaseDef *def);
    void parseClassInfo(ClassDef *def);
    void parseInterfaces(ClassDef *def);
    void parseDeclareInterface();
    void parseDeclareMetatype();
    void parseMocInclude();
    void parseSlotInPrivate(ClassDef *def, FunctionDef::Access access);
    QByteArray parsePropertyAccessor();
    void parsePrivateProperty(ClassDef *def, PropertyMode mode);

    void parseFunctionArguments(FunctionDef *def);

    QByteArray lexemUntil(Token);
    bool until(Token);

    // test for Q_INVOCABLE, Q_SCRIPTABLE, etc. and set the flags
    // in FunctionDef accordingly
    bool testFunctionAttribute(FunctionDef *def);
    bool testFunctionAttribute(Token tok, FunctionDef *def);
    bool testFunctionRevision(FunctionDef *def);
    QTypeRevision parseRevision();

    bool skipCxxAttributes();

    void checkSuperClasses(ClassDef *def);
    void checkProperties(ClassDef* cdef);
    bool testForFunctionModifiers(FunctionDef *def);
};

inline QByteArray noRef(const QByteArray &type)
{
    if (type.endsWith('&')) {
        if (type.endsWith("&&"))
            return type.left(type.size()-2);
        return type.left(type.size()-1);
    }
    return type;
}

QT_END_NAMESPACE

#endif // MOC_H
