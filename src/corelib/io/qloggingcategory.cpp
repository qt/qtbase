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

    \brief A category, or 'area' in the logging infrastructure.

    QLoggingCategory represents a certain logging category - identified
    by a string - at runtime. Whether a category should be actually logged or
    not can be checked with the \l isEnabled() methods.

    \section1 Creating category objects

    Qt provides the \l Q_LOGGING_CATEGORY(), Q_DECLARE_LOGGING_CATEGORY() macros
    to conveniently create static QLoggingCategory objects on the heap:

    \snippet qloggingcategory/main.cpp 1

    \section1 Checking category configuration

    QLoggingCategory provides two isEnabled methods, a template one and a
    non-template one, for checking whether the current category is enabled.
    The template version checks for the most common case that no special rules
    are applied in inline code, and should be preferred:

    \snippet qloggingcategory/main.cpp 2

    Note that qCDebug() prevents arguments from being evaluated
    if the string won't print, so calling isEnabled explicitly is not needed:
    \l isEnabled().

    \snippet qloggingcategory/main.cpp 3

    \section1 Default configuration

    In the default configuration \l isEnabled() will return true for all
    \l QtMsgType types except QtDebugMsg: QtDebugMsg is only active by default
    for the \c "default" category.

    \section1 Changing configuration

    The default configuration can be changed by calling \l setEnabled(). However,
    this only affects the current category object, not e.g. another object for the
    same category name. Use either \l setFilterRules() or \l installFilter() to
    configure categories globally.
*/

/*!
  \internal
*/
typedef QVector<QTracer *> Tracers;

/*!
  \internal
*/
class QLoggingCategoryPrivate
{
public:
    Tracers tracers;
};

/*!
    Constructs a QLoggingCategory object with the provided \a category name.
    The object becomes the local identifier for the category.

    If \a category is \c{0}, the category name is changed to \c{"default"}.
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
    Destructs a QLoggingCategory object
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
    \fn bool QLoggingCategory::isEnabled() const

    Returns true if a message of the template \c QtMsgType argument should be
    shown. Returns false otherwise.

    \note The qCDebug, qCWarning, qCCritical macros already do this check before
    executing any code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    Returns true if a message of type \a msgtype for the category should be
    shown. Returns false otherwise.

    \note The templated, inline version of this method, \l isEnabled(), is
    optimized for the common case that no configuration is set, and should
    generally be preferred.
*/
bool QLoggingCategory::isEnabled(QtMsgType msgtype) const
{
    switch (msgtype) {
    case QtDebugMsg: return enabledDebug;
    case QtWarningMsg: return enabledWarning;
    case QtCriticalMsg: return enabledCritical;
    case QtTraceMsg: return enabledTrace;
    case QtFatalMsg: return true;
    default: break;
    }
    return false;
}

/*!
    Changes the type \a type for the category to \a enable.

    Changes only affect the current QLoggingCategory object, and won't
    change e.g. the settings of another objects for the same category name.

    \note QtFatalMsg cannot be changed. It will always return true.

    Example:

    \snippet qtracer/ftracer.cpp 5
*/
void QLoggingCategory::setEnabled(QtMsgType type, bool enable)
{
    switch (type) {
    case QtDebugMsg: enabledDebug = enable; break;
    case QtWarningMsg: enabledWarning = enable; break;
    case QtCriticalMsg: enabledCritical = enable; break;
    case QtTraceMsg: enabledTrace = enable; break;
    case QtFatalMsg:
    default: break;
    }
}

/*!
    \fn QLoggingCategory &QLoggingCategory::operator()()

    Returns the object itself. This allows both a QLoggingCategory variable, and
    a factory method returning a QLoggingCategory, to be used in qCDebug(),
    qCWarning(), qCCritial() macros.
 */

/*!
    Returns the category "default" that is used e.g. by qDebug(), qWarning(),
    qCritical(), qFatal().
 */
QLoggingCategory &QLoggingCategory::defaultCategory()
{
    return *qtDefaultCategory();
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
    be either \c debug, \c warning, or \c critical.

    The rules might be ignored if a custom category filter is installed with
    \l installFilter().
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

    The macro expands to code that first checks whether
    \l QLoggingCategory::isEnabled() evaluates for debug output to \c{true}.
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

    The macro expands to code that first checks whether
    \l QLoggingCategory::isEnabled() evaluates for warning output to \c{true}.
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

    The macro expands to code that first checks whether
    \l QLoggingCategory::isEnabled() evaluates for critical output to \c{true}.
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

    The macro expands to code that first checks whether
    \l QLoggingCategory::isEnabled() evaluates for trace output to \c{true}.
    If so, the stream arguments are processed and sent to the tracers
    registered with the category.

    \note Arguments are not processed if trace output for the category is not
    enabled, so do not rely on any side effects.

    Example:

    \snippet qtracer/ftracer.cpp 6

    \sa qCTraceGuard()
*/

/*!
    \relates QLoggingCategory
    \macro qCTraceGuard(category)
    \since 5.2

    The macro expands to code that creates a guard object with automatic
    storage. The guard constructor checks whether
    \l QLoggingCategory::isEnabled() evaluates for trace output to \c{true}.
    If so, the stream arguments are processed and the \c{start()}
    functions of the tracers registered with the \a category are called.

    The guard destructor also checks whether the category is enabled for
    tracing and if so, the \c{end()}
    functions of the tracers registered with the \a category are called.

    \note Arguments are always processed, even if trace output for the
    category is disabled. They will, however, in that case not be passed
    to the \c{record()} functions of the registered tracers.

    Example:

    \snippet qtracer/ftracer.cpp 4

    \sa qCTrace()
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
