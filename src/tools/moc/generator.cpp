// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Olivier Goffart <ogoffart@woboq.com>
// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "generator.h"
#include "cbordevice.h"
#include "outputrevision.h"
#include "utils.h"
#include <QtCore/qmetatype.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qplugin.h>
#include <QtCore/qstringview.h>
#include <QtCore/qtmocconstants.h>

#include <math.h>
#include <stdio.h>

#include <private/qmetaobject_p.h> //for the flags.
#include <private/qplugin_p.h> //for the flags.

QT_BEGIN_NAMESPACE

using namespace QtMiscUtils;

static int nameToBuiltinType(const QByteArray &name)
{
    if (name.isEmpty())
        return 0;

    uint tp = QMetaType::UnknownType;
    if (const QtPrivate::QMetaTypeInterface *iface = QMetaType::fromName(name).iface())
        tp = iface->typeId.loadRelaxed(); // always registered

#ifndef QT_BOOTSTRAPPED
    if (tp >= uint(QMetaType::User))
        tp = QMetaType::UnknownType;
#endif

    return int(tp);
}

/*
  Returns \c true if the type is a built-in type.
*/
static bool isBuiltinType(const QByteArray &type)
{
    int id = nameToBuiltinType(type);
    return id != QMetaType::UnknownType;
}

constexpr const char *cxxTypeTag(TypeTags t)
{
    if (t & TypeTag::HasEnum) {
        if (t & TypeTag::HasClass)
            return "enum class ";
        if (t & TypeTag::HasStruct)
            return "enum struct ";
        return "enum ";
    }
    if (t & TypeTag::HasClass) return "class ";
    if (t & TypeTag::HasStruct) return "struct ";
    return "";
}

static const char *metaTypeEnumValueString(int type)
 {
#define RETURN_METATYPENAME_STRING(MetaTypeName, MetaTypeId, RealType) \
    case QMetaType::MetaTypeName: return #MetaTypeName;

    switch (type) {
QT_FOR_EACH_STATIC_TYPE(RETURN_METATYPENAME_STRING)
    }
#undef RETURN_METATYPENAME_STRING
    return nullptr;
 }

 Generator::Generator(Moc *moc, ClassDef *classDef, const QList<QByteArray> &metaTypes,
                      const QHash<QByteArray, QByteArray> &knownQObjectClasses,
                      const QHash<QByteArray, QByteArray> &knownGadgets, FILE *outfile,
                      bool requireCompleteTypes)
     : parser(moc),
       out(outfile),
       cdef(classDef),
       metaTypes(metaTypes),
       knownQObjectClasses(knownQObjectClasses),
       knownGadgets(knownGadgets),
       requireCompleteTypes(requireCompleteTypes)
 {
     if (cdef->superclassList.size())
         purestSuperClass = cdef->superclassList.constFirst().classname;
}

static inline qsizetype lengthOfEscapeSequence(const QByteArray &s, qsizetype i)
{
    if (s.at(i) != '\\' || i >= s.size() - 1)
        return 1;
    const qsizetype startPos = i;
    ++i;
    char ch = s.at(i);
    if (ch == 'x') {
        ++i;
        while (i < s.size() && isHexDigit(s.at(i)))
            ++i;
    } else if (isOctalDigit(ch)) {
        while (i < startPos + 4
               && i < s.size()
               && isOctalDigit(s.at(i))) {
            ++i;
        }
    } else { // single character escape sequence
        i = qMin(i + 1, s.size());
    }
    return i - startPos;
}

// Prints \a s to \a out, breaking it into lines of at most ColumnWidth. The
// opening and closing quotes are NOT included (it's up to the caller).
static void printStringWithIndentation(FILE *out, const QByteArray &s)
{
    static constexpr int ColumnWidth = 68;
    const qsizetype len = s.size();
    qsizetype idx = 0;

    do {
        qsizetype spanLen = qMin(ColumnWidth - 2, len - idx);
        // don't cut escape sequences at the end of a line
        const qsizetype backSlashPos = s.lastIndexOf('\\', idx + spanLen - 1);
        if (backSlashPos >= idx) {
            const qsizetype escapeLen = lengthOfEscapeSequence(s, backSlashPos);
            spanLen = qBound(spanLen, backSlashPos + escapeLen - idx, len - idx);
        }
        fprintf(out, "\n        \"%.*s\"", int(spanLen), s.constData() + idx);
        idx += spanLen;
    } while (idx < len);
}

void Generator::strreg(const QByteArray &s)
{
    if (!strings.contains(s))
        strings.append(s);
}

int Generator::stridx(const QByteArray &s)
{
    int i = int(strings.indexOf(s));
    Q_ASSERT_X(i != -1, Q_FUNC_INFO, "We forgot to register some strings");
    return i;
}

bool Generator::registerableMetaType(const QByteArray &propertyType)
{
    if (metaTypes.contains(propertyType))
        return true;

    if (propertyType.endsWith('*')) {
        QByteArray objectPointerType = propertyType;
        // The objects container stores class names, such as 'QState', 'QLabel' etc,
        // not 'QState*', 'QLabel*'. The propertyType does contain the '*', so we need
        // to chop it to find the class type in the known QObjects list.
        objectPointerType.chop(1);
        if (knownQObjectClasses.contains(objectPointerType))
            return true;
    }

    static const QList<QByteArray> smartPointers = QList<QByteArray>()
#define STREAM_SMART_POINTER(SMART_POINTER) << #SMART_POINTER
            QT_FOR_EACH_AUTOMATIC_TEMPLATE_SMART_POINTER(STREAM_SMART_POINTER)
#undef STREAM_SMART_POINTER
            ;

    for (const QByteArray &smartPointer : smartPointers) {
        QByteArray ba = smartPointer + "<";
        if (propertyType.startsWith(ba) && !propertyType.endsWith("&"))
            return knownQObjectClasses.contains(propertyType.mid(smartPointer.size() + 1, propertyType.size() - smartPointer.size() - 1 - 1));
    }

    static const QList<QByteArray> oneArgTemplates = QList<QByteArray>()
#define STREAM_1ARG_TEMPLATE(TEMPLATENAME) << #TEMPLATENAME
            QT_FOR_EACH_AUTOMATIC_TEMPLATE_1ARG(STREAM_1ARG_TEMPLATE)
#undef STREAM_1ARG_TEMPLATE
            ;
    for (const QByteArray &oneArgTemplateType : oneArgTemplates) {
        const QByteArray ba = oneArgTemplateType + "<";
        if (propertyType.startsWith(ba) && propertyType.endsWith(">")) {
            const qsizetype argumentSize = propertyType.size() - ba.size()
                                     // The closing '>'
                                     - 1
                                     // templates inside templates have an extra whitespace char to strip.
                                     - (propertyType.at(propertyType.size() - 2) == ' ' ? 1 : 0 );
            const QByteArray templateArg = propertyType.sliced(ba.size(), argumentSize);
            return isBuiltinType(templateArg) || registerableMetaType(templateArg);
        }
    }
    return false;
}

/* returns \c true if name and qualifiedName refers to the same name.
 * If qualified name is "A::B::C", it returns \c true for "C", "B::C" or "A::B::C" */
static bool qualifiedNameEquals(const QByteArray &qualifiedName, const QByteArray &name)
{
    if (qualifiedName == name)
        return true;
    const qsizetype index = qualifiedName.indexOf("::");
    if (index == -1)
        return false;
    return qualifiedNameEquals(qualifiedName.mid(index+2), name);
}

