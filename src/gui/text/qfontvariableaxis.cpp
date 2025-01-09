// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfontvariableaxis.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class QFontVariableAxisPrivate : public QSharedData
{
public:
    QFont::Tag tag;
    QString name;
    qreal minimumValue = 0.0;
    qreal maximumValue = 0.0;
    qreal defaultValue = 0.0;
};

/*!
    \class QFontVariableAxis
    \reentrant
    \inmodule QtGui
    \ingroup shared
    \since 6.9

    \brief The QFontVariableAxis class represents a variable axis in a font.

    Variable fonts provide a way to store multiple variations (with different weights, widths
    or styles) in the same font file. The variations are given as floating point values for
    a pre-defined set of parameters, called "variable axes".

    Specific parameterizations (sets of values for the axes in a font) can be selected using
    the properties in QFont, same as with traditional subfamilies that are defined as stand-alone
    font files. But with variable fonts, arbitrary values can be provided for each axis to gain a
    fine-grained customization of the font's appearance.

    QFontVariableAxis contains information of one axis. Use \l{QFontInfo::variableAxes()}
    to retrieve a list of the variable axes defined for a given font. Specific values can be
    provided for an axis by using \l{QFont::setVariableAxis()} and passing in the \l{tag()}.

    \note On Windows, variable axes are not supported if the optional GDI font backend is in use.
*/
QFontVariableAxis::QFontVariableAxis()
    : d_ptr(new QFontVariableAxisPrivate)
{
}

/*!
    Destroys this QFontVariableAxis object.
*/
QFontVariableAxis::~QFontVariableAxis() = default;
QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QFontVariableAxisPrivate)

/*!
    Creates a QFontVariableAxis object that is a copy of the given \a axis.

    \sa operator=()
*/
QFontVariableAxis::QFontVariableAxis(const QFontVariableAxis &axis) = default;

/*!
    Returns the tag of the axis. This is a four-character sequence which identifies the axis.
    Certain tags have standardized meanings, such as "wght" (weight) and "wdth" (width), but any
    sequence of four latin-1 characters is a valid tag. By convention, non-standard/custom axes
    are denoted by tags in all uppercase.

    \sa QFont::setVariableAxis(), name()
 */
QFont::Tag QFontVariableAxis::tag() const
{
    Q_D(const QFontVariableAxis);
    return d->tag;
}

/*!
    Sets the tag of QFontVariableAxis to \a tag.

    \note Typically, there will be no need to call this function as it will not affect the font
    itself, only this particular representation.

    \sa tag()
 */
void QFontVariableAxis::setTag(QFont::Tag tag)
{
    if (d_func()->tag == tag)
        return;
    detach();
    Q_D(QFontVariableAxis);
    d->tag = tag;
}

/*!
    Returns the name of the axis, if provided by the font.

    \sa tag()
*/
QString QFontVariableAxis::name() const
{
    Q_D(const QFontVariableAxis);
    return d->name;
}

/*!
    Sets the name of this QFontVariableAxis to \a name.

    \note Typically, there will be no need to call this function as it will not affect the font
    itself, only this particular representation.

    \sa name()
 */
void QFontVariableAxis::setName(const QString &name)
{
    if (d_func()->name == name)
        return;
    detach();
    Q_D(QFontVariableAxis);
    d->name = name;
}

/*!
    Returns the minimum value of the axis. Setting the axis to a value which is lower than this
    is not supported.

    \sa maximumValue(), defaultValue()
*/
qreal QFontVariableAxis::minimumValue() const
{
    Q_D(const QFontVariableAxis);
    return d->minimumValue;
}

/*!
    Sets the minimum value of this QFontVariableAxis to \a minimumValue.

    \note Typically, there will be no need to call this function as it will not affect the font
    itself, only this particular representation.

    \sa minimumValue()
*/
void QFontVariableAxis::setMinimumValue(qreal minimumValue)
{
    if (d_func()->minimumValue == minimumValue)
        return;
    detach();
    Q_D(QFontVariableAxis);
    d->minimumValue = minimumValue;
}

/*!
    Returns the maximum value of the axis. Setting the axis to a value which is higher than this
    is not supported.

    \sa minimumValue(), defaultValue()
*/
qreal QFontVariableAxis::maximumValue() const
{
    Q_D(const QFontVariableAxis);
    return d->maximumValue;
}

/*!
    Sets the maximum value of this QFontVariableAxis to \a maximumValue.

    \note Typically, there will be no need to call this function as it will not affect the font
    itself, only this particular representation.

    \sa maximumValue()
*/
void QFontVariableAxis::setMaximumValue(qreal maximumValue)
{
    if (d_func()->maximumValue == maximumValue)
        return;
    detach();
    Q_D(QFontVariableAxis);
    d->maximumValue = maximumValue;
}

/*!
    Returns the default value of the axis. This is the value the axis will have if none has been
    provided in the QFont query.

    \sa minimumValue(), maximumValue()
*/
qreal QFontVariableAxis::defaultValue() const
{
    Q_D(const QFontVariableAxis);
    return d->defaultValue;
}

/*!
    Sets the default value of this QFontVariableAxis to \a defaultValue.

    \note Typically, there will be no need to call this function as it will not affect the font
    itself, only this particular representation.

    \sa defaultValue()
*/
void QFontVariableAxis::setDefaultValue(qreal defaultValue)
{
    if (d_func()->defaultValue == defaultValue)
        return;
    detach();
    Q_D(QFontVariableAxis);
    d->defaultValue = defaultValue;
}

/*!
    Assigns the given \a axis to this QFontVariableAxis.

    \sa QFontVariableAxis()
*/
QFontVariableAxis &QFontVariableAxis::operator=(const QFontVariableAxis &axis)
{
    QFontVariableAxis copy(axis);
    swap(copy);
    return *this;
}

/*!
    \internal
 */
void QFontVariableAxis::detach()
{
    d_ptr.detach();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QFontVariableAxis &axis)
{
    QDebugStateSaver save(debug);

    debug.nospace().noquote();
    const QString name = axis.name();
    if (!name.isEmpty())
        debug << name << '(';

    debug << axis.tag();
    if (!name.isEmpty())
        debug << ')';
    debug << '[' << axis.minimumValue() << "..." << axis.maximumValue()
          << "; default=" << axis.defaultValue() << ']';

    return debug;
}
#endif

QT_END_NAMESPACE
