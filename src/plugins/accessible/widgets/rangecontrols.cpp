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
: QAccessibleWidget(w, QAccessible::SpinBox)
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

QString QAccessibleAbstractSpinBox::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return abstractSpinBox()->text();
    return QAccessibleWidget::text(t);
}

void *QAccessibleAbstractSpinBox::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ValueInterface)
        return static_cast<QAccessibleValueInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
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

/*!
  \class QAccessibleSpinBox
  \brief The QAccessibleSpinBox class implements the QAccessibleInterface for spinbox widgets.
  \internal

  \ingroup accessibility
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


// ================================== QAccessibleDoubleSpinBox ==================================
QAccessibleDoubleSpinBox::QAccessibleDoubleSpinBox(QWidget *widget)
    : QAccessibleAbstractSpinBox(widget)
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

QString QAccessibleDoubleSpinBox::text(QAccessible::Text textType) const
{
    if (textType == QAccessible::Value)
        return doubleSpinBox()->textFromValue(doubleSpinBox()->value());
    return QAccessibleWidget::text(textType);
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
  Constructs a QAccessibleScrollBar object for \a w.
  \a name is propagated to the QAccessibleWidget constructor.
*/
QAccessibleScrollBar::QAccessibleScrollBar(QWidget *w)
: QAccessibleAbstractSlider(w, QAccessible::ScrollBar)
{
    Q_ASSERT(scrollBar());
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

/*! Returns the scroll bar. */
QScrollBar *QAccessibleScrollBar::scrollBar() const
{
    return qobject_cast<QScrollBar*>(object());
}

QString QAccessibleScrollBar::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return QString::number(scrollBar()->value());
    return QAccessibleAbstractSlider::text(t);
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

QString QAccessibleSlider::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return QString::number(slider()->value());

    return QAccessibleAbstractSlider::text(t);
}

QAccessibleAbstractSlider::QAccessibleAbstractSlider(QWidget *w, QAccessible::Role r)
    : QAccessibleWidget(w, r)
{
    Q_ASSERT(qobject_cast<QAbstractSlider *>(w));
}

void *QAccessibleAbstractSlider::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ValueInterface)
        return static_cast<QAccessibleValueInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
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
    : QAccessibleAbstractSlider(widget, QAccessible::Dial)
{
    Q_ASSERT(qobject_cast<QDial *>(widget));
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

QString QAccessibleDial::text(QAccessible::Text textType) const
{
    if (textType == QAccessible::Value)
        return QString::number(dial()->value());

    return QAccessibleAbstractSlider::text(textType);
}

QDial *QAccessibleDial::dial() const
{
    return static_cast<QDial*>(object());
}
#endif // QT_NO_DIAL

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE
