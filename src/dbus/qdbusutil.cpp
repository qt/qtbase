// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qdbusutil_p.h"

#include "qdbus_symbols_p.h"

#include <QtCore/qlist.h>
#include <QtCore/qstringlist.h>
#include <private/qtools_p.h>

#include "qdbusargument.h"
#include "qdbusunixfiledescriptor.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtMiscUtils;

static inline bool isValidCharacterNoDash(QChar c)
{
    ushort u = c.unicode();
    return isAsciiLetterOrNumber(u) || (u == '_');
}

static inline bool isValidCharacter(QChar c)
{
    ushort u = c.unicode();
    return isAsciiLetterOrNumber(u)
            || (u == '_') || (u == '-');
}

static inline bool isValidNumber(QChar c)
{
    return (isAsciiDigit(c.toLatin1()));
}

#ifndef QT_BOOTSTRAPPED
static bool argToString(const QDBusArgument &arg, QString &out);

static bool variantToString(const QVariant &arg, QString &out)
{
    int argType = arg.metaType().id();

    if (argType == QMetaType::QStringList) {
        out += u'{';
        const QStringList list = arg.toStringList();
        for (const QString &item : list)
            out += u'\"' + item + "\", "_L1;
        if (!list.isEmpty())
            out.chop(2);
        out += u'}';
    } else if (argType == QMetaType::QByteArray) {
        out += u'{';
        QByteArray list = arg.toByteArray();
        for (int i = 0; i < list.size(); ++i) {
            out += QString::number(list.at(i));
            out += ", "_L1;
        }
        if (!list.isEmpty())
            out.chop(2);
        out += u'}';
    } else if (argType == QMetaType::QVariantList) {
        out += u'{';
        const QList<QVariant> list = arg.toList();
        for (const QVariant &item : list) {
            if (!variantToString(item, out))
                return false;
            out += ", "_L1;
        }
        if (!list.isEmpty())
            out.chop(2);
        out += u'}';
    } else if (argType == QMetaType::Char || argType == QMetaType::Short || argType == QMetaType::Int
               || argType == QMetaType::Long || argType == QMetaType::LongLong) {
        out += QString::number(arg.toLongLong());
    } else if (argType == QMetaType::UChar || argType == QMetaType::UShort || argType == QMetaType::UInt
               || argType == QMetaType::ULong || argType == QMetaType::ULongLong) {
        out += QString::number(arg.toULongLong());
    } else if (argType == QMetaType::Double) {
        out += QString::number(arg.toDouble());
    } else if (argType == QMetaType::Bool) {
        out += arg.toBool() ? "true"_L1 : "false"_L1;
    } else if (argType == qMetaTypeId<QDBusArgument>()) {
        argToString(qvariant_cast<QDBusArgument>(arg), out);
    } else if (argType == qMetaTypeId<QDBusObjectPath>()) {
        const QString path = qvariant_cast<QDBusObjectPath>(arg).path();
        out += "[ObjectPath: "_L1;
        out += path;
        out += u']';
    } else if (argType == qMetaTypeId<QDBusSignature>()) {
        out += "[Signature: "_L1 + qvariant_cast<QDBusSignature>(arg).signature();
        out += u']';
    } else if (argType == qMetaTypeId<QDBusUnixFileDescriptor>()) {
        out += "[Unix FD: "_L1;
        out += qvariant_cast<QDBusUnixFileDescriptor>(arg).isValid() ? "valid"_L1 : "not valid"_L1;
        out += u']';
    } else if (argType == qMetaTypeId<QDBusVariant>()) {
        const QVariant v = qvariant_cast<QDBusVariant>(arg).variant();
        out += "[Variant"_L1;
        QMetaType vUserType = v.metaType();
        if (vUserType != QMetaType::fromType<QDBusVariant>()
                && vUserType != QMetaType::fromType<QDBusSignature>()
                && vUserType != QMetaType::fromType<QDBusObjectPath>()
                && vUserType != QMetaType::fromType<QDBusArgument>())
            out += u'(' + QLatin1StringView(v.typeName()) + u')';
        out += ": "_L1;
        if (!variantToString(v, out))
            return false;
        out += u']';
    } else if (arg.canConvert<QString>()) {
        out += u'\"' + arg.toString() + u'\"';
    } else {
        out += u'[';
        out += QLatin1StringView(arg.typeName());
        out += u']';
    }

    return true;
}