static QByteArray generateQualifiedClassNameIdentifier(const QByteArray &identifier)
{
    // This is similar to the IA-64 C++ ABI mangling scheme.
    QByteArray qualifiedClassNameIdentifier = "ZN";
    for (const auto scope : qTokenize(QLatin1StringView(identifier), QLatin1Char(':'),
                                      Qt::SkipEmptyParts)) {
        qualifiedClassNameIdentifier += QByteArray::number(scope.size());
        qualifiedClassNameIdentifier += scope;
    }
    qualifiedClassNameIdentifier += 'E';
    return qualifiedClassNameIdentifier;
}

void Generator::generateCode()
{
    bool isQObject = (cdef->classname == "QObject");
    bool isConstructible = !cdef->constructorList.isEmpty();

    // filter out undeclared enumerators and sets
    {
        QList<EnumDef> enumList;
        for (EnumDef def : std::as_const(cdef->enumList)) {
            if (cdef->enumDeclarations.contains(def.name)) {
                enumList += def;
            }
            def.enumName = def.name;
            QByteArray alias = cdef->flagAliases.value(def.name);
            if (cdef->enumDeclarations.contains(alias)) {
                def.name = alias;
                def.flags |= cdef->enumDeclarations[alias];
                enumList += def;
            }
        }
        cdef->enumList = enumList;
    }

//
// Register all strings used in data section
//
    strreg(cdef->qualified);
    registerClassInfoStrings();
    registerFunctionStrings(cdef->signalList);
    registerFunctionStrings(cdef->slotList);
    registerFunctionStrings(cdef->methodList);
    registerFunctionStrings(cdef->constructorList);
    registerByteArrayVector(cdef->nonClassSignalList);
    registerPropertyStrings();
    registerEnumStrings();

    const bool requireCompleteness = requireCompleteTypes || cdef->requireCompleteMethodTypes;
    bool hasStaticMetaCall =
            (cdef->hasQObject || !cdef->methodList.isEmpty()
             || !cdef->propertyList.isEmpty() || !cdef->constructorList.isEmpty());
    if (parser->activeQtMode)
        hasStaticMetaCall = false;

    const QByteArray qualifiedClassNameIdentifier = generateQualifiedClassNameIdentifier(cdef->qualified);

    // type name for the Q_OJBECT/GADGET itself, void for namespaces
    const char *ownType = !cdef->hasQNamespace ? cdef->classname.data() : "void";

    // ensure the qt_meta_tag_XXXX_t type is local
    fprintf(out, "namespace {\n"
                 "struct qt_meta_tag_%s_t {};\n"
                 "} // unnamed namespace\n\n",
            qualifiedClassNameIdentifier.constData());

//
// build the strings, data, and metatype arrays
//

    // We define a method inside the context of the class or namespace we're
    // creating the meta object for, so we get access to everything it has
    // access to and with the same contexts (for example, member enums and
    // types).
    fprintf(out, "template <> constexpr inline auto %s::qt_create_metaobjectdata<qt_meta_tag_%s_t>()\n"
                 "{\n"
                 "    namespace QMC = QtMocConstants;\n",
            cdef->qualified.constData(), qualifiedClassNameIdentifier.constData());

    fprintf(out, "    QtMocHelpers::StringRefStorage qt_stringData {");
    addStrings(strings);
    fprintf(out, "\n    };\n\n");

    fprintf(out, "    QtMocHelpers::UintData qt_methods {\n");

    // Build signals array first, otherwise the signal indices would be wrong
    addFunctions(cdef->signalList, "Signal");
    addFunctions(cdef->slotList, "Slot");
    addFunctions(cdef->methodList, "Method");
    fprintf(out, "    };\n"
                 "    QtMocHelpers::UintData qt_properties {\n");
    addProperties();
    fprintf(out, "    };\n"
                 "    QtMocHelpers::UintData qt_enums {\n");
    addEnums();
    fprintf(out, "    };\n");

    const char *uintDataParams = "";
    if (isConstructible || !cdef->classInfoList.isEmpty()) {
        if (isConstructible) {
            fprintf(out, "    using Constructor = QtMocHelpers::NoType;\n"
                         "    QtMocHelpers::UintData qt_constructors {\n");
            addFunctions(cdef->constructorList, "Constructor");
            fprintf(out, "    };\n");
        } else {
            fputs("    QtMocHelpers::UintData qt_constructors {};\n", out);
        }

        uintDataParams = ", qt_constructors";
        if (!cdef->classInfoList.isEmpty()) {
            fprintf(out, "    QtMocHelpers::ClassInfos qt_classinfo({\n");
            addClassInfos();
            fprintf(out, "    });\n");
            uintDataParams = ", qt_constructors, qt_classinfo";
        }
    }

    const char *metaObjectFlags = "QMC::MetaObjectFlag{}";
    if (cdef->hasQGadget || cdef->hasQNamespace) {
        // Ideally, all the classes could have that flag. But this broke
        // classes generated by qdbusxml2cpp which generate code that require
        // that we call qt_metacall for properties.
        metaObjectFlags = "QMC::PropertyAccessInStaticMetaCall";
    }
    {
        QByteArray tagType = QByteArrayLiteral("void");
        if (!requireCompleteness)
            tagType = "qt_meta_tag_" + qualifiedClassNameIdentifier +  "_t";
        fprintf(out, "    return QtMocHelpers::metaObjectData<%s, %s>(%s, qt_stringData,\n"
                     "            qt_methods, qt_properties, qt_enums%s);\n"
                     "}\n",
                ownType, tagType.constData(), metaObjectFlags, uintDataParams);
    }

    QByteArray metaVarNameSuffix;
    if (cdef->hasQNamespace) {
        // Q_NAMESPACE does not define the variables, so we have to. Declare as
        // plain, file-scope static variables (not templates).
        metaVarNameSuffix = '_' + qualifiedClassNameIdentifier;
        const char *n = metaVarNameSuffix.constData();
        fprintf(out, R"(
static constexpr auto qt_staticMetaObjectContent%s =
    %s::qt_create_metaobjectdata<qt_meta_tag%s_t>();
static constexpr auto qt_staticMetaObjectStaticContent%s =
    qt_staticMetaObjectContent%s.staticData;
static constexpr auto qt_staticMetaObjectRelocatingContent%s =
    qt_staticMetaObjectContent%s.relocatingData;

)",
                n, cdef->qualified.constData(), n,
                n, n,
                n, n);
    } else {
        // Q_OBJECT and Q_GADGET do declare them, so we just use the templates.
        metaVarNameSuffix = "<qt_meta_tag_" + qualifiedClassNameIdentifier + "_t>";
    }

//
// Build extra array
//
    QList<QByteArray> extraList;
    QMultiHash<QByteArray, QByteArray> knownExtraMetaObject(knownGadgets);
    knownExtraMetaObject.unite(knownQObjectClasses);

    for (const PropertyDef &p : std::as_const(cdef->propertyList)) {
        if (isBuiltinType(p.type))
            continue;

        if (p.type.contains('*') || p.type.contains('<') || p.type.contains('>'))
            continue;

        const qsizetype s = p.type.lastIndexOf("::");
        if (s <= 0)
            continue;

        QByteArray unqualifiedScope = p.type.left(s);

        // The scope may be a namespace for example, so it's only safe to include scopes that are known QObjects (QTBUG-2151)
        QMultiHash<QByteArray, QByteArray>::ConstIterator scopeIt;

        QByteArray thisScope = cdef->qualified;
        do {
            const qsizetype s = thisScope.lastIndexOf("::");
            thisScope = thisScope.left(s);
            QByteArray currentScope = thisScope.isEmpty() ? unqualifiedScope : thisScope + "::" + unqualifiedScope;
            scopeIt = knownExtraMetaObject.constFind(currentScope);
        } while (!thisScope.isEmpty() && scopeIt == knownExtraMetaObject.constEnd());

        if (scopeIt == knownExtraMetaObject.constEnd())
            continue;

        const QByteArray &scope = *scopeIt;

        if (scope == "Qt")
            continue;
        if (qualifiedNameEquals(cdef->qualified, scope))
            continue;

        if (!extraList.contains(scope))
            extraList += scope;
    }

    // QTBUG-20639 - Accept non-local enums for QML signal/slot parameters.
    // Look for any scoped enum declarations, and add those to the list
    // of extra/related metaobjects for this object.
    for (auto it = cdef->enumDeclarations.keyBegin(),
         end = cdef->enumDeclarations.keyEnd(); it != end; ++it) {
        const QByteArray &enumKey = *it;
        const qsizetype s = enumKey.lastIndexOf("::");
        if (s > 0) {
            QByteArray scope = enumKey.left(s);
            if (scope != "Qt" && !qualifiedNameEquals(cdef->qualified, scope) && !extraList.contains(scope))
                extraList += scope;
        }
    }

//
// Generate meta object link to parent meta objects
//

    if (!extraList.isEmpty()) {
        fprintf(out, "Q_CONSTINIT static const QMetaObject::SuperData qt_meta_extradata_%s[] = {\n",
                qualifiedClassNameIdentifier.constData());
        for (const QByteArray &ba : std::as_const(extraList))
            fprintf(out, "    QMetaObject::SuperData::link<%s::staticMetaObject>(),\n", ba.constData());

        fprintf(out, "    nullptr\n};\n\n");
    }

//
// Finally create and initialize the static meta object
//
    fprintf(out, "Q_CONSTINIT const QMetaObject %s::staticMetaObject = { {\n",
            cdef->qualified.constData());

    if (isQObject)
        fprintf(out, "    nullptr,\n");
    else if (cdef->superclassList.size() && !cdef->hasQGadget && !cdef->hasQNamespace) // for qobject, we know the super class must have a static metaobject
        fprintf(out, "    QMetaObject::SuperData::link<%s::staticMetaObject>(),\n", purestSuperClass.constData());
    else if (cdef->superclassList.size()) // for gadgets we need to query at compile time for it
        fprintf(out, "    QtPrivate::MetaObjectForType<%s>::value,\n", purestSuperClass.constData());
    else
        fprintf(out, "    nullptr,\n");
    fprintf(out, "    qt_staticMetaObjectStaticContent%s.stringdata,\n"
            "    qt_staticMetaObjectStaticContent%s.data,\n",
            metaVarNameSuffix.constData(),
            metaVarNameSuffix.constData());
    if (hasStaticMetaCall)
        fprintf(out, "    qt_static_metacall,\n");
    else
        fprintf(out, "    nullptr,\n");

    if (extraList.isEmpty())
        fprintf(out, "    nullptr,\n");
    else
        fprintf(out, "    qt_meta_extradata_%s,\n", qualifiedClassNameIdentifier.constData());

    fprintf(out, "    qt_staticMetaObjectRelocatingContent%s.metaTypes,\n",
            metaVarNameSuffix.constData());

    fprintf(out, "    nullptr\n} };\n\n");

//
// Generate internal qt_static_metacall() function
//
    if (hasStaticMetaCall)
        generateStaticMetacall();

    if (!cdef->hasQObject)
        return;

    fprintf(out, "\nconst QMetaObject *%s::metaObject() const\n{\n"
                 "    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;\n"
                 "}\n",
            cdef->qualified.constData());

//
// Generate smart cast function
//
    fprintf(out, "\nvoid *%s::qt_metacast(const char *_clname)\n{\n", cdef->qualified.constData());
    fprintf(out, "    if (!_clname) return nullptr;\n");
    fprintf(out, "    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_%s_t>.strings))\n"
                  "        return static_cast<void*>(this);\n",
            qualifiedClassNameIdentifier.constData());

    // for all superclasses but the first one
    if (cdef->superclassList.size() > 1) {
        auto it = cdef->superclassList.cbegin() + 1;
        const auto end = cdef->superclassList.cend();
        for (; it != end; ++it) {
            if (it->access == FunctionDef::Private)
                continue;
            const char *cname = it->classname.constData();
            fprintf(out, "    if (!strcmp(_clname, \"%s\"))\n        return static_cast< %s*>(this);\n",
                    cname, cname);
        }
    }

    for (const QList<ClassDef::Interface> &iface : std::as_const(cdef->interfaceList)) {
        for (qsizetype j = 0; j < iface.size(); ++j) {
            fprintf(out, "    if (!strcmp(_clname, %s))\n        return ", iface.at(j).interfaceId.constData());
            for (qsizetype k = j; k >= 0; --k)
                fprintf(out, "static_cast< %s*>(", iface.at(k).className.constData());
            fprintf(out, "this%s;\n", QByteArray(j + 1, ')').constData());
        }
    }
    if (!purestSuperClass.isEmpty() && !isQObject) {
        QByteArray superClass = purestSuperClass;
        fprintf(out, "    return %s::qt_metacast(_clname);\n", superClass.constData());
    } else {
        fprintf(out, "    return nullptr;\n");
    }
    fprintf(out, "}\n");

    if (parser->activeQtMode)
        return;

//
// Generate internal qt_metacall()  function
//
    generateMetacall();

//
// Generate internal signal functions
//
    for (int signalindex = 0; signalindex < int(cdef->signalList.size()); ++signalindex)
        generateSignal(&cdef->signalList.at(signalindex), signalindex);

//
// Generate plugin meta data
//
    generatePluginMetaData();

//
// Generate function to make sure the non-class signals exist in the parent classes
//
    if (!cdef->nonClassSignalList.isEmpty()) {
        fprintf(out, "namespace CheckNotifySignalValidity_%s {\n", qualifiedClassNameIdentifier.constData());
        for (const QByteArray &nonClassSignal : std::as_const(cdef->nonClassSignalList)) {
            const auto propertyIt = std::find_if(cdef->propertyList.constBegin(),
                                   cdef->propertyList.constEnd(),
                                   [&nonClassSignal](const PropertyDef &p) {
                return nonClassSignal == p.notify;
            });
            // must find something, otherwise checkProperties wouldn't have inserted an entry into nonClassSignalList
            Q_ASSERT(propertyIt != cdef->propertyList.constEnd());
            fprintf(out, "template<typename T> using has_nullary_%s = decltype(std::declval<T>().%s());\n",
                    nonClassSignal.constData(),
                    nonClassSignal.constData());
            const auto &propertyType = propertyIt->type;
            fprintf(out, "template<typename T> using has_unary_%s = decltype(std::declval<T>().%s(std::declval<%s>()));\n",
                    nonClassSignal.constData(),
                    nonClassSignal.constData(),
                    propertyType.constData());
            fprintf(out, "static_assert(qxp::is_detected_v<has_nullary_%s, %s> || qxp::is_detected_v<has_unary_%s, %s>,\n"
                         "              \"NOTIFY signal %s does not exist in class (or is private in its parent)\");\n",
                    nonClassSignal.constData(), cdef->qualified.constData(),
                    nonClassSignal.constData(), cdef->qualified.constData(),
                    nonClassSignal.constData());
        }
        fprintf(out, "}\n");
    }
}


