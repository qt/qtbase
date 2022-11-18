// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2014 Keith Gardner <kreios4004@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qversionnumber.h>
#include <QtCore/qhash.h>
#include <QtCore/private/qlocale_tools_p.h>
#include <QtCore/qcollator.h>

#ifndef QT_NO_DATASTREAM
#  include <QtCore/qdatastream.h>
#endif

#ifndef QT_NO_DEBUG_STREAM
#  include <QtCore/qdebug.h>
#endif

#include <algorithm>
#include <limits>

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QVersionNumber)
QT_IMPL_METATYPE_EXTERN(QTypeRevision)

/*!
    \class QVersionNumber
    \inmodule QtCore
    \since 5.6
    \brief The QVersionNumber class contains a version number with an arbitrary
           number of segments.

    \snippet qversionnumber/main.cpp 0
*/

/*!
    \fn QVersionNumber::QVersionNumber()

    Produces a null version.

    \sa isNull()
*/

/*!
    \fn QVersionNumber::QVersionNumber(int maj)

    Constructs a QVersionNumber consisting of just the major version number \a maj.
*/

/*!
    \fn QVersionNumber::QVersionNumber(int maj, int min)

    Constructs a QVersionNumber consisting of the major and minor
    version numbers \a maj and \a min, respectively.
*/

/*!
    \fn QVersionNumber::QVersionNumber(int maj, int min, int mic)

    Constructs a QVersionNumber consisting of the major, minor, and
    micro version numbers \a maj, \a min and \a mic, respectively.
*/

/*!
    \fn QVersionNumber::QVersionNumber(const QList<int> &seg)

    Constructs a version number from the list of numbers contained in \a seg.
*/

/*!
    \fn QVersionNumber::QVersionNumber(QList<int> &&seg)

    Move-constructs a version number from the list of numbers contained in \a seg.

    This constructor is only enabled if the compiler supports C++11 move semantics.
*/

/*!
    \fn QVersionNumber::QVersionNumber(std::initializer_list<int> args)

    Construct a version number from the std::initializer_list specified by
    \a args.

    This constructor is only enabled if the compiler supports C++11 initializer
    lists.
*/

/*!
    \fn template <qsizetype N> QVersionNumber::QVersionNumber(const QVarLengthArray<int, N> &seg)
    \since 6.4

    Constructs a version number from the list of numbers contained in \a seg.
*/

/*!
    \fn bool QVersionNumber::isNull() const

    Returns \c true if there are zero numerical segments, otherwise returns
    \c false.

    \sa segments()
*/

/*!
  \fn bool QVersionNumber::isNormalized() const

  Returns \c true if the version number does not contain any trailing zeros,
  otherwise returns \c false.

  \sa normalized()
*/

/*!
    \fn int QVersionNumber::majorVersion() const

    Returns the major version number, that is, the first segment.
    This function is equivalent to segmentAt(0). If this QVersionNumber object
    is null, this function returns 0.

    \sa isNull(), segmentAt()
*/

/*!
    \fn int QVersionNumber::minorVersion() const

    Returns the minor version number, that is, the second segment.
    This function is equivalent to segmentAt(1). If this QVersionNumber object
    does not contain a minor number, this function returns 0.

    \sa isNull(), segmentAt()
*/

/*!
    \fn int QVersionNumber::microVersion() const

    Returns the micro version number, that is, the third segment.
    This function is equivalent to segmentAt(2). If this QVersionNumber object
    does not contain a micro number, this function returns 0.

    \sa isNull(), segmentAt()
*/

/*!
    \fn QList<int> QVersionNumber::segments() const

    Returns all of the numerical segments.

    \sa majorVersion(), minorVersion(), microVersion()
*/
QList<int> QVersionNumber::segments() const
{
    if (m_segments.isUsingPointer())
        return *m_segments.pointer_segments;

    QList<int> result;
    result.resize(segmentCount());
    for (qsizetype i = 0; i < segmentCount(); ++i)
        result[i] = segmentAt(i);
    return result;
}

/*!
    \fn int QVersionNumber::segmentAt(qsizetype index) const

    Returns the segment value at \a index.  If the index does not exist,
    returns 0.

    \sa segments(), segmentCount()
*/

/*!
    \fn qsizetype QVersionNumber::segmentCount() const

    Returns the number of integers stored in segments().

    \sa segments()
*/

/*!
    \fn QVersionNumber QVersionNumber::normalized() const

    Returns an equivalent version number but with all trailing zeros removed.

    To check if two numbers are equivalent, use normalized() on both version
    numbers before performing the compare.

    \snippet qversionnumber/main.cpp 4
 */