bool argToString(const QDBusArgument &busArg, QString &out)
{
    QString busSig = busArg.currentSignature();
    bool doIterate = false;
    QDBusArgument::ElementType elementType = busArg.currentType();

    if (elementType != QDBusArgument::BasicType && elementType != QDBusArgument::VariantType
            && elementType != QDBusArgument::MapEntryType)
        out += "[Argument: "_L1 + busSig + u' ';

    switch (elementType) {
        case QDBusArgument::BasicType:
        case QDBusArgument::VariantType:
            if (!variantToString(busArg.asVariant(), out))
                return false;
            break;
        case QDBusArgument::StructureType:
            busArg.beginStructure();
            doIterate = true;
            break;
        case QDBusArgument::ArrayType:
            busArg.beginArray();
            out += u'{';
            doIterate = true;
            break;
        case QDBusArgument::MapType:
            busArg.beginMap();
            out += u'{';
            doIterate = true;
            break;
        case QDBusArgument::MapEntryType:
            busArg.beginMapEntry();
            if (!variantToString(busArg.asVariant(), out))
                return false;
            out += " = "_L1;
            if (!argToString(busArg, out))
                return false;
            busArg.endMapEntry();
            break;
        case QDBusArgument::UnknownType:
        default:
            out += "<ERROR - Unknown Type>"_L1;
            return false;
    }
    if (doIterate && !busArg.atEnd()) {
        while (!busArg.atEnd()) {
            if (!argToString(busArg, out))
                return false;
            out += ", "_L1;
        }
        out.chop(2);
    }
    switch (elementType) {
        case QDBusArgument::BasicType:
        case QDBusArgument::VariantType:
        case QDBusArgument::UnknownType:
        case QDBusArgument::MapEntryType:
            // nothing to do
            break;
        case QDBusArgument::StructureType:
            busArg.endStructure();
            break;
        case QDBusArgument::ArrayType:
            out += u'}';
            busArg.endArray();
            break;
        case QDBusArgument::MapType:
            out += u'}';
            busArg.endMap();
            break;
    }

    if (elementType != QDBusArgument::BasicType && elementType != QDBusArgument::VariantType
            && elementType != QDBusArgument::MapEntryType)
        out += u']';

    return true;
}
#endif

//------- D-Bus Types --------
static const char oneLetterTypes[] = "vsogybnqiuxtdh";
static const char basicTypes[] =      "sogybnqiuxtdh";
static const char fixedTypes[] =         "ybnqiuxtdh";

/*
    D-Bus signature grammar (in ABNF), as of 0.42 (2023-08-21):
    https://dbus.freedesktop.org/doc/dbus-specification.html#type-system

    <signature>            = *<single-complete-type>
    <single-complete-type> = <basic-type> / <variant> / <struct> / <array>
    <fixed-type>           = "y" / "b" / "n" / "q" / "i" / "u" / "x" / "t" / "d" / "h"
    <variable-length-type> = "s" / "o" / "g"
    <basic-type>           = <variable-length-type> / <fixed-type>
    <variant>              = "v"
    <struct>               = "(" 1*<single-complete-type> ")"
    <array>                = "a" ( <single-complete-type> / <dict-entry> )
    <dict-entry>           = "{" <basic-type> <single-complete-type> "}"
*/

static bool isBasicType(int c)
{
    return c != DBUS_TYPE_INVALID && strchr(basicTypes, c) != nullptr;
}

static bool isFixedType(int c)
{
    return c != DBUS_TYPE_INVALID && strchr(fixedTypes, c) != nullptr;
}

// Returns a pointer to one-past-end of this type if it's valid;
// returns NULL if it isn't valid.
static const char *validateSingleType(const char *signature)
{
    char c = *signature;
    if (c == DBUS_TYPE_INVALID)
        return nullptr;

    // is it one of the one-letter types?
    if (strchr(oneLetterTypes, c) != nullptr)
        return signature + 1;

    // is it an array?
    if (c == DBUS_TYPE_ARRAY) {
        // then it's valid if the next type is valid
        // or if it's a dict-entry
        c = *++signature;
        if (c == DBUS_DICT_ENTRY_BEGIN_CHAR) {
            // beginning of a dictionary entry
            // a dictionary entry has a key which is of basic types
            // and a free value
            c = *++signature;
            if (!isBasicType(c))
                return nullptr;
            signature = validateSingleType(signature + 1);
            return signature && *signature == DBUS_DICT_ENTRY_END_CHAR ? signature + 1 : nullptr;
        }

        return validateSingleType(signature);
    }

    if (c == DBUS_STRUCT_BEGIN_CHAR) {
        // beginning of a struct
        ++signature;
        while (true) {
            signature = validateSingleType(signature);
            if (!signature)
                return nullptr;
            if (*signature == DBUS_STRUCT_END_CHAR)
                return signature + 1;
        }
    }

    // invalid/unknown type
    return nullptr;
}

/*!
    \namespace QDBusUtil
    \inmodule QtDBus
    \internal

    \brief The QDBusUtil namespace contains a few functions that are of general use when
    dealing with D-Bus strings.
*/
namespace QDBusUtil
{
    /*!
        \internal
        \since 4.5
        Dumps the contents of a Qt D-Bus argument from \a arg into a string.
    */
    QString argumentToString(const QVariant &arg)
    {
        QString out;

#ifndef QT_BOOTSTRAPPED
        variantToString(arg, out);
#else
        Q_UNUSED(arg);
#endif

        return out;
    }

    /*!
        \internal
        \fn bool isValidPartOfObjectPath(QStringView part)
        See isValidObjectPath
    */
    bool isValidPartOfObjectPath(QStringView part)
    {
        if (part.isEmpty())
            return false;       // can't be valid if it's empty

        const QChar *c = part.data();
        for (int i = 0; i < part.size(); ++i)
            if (!isValidCharacterNoDash(c[i]))
                return false;

        return true;
    }

    /*!
        \fn bool isValidInterfaceName(const QString &ifaceName)
        Returns \c true if this is \a ifaceName is a valid interface name.

        Valid interface names must:
        \list
          \li not be empty
          \li not exceed 255 characters in length
          \li be composed of dot-separated string components that contain only ASCII letters, digits
             and the underscore ("_") character
          \li contain at least two such components
        \endlist
    */
    bool isValidInterfaceName(const QString& ifaceName)
    {
        if (ifaceName.isEmpty() || ifaceName.size() > DBUS_MAXIMUM_NAME_LENGTH)
            return false;

        const auto parts = QStringView{ifaceName}.split(u'.');
        if (parts.size() < 2)
            return false;           // at least two parts

        for (auto part : parts)
            if (!isValidMemberName(part))
                return false;

        return true;
    }

    /*!
        \fn bool isValidUniqueConnectionName(QStringView connName)
        Returns \c true if \a connName is a valid unique connection name.

        Unique connection names start with a colon (":") and are followed by a list of dot-separated
        components composed of ASCII letters, digits, the hyphen or the underscore ("_") character.
    */
    bool isValidUniqueConnectionName(QStringView connName)
    {
        if (connName.isEmpty() || connName.size() > DBUS_MAXIMUM_NAME_LENGTH ||
            !connName.startsWith(u':'))
            return false;

        const auto parts = connName.mid(1).split(u'.');
        if (parts.size() < 1)
            return false;

        for (QStringView part : parts) {
            if (part.isEmpty())
                 return false;

            const QChar* c = part.data();
            for (int j = 0; j < part.size(); ++j)
                if (!isValidCharacter(c[j]))
                    return false;
        }

        return true;
    }

