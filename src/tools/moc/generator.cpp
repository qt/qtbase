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

#include "generator.h"
#include "outputrevision.h"
#include "utils.h"
#include <QtCore/qmetatype.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qplugin.h>
#include <stdio.h>

#include <private/qmetaobject_p.h> //for the flags.

QT_BEGIN_NAMESPACE

uint qvariant_nameToType(const QByteArray &name)
{
    if (name.isEmpty())
        return 0;

    uint tp = QMetaType::type(name.constData());
    return tp < QMetaType::User ? tp : 0;
}

/*
  Returns true if the type is a QVariant types.
*/
bool isVariantType(const QByteArray &type)
{
    return qvariant_nameToType(type) != 0;
}

/*!
  Returns true if the type is qreal.
*/
static bool isQRealType(const QByteArray &type)
{
    return (type == "qreal");
}

Generator::Generator(ClassDef *classDef, const QList<QByteArray> &metaTypes, FILE *outfile)
    : out(outfile), cdef(classDef), metaTypes(metaTypes)
{
    if (cdef->superclassList.size())
        purestSuperClass = cdef->superclassList.first().first;
}

static inline int lengthOfEscapeSequence(const QByteArray &s, int i)
{
    if (s.at(i) != '\\' || i >= s.length() - 1)
        return 1;
    const int startPos = i;
    ++i;
    char ch = s.at(i);
    if (ch == 'x') {
        ++i;
        while (i < s.length() && is_hex_char(s.at(i)))
            ++i;
    } else if (is_octal_char(ch)) {
        while (i < startPos + 4
               && i < s.length()
               && is_octal_char(s.at(i))) {
            ++i;
        }
    } else { // single character escape sequence
        i = qMin(i + 1, s.length());
    }
    return i - startPos;
}

int Generator::strreg(const QByteArray &s)
{
    int idx = 0;
    for (int i = 0; i < strings.size(); ++i) {
        const QByteArray &str = strings.at(i);
        if (str == s)
            return idx;
        idx += str.length() + 1;
        for (int i = 0; i < str.length(); ++i) {
            if (str.at(i) == '\\') {
                int cnt = lengthOfEscapeSequence(str, i) - 1;
                idx -= cnt;
                i += cnt;
            }
        }
    }
    strings.append(s);
    return idx;
}

