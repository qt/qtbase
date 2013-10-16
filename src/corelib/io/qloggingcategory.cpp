/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qloggingcategory.h"
#include "qloggingcategory_p.h"
#include "qloggingregistry_p.h"

QT_BEGIN_NAMESPACE

const char qtDefaultCategoryName[] = "default";

Q_GLOBAL_STATIC_WITH_ARGS(QLoggingCategory, qtDefaultCategory,
                          (qtDefaultCategoryName))

/*!
    \class QLoggingCategory
    \inmodule QtCore
    \since 5.2

    \brief The QLoggingCategory class represents a category, or 'area' in the
    logging infrastructure.

    QLoggingCategory represents a certain logging category - identified
    by a string - at runtime. Whether a category should be actually logged or
    not can be checked with the \l isEnabled() methods.

    \section1 Creating category objects

    The Q_LOGGING_CATEGORY() and the Q_DECLARE_LOGGING_CATEGORY() macros
    conveniently declare and create QLoggingCategory objects:

    \snippet qloggingcategory/main.cpp 1

    \section1 Checking category configuration

    QLoggingCategory provides \l isDebugEnabled(), \l isWarningEnabled(),
    \l isCriticalEnabled(), \l isTraceEnabled(), as well as \l isEnabled()
    to check whether messages for the given message type should be logged.

    \note The qCDebug(), qCWarning(), qCCritical(), qCTrace() and
    qCTraceGuard() macros prevent arguments from being evaluated if the
    respective message types are not enabled for the category, so explicit
    checking is not needed:

    \snippet qloggingcategory/main.cpp 4

    \section1 Default category configuration

    In the default configuration \l isWarningEnabled() and \l isCriticalEnabled()
    will return \c true. \l isDebugEnabled() will return \c true only
    for the \c "default" category.

    \section1 Changing the configuration of a category

    Use either \l setFilterRules() or \l installFilter() to
    configure categories, for example

    \snippet qloggingcategory/main.cpp 2

    \section1 Printing the category

    Use the \c %{category} place holder to print the category in the default
    message handler:

    \snippet qloggingcategory/main.cpp 3
*/

typedef QVector<QTracer *> Tracers;

class QLoggingCategoryPrivate
{
public:
    Tracers tracers;
};

/*!
    Constructs a QLoggingCategory object with the provided \a category name.
    The object becomes the local identifier for the category.

    If \a category is \c{0}, the category name is changed to \c "default".
*/
QLoggingCategory::QLoggingCategory(const char *category)
    : d(new QLoggingCategoryPrivate),
      name(0),
      enabledDebug(false),
      enabledWarning(true),
      enabledCritical(true),
      enabledTrace(false)
{
    bool isDefaultCategory
            = (category == 0) || (strcmp(category, qtDefaultCategoryName) == 0);

    if (isDefaultCategory) {
        // normalize default category names, so that we can just do
        // pointer comparison in QLoggingRegistry::updateCategory
        name = qtDefaultCategoryName;
        enabledDebug = true;
    } else {
        name = category;
    }

    if (QLoggingRegistry *reg = QLoggingRegistry::instance())
        reg->registerCategory(this);
}

/*!
    Destructs a QLoggingCategory object.
*/
QLoggingCategory::~QLoggingCategory()
{
    if (QLoggingRegistry *reg = QLoggingRegistry::instance())
        reg->unregisterCategory(this);
    delete d;
}

/*!
   \fn const char *QLoggingCategory::categoryName() const

    Returns the name of the category.
*/

