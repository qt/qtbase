/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "rangecontrols.h"

#include <qslider.h>
#include <qdial.h>
#include <qspinbox.h>
#include <qscrollbar.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qdebug.h>
#include <qglobal.h>
#include <QDoubleSpinBox>
#include <QDial>
#include <qmath.h>
#include <private/qmath_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY
extern QString Q_GUI_EXPORT qt_accStripAmp(const QString &text);
#ifndef QT_NO_SCROLLBAR
extern QStyleOptionSlider Q_GUI_EXPORT qt_qscrollbarStyleOption(QScrollBar *scrollBar);
#endif
#ifndef QT_NO_SLIDER
extern QStyleOptionSlider Q_GUI_EXPORT qt_qsliderStyleOption(QSlider *slider);
#endif

#ifndef QT_NO_SPINBOX
QAccessibleAbstractSpinBox::QAccessibleAbstractSpinBox(QWidget *w)
: QAccessibleWidgetEx(w, SpinBox)
{
    Q_ASSERT(abstractSpinBox());
}

/*!
    Returns the underlying QAbstractSpinBox.
*/
QAbstractSpinBox *QAccessibleAbstractSpinBox::abstractSpinBox() const
{
    return qobject_cast<QAbstractSpinBox*>(object());
}

/*! \reimp */
int QAccessibleAbstractSpinBox::childCount() const
{
    return ValueDown;
}

/*! \reimp */
QRect QAccessibleAbstractSpinBox::rect(int child) const
{
    QRect rect;
    if (!abstractSpinBox()->isVisible())
        return rect;
    QStyleOptionSpinBox so;
    so.rect = widget()->rect();
    switch(child) {
    case Editor:
        rect = widget()->style()->subControlRect(QStyle::CC_SpinBox, &so,
                                                 QStyle::SC_SpinBoxEditField, widget());
        break;
    case ValueUp:
        rect = widget()->style()->subControlRect(QStyle::CC_SpinBox, &so,
                                                 QStyle::SC_SpinBoxUp, widget());
        break;
    case ValueDown:
        rect = widget()->style()->subControlRect(QStyle::CC_SpinBox, &so,
                                                 QStyle::SC_SpinBoxDown, widget());
        break;
    default:
        rect = so.rect;
        break;
    }
    QPoint tl = widget()->mapToGlobal(QPoint(0, 0));
    return QRect(tl.x() + rect.x(), tl.y() + rect.y(), rect.width(), rect.height());
}

/*! \reimp */
int QAccessibleAbstractSpinBox::navigate(RelationFlag rel, int entry, QAccessibleInterface **target) const
{
    *target = 0;

    if (entry) switch (rel) {
    case Child:
        return entry <= childCount() ? entry : -1;
    case QAccessible::Left:
        return (entry == ValueUp || entry == ValueDown) ? Editor : -1;
    case QAccessible::Right:
        return entry == Editor ? ValueUp : -1;
    case QAccessible::Up:
        return entry == ValueDown ? ValueUp : -1;
    case QAccessible::Down:
        return entry == ValueUp ? ValueDown : -1;
    default:
        break;
    }
    return QAccessibleWidgetEx::navigate(rel, entry, target);
}

/*! \reimp */
QString QAccessibleAbstractSpinBox::text(Text t, int child) const
{
    if (!abstractSpinBox()->isVisible())
        return QString();
    switch (t) {
    case Name:
        switch (child) {
        case ValueUp:
            return QSpinBox::tr("More");
        case ValueDown:
            return QSpinBox::tr("Less");
        }
        break;
    case Value:
        if (child == Editor || child == SpinBoxSelf)
            return abstractSpinBox()->text();
        break;
    default:
        break;
    }
    return QAccessibleWidgetEx::text(t, 0);
}