void Generator::registerClassInfoStrings()
{
    for (const ClassInfoDef &c : std::as_const(cdef->classInfoList)) {
        strreg(c.name);
        strreg(c.value);
    }
}

void Generator::addClassInfos()
{
    for (const ClassInfoDef &c : std::as_const(cdef->classInfoList))
        fprintf(out, "            { %4d, %4d },\n", stridx(c.name), stridx(c.value));
}

void Generator::registerFunctionStrings(const QList<FunctionDef> &list)
{
    for (const FunctionDef &f : list) {
        strreg(f.name);
        if (!isBuiltinType(f.normalizedType))
            strreg(f.normalizedType);
        strreg(f.tag);

        for (const ArgumentDef &a : f.arguments) {
            if (!isBuiltinType(a.normalizedType))
                strreg(a.normalizedType);
            strreg(a.name);
        }
    }
}

void Generator::registerByteArrayVector(const QList<QByteArray> &list)
{
    for (const QByteArray &ba : list)
        strreg(ba);
}

void Generator::addStrings(const QByteArrayList &strings)
{
    char comma = 0;
    for (const QByteArray &str : strings) {
        if (comma)
            fputc(comma, out);
        printStringWithIndentation(out, str);
        comma = ',';
    }
}

void Generator::addFunctions(const QList<FunctionDef> &list, const char *functype)
{
    for (const FunctionDef &f : list) {
        if (!f.isConstructor)
            fprintf(out, "        // %s '%s'\n", functype, f.name.constData());
        fprintf(out, "        QtMocHelpers::%s%sData<",
                f.revision > 0 ? "Revisioned" : "", functype);

        if (f.isConstructor)
            fprintf(out, "Constructor(");
        else
            fprintf(out, "%s(", disambiguatedTypeName(f.type.name).constData());   // return type

        const char *comma = "";
        for (const auto &argument : f.arguments) {
            fprintf(out, "%s%s", comma, disambiguatedTypeName(argument.type.name).constData());
            comma = ", ";
        }

        if (f.isConstructor)
            fprintf(out, ")>(%d, ", stridx(f.tag));
        else
            fprintf(out, ")%s>(%d, %d, ", f.isConst ? " const" : "", stridx(f.name), stridx(f.tag));

        // flags
        // access right is always present
        if (f.access == FunctionDef::Private)
            fprintf(out, "QMC::AccessPrivate");
        else if (f.access == FunctionDef::Public)
            fprintf(out, "QMC::AccessPublic");
        else if (f.access == FunctionDef::Protected)
            fprintf(out, "QMC::AccessProtected");
        if (f.isCompat)
            fprintf(out, " | QMC::MethodCompatibility");
        if (f.wasCloned)
            fprintf(out, " | QMC::MethodCloned");
        if (f.isScriptable)
            fprintf(out, " | QMC::MethodScriptable");

        // QtMocConstants::MethodRevisioned is implied by the call we're making
        if (f.revision > 0)
            fprintf(out, ", %#x", f.revision);

        // return type (if not a constructor)
        if (!f.isConstructor) {
            fprintf(out, ", ");
            generateTypeInfo(f.normalizedType);
        }

        if (f.arguments.isEmpty()) {
            fprintf(out, "),\n");
        } else {
            // array of parameter types (or type names) and names
            fprintf(out, ", {{");
            for (qsizetype i = 0; i < f.arguments.size(); ++i) {
                if ((i % 4) == 0)
                    fprintf(out, "\n           ");
                const ArgumentDef &arg = f.arguments.at(i);
                fprintf(out, " { ");
                generateTypeInfo(arg.normalizedType);
                fprintf(out, ", %d },", stridx(arg.name));
            }

            fprintf(out, "\n        }}),\n");
        }
    }
}


void Generator::generateTypeInfo(const QByteArray &typeName, bool allowEmptyName)
{
    Q_UNUSED(allowEmptyName);
    if (isBuiltinType(typeName)) {
        int type;
        const char *valueString;
        if (typeName == "qreal") {
            type = QMetaType::UnknownType;
            valueString = "QReal";
        } else {
            type = nameToBuiltinType(typeName);
            valueString = metaTypeEnumValueString(type);
        }
        if (valueString) {
            fprintf(out, "QMetaType::%s", valueString);
        } else {
            Q_ASSERT(type != QMetaType::UnknownType);
            fprintf(out, "%4d", type);
        }
    } else {
        Q_ASSERT(!typeName.isEmpty() || allowEmptyName);
        fprintf(out, "0x%.8x | %d", IsUnresolvedType, stridx(typeName));
    }
}

void Generator::registerPropertyStrings()
{
    for (const PropertyDef &p : std::as_const(cdef->propertyList)) {
        strreg(p.name);
        if (!isBuiltinType(p.type))
            strreg(p.type);
    }
}

void Generator::addProperties()
{
    for (const PropertyDef &p : std::as_const(cdef->propertyList)) {
        fprintf(out, "        // property '%s'\n"
                     "        QtMocHelpers::PropertyData<%s%s>(%d, ",
                p.name.constData(), cxxTypeTag(p.typeTag),
                disambiguatedTypeName(p.type, p.typeTag).constData(),
                stridx(p.name));
        generateTypeInfo(p.type);
        fputc(',', out);

        const char *separator = "";
        auto addFlag = [this, &separator](const char *text) {
            fprintf(out, "%s QMC::%s", separator, text);
            separator = " |";
        };
        bool readable = !p.read.isEmpty() || !p.member.isEmpty();
        bool designable = p.designable != "false";
        bool scriptable = p.scriptable != "false";
        bool stored = p.stored != "false";
        if (readable && designable && scriptable && stored) {
            addFlag("DefaultPropertyFlags");
            if ((!p.member.isEmpty() && !p.constant) || !p.write.isEmpty())
                addFlag("Writable");
        } else {
            if (readable)
                addFlag("Readable");
            if ((!p.member.isEmpty() && !p.constant) || !p.write.isEmpty())
                addFlag("Writable");
            if (designable)
                addFlag("Designable");
            if (scriptable)
                addFlag("Scriptable");
            if (stored)
                addFlag("Stored");
        }
        if (!p.reset.isEmpty())
            addFlag("Resettable");
        if (!isBuiltinType(p.type))
            addFlag("EnumOrFlag");
        if (p.stdCppSet())
            addFlag("StdCppSet");
        if (p.constant)
            addFlag("Constant");
        if (p.final)
            addFlag("Final");
        if (p.user != "false")
            addFlag("User");
        if (p.required)
            addFlag("Required");
        if (!p.bind.isEmpty())
            addFlag("Bindable");

        if (*separator == '\0')
            addFlag("Invalid");

        int notifyId = p.notifyId;
        if (notifyId != -1 || p.revision > 0) {
            fprintf(out, ", ");
            if (p.notifyId < -1) {
                // signal is in parent class
                const int indexInStrings = int(strings.indexOf(p.notify));
                notifyId = indexInStrings;
                fprintf(out, "%#x | ", IsUnresolvedSignal);
            }
            fprintf(out, "%d", notifyId);
            if (p.revision > 0)
                fprintf(out, ", %#x", p.revision);
        }

        fprintf(out, "),\n");
    }
}