/*!
    \fn bool QLoggingCategory::isDebugEnabled() const

    Returns \c true if debug messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCDebug() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    \fn bool QLoggingCategory::isWarningEnabled() const

    Returns \c true if warning messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCWarning() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    \fn bool QLoggingCategory::isCriticalEnabled() const

    Returns \c true if critical messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCCritical() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    \fn bool QLoggingCategory::isTraceEnabled() const

    Returns \c true if the tracers associated with this category should
    receive messages. Returns \c false otherwise.

    \note The \l qCTrace() and \l qCTraceGuard() macros already do this check
    before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    Returns \c true if a message of type \a msgtype for the category should be
    shown. Returns \c false otherwise.
*/
bool QLoggingCategory::isEnabled(QtMsgType msgtype) const
{
    switch (msgtype) {
    case QtDebugMsg: return enabledDebug;
    case QtWarningMsg: return enabledWarning;
    case QtCriticalMsg: return enabledCritical;
    case QtTraceMsg: return enabledTrace;
    case QtFatalMsg: return true;
    }
    return false;
}

/*!
    Changes the message type \a type for the category to \a enable.

    \note Changes only affect the current QLoggingCategory object, and won't
    change the settings of other objects for the same category name.
    Use either \l setFilterRules() or \l installFilter() to change the
    configuration globally.

    \note \c QtFatalMsg cannot be changed. It will always return \c true.
*/
void QLoggingCategory::setEnabled(QtMsgType type, bool enable)
{
    switch (type) {
    case QtDebugMsg: enabledDebug = enable; break;
    case QtWarningMsg: enabledWarning = enable; break;
    case QtCriticalMsg: enabledCritical = enable; break;
    case QtTraceMsg: enabledTrace = enable; break;
    case QtFatalMsg: break;
    }
}

/*!
    \fn QLoggingCategory &QLoggingCategory::operator()()

    Returns the object itself. This allows both a QLoggingCategory variable, and
    a factory method returning a QLoggingCategory, to be used in \l qCDebug(),
    \l qCWarning(), \l qCCritical() macros.
 */

/*!
    Returns a pointer to the global category \c "default" that
    is used e.g. by qDebug(), qWarning(), qCritical(), qFatal().

    \note The returned pointer may be null during destruction of
    static objects.

    \note Ownership of the category is not transferred, do not
    \c delete the returned pointer.

 */
QLoggingCategory *QLoggingCategory::defaultCategory()
{
    return qtDefaultCategory();
}

/*!
    \typedef QLoggingCategory::CategoryFilter

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet qloggingcategory/main.cpp 20

    A function with this signature can be installed with \l installFilter().
*/

/*!
    Installs a function \a filter that is used to determine which categories
    and message types should be enabled. Returns a pointer to the previous
    installed filter.

    Every QLoggingCategory object created is passed to the filter, and the
    filter is free to change the respective category configuration with
    \l setEnabled().

    The filter might be called concurrently from different threads, and
    therefore has to be reentrant.

    Example:
    \snippet qloggingcategory/main.cpp 21

    An alternative way of configuring the default filter is via
    \l setFilterRules().
 */
QLoggingCategory::CategoryFilter
QLoggingCategory::installFilter(QLoggingCategory::CategoryFilter filter)
{
    return QLoggingRegistry::instance()->installFilter(filter);
}

/*!
    Configures which categories and message types should be enabled through a
    a set of \a rules.

    Each line in \a rules must have the format

    \code
    <category>[.<type>] = true|false
    \endcode

    where \c <category> is the name of the category, potentially with \c{*} as a
    wildcard symbol at the start and/or the end. The optional \c <type> must
    be either \c debug, \c warning, \c critical, or \c trace.

    Example:

    \snippet qloggingcategory/main.cpp 2

    \note The rules might be ignored if a custom category filter is installed
    with \l installFilter().

*/
void QLoggingCategory::setFilterRules(const QString &rules)
{
    QLoggingRegistry::instance()->rulesParser.setRules(rules);
}

/*!
    \macro qCDebug(category)
    \relates QLoggingCategory
    \since 5.2

    Returns an output stream for debug messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isDebugEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 10

    \note Arguments are not processed if debug output for the category is not
    enabled, so do not rely on any side effects.

    \sa qDebug()
*/

