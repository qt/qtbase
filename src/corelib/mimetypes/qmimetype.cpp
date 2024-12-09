// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmimetype.h"

#include "qmimetype_p.h"
#include "qmimedatabase_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QLocale>
#include <QtCore/QHashFunctions>

#include <memory>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QMimeType
    \inmodule QtCore
    \ingroup shared
    \brief The QMimeType class describes types of file or data, represented by a MIME type string.

    \since 5.0
    \compares equality

    For instance a file named "readme.txt" has the MIME type "text/plain".
    The MIME type can be determined from the file name, or from the file
    contents, or from both. MIME type determination can also be done on
    buffers of data not coming from files.

    Determining the MIME type of a file can be useful to make sure your
    application supports it. It is also useful in file-manager-like applications
    or widgets, in order to display an appropriate \l {QMimeType::iconName}{icon} for the file, or even
    the descriptive \l {QMimeType::comment()}{comment} in detailed views.

    To check if a file has the expected MIME type, you should use inherits()
    rather than a simple string comparison based on the name(). This is because
    MIME types can inherit from each other: for instance a C source file is
    a specific type of plain text file, so text/x-csrc inherits text/plain.

    \sa QMimeDatabase, {MIME Type Browser}
 */

/*!
    \fn QMimeType &QMimeType::operator=(QMimeType &&other)

    Move-assigns \a other to this QMimeType instance.

    \since 5.2
*/

/*!
    \fn QMimeType::QMimeType();
    Constructs this QMimeType object initialized with default property values that indicate an invalid MIME type.
 */
QMimeType::QMimeType() :
        d(new QMimeTypePrivate())
{
}

/*!
    \fn QMimeType::QMimeType(const QMimeType &other);
    Constructs this QMimeType object as a copy of \a other.
 */
QMimeType::QMimeType(const QMimeType &other) :
        d(other.d)
{
}

/*!
    \fn QMimeType &QMimeType::operator=(const QMimeType &other);
    Assigns the data of \a other to this QMimeType object, and returns a reference to this object.
 */
QMimeType &QMimeType::operator=(const QMimeType &other)
{
    if (d != other.d)
        d = other.d;
    return *this;
}

/*!
    \fn QMimeType::QMimeType(const QMimeTypePrivate &dd);
    Assigns the data of the QMimeTypePrivate \a dd to this QMimeType object, and returns a reference to this object.
    \internal
 */
QMimeType::QMimeType(const QMimeTypePrivate &dd) :
        d(new QMimeTypePrivate(dd))
{
}

/*!
    \fn void QMimeType::swap(QMimeType &other);
    \memberswap{mime type}

    The swap() method helps with the implementation of assignment
    operators in an exception-safe way. For more information consult
    \l {http://en.wikibooks.org/wiki/More_C++_Idioms/Copy-and-swap}
    {More C++ Idioms - Copy-and-swap}.
 */

/*!
    \fn QMimeType::~QMimeType();
    Destroys the QMimeType object, and releases the d pointer.
 */
QMimeType::~QMimeType()
{
}

/*!
    \fn bool QMimeType::operator==(const QMimeType &lhs, const QMimeType &rhs);
    Returns \c true if \a lhs equals to the \a rhs QMimeType object, otherwise
    returns \c false.
    The name is the unique identifier for a mimetype, so two mimetypes with
    the same name, are equal.
 */
bool comparesEqual(const QMimeType &lhs, const QMimeType &rhs) noexcept
{
    return lhs.d == rhs.d || lhs.d->name == rhs.d->name;
}

/*!
    \since 5.6
    \qhashold{QMimeType}
 */
size_t qHash(const QMimeType &key, size_t seed) noexcept
{
    return qHash(key.d->name, seed);
}

/*!
    \fn bool QMimeType::operator!=(const QMimeType &lhs, const QMimeType &rhs);
    Returns \c true if QMimeType \a lhs is not equal to QMimeType \a rhs,
    otherwise returns \c false.
 */

