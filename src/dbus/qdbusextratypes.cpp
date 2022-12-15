// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbusextratypes.h"
#include "qdbusutil_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QDBusVariant)
QT_IMPL_METATYPE_EXTERN(QDBusObjectPath)
QT_IMPL_METATYPE_EXTERN(QDBusSignature)

void QDBusObjectPath::doCheck()
{
    if (!QDBusUtil::isValidObjectPath(m_path)) {
        qWarning("QDBusObjectPath: invalid path \"%s\"", qPrintable(m_path));
        m_path.clear();
    }
}

void QDBusSignature::doCheck()
{
    if (!QDBusUtil::isValidSignature(m_signature)) {
        qWarning("QDBusSignature: invalid signature \"%s\"", qPrintable(m_signature));
        m_signature.clear();
    }
}

/*!
    \class QDBusVariant
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusVariant class enables the programmer to identify
    the variant type provided by the D-Bus typesystem.

    A D-Bus function that takes an integer, a D-Bus variant and a string as parameters
    can be called with the following argument list (see QDBusMessage::setArguments()):

    \snippet qdbusextratypes/qdbusextratypes.cpp 0

    When a D-Bus function returns a D-Bus variant, it can be retrieved as follows:

    \snippet qdbusextratypes/qdbusextratypes.cpp 1

    The QVariant within a QDBusVariant is required to distinguish between a normal
    D-Bus value and a value within a D-Bus variant.

    \sa {The Qt D-Bus Type System}
*/

/*!
    \fn QDBusVariant::QDBusVariant()

    Constructs a new D-Bus variant.
*/

/*!
    \fn QDBusVariant::QDBusVariant(const QVariant &variant)

    Constructs a new D-Bus variant from the given Qt \a variant.

    \sa setVariant()
*/

/*!
    \fn QVariant QDBusVariant::variant() const

    Returns this D-Bus variant as a QVariant object.

    \sa setVariant()
*/

/*!
    \fn void QDBusVariant::setVariant(const QVariant &variant)

    Assigns the value of the given Qt \a variant to this D-Bus variant.

    \sa variant()
*/

/*!
    \class QDBusObjectPath
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusObjectPath class enables the programmer to
    identify the OBJECT_PATH type provided by the D-Bus typesystem.

    \sa {The Qt D-Bus Type System}
*/

/*!
    \fn QDBusObjectPath::QDBusObjectPath()

    Constructs a new object path.
*/

/*!
    \fn QDBusObjectPath::QDBusObjectPath(const char *path)

    Constructs a new object path from the given \a path.

    \sa setPath()
*/

/*!
    \fn QDBusObjectPath::QDBusObjectPath(QLatin1StringView path)

    Constructs a new object path from the Latin-1 string viewed by \a path.
*/

/*!
    \fn QDBusObjectPath::QDBusObjectPath(const QString &path)

    Constructs a new object path from the given \a path.
*/

/*!
    \fn QString QDBusObjectPath::path() const

    Returns this object path.

    \sa setPath()
*/

/*!
    \fn void QDBusObjectPath::setPath(const QString &path)

    Assigns the value of the given \a path to this object path.

    \sa path()
*/

/*!
    \since 5.14

    Implicit cast to QVariant. Equivalent to calling
    QVariant::fromValue() with this object as argument.
*/
QDBusObjectPath::operator QVariant() const { return QVariant::fromValue(*this); }

/*!
    \class QDBusSignature
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusSignature class enables the programmer to
    identify the SIGNATURE type provided by the D-Bus typesystem.

    \sa {The Qt D-Bus Type System}
*/

/*!
    \fn QDBusSignature::QDBusSignature()

    Constructs a new signature.

    \sa setSignature()
*/

/*!
    \fn QDBusSignature::QDBusSignature(const char *signature)

    Constructs a new signature from the given \a signature.
*/

/*!
    \fn QDBusSignature::QDBusSignature(QLatin1StringView signature)

    Constructs a new signature from the Latin-1 string viewed by \a signature.
*/

/*!
    \fn QDBusSignature::QDBusSignature(const QString &signature)

    Constructs a new signature from the given \a signature.
*/

/*!
    \fn QString QDBusSignature::signature() const

    Returns this signature.

    \sa setSignature()
*/

/*!
    \fn void QDBusSignature::setSignature(const QString &signature)

    Assigns the value of the given \a signature to this signature.
    \sa signature()
*/

/*!
    \fn void QDBusObjectPath::swap(QDBusObjectPath &other)

    Swaps this QDBusObjectPath instance with \a other.
*/

/*!
    \fn void QDBusSignature::swap(QDBusSignature &other)

    Swaps this QDBusSignature instance with \a other.
*/

/*!
    \fn void QDBusVariant::swap(QDBusVariant &other)

    Swaps this QDBusVariant instance with \a other.
*/

QT_END_NAMESPACE

#endif // QT_NO_DBUS