/*!
    \macro qCWarning(category)
    \relates QLoggingCategory
    \since 5.2

    Returns an output stream for warning messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isWarningEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 11

    \note Arguments are not processed if warning output for the category is not
    enabled, so do not rely on any side effects.

    \sa qWarning()
*/

/*!
    \macro qCCritical(category)
    \relates QLoggingCategory
    \since 5.2

    Returns an output stream for critical messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isCriticalEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 12

    \note Arguments are not processed if critical output for the category is not
    enabled, so do not rely on any side effects.

    \sa qCritical()
*/

/*!
    \relates QLoggingCategory
    \macro qCTrace(category)
    \since 5.2

    Returns an output stream for trace messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isTraceEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the \l QTracer objects
    registered with the category.

    \note Arguments are not processed if trace output for the category is not
    enabled, so do not rely on any side effects.

    Example:

    \snippet qtracer/ftracer.cpp 6

    \sa qCTraceGuard() QTraceGuard
*/

/*!
    \relates QLoggingCategory
    \macro qCTraceGuard(category)
    \since 5.2

    The macro expands to code that creates a guard object with automatic
    storage. The guard constructor checks whether
    \l QLoggingCategory::isTraceEnabled() evaluates to \c true.
    If so, the stream arguments are processed and the \c{start()}
    functions of the \l QTracer objects registered with the \a category are
    called.

    The guard destructor also checks whether the category is enabled for
    tracing and if so, the \c{end()}
    functions of the \l QTracer objects registered with the \a category are called.

    \note Arguments are always processed, even if trace output for the
    category is disabled. They will, however, in that case not be passed
    to the \c{record()} functions of the registered tracers.

    Example:

    \snippet qtracer/ftracer.cpp 4

    \sa qCTrace() QTracer
*/

/*!
    \macro Q_DECLARE_LOGGING_CATEGORY(name)
    \relates QLoggingCategory
    \since 5.2

    Declares a logging category \a name. The macro can be used to declare
    a common logging category shared in different parts of the program.

    This macro must be used outside of a class or method.
*/

/*!
    \macro Q_LOGGING_CATEGORY(name, string)
    \relates QLoggingCategory
    \since 5.2

    Defines a logging category \a name, and makes it configurable under the
    \a string identifier.

    Only one translation unit in a library or executable can define a category
    with a specific name.

    This macro must be used outside of a class or method.
*/


/*!
    \class QTracer
    \inmodule QtCore
    \since 5.2

    \brief The QTracer class provides an interface for handling
    trace events associated with a logging category.

    \c QTracer objects are registered with logging categories.
    Multiple \c QTracer objects
    can be registered with the same category, and the same
    \c QTracer object can be registered with different categories.

    If code containing \c qCTrace is executed, and the associated
    logging category is enabled for tracing, all \c QTracer objects
    that are registered with the category are notified.

    \c QTracer objects
*/

/*!
    \fn QTracer::QTracer()

    Constructs a tracer object.

    Example:

    \snippet qtracer/ftracer.cpp 2
*/

/*!
    \fn QTracer::~QTracer()

    Destroys the tracer object.
*/

/*!
    Registers this tracer for the \a category.

    The tracer will later be notified of messages of type
    \c QtTraceMsg, as long as that message type
    is enabled in the category.

    Example:

    \snippet qtracer/ftracer.cpp 1
    \codeline
    \snippet qtracer/ftracer.cpp 7
*/

void QTracer::addToCategory(QLoggingCategory &category)
{
    category.d->tracers.append(this);
}

/*!
    \fn void QTracer::start()

    This function is invoked when a tracing activity starts,
    typically from the constructor of a \c QTraceGuard object
    defined by \c qCTrace() or \c qCTraceGuard().

    The base implementation does nothing. \c QTracer subclasses
    are advised to override it if needed.

    \sa qCTrace(), qCTraceGuard()
*/