/*!
    \property QMimeType::valid
    \brief \c true if the QMimeType object contains valid data, \c false otherwise

    A valid MIME type has a non-empty name().
    The invalid MIME type is the default-constructed QMimeType.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
bool QMimeType::isValid() const
{
    return !d->name.isEmpty();
}

/*!
    \property QMimeType::isDefault
    \brief \c true if this MIME type is the default MIME type which
    applies to all files: application/octet-stream.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
bool QMimeType::isDefault() const
{
    return d->name == QMimeDatabasePrivate::instance()->defaultMimeType();
}

/*!
    \property QMimeType::name
    \brief the name of the MIME type

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString QMimeType::name() const
{
    return d->name;
}

/*!
    \property QMimeType::comment
    \brief the description of the MIME type to be displayed on user interfaces

    Returns a description for a MIME type, localized to the user's
    current language settings.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString QMimeType::comment() const
{
    const auto isEnUs = [](QStringView lang) {
        // All synonyms of en_US, according to CLDR likely subtag rules:
        static constexpr QLatin1StringView usaIsh[] =
                { "en_Latn_US"_L1, "en_US"_L1, "en_Latn"_L1, "en"_L1 };
        return std::find(std::begin(usaIsh), std::end(usaIsh), lang) != std::end(usaIsh);
    };
    const auto localeComments = QMimeDatabasePrivate::instance()->localeComments(d->name);
    const QStringList languageList = QLocale().uiLanguages(QLocale::TagSeparator::Underscore);
    QString comment = localeComments.value(u"default"_s);

    for (const QString &language : std::as_const(languageList)) {
        const QString lang = language == "C"_L1 ? u"en_US"_s : language;
        QString translated = localeComments.value(lang);
        if (!translated.isEmpty())
            return translated;
        if (!comment.isEmpty() && isEnUs(lang))
            return comment; // The default entry is assumed to be in en-US
    }
    QString translated =
            QCoreApplication::translate("QMimeType", comment.toUtf8().constData());
    if (!translated.isEmpty())
        return translated;

    // Use the mimetype name as fallback
    return d->name;
}

/*!
    \property QMimeType::genericIconName
    \brief the file name of a generic icon that represents the MIME type

    This should be used if the icon returned by iconName() cannot be found on
    the system. It is used for categories of similar types (like spreadsheets
    or archives) that can use a common icon.
    The freedesktop.org Icon Naming Specification lists a set of such icon names.

    The icon name can be given to QIcon::fromTheme() in order to load the icon.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString QMimeType::genericIconName() const
{
    QString genericIconName = QMimeDatabasePrivate::instance()->genericIcon(d->name);
    if (genericIconName.isEmpty()) {
        // From the spec:
        // If the generic icon name is empty (not specified by the mimetype definition)
        // then the mimetype is used to generate the generic icon by using the top-level
        // media type (e.g.  "video" in "video/ogg") and appending "-x-generic"
        // (i.e. "video-x-generic" in the previous example).
        const QString group = name();
        QStringView groupRef(group);
        const qsizetype slashindex = groupRef.indexOf(u'/');
        if (slashindex != -1)
            groupRef = groupRef.left(slashindex);
        return groupRef + "-x-generic"_L1;
    }
    return genericIconName;
}

static QString make_default_icon_name_from_mimetype_name(QString iconName)
{
    const qsizetype slashindex = iconName.indexOf(u'/');
    if (slashindex != -1)
        iconName[slashindex] = u'-';
    return iconName;
}

/*!
    \property QMimeType::iconName
    \brief the file name of an icon image that represents the MIME type

    The icon name can be given to QIcon::fromTheme() in order to load the icon.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString QMimeType::iconName() const
{
    QString iconName = QMimeDatabasePrivate::instance()->icon(d->name);
    if (iconName.isEmpty()) {
        return make_default_icon_name_from_mimetype_name(name());
    }
    return iconName;
}

/*!
    \property QMimeType::globPatterns
    \brief the list of glob matching patterns

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QStringList QMimeType::globPatterns() const
{
    return QMimeDatabasePrivate::instance()->globPatterns(d->name);
}

/*!
    \property QMimeType::parentMimeTypes
    \brief the names of parent MIME types

    A type is a subclass of another type if any instance of the first type is
    also an instance of the second. For example, all image/svg+xml files are also
    text/xml, text/plain and application/octet-stream files. Subclassing is about
    the format, rather than the category of the data (for example, there is no
    'generic spreadsheet' class that all spreadsheets inherit from).
    Conversely, the parent mimetype of image/svg+xml is text/xml.

    A mimetype can have multiple parents. For instance application/x-perl
    has two parents: application/x-executable and text/plain. This makes
    it possible to both execute perl scripts, and to open them in text editors.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
*/
QStringList QMimeType::parentMimeTypes() const
{
    return QMimeDatabasePrivate::instance()->mimeParents(d->name);
}

