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

#include "simplewidgets.h"

#include <qabstractbutton.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qprogressbar.h>
#include <qradiobutton.h>
#include <qtoolbutton.h>
#include <qmenu.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qlcdnumber.h>
#include <qlineedit.h>
#include <qstyle.h>
#include <qstyleoption.h>

#ifdef Q_OS_MAC
#include <qfocusframe.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

using namespace QAccessible2;
extern QList<QWidget*> childWidgets(const QWidget *widget, bool includeTopLevel = false);

QString Q_GUI_EXPORT qt_accStripAmp(const QString &text);
QString Q_GUI_EXPORT qt_accHotKey(const QString &text);

QString Q_GUI_EXPORT qTextBeforeOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text);
QString Q_GUI_EXPORT qTextAtOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text);
QString Q_GUI_EXPORT qTextAfterOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text);

/*!
  \class QAccessibleButton
  \brief The QAccessibleButton class implements the QAccessibleInterface for button type widgets.
  \internal

  \ingroup accessibility
*/

/*!
  Creates a QAccessibleButton object for \a w.
  \a role is propagated to the QAccessibleWidget constructor.
*/
QAccessibleButton::QAccessibleButton(QWidget *w, Role role)
: QAccessibleWidget(w, role)
{
    Q_ASSERT(button());
    if (button()->isCheckable())
        addControllingSignal(QLatin1String("toggled(bool)"));
    else
        addControllingSignal(QLatin1String("clicked()"));
}

/*! Returns the button. */
QAbstractButton *QAccessibleButton::button() const
{
    return qobject_cast<QAbstractButton*>(object());
}

/*! \reimp */
QString QAccessibleButton::text(Text t, int child) const
{
    Q_ASSERT(child == 0);
    QString str;
    switch (t) {
    case Accelerator:
    {
#ifndef QT_NO_SHORTCUT
        QPushButton *pb = qobject_cast<QPushButton*>(object());
        if (pb && pb->isDefault())
            str = (QString)QKeySequence(Qt::Key_Enter);
#endif
        if (str.isEmpty())
            str = qt_accHotKey(button()->text());
    }
         break;
    case Name:
        str = widget()->accessibleName();
        if (str.isEmpty())
            str = button()->text();
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t, child);
    return qt_accStripAmp(str);
}

QAccessible::State QAccessibleButton::state(int child) const
{
    Q_ASSERT(child == 0);
    State state = QAccessibleWidget::state();

    QAbstractButton *b = button();
    QCheckBox *cb = qobject_cast<QCheckBox *>(b);
    if (b->isChecked())
        state |= Checked;
    else if (cb && cb->checkState() == Qt::PartiallyChecked)
        state |= Mixed;
    if (b->isDown())
        state |= Pressed;
    QPushButton *pb = qobject_cast<QPushButton*>(b);
    if (pb) {
        if (pb->isDefault())
            state |= DefaultButton;
#ifndef QT_NO_MENU
        if (pb->menu())
            state |= HasPopup;
#endif
    }

    return state;
}

QStringList QAccessibleButton::actionNames() const
{
    QStringList names;
    if (widget()->isEnabled()) {
        switch (role()) {
        case ButtonMenu:
            names << ShowMenuAction;
            break;
        case RadioButton:
            names << CheckAction;
            break;
        default:
            if (button()->isCheckable()) {
                if (state() & Checked) {
                    names <<  UncheckAction;
                } else {
                    // FIXME
    //                QCheckBox *cb = qobject_cast<QCheckBox*>(object());
    //                if (!cb || !cb->isTristate() || cb->checkState() == Qt::PartiallyChecked)
                    names <<  CheckAction;
                }
            } else {
                names << PressAction;
            }
            break;
        }
    }
    return names;
}