/*!
    \fn void QTracer::end()

    This function is invoked when a tracing activity ends,
    typically from the destructor of a \c QTraceGuard object
    defined by \c qCTrace() or \c qCTraceGuard().

    The base implementation does nothing. It is common for
    \c QTracer subclasses to override it to perform flushing
    of collected data.

    \sa qCTrace(), qCTraceGuard()
*/

/*!
    \fn void QTracer::record(int data)

    This function is invoked during a tracing activity to
    pass integer \a data to the \c QTracer object.

    Example:

    \snippet qtracer/ftracer.cpp 3
*/

/*!
    \fn void QTracer::record(const char *data)

    This function is invoked during a tracing activity to
    pass string \a data to the \c QTracer object.
*/

/*!
    \fn void QTracer::record(const QVariant &data)

    This function is invoked during a tracing activity to
    pass abitrary (non-integer, non-string) \a data to
    the \c QTracer object.
*/

/*!
    \class QTraceGuard
    \since 5.2
    \inmodule QtCore

    \brief The QTraceGuard class facilitates notifications to
    \c QTracer objects.

    \c QTraceGuard objects are typically implicitly created on the
    stack when using the \c qCTrace or \c qCTraceGuard macros and
    are associated to a \c QLoggingCategory.

    The constructor of a \c QTraceGuard objects checks whether
    its associated category is enabled, and if so, informs all
    \c QTracer objects registered with the category that a
    tracing activity starts.

    The destructor of a \c QTraceGuard objects checks whether
    its associated category is enabled, and if so, informs all
    \c QTracer objects registered with the category that a
    tracing activity ended.

    A \c QTraceGuard object created by \c qCTrace will be destroyed
    at the end of the full expression, a guard created by
    \c qCTraceGuard at the end of the block containing the macro.

    During the lifetime of a QTraceGuard object, its \c operator<<()
    can be used to pass additional data to the active tracers.
    The fast path handles only \c int and \c{const char *} data,
    but it is possible to use arbitrary values wrapped in \c QVariants.

    \sa QTracer
*/

/*!
    \fn QTraceGuard::QTraceGuard(QLoggingCategory &category)
    \internal

    Constructs a trace guard object relaying to \a category.
*/

/*!
    \fn QTraceGuard::~QTraceGuard()
    \internal

    Destroys the trace guard object.
*/

/*!
    \internal

    Calls \c start() on all registered tracers.
*/

void QTraceGuard::start()
{
    const Tracers &tracers = target->d->tracers;
    for (int i = tracers.size(); --i >= 0; )
        tracers.at(i)->start();
}

/*!
    \internal

    Calls \c end() on all registered tracers.
*/

void QTraceGuard::end()
{
    const Tracers &tracers = target->d->tracers;
    for (int i = tracers.size(); --i >= 0; )
        tracers.at(i)->end();
}


/*!
    \internal

    This function is called for int parameters passed to the
    qCTrace stream.
*/

QTraceGuard &QTraceGuard::operator<<(int msg)
{
    const Tracers &tracers = target->d->tracers;
    for (int i = tracers.size(); --i >= 0; )
        tracers.at(i)->record(msg);
    return *this;
}

/*!
    \internal

    This function is called for string parameters passed to the
    qCTrace stream.
*/

QTraceGuard &QTraceGuard::operator<<(const char *msg)
{
    const Tracers &tracers = target->d->tracers;
    for (int i = tracers.size(); --i >= 0; )
        tracers.at(i)->record(msg);
    return *this;
}


/*!
    \internal

    This function is called for QVariant parameters passed to the
    qCTrace stream.
*/

QTraceGuard &QTraceGuard::operator<<(const QVariant &msg)
{
    const Tracers &tracers = target->d->tracers;
    for (int i = tracers.size(); --i >= 0; )
        tracers.at(i)->record(msg);
    return *this;
}

QT_END_NAMESPACE