void Generator::generateCode()
{
    bool isQt = (cdef->classname == "Qt");
    bool isQObject = (cdef->classname == "QObject");
    bool isConstructible = !cdef->constructorList.isEmpty();

//
// build the data array
//

    // filter out undeclared enumerators and sets
    {
        QList<EnumDef> enumList;
        for (int i = 0; i < cdef->enumList.count(); ++i) {
            EnumDef def = cdef->enumList.at(i);
            if (cdef->enumDeclarations.contains(def.name)) {
                enumList += def;
            }
            QByteArray alias = cdef->flagAliases.value(def.name);
            if (cdef->enumDeclarations.contains(alias)) {
                def.name = alias;
                enumList += def;
            }
        }
        cdef->enumList = enumList;
    }


    QByteArray qualifiedClassNameIdentifier = cdef->qualified;
    qualifiedClassNameIdentifier.replace(':', '_');

    int index = MetaObjectPrivateFieldCount;
    fprintf(out, "static const uint qt_meta_data_%s[] = {\n", qualifiedClassNameIdentifier.constData());
    fprintf(out, "\n // content:\n");
    fprintf(out, "    %4d,       // revision\n", int(QMetaObjectPrivate::OutputRevision));
    fprintf(out, "    %4d,       // classname\n", strreg(cdef->qualified));
    fprintf(out, "    %4d, %4d, // classinfo\n", cdef->classInfoList.count(), cdef->classInfoList.count() ? index : 0);
    index += cdef->classInfoList.count() * 2;

    int methodCount = cdef->signalList.count() + cdef->slotList.count() + cdef->methodList.count();
    fprintf(out, "    %4d, %4d, // methods\n", methodCount, methodCount ? index : 0);
    index += methodCount * 5;
    if (cdef->revisionedMethods)
        index += methodCount;
    fprintf(out, "    %4d, %4d, // properties\n", cdef->propertyList.count(), cdef->propertyList.count() ? index : 0);
    index += cdef->propertyList.count() * 3;
    if(cdef->notifyableProperties)
        index += cdef->propertyList.count();
    if (cdef->revisionedProperties)
        index += cdef->propertyList.count();
    fprintf(out, "    %4d, %4d, // enums/sets\n", cdef->enumList.count(), cdef->enumList.count() ? index : 0);

    int enumsIndex = index;
    for (int i = 0; i < cdef->enumList.count(); ++i)
        index += 4 + (cdef->enumList.at(i).values.count() * 2);
    fprintf(out, "    %4d, %4d, // constructors\n", isConstructible ? cdef->constructorList.count() : 0,
            isConstructible ? index : 0);

    fprintf(out, "    %4d,       // flags\n", 0);
    fprintf(out, "    %4d,       // signalCount\n", cdef->signalList.count());


//
// Build classinfo array
//
    generateClassInfos();

//
// Build signals array first, otherwise the signal indices would be wrong
//
    generateFunctions(cdef->signalList, "signal", MethodSignal);

//
// Build slots array
//
    generateFunctions(cdef->slotList, "slot", MethodSlot);

//
// Build method array
//
    generateFunctions(cdef->methodList, "method", MethodMethod);

//
// Build method version arrays
//
    if (cdef->revisionedMethods) {
        generateFunctionRevisions(cdef->signalList, "signal");
        generateFunctionRevisions(cdef->slotList, "slot");
        generateFunctionRevisions(cdef->methodList, "method");
    }

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
        generateFunctions(cdef->constructorList, "constructor", MethodConstructor);

//
// Terminate data array
//
    fprintf(out, "\n       0        // eod\n};\n\n");

//
// Build stringdata array
//
    fprintf(out, "static const char qt_meta_stringdata_%s[] = {\n", qualifiedClassNameIdentifier.constData());
    fprintf(out, "    \"");
    int col = 0;
    int len = 0;
    for (int i = 0; i < strings.size(); ++i) {
        QByteArray s = strings.at(i);
        len = s.length();
        if (col && col + len >= 72) {
            fprintf(out, "\"\n    \"");
            col = 0;
        } else if (len && s.at(0) >= '0' && s.at(0) <= '9') {
            fprintf(out, "\"\"");
            len += 2;
        }
        int idx = 0;
        while (idx < s.length()) {
            if (idx > 0) {
                col = 0;
                fprintf(out, "\"\n    \"");
            }
            int spanLen = qMin(70, s.length() - idx);
            // don't cut escape sequences at the end of a line
            int backSlashPos = s.lastIndexOf('\\', idx + spanLen - 1);
            if (backSlashPos >= idx) {
                int escapeLen = lengthOfEscapeSequence(s, backSlashPos);
                spanLen = qBound(spanLen, backSlashPos + escapeLen - idx, s.length() - idx);
            }
            fwrite(s.constData() + idx, 1, spanLen, out);
            idx += spanLen;
            col += spanLen;
        }

        fputs("\\0", out);
        col += len + 2;
    }
    fprintf(out, "\"\n};\n\n");

//
// Generate internal qt_static_metacall() function
//
    if (cdef->hasQObject && !isQt)
        generateStaticMetacall();

//
// Build extra array
//
    QList<QByteArray> extraList;
    for (int i = 0; i < cdef->propertyList.count(); ++i) {
        const PropertyDef &p = cdef->propertyList.at(i);
        if (!isVariantType(p.type) && !metaTypes.contains(p.type) && !p.type.contains('*') &&
                !p.type.contains('<') && !p.type.contains('>')) {
            int s = p.type.lastIndexOf("::");
            if (s > 0) {
                QByteArray scope = p.type.left(s);
                if (scope != "Qt" && scope != cdef->classname && !extraList.contains(scope))
                    extraList += scope;
            }
        }
    }

    // QTBUG-20639 - Accept non-local enums for QML signal/slot parameters.
    // Look for any scoped enum declarations, and add those to the list
    // of extra/related metaobjects for this object.
    QList<QByteArray> enumKeys = cdef->enumDeclarations.keys();
    for (int i = 0; i < enumKeys.count(); ++i) {
        const QByteArray &enumKey = enumKeys[i];
        int s = enumKey.lastIndexOf("::");
        if (s > 0) {
            QByteArray scope = enumKey.left(s);
            if (scope != "Qt" && scope != cdef->classname && !extraList.contains(scope))
                extraList += scope;
        }
    }

    if (!extraList.isEmpty()) {
        fprintf(out, "static const QMetaObject *qt_meta_extradata_%s[] = {\n    ", qualifiedClassNameIdentifier.constData());
        for (int i = 0; i < extraList.count(); ++i) {
            fprintf(out, "    &%s::staticMetaObject,\n", extraList.at(i).constData());
        }
        fprintf(out, "    0\n};\n\n");
    }

    bool hasExtraData = (cdef->hasQObject && !isQt) || !extraList.isEmpty();
    if (hasExtraData) {
        fprintf(out, "const QMetaObjectExtraData %s::staticMetaObjectExtraData = {\n    ",
                cdef->qualified.constData());
        if (extraList.isEmpty())
            fprintf(out, "0, ");
        else
            fprintf(out, "qt_meta_extradata_%s, ", qualifiedClassNameIdentifier.constData());

        if (cdef->hasQObject && !isQt)
            fprintf(out, " qt_static_metacall");
        else
            fprintf(out, " 0");
        fprintf(out, " \n};\n\n");
    }

//
// Finally create and initialize the static meta object
//
    if (isQt)
        fprintf(out, "const QMetaObject QObject::staticQtMetaObject = {\n");
    else
        fprintf(out, "const QMetaObject %s::staticMetaObject = {\n", cdef->qualified.constData());

    if (isQObject)
        fprintf(out, "    { 0, ");
    else if (cdef->superclassList.size())
        fprintf(out, "    { &%s::staticMetaObject, ", purestSuperClass.constData());
    else
        fprintf(out, "    { 0, ");
    fprintf(out, "qt_meta_stringdata_%s,\n      qt_meta_data_%s, ",
             qualifiedClassNameIdentifier.constData(), qualifiedClassNameIdentifier.constData());
    if (!hasExtraData)
        fprintf(out, "0 }\n");
    else
        fprintf(out, "&staticMetaObjectExtraData }\n");
    fprintf(out, "};\n");

    if(isQt)
        return;


    if (!cdef->hasQObject)
        return;

    fprintf(out, "\nconst QMetaObject *%s::metaObject() const\n{\n    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;\n}\n",
            cdef->qualified.constData());

//
// Generate smart cast function
//
    fprintf(out, "\nvoid *%s::qt_metacast(const char *_clname)\n{\n", cdef->qualified.constData());
    fprintf(out, "    if (!_clname) return 0;\n");
    fprintf(out, "    if (!strcmp(_clname, qt_meta_stringdata_%s))\n"
                  "        return static_cast<void*>(const_cast< %s*>(this));\n",
            qualifiedClassNameIdentifier.constData(), cdef->classname.constData());
    for (int i = 1; i < cdef->superclassList.size(); ++i) { // for all superclasses but the first one
        if (cdef->superclassList.at(i).second == FunctionDef::Private)
            continue;
        const char *cname = cdef->superclassList.at(i).first.constData();
        fprintf(out, "    if (!strcmp(_clname, \"%s\"))\n        return static_cast< %s*>(const_cast< %s*>(this));\n",
                cname, cname, cdef->classname.constData());
    }
    for (int i = 0; i < cdef->interfaceList.size(); ++i) {
        const QList<ClassDef::Interface> &iface = cdef->interfaceList.at(i);
        for (int j = 0; j < iface.size(); ++j) {
            fprintf(out, "    if (!strcmp(_clname, %s))\n        return ", iface.at(j).interfaceId.constData());
            for (int k = j; k >= 0; --k)
                fprintf(out, "static_cast< %s*>(", iface.at(k).className.constData());
            fprintf(out, "const_cast< %s*>(this)%s;\n",
                    cdef->classname.constData(), QByteArray(j+1, ')').constData());
        }
    }
    if (!purestSuperClass.isEmpty() && !isQObject) {
        QByteArray superClass = purestSuperClass;
        // workaround for VC6
        if (superClass.contains("::")) {
            fprintf(out, "    typedef %s QMocSuperClass;\n", superClass.constData());
            superClass = "QMocSuperClass";
        }
        fprintf(out, "    return %s::qt_metacast(_clname);\n", superClass.constData());
    } else {
        fprintf(out, "    return 0;\n");
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
}


void Generator::generateClassInfos()
{
    if (cdef->classInfoList.isEmpty())
        return;

    fprintf(out, "\n // classinfo: key, value\n");

    for (int i = 0; i < cdef->classInfoList.size(); ++i) {
        const ClassInfoDef &c = cdef->classInfoList.at(i);
        fprintf(out, "    %4d, %4d,\n", strreg(c.name), strreg(c.value));
    }
}

void Generator::generateFunctions(const QList<FunctionDef>& list, const char *functype, int type)
{
    if (list.isEmpty())
        return;
    fprintf(out, "\n // %ss: signature, parameters, type, tag, flags\n", functype);

    for (int i = 0; i < list.count(); ++i) {
        const FunctionDef &f = list.at(i);

        QByteArray sig = f.name + '(';
        QByteArray arguments;

        for (int j = 0; j < f.arguments.count(); ++j) {
            const ArgumentDef &a = f.arguments.at(j);
            if (j) {
                sig += ",";
                arguments += ",";
            }
            sig += a.normalizedType;
            arguments += a.name;
        }
        sig += ')';

        unsigned char flags = type;
        if (f.access == FunctionDef::Private)
            flags |= AccessPrivate;
        else if (f.access == FunctionDef::Public)
            flags |= AccessPublic;
        else if (f.access == FunctionDef::Protected)
            flags |= AccessProtected;
        if (f.access == FunctionDef::Private)
            flags |= AccessPrivate;
        else if (f.access == FunctionDef::Public)
            flags |= AccessPublic;
        else if (f.access == FunctionDef::Protected)
            flags |= AccessProtected;
        if (f.isCompat)
            flags |= MethodCompatibility;
        if (f.wasCloned)
            flags |= MethodCloned;
        if (f.isScriptable)
            flags |= MethodScriptable;
        if (f.revision > 0)
            flags |= MethodRevisioned;
        fprintf(out, "    %4d, %4d, %4d, %4d, 0x%02x,\n", strreg(sig),
                strreg(arguments), strreg(f.normalizedType), strreg(f.tag), flags);
    }
}

void Generator::generateFunctionRevisions(const QList<FunctionDef>& list, const char *functype)
{
    if (list.count())
        fprintf(out, "\n // %ss: revision\n", functype);
    for (int i = 0; i < list.count(); ++i) {
        const FunctionDef &f = list.at(i);
        fprintf(out, "    %4d,\n", f.revision);
    }
}

void Generator::generateProperties()
{
    //
    // Create meta data
    //

    if (cdef->propertyList.count())
        fprintf(out, "\n // properties: name, type, flags\n");
    for (int i = 0; i < cdef->propertyList.count(); ++i) {
        const PropertyDef &p = cdef->propertyList.at(i);
        uint flags = Invalid;
        if (!isVariantType(p.type)) {
            flags |= EnumOrFlag;
        } else if (!isQRealType(p.type)) {
            flags |= qvariant_nameToType(p.type) << 24;
        }
        if (!p.read.isEmpty())
            flags |= Readable;
        if (!p.write.isEmpty()) {
            flags |= Writable;
            if (p.stdCppSet())
                flags |= StdCppSet;
        }
        if (!p.reset.isEmpty())
            flags |= Resettable;

//         if (p.override)
//             flags |= Override;

        if (p.designable.isEmpty())
            flags |= ResolveDesignable;
        else if (p.designable != "false")
            flags |= Designable;

        if (p.scriptable.isEmpty())
            flags |= ResolveScriptable;
        else if (p.scriptable != "false")
            flags |= Scriptable;

        if (p.stored.isEmpty())
            flags |= ResolveStored;
        else if (p.stored != "false")
            flags |= Stored;

        if (p.editable.isEmpty())
            flags |= ResolveEditable;
        else if (p.editable != "false")
            flags |= Editable;

        if (p.user.isEmpty())
            flags |= ResolveUser;
        else if (p.user != "false")
            flags |= User;

        if (p.notifyId != -1)
            flags |= Notify;

        if (p.revision > 0)
            flags |= Revisioned;

        if (p.constant)
            flags |= Constant;
        if (p.final)
            flags |= Final;

        fprintf(out, "    %4d, %4d, ",
                strreg(p.name),
                strreg(p.type));
        if (!(flags >> 24) && isQRealType(p.type))
            fprintf(out, "(QMetaType::QReal << 24) | ");
        fprintf(out, "0x%.8x,\n", flags);
    }

    if(cdef->notifyableProperties) {
        fprintf(out, "\n // properties: notify_signal_id\n");
        for (int i = 0; i < cdef->propertyList.count(); ++i) {
            const PropertyDef &p = cdef->propertyList.at(i);
            if(p.notifyId == -1)
                fprintf(out, "    %4d,\n",
                        0);
            else
                fprintf(out, "    %4d,\n",
                        p.notifyId);
        }
    }
    if (cdef->revisionedProperties) {
        fprintf(out, "\n // properties: revision\n");
        for (int i = 0; i < cdef->propertyList.count(); ++i) {
            const PropertyDef &p = cdef->propertyList.at(i);
            fprintf(out, "    %4d,\n", p.revision);
        }
    }
}

void Generator::generateEnums(int index)
{
    if (cdef->enumDeclarations.isEmpty())
        return;

    fprintf(out, "\n // enums: name, flags, count, data\n");
    index += 4 * cdef->enumList.count();
    int i;
    for (i = 0; i < cdef->enumList.count(); ++i) {
        const EnumDef &e = cdef->enumList.at(i);
        fprintf(out, "    %4d, 0x%.1x, %4d, %4d,\n",
                 strreg(e.name),
                 cdef->enumDeclarations.value(e.name) ? 1 : 0,
                 e.values.count(),
                 index);
        index += e.values.count() * 2;
    }

    fprintf(out, "\n // enum data: key, value\n");
    for (i = 0; i < cdef->enumList.count(); ++i) {
        const EnumDef &e = cdef->enumList.at(i);
        for (int j = 0; j < e.values.count(); ++j) {
            const QByteArray &val = e.values.at(j);
            QByteArray code = cdef->qualified.constData();
            if (e.isEnumClass)
                code += "::" + e.name;
            code += "::" + val;
            fprintf(out, "    %4d, uint(%s),\n",
                    strreg(val), code.constData());
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
        // workaround for VC6
        if (superClass.contains("::")) {
            fprintf(out, "    typedef %s QMocSuperClass;\n", superClass.constData());
            superClass = "QMocSuperClass";
        }
        fprintf(out, "    _id = %s::qt_metacall(_c, _id, _a);\n", superClass.constData());
    }

    fprintf(out, "    if (_id < 0)\n        return _id;\n");
    fprintf(out, "    ");

    bool needElse = false;
    QList<FunctionDef> methodList;
    methodList += cdef->signalList;
    methodList += cdef->slotList;
    methodList += cdef->methodList;

    if (methodList.size()) {
        needElse = true;
        fprintf(out, "if (_c == QMetaObject::InvokeMetaMethod) {\n");
        fprintf(out, "        if (_id < %d)\n", methodList.size());
        fprintf(out, "            qt_static_metacall(this, _c, _id, _a);\n");
        fprintf(out, "        _id -= %d;\n    }", methodList.size());
    }

    if (cdef->propertyList.size()) {
        bool needGet = false;
        bool needTempVarForGet = false;
        bool needSet = false;
        bool needReset = false;
        bool needDesignable = false;
        bool needScriptable = false;
        bool needStored = false;
        bool needEditable = false;
        bool needUser = false;
        for (int i = 0; i < cdef->propertyList.size(); ++i) {
            const PropertyDef &p = cdef->propertyList.at(i);
            needGet |= !p.read.isEmpty();
            if (!p.read.isEmpty())
                needTempVarForGet |= (p.gspec != PropertyDef::PointerSpec
                                      && p.gspec != PropertyDef::ReferenceSpec);

            needSet |= !p.write.isEmpty();
            needReset |= !p.reset.isEmpty();
            needDesignable |= p.designable.endsWith(')');
            needScriptable |= p.scriptable.endsWith(')');
            needStored |= p.stored.endsWith(')');
            needEditable |= p.editable.endsWith(')');
            needUser |= p.user.endsWith(')');
        }
        fprintf(out, "\n#ifndef QT_NO_PROPERTIES\n     ");

        if (needElse)
            fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::ReadProperty) {\n");
        if (needGet) {
            if (needTempVarForGet)
                fprintf(out, "        void *_v = _a[0];\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (p.read.isEmpty())
                    continue;
                QByteArray prefix;
                if (p.inPrivateClass.size()) {
                    prefix = p.inPrivateClass;
                    prefix.append("->");
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
                else
                    fprintf(out, "        case %d: *reinterpret_cast< %s*>(_v) = %s%s(); break;\n",
                            propindex, p.type.constData(), prefix.constData(), p.read.constData());
            }
            fprintf(out, "        }\n");
        }

        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::WriteProperty) {\n");

        if (needSet) {
            fprintf(out, "        void *_v = _a[0];\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (p.write.isEmpty())
                    continue;
                QByteArray prefix;
                if (p.inPrivateClass.size()) {
                    prefix = p.inPrivateClass;
                    prefix.append("->");
                }
                if (cdef->enumDeclarations.value(p.type, false)) {
                    fprintf(out, "        case %d: %s%s(QFlag(*reinterpret_cast<int*>(_v))); break;\n",
                            propindex, prefix.constData(), p.write.constData());
                } else {
                    fprintf(out, "        case %d: %s%s(*reinterpret_cast< %s*>(_v)); break;\n",
                            propindex, prefix.constData(), p.write.constData(), p.type.constData());
                }
            }
            fprintf(out, "        }\n");
        }

        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::ResetProperty) {\n");
        if (needReset) {
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.reset.endsWith(')'))
                    continue;
                QByteArray prefix;
                if (p.inPrivateClass.size()) {
                    prefix = p.inPrivateClass;
                    prefix.append("->");
                }
                fprintf(out, "        case %d: %s%s; break;\n",
                        propindex, prefix.constData(), p.reset.constData());
            }
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyDesignable) {\n");
        if (needDesignable) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.designable.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.designable.constData());
            }
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyScriptable) {\n");
        if (needScriptable) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.scriptable.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.scriptable.constData());
            }
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyStored) {\n");
        if (needStored) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.stored.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.stored.constData());
            }
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyEditable) {\n");
        if (needEditable) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.editable.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.editable.constData());
            }
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());


        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyUser) {\n");
        if (needUser) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.user.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.user.constData());
            }
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());


        fprintf(out, "\n#endif // QT_NO_PROPERTIES");
    }
    if (methodList.size() || cdef->signalList.size() || cdef->propertyList.size())
        fprintf(out, "\n    ");
    fprintf(out,"return _id;\n}\n");
}