/*! \reimp */
QAccessible::Role QAccessibleAbstractSpinBox::role(int child) const
{
    switch(child) {
    case Editor:
        return EditableText;
    case ValueUp:
    case ValueDown:
        return PushButton;
    default:
        break;
    }
    return QAccessibleWidgetEx::role(child);
}

/*! \reimp */
bool QAccessibleAbstractSpinBox::doAction(int action, int child, const QVariantList &params)
{
    if (!widget()->isEnabled())
        return false;

    if (action == Press) {
        switch(child) {
        case ValueUp:
            abstractSpinBox()->stepUp();
            return true;
        case ValueDown:
            abstractSpinBox()->stepDown();
            return true;
        default:
            break;
        }
    }
    return QAccessibleWidgetEx::doAction(action, 0, params);
}

QVariant QAccessibleAbstractSpinBox::currentValue()
{
    QVariant result = abstractSpinBox()->property("value");
    QVariant::Type type = result.type();

    // IA2 only allows numeric types
    if (type == QVariant::Int || type == QVariant::UInt || type == QVariant::LongLong
        || type == QVariant::ULongLong || type == QVariant::Double)
        return result;

    return QVariant();
}

void QAccessibleAbstractSpinBox::setCurrentValue(const QVariant &value)
{
    abstractSpinBox()->setProperty("value", value);
}

QVariant QAccessibleAbstractSpinBox::maximumValue()
{
    return abstractSpinBox()->property("maximum");
}

QVariant QAccessibleAbstractSpinBox::minimumValue()
{
    return abstractSpinBox()->property("minimum");
}

QVariant QAccessibleAbstractSpinBox::invokeMethodEx(Method method, int child, const QVariantList &params)
{
    switch (method) {
    case ListSupportedMethods: {
        QSet<QAccessible::Method> set;
        set << ListSupportedMethods;
        return QVariant::fromValue(set | qvariant_cast<QSet<QAccessible::Method> >(
                    QAccessibleWidgetEx::invokeMethodEx(method, child, params)));
    }
    default:
        return QAccessibleWidgetEx::invokeMethodEx(method, child, params);
    }
}


/*!
  \class QAccessibleSpinBox
  \brief The QAccessibleSpinBox class implements the QAccessibleInterface for spinbox widgets.
  \internal

  \ingroup accessibility
*/

/*!
    \enum QAccessibleAbstractSpinBox::SpinBoxElements

    This enum identifies the components of the spin box.

    \value SpinBoxSelf The spin box as a whole
    \value Editor The line edit sub-widget.
    \value ValueUp The up sub-widget (i.e. the up arrow or + button)
    \value ValueDown The down sub-widget (i.e. the down arrow or - button)
*/

/*!
  Constructs a QAccessibleSpinWidget object for \a w.
*/
QAccessibleSpinBox::QAccessibleSpinBox(QWidget *w)
: QAccessibleAbstractSpinBox(w)
{
    Q_ASSERT(spinBox());
    addControllingSignal(QLatin1String("valueChanged(int)"));
    addControllingSignal(QLatin1String("valueChanged(QString)"));
}

/*!
    Returns the underlying QSpinBox.
*/
QSpinBox *QAccessibleSpinBox::spinBox() const
{
    return qobject_cast<QSpinBox*>(object());
}

/*! \reimp */
QAccessible::State QAccessibleSpinBox::state(int child) const
{
    State state = QAccessibleAbstractSpinBox::state(child);
    switch(child) {
    case ValueUp:
        if (spinBox()->value() >= spinBox()->maximum())
            state |= Unavailable;
        return state;
    case ValueDown:
        if (spinBox()->value() <= spinBox()->minimum())
            state |= Unavailable;
        return state;
    default:
        break;
    }
    return state;
}

/*! \reimp */
bool QAccessibleSpinBox::doAction(int action, int child, const QVariantList &params)
{
    if (!widget()->isEnabled())
        return false;

    if (action == Press) {
        switch(child) {
        case ValueUp:
            if (spinBox()->value() >= spinBox()->maximum())
                return false;
            spinBox()->stepUp();
            return true;
        case ValueDown:
            if (spinBox()->value() <= spinBox()->minimum())
                return false;
            spinBox()->stepDown();
            return true;
        default:
            break;
        }
    }
    return QAccessibleAbstractSpinBox::doAction(action, 0, params);
}