void QAccessibleButton::doAction(const QString &actionName)
{
    if (!widget()->isEnabled())
        return;
    if (actionName == PressAction ||
        actionName == ShowMenuAction) {
#ifndef QT_NO_MENU
        QPushButton *pb = qobject_cast<QPushButton*>(object());
        if (pb && pb->menu())
            pb->showMenu();
        else
#endif
            button()->animateClick();
    }

    if (actionName == CheckAction)
        button()->setChecked(true);
    if (actionName == UncheckAction)
        button()->setChecked(false);
}

QStringList QAccessibleButton::keyBindingsForAction(const QString &actionName) const
{
    if (actionName == PressAction) {
#ifndef QT_NO_SHORTCUT
        return QStringList() << button()->shortcut().toString();
#endif
    }
    return QStringList();
}


#ifndef QT_NO_TOOLBUTTON
/*!
  \class QAccessibleToolButton
  \brief The QAccessibleToolButton class implements the QAccessibleInterface for tool buttons.
  \internal

  \ingroup accessibility
*/

/*!
  Creates a QAccessibleToolButton object for \a w.
  \a role is propagated to the QAccessibleWidget constructor.
*/
QAccessibleToolButton::QAccessibleToolButton(QWidget *w, Role role)
: QAccessibleButton(w, role)
{
    Q_ASSERT(toolButton());
}

/*! Returns the button. */
QToolButton *QAccessibleToolButton::toolButton() const
{
    return qobject_cast<QToolButton*>(object());
}

/*!
    Returns true if this tool button is a split button.
*/
bool QAccessibleToolButton::isSplitButton() const
{
#ifndef QT_NO_MENU
    return toolButton()->menu() && toolButton()->popupMode() == QToolButton::MenuButtonPopup;
#else
    return false;
#endif
}

QAccessible::State QAccessibleToolButton::state(int) const
{
    QAccessible::State st = QAccessibleButton::state();
    if (toolButton()->autoRaise())
        st |= HotTracked;
#ifndef QT_NO_MENU
    if (toolButton()->menu())
        st |= HasPopup;
#endif
    return st;
}

int QAccessibleToolButton::childCount() const
{
    return isSplitButton() ? 1 : 0;
}

QAccessibleInterface *QAccessibleToolButton::child(int index) const
{
#ifndef QT_NO_MENU
    if (index == 0 && toolButton()->menu())
    {
        return queryAccessibleInterface(toolButton()->menu());
    }
#endif
    return 0;
}