void Generator::generateStaticMetacall()
{
    fprintf(out, "void %s::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)\n{\n",
            cdef->qualified.constData());

    bool needElse = false;
    bool isUsed_a = false;

    if (!cdef->constructorList.isEmpty()) {
        fprintf(out, "    if (_c == QMetaObject::CreateInstance) {\n");
        fprintf(out, "        switch (_id) {\n");
        for (int ctorindex = 0; ctorindex < cdef->constructorList.count(); ++ctorindex) {
            fprintf(out, "        case %d: { %s *_r = new %s(", ctorindex,
                    cdef->classname.constData(), cdef->classname.constData());
            const FunctionDef &f = cdef->constructorList.at(ctorindex);
            int offset = 1;
            for (int j = 0; j < f.arguments.count(); ++j) {
                const ArgumentDef &a = f.arguments.at(j);
                if (j)
                    fprintf(out, ",");
                fprintf(out, "(*reinterpret_cast< %s>(_a[%d]))", a.typeNameForCast.constData(), offset++);
            }
            fprintf(out, ");\n");
            fprintf(out, "            if (_a[0]) *reinterpret_cast<QObject**>(_a[0]) = _r; } break;\n");
        }
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
#ifndef QT_NO_DEBUG
        fprintf(out, "        Q_ASSERT(staticMetaObject.cast(_o));\n");
#endif
        fprintf(out, "        %s *_t = static_cast<%s *>(_o);\n", cdef->classname.constData(), cdef->classname.constData());
        fprintf(out, "        switch (_id) {\n");
        for (int methodindex = 0; methodindex < methodList.size(); ++methodindex) {
            const FunctionDef &f = methodList.at(methodindex);
            fprintf(out, "        case %d: ", methodindex);
            if (f.normalizedType.size())
                fprintf(out, "{ %s _r = ", noRef(f.normalizedType).constData());
            fprintf(out, "_t->");
            if (f.inPrivateClass.size())
                fprintf(out, "%s->", f.inPrivateClass.constData());
            fprintf(out, "%s(", f.name.constData());
            int offset = 1;
            for (int j = 0; j < f.arguments.count(); ++j) {
                const ArgumentDef &a = f.arguments.at(j);
                if (j)
                    fprintf(out, ",");
                fprintf(out, "(*reinterpret_cast< %s>(_a[%d]))",a.typeNameForCast.constData(), offset++);
                isUsed_a = true;
            }
            fprintf(out, ");");
            if (f.normalizedType.size()) {
                fprintf(out, "\n            if (_a[0]) *reinterpret_cast< %s*>(_a[0]) = _r; } ",
                        noRef(f.normalizedType).constData());
                isUsed_a = true;
            }
            fprintf(out, " break;\n");
        }
        fprintf(out, "        default: ;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }");
        needElse = true;
    }
    if (!cdef->signalList.isEmpty()) {
        Q_ASSERT(needElse); // if there is signal, there was method.
        fprintf(out, " else if (_c == QMetaObject::IndexOfMethod) {\n");
        fprintf(out, "        int *result = reinterpret_cast<int *>(_a[0]);\n");
        fprintf(out, "        void **func = reinterpret_cast<void **>(_a[1]);\n");
        bool anythingUsed = false;
        for (int methodindex = 0; methodindex < cdef->signalList.size(); ++methodindex) {
            const FunctionDef &f = cdef->signalList.at(methodindex);
            if (f.wasCloned || !f.inPrivateClass.isEmpty() || f.isStatic)
                continue;
            anythingUsed = true;
            fprintf(out, "        {\n");
            fprintf(out, "            typedef %s (%s::*_t)(",f.type.rawName.constData() , cdef->classname.constData());
            for (int j = 0; j < f.arguments.count(); ++j) {
                const ArgumentDef &a = f.arguments.at(j);
                if (j)
                    fprintf(out, ", ");
                fprintf(out, "%s", QByteArray(a.type.name + ' ' + a.rightType).constData());
            }
            if (f.isConst)
                fprintf(out, ") const;\n");
            else
                fprintf(out, ");\n");
            fprintf(out, "            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&%s::%s)) {\n",
                    cdef->classname.constData(), f.name.constData());
            fprintf(out, "                *result = %d;\n", methodindex);
            fprintf(out, "            }\n        }\n");
        }
        if (!anythingUsed)
            fprintf(out, "        Q_UNUSED(result);\n        Q_UNUSED(func);\n");
        fprintf(out, "    }");
        needElse = true;
    }

    if (needElse)
        fprintf(out, "\n");

    if (methodList.isEmpty()) {
        fprintf(out, "    Q_UNUSED(_o);\n");
        if (cdef->constructorList.isEmpty()) {
            fprintf(out, "    Q_UNUSED(_id);\n");
            fprintf(out, "    Q_UNUSED(_c);\n");
        }
    }
    if (!isUsed_a)
        fprintf(out, "    Q_UNUSED(_a);\n");

    fprintf(out, "}\n\n");
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
        thisPtr = "const_cast< ";
        thisPtr += cdef->qualified;
        thisPtr += " *>(this)";
        constQualifier = "const";
    }

    if (def->arguments.isEmpty() && def->normalizedType.isEmpty()) {
        fprintf(out, ")%s\n{\n"
                "    QMetaObject::activate(%s, &staticMetaObject, %d, 0);\n"
                "}\n", constQualifier, thisPtr.constData(), index);
        return;
    }

    int offset = 1;
    for (int j = 0; j < def->arguments.count(); ++j) {
        const ArgumentDef &a = def->arguments.at(j);
        if (j)
            fprintf(out, ", ");
        fprintf(out, "%s _t%d%s", a.type.name.constData(), offset++, a.rightType.constData());
    }
    fprintf(out, ")%s\n{\n", constQualifier);
    if (def->type.name.size() && def->normalizedType.size()) {
        QByteArray returnType = noRef(def->normalizedType);
        if (returnType.endsWith('*')) {
            fprintf(out, "    %s _t0 = 0;\n", returnType.constData());
        } else {
            fprintf(out, "    %s _t0 = %s();\n", returnType.constData(), returnType.constData());
        }
    }

    fprintf(out, "    void *_a[] = { ");
    if (def->normalizedType.isEmpty()) {
        fprintf(out, "0");
    } else {
        if (def->returnTypeIsVolatile)
             fprintf(out, "const_cast<void*>(reinterpret_cast<const volatile void*>(&_t0))");
        else
             fprintf(out, "const_cast<void*>(reinterpret_cast<const void*>(&_t0))");
    }
    int i;
    for (i = 1; i < offset; ++i)
        if (def->arguments.at(i - 1).type.isVolatile)
            fprintf(out, ", const_cast<void*>(reinterpret_cast<const volatile void*>(&_t%d))", i);
        else
            fprintf(out, ", const_cast<void*>(reinterpret_cast<const void*>(&_t%d))", i);
    fprintf(out, " };\n");
    fprintf(out, "    QMetaObject::activate(%s, &staticMetaObject, %d, _a);\n", thisPtr.constData(), index);
    if (def->normalizedType.size())
        fprintf(out, "    return _t0;\n");
    fprintf(out, "}\n");
}

