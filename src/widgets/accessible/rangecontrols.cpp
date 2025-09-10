// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "rangecontrols_p.h"

#if QT_CONFIG(slider)
#include <qslider.h>
#endif
#if QT_CONFIG(dial)
#include <qdial.h>
#endif
#if QT_CONFIG(spinbox)
#include <qspinbox.h>
#endif
#if QT_CONFIG(scrollbar)
#include <qscrollbar.h>
#endif
#include <qstyle.h>
#include <qstyleoption.h>
#include <qdebug.h>
#include <qglobal.h>
#if QT_CONFIG(lineedit)
#include <QtWidgets/qlineedit.h>
#endif
#include <qmath.h>
#include <private/qmath_p.h>

#include "simplewidgets_p.h" // let spinbox use line edit's interface

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(accessibility)

#if QT_CONFIG(spinbox)
QAccessibleAbstractSpinBox::QAccessibleAbstractSpinBox(QWidget *w)
: QAccessibleWidget(w, QAccessible::SpinBox), lineEdit(nullptr)
{
    Q_ASSERT(abstractSpinBox());
}

QAccessibleAbstractSpinBox::~QAccessibleAbstractSpinBox()
{
    delete lineEdit;
}

/*!
    Returns the underlying QAbstractSpinBox.
*/
QAbstractSpinBox *QAccessibleAbstractSpinBox::abstractSpinBox() const
{
    return qobject_cast<QAbstractSpinBox*>(object());
}

QAccessibleInterface *QAccessibleAbstractSpinBox::lineEditIface() const
{
#if QT_CONFIG(lineedit)
    // QAccessibleLineEdit is only used to forward the text functions
    if (!lineEdit)
        lineEdit = new QAccessibleLineEdit(abstractSpinBox()->lineEdit());
    return lineEdit;
#else
    return nullptr;
#endif
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
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface*>(this);
    if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

QVariant QAccessibleAbstractSpinBox::currentValue() const
{
    return abstractSpinBox()->property("value");
}

void QAccessibleAbstractSpinBox::setCurrentValue(const QVariant &value)
{
    abstractSpinBox()->setProperty("value", value);
}

QVariant QAccessibleAbstractSpinBox::maximumValue() const
{
    return abstractSpinBox()->property("maximum");
}

QVariant QAccessibleAbstractSpinBox::minimumValue() const
{
    return abstractSpinBox()->property("minimum");
}

QVariant QAccessibleAbstractSpinBox::minimumStepSize() const
{
    return abstractSpinBox()->property("stepSize");
}

void QAccessibleAbstractSpinBox::addSelection(int startOffset, int endOffset)
{
    lineEditIface()->textInterface()->addSelection(startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::attributes(int offset, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->attributes(offset, startOffset, endOffset);
}

int QAccessibleAbstractSpinBox::cursorPosition() const
{
    return lineEditIface()->textInterface()->cursorPosition();
}

QRect QAccessibleAbstractSpinBox::characterRect(int offset) const
{
    return lineEditIface()->textInterface()->characterRect(offset);
}

int QAccessibleAbstractSpinBox::selectionCount() const
{
    return lineEditIface()->textInterface()->selectionCount();
}

int QAccessibleAbstractSpinBox::offsetAtPoint(const QPoint &point) const
{
    return lineEditIface()->textInterface()->offsetAtPoint(point);
}

void QAccessibleAbstractSpinBox::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    lineEditIface()->textInterface()->selection(selectionIndex, startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::text(int startOffset, int endOffset) const
{
    return lineEditIface()->textInterface()->text(startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->textBeforeOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->textAfterOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->textAtOffset(offset, boundaryType, startOffset, endOffset);
}

void QAccessibleAbstractSpinBox::removeSelection(int selectionIndex)
{
    lineEditIface()->textInterface()->removeSelection(selectionIndex);
}

void QAccessibleAbstractSpinBox::setCursorPosition(int position)
{
    lineEditIface()->textInterface()->setCursorPosition(position);
}

void QAccessibleAbstractSpinBox::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    lineEditIface()->textInterface()->setSelection(selectionIndex, startOffset, endOffset);
}

int QAccessibleAbstractSpinBox::characterCount() const
{
    return lineEditIface()->textInterface()->characterCount();
}

void QAccessibleAbstractSpinBox::scrollToSubstring(int startIndex, int endIndex)
{
    lineEditIface()->textInterface()->scrollToSubstring(startIndex, endIndex);
}

void QAccessibleAbstractSpinBox::deleteText(int startOffset, int endOffset)
{
    lineEditIface()->editableTextInterface()->deleteText(startOffset, endOffset);
}

void QAccessibleAbstractSpinBox::insertText(int offset, const QString &text)
{
    lineEditIface()->editableTextInterface()->insertText(offset, text);
}

void QAccessibleAbstractSpinBox::replaceText(int startOffset, int endOffset, const QString &text)
{
    lineEditIface()->editableTextInterface()->replaceText(startOffset, endOffset, text);
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
    addControllingSignal("valueChanged(int)"_L1);
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
    addControllingSignal("valueChanged(double)"_L1);
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

#endif // QT_CONFIG(spinbox)

#if QT_CONFIG(scrollbar)
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
    addControllingSignal("valueChanged(int)"_L1);
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

#endif // QT_CONFIG(scrollbar)

#if QT_CONFIG(slider)
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
    addControllingSignal("valueChanged(int)"_L1);
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

QVariant QAccessibleAbstractSlider::currentValue() const
{
    return abstractSlider()->value();
}

void QAccessibleAbstractSlider::setCurrentValue(const QVariant &value)
{
    abstractSlider()->setValue(value.toInt());
}

QVariant QAccessibleAbstractSlider::maximumValue() const
{
    return abstractSlider()->maximum();
}

QVariant QAccessibleAbstractSlider::minimumValue() const
{
    return abstractSlider()->minimum();
}

QVariant QAccessibleAbstractSlider::minimumStepSize() const
{
    return abstractSlider()->singleStep();
}

QAbstractSlider *QAccessibleAbstractSlider::abstractSlider() const
{
    return static_cast<QAbstractSlider *>(object());
}

#endif // QT_CONFIG(slider)

#if QT_CONFIG(dial)
// ======================================= QAccessibleDial ======================================
QAccessibleDial::QAccessibleDial(QWidget *widget)
    : QAccessibleAbstractSlider(widget, QAccessible::Dial)
{
    Q_ASSERT(qobject_cast<QDial *>(widget));
    addControllingSignal("valueChanged(int)"_L1);
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
#endif // QT_CONFIG(dial)

#endif // QT_CONFIG(accessibility)

QT_END_NAMESPACE