/*!
    \internal

    Returns the button's text label, depending on the text \a t, and
    the \a child.
*/
QString QAccessibleToolButton::text(Text t, int) const
{
    QString str;
    switch (t) {
    case Name:
        str = toolButton()->accessibleName();
        if (str.isEmpty())
            str = toolButton()->text();
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleButton::text(t);
    return qt_accStripAmp(str);
}

/*!
    \internal

    Returns the number of actions. 1 to trigger the button, 2 to show the menu.
*/
int QAccessibleToolButton::actionCount(int) const
{
    return 1;
}

/*
   The three different tool button types can have the following actions:
| DelayedPopup    | ShowMenuAction + (PressedAction || CheckedAction) |
| MenuButtonPopup | ShowMenuAction + (PressedAction || CheckedAction) |
| InstantPopup    | ShowMenuAction |
*/
QStringList QAccessibleToolButton::actionNames() const
{
    QStringList names;
    if (widget()->isEnabled()) {
        if (toolButton()->menu())
            names << ShowMenuAction;
        if (toolButton()->popupMode() != QToolButton::InstantPopup)
            names << QAccessibleButton::actionNames();
    }
    return names;
}

void QAccessibleToolButton::doAction(const QString &actionName)
{
    if (!widget()->isEnabled())
        return;

    if (actionName == PressAction) {
        button()->click();
    } else if (actionName == ShowMenuAction) {
        if (toolButton()->popupMode() != QToolButton::InstantPopup) {
            toolButton()->setDown(true);
#ifndef QT_NO_MENU
            toolButton()->showMenu();
#endif
        }
    } else {
        QAccessibleButton::doAction(actionName);
    }

}

#endif // QT_NO_TOOLBUTTON

/*!
  \class QAccessibleDisplay
  \brief The QAccessibleDisplay class implements the QAccessibleInterface for widgets that display information.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleDisplay object for \a w.
  \a role is propagated to the QAccessibleWidget constructor.
*/
QAccessibleDisplay::QAccessibleDisplay(QWidget *w, Role role)
: QAccessibleWidget(w, role)
{
}

QAccessible::Role QAccessibleDisplay::role(int child) const
{
    Q_ASSERT(child == 0);
    QLabel *l = qobject_cast<QLabel*>(object());
    if (l) {
        if (l->pixmap())
            return Graphic;
#ifndef QT_NO_PICTURE
        if (l->picture())
            return Graphic;
#endif
#ifndef QT_NO_MOVIE
        if (l->movie())
            return Animation;
#endif
#ifndef QT_NO_PROGRESSBAR
    } else if (qobject_cast<QProgressBar*>(object())) {
        return ProgressBar;
#endif
    }
    return QAccessibleWidget::role(child);
}

QString QAccessibleDisplay::text(Text t, int child) const
{
    Q_ASSERT(child == 0);
    QString str;
    switch (t) {
    case Name:
        str = widget()->accessibleName();
        if (str.isEmpty()) {
            if (qobject_cast<QLabel*>(object())) {
                str = qobject_cast<QLabel*>(object())->text();
#ifndef QT_NO_GROUPBOX
            } else if (qobject_cast<QGroupBox*>(object())) {
                str = qobject_cast<QGroupBox*>(object())->title();
#endif
#ifndef QT_NO_LCDNUMBER
            } else if (qobject_cast<QLCDNumber*>(object())) {
                QLCDNumber *l = qobject_cast<QLCDNumber*>(object());
                if (l->digitCount())
                    str = QString::number(l->value());
                else
                    str = QString::number(l->intValue());
#endif
            }
        }
        break;
    case Value:
#ifndef QT_NO_PROGRESSBAR
        if (qobject_cast<QProgressBar*>(object()))
            str = QString::number(qobject_cast<QProgressBar*>(object())->value());
#endif
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t, child);;
    return qt_accStripAmp(str);
}

QAccessible::Relation QAccessibleDisplay::relationTo(int child, const QAccessibleInterface *other,
                                                     int otherChild) const
{
    Q_ASSERT(child == 0);
    Relation relation = QAccessibleWidget::relationTo(child, other, otherChild);
    if (child || otherChild)
        return relation;

    QObject *o = other->object();
    QLabel *label = qobject_cast<QLabel*>(object());
    if (label) {
#ifndef QT_NO_SHORTCUT
        if (o == label->buddy())
            relation |= Label;
#endif
#ifndef QT_NO_GROUPBOX
    } else {
	QGroupBox *groupbox = qobject_cast<QGroupBox*>(object());
	if (groupbox && !groupbox->title().isEmpty())
	    if (groupbox->children().contains(o))
		relation |= Label;
#endif
    }
    return relation;
}

int QAccessibleDisplay::navigate(RelationFlag rel, int entry, QAccessibleInterface **target) const
{
    *target = 0;
    if (rel == Labelled) {
        QObject *targetObject = 0;
        QLabel *label = qobject_cast<QLabel*>(object());
        if (label) {
#ifndef QT_NO_SHORTCUT
            if (entry == 1)
                targetObject = label->buddy();
#endif
#ifndef QT_NO_GROUPBOX
        } else {
	    QGroupBox *groupbox = qobject_cast<QGroupBox*>(object());
	    if (groupbox && !groupbox->title().isEmpty())
		rel = Child;
#endif
        }
        *target = QAccessible::queryAccessibleInterface(targetObject);
        if (*target)
            return 0;
    }
    return QAccessibleWidget::navigate(rel, entry, target);
}

/*! \internal */
QString QAccessibleDisplay::imageDescription()
{
#ifndef QT_NO_TOOLTIP
    return widget()->toolTip();
#else
    return QString::null;
#endif
}

/*! \internal */
QSize QAccessibleDisplay::imageSize()
{
    QLabel *label = qobject_cast<QLabel *>(widget());
    if (!label)
        return QSize();
    const QPixmap *pixmap = label->pixmap();
    if (!pixmap)
        return QSize();
    return pixmap->size();
}

/*! \internal */
QRect QAccessibleDisplay::imagePosition(QAccessible2::CoordinateType coordType)
{
    QLabel *label = qobject_cast<QLabel *>(widget());
    if (!label)
        return QRect();
    const QPixmap *pixmap = label->pixmap();
    if (!pixmap)
        return QRect();

    switch (coordType) {
    case QAccessible2::RelativeToScreen:
        return QRect(label->mapToGlobal(label->pos()), label->size());
    case QAccessible2::RelativeToParent:
        return label->geometry();
    }

    return QRect();
}

#ifndef QT_NO_LINEEDIT
/*!
  \class QAccessibleLineEdit
  \brief The QAccessibleLineEdit class implements the QAccessibleInterface for widgets with editable text
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleLineEdit object for \a w.
  \a name is propagated to the QAccessibleWidget constructor.
*/
QAccessibleLineEdit::QAccessibleLineEdit(QWidget *w, const QString &name)
: QAccessibleWidget(w, EditableText, name), QAccessibleSimpleEditableTextInterface(this)
{
    addControllingSignal(QLatin1String("textChanged(const QString&)"));
    addControllingSignal(QLatin1String("returnPressed()"));
}

/*! Returns the line edit. */
QLineEdit *QAccessibleLineEdit::lineEdit() const
{
    return qobject_cast<QLineEdit*>(object());
}

QString QAccessibleLineEdit::text(Text t, int child) const
{
    Q_ASSERT(child == 0);
    QString str;
    switch (t) {
    case Value:
        if (lineEdit()->echoMode() == QLineEdit::Normal)
            str = lineEdit()->text();
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t, child);;
    return qt_accStripAmp(str);
}

void QAccessibleLineEdit::setText(Text t, int control, const QString &text)
{
    if (t != Value || control) {
        QAccessibleWidget::setText(t, control, text);
        return;
    }

    QString newText = text;
    if (lineEdit()->validator()) {
        int pos = 0;
        if (lineEdit()->validator()->validate(newText, pos) != QValidator::Acceptable)
            return;
    }
    lineEdit()->setText(newText);
}

QAccessible::State QAccessibleLineEdit::state(int child) const
{
    Q_ASSERT(child == 0);
    State state = QAccessibleWidget::state(child);

    QLineEdit *l = lineEdit();
    if (l->isReadOnly())
        state |= ReadOnly;
    if (l->echoMode() != QLineEdit::Normal)
        state |= Protected;
    state |= Selectable;
    if (l->hasSelectedText())
        state |= Selected;

    if (l->contextMenuPolicy() != Qt::NoContextMenu
        && l->contextMenuPolicy() != Qt::PreventContextMenu)
        state |= HasPopup;

    return state;
}

QVariant QAccessibleLineEdit::invokeMethod(QAccessible::Method method, int child,
                                                     const QVariantList &params)
{
    Q_ASSERT(child == 0);

    switch (method) {
    case ListSupportedMethods: {
        QSet<QAccessible::Method> set;
        set << ListSupportedMethods << SetCursorPosition << GetCursorPosition;
        return QVariant::fromValue(set | qvariant_cast<QSet<QAccessible::Method> >(
                QAccessibleWidget::invokeMethod(method, child, params)));
    }
    case SetCursorPosition:
        setCursorPosition(params.value(0).toInt());
        return true;
    case GetCursorPosition:
        return cursorPosition();
    default:
        return QAccessibleWidget::invokeMethod(method, child, params);
    }
}

void QAccessibleLineEdit::addSelection(int startOffset, int endOffset)
{
    setSelection(0, startOffset, endOffset);
}

QString QAccessibleLineEdit::attributes(int offset, int *startOffset, int *endOffset)
{
    // QLineEdit doesn't have text attributes
    *startOffset = *endOffset = offset;
    return QString();
}

int QAccessibleLineEdit::cursorPosition()
{
    return lineEdit()->cursorPosition();
}

QRect QAccessibleLineEdit::characterRect(int /*offset*/, CoordinateType /*coordType*/)
{
    // QLineEdit doesn't hand out character rects
    return QRect();
}

int QAccessibleLineEdit::selectionCount()
{
    return lineEdit()->hasSelectedText() ? 1 : 0;
}

int QAccessibleLineEdit::offsetAtPoint(const QPoint &point, CoordinateType coordType)
{
    QPoint p = point;
    if (coordType == RelativeToScreen)
        p = lineEdit()->mapFromGlobal(p);

    return lineEdit()->cursorPositionAt(p);
}

void QAccessibleLineEdit::selection(int selectionIndex, int *startOffset, int *endOffset)
{
    *startOffset = *endOffset = 0;
    if (selectionIndex != 0)
        return;

    *startOffset = lineEdit()->selectionStart();
    *endOffset = *startOffset + lineEdit()->selectedText().count();
}

QString QAccessibleLineEdit::text(int startOffset, int endOffset)
{
    if (startOffset > endOffset)
        return QString();

    if (lineEdit()->echoMode() != QLineEdit::Normal)
        return QString();

    return lineEdit()->text().mid(startOffset, endOffset - startOffset);
}

QString QAccessibleLineEdit::textBeforeOffset(int offset, BoundaryType boundaryType,
        int *startOffset, int *endOffset)
{
    if (lineEdit()->echoMode() != QLineEdit::Normal) {
        *startOffset = *endOffset = -1;
        return QString();
    }
    return qTextBeforeOffsetFromString(offset, boundaryType, startOffset, endOffset, lineEdit()->text());
}

QString QAccessibleLineEdit::textAfterOffset(int offset, BoundaryType boundaryType,
        int *startOffset, int *endOffset)
{
    if (lineEdit()->echoMode() != QLineEdit::Normal) {
        *startOffset = *endOffset = -1;
        return QString();
    }
    return qTextAfterOffsetFromString(offset, boundaryType, startOffset, endOffset, lineEdit()->text());
}

QString QAccessibleLineEdit::textAtOffset(int offset, BoundaryType boundaryType,
        int *startOffset, int *endOffset)
{
    if (lineEdit()->echoMode() != QLineEdit::Normal) {
        *startOffset = *endOffset = -1;
        return QString();
    }
    return qTextAtOffsetFromString(offset, boundaryType, startOffset, endOffset, lineEdit()->text());
}

void QAccessibleLineEdit::removeSelection(int selectionIndex)
{
    if (selectionIndex != 0)
        return;

    lineEdit()->deselect();
}

void QAccessibleLineEdit::setCursorPosition(int position)
{
    lineEdit()->setCursorPosition(position);
}

void QAccessibleLineEdit::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    if (selectionIndex != 0)
        return;

    lineEdit()->setSelection(startOffset, endOffset - startOffset);
}

int QAccessibleLineEdit::characterCount()
{
    return lineEdit()->text().count();
}

void QAccessibleLineEdit::scrollToSubstring(int startIndex, int endIndex)
{
    lineEdit()->setCursorPosition(endIndex);
    lineEdit()->setCursorPosition(startIndex);
}

#endif // QT_NO_LINEEDIT

#ifndef QT_NO_PROGRESSBAR
QAccessibleProgressBar::QAccessibleProgressBar(QWidget *o)
    : QAccessibleDisplay(o)
{
    Q_ASSERT(progressBar());
}

QVariant QAccessibleProgressBar::currentValue()
{
    return progressBar()->value();
}

QVariant QAccessibleProgressBar::maximumValue()
{
    return progressBar()->maximum();
}

QVariant QAccessibleProgressBar::minimumValue()
{
    return progressBar()->minimum();
}

QProgressBar *QAccessibleProgressBar::progressBar() const
{
    return qobject_cast<QProgressBar *>(object());
}
#endif

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