static void writePluginMetaData(FILE *out, const QJsonObject &data)
{
    const QJsonDocument doc(data);

    fputs("\nQT_PLUGIN_METADATA_SECTION\n"
          "static const unsigned char qt_pluginMetaData[] = {\n"
          "    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', ' ',\n   ", out);
#if 0
    fprintf(out, "\"%s\";\n", doc.toJson().constData());
#else
    const QByteArray binary = doc.toBinaryData();
    const int last = binary.size() - 1;
    for (int i = 0; i < last; ++i) {
        fprintf(out, " 0x%02x,", (uchar)binary.at(i));
        if (!((i + 1) % 8))
            fputs("\n   ", out);
    }
    fprintf(out, " 0x%02x\n};\n", (uchar)binary.at(last));
#endif
}

void Generator::generatePluginMetaData()
{
    if (cdef->pluginData.iid.isEmpty())
        return;

    // Write plugin meta data #ifdefed QT_NO_DEBUG with debug=false,
    // true, respectively.

    QJsonObject data;
    const QString debugKey = QStringLiteral("debug");
    data.insert(QStringLiteral("IID"), QLatin1String(cdef->pluginData.iid.constData()));
    data.insert(QStringLiteral("className"), QLatin1String(cdef->classname.constData()));
    data.insert(QStringLiteral("version"), (int)QT_VERSION);
    data.insert(debugKey, QJsonValue(false));
    data.insert(QStringLiteral("MetaData"), cdef->pluginData.metaData.object());

    fputs("\nQT_PLUGIN_METADATA_SECTION const uint qt_section_alignment_dummy = 42;\n\n"
          "#ifdef QT_NO_DEBUG\n", out);
    writePluginMetaData(out, data);

    fputs("\n#else // QT_NO_DEBUG\n", out);

    data.remove(debugKey);
    data.insert(debugKey, QJsonValue(true));
    writePluginMetaData(out, data);

    fputs("#endif // QT_NO_DEBUG\n\n", out);

    // 'Use' all namespaces.
    int pos = cdef->qualified.indexOf("::");
    for ( ; pos != -1 ; pos = cdef->qualified.indexOf("::", pos + 2) )
        fprintf(out, "using namespace %s;\n", cdef->qualified.left(pos).constData());
    fprintf(out, "QT_MOC_EXPORT_PLUGIN(%s)\n\n", cdef->classname.constData());
}

QT_END_NAMESPACE
