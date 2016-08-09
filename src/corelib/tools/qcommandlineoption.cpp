/****************************************************************************
**
** Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
** Copyright (C) 2013 David Faure <faure@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcommandlineoption.h"

#include "qset.h"

QT_BEGIN_NAMESPACE

class QCommandLineOptionPrivate : public QSharedData
{
public:
    Q_NEVER_INLINE
    explicit QCommandLineOptionPrivate(const QString &name)
        : names(removeInvalidNames(QStringList(name)))
    { }

    Q_NEVER_INLINE
    explicit QCommandLineOptionPrivate(const QStringList &names)
        : names(removeInvalidNames(names))
    { }

    static QStringList removeInvalidNames(QStringList nameList);

    //! The list of names used for this option.
    QStringList names;

    //! The documentation name for the value, if one is expected
    //! Example: "-o <file>" means valueName == "file"
    QString valueName;

    //! The description used for this option.
    QString description;

    //! The list of default values used for this option.
    QStringList defaultValues;

    QCommandLineOption::Flags flags;
};

/*!
    \since 5.2
    \class QCommandLineOption
    \brief The QCommandLineOption class defines a possible command-line option.
    \inmodule QtCore
    \ingroup shared
    \ingroup tools

    This class is used to describe an option on the command line. It allows
    different ways of defining the same option with multiple aliases possible.
    It is also used to describe how the option is used - it may be a flag (e.g. \c{-v})
    or take a value (e.g. \c{-o file}).

    Examples:
    \snippet code/src_corelib_tools_qcommandlineoption.cpp 0

    \sa QCommandLineParser
*/

/*!
    \fn QCommandLineOption &QCommandLineOption::operator=(QCommandLineOption &&other)

    Move-assigns \a other to this QCommandLineOption instance.

    \since 5.2
*/

/*!
    Constructs a command line option object with the name \a name.

    The name can be either short or long. If the name is one character in
    length, it is considered a short name. Option names must not be empty,
    must not start with a dash or a slash character, must not contain a \c{=}
    and cannot be repeated.

    \sa setDescription(), setValueName(), setDefaultValues()
*/
QCommandLineOption::QCommandLineOption(const QString &name)
    : d(new QCommandLineOptionPrivate(name))
{
}

/*!
    Constructs a command line option object with the names \a names.

    This overload allows to set multiple names for the option, for instance
    \c{o} and \c{output}.

    The names can be either short or long. Any name in the list that is one
    character in length is a short name. Option names must not be empty,
    must not start with a dash or a slash character, must not contain a \c{=}
    and cannot be repeated.

    \sa setDescription(), setValueName(), setDefaultValues()
*/
QCommandLineOption::QCommandLineOption(const QStringList &names)
    : d(new QCommandLineOptionPrivate(names))
{
}

/*!
    Constructs a command line option object with the given arguments.

    The name of the option is set to \a name.
    The name can be either short or long. If the name is one character in
    length, it is considered a short name. Option names must not be empty,
    must not start with a dash or a slash character, must not contain a \c{=}
    and cannot be repeated.

    The description is set to \a description. It is customary to add a "."
    at the end of the description.

    In addition, the \a valueName needs to be set if the option expects a value.
    The default value for the option is set to \a defaultValue.

    In Qt versions before 5.4, this constructor was \c explicit. In Qt 5.4
    and later, it no longer is and can be used for C++11-style uniform
    initialization:

    \snippet code/src_corelib_tools_qcommandlineoption.cpp cxx11-init

    \sa setDescription(), setValueName(), setDefaultValues()
*/
QCommandLineOption::QCommandLineOption(const QString &name, const QString &description,
                                       const QString &valueName,
                                       const QString &defaultValue)
    : d(new QCommandLineOptionPrivate(name))
{
    setValueName(valueName);
    setDescription(description);
    setDefaultValue(defaultValue);
}

/*!
    Constructs a command line option object with the given arguments.

    This overload allows to set multiple names for the option, for instance
    \c{o} and \c{output}.

    The names of the option are set to \a names.
    The names can be either short or long. Any name in the list that is one
    character in length is a short name. Option names must not be empty,
    must not start with a dash or a slash character, must not contain a \c{=}
    and cannot be repeated.

    The description is set to \a description. It is customary to add a "."
    at the end of the description.

    In addition, the \a valueName needs to be set if the option expects a value.
    The default value for the option is set to \a defaultValue.

    In Qt versions before 5.4, this constructor was \c explicit. In Qt 5.4
    and later, it no longer is and can be used for C++11-style uniform
    initialization:

    \snippet code/src_corelib_tools_qcommandlineoption.cpp cxx11-init-list

    \sa setDescription(), setValueName(), setDefaultValues()
*/
QCommandLineOption::QCommandLineOption(const QStringList &names, const QString &description,
                                       const QString &valueName,
                                       const QString &defaultValue)
    : d(new QCommandLineOptionPrivate(names))
{
    setValueName(valueName);
    setDescription(description);
    setDefaultValue(defaultValue);
}

/*!
    Constructs a QCommandLineOption object that is a copy of the QCommandLineOption
    object \a other.

    \sa operator=()
*/
QCommandLineOption::QCommandLineOption(const QCommandLineOption &other)
    : d(other.d)
{
}

/*!
    Destroys the command line option object.
*/
QCommandLineOption::~QCommandLineOption()
{
}