QVersionNumber QVersionNumber::normalized() const
{
    qsizetype i;
    for (i = m_segments.size(); i; --i)
        if (m_segments.at(i - 1) != 0)
            break;

    QVersionNumber result(*this);
    result.m_segments.resize(i);
    return result;
}

/*!
    \fn bool QVersionNumber::isPrefixOf(const QVersionNumber &other) const

    Returns \c true if the current version number is contained in the \a other
    version number, otherwise returns \c false.

    \snippet qversionnumber/main.cpp 2

    \sa commonPrefix()
*/
bool QVersionNumber::isPrefixOf(const QVersionNumber &other) const noexcept
{
    if (segmentCount() > other.segmentCount())
        return false;
    for (qsizetype i = 0; i < segmentCount(); ++i) {
        if (segmentAt(i) != other.segmentAt(i))
            return false;
    }
    return true;
}

/*!
    \fn int QVersionNumber::compare(const QVersionNumber &v1,
                                    const QVersionNumber &v2)

    Compares \a v1 with \a v2 and returns an integer less than, equal to, or
    greater than zero, depending on whether \a v1 is less than, equal to, or
    greater than \a v2, respectively.

    Comparisons are performed by comparing the segments of \a v1 and \a v2
    starting at index 0 and working towards the end of the longer list.

    \snippet qversionnumber/main.cpp 1
*/
int QVersionNumber::compare(const QVersionNumber &v1, const QVersionNumber &v2) noexcept
{
    qsizetype commonlen;

    if (Q_LIKELY(!v1.m_segments.isUsingPointer() && !v2.m_segments.isUsingPointer())) {
        // we can't use memcmp because it interprets the data as unsigned bytes
        const qint8 *ptr1 = v1.m_segments.inline_segments + InlineSegmentStartIdx;
        const qint8 *ptr2 = v2.m_segments.inline_segments + InlineSegmentStartIdx;
        commonlen = qMin(v1.m_segments.size(),
                         v2.m_segments.size());
        for (qsizetype i = 0; i < commonlen; ++i)
            if (int x = ptr1[i] - ptr2[i])
                return x;
    } else {
        commonlen = qMin(v1.segmentCount(), v2.segmentCount());
        for (qsizetype i = 0; i < commonlen; ++i) {
            if (v1.segmentAt(i) != v2.segmentAt(i))
                return v1.segmentAt(i) - v2.segmentAt(i);
        }
    }

    // ran out of segments in v1 and/or v2 and need to check the first trailing
    // segment to finish the compare
    if (v1.segmentCount() > commonlen) {
        // v1 is longer
        if (v1.segmentAt(commonlen) != 0)
            return v1.segmentAt(commonlen);
        else
            return 1;
    } else if (v2.segmentCount() > commonlen) {
        // v2 is longer
        if (v2.segmentAt(commonlen) != 0)
            return -v2.segmentAt(commonlen);
        else
            return -1;
    }

    // the two version numbers are the same
    return 0;
}

/*!
    QVersionNumber QVersionNumber::commonPrefix(const QVersionNumber &v1,
                                                    const QVersionNumber &v2)

    Returns a version number that is a parent version of both \a v1 and \a v2.

    \sa isPrefixOf()
*/
QVersionNumber QVersionNumber::commonPrefix(const QVersionNumber &v1,
                                            const QVersionNumber &v2)
{
    qsizetype commonlen = qMin(v1.segmentCount(), v2.segmentCount());
    qsizetype i;
    for (i = 0; i < commonlen; ++i) {
        if (v1.segmentAt(i) != v2.segmentAt(i))
            break;
    }

    if (i == 0)
        return QVersionNumber();

    // try to use the one with inline segments, if there's one
    QVersionNumber result(!v1.m_segments.isUsingPointer() ? v1 : v2);
    result.m_segments.resize(i);
    return result;
}

/*!
    \fn bool QVersionNumber::operator<(const QVersionNumber &lhs, const QVersionNumber &rhs)

    Returns \c true if \a lhs is less than \a rhs; otherwise returns \c false.

    \sa QVersionNumber::compare()
*/

/*!
    \fn bool QVersionNumber::operator<=(const QVersionNumber &lhs, const QVersionNumber &rhs)

    Returns \c true if \a lhs is less than or equal to \a rhs; otherwise
    returns \c false.

    \sa QVersionNumber::compare()
*/