// ================================== QAccessibleDoubleSpinBox ==================================
QAccessibleDoubleSpinBox::QAccessibleDoubleSpinBox(QWidget *widget)
    : QAccessibleWidgetEx(widget, SpinBox)
{
    Q_ASSERT(qobject_cast<QDoubleSpinBox *>(widget));
    addControllingSignal(QLatin1String("valueChanged(double)"));
    addControllingSignal(QLatin1String("valueChanged(QString)"));
}

/*!
    Returns the underlying QDoubleSpinBox.
*/
QDoubleSpinBox *QAccessibleDoubleSpinBox::doubleSpinBox() const
{
    return static_cast<QDoubleSpinBox*>(object());
}

/*! \reimp */
int QAccessibleDoubleSpinBox::childCount() const
{
    return ValueDown;
}

/*! \reimp */
QRect QAccessibleDoubleSpinBox::rect(int child) const
{
    QRect rect;
    if (!doubleSpinBox()->isVisible())
        return rect;
    QStyleOptionSpinBox spinBoxOption;
    spinBoxOption.initFrom(doubleSpinBox());
    switch (child) {
    case Editor:
        rect = doubleSpinBox()->style()->subControlRect(QStyle::CC_SpinBox, &spinBoxOption,
                                                 QStyle::SC_SpinBoxEditField, doubleSpinBox());
        break;
    case ValueUp:
        rect = doubleSpinBox()->style()->subControlRect(QStyle::CC_SpinBox, &spinBoxOption,
                                                 QStyle::SC_SpinBoxUp, doubleSpinBox());
        break;
    case ValueDown:
        rect = doubleSpinBox()->style()->subControlRect(QStyle::CC_SpinBox, &spinBoxOption,
                                                 QStyle::SC_SpinBoxDown, doubleSpinBox());
        break;
    default:
        rect = spinBoxOption.rect;
        break;
    }
    const QPoint globalPos = doubleSpinBox()->mapToGlobal(QPoint(0, 0));
    return QRect(globalPos.x() + rect.x(), globalPos.y() + rect.y(), rect.width(), rect.height());
}

/*! \reimp */
int QAccessibleDoubleSpinBox::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    if (entry <= 0)
        return QAccessibleWidgetEx::navigate(relation, entry, target);

    *target = 0;
    switch (relation) {
    case Child:
        return entry <= childCount() ? entry : -1;
    case QAccessible::Left:
        return (entry == ValueUp || entry == ValueDown) ? Editor : -1;
    case QAccessible::Right:
        return entry == Editor ? ValueUp : -1;
    case QAccessible::Up:
        return entry == ValueDown ? ValueUp : -1;
    case QAccessible::Down:
        return entry == ValueUp ? ValueDown : -1;
    default:
        break;
    }
    return QAccessibleWidgetEx::navigate(relation, entry, target);
}

QVariant QAccessibleDoubleSpinBox::invokeMethodEx(QAccessible::Method, int, const QVariantList &)
{
    return QVariant();
}

/*! \reimp */
QString QAccessibleDoubleSpinBox::text(Text textType, int child) const
{
    switch (textType) {
    case Name:
        if (child == ValueUp)
            return QDoubleSpinBox::tr("More");
        else if (child == ValueDown)
            return QDoubleSpinBox::tr("Less");
        break;
    case Value:
        if (child == Editor || child == SpinBoxSelf)
            return doubleSpinBox()->textFromValue(doubleSpinBox()->value());
        break;
    default:
        break;
    }
    return QAccessibleWidgetEx::text(textType, 0);
}

