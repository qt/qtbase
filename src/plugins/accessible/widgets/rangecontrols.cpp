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
: QAccessibleWidget(w, SpinBox)
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
QRect QAccessibleAbstractSpinBox::rect(int child) const
{
    QRect rect;
    if (!abstractSpinBox()->isVisible())
        return rect;
    return widget()->rect();
}

/*! \reimp */
QString QAccessibleAbstractSpinBox::text(Text t, int child) const
{
    if (t == QAccessible::Value)
        return abstractSpinBox()->text();
    return QAccessibleWidget::text(t, 0);
}

/*! \reimp */
bool QAccessibleAbstractSpinBox::doAction(int action, int child, const QVariantList &params)
{
    if (!widget()->isEnabled())
        return false;
    return QAccessibleWidget::doAction(action, 0, params);
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

QVariant QAccessibleAbstractSpinBox::invokeMethod(Method method, int child, const QVariantList &params)
{
    switch (method) {
    case ListSupportedMethods: {
        QSet<QAccessible::Method> set;
        set << ListSupportedMethods;
        return QVariant::fromValue(set | qvariant_cast<QSet<QAccessible::Method> >(
                    QAccessibleWidget::invokeMethod(method, child, params)));
    }
    default:
        return QAccessibleWidget::invokeMethod(method, child, params);
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
bool QAccessibleSpinBox::doAction(int action, int child, const QVariantList &params)
{
    if (!widget()->isEnabled())
        return false;

    return QAccessibleAbstractSpinBox::doAction(action, 0, params);
}

// ================================== QAccessibleDoubleSpinBox ==================================
QAccessibleDoubleSpinBox::QAccessibleDoubleSpinBox(QWidget *widget)
    : QAccessibleWidget(widget, SpinBox)
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
        return QAccessibleWidget::navigate(relation, entry, target);

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
    return QAccessibleWidget::navigate(relation, entry, target);
}

QVariant QAccessibleDoubleSpinBox::invokeMethod(QAccessible::Method, int, const QVariantList &)
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
    return QAccessibleWidget::text(textType, 0);
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
    return QAccessibleWidget::role(child);
}

/*! \reimp */
QAccessible::State QAccessibleDoubleSpinBox::state(int child) const
{
    State state = QAccessibleWidget::state(child);
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
  \a name is propagated to the QAccessibleWidget constructor.
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
QString QAccessibleScrollBar::text(Text t, int child) const
{
    if (t == Value)
        return QString::number(scrollBar()->value());
    return QAccessibleAbstractSlider::text(t, child);
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
  \a name is propagated to the QAccessibleWidget constructor.
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
QString QAccessibleSlider::text(Text t, int child) const
{
    if (t == Value)
        return QString::number(slider()->value());

    return QAccessibleAbstractSlider::text(t, child);
}

/*!
    \fn int QAccessibleSlider::defaultAction(int child) const

    Returns the default action for the given \a child. The base class
    implementation returns 0.
*/
int QAccessibleSlider::defaultAction(int /*child*/) const
{
    return 0;
}

/*! \internal */
QString QAccessibleSlider::actionText(int /*action*/, Text /*t*/, int /*child*/) const
{
    return QLatin1String("");
}

QAccessibleAbstractSlider::QAccessibleAbstractSlider(QWidget *w, Role r)
    : QAccessibleWidget(w, r)
{
    Q_ASSERT(qobject_cast<QAbstractSlider *>(w));
}

QVariant QAccessibleAbstractSlider::invokeMethod(Method method, int child, const QVariantList &params)
{
    switch (method) {
    case ListSupportedMethods: {
        QSet<QAccessible::Method> set;
        set << ListSupportedMethods;
        return QVariant::fromValue(set | qvariant_cast<QSet<QAccessible::Method> >(
                    QAccessibleWidget::invokeMethod(method, child, params)));
    }
    default:
        return QAccessibleWidget::invokeMethod(method, child, params);
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
    : QAccessibleWidget(widget, Dial)
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
        return QAccessibleWidget::rect(child);
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
    return QAccessibleWidget::text(textType, child);
}

QAccessible::Role QAccessibleDial::role(int child) const
{
    if (child == SpeedoMeter)
        return Slider;
    else if (child == SliderHandle)
        return Indicator;
    return QAccessibleWidget::role(child);
}

QAccessible::State QAccessibleDial::state(int child) const
{
    const State parentState = QAccessibleWidget::state(0);
    if (child == SliderHandle)
        return parentState | HotTracked;
    return parentState;
}

QVariant QAccessibleDial::invokeMethod(Method, int, const QVariantList &)
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