static void collectParentMimeTypes(const QString &mime, QStringList &allParents)
{
    const QStringList parents = QMimeDatabasePrivate::instance()->mimeParents(mime);
    QStringList newParents;
    for (const QString &parent : parents) {
        // I would use QSet, but since order matters I better not
        if (!allParents.contains(parent)) {
            allParents.append(parent);
            newParents.append(parent);
        }
    }
    // We want a breadth-first search, so that the least-specific parent (octet-stream) is last
    // This means iterating twice, unfortunately.
    for (const QString &parent : newParents)
        collectParentMimeTypes(parent, allParents);
}

/*!
    \property QMimeType::allAncestors
    \brief the names of direct and indirect parent MIME types

    Return all the parent mimetypes of this mimetype, direct and indirect.
    This includes the parent(s) of its parent(s), etc.

    For instance, for image/svg+xml the list would be:
    application/xml, text/plain, application/octet-stream.

    Note that application/octet-stream is the ultimate parent for all types
    of files (but not directories).

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
*/
QStringList QMimeType::allAncestors() const
{
    QStringList allParents;
    collectParentMimeTypes(d->name, allParents);
    return allParents;
}

/*!
    \property QMimeType::aliases
    \brief the list of aliases of this mimetype

    For instance, for text/csv, the returned list would be:
    text/x-csv, text/x-comma-separated-values.

    Note that all QMimeType instances refer to proper mimetypes,
    never to aliases directly.

    The order of the aliases in the list is undefined.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
*/
QStringList QMimeType::aliases() const
{
    return QMimeDatabasePrivate::instance()->listAliases(d->name);
}

/*!
    \property QMimeType::suffixes
    \brief the known suffixes for the MIME type

    No leading dot is included, so for instance this would return "jpg", "jpeg" for image/jpeg.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QStringList QMimeType::suffixes() const
{
    const QStringList patterns = globPatterns();

    QStringList result;
    result.reserve(patterns.size());
    for (const QString &pattern : patterns) {
        // Not a simple suffix if it looks like: README or *. or *.* or *.JP*G or *.JP?
        if (pattern.startsWith("*."_L1) &&
            pattern.size() > 2 &&
            pattern.indexOf(u'*', 2) < 0 && pattern.indexOf(u'?', 2) < 0) {
            const QString suffix = pattern.mid(2);
            result.append(suffix);
        }
    }

    return result;
}

/*!
    \property QMimeType::preferredSuffix
    \brief the preferred suffix for the MIME type

    No leading dot is included, so for instance this would return "pdf" for application/pdf.
    The return value can be empty, for mime types which do not have any suffixes associated.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString QMimeType::preferredSuffix() const
{
    if (isDefault()) // workaround for unwanted *.bin suffix for octet-stream, https://bugs.freedesktop.org/show_bug.cgi?id=101667, fixed upstream in 1.10
        return QString();
    const QStringList suffixList = suffixes();
    return suffixList.isEmpty() ? QString() : suffixList.at(0);
}

/*!
    \property QMimeType::filterString
    \brief a filter string usable for a file dialog

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
*/
QString QMimeType::filterString() const
{
    const QStringList patterns = globPatterns();
    QString filter;

    if (!patterns.isEmpty()) {
        filter = comment() + " ("_L1 + patterns.join(u' ') + u')';
    }

    return filter;
}

/*!
    \fn bool QMimeType::inherits(const QString &mimeTypeName) const;
    Returns \c true if this mimetype is \a mimeTypeName,
    or inherits \a mimeTypeName (see parentMimeTypes()),
    or \a mimeTypeName is an alias for this mimetype.

    This method has been made invokable from QML since 5.10.
 */
bool QMimeType::inherits(const QString &mimeTypeName) const
{
    if (d->name == mimeTypeName)
        return true;
    return QMimeDatabasePrivate::instance()->mimeInherits(d->name, mimeTypeName);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QMimeType &mime)
{
    QDebugStateSaver saver(debug);
    if (!mime.isValid()) {
        debug.nospace() << "QMimeType(invalid)";
    } else {
        debug.nospace() << "QMimeType(" << mime.name() << ")";
    }
    return debug;
}
#endif

QT_END_NAMESPACE

#include "moc_qmimetype.cpp"