void Generator::registerEnumStrings()
{
    for (const EnumDef &e : std::as_const(cdef->enumList)) {
        strreg(e.name);
        if (!e.enumName.isNull())
            strreg(e.enumName);
        for (const QByteArray &val : e.values)
            strreg(val);
    }
}

void Generator::addEnums()
{
    for (const EnumDef &e : std::as_const(cdef->enumList)) {
        const QByteArray &typeName = e.enumName.isNull() ? e.name : e.enumName;
        fprintf(out, "        // %s '%s'\n"
                     "        QtMocHelpers::EnumData<%s>(%d, %d,",
                e.flags & EnumIsFlag ? "flag" : "enum", e.name.constData(),
                disambiguatedTypeName(e.name).constData(), stridx(e.name), stridx(typeName));

        if (e.flags) {
            const char *separator = "";
            auto addFlag = [this, &separator](const char *text) {
                fprintf(out, "%s QMC::%s", separator, text);
                separator = " |";
            };
            if (e.flags & EnumIsFlag)
                addFlag("EnumIsFlag");
            if (e.flags & EnumIsScoped)
                addFlag("EnumIsScoped");
        } else {
            fprintf(out, " QMC::EnumFlags{}");
        }

        if (e.values.isEmpty()) {
            fprintf(out, "),\n");
            continue;
        }

        // add the enumerations
        fprintf(out, ").add({\n");
        QByteArray prefix = (e.enumName.isNull() ? e.name : e.enumName);
        for (const QByteArray &val : e.values) {
            fprintf(out, "            { %4d, %s::%s },\n", stridx(val),
                    prefix.constData(), val.constData());
        }

        fprintf(out, "        }),\n");
    }
}

void Generator::generateMetacall()
{
    bool isQObject = (cdef->classname == "QObject");

    fprintf(out, "\nint %s::qt_metacall(QMetaObject::Call _c, int _id, void **_a)\n{\n",
             cdef->qualified.constData());

    if (!purestSuperClass.isEmpty() && !isQObject) {
        QByteArray superClass = purestSuperClass;
        fprintf(out, "    _id = %s::qt_metacall(_c, _id, _a);\n", superClass.constData());
    }


    QList<FunctionDef> methodList;
    methodList += cdef->signalList;
    methodList += cdef->slotList;
    methodList += cdef->methodList;

    // If there are no methods or properties, we will return _id anyway, so
    // don't emit this comparison -- it is unnecessary, and it makes coverity
    // unhappy.
    if (methodList.size() || cdef->propertyList.size()) {
        fprintf(out, "    if (_id < 0)\n        return _id;\n");
    }

    if (methodList.size()) {
        fprintf(out, "    if (_c == QMetaObject::InvokeMetaMethod) {\n");
        fprintf(out, "        if (_id < %d)\n", int(methodList.size()));
        fprintf(out, "            qt_static_metacall(this, _c, _id, _a);\n");
        fprintf(out, "        _id -= %d;\n    }\n", int(methodList.size()));

        fprintf(out, "    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {\n");
        fprintf(out, "        if (_id < %d)\n", int(methodList.size()));

        if (methodsWithAutomaticTypesHelper(methodList).isEmpty())
            fprintf(out, "            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();\n");
        else
            fprintf(out, "            qt_static_metacall(this, _c, _id, _a);\n");
        fprintf(out, "        _id -= %d;\n    }\n", int(methodList.size()));

    }

    if (cdef->propertyList.size()) {
        fprintf(out,
            "    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty\n"
            "            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty\n"
            "            || _c == QMetaObject::RegisterPropertyMetaType) {\n"
            "        qt_static_metacall(this, _c, _id, _a);\n"
            "        _id -= %d;\n    }\n", int(cdef->propertyList.size()));
    }
    fprintf(out,"    return _id;\n}\n");
}


// ### Qt 7 (6.x?): remove
QMultiMap<QByteArray, int> Generator::automaticPropertyMetaTypesHelper()
{
    QMultiMap<QByteArray, int> automaticPropertyMetaTypes;
    for (int i = 0; i < int(cdef->propertyList.size()); ++i) {
        const PropertyDef &p = cdef->propertyList.at(i);
        const QByteArray &propertyType = p.type;
        if (registerableMetaType(propertyType) && !isBuiltinType(propertyType))
            automaticPropertyMetaTypes.insert(cxxTypeTag(p.typeTag) + propertyType, i);
    }
    return automaticPropertyMetaTypes;
}

QMap<int, QMultiMap<QByteArray, int>>
Generator::methodsWithAutomaticTypesHelper(const QList<FunctionDef> &methodList)
{
    QMap<int, QMultiMap<QByteArray, int> > methodsWithAutomaticTypes;
    for (int i = 0; i < methodList.size(); ++i) {
        const FunctionDef &f = methodList.at(i);
        for (int j = 0; j < f.arguments.size(); ++j) {
            const QByteArray &argType = f.arguments.at(j).normalizedType;
            if (registerableMetaType(argType) && !isBuiltinType(argType))
                methodsWithAutomaticTypes[i].insert(argType, j);
        }
    }
    return methodsWithAutomaticTypes;
}

