/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qsizepolicy.h"

#include <qdatastream.h>
#include <qdebug.h>
#include <qvariant.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSizePolicy
    \brief The QSizePolicy class is a layout attribute describing horizontal
    and vertical resizing policy.

    \ingroup geomanagement
    \inmodule QtWidgets

    The size policy of a widget is an expression of its willingness to
    be resized in various ways, and affects how the widget is treated
    by the \l{Layout Management}{layout engine}. Each widget returns a
    QSizePolicy that describes the horizontal and vertical resizing
    policy it prefers when being laid out. You can change this for
    a specific widget by changing its QWidget::sizePolicy property.

    QSizePolicy contains two independent QSizePolicy::Policy values
    and two stretch factors; one describes the widgets's horizontal
    size policy, and the other describes its vertical size policy. It
    also contains a flag to indicate whether the height and width of
    its preferred size are related.

    The horizontal and vertical policies can be set in the
    constructor, and altered using the setHorizontalPolicy() and
    setVerticalPolicy() functions. The stretch factors can be set
    using the setHorizontalStretch() and setVerticalStretch()
    functions. The flag indicating whether the widget's
    \l{QWidget::sizeHint()}{sizeHint()} is width-dependent (such as a
    menu bar or a word-wrapping label) can be set using the
    setHeightForWidth() function.

    The current size policies and stretch factors be retrieved using
    the horizontalPolicy(), verticalPolicy(), horizontalStretch() and
    verticalStretch() functions. Alternatively, use the transpose()
    function to swap the horizontal and vertical policies and
    stretches. The hasHeightForWidth() function returns the current
    status of the flag indicating the size hint dependencies.

    Use the expandingDirections() function to determine whether the
    associated widget can make use of more space than its
    \l{QWidget::sizeHint()}{sizeHint()} function indicates, as well as
    find out in which directions it can expand.

    Finally, the QSizePolicy class provides operators comparing this
    size policy to a given policy, as well as a QVariant operator
    storing this QSizePolicy as a QVariant object.

    \sa QSize, QWidget::sizeHint(), QWidget::sizePolicy,
    QLayoutItem::sizeHint()
*/

/*!
    \enum QSizePolicy::PolicyFlag

    These flags are combined together to form the various \l{Policy}
    values:

    \value GrowFlag  The widget can grow beyond its size hint if necessary.
    \value ExpandFlag  The widget should get as much space as possible.
    \value ShrinkFlag  The widget can shrink below its size hint if necessary.
    \value IgnoreFlag  The widget's size hint is ignored. The widget will get
        as much space as possible.

    \sa Policy
*/

/*!
    \enum QSizePolicy::Policy

    This enum describes the various per-dimension sizing types used
    when constructing a QSizePolicy.

    \value Fixed  The QWidget::sizeHint() is the only acceptable
        alternative, so the widget can never grow or shrink (e.g. the
        vertical direction of a push button).

    \value Minimum  The sizeHint() is minimal, and sufficient. The
        widget can be expanded, but there is no advantage to it being
        larger (e.g. the horizontal direction of a push button).
        It cannot be smaller than the size provided by sizeHint().

    \value Maximum  The sizeHint() is a maximum. The widget can be
        shrunk any amount without detriment if other widgets need the
        space (e.g. a separator line).
        It cannot be larger than the size provided by sizeHint().

    \value Preferred  The sizeHint() is best, but the widget can be
        shrunk and still be useful. The widget can be expanded, but there
        is no advantage to it being larger than sizeHint() (the default
        QWidget policy).

    \value Expanding  The sizeHint() is a sensible size, but the
        widget can be shrunk and still be useful. The widget can make use
        of extra space, so it should get as much space as possible (e.g.
        the horizontal direction of a horizontal slider).

    \value MinimumExpanding  The sizeHint() is minimal, and sufficient.
        The widget can make use of extra space, so it should get as much
        space as possible (e.g. the horizontal direction of a horizontal
        slider).

    \value Ignored  The sizeHint() is ignored. The widget will get as
        much space as possible.

    \sa PolicyFlag, setHorizontalPolicy(), setVerticalPolicy()
*/

/*!
    \fn QSizePolicy::QSizePolicy()

    Constructs a QSizePolicy object with \l Fixed as its horizontal
    and vertical policies.

    The policies can be altered using the setHorizontalPolicy() and
    setVerticalPolicy() functions. Use the setHeightForWidth()
    function if the preferred height of the widget is dependent on the
    width of the widget (for example, a QLabel with line wrapping).

    \sa setHorizontalStretch(), setVerticalStretch()
*/

/*!
    \fn QSizePolicy::QSizePolicy(Policy horizontal, Policy vertical, ControlType type)
    \since 4.3

    Constructs a QSizePolicy object with the given \a horizontal and
    \a vertical policies, and the specified control \a type.

    Use setHeightForWidth() if the preferred height of the widget is
    dependent on the width of the widget (for example, a QLabel with
    line wrapping).

    \sa setHorizontalStretch(), setVerticalStretch(), controlType()
*/

/*!
    \fn QSizePolicy::Policy QSizePolicy::horizontalPolicy() const

    Returns the horizontal component of the size policy.

    \sa setHorizontalPolicy(), verticalPolicy(), horizontalStretch()
*/

/*!
    \fn QSizePolicy::Policy QSizePolicy::verticalPolicy() const

    Returns the vertical component of the size policy.

    \sa setVerticalPolicy(), horizontalPolicy(), verticalStretch()
*/

/*!
    \fn void QSizePolicy::setHorizontalPolicy(Policy policy)

    Sets the horizontal component to the given \a policy.

    \sa horizontalPolicy(), setVerticalPolicy(), setHorizontalStretch()
*/

/*!
    \fn void QSizePolicy::setVerticalPolicy(Policy policy)

    Sets the vertical component to the given \a policy.

    \sa verticalPolicy(), setHorizontalPolicy(), setVerticalStretch()
*/

/*!
    \fn Qt::Orientations QSizePolicy::expandingDirections() const

    Returns whether a widget can make use of more space than the
    QWidget::sizeHint() function indicates.

    A value of Qt::Horizontal or Qt::Vertical means that the widget
    can grow horizontally or vertically (i.e., the horizontal or
    vertical policy is \l Expanding or \l MinimumExpanding), whereas
    Qt::Horizontal | Qt::Vertical means that it can grow in both
    dimensions.

    \sa horizontalPolicy(), verticalPolicy()
*/

/*!
    \since 4.3

    Returns the control type associated with the widget for which
    this size policy applies.
*/
QSizePolicy::ControlType QSizePolicy::controlType() const
{
    return QSizePolicy::ControlType(1 << bits.ctype);
}


/*!
    \since 4.3

    Sets the control type associated with the widget for which this
    size policy applies to \a type.

    The control type specifies the type of the widget for which this
    size policy applies. It is used by some styles, notably
    QMacStyle, to insert proper spacing between widgets. For example,
    the \macos Aqua guidelines specify that push buttons should be
    separated by 12 pixels, whereas vertically stacked radio buttons
    only require 6 pixels.

    \sa QStyle::layoutSpacing()
*/
void QSizePolicy::setControlType(ControlType type)
{
    /*
        The control type is a flag type, with values 0x1, 0x2, 0x4, 0x8, 0x10,
        etc. In memory, we pack it onto the available bits (CTSize) in
        setControlType(), and unpack it here.

        Example:

            0x00000001 maps to 0
            0x00000002 maps to 1
            0x00000004 maps to 2
            0x00000008 maps to 3
            etc.
    */

    int i = 0;
    while (true) {
        if (type & (0x1 << i)) {
            bits.ctype = i;
            return;
        }
        ++i;
    }
}

/*!
    \fn void QSizePolicy::setHeightForWidth(bool dependent)

    Sets the flag determining whether the widget's preferred height
    depends on its width, to \a dependent.

    \sa hasHeightForWidth(), setWidthForHeight()
*/

/*!
    \fn bool QSizePolicy::hasHeightForWidth() const

    Returns \c true if the widget's preferred height depends on its
    width; otherwise returns \c false.

    \sa setHeightForWidth()
*/

/*!
    \fn void QSizePolicy::setWidthForHeight(bool dependent)

    Sets the flag determining whether the widget's width
    depends on its height, to \a dependent.

    This is only supported for QGraphicsLayout's subclasses.
    It is not possible to have a layout with both height-for-width
    and width-for-height constraints at the same time.

    \sa hasWidthForHeight(), setHeightForWidth()
*/

/*!
    \fn bool QSizePolicy::hasWidthForHeight() const

    Returns \c true if the widget's width depends on its
    height; otherwise returns \c false.

    \sa setWidthForHeight()
*/

/*!
    \fn bool QSizePolicy::operator==(const QSizePolicy &other) const

    Returns \c true if this policy is equal to \a other; otherwise
    returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QSizePolicy::operator!=(const QSizePolicy &other) const

    Returns \c true if this policy is different from \a other; otherwise
    returns \c false.

    \sa operator==()
*/

/*!
    \fn uint qHash(QSizePolicy key, uint seed = 0)
    \since 5.6
    \relates QSizePolicy

    Returns the hash value for \a key, using
    \a seed to seed the calculation.
*/

/*!
    \fn int QSizePolicy::horizontalStretch() const

    Returns the horizontal stretch factor of the size policy.

    \sa setHorizontalStretch(), verticalStretch(), horizontalPolicy()
*/

/*!
    \fn int QSizePolicy::verticalStretch() const

    Returns the vertical stretch factor of the size policy.

    \sa setVerticalStretch(), horizontalStretch(), verticalPolicy()
*/