/*! \reimp */
QAccessible::Role QAccessibleDoubleSpinBox::role(int child) const
{
    switch (child) {
    case Editor:
        return EditableText;
    case ValueUp:
    case ValueDown:
        return PushButton;
    default:
        break;
    }
    return QAccessibleWidgetEx::role(child);
}

/*! \reimp */
QAccessible::State QAccessibleDoubleSpinBox::state(int child) const
{
    State state = QAccessibleWidgetEx::state(child);
    switch (child) {
    case ValueUp:
        if (doubleSpinBox()->value() >= doubleSpinBox()->maximum())
            state |= Unavailable;
        break;
    case ValueDown:
        if (doubleSpinBox()->value() <= doubleSpinBox()->minimum())
            state |= Unavailable;
        break;
    default:
        break;
    }
    return state;
}
#endif // QT_NO_SPINBOX

#ifndef QT_NO_SCROLLBAR
/*!
  \class QAccessibleScrollBar
  \brief The QAccessibleScrollBar class implements the QAccessibleInterface for scroll bars.
  \internal

  \ingroup accessibility
*/

/*!
    \enum QAccessibleScrollBar::ScrollBarElements

    This enum identifies the components of the scroll bar.

    \value ScrollBarSelf The scroll bar as a whole.
    \value LineUp The up arrow button.
    \value PageUp The area between the position and the up arrow button.
    \value Position The position marking rectangle.
    \value PageDown The area between the position and the down arrow button.
    \value LineDown The down arrow button.
*/