/*!
    \fn bool QVersionNumber::operator>(const QVersionNumber &lhs, const QVersionNumber &rhs)

    Returns \c true if \a lhs is greater than \a rhs; otherwise returns \c
    false.

    \sa QVersionNumber::compare()
*/

/*!
    \fn bool QVersionNumber::operator>=(const QVersionNumber &lhs, const QVersionNumber &rhs)

    Returns \c true if \a lhs is greater than or equal to \a rhs; otherwise
    returns \c false.

    \sa QVersionNumber::compare()
*/

/*!
    \fn bool QVersionNumber::operator==(const QVersionNumber &lhs, const QVersionNumber &rhs)

    Returns \c true if \a lhs is equal to \a rhs; otherwise returns \c false.

    \sa QVersionNumber::compare()
*/

/*!
    \fn bool QVersionNumber::operator!=(const QVersionNumber &lhs, const QVersionNumber &rhs)

    Returns \c true if \a lhs is not equal to \a rhs; otherwise returns
    \c false.

    \sa QVersionNumber::compare()
*/

/*!
    \fn QString QVersionNumber::toString() const

    Returns a string with all of the segments delimited by a period (\c{.}).

    \sa majorVersion(), minorVersion(), microVersion(), segments()
*/
QString QVersionNumber::toString() const
{
    QString version;
    version.reserve(qMax(segmentCount() * 2 - 1, 0));
    bool first = true;
    for (qsizetype i = 0; i < segmentCount(); ++i) {
        if (!first)
            version += u'.';
        version += QString::number(segmentAt(i));
        first = false;
    }
    return version;
}

/*!
    \fn QVersionNumber QVersionNumber::fromString(QAnyStringView string, qsizetype *suffixIndex)
    \since 6.4

    Constructs a QVersionNumber from a specially formatted \a string of
    non-negative decimal numbers delimited by a period (\c{.}).

    Once the numerical segments have been parsed, the remainder of the string
    is considered to be the suffix string.  The start index of that string will be
    stored in \a suffixIndex if it is not null.

    \snippet qversionnumber/main.cpp 3-latin1-1

    \note In versions prior to Qt 6.4, this function was overloaded for QString,
    QLatin1StringView and QStringView instead, and \a suffixIndex was an \c{int*}.

    \sa isNull()
*/

static QVersionNumber from_string(QLatin1StringView string, qsizetype *suffixIndex)
{
    // 32 should be more than enough, and, crucially, it means we're allocating
    // not more (and often less) often when compared with direct QList usage
    // for all possible segment counts (under the constraint that we don't want
    // to keep more capacity around for the lifetime of the resulting
    // QVersionNumber than required), esp. in the common case where the inline
    // storage can be used.
    QVarLengthArray<int, 32> seg;

    const char *start = string.begin();
    const char *lastGoodEnd = start;
    const char *endOfString = string.end();

    do {
        // parsing as unsigned so a minus sign is rejected
        auto [value, used] = qstrntoull(start, endOfString - start, 10);
        if (used <= 0 || value > qulonglong(std::numeric_limits<int>::max()))
            break;
        seg.append(int(value));
        start += used + 1;
        lastGoodEnd = start - 1;
    } while (start < endOfString && *lastGoodEnd == '.');

    if (suffixIndex)
        *suffixIndex = lastGoodEnd - string.begin();

    return QVersionNumber(seg);
}

static QVersionNumber from_string(q_no_char8_t::QUtf8StringView string, qsizetype *suffixIndex)
{
    return from_string(QLatin1StringView(string.data(), string.size()), suffixIndex);
}

// in qstring.cpp
extern void qt_to_latin1(uchar *dst, const char16_t *uc, qsizetype len);

static QVersionNumber from_string(QStringView string, qsizetype *suffixIndex)
{
    QVarLengthArray<char> copy;
    copy.resize(string.size());
    qt_to_latin1(reinterpret_cast<uchar*>(copy.data()), string.utf16(), string.size());
    return from_string(QLatin1StringView(copy.data(), copy.size()), suffixIndex);
}

QVersionNumber QVersionNumber::fromString(QAnyStringView string, qsizetype *suffixIndex)
{
    return string.visit([=] (auto string) { return from_string(string, suffixIndex); });
}

void QVersionNumber::SegmentStorage::setListData(const QList<int> &seg)
{
    pointer_segments = new QList<int>(seg);
}

void QVersionNumber::SegmentStorage::setListData(QList<int> &&seg)
{
    pointer_segments = new QList<int>(std::move(seg));
}

void QVersionNumber::SegmentStorage::setListData(const int *first, const int *last)
{
    pointer_segments = new QList<int>(first, last);
}

void QVersionNumber::SegmentStorage::resize(qsizetype len)
{
    if (isUsingPointer())
        pointer_segments->resize(len);
    else
        setInlineSize(len);
}

void QVersionNumber::SegmentStorage::setVector(int len, int maj, int min, int mic)
{
    pointer_segments = new QList<int>;
    pointer_segments->resize(len);
    pointer_segments->data()[0] = maj;
    if (len > 1) {
        pointer_segments->data()[1] = min;
        if (len > 2) {
            pointer_segments->data()[2] = mic;
        }
    }
}

#ifndef QT_NO_DATASTREAM
/*!
   \fn  QDataStream& operator<<(QDataStream &out,
                                const QVersionNumber &version)
   \relates QVersionNumber

   Writes the version number \a version to stream \a out.

   Note that this has nothing to do with QDataStream::version().
 */
QDataStream& operator<<(QDataStream &out, const QVersionNumber &version)
{
    out << version.segments();
    return out;
}

/*!
   \fn QDataStream& operator>>(QDataStream &in, QVersionNumber &version)
   \relates QVersionNumber

   Reads a version number from stream \a in and stores it in \a version.

   Note that this has nothing to do with QDataStream::version().
 */
QDataStream& operator>>(QDataStream &in, QVersionNumber &version)
{
    if (!version.m_segments.isUsingPointer())
        version.m_segments.pointer_segments = new QList<int>;
    in >> *version.m_segments.pointer_segments;
    return in;
}
#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QVersionNumber &version)
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote();
    debug << "QVersionNumber(" << version.toString() << ")";
    return debug;
}
#endif

/*!
    \relates QHash
    \since 5.6

    Returns the hash value for the \a key, using \a seed to seed the
    calculation.
*/
size_t qHash(const QVersionNumber &key, size_t seed)
{
    QtPrivate::QHashCombine hash;
    for (int i = 0; i < key.segmentCount(); ++i)
        seed = hash(seed, key.segmentAt(i));
    return seed;
}

/*!
    \class QTypeRevision
    \inmodule QtCore
    \since 6.0
    \brief The QTypeRevision class contains a lightweight representation of
           a version number with two 8-bit segments, major and minor, either
           of which can be unknown.

    Use this class to describe revisions of a type. Compatible revisions can be
    expressed as increments of the minor version. Breaking changes can be
    expressed as increments of the major version. The return values of
    \l QMetaMethod::revision() and \l QMetaProperty::revision() can be passed to
    \l QTypeRevision::fromEncodedVersion(). The resulting major and minor versions
    specify in which Qt versions the properties and methods were added.

    \sa QMetaMethod::revision(), QMetaProperty::revision()
*/

/*!
    \fn template<typename Integer> static bool QTypeRevision::isValidSegment(Integer segment)

    Returns true if the given number can be used as either major or minor
    version in a QTypeRevision. The valid range for \a segment is \c {>= 0} and \c {< 255}.
*/

/*!
    \fn QTypeRevision::QTypeRevision()

    Produces an invalid revision.

    \sa isValid()
*/

/*!
    \fn template <typename Major, typename Minor> static QTypeRevision QTypeRevision::fromVersion(Major majorVersion, Minor minorVersion)

    Produces a QTypeRevision from the given \a majorVersion and \a minorVersion,
    both of which need to be a valid segments.

    \sa isValidSegment()
*/

/*!
    \fn template <typename Major> static QTypeRevision QTypeRevision::fromMajorVersion(Major majorVersion)

    Produces a QTypeRevision from the given \a majorVersion with an invalid minor
    version. \a majorVersion needs to be a valid segment.

    \sa isValidSegment()
*/

/*!
    \fn template <typename Minor> static QTypeRevision QTypeRevision::fromMinorVersion(Minor minorVersion)

    Produces a QTypeRevision from the given \a minorVersion with an invalid major
    version. \a minorVersion needs to be a valid segment.

    \sa isValidSegment()
*/

/*!
    \fn template <typename Integer> static QTypeRevision QTypeRevision::fromEncodedVersion(Integer value)

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
    \fn template<typename Integer> Integer QTypeRevision::toEncodedVersion() const

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
    \relates QHash
    \since 6.0

    Returns the hash value for the \a key, using \a seed to seed the
    calculation.
*/
size_t qHash(const QTypeRevision &key, size_t seed)
{
    return qHash(key.toEncodedVersion<quint16>(), seed);
}

QT_END_NAMESPACE