/*!
    \fn void QSizePolicy::setHorizontalStretch(int stretchFactor)

    Sets the horizontal stretch factor of the size policy to the given \a
    stretchFactor. \a stretchFactor must be in the range [0,255].

    When two widgets are adjacent to each other in a horizontal layout,
    setting the horizontal stretch factor of the widget on the left to 2
    and the factor of widget on the right to 1 will ensure that the widget
    on the left will always be twice the size of the one on the right.

    \sa horizontalStretch(), setVerticalStretch(), setHorizontalPolicy()
*/

/*!
    \fn void QSizePolicy::setVerticalStretch(int stretchFactor)

    Sets the vertical stretch factor of the size policy to the given
    \a stretchFactor. \a stretchFactor must be in the range [0,255].

    When two widgets are adjacent to each other in a vertical layout,
    setting the vertical stretch factor of the widget on the top to 2
    and the factor of widget on the bottom to 1 will ensure that
    the widget on the top will always be twice the size of the one
    on the bottom.

    \sa verticalStretch(), setHorizontalStretch(), setVerticalPolicy()
*/

/*!
    \fn void QSizePolicy::transpose()

    Swaps the horizontal and vertical policies and stretches.
*/

/*!
    \fn void QSizePolicy::retainSizeWhenHidden() const
    \since 5.2

    Returns whether the layout should retain the widget's size when it is hidden.
    This is \c false by default.

    \sa setRetainSizeWhenHidden()
*/

/*!
   \fn void QSizePolicy::setRetainSizeWhenHidden(bool retainSize)
   \since 5.2

    Sets whether a layout should retain the widget's size when it is hidden.
    If \a retainSize is \c true, the layout will not be changed by hiding the widget.

    \sa retainSizeWhenHidden()
*/

/*!
    \enum QSizePolicy::ControlType
    \since 4.3

    This enum specifies the different types of widgets in terms of
    layout interaction:

    \value DefaultType  The default type, when none is specified.
    \value ButtonBox  A QDialogButtonBox instance.
    \value CheckBox  A QCheckBox instance.
    \value ComboBox  A QComboBox instance.
    \value Frame  A QFrame instance.
    \value GroupBox  A QGroupBox instance.
    \value Label  A QLabel instance.
    \value Line  A QFrame instance with QFrame::HLine or QFrame::VLine.
    \value LineEdit  A QLineEdit instance.
    \value PushButton  A QPushButton instance.
    \value RadioButton  A QRadioButton instance.
    \value Slider  A QAbstractSlider instance.
    \value SpinBox  A QAbstractSpinBox instance.
    \value TabWidget  A QTabWidget instance.
    \value ToolButton  A QToolButton instance.

    \sa setControlType(), controlType()
*/

/*!
   Returns a QVariant storing this QSizePolicy.
*/
QSizePolicy::operator QVariant() const
{
    return QVariant(QVariant::SizePolicy, this);
}

#ifndef QT_NO_DATASTREAM

/*!
    \relates QSizePolicy
    \since 4.2

    Writes the size \a policy to the data stream \a stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator<<(QDataStream &stream, const QSizePolicy &policy)
{
    // The order here is for historical reasons. (compatibility with Qt4)
    quint32 data = (policy.bits.horPolicy |         // [0, 3]
                    policy.bits.verPolicy << 4 |    // [4, 7]
                    policy.bits.hfw << 8 |          // [8]
                    policy.bits.ctype << 9 |        // [9, 13]
                    policy.bits.wfh << 14 |         // [14]
                    policy.bits.retainSizeWhenHidden << 15 |     // [15]
                    policy.bits.verStretch << 16 |  // [16, 23]
                    policy.bits.horStretch << 24);  // [24, 31]
    return stream << data;
}

#define VALUE_OF_BITS(data, bitstart, bitcount) ((data >> bitstart) & ((1 << bitcount) -1))

/*!
    \relates QSizePolicy
    \since 4.2

    Reads the size \a policy from the data stream \a stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator>>(QDataStream &stream, QSizePolicy &policy)
{
    quint32 data;
    stream >> data;
    policy.bits.horPolicy =  VALUE_OF_BITS(data, 0, 4);
    policy.bits.verPolicy =  VALUE_OF_BITS(data, 4, 4);
    policy.bits.hfw =        VALUE_OF_BITS(data, 8, 1);
    policy.bits.ctype =      VALUE_OF_BITS(data, 9, 5);
    policy.bits.wfh =        VALUE_OF_BITS(data, 14, 1);
    policy.bits.retainSizeWhenHidden =    VALUE_OF_BITS(data, 15, 1);
    policy.bits.verStretch = VALUE_OF_BITS(data, 16, 8);
    policy.bits.horStretch = VALUE_OF_BITS(data, 24, 8);
    return stream;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSizePolicy &p)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QSizePolicy(horizontalPolicy = " << p.horizontalPolicy()
                  << ", verticalPolicy = " << p.verticalPolicy() << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE

#include "moc_qsizepolicy.cpp"