    /*!
        \fn bool isValidBusName(const QString &busName)
        Returns \c true if \a busName is a valid bus name.

        A valid bus name is either a valid unique connection name or follows the rules:
        \list
          \li is not empty
          \li does not exceed 255 characters in length
          \li be composed of dot-separated string components that contain only ASCII letters, digits,
             hyphens or underscores ("_"), but don't start with a digit
          \li contains at least two such elements
        \endlist

        \sa isValidUniqueConnectionName()
    */
    bool isValidBusName(const QString &busName)
    {
        if (busName.isEmpty() || busName.size() > DBUS_MAXIMUM_NAME_LENGTH)
            return false;

        if (busName.startsWith(u':'))
            return isValidUniqueConnectionName(busName);

        const auto parts = QStringView{busName}.split(u'.');
        if (parts.size() < 2)
            return false;
        for (QStringView part : parts) {
            if (part.isEmpty())
                return false;

            const QChar *c = part.data();
            if (isValidNumber(c[0]))
                return false;
            for (int j = 0; j < part.size(); ++j)
                if (!isValidCharacter(c[j]))
                    return false;
        }

        return true;
    }

    /*!
        \fn bool isValidMemberName(QStringView memberName)
        Returns \c true if \a memberName is a valid member name. A valid member name does not exceed
        255 characters in length, is not empty, is composed only of ASCII letters, digits and
        underscores, but does not start with a digit.
    */
    bool isValidMemberName(QStringView memberName)
    {
        if (memberName.isEmpty() || memberName.size() > DBUS_MAXIMUM_NAME_LENGTH)
            return false;

        const QChar* c = memberName.data();
        if (isValidNumber(c[0]))
            return false;
        for (int j = 0; j < memberName.size(); ++j)
            if (!isValidCharacterNoDash(c[j]))
                return false;
        return true;
    }

    /*!
        \fn bool isValidErrorName(const QString &errorName)
        Returns \c true if \a errorName is a valid error name. Valid error names are valid interface
        names and vice-versa, so this function is actually an alias for isValidInterfaceName.
    */
    bool isValidErrorName(const QString &errorName)
    {
        return isValidInterfaceName(errorName);
    }

    /*!
        \fn bool isValidObjectPath(const QString &path)
        Returns \c true if \a path is valid object path.

        Valid object paths follow the rules:
        \list
          \li start with the slash character ("/")
          \li do not end in a slash, unless the path is just the initial slash
          \li contain slash-separated parts, each of which is not empty, and composed
              only of ASCII letters, digits and underscores ("_").
        \endlist
    */
    bool isValidObjectPath(const QString &path)
    {
        if (path == "/"_L1)
            return true;

        if (!path.startsWith(u'/') || path.indexOf("//"_L1) != -1 ||
            path.endsWith(u'/'))
            return false;

        // it starts with /, so we skip the empty first part
        const auto parts = QStringView{path}.mid(1).split(u'/');
        for (QStringView part : parts)
            if (!isValidPartOfObjectPath(part))
                return false;

        return true;
    }

    /*!
        \fn bool isValidBasicType(int type)
        Returns \c true if \a c is a valid, basic D-Bus type.
     */
    bool isValidBasicType(int c)
    {
        return isBasicType(c);
    }

    /*!
        \fn bool isValidFixedType(int type)
        Returns \c true if \a c is a valid, fixed D-Bus type.
     */
    bool isValidFixedType(int c)
    {
        return isFixedType(c);
    }


    /*!
        \fn bool isValidSignature(const QString &signature)
        Returns \c true if \a signature is a valid D-Bus type signature for one or more types.
        This function returns \c true if it can all of \a signature into valid, individual types and no
        characters remain in \a signature.

        \sa isValidSingleSignature()
    */
    bool isValidSignature(const QString &signature)
    {
        QByteArray ba = signature.toLatin1();
        const char *data = ba.constBegin();
        const char *end = ba.constEnd();
        while (data != end) {
            data = validateSingleType(data);
            if (!data)
                return false;
        }
        return true;
    }

    /*!
        \fn bool isValidSingleSignature(const QString &signature)
        Returns \c true if \a signature is a valid D-Bus type signature for exactly one full type. This
        function tries to convert the type signature into a D-Bus type and, if it succeeds and no
        characters remain in the signature, it returns \c true.
    */
    bool isValidSingleSignature(const QString &signature)
    {
        QByteArray ba = signature.toLatin1();
        const char *data = validateSingleType(ba.constData());
        return data && *data == '\0';
    }

} // namespace QDBusUtil

QT_END_NAMESPACE

#endif // QT_NO_DBUS