void Generator::generateStaticMetacall()
{
    fprintf(out, "void %s::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)\n{\n",
            cdef->qualified.constData());

    enum UsedArgs {
        UsedT = 1,
        UsedC = 2,
        UsedId = 4,
        UsedA = 8,
    };
    uint usedArgs = 0;

    if (cdef->hasQObject) {
#ifndef QT_NO_DEBUG
        fprintf(out, "    Q_ASSERT(_o == nullptr || staticMetaObject.cast(_o));\n");
#endif
        fprintf(out, "    auto *_t = static_cast<%s *>(_o);\n", cdef->classname.constData());
    } else {
        fprintf(out, "    auto *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.constData());
    }

    const auto generateCtorArguments = [&](int ctorindex) {
        const FunctionDef &f = cdef->constructorList.at(ctorindex);
        Q_ASSERT(!f.isPrivateSignal); // That would be a strange ctor indeed
        int offset = 1;

        const auto begin = f.arguments.cbegin();
        const auto end = f.arguments.cend();
        for (auto it = begin; it != end; ++it) {
            const ArgumentDef &a = *it;
            if (it != begin)
                fprintf(out, ",");
            fprintf(out, "(*reinterpret_cast<%s>(_a[%d]))",
                    disambiguatedTypeNameForCast(a.normalizedType).constData(), offset++);
        }
    };

    if (!cdef->constructorList.isEmpty()) {
        fprintf(out, "    if (_c == QMetaObject::CreateInstance) {\n");
        fprintf(out, "        switch (_id) {\n");
        const int ctorend = int(cdef->constructorList.size());
        for (int ctorindex = 0; ctorindex < ctorend; ++ctorindex) {
            fprintf(out, "        case %d: { %s *_r = new %s(", ctorindex,
                    cdef->classname.constData(), cdef->classname.constData());
            generateCtorArguments(ctorindex);
            fprintf(out, ");\n");
            fprintf(out, "            if (_a[0]) *reinterpret_cast<%s**>(_a[0]) = _r; } break;\n",
                    (cdef->hasQGadget || cdef->hasQNamespace) ? "void" : "QObject");
        }
        fprintf(out, "        default: break;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }\n");
        fprintf(out, "    if (_c == QMetaObject::ConstructInPlace) {\n");
        fprintf(out, "        switch (_id) {\n");
        for (int ctorindex = 0; ctorindex < ctorend; ++ctorindex) {
            fprintf(out, "        case %d: { new (_a[0]) %s(",
                    ctorindex, cdef->classname.constData());
            generateCtorArguments(ctorindex);
            fprintf(out, "); } break;\n");
        }
        fprintf(out, "        default: break;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }\n");
        usedArgs |= UsedC | UsedId | UsedA;
    }

    QList<FunctionDef> methodList;
    methodList += cdef->signalList;
    methodList += cdef->slotList;
    methodList += cdef->methodList;

    if (!methodList.isEmpty()) {
        usedArgs |= UsedT | UsedC | UsedId;
        fprintf(out, "    if (_c == QMetaObject::InvokeMetaMethod) {\n");
        fprintf(out, "        switch (_id) {\n");
        for (int methodindex = 0; methodindex < methodList.size(); ++methodindex) {
            const FunctionDef &f = methodList.at(methodindex);
            Q_ASSERT(!f.normalizedType.isEmpty());
            fprintf(out, "        case %d: ", methodindex);
            if (f.normalizedType != "void")
                fprintf(out, "{ %s _r = ", disambiguatedTypeName(noRef(f.normalizedType)).constData());
            fprintf(out, "_t->");
            if (f.inPrivateClass.size())
                fprintf(out, "%s->", f.inPrivateClass.constData());
            fprintf(out, "%s(", f.name.constData());
            int offset = 1;

            if (f.isRawSlot) {
                fprintf(out, "QMethodRawArguments{ _a }");
                usedArgs |= UsedA;
            } else {
                const auto begin = f.arguments.cbegin();
                const auto end = f.arguments.cend();
                for (auto it = begin; it != end; ++it) {
                    const ArgumentDef &a = *it;
                    if (it != begin)
                        fprintf(out, ",");
                    fprintf(out, "(*reinterpret_cast<%s>(_a[%d]))", disambiguatedTypeNameForCast(a.normalizedType).constData(), offset++);
                    usedArgs |= UsedA;
                }
                if (f.isPrivateSignal) {
                    if (!f.arguments.isEmpty())
                        fprintf(out, ", ");
                    fprintf(out, "%s", "QPrivateSignal()");
                }
            }
            fprintf(out, ");");
            if (f.normalizedType != "void") {
                fprintf(out, "\n            if (_a[0]) *reinterpret_cast<%s*>(_a[0]) = std::move(_r); } ",
                        disambiguatedTypeName(noRef(f.normalizedType)).constData());
                usedArgs |= UsedA;
            }
            fprintf(out, " break;\n");
        }
        fprintf(out, "        default: ;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }\n");

        QMap<int, QMultiMap<QByteArray, int> > methodsWithAutomaticTypes = methodsWithAutomaticTypesHelper(methodList);

        if (!methodsWithAutomaticTypes.isEmpty()) {
            fprintf(out, "    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {\n");
            fprintf(out, "        switch (_id) {\n");
            fprintf(out, "        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;\n");
            QMap<int, QMultiMap<QByteArray, int> >::const_iterator it = methodsWithAutomaticTypes.constBegin();
            const QMap<int, QMultiMap<QByteArray, int> >::const_iterator end = methodsWithAutomaticTypes.constEnd();
            for ( ; it != end; ++it) {
                fprintf(out, "        case %d:\n", it.key());
                fprintf(out, "            switch (*reinterpret_cast<int*>(_a[1])) {\n");
                fprintf(out, "            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;\n");
                auto jt = it->begin();
                const auto jend = it->end();
                while (jt != jend) {
                    fprintf(out, "            case %d:\n", jt.value());
                    const QByteArray &lastKey = jt.key();
                    ++jt;
                    if (jt == jend || jt.key() != lastKey)
                        fprintf(out, "                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< %s >(); break;\n", lastKey.constData());
                }
                fprintf(out, "            }\n");
                fprintf(out, "            break;\n");
            }
            fprintf(out, "        }\n");
            fprintf(out, "    }\n");
            usedArgs |= UsedC | UsedId | UsedA;
        }

    }
    if (!cdef->signalList.isEmpty()) {
        usedArgs |= UsedC | UsedA;
        fprintf(out, "    if (_c == QMetaObject::IndexOfMethod) {\n");
        for (int methodindex = 0; methodindex < int(cdef->signalList.size()); ++methodindex) {
            const FunctionDef &f = cdef->signalList.at(methodindex);
            if (f.wasCloned || !f.inPrivateClass.isEmpty() || f.isStatic)
                continue;
            fprintf(out, "        if (QtMocHelpers::indexOfMethod<%s (%s::*)(",
                    f.type.rawName.constData() , cdef->classname.constData());

            const auto begin = f.arguments.cbegin();
            const auto end = f.arguments.cend();
            for (auto it = begin; it != end; ++it) {
                const ArgumentDef &a = *it;
                if (it != begin)
                    fprintf(out, ", ");
                fprintf(out, "%s", QByteArray(a.type.name + ' ' + a.rightType).constData());
            }
            if (f.isPrivateSignal) {
                if (!f.arguments.isEmpty())
                    fprintf(out, ", ");
                fprintf(out, "%s", "QPrivateSignal");
            }
            fprintf(out, ")%s>(_a, &%s::%s, %d))\n",
                    f.isConst ? " const" : "",
                    cdef->classname.constData(), f.name.constData(), methodindex);
            fprintf(out, "            return;\n");
        }
        fprintf(out, "    }\n");
    }

    const QMultiMap<QByteArray, int> automaticPropertyMetaTypes = automaticPropertyMetaTypesHelper();

    if (!automaticPropertyMetaTypes.isEmpty()) {
        fprintf(out, "    if (_c == QMetaObject::RegisterPropertyMetaType) {\n");
        fprintf(out, "        switch (_id) {\n");
        fprintf(out, "        default: *reinterpret_cast<int*>(_a[0]) = -1; break;\n");
        auto it = automaticPropertyMetaTypes.begin();
        const auto end = automaticPropertyMetaTypes.end();
        while (it != end) {
            fprintf(out, "        case %d:\n", it.value());
            const QByteArray &lastKey = it.key();
            ++it;
            if (it == end || it.key() != lastKey)
                fprintf(out, "            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< %s >(); break;\n", lastKey.constData());
        }
        fprintf(out, "        }\n");
        fprintf(out, "    }\n");
        usedArgs |= UsedC | UsedId | UsedA;
    }

    if (!cdef->propertyList.empty()) {
        bool needGet = false;
        bool needTempVarForGet = false;
        bool needSet = false;
        bool needReset = false;
        bool hasBindableProperties = false;
        for (const PropertyDef &p : std::as_const(cdef->propertyList)) {
            needGet |= !p.read.isEmpty() || !p.member.isEmpty();
            if (!p.read.isEmpty() || !p.member.isEmpty())
                needTempVarForGet |= (p.gspec != PropertyDef::PointerSpec
                                      && p.gspec != PropertyDef::ReferenceSpec);

            needSet |= !p.write.isEmpty() || (!p.member.isEmpty() && !p.constant);
            needReset |= !p.reset.isEmpty();
            hasBindableProperties |= !p.bind.isEmpty();
        }
        if (needGet || needSet || hasBindableProperties || needReset)
            usedArgs |= UsedT | UsedC | UsedId;
        if (needGet || needSet || hasBindableProperties)
            usedArgs |= UsedA;  // resetting doesn't need arguments

        if (needGet) {
            fprintf(out, "    if (_c == QMetaObject::ReadProperty) {\n");
            if (needTempVarForGet)
                fprintf(out, "        void *_v = _a[0];\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < int(cdef->propertyList.size()); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (p.read.isEmpty() && p.member.isEmpty())
                    continue;
                QByteArray prefix = "_t->";
                if (p.inPrivateClass.size()) {
                    prefix += p.inPrivateClass + "->";
                }

                if (p.gspec == PropertyDef::PointerSpec)
                    fprintf(out, "        case %d: _a[0] = const_cast<void*>(reinterpret_cast<const void*>(%s%s())); break;\n",
                            propindex, prefix.constData(), p.read.constData());
                else if (p.gspec == PropertyDef::ReferenceSpec)
                    fprintf(out, "        case %d: _a[0] = const_cast<void*>(reinterpret_cast<const void*>(&%s%s())); break;\n",
                            propindex, prefix.constData(), p.read.constData());
#if QT_VERSION <= QT_VERSION_CHECK(7, 0, 0)
                else if (auto eflags = cdef->enumDeclarations.value(p.type); eflags & EnumIsFlag)
                    fprintf(out, "        case %d: QtMocHelpers::assignFlags<%s>(_v, %s%s()); break;\n",
                            propindex, disambiguatedTypeName(p.type, p.typeTag).constData(), prefix.constData(), p.read.constData());
#endif
                else if (p.read == "default")
                    fprintf(out, "        case %d: *reinterpret_cast<%s%s*>(_v) = %s%s().value(); break;\n",
                            propindex, cxxTypeTag(p.typeTag), disambiguatedTypeName(p.type, p.typeTag).constData(),
                            prefix.constData(), p.bind.constData());
                else if (!p.read.isEmpty())
                    fprintf(out, "        case %d: *reinterpret_cast<%s%s*>(_v) = %s%s(); break;\n",
                            propindex, cxxTypeTag(p.typeTag), disambiguatedTypeName(p.type, p.typeTag).constData(),
                            prefix.constData(), p.read.constData());
                else
                    fprintf(out, "        case %d: *reinterpret_cast<%s%s*>(_v) = %s%s; break;\n",
                            propindex, cxxTypeTag(p.typeTag), disambiguatedTypeName(p.type, p.typeTag).constData(),
                            prefix.constData(), p.member.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
            fprintf(out, "    }\n");
        }

        if (needSet) {
            fprintf(out, "    if (_c == QMetaObject::WriteProperty) {\n");
            fprintf(out, "        void *_v = _a[0];\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < int(cdef->propertyList.size()); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (p.constant)
                    continue;
                if (p.write.isEmpty() && p.member.isEmpty())
                    continue;
                QByteArray prefix = "_t->";
                if (p.inPrivateClass.size()) {
                    prefix += p.inPrivateClass + "->";
                }
                if (p.write == "default") {
                    fprintf(out, "        case %d: {\n", propindex);
                    fprintf(out, "            %s%s().setValue(*reinterpret_cast<%s%s*>(_v));\n",
                            prefix.constData(), p.bind.constData(), cxxTypeTag(p.typeTag),
                            disambiguatedTypeName(p.type, p.typeTag).constData());
                    fprintf(out, "            break;\n");
                    fprintf(out, "        }\n");
                } else if (!p.write.isEmpty()) {
                    fprintf(out, "        case %d: %s%s(*reinterpret_cast<%s%s*>(_v)); break;\n",
                            propindex, prefix.constData(), p.write.constData(),
                            cxxTypeTag(p.typeTag), disambiguatedTypeName(p.type, p.typeTag).constData());
                } else {
                    fprintf(out, "        case %d:", propindex);
                    if (p.notify.isEmpty()) {
                        fprintf(out, " QtMocHelpers::setProperty(%s%s, *reinterpret_cast<%s%s*>(_v)); break;\n",
                                prefix.constData(), p.member.constData(), cxxTypeTag(p.typeTag),
                                disambiguatedTypeName(p.type, p.typeTag).constData());
                    } else {
                        fprintf(out, "\n            if (QtMocHelpers::setProperty(%s%s, *reinterpret_cast<%s%s*>(_v)))\n",
                                prefix.constData(), p.member.constData(), cxxTypeTag(p.typeTag),
                                disambiguatedTypeName(p.type, p.typeTag).constData());
                        fprintf(out, "                Q_EMIT _t->%s(", p.notify.constData());
                        if (p.notifyId > -1) {
                            const FunctionDef &f = cdef->signalList.at(p.notifyId);
                            if (f.arguments.size() == 1 && f.arguments.at(0).normalizedType == p.type)
                                fprintf(out, "%s%s", prefix.constData(), p.member.constData());
                        }
                        fprintf(out, ");\n");
                        fprintf(out, "            break;\n");
                    }
                }
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
            fprintf(out, "    }\n");
        }

        if (needReset) {
            fprintf(out, "    if (_c == QMetaObject::ResetProperty) {\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < int(cdef->propertyList.size()); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (p.reset.isEmpty())
                    continue;
                QByteArray prefix = "_t->";
                if (p.inPrivateClass.size()) {
                    prefix += p.inPrivateClass + "->";
                }
                fprintf(out, "        case %d: %s%s(); break;\n",
                        propindex, prefix.constData(), p.reset.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
            fprintf(out, "    }\n");
        }

        if (hasBindableProperties) {
            fprintf(out, "    if (_c == QMetaObject::BindableProperty) {\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < int(cdef->propertyList.size()); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (p.bind.isEmpty())
                    continue;
                QByteArray prefix = "_t->";
                if (p.inPrivateClass.size()) {
                    prefix += p.inPrivateClass + "->";
                }
                fprintf(out,
                        "        case %d: *static_cast<QUntypedBindable *>(_a[0]) = %s%s(); "
                        "break;\n",
                        propindex, prefix.constData(), p.bind.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
            fprintf(out, "    }\n");
        }
    }

    auto printUnused = [&](UsedArgs entry, const char *name) {
        if ((usedArgs & entry) == 0)
            fprintf(out, "    (void)%s;\n", name);
    };
    printUnused(UsedT, "_t");
    printUnused(UsedC, "_c");
    printUnused(UsedId, "_id");
    printUnused(UsedA, "_a");

    fprintf(out, "}\n");
}

void Generator::generateSignal(const FunctionDef *def, int index)
{
    if (def->wasCloned || def->isAbstract)
        return;
    fprintf(out, "\n// SIGNAL %d\n%s %s::%s(",
            index, def->type.name.constData(), cdef->qualified.constData(), def->name.constData());

    QByteArray thisPtr = "this";
    const char *constQualifier = "";

    if (def->isConst) {
        thisPtr = "const_cast< " + cdef->qualified + " *>(this)";
        constQualifier = "const";
    }

    Q_ASSERT(!def->normalizedType.isEmpty());
    if (def->arguments.isEmpty() && def->normalizedType == "void" && !def->isPrivateSignal) {
        fprintf(out, ")%s\n{\n"
                "    QMetaObject::activate(%s, &staticMetaObject, %d, nullptr);\n"
                "}\n", constQualifier, thisPtr.constData(), index);
        return;
    }

    int offset = 1;
    const auto begin = def->arguments.cbegin();
    const auto end = def->arguments.cend();
    for (auto it = begin; it != end; ++it) {
        const ArgumentDef &a = *it;
        if (it != begin)
            fputs(", ", out);
        if (a.type.name.size())
            fputs(a.type.name.constData(), out);
        fprintf(out, " _t%d", offset++);
        if (a.rightType.size())
            fputs(a.rightType.constData(), out);
    }
    if (def->isPrivateSignal) {
        if (!def->arguments.isEmpty())
            fprintf(out, ", ");
        fprintf(out, "QPrivateSignal _t%d", offset++);
    }

    fprintf(out, ")%s\n{\n", constQualifier);
    if (def->type.name.size() && def->normalizedType != "void") {
        QByteArray returnType = noRef(def->normalizedType);
        fprintf(out, "    %s _t0{};\n", returnType.constData());
    }

    fprintf(out, "    QMetaObject::activate<%s>(%s, &staticMetaObject, %d, ",
            def->normalizedType.constData(), thisPtr.constData(), index);
    if (def->normalizedType == "void") {
        fprintf(out, "nullptr");
    } else {
        fprintf(out, "std::addressof(_t0)");
    }
    int i;
    for (i = 1; i < offset; ++i)
        fprintf(out, ", _t%d", i);
    fprintf(out, ");\n");

    if (def->normalizedType != "void")
        fprintf(out, "    return _t0;\n");
    fprintf(out, "}\n");
}

static CborError jsonValueToCbor(CborEncoder *parent, const QJsonValue &v);
static CborError jsonObjectToCbor(CborEncoder *parent, const QJsonObject &o)
{
    auto it = o.constBegin();
    auto end = o.constEnd();
    CborEncoder map;
    cbor_encoder_create_map(parent, &map, o.size());

    for ( ; it != end; ++it) {
        QByteArray key = it.key().toUtf8();
        cbor_encode_text_string(&map, key.constData(), key.size());
        jsonValueToCbor(&map, it.value());
    }
    return cbor_encoder_close_container(parent, &map);
}

static CborError jsonArrayToCbor(CborEncoder *parent, const QJsonArray &a)
{
    CborEncoder array;
    cbor_encoder_create_array(parent, &array, a.size());
    for (const QJsonValue v : a)
        jsonValueToCbor(&array, v);
    return cbor_encoder_close_container(parent, &array);
}

static CborError jsonValueToCbor(CborEncoder *parent, const QJsonValue &v)
{
    switch (v.type()) {
    case QJsonValue::Null:
    case QJsonValue::Undefined:
        return cbor_encode_null(parent);
    case QJsonValue::Bool:
        return cbor_encode_boolean(parent, v.toBool());
    case QJsonValue::Array:
        return jsonArrayToCbor(parent, v.toArray());
    case QJsonValue::Object:
        return jsonObjectToCbor(parent, v.toObject());
    case QJsonValue::String: {
        QByteArray s = v.toString().toUtf8();
        return cbor_encode_text_string(parent, s.constData(), s.size());
    }
    case QJsonValue::Double: {
        double d = v.toDouble();
        if (d == floor(d) && fabs(d) <= (Q_INT64_C(1) << std::numeric_limits<double>::digits))
            return cbor_encode_int(parent, qint64(d));
        return cbor_encode_double(parent, d);
    }
    }
    Q_UNREACHABLE_RETURN(CborUnknownError);
}

void Generator::generatePluginMetaData()
{
    if (cdef->pluginData.iid.isEmpty())
        return;

    auto outputCborData = [this]() {
        CborDevice dev(out);
        CborEncoder enc;
        cbor_encoder_init_writer(&enc, CborDevice::callback, &dev);

        CborEncoder map;
        cbor_encoder_create_map(&enc, &map, CborIndefiniteLength);

        dev.nextItem("\"IID\"");
        cbor_encode_int(&map, int(QtPluginMetaDataKeys::IID));
        cbor_encode_text_string(&map, cdef->pluginData.iid.constData(), cdef->pluginData.iid.size());

        dev.nextItem("\"className\"");
        cbor_encode_int(&map, int(QtPluginMetaDataKeys::ClassName));
        cbor_encode_text_string(&map, cdef->classname.constData(), cdef->classname.size());

        QJsonObject o = cdef->pluginData.metaData.object();
        if (!o.isEmpty()) {
            dev.nextItem("\"MetaData\"");
            cbor_encode_int(&map, int(QtPluginMetaDataKeys::MetaData));
            jsonObjectToCbor(&map, o);
        }

        if (!cdef->pluginData.uri.isEmpty()) {
            dev.nextItem("\"URI\"");
            cbor_encode_int(&map, int(QtPluginMetaDataKeys::URI));
            cbor_encode_text_string(&map, cdef->pluginData.uri.constData(), cdef->pluginData.uri.size());
        }

        // Add -M args from the command line:
        for (auto it = cdef->pluginData.metaArgs.cbegin(), end = cdef->pluginData.metaArgs.cend(); it != end; ++it) {
            const QJsonArray &a = it.value();
            QByteArray key = it.key().toUtf8();
            dev.nextItem(QByteArray("command-line \"" + key + "\"").constData());
            cbor_encode_text_string(&map, key.constData(), key.size());
            jsonArrayToCbor(&map, a);
        }

        // Close the CBOR map manually
        dev.nextItem();
        cbor_encoder_close_container(&enc, &map);
    };

    // 'Use' all namespaces.
    qsizetype pos = cdef->qualified.indexOf("::");
    for ( ; pos != -1 ; pos = cdef->qualified.indexOf("::", pos + 2) )
        fprintf(out, "using namespace %s;\n", cdef->qualified.left(pos).constData());

    fputs("\n#ifdef QT_MOC_EXPORT_PLUGIN_V2", out);

    // Qt 6.3+ output
    fprintf(out, "\nstatic constexpr unsigned char qt_pluginMetaDataV2_%s[] = {",
          cdef->classname.constData());
    outputCborData();
    fprintf(out, "\n};\nQT_MOC_EXPORT_PLUGIN_V2(%s, %s, qt_pluginMetaDataV2_%s)\n",
            cdef->qualified.constData(), cdef->classname.constData(), cdef->classname.constData());

    // compatibility with Qt 6.0-6.2
    fprintf(out, "#else\nQT_PLUGIN_METADATA_SECTION\n"
          "Q_CONSTINIT static constexpr unsigned char qt_pluginMetaData_%s[] = {\n"
          "    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',\n"
          "    // metadata version, Qt version, architectural requirements\n"
          "    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),",
          cdef->classname.constData());
    outputCborData();
    fprintf(out, "\n};\nQT_MOC_EXPORT_PLUGIN(%s, %s)\n"
                 "#endif  // QT_MOC_EXPORT_PLUGIN_V2\n",
            cdef->qualified.constData(), cdef->classname.constData());

    fputs("\n", out);
}

QByteArray Generator::disambiguatedTypeName(const QByteArray &name)
{
    if (cdef->allEnumNames.contains(name))
        return "enum " + name;
    return name;
}

// in contexts where we already print the type tag, we don't want to do the
// disambiguation
QByteArray Generator::disambiguatedTypeName(const QByteArray &name, TypeTags tag)
{
    if (tag == TypeTag::None)
        return disambiguatedTypeName(name);
    return name;
}

QByteArray Generator::disambiguatedTypeNameForCast(const QByteArray &name)
{
    return QByteArray("std::add_pointer_t<"+ disambiguatedTypeName(name) +">");
}

QT_WARNING_DISABLE_GCC("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wundefined-internal")
QT_WARNING_DISABLE_MSVC(4334) // '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)

#define CBOR_NO_HALF_FLOAT_TYPE         1
#define CBOR_ENCODER_WRITER_CONTROL     1
#define CBOR_ENCODER_WRITE_FUNCTION     CborDevice::callback

QT_END_NAMESPACE

#include "cborencoder.c"