/*!
    Makes a copy of the \a other object and assigns it to this QCommandLineOption
    object.
*/
QCommandLineOption &QCommandLineOption::operator=(const QCommandLineOption &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QCommandLineOption::swap(QCommandLineOption &other)

    Swaps option \a other with this option. This operation is very
    fast and never fails.
*/

/*!
    Returns the names set for this option.
 */
QStringList QCommandLineOption::names() const
{
    return d->names;
}

namespace {
    struct IsInvalidName
    {
        typedef bool result_type;
        typedef QString argument_type;

        Q_NEVER_INLINE
        result_type operator()(const QString &name) const Q_DECL_NOEXCEPT
        {
            if (Q_UNLIKELY(name.isEmpty()))
                return warn("be empty");

            const QChar c = name.at(0);
            if (Q_UNLIKELY(c == QLatin1Char('-')))
                return warn("start with a '-'");
            if (Q_UNLIKELY(c == QLatin1Char('/')))
                return warn("start with a '/'");
            if (Q_UNLIKELY(name.contains(QLatin1Char('='))))
                return warn("contain a '='");

            return false;
        }

        Q_NEVER_INLINE
        static bool warn(const char *what) Q_DECL_NOEXCEPT
        {
            qWarning("QCommandLineOption: Option names cannot %s", what);
            return true;
        }
    };
} // unnamed namespace

// static
QStringList QCommandLineOptionPrivate::removeInvalidNames(QStringList nameList)
{
    if (Q_UNLIKELY(nameList.isEmpty()))
        qWarning("QCommandLineOption: Options must have at least one name");
    else
        nameList.erase(std::remove_if(nameList.begin(), nameList.end(), IsInvalidName()),
                       nameList.end());
    return nameList;
}

/*!
    Sets the name of the expected value, for the documentation, to \a valueName.

    Options without a value assigned have a boolean-like behavior:
    either the user specifies --option or they don't.

    Options with a value assigned need to set a name for the expected value,
    for the documentation of the option in the help output. An option with names \c{o} and \c{output},
    and a value name of \c{file} will appear as \c{-o, --output <file>}.

    Call QCommandLineParser::value() if you expect the option to be present
    only once, and QCommandLineParser::values() if you expect that option
    to be present multiple times.

    \sa valueName()
 */
void QCommandLineOption::setValueName(const QString &valueName)
{
    d->valueName = valueName;
}

/*!
    Returns the name of the expected value.

    If empty, the option doesn't take a value.

    \sa setValueName()
 */
QString QCommandLineOption::valueName() const
{
    return d->valueName;
}

/*!
    Sets the description used for this option to \a description.

    It is customary to add a "." at the end of the description.

    The description is used by QCommandLineParser::showHelp().

    \sa description()
 */
void QCommandLineOption::setDescription(const QString &description)
{
    d->description = description;
}

/*!
    Returns the description set for this option.

    \sa setDescription()
 */
QString QCommandLineOption::description() const
{
    return d->description;
}

/*!
    Sets the default value used for this option to \a defaultValue.

    The default value is used if the user of the application does not specify
    the option on the command line.

    If \a defaultValue is empty, the option has no default values.

    \sa defaultValues() setDefaultValues()
 */
void QCommandLineOption::setDefaultValue(const QString &defaultValue)
{
    QStringList newDefaultValues;
    if (!defaultValue.isEmpty()) {
        newDefaultValues.reserve(1);
        newDefaultValues << defaultValue;
    }
    // commit:
    d->defaultValues.swap(newDefaultValues);
}

/*!
    Sets the list of default values used for this option to \a defaultValues.

    The default values are used if the user of the application does not specify
    the option on the command line.

    \sa defaultValues() setDefaultValue()
 */
void QCommandLineOption::setDefaultValues(const QStringList &defaultValues)
{
    d->defaultValues = defaultValues;
}

/*!
    Returns the default values set for this option.

    \sa setDefaultValues()
 */
QStringList QCommandLineOption::defaultValues() const
{
    return d->defaultValues;
}

#if QT_DEPRECATED_SINCE(5, 8)
/*!
    Sets whether to hide this option in the user-visible help output.

    All options are visible by default. Setting \a hide to true for
    a particular option makes it internal, i.e. not listed in the help output.

    \since 5.6
    \obsolete Use setFlags(QCommandLineOption::HiddenFromHelp), QCommandLineOption::HiddenFromHelp
    \sa isHidden
 */
void QCommandLineOption::setHidden(bool hide)
{
    d->flags.setFlag(HiddenFromHelp, hide);
}

/*!
    Returns true if this option is omitted from the help output,
    false if the option is listed.

    \since 5.6
    \obsolete Use flags() & QCommandLineOption::HiddenFromHelp
    \sa setHidden(), QCommandLineOption::HiddenFromHelp
 */
bool QCommandLineOption::isHidden() const
{
    return d->flags & HiddenFromHelp;
}
#endif

/*!
    Returns a set of flags that affect this command-line option.

    \since 5.8
    \sa setFlags(), QCommandLineOption::Flags
 */
QCommandLineOption::Flags QCommandLineOption::flags() const
{
    return d->flags;
}

/*!
    Set the set of flags that affect this command-line option to \a flags.

    \since 5.8
    \sa flags(), QCommandLineOption::Flags
 */
void QCommandLineOption::setFlags(Flags flags)
{
    d->flags = flags;
}

/*!
    \enum QCommandLineOption::Flag

    \value HiddenFromHelp Hide this option in the user-visible help output. All
    options are visible by default. Setting this flag for a particular
    option makes it internal, i.e. not listed in the help output.

    \value ShortOptionStyle The option will always be understood as a short
    option, regardless of what was set by
    QCommandLineParser::setSingleDashWordOptionMode.
    This allows flags such as \c{-DDEFINE=VALUE} or \c{-I/include/path} to be
    interpreted as short flags even when the parser is in
    QCommandLineParser::ParseAsLongOptions mode.

    \sa QCommandLineOption::setFlags(), QCommandLineOption::flags()
*/

QT_END_NAMESPACE
