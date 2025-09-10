// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qtyperevision.h>
#include <QtCore/qhashfunctions.h>

#ifndef QT_NO_DATASTREAM
#  include <QtCore/qdatastream.h>
#endif

#ifndef QT_NO_DEBUG_STREAM
#  include <QtCore/qdebug.h>
#endif

#include <algorithm>
#include <limits>

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QTypeRevision)

/*!
    \class QTypeRevision
    \inmodule QtCore
    \since 6.0
    \brief The QTypeRevision class contains a lightweight representation of
           a version number with two 8-bit segments, major and minor, either
           of which can be unknown.
    \compares strong

    Use this class to describe revisions of a type. Compatible revisions can be
    expressed as increments of the minor version. Breaking changes can be
    expressed as increments of the major version. The return values of
    \l QMetaMethod::revision() and \l QMetaProperty::revision() can be passed to
    \l QTypeRevision::fromEncodedVersion(). The resulting major and minor versions
    specify in which Qt versions the properties and methods were added.

    \sa QMetaMethod::revision(), QMetaProperty::revision()
*/

/*!
    \fn template<typename Integer, QTypeRevision::if_valid_segment_type<Integer> = true> static bool QTypeRevision::isValidSegment(Integer segment)

    Returns true if the given number can be used as either major or minor
    version in a QTypeRevision. The valid range for \a segment is \c {>= 0} and \c {< 255}.
*/

/*!
    \fn QTypeRevision::QTypeRevision()

    Produces an invalid revision.

    \sa isValid()
*/

/*!
    \fn template<typename Major, typename Minor, QTypeRevision::if_valid_segment_type<Major> = true, QTypeRevision::if_valid_segment_type<Minor> = true> static QTypeRevision QTypeRevision::fromVersion(Major majorVersion, Minor minorVersion)

    Produces a QTypeRevision from the given \a majorVersion and \a minorVersion,
    both of which need to be a valid segments.

    \sa isValidSegment()
*/

/*!
    \fn template<typename Major, QTypeRevision::if_valid_segment_type<Major> = true> static QTypeRevision QTypeRevision::fromMajorVersion(Major majorVersion)

    Produces a QTypeRevision from the given \a majorVersion with an invalid minor
    version. \a majorVersion needs to be a valid segment.

    \sa isValidSegment()
*/

/*!
    \fn template<typename Minor, QTypeRevision::if_valid_segment_type<Minor> = true> static QTypeRevision QTypeRevision::fromMinorVersion(Minor minorVersion)

    Produces a QTypeRevision from the given \a minorVersion with an invalid major
    version. \a minorVersion needs to be a valid segment.

    \sa isValidSegment()
*/

/*!
    \fn template<typename Integer, QTypeRevision::if_valid_value_type<Integer> = true> static QTypeRevision QTypeRevision::fromEncodedVersion(Integer value)

    Produces a QTypeRevision from the given \a value. \a value encodes both the
    minor and major versions in the least significant and second least
    significant byte, respectively.

    \a value must not have any bits outside the least significant two bytes set.
    \c Integer needs to be at least 16 bits wide, and must not have a sign bit
    in the least significant 16 bits.

    \sa toEncodedVersion()
*/

/*!
    \fn static QTypeRevision QTypeRevision::zero()

    Produces a QTypeRevision with major and minor version \c{0}.
*/

/*!
    \fn bool QTypeRevision::hasMajorVersion() const

    Returns true if the major version is known, otherwise false.

    \sa majorVersion(), hasMinorVersion()
*/

/*!
    \fn quint8 QTypeRevision::majorVersion() const

    Returns the major version encoded in the revision.

    \sa hasMajorVersion(), minorVersion()
*/

/*!
    \fn bool QTypeRevision::hasMinorVersion() const

    Returns true if the minor version is known, otherwise false.

    \sa minorVersion(), hasMajorVersion()
*/

/*!
    \fn quint8 QTypeRevision::minorVersion() const

    Returns the minor version encoded in the revision.

    \sa hasMinorVersion(), majorVersion()
*/

/*!
    \fn bool QTypeRevision::isValid() const

    Returns true if the major version or the minor version is known,
    otherwise false.

    \sa hasMajorVersion(), hasMinorVersion()
*/

/*!
    \fn template<typename Integer, QTypeRevision::if_valid_value_type<Integer> = true> Integer QTypeRevision::toEncodedVersion() const

    Transforms the revision into an integer value, encoding the minor
    version into the least significant byte, and the major version into
    the second least significant byte.

    \c Integer needs to be at  least 16 bits wide, and must not have a sign bit
    in the least significant 16 bits.

    \sa fromEncodedVersion()
*/

#ifndef QT_NO_DATASTREAM
/*!
   \fn QDataStream& operator<<(QDataStream &out, const QTypeRevision &revision)
   \relates QTypeRevision
   \since 6.0

   Writes the revision \a revision to stream \a out.
 */
QDataStream &operator<<(QDataStream &out, const QTypeRevision &revision)
{
    return out << revision.toEncodedVersion<quint16>();
}

/*!
   \fn QDataStream& operator>>(QDataStream &in, QTypeRevision &revision)
   \relates QTypeRevision
   \since 6.0

   Reads a revision from stream \a in and stores it in \a revision.
 */
QDataStream &operator>>(QDataStream &in, QTypeRevision &revision)
{
    quint16 value;
    in >> value;
    revision = QTypeRevision::fromEncodedVersion(value);
    return in;
}
#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QTypeRevision &revision)
{
    QDebugStateSaver saver(debug);
    if (revision.hasMajorVersion()) {
        if (revision.hasMinorVersion())
            debug.nospace() << revision.majorVersion() << '.' << revision.minorVersion();
        else
            debug.nospace().noquote() << revision.majorVersion() << ".x";
    } else {
        if (revision.hasMinorVersion())
            debug << revision.minorVersion();
        else
            debug.noquote() << "invalid";
    }
    return debug;
}
#endif

/*!
    \qhashold{QHash}
    \since 6.0
*/
size_t qHash(const QTypeRevision &key, size_t seed)
{
    return qHash(key.toEncodedVersion<quint16>(), seed);
}

QT_END_NAMESPACE