/*!
  Constructs a QAccessibleScrollBar object for \a w.
  \a name is propagated to the QAccessibleWidgetEx constructor.
*/
QAccessibleScrollBar::QAccessibleScrollBar(QWidget *w)
: QAccessibleAbstractSlider(w, ScrollBar)
{
    Q_ASSERT(scrollBar());
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

/*! Returns the scroll bar. */
QScrollBar *QAccessibleScrollBar::scrollBar() const
{
    return qobject_cast<QScrollBar*>(object());
}

/*! \reimp */
QRect QAccessibleScrollBar::rect(int child) const
{
    if (!scrollBar()->isVisible())
        return QRect();

    QStyle::SubControl subControl;
    switch (child) {
    case LineUp:
        subControl = QStyle ::SC_ScrollBarSubLine;
        break;
    case PageUp:
        subControl = QStyle::SC_ScrollBarSubPage;
        break;
    case Position:
        subControl = QStyle::SC_ScrollBarSlider;
        break;
    case PageDown:
        subControl = QStyle::SC_ScrollBarAddPage;
        break;
    case LineDown:
        subControl = QStyle::SC_ScrollBarAddLine;
        break;
    default:
        return QAccessibleAbstractSlider::rect(child);
    }

    const QStyleOptionSlider option = qt_qscrollbarStyleOption(scrollBar());
    const QRect rect = scrollBar()->style()->subControlRect(QStyle::CC_ScrollBar, &option,
                                                       subControl, scrollBar());
    const QPoint tp = scrollBar()->mapToGlobal(QPoint(0,0));
    return QRect(tp.x() + rect.x(), tp.y() + rect.y(), rect.width(), rect.height());
}

/*! \reimp */
int QAccessibleScrollBar::childCount() const
{
    return LineDown;
}

/*! \reimp */
QString QAccessibleScrollBar::text(Text t, int child) const
{
    switch (t) {
    case Value:
        if (!child || child == Position)
            return QString::number(scrollBar()->value());
        return QString();
    case Name:
        switch (child) {
        case LineUp:
            return QScrollBar::tr("Line up");
        case PageUp:
            return QScrollBar::tr("Page up");
        case Position:
            return QScrollBar::tr("Position");
        case PageDown:
            return QScrollBar::tr("Page down");
        case LineDown:
            return QScrollBar::tr("Line down");
        }
        break;
    default:
        break;
    }
    return QAccessibleAbstractSlider::text(t, child);
}

/*! \reimp */
QAccessible::Role QAccessibleScrollBar::role(int child) const
{
    switch (child) {
    case LineUp:
    case PageUp:
    case PageDown:
    case LineDown:
        return PushButton;
    case Position:
        return Indicator;
    default:
        return ScrollBar;
    }
}

/*! \reimp */
QAccessible::State QAccessibleScrollBar::state(int child) const
{
    const State parentState = QAccessibleAbstractSlider::state(0);

    if (child == 0)
        return parentState;

    // Inherit the Invisible state from parent.
    State state = parentState & QAccessible::Invisible;

    // Disable left/right if we are at the minimum/maximum.
    const QScrollBar * const scrollBar = QAccessibleScrollBar::scrollBar();
    switch (child) {
    case LineUp:
    case PageUp:
        if (scrollBar->value() <= scrollBar->minimum())
            state |= Unavailable;
        break;
    case LineDown:
    case PageDown:
        if (scrollBar->value() >= scrollBar->maximum())
            state |= Unavailable;
        break;
    case Position:
    default:
        break;
    }

    return state;
}
#endif // QT_NO_SCROLLBAR

#ifndef QT_NO_SLIDER
/*!
  \class QAccessibleSlider
  \brief The QAccessibleSlider class implements the QAccessibleInterface for sliders.
  \internal

  \ingroup accessibility
*/

/*!
    \enum QAccessibleSlider::SliderElements

    This enum identifies the components of the slider.

    \value SliderSelf The slider as a whole.
    \value PageLeft The area to the left of the position.
    \value Position The position indicator.
    \value PageRight The area to the right of the position.
*/

/*!
  Constructs a QAccessibleScrollBar object for \a w.
  \a name is propagated to the QAccessibleWidgetEx constructor.
*/
QAccessibleSlider::QAccessibleSlider(QWidget *w)
: QAccessibleAbstractSlider(w)
{
    Q_ASSERT(slider());
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

/*! Returns the slider. */
QSlider *QAccessibleSlider::slider() const
{
    return qobject_cast<QSlider*>(object());
}

/*! \reimp */
QRect QAccessibleSlider::rect(int child) const
{
    QRect rect;
    if (!slider()->isVisible())
        return rect;
    const QStyleOptionSlider option = qt_qsliderStyleOption(slider());
    QRect srect = slider()->style()->subControlRect(QStyle::CC_Slider, &option,
                                                    QStyle::SC_SliderHandle, slider());

    switch (child) {
    case PageLeft:
        if (slider()->orientation() == Qt::Vertical)
            rect = QRect(0, 0, slider()->width(), srect.y());
        else
            rect = QRect(0, 0, srect.x(), slider()->height());
        break;
    case Position:
        rect = srect;
        break;
    case PageRight:
        if (slider()->orientation() == Qt::Vertical)
            rect = QRect(0, srect.y() + srect.height(), slider()->width(), slider()->height()- srect.y() - srect.height());
        else
            rect = QRect(srect.x() + srect.width(), 0, slider()->width() - srect.x() - srect.width(), slider()->height());
        break;
    default:
        return QAccessibleAbstractSlider::rect(child);
    }

    QPoint tp = slider()->mapToGlobal(QPoint(0,0));
    return QRect(tp.x() + rect.x(), tp.y() + rect.y(), rect.width(), rect.height());
}

/*! \reimp */
int QAccessibleSlider::childCount() const
{
    return PageRight;
}

/*! \reimp */
QString QAccessibleSlider::text(Text t, int child) const
{
    switch (t) {
    case Value:
        if (!child || child == 2)
            return QString::number(slider()->value());
        return QString();
    case Name:
        switch (child) {
        case PageLeft:
            return slider()->orientation() == Qt::Horizontal ?
                QSlider::tr("Page left") : QSlider::tr("Page up");
        case Position:
            return QSlider::tr("Position");
        case PageRight:
            return slider()->orientation() == Qt::Horizontal ?
                QSlider::tr("Page right") : QSlider::tr("Page down");
        }
        break;
    default:
        break;
    }
    return QAccessibleAbstractSlider::text(t, child);
}

/*! \reimp */
QAccessible::Role QAccessibleSlider::role(int child) const
{
    switch (child) {
    case PageLeft:
    case PageRight:
        return PushButton;
    case Position:
        return Indicator;
    default:
        return Slider;
    }
}

/*! \reimp */
QAccessible::State QAccessibleSlider::state(int child) const
{
    const State parentState = QAccessibleAbstractSlider::state(0);

    if (child == 0)
        return parentState;

    // Inherit the Invisible state from parent.
    State state = parentState & QAccessible::Invisible;

    // Disable left/right if we are at the minimum/maximum.
    const QSlider * const slider = QAccessibleSlider::slider();
    switch (child) {
    case PageLeft:
        if (slider->value() <= slider->minimum())
            state |= Unavailable;
        break;
    case PageRight:
        if (slider->value() >= slider->maximum())
            state |= Unavailable;
        break;
    case Position:
    default:
        break;
    }

    return state;
}

/*!
    \fn int QAccessibleSlider::defaultAction(int child) const

    Returns the default action for the given \a child. The base class
    implementation returns 0.
*/
int QAccessibleSlider::defaultAction(int /*child*/) const
{
/*
    switch (child) {
    case SliderSelf:
        return SetFocus;
    case PageLeft:
        return Press;
    case PageRight:
        return Press;
    }
*/
    return 0;
}

/*! \internal */
QString QAccessibleSlider::actionText(int /*action*/, Text /*t*/, int /*child*/) const
{
    return QLatin1String("");
}

QAccessibleAbstractSlider::QAccessibleAbstractSlider(QWidget *w, Role r)
    : QAccessibleWidgetEx(w, r)
{
    Q_ASSERT(qobject_cast<QAbstractSlider *>(w));
}

QVariant QAccessibleAbstractSlider::invokeMethodEx(Method method, int child, const QVariantList &params)
{
    switch (method) {
    case ListSupportedMethods: {
        QSet<QAccessible::Method> set;
        set << ListSupportedMethods;
        return QVariant::fromValue(set | qvariant_cast<QSet<QAccessible::Method> >(
                    QAccessibleWidgetEx::invokeMethodEx(method, child, params)));
    }
    default:
        return QAccessibleWidgetEx::invokeMethodEx(method, child, params);
    }
}

QVariant QAccessibleAbstractSlider::currentValue()
{
    return abstractSlider()->value();
}

void QAccessibleAbstractSlider::setCurrentValue(const QVariant &value)
{
    abstractSlider()->setValue(value.toInt());
}

QVariant QAccessibleAbstractSlider::maximumValue()
{
    return abstractSlider()->maximum();
}

QVariant QAccessibleAbstractSlider::minimumValue()
{
    return abstractSlider()->minimum();
}

QAbstractSlider *QAccessibleAbstractSlider::abstractSlider() const
{
    return static_cast<QAbstractSlider *>(object());
}

#endif // QT_NO_SLIDER

#ifndef QT_NO_DIAL
// ======================================= QAccessibleDial ======================================
QAccessibleDial::QAccessibleDial(QWidget *widget)
    : QAccessibleWidgetEx(widget, Dial)
{
    Q_ASSERT(qobject_cast<QDial *>(widget));
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

QRect QAccessibleDial::rect(int child) const
{
    QRect rect;
    if (!dial()->isVisible())
        return rect;
    switch (child) {
    case Self:
        return QAccessibleWidgetEx::rect(child);
    case SpeedoMeter: {
        // Mixture from qcommonstyle.cpp (focus rect).
        int width = dial()->width();
        int height = dial()->height();
        qreal radius = qMin(width, height) / 2.0;
        qreal delta = radius / 6.0;
        qreal dx = delta + (width - 2 * radius) / 2.0;
        qreal dy = delta + (height - 2 * radius) / 2.0;
        rect = QRect(int(dx), int(dy), int(radius * 2 - 2 * delta), int(radius * 2 - 2 * delta));
        if (dial()->notchesVisible()) {
            rect.translate(int(-radius / 6), int(-radius / 6));
            rect.setWidth(rect.width() + int(radius / 3));
            rect.setHeight(rect.height() + int(radius / 3));
        }
        break;
    }
    case SliderHandle: {
        // Mixture from qcommonstyle.cpp and qdial.cpp.
        int sliderValue = !dial()->invertedAppearance() ? dial()->value()
                                                        : (dial()->maximum() - dial()->value());
        qreal angle = 0;
        if (dial()->maximum() == dial()->minimum()) {
            angle = Q_PI / 2;
        } else if (dial()->wrapping()) {
            angle = Q_PI * 3 / 2 - (sliderValue - dial()->minimum()) * 2 * Q_PI
                    / (dial()->maximum() - dial()->minimum());
        } else {
            angle = (Q_PI * 8 - (sliderValue - dial()->minimum()) * 10 * Q_PI
                    / (dial()->maximum() - dial()->minimum())) / 6;
        }

        int width = dial()->rect().width();
        int height = dial()->rect().height();
        int radius = qMin(width, height) / 2;
        int xc = width / 2;
        int yc = height / 2;
        int bigLineSize = radius / 6;
        if (bigLineSize < 4)
            bigLineSize = 4;
        if (bigLineSize > radius / 2)
            bigLineSize = radius / 2;
        int len = radius - bigLineSize - 5;
        if (len < 5)
            len = 5;
        int back = len / 2;

        QPolygonF arrow(3);
        arrow[0] = QPointF(0.5 + xc + len * qCos(angle),
                           0.5 + yc - len * qSin(angle));
        arrow[1] = QPointF(0.5 + xc + back * qCos(angle + Q_PI * 5 / 6),
                           0.5 + yc - back * qSin(angle + Q_PI * 5 / 6));
        arrow[2] = QPointF(0.5 + xc + back * qCos(angle - Q_PI * 5 / 6),
                           0.5 + yc - back * qSin(angle - Q_PI * 5 / 6));
        rect = arrow.boundingRect().toRect();
        break;
    }
    default:
        return QRect();
    }

    QPoint globalPos = dial()->mapToGlobal(QPoint(0,0));
    return QRect(globalPos.x() + rect.x(), globalPos.y() + rect.y(), rect.width(), rect.height());
}

int QAccessibleDial::childCount() const
{
    return SliderHandle;
}

QString QAccessibleDial::text(Text textType, int child) const
{
    if (textType == Value && child >= Self && child <= SliderHandle)
        return QString::number(dial()->value());
    if (textType == Name) {
        switch (child) {
        case Self:
            if (!widget()->accessibleName().isEmpty())
                return widget()->accessibleName();
            return QDial::tr("QDial");
        case SpeedoMeter:
            return QDial::tr("SpeedoMeter");
        case SliderHandle:
            return QDial::tr("SliderHandle");
        }
    }
    return QAccessibleWidgetEx::text(textType, child);
}

QAccessible::Role QAccessibleDial::role(int child) const
{
    if (child == SpeedoMeter)
        return Slider;
    else if (child == SliderHandle)
        return Indicator;
    return QAccessibleWidgetEx::role(child);
}

QAccessible::State QAccessibleDial::state(int child) const
{
    const State parentState = QAccessibleWidgetEx::state(0);
    if (child == SliderHandle)
        return parentState | HotTracked;
    return parentState;
}

QVariant QAccessibleDial::invokeMethodEx(Method, int, const QVariantList &)
{
    return QVariant();
}

QDial *QAccessibleDial::dial() const
{
    return static_cast<QDial*>(object());
}
#endif // QT_NO_DIAL

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE
