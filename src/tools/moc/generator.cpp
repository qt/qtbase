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

#include <math.h>
#include <stdio.h>

#include <private/qmetaobject_p.h> //for the flags.
#include <private/qplugin_p.h> //for the flags.

QT_BEGIN_NAMESPACE

uint nameToBuiltinType(const QByteArray &name)
{
    if (name.isEmpty())
        return 0;

    uint tp = qMetaTypeTypeInternal(name.constData());
    return tp < uint(QMetaType::User) ? tp : uint(QMetaType::UnknownType);
}

/*
  Returns \c true if the type is a built-in type.
*/
bool isBuiltinType(const QByteArray &type)
 {
    int id = qMetaTypeTypeInternal(type.constData());
    if (id == QMetaType::UnknownType)
        return false;
    return (id < QMetaType::User);
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

 Generator::Generator(ClassDef *classDef, const QList<QByteArray> &metaTypes,
                      const QHash<QByteArray, QByteArray> &knownQObjectClasses,
                      const QHash<QByteArray, QByteArray> &knownGadgets, FILE *outfile,
                      bool requireCompleteTypes)
     : out(outfile),
       cdef(classDef),
       metaTypes(metaTypes),
       knownQObjectClasses(knownQObjectClasses),
       knownGadgets(knownGadgets),
       requireCompleteTypes(requireCompleteTypes)
 {
     if (cdef->superclassList.size())
         purestSuperClass = cdef->superclassList.constFirst().first;
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
        while (i < s.size() && is_hex_char(s.at(i)))
            ++i;
    } else if (is_octal_char(ch)) {
        while (i < startPos + 4
               && i < s.size()
               && is_octal_char(s.at(i))) {
            ++i;
        }
    } else { // single character escape sequence
        i = qMin(i + 1, s.size());
    }
    return i - startPos;
}

static inline uint lengthOfEscapedString(const QByteArray &str)
{
    int extra = 0;
    for (int j = 0; j < str.size(); ++j) {
        if (str.at(j) == '\\') {
            int cnt = lengthOfEscapeSequence(str, j) - 1;
            extra += cnt;
            j += cnt;
        }
    }
    return str.size() - extra;
}

// Prints \a s to \a out, breaking it into lines of at most ColumnWidth. The
// opening and closing quotes are NOT included (it's up to the caller).
static void printStringWithIndentation(FILE *out, const QByteArray &s)
{
    static constexpr int ColumnWidth = 72;
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
        fprintf(out, "\n    \"%.*s\"", int(spanLen), s.constData() + idx);
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
    int i = strings.indexOf(s);
    Q_ASSERT_X(i != -1, Q_FUNC_INFO, "We forgot to register some strings");
    return i;
}

// Returns the sum of all parameters (including return type) for the given
// \a list of methods. This is needed for calculating the size of the methods'
// parameter type/name meta-data.
static int aggregateParameterCount(const QList<FunctionDef> &list)
{
    int sum = 0;
    for (const FunctionDef &def : list)
        sum += int(def.arguments.size()) + 1; // +1 for return type
    return sum;
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
        QByteArray ba = oneArgTemplateType + "<";
        if (propertyType.startsWith(ba) && propertyType.endsWith(">")) {
            const qsizetype argumentSize = propertyType.size() - oneArgTemplateType.size() - 1
                                     // The closing '>'
                                     - 1
                                     // templates inside templates have an extra whitespace char to strip.
                                     - (propertyType.at(propertyType.size() - 2) == ' ' ? 1 : 0 );
            const QByteArray templateArg = propertyType.mid(oneArgTemplateType.size() + 1, argumentSize);
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
    QByteArray qualifiedClassNameIdentifier = identifier;

    // Remove ':'s in the name, but be sure not to create any illegal
    // identifiers in the process. (Don't replace with '_', because
    // that will create problems with things like NS_::_class.)
    qualifiedClassNameIdentifier.replace("::", "SCOPE");

    // Also, avoid any leading/trailing underscores (we'll concatenate
    // the generated name with other prefixes/suffixes, and these latter
    // may already include an underscore, leading to two underscores)
    qualifiedClassNameIdentifier = "CLASS" + qualifiedClassNameIdentifier + "ENDCLASS";
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

    const bool hasStaticMetaCall =
            (cdef->hasQObject || !cdef->methodList.isEmpty()
             || !cdef->propertyList.isEmpty() || !cdef->constructorList.isEmpty());

    const QByteArray qualifiedClassNameIdentifier = generateQualifiedClassNameIdentifier(cdef->qualified);

    // ensure the qt_meta_stringdata_XXXX_t type is local
    fprintf(out, "namespace {\n");

//
// Build the strings using QtMocHelpers::StringData
//

    fprintf(out, "\n#ifdef QT_MOC_HAS_STRINGDATA\n"
                 "struct qt_meta_stringdata_%s_t {};\n"
                 "static constexpr auto qt_meta_stringdata_%s = QtMocHelpers::stringData(",
            qualifiedClassNameIdentifier.constData(), qualifiedClassNameIdentifier.constData());
    {
        char comma = 0;
        for (const QByteArray &str : strings) {
            if (comma)
                fputc(comma, out);
            printStringWithIndentation(out, str);
            comma = ',';
        }
    }
    fprintf(out, "\n);\n"
            "#else  // !QT_MOC_HAS_STRING_DATA\n");

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    fprintf(out, "#error \"qtmochelpers.h not found or too old.\"\n");
#else
//
// Build stringdata struct
//

    fprintf(out, "struct qt_meta_stringdata_%s_t {\n", qualifiedClassNameIdentifier.constData());
    fprintf(out, "    uint offsetsAndSizes[%d];\n", int(strings.size() * 2));
    for (int i = 0; i < strings.size(); ++i) {
        int thisLength = lengthOfEscapedString(strings.at(i)) + 1;
        fprintf(out, "    char stringdata%d[%d];\n", i, thisLength);
    }
    fprintf(out, "};\n");

    // Macro that simplifies the string data listing. The offset is calculated
    // from the top of the stringdata object (i.e., past the uints).
    fprintf(out, "#define QT_MOC_LITERAL(ofs, len) \\\n"
            "    uint(sizeof(qt_meta_stringdata_%s_t::offsetsAndSizes) + ofs), len \n",
            qualifiedClassNameIdentifier.constData());

    fprintf(out, "Q_CONSTINIT static const qt_meta_stringdata_%s_t qt_meta_stringdata_%s = {\n",
            qualifiedClassNameIdentifier.constData(), qualifiedClassNameIdentifier.constData());
    fprintf(out, "    {");
    {
        int idx = 0;
        for (int i = 0; i < strings.size(); ++i) {
            const QByteArray &str = strings.at(i);
            const QByteArray comment = str.size() > 32 ? str.left(29) + "..." : str;
            const char *comma = (i != strings.size() - 1 ? "," : " ");
            int len = lengthOfEscapedString(str);
            fprintf(out, "\n        QT_MOC_LITERAL(%d, %d)%s  // \"%s\"", idx, len, comma,
                    comment.constData());

            idx += len + 1;
        }
        fprintf(out, "\n    }");
    }

//
// Build stringdata arrays
//
    for (const QByteArray &s : std::as_const(strings)) {
        fputc(',', out);
        printStringWithIndentation(out, s);
    }

// Terminate stringdata struct
    fprintf(out, "\n};\n");
    fprintf(out, "#undef QT_MOC_LITERAL\n");
#endif // Qt 6.9

    fprintf(out, "#endif // !QT_MOC_HAS_STRING_DATA\n");
    fprintf(out, "} // unnamed namespace\n\n");

//
// build the data array
//

    int index = MetaObjectPrivateFieldCount;
    fprintf(out, "Q_CONSTINIT static const uint qt_meta_data_%s[] = {\n", qualifiedClassNameIdentifier.constData());
    fprintf(out, "\n // content:\n");
    fprintf(out, "    %4d,       // revision\n", int(QMetaObjectPrivate::OutputRevision));
    fprintf(out, "    %4d,       // classname\n", stridx(cdef->qualified));
    fprintf(out, "    %4d, %4d, // classinfo\n", int(cdef->classInfoList.size()), int(cdef->classInfoList.size() ? index : 0));
    index += cdef->classInfoList.size() * 2;

    const qsizetype methodCount = cdef->signalList.size() + cdef->slotList.size() + cdef->methodList.size();
    fprintf(out, "    %4" PRIdQSIZETYPE ", %4d, // methods\n", methodCount, methodCount ? index : 0);
    index += methodCount * QMetaObjectPrivate::IntsPerMethod;
    if (cdef->revisionedMethods)
        index += methodCount;
    int paramsIndex = index;
    int totalParameterCount = aggregateParameterCount(cdef->signalList)
            + aggregateParameterCount(cdef->slotList)
            + aggregateParameterCount(cdef->methodList)
            + aggregateParameterCount(cdef->constructorList);
    index += totalParameterCount * 2 // types and parameter names
            - methodCount // return "parameters" don't have names
            - cdef->constructorList.size(); // "this" parameters don't have names

    fprintf(out, "    %4d, %4d, // properties\n", int(cdef->propertyList.size()), int(cdef->propertyList.size() ? index : 0));
    index += cdef->propertyList.size() * QMetaObjectPrivate::IntsPerProperty;
    fprintf(out, "    %4d, %4d, // enums/sets\n", int(cdef->enumList.size()), cdef->enumList.size() ? index : 0);

    int enumsIndex = index;
    for (const EnumDef &def : std::as_const(cdef->enumList))
        index += QMetaObjectPrivate::IntsPerEnum + (def.values.size() * 2);

    fprintf(out, "    %4d, %4d, // constructors\n", isConstructible ? int(cdef->constructorList.size()) : 0,
            isConstructible ? index : 0);

    int flags = 0;
    if (cdef->hasQGadget || cdef->hasQNamespace) {
        // Ideally, all the classes could have that flag. But this broke classes generated
        // by qdbusxml2cpp which generate code that require that we call qt_metacall for properties
        flags |= PropertyAccessInStaticMetaCall;
    }
    fprintf(out, "    %4d,       // flags\n", flags);
    fprintf(out, "    %4d,       // signalCount\n", int(cdef->signalList.size()));


//
// Build classinfo array
//
    generateClassInfos();

    // all property metatypes + all enum metatypes + 1 for the type of the current class itself
    int initialMetaTypeOffset = cdef->propertyList.size() + cdef->enumList.size() + 1;

//
// Build signals array first, otherwise the signal indices would be wrong
//
    generateFunctions(cdef->signalList, "signal", MethodSignal, paramsIndex, initialMetaTypeOffset);

//
// Build slots array
//
    generateFunctions(cdef->slotList, "slot", MethodSlot, paramsIndex, initialMetaTypeOffset);

//
// Build method array
//
    generateFunctions(cdef->methodList, "method", MethodMethod, paramsIndex, initialMetaTypeOffset);

//
// Build method version arrays
//
    if (cdef->revisionedMethods) {
        generateFunctionRevisions(cdef->signalList, "signal");
        generateFunctionRevisions(cdef->slotList, "slot");
        generateFunctionRevisions(cdef->methodList, "method");
    }

//
// Build method parameters array
//
    generateFunctionParameters(cdef->signalList, "signal");
    generateFunctionParameters(cdef->slotList, "slot");
    generateFunctionParameters(cdef->methodList, "method");
    if (isConstructible)
        generateFunctionParameters(cdef->constructorList, "constructor");

//
// Build property array
//
    generateProperties();

//
// Build enums array
//
    generateEnums(enumsIndex);

//
// Build constructors array
//
    if (isConstructible)
        generateFunctions(cdef->constructorList, "constructor", MethodConstructor, paramsIndex, initialMetaTypeOffset);

//
// Terminate data array
//
    fprintf(out, "\n       0        // eod\n};\n\n");

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
    fprintf(out, "    qt_meta_stringdata_%s.offsetsAndSizes,\n"
            "    qt_meta_data_%s,\n", qualifiedClassNameIdentifier.constData(),
            qualifiedClassNameIdentifier.constData());
    if (hasStaticMetaCall)
        fprintf(out, "    qt_static_metacall,\n");
    else
        fprintf(out, "    nullptr,\n");

    if (extraList.isEmpty())
        fprintf(out, "    nullptr,\n");
    else
        fprintf(out, "    qt_meta_extradata_%s,\n", qualifiedClassNameIdentifier.constData());

    const char *comma = "";
    const bool requireCompleteness = requireCompleteTypes || cdef->requireCompleteMethodTypes;
    auto stringForType = [requireCompleteness](const QByteArray &type, bool forceComplete) -> QByteArray {
        const char *forceCompleteType = forceComplete ? ", std::true_type>" : ", std::false_type>";
        if (requireCompleteness)
            return type;
        return "QtPrivate::TypeAndForceComplete<" % type % forceCompleteType;
    };
    if (!requireCompleteness) {
        fprintf(out, "    qt_incomplete_metaTypeArray<qt_meta_stringdata_%s_t", qualifiedClassNameIdentifier.constData());
        comma = ",";
    } else {
        fprintf(out, "    qt_metaTypeArray<");
    }
    // metatypes for properties
    for (const PropertyDef &p : std::as_const(cdef->propertyList)) {
        fprintf(out, "%s\n        // property '%s'\n        %s",
                comma, p.name.constData(), stringForType(p.type, true).constData());
        comma = ",";
    }

    // metatypes for enums
    for (const EnumDef &e : std::as_const(cdef->enumList)) {
        fprintf(out, "%s\n        // enum '%s'\n        %s",
                comma, e.name.constData(), stringForType(e.qualifiedType(cdef), true).constData());
        comma = ",";
    }

    // type name for the Q_OJBECT/GADGET itself, void for namespaces
    auto ownType = !cdef->hasQNamespace ? cdef->classname.data() : "void";
    fprintf(out, "%s\n        // Q_OBJECT / Q_GADGET\n        %s",
            comma, stringForType(ownType, true).constData());
    comma = ",";

    // metatypes for all exposed methods
    // because we definitely printed something above, this section doesn't need comma control
    for (const QList<FunctionDef> &methodContainer :
    { cdef->signalList, cdef->slotList, cdef->methodList }) {
        for (const FunctionDef &fdef : methodContainer) {
            fprintf(out, ",\n        // method '%s'\n        %s",
                    fdef.name.constData(), stringForType(fdef.type.name, false).constData());
            for (const auto &argument: fdef.arguments)
                fprintf(out, ",\n        %s", stringForType(argument.type.name, false).constData());
        }
    }

    // but constructors have no return types, so this needs comma control again
    for (const FunctionDef &fdef : std::as_const(cdef->constructorList)) {
        if (fdef.arguments.isEmpty())
            continue;

        fprintf(out, "%s\n        // constructor '%s'", comma, fdef.name.constData());
        comma = "";
        for (const auto &argument: fdef.arguments) {
            fprintf(out, "%s\n        %s", comma,
                    stringForType(argument.type.name, false).constData());
            comma = ",";
        }
    }
    fprintf(out, "\n    >,\n");

    fprintf(out, "    nullptr\n} };\n\n");

//
// Generate internal qt_static_metacall() function
//
    if (hasStaticMetaCall)
        generateStaticMetacall();

    if (!cdef->hasQObject)
        return;

    fprintf(out, "\nconst QMetaObject *%s::metaObject() const\n{\n    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;\n}\n",
            cdef->qualified.constData());


//
// Generate smart cast function
//
    fprintf(out, "\nvoid *%s::qt_metacast(const char *_clname)\n{\n", cdef->qualified.constData());
    fprintf(out, "    if (!_clname) return nullptr;\n");
    fprintf(out, "    if (!strcmp(_clname, qt_meta_stringdata_%s.stringdata0))\n"
                  "        return static_cast<void*>(this);\n",
            qualifiedClassNameIdentifier.constData());

    // for all superclasses but the first one
    if (cdef->superclassList.size() > 1) {
        auto it = cdef->superclassList.cbegin() + 1;
        const auto end = cdef->superclassList.cend();
        for (; it != end; ++it) {
            const auto &[className, access] = *it;
            if (access == FunctionDef::Private)
                continue;
            const char *cname = className.constData();
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

//
// Generate internal qt_metacall()  function
//
    generateMetacall();

//
// Generate internal signal functions
//
    for (int signalindex = 0; signalindex < cdef->signalList.size(); ++signalindex)
        generateSignal(&cdef->signalList[signalindex], signalindex);

//
// Generate plugin meta data
//
    generatePluginMetaData();

//
// Generate function to make sure the non-class signals exist in the parent classes
//
    if (!cdef->nonClassSignalList.isEmpty()) {
        fprintf(out, "// If you get a compile error in this function it can be because either\n");
        fprintf(out, "//     a) You are using a NOTIFY signal that does not exist. Fix it.\n");
        fprintf(out, "//     b) You are using a NOTIFY signal that does exist (in a parent class) but has a non-empty parameter list. This is a moc limitation.\n");
        fprintf(out, "[[maybe_unused]] static void checkNotifySignalValidity_%s(%s *t) {\n", qualifiedClassNameIdentifier.constData(), cdef->qualified.constData());
        for (const QByteArray &nonClassSignal : std::as_const(cdef->nonClassSignalList))
            fprintf(out, "    t->%s();\n", nonClassSignal.constData());
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

void Generator::generateClassInfos()
{
    if (cdef->classInfoList.isEmpty())
        return;

    fprintf(out, "\n // classinfo: key, value\n");

    for (const ClassInfoDef &c : std::as_const(cdef->classInfoList))
        fprintf(out, "    %4d, %4d,\n", stridx(c.name), stridx(c.value));
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

void Generator::generateFunctions(const QList<FunctionDef> &list, const char *functype, int type,
                                  int &paramsIndex, int &initialMetatypeOffset)
{
    if (list.isEmpty())
        return;
    fprintf(out, "\n // %ss: name, argc, parameters, tag, flags, initial metatype offsets\n", functype);

    for (const FunctionDef &f : list) {
        QByteArray comment;
        uint flags = type;
        if (f.access == FunctionDef::Private) {
            flags |= AccessPrivate;
            comment.append("Private");
        } else if (f.access == FunctionDef::Public) {
            flags |= AccessPublic;
            comment.append("Public");
        } else if (f.access == FunctionDef::Protected) {
            flags |= AccessProtected;
            comment.append("Protected");
        }
        if (f.isCompat) {
            flags |= MethodCompatibility;
            comment.append(" | MethodCompatibility");
        }
        if (f.wasCloned) {
            flags |= MethodCloned;
            comment.append(" | MethodCloned");
        }
        if (f.isScriptable) {
            flags |= MethodScriptable;
            comment.append(" | isScriptable");
        }
        if (f.revision > 0) {
            flags |= MethodRevisioned;
            comment.append(" | MethodRevisioned");
        }

        if (f.isConst) {
            flags |= MethodIsConst;
            comment.append(" | MethodIsConst ");
        }

        int argc = f.arguments.size();
        fprintf(out, "    %4d, %4d, %4d, %4d, 0x%02x, %4d /* %s */,\n",
            stridx(f.name), argc, paramsIndex, stridx(f.tag), flags, initialMetatypeOffset, comment.constData());

        paramsIndex += 1 + argc * 2;
        // constructors don't have a return type
        initialMetatypeOffset += (f.isConstructor ? 0 : 1) + argc;
    }
}

void Generator::generateFunctionRevisions(const QList<FunctionDef> &list, const char *functype)
{
    if (list.size())
        fprintf(out, "\n // %ss: revision\n", functype);
    for (const FunctionDef &f : list)
        fprintf(out, "    %4d,\n", f.revision);
}

void Generator::generateFunctionParameters(const QList<FunctionDef> &list, const char *functype)
{
    if (list.isEmpty())
        return;
    fprintf(out, "\n // %ss: parameters\n", functype);
    for (const FunctionDef &f : list) {
        fprintf(out, "    ");

        // Types
        int argsCount = f.arguments.size();
        for (int j = -1; j < argsCount; ++j) {
            if (j > -1)
                fputc(' ', out);
            const QByteArray &typeName = (j < 0) ? f.normalizedType : f.arguments.at(j).normalizedType;
            generateTypeInfo(typeName, /*allowEmptyName=*/f.isConstructor);
            fputc(',', out);
        }

        // Parameter names
        for (const ArgumentDef &arg : f.arguments)
            fprintf(out, " %4d,", stridx(arg.name));

        fprintf(out, "\n");
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

void Generator::generateProperties()
{
    //
    // Create meta data
    //

    if (cdef->propertyList.size())
        fprintf(out, "\n // properties: name, type, flags\n");
    for (const PropertyDef &p : std::as_const(cdef->propertyList)) {
        uint flags = Invalid;
        if (!isBuiltinType(p.type))
            flags |= EnumOrFlag;
        if (!p.member.isEmpty() && !p.constant)
            flags |= Writable;
        if (!p.read.isEmpty() || !p.member.isEmpty())
            flags |= Readable;
        if (!p.write.isEmpty()) {
            flags |= Writable;
            if (p.stdCppSet())
                flags |= StdCppSet;
        }

        if (!p.reset.isEmpty())
            flags |= Resettable;

        if (p.designable != "false")
            flags |= Designable;

        if (p.scriptable != "false")
            flags |= Scriptable;

        if (p.stored != "false")
            flags |= Stored;

        if (p.user != "false")
            flags |= User;

        if (p.constant)
            flags |= Constant;
        if (p.final)
            flags |= Final;
        if (p.required)
            flags |= Required;

        if (!p.bind.isEmpty())
            flags |= Bindable;

        fprintf(out, "    %4d, ", stridx(p.name));
        generateTypeInfo(p.type);
        int notifyId = p.notifyId;
        if (p.notifyId < -1) {
            // signal is in parent class
            const int indexInStrings = strings.indexOf(p.notify);
            notifyId = indexInStrings | IsUnresolvedSignal;
        }
        fprintf(out, ", 0x%.8x, uint(%d), %d,\n", flags, notifyId, p.revision);
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

void Generator::generateEnums(int index)
{
    if (cdef->enumDeclarations.isEmpty())
        return;

    fprintf(out, "\n // enums: name, alias, flags, count, data\n");
    index += QMetaObjectPrivate::IntsPerEnum * cdef->enumList.size();
    int i;
    for (i = 0; i < cdef->enumList.size(); ++i) {
        const EnumDef &e = cdef->enumList.at(i);
        int flags = 0;
        if (cdef->enumDeclarations.value(e.name))
            flags |= EnumIsFlag;
        if (e.isEnumClass)
            flags |= EnumIsScoped;
        fprintf(out, "    %4d, %4d, 0x%.1x, %4d, %4d,\n",
                 stridx(e.name),
                 e.enumName.isNull() ? stridx(e.name) : stridx(e.enumName),
                 flags,
                 int(e.values.size()),
                 index);
        index += e.values.size() * 2;
    }

    fprintf(out, "\n // enum data: key, value\n");
    for (const EnumDef &e : std::as_const(cdef->enumList)) {
        for (const QByteArray &val : e.values) {
            QByteArray code = cdef->qualified.constData();
            if (e.isEnumClass)
                code += "::" + (e.enumName.isNull() ? e.name : e.enumName);
            code += "::" + val;
            fprintf(out, "    %4d, uint(%s),\n",
                    stridx(val), code.constData());
        }
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


    bool needElse = false;
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

    fprintf(out, "    ");

    if (methodList.size()) {
        needElse = true;
        fprintf(out, "if (_c == QMetaObject::InvokeMetaMethod) {\n");
        fprintf(out, "        if (_id < %d)\n", int(methodList.size()));
        fprintf(out, "            qt_static_metacall(this, _c, _id, _a);\n");
        fprintf(out, "        _id -= %d;\n    }", int(methodList.size()));

        fprintf(out, " else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {\n");
        fprintf(out, "        if (_id < %d)\n", int(methodList.size()));

        if (methodsWithAutomaticTypesHelper(methodList).isEmpty())
            fprintf(out, "            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();\n");
        else
            fprintf(out, "            qt_static_metacall(this, _c, _id, _a);\n");
        fprintf(out, "        _id -= %d;\n    }", int(methodList.size()));

    }

    if (cdef->propertyList.size()) {
        if (needElse)
            fprintf(out, "else ");
        fprintf(out,
            "if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty\n"
            "            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty\n"
            "            || _c == QMetaObject::RegisterPropertyMetaType) {\n"
            "        qt_static_metacall(this, _c, _id, _a);\n"
            "        _id -= %d;\n    }", int(cdef->propertyList.size()));
    }
    if (methodList.size() || cdef->propertyList.size())
        fprintf(out, "\n    ");
    fprintf(out,"return _id;\n}\n");
}


// ### Qt 7 (6.x?): remove
QMultiMap<QByteArray, int> Generator::automaticPropertyMetaTypesHelper()
{
    QMultiMap<QByteArray, int> automaticPropertyMetaTypes;
    for (int i = 0; i < cdef->propertyList.size(); ++i) {
        const QByteArray propertyType = cdef->propertyList.at(i).type;
        if (registerableMetaType(propertyType) && !isBuiltinType(propertyType))
            automaticPropertyMetaTypes.insert(propertyType, i);
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
            const QByteArray argType = f.arguments.at(j).normalizedType;
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

    bool needElse = false;
    bool isUsed_a = false;

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
                    a.typeNameForCast.constData(), offset++);
        }
    };

    if (!cdef->constructorList.isEmpty()) {
        fprintf(out, "    if (_c == QMetaObject::CreateInstance) {\n");
        fprintf(out, "        switch (_id) {\n");
        const int ctorend = cdef->constructorList.size();
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
        fprintf(out, "    } else if (_c == QMetaObject::ConstructInPlace) {\n");
        fprintf(out, "        switch (_id) {\n");
        for (int ctorindex = 0; ctorindex < ctorend; ++ctorindex) {
            fprintf(out, "        case %d: { new (_a[0]) %s(",
                    ctorindex, cdef->classname.constData());
            generateCtorArguments(ctorindex);
            fprintf(out, "); } break;\n");
        }
        fprintf(out, "        default: break;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }");
        needElse = true;
        isUsed_a = true;
    }

    QList<FunctionDef> methodList;
    methodList += cdef->signalList;
    methodList += cdef->slotList;
    methodList += cdef->methodList;

    if (!methodList.isEmpty()) {
        if (needElse)
            fprintf(out, " else ");
        else
            fprintf(out, "    ");
        fprintf(out, "if (_c == QMetaObject::InvokeMetaMethod) {\n");
        if (cdef->hasQObject) {
#ifndef QT_NO_DEBUG
            fprintf(out, "        Q_ASSERT(staticMetaObject.cast(_o));\n");
#endif
            fprintf(out, "        auto *_t = static_cast<%s *>(_o);\n", cdef->classname.constData());
        } else {
            fprintf(out, "        auto *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.constData());
        }
        fprintf(out, "        (void)_t;\n");
        fprintf(out, "        switch (_id) {\n");
        for (int methodindex = 0; methodindex < methodList.size(); ++methodindex) {
            const FunctionDef &f = methodList.at(methodindex);
            Q_ASSERT(!f.normalizedType.isEmpty());
            fprintf(out, "        case %d: ", methodindex);
            if (f.normalizedType != "void")
                fprintf(out, "{ %s _r = ", noRef(f.normalizedType).constData());
            fprintf(out, "_t->");
            if (f.inPrivateClass.size())
                fprintf(out, "%s->", f.inPrivateClass.constData());
            fprintf(out, "%s(", f.name.constData());
            int offset = 1;

            if (f.isRawSlot) {
                fprintf(out, "QMethodRawArguments{ _a }");
            } else {
                const auto begin = f.arguments.cbegin();
                const auto end = f.arguments.cend();
                for (auto it = begin; it != end; ++it) {
                    const ArgumentDef &a = *it;
                    if (it != begin)
                        fprintf(out, ",");
                    fprintf(out, "(*reinterpret_cast< %s>(_a[%d]))",a.typeNameForCast.constData(), offset++);
                    isUsed_a = true;
                }
                if (f.isPrivateSignal) {
                    if (!f.arguments.isEmpty())
                        fprintf(out, ", ");
                    fprintf(out, "%s", "QPrivateSignal()");
                }
            }
            fprintf(out, ");");
            if (f.normalizedType != "void") {
                fprintf(out, "\n            if (_a[0]) *reinterpret_cast< %s*>(_a[0]) = std::move(_r); } ",
                        noRef(f.normalizedType).constData());
                isUsed_a = true;
            }
            fprintf(out, " break;\n");
        }
        fprintf(out, "        default: ;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }");
        needElse = true;

        QMap<int, QMultiMap<QByteArray, int> > methodsWithAutomaticTypes = methodsWithAutomaticTypesHelper(methodList);

        if (!methodsWithAutomaticTypes.isEmpty()) {
            fprintf(out, " else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {\n");
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
            fprintf(out, "    }");
            isUsed_a = true;
        }

    }
    if (!cdef->signalList.isEmpty()) {
        Q_ASSERT(needElse); // if there is signal, there was method.
        fprintf(out, " else if (_c == QMetaObject::IndexOfMethod) {\n");
        fprintf(out, "        int *result = reinterpret_cast<int *>(_a[0]);\n");
        bool anythingUsed = false;
        for (int methodindex = 0; methodindex < cdef->signalList.size(); ++methodindex) {
            const FunctionDef &f = cdef->signalList.at(methodindex);
            if (f.wasCloned || !f.inPrivateClass.isEmpty() || f.isStatic)
                continue;
            anythingUsed = true;
            fprintf(out, "        {\n");
            fprintf(out, "            using _t = %s (%s::*)(",f.type.rawName.constData() , cdef->classname.constData());

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
            if (f.isConst)
                fprintf(out, ") const;\n");
            else
                fprintf(out, ");\n");
            fprintf(out, "            if (_t _q_method = &%s::%s; *reinterpret_cast<_t *>(_a[1]) == _q_method) {\n",
                    cdef->classname.constData(), f.name.constData());
            fprintf(out, "                *result = %d;\n", methodindex);
            fprintf(out, "                return;\n");
            fprintf(out, "            }\n        }\n");
        }
        if (!anythingUsed)
            fprintf(out, "        (void)result;\n");
        fprintf(out, "    }");
        needElse = true;
    }

    const QMultiMap<QByteArray, int> automaticPropertyMetaTypes = automaticPropertyMetaTypesHelper();

    if (!automaticPropertyMetaTypes.isEmpty()) {
        if (needElse)
            fprintf(out, " else ");
        else
            fprintf(out, "    ");
        fprintf(out, "if (_c == QMetaObject::RegisterPropertyMetaType) {\n");
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
        fprintf(out, "    } ");
        isUsed_a = true;
        needElse = true;
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
        if (needElse)
            fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::ReadProperty) {\n");

        auto setupMemberAccess = [this]() {
            if (cdef->hasQObject) {
#ifndef QT_NO_DEBUG
                fprintf(out, "        Q_ASSERT(staticMetaObject.cast(_o));\n");
#endif
                fprintf(out, "        auto *_t = static_cast<%s *>(_o);\n", cdef->classname.constData());
            } else {
                fprintf(out, "        auto *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.constData());
            }
            fprintf(out, "        (void)_t;\n");
        };

        if (needGet) {
            setupMemberAccess();
            if (needTempVarForGet)
                fprintf(out, "        void *_v = _a[0];\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
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
                else if (cdef->enumDeclarations.value(p.type, false))
                    fprintf(out, "        case %d: *reinterpret_cast<int*>(_v) = QFlag(%s%s()); break;\n",
                            propindex, prefix.constData(), p.read.constData());
                else if (p.read == "default")
                    fprintf(out, "        case %d: *reinterpret_cast< %s*>(_v) = %s%s().value(); break;\n",
                            propindex, p.type.constData(), prefix.constData(), p.bind.constData());
                else if (!p.read.isEmpty())
                    fprintf(out, "        case %d: *reinterpret_cast< %s*>(_v) = %s%s(); break;\n",
                            propindex, p.type.constData(), prefix.constData(), p.read.constData());
                else
                    fprintf(out, "        case %d: *reinterpret_cast< %s*>(_v) = %s%s; break;\n",
                            propindex, p.type.constData(), prefix.constData(), p.member.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }

        fprintf(out, "    }");

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::WriteProperty) {\n");

        if (needSet) {
            setupMemberAccess();
            fprintf(out, "        void *_v = _a[0];\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (p.constant)
                    continue;
                if (p.write.isEmpty() && p.member.isEmpty())
                    continue;
                QByteArray prefix = "_t->";
                if (p.inPrivateClass.size()) {
                    prefix += p.inPrivateClass + "->";
                }
                if (cdef->enumDeclarations.value(p.type, false)) {
                    fprintf(out, "        case %d: %s%s(QFlag(*reinterpret_cast<int*>(_v))); break;\n",
                            propindex, prefix.constData(), p.write.constData());
                } else if (p.write == "default") {
                    fprintf(out, "        case %d: {\n", propindex);
                    fprintf(out, "            %s%s().setValue(*reinterpret_cast< %s*>(_v));\n",
                            prefix.constData(), p.bind.constData(), p.type.constData());
                    fprintf(out, "            break;\n");
                    fprintf(out, "        }\n");
                } else if (!p.write.isEmpty()) {
                    fprintf(out, "        case %d: %s%s(*reinterpret_cast< %s*>(_v)); break;\n",
                            propindex, prefix.constData(), p.write.constData(), p.type.constData());
                } else {
                    fprintf(out, "        case %d:\n", propindex);
                    fprintf(out, "            if (%s%s != *reinterpret_cast< %s*>(_v)) {\n",
                            prefix.constData(), p.member.constData(), p.type.constData());
                    fprintf(out, "                %s%s = *reinterpret_cast< %s*>(_v);\n",
                            prefix.constData(), p.member.constData(), p.type.constData());
                    if (!p.notify.isEmpty() && p.notifyId > -1) {
                        const FunctionDef &f = cdef->signalList.at(p.notifyId);
                        if (f.arguments.size() == 0)
                            fprintf(out, "                Q_EMIT _t->%s();\n", p.notify.constData());
                        else if (f.arguments.size() == 1 && f.arguments.at(0).normalizedType == p.type)
                            fprintf(out, "                Q_EMIT _t->%s(%s%s);\n",
                                    p.notify.constData(), prefix.constData(), p.member.constData());
                    } else if (!p.notify.isEmpty() && p.notifyId < -1) {
                        fprintf(out, "                Q_EMIT _t->%s();\n", p.notify.constData());
                    }
                    fprintf(out, "            }\n");
                    fprintf(out, "            break;\n");
                }
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }

        fprintf(out, "    }");

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::ResetProperty) {\n");
        if (needReset) {
            setupMemberAccess();
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
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
        }
        fprintf(out, "    }");

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::BindableProperty) {\n");
        if (hasBindableProperties) {
            setupMemberAccess();
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
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
        }
        fprintf(out, "    }");
        needElse = true;
    }

    if (needElse)
        fprintf(out, "\n");

    if (methodList.isEmpty()) {
        fprintf(out, "    (void)_o;\n");
        if (cdef->constructorList.isEmpty() && automaticPropertyMetaTypes.isEmpty() && methodsWithAutomaticTypesHelper(methodList).isEmpty()) {
            fprintf(out, "    (void)_id;\n");
            fprintf(out, "    (void)_c;\n");
        }
    }
    if (!isUsed_a)
        fprintf(out, "    (void)_a;\n");

    fprintf(out, "}\n");
}

void Generator::generateSignal(FunctionDef *def,int index)
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

    fprintf(out, "    void *_a[] = { ");
    if (def->normalizedType == "void") {
        fprintf(out, "nullptr");
    } else {
        if (def->returnTypeIsVolatile)
             fprintf(out, "const_cast<void*>(reinterpret_cast<const volatile void*>(std::addressof(_t0)))");
        else
             fprintf(out, "const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t0)))");
    }
    int i;
    for (i = 1; i < offset; ++i)
        if (i <= def->arguments.size() && def->arguments.at(i - 1).type.isVolatile)
            fprintf(out, ", const_cast<void*>(reinterpret_cast<const volatile void*>(std::addressof(_t%d)))", i);
        else
            fprintf(out, ", const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t%d)))", i);
    fprintf(out, " };\n");
    fprintf(out, "    QMetaObject::activate(%s, &staticMetaObject, %d, _a);\n", thisPtr.constData(), index);
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

QT_WARNING_DISABLE_GCC("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wundefined-internal")
QT_WARNING_DISABLE_MSVC(4334) // '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)

#define CBOR_ENCODER_WRITER_CONTROL     1
#define CBOR_ENCODER_WRITE_FUNCTION     CborDevice::callback

QT_END_NAMESPACE

#include "cborencoder.c"
