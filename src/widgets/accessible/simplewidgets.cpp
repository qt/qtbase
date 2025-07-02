// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "simplewidgets_p.h"

#if QT_CONFIG(abstractbutton)
#include <qabstractbutton.h>
#endif
#if QT_CONFIG(checkbox)
#include <qcheckbox.h>
#endif
#if QT_CONFIG(pushbutton)
#include <qpushbutton.h>
#endif
#if QT_CONFIG(progressbar)
#include <qprogressbar.h>
#endif
#if QT_CONFIG(statusbar)
#include <qstatusbar.h>
#endif
#if QT_CONFIG(radiobutton)
#include <qradiobutton.h>
#endif
#if QT_CONFIG(toolbutton)
#include <qtoolbutton.h>
#endif
#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#if QT_CONFIG(label)
#include <qlabel.h>
#endif
#if QT_CONFIG(groupbox)
#include <qgroupbox.h>
#endif
#if QT_CONFIG(lcdnumber)
#include <qlcdnumber.h>
#endif
#if QT_CONFIG(lineedit)
#include <qlineedit.h>
#include <private/qlineedit_p.h>
#endif
#ifndef QT_NO_PICTURE
#include <QtGui/qpicture.h>
#endif
#if QT_CONFIG(messagebox)
#include <qmessagebox.h>
#endif
#include <qstyle.h>
#include <qstyleoption.h>
#include <qtextdocument.h>
#include <qwindow.h>
#include <private/qwindowcontainer_p.h>
#include <QtCore/qvarlengtharray.h>
#if QT_CONFIG(accessibility)
#include <QtGui/private/qaccessiblehelper_p.h>
#endif
#include <QtGui/qvalidator.h>

#ifdef Q_OS_MAC
#include <qfocusframe.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(accessibility)

QWidgetList _q_ac_childWidgets(const QWidget *widget);

QString qt_accHotKey(const QString &text);

#if QT_CONFIG(abstractbutton)
/*!
  \class QAccessibleButton
  \brief The QAccessibleButton class implements the QAccessibleInterface for button type widgets.
  \internal

  \ingroup accessibility
*/

/*!
  Creates a QAccessibleButton object for \a w.
*/
QAccessibleButton::QAccessibleButton(QWidget *w)
: QAccessibleWidgetV2(w)
{
    Q_ASSERT(button());

    // FIXME: The checkable state of the button is dynamic,
    // while we only update the controlling signal once :(
    if (button()->isCheckable())
        addControllingSignal("toggled(bool)"_L1);
    else
        addControllingSignal("clicked()"_L1);
}

/*! Returns the button. */
QAbstractButton *QAccessibleButton::button() const
{
    return qobject_cast<QAbstractButton*>(object());
}

/*! \reimp */
QString QAccessibleButton::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
    case QAccessible::Accelerator:
    {
#if QT_CONFIG(shortcut) && QT_CONFIG(pushbutton)
        QPushButton *pb = qobject_cast<QPushButton*>(object());
        if (pb && pb->isDefault())
            str = QKeySequence(Qt::Key_Enter).toString(QKeySequence::NativeText);
#endif
        if (str.isEmpty())
            str = qt_accHotKey(button()->text());
    }
         break;
    case QAccessible::Name:
        str = widget()->accessibleName();
        if (str.isEmpty())
            str = qt_accStripAmp(button()->text());
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidgetV2::text(t);
    return str;
}

QAccessible::State QAccessibleButton::state() const
{
    QAccessible::State state = QAccessibleWidgetV2::state();

    QAbstractButton *b = button();
#if QT_CONFIG(checkbox)
    QCheckBox *cb = qobject_cast<QCheckBox *>(b);
#endif
    if (b->isCheckable())
        state.checkable = true;
    if (b->isChecked())
        state.checked = true;
#if QT_CONFIG(checkbox)
    if (cb && cb->checkState() == Qt::PartiallyChecked)
        state.checkStateMixed = true;
#endif
    if (b->isDown())
        state.pressed = true;
#if QT_CONFIG(pushbutton)
    QPushButton *pb = qobject_cast<QPushButton*>(b);
    if (pb) {
        if (pb->isDefault())
            state.defaultButton = true;
#if QT_CONFIG(menu)
        if (pb->menu())
            state.hasPopup = true;
#endif
    }
#endif

    return state;
}

QRect QAccessibleButton::rect() const
{
    QAbstractButton *ab = button();
    if (!ab->isVisible())
        return QRect();

#if QT_CONFIG(checkbox)
    if (QCheckBox *cb = qobject_cast<QCheckBox *>(ab)) {
        QPoint wpos = cb->mapToGlobal(QPoint(0, 0));
        QStyleOptionButton opt;
        cb->initStyleOption(&opt);
        return cb->style()->subElementRect(QStyle::SE_CheckBoxClickRect, &opt, cb).translated(wpos);
    }
#endif
#if QT_CONFIG(radiobutton)
    else if (QRadioButton *rb = qobject_cast<QRadioButton *>(ab)) {
        QPoint wpos = rb->mapToGlobal(QPoint(0, 0));
        QStyleOptionButton opt;
        rb->initStyleOption(&opt);
        return rb->style()->subElementRect(QStyle::SE_RadioButtonClickRect, &opt, rb).translated(wpos);
    }
#endif
    return QAccessibleWidgetV2::rect();
}

QAccessible::Role QAccessibleButton::role() const
{
    QAbstractButton *ab = button();

#if QT_CONFIG(menu)
    if (QPushButton *pb = qobject_cast<QPushButton*>(ab)) {
        if (pb->menu())
            return QAccessible::ButtonMenu;
    }
#endif

    if (ab->isCheckable())
        return ab->autoExclusive() ? QAccessible::RadioButton : QAccessible::CheckBox;

    return QAccessible::Button;
}

QStringList QAccessibleButton::actionNames() const
{
    QStringList names;
    if (widget()->isEnabled()) {
        switch (role()) {
        case QAccessible::ButtonMenu:
            names << showMenuAction();
            break;
        case QAccessible::RadioButton:
            names << toggleAction();
            break;
        default:
            if (button()->isCheckable())
                names <<  toggleAction();
            names << pressAction();
            break;
        }
    }
    names << QAccessibleWidgetV2::actionNames();
    return names;
}

void QAccessibleButton::doAction(const QString &actionName)
{
    if (!widget()->isEnabled())
        return;
    if (actionName == pressAction() ||
        actionName == showMenuAction()) {
#if QT_CONFIG(menu)
        QPushButton *pb = qobject_cast<QPushButton*>(object());
        if (pb && pb->menu())
            pb->showMenu();
        else
#endif
            button()->animateClick();
    } else if (actionName == toggleAction()) {
        button()->toggle();
    } else {
        QAccessibleWidgetV2::doAction(actionName);
    }
}

QStringList QAccessibleButton::keyBindingsForAction(const QString &actionName) const
{
    if (actionName == pressAction()) {
#ifndef QT_NO_SHORTCUT
        return QStringList() << button()->shortcut().toString();
#endif
    }
    return QStringList();
}
#endif // QT_CONFIG(abstractbutton)

#if QT_CONFIG(toolbutton)
/*!
  \class QAccessibleToolButton
  \brief The QAccessibleToolButton class implements the QAccessibleInterface for tool buttons.
  \internal

  \ingroup accessibility
*/

/*!
  Creates a QAccessibleToolButton object for \a w.
*/
QAccessibleToolButton::QAccessibleToolButton(QWidget *w)
: QAccessibleButton(w)
{
    Q_ASSERT(toolButton());
}

/*! Returns the button. */
QToolButton *QAccessibleToolButton::toolButton() const
{
    return qobject_cast<QToolButton*>(object());
}

/*!
    Returns \c true if this tool button is a split button.
*/
bool QAccessibleToolButton::isSplitButton() const
{
#if QT_CONFIG(menu)
    return menu() && toolButton()->popupMode() == QToolButton::MenuButtonPopup;
#else
    return false;
#endif
}

#if QT_CONFIG(menu)
QMenu *QAccessibleToolButton::menu() const
{
    if (QMenu *menu = toolButton()->menu())
        return menu;

    if (QAction *defaultAction = toolButton()->defaultAction())
        return defaultAction->menu();

    return nullptr;
}
#endif

QAccessible::State QAccessibleToolButton::state() const
{
    QAccessible::State st = QAccessibleButton::state();
    if (toolButton()->autoRaise())
        st.hotTracked = true;
#if QT_CONFIG(menu)
    if (menu())
        st.hasPopup = true;
#endif
    return st;
}

int QAccessibleToolButton::childCount() const
{
    return isSplitButton() ? 1 : 0;
}

QAccessible::Role QAccessibleToolButton::role() const
{
#if QT_CONFIG(menu)
    QToolButton *tb = toolButton();
    if (!menu())
        return tb->isCheckable() ? QAccessible::CheckBox : QAccessible::PushButton;
    else if (tb->popupMode() == QToolButton::DelayedPopup)
        return QAccessible::ButtonDropDown;
#endif

    return QAccessible::ButtonMenu;
}

QAccessibleInterface *QAccessibleToolButton::child(int index) const
{
#if QT_CONFIG(menu)
    if (index == 0 && menu())
        return QAccessible::queryAccessibleInterface(menu());
#else
    Q_UNUSED(index);
#endif
    return nullptr;
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
#if QT_CONFIG(menu)
        if (toolButton()->menu())
            names << showMenuAction();
        if (toolButton()->popupMode() != QToolButton::InstantPopup)
            names << QAccessibleButton::actionNames();
#endif
    }
    return names;
}

void QAccessibleToolButton::doAction(const QString &actionName)
{
    if (!widget()->isEnabled())
        return;

    if (actionName == pressAction()) {
        button()->click();
    } else if (actionName == showMenuAction()) {
#if QT_CONFIG(menu)
        if (toolButton()->popupMode() != QToolButton::InstantPopup) {
            toolButton()->setDown(true);
            toolButton()->showMenu();
        }
#endif
    } else {
        QAccessibleButton::doAction(actionName);
    }

}

#endif // QT_CONFIG(toolbutton)

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
QAccessibleDisplay::QAccessibleDisplay(QWidget *w, QAccessible::Role role)
: QAccessibleWidgetV2(w, role)
{
}

QAccessible::Role QAccessibleDisplay::role() const
{
#if QT_CONFIG(label)
    QLabel *l = qobject_cast<QLabel*>(object());
    if (l) {
        if (!l->pixmap().isNull())
            return QAccessible::Graphic;
#ifndef QT_NO_PICTURE
        if (!l->picture().isNull())
            return QAccessible::Graphic;
#endif
#if QT_CONFIG(movie)
        if (l->movie())
            return QAccessible::Animation;
#endif
#if QT_CONFIG(progressbar)
    } else if (qobject_cast<QProgressBar*>(object())) {
        return QAccessible::ProgressBar;
#endif
#if QT_CONFIG(statusbar)
    } else if (qobject_cast<QStatusBar*>(object())) {
        return QAccessible::StatusBar;
#endif
    }
#endif
    return QAccessibleWidgetV2::role();
}

QAccessible::State QAccessibleDisplay::state() const
{
    QAccessible::State s = QAccessibleWidgetV2::state();
    s.readOnly = true;
    return s;
}

QString QAccessibleDisplay::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
    case QAccessible::Name:
        str = widget()->accessibleName();
        if (str.isEmpty()) {
            if (false) {
#if QT_CONFIG(label)
            } else if (qobject_cast<QLabel*>(object())) {
                QLabel *label = qobject_cast<QLabel*>(object());
                str = label->text();
#ifndef QT_NO_TEXTHTMLPARSER
                if (label->textFormat() == Qt::RichText
                    || (label->textFormat() == Qt::AutoText && Qt::mightBeRichText(str))) {
                    QTextDocument doc;
                    doc.setHtml(str);
                    str = doc.toPlainText();
                }
#endif
#ifndef QT_NO_SHORTCUT
                if (label->buddy())
                    str = qt_accStripAmp(str);
#endif
#endif // QT_CONFIG(label)
#if QT_CONFIG(lcdnumber)
            } else if (qobject_cast<QLCDNumber*>(object())) {
                QLCDNumber *l = qobject_cast<QLCDNumber*>(object());
                if (l->digitCount())
                    str = QString::number(l->value());
                else
                    str = QString::number(l->intValue());
#endif
#if QT_CONFIG(statusbar)
            } else if (qobject_cast<QStatusBar*>(object())) {
                return qobject_cast<QStatusBar*>(object())->currentMessage();
#endif
            }
        }
        break;
    case QAccessible::Value:
#if QT_CONFIG(progressbar)
        if (qobject_cast<QProgressBar*>(object()))
            str = QString::number(qobject_cast<QProgressBar*>(object())->value());
#endif
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidgetV2::text(t);
    return str;
}

/*! \reimp */
QList<std::pair<QAccessibleInterface *, QAccessible::Relation>>
QAccessibleDisplay::relations(QAccessible::Relation match /* = QAccessible::AllRelations */) const
{
    QList<std::pair<QAccessibleInterface *, QAccessible::Relation>> rels =
            QAccessibleWidgetV2::relations(match);
#    if QT_CONFIG(shortcut) && QT_CONFIG(label)
    if (match & QAccessible::Labelled) {
        if (QLabel *label = qobject_cast<QLabel*>(object())) {
            const QAccessible::Relation rel = QAccessible::Labelled;
            if (QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(label->buddy()))
                rels.emplace_back(iface, rel);
        }
    }
#endif
    return rels;
}

void *QAccessibleDisplay::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ImageInterface)
        return static_cast<QAccessibleImageInterface*>(this);
    return QAccessibleWidgetV2::interface_cast(t);
}

/*! \internal */
QString QAccessibleDisplay::imageDescription() const
{
#if QT_CONFIG(tooltip)
    return widget()->toolTip();
#else
    return QString();
#endif
}

/*! \internal */
QSize QAccessibleDisplay::imageSize() const
{
#if QT_CONFIG(label)
    QLabel *label = qobject_cast<QLabel *>(widget());
    if (!label)
#endif
        return QSize();
#if QT_CONFIG(label)
    return label->pixmap().size();
#endif
}

/*! \internal */
QPoint QAccessibleDisplay::imagePosition() const
{
#if QT_CONFIG(label)
    QLabel *label = qobject_cast<QLabel *>(widget());
    if (!label)
#endif
        return QPoint();
#if QT_CONFIG(label)
    if (label->pixmap().isNull())
        return QPoint();

    return QPoint(label->mapToGlobal(label->pos()));
#endif
}

#if QT_CONFIG(groupbox)
QAccessibleGroupBox::QAccessibleGroupBox(QWidget *w)
: QAccessibleWidgetV2(w)
{
}

QGroupBox* QAccessibleGroupBox::groupBox() const
{
    return static_cast<QGroupBox *>(widget());
}

QString QAccessibleGroupBox::text(QAccessible::Text t) const
{
    QString txt = QAccessibleWidgetV2::text(t);

    if (txt.isEmpty()) {
        switch (t) {
        case QAccessible::Name:
            txt = qt_accStripAmp(groupBox()->title());
            break;
#if QT_CONFIG(tooltip)
        case QAccessible::Description:
            txt = groupBox()->toolTip();
            break;
#endif
        case QAccessible::Accelerator:
            txt = qt_accHotKey(groupBox()->title());
            break;
        default:
            break;
        }
    }

    return txt;
}

QAccessible::State QAccessibleGroupBox::state() const
{
    QAccessible::State st = QAccessibleWidgetV2::state();
    st.checkable = groupBox()->isCheckable();
    st.checked = groupBox()->isChecked();
    return st;
}

QAccessible::Role QAccessibleGroupBox::role() const
{
    return groupBox()->isCheckable() ? QAccessible::CheckBox : QAccessible::Grouping;
}

QList<std::pair<QAccessibleInterface *, QAccessible::Relation>>
QAccessibleGroupBox::relations(QAccessible::Relation match /* = QAccessible::AllRelations */) const
{
    QList<std::pair<QAccessibleInterface *, QAccessible::Relation>> rels =
            QAccessibleWidgetV2::relations(match);

    if ((match & QAccessible::Labelled) && (!groupBox()->title().isEmpty())) {
        const QList<QWidget*> kids = _q_ac_childWidgets(widget());
        for (QWidget *kid : kids) {
            QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(kid);
            if (iface)
                rels.emplace_back(iface, QAccessible::Relation(QAccessible::Labelled));
        }
    }
    return rels;
}

QStringList QAccessibleGroupBox::actionNames() const
{
    QStringList actions = QAccessibleWidgetV2::actionNames();

    if (groupBox()->isCheckable()) {
        actions.prepend(QAccessibleActionInterface::toggleAction());
    }
    return actions;
}

void QAccessibleGroupBox::doAction(const QString &actionName)
{
    if (actionName == QAccessibleActionInterface::toggleAction())
        groupBox()->setChecked(!groupBox()->isChecked());
}

QStringList QAccessibleGroupBox::keyBindingsForAction(const QString &) const
{
    return QStringList();
}

#endif

#if QT_CONFIG(lineedit)
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
: QAccessibleWidgetV2(w, QAccessible::EditableText, name)
{
    addControllingSignal("textChanged(const QString&)"_L1);
    addControllingSignal("returnPressed()"_L1);
}

/*! Returns the line edit. */
QLineEdit *QAccessibleLineEdit::lineEdit() const
{
    return qobject_cast<QLineEdit*>(object());
}

QString QAccessibleLineEdit::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
    case QAccessible::Value:
        str = lineEdit()->displayText();
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidgetV2::text(t);
    if (str.isEmpty() && t == QAccessible::Description)
        str = lineEdit()->placeholderText();
    return str;
}

void QAccessibleLineEdit::setText(QAccessible::Text t, const QString &text)
{
    if (t != QAccessible::Value) {
        QAccessibleWidgetV2::setText(t, text);
        return;
    }

    QString newText = text;
#if QT_CONFIG(validator)
    if (lineEdit()->validator()) {
        int pos = 0;
        if (lineEdit()->validator()->validate(newText, pos) != QValidator::Acceptable)
            return;
    }
#endif
    lineEdit()->setText(newText);
}

QAccessible::State QAccessibleLineEdit::state() const
{
    QAccessible::State state = QAccessibleWidgetV2::state();

    QLineEdit *l = lineEdit();
    state.editable = true;
    if (l->isReadOnly())
        state.readOnly = true;

    if (l->echoMode() != QLineEdit::Normal)
        state.passwordEdit = true;

    state.selectableText = true;
    return state;
}

void *QAccessibleLineEdit::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface*>(this);
    if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleWidgetV2::interface_cast(t);
}

void QAccessibleLineEdit::addSelection(int startOffset, int endOffset)
{
    setSelection(0, startOffset, endOffset);
}

QString QAccessibleLineEdit::attributes(int offset, int *startOffset, int *endOffset) const
{
    // QLineEdit doesn't have text attributes
    *startOffset = *endOffset = offset;
    return QString();
}

int QAccessibleLineEdit::cursorPosition() const
{
    return lineEdit()->cursorPosition();
}

QRect QAccessibleLineEdit::characterRect(int offset) const
{
    int x = lineEdit()->d_func()->control->cursorToX(offset);
    int y = lineEdit()->textMargins().top();
    QFontMetrics fm(lineEdit()->font());
    const QString ch = text(offset, offset + 1);
    if (ch.isEmpty())
        return QRect();
    int w = fm.horizontalAdvance(ch);
    int h = fm.height();
    QRect r(x, y, w, h);
    r.moveTo(lineEdit()->mapToGlobal(r.topLeft()));
    return r;
}

int QAccessibleLineEdit::selectionCount() const
{
    return lineEdit()->hasSelectedText() ? 1 : 0;
}

int QAccessibleLineEdit::offsetAtPoint(const QPoint &point) const
{
    QPoint p = lineEdit()->mapFromGlobal(point);

    return lineEdit()->cursorPositionAt(p);
}

void QAccessibleLineEdit::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    *startOffset = *endOffset = 0;
    if (selectionIndex != 0)
        return;

    *startOffset = lineEdit()->selectionStart();
    *endOffset = *startOffset + lineEdit()->selectedText().size();
}

QString QAccessibleLineEdit::text(int startOffset, int endOffset) const
{
    if (startOffset > endOffset)
        return QString();

    return lineEdit()->displayText().mid(startOffset, endOffset - startOffset);
}

QString QAccessibleLineEdit::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
        int *startOffset, int *endOffset) const
{
    if (offset == -2)
        offset = cursorPosition();
    return QAccessibleTextInterface::textBeforeOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleLineEdit::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
        int *startOffset, int *endOffset) const
{
    if (offset == -2)
        offset = cursorPosition();
    return QAccessibleTextInterface::textAfterOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleLineEdit::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
        int *startOffset, int *endOffset) const
{
    if (offset == -2)
        offset = cursorPosition();
    return QAccessibleTextInterface::textAtOffset(offset, boundaryType, startOffset, endOffset);
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

int QAccessibleLineEdit::characterCount() const
{
    return lineEdit()->displayText().size();
}

void QAccessibleLineEdit::scrollToSubstring(int startIndex, int endIndex)
{
    lineEdit()->setCursorPosition(endIndex);
    lineEdit()->setCursorPosition(startIndex);
}

void QAccessibleLineEdit::deleteText(int startOffset, int endOffset)
{
    lineEdit()->setText(lineEdit()->text().remove(startOffset, endOffset - startOffset));
}

void QAccessibleLineEdit::insertText(int offset, const QString &text)
{
    lineEdit()->setText(lineEdit()->text().insert(offset, text));
}

void QAccessibleLineEdit::replaceText(int startOffset, int endOffset, const QString &text)
{
    lineEdit()->setText(lineEdit()->text().replace(startOffset, endOffset - startOffset, text));
}

#endif // QT_CONFIG(lineedit)

#if QT_CONFIG(progressbar)
QAccessibleProgressBar::QAccessibleProgressBar(QWidget *o)
    : QAccessibleDisplay(o)
{
    Q_ASSERT(progressBar());
}

void *QAccessibleProgressBar::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ValueInterface)
        return static_cast<QAccessibleValueInterface*>(this);
    return QAccessibleDisplay::interface_cast(t);
}

QVariant QAccessibleProgressBar::currentValue() const
{
    return progressBar()->value();
}

QVariant QAccessibleProgressBar::maximumValue() const
{
    return progressBar()->maximum();
}

QVariant QAccessibleProgressBar::minimumValue() const
{
    return progressBar()->minimum();
}

QVariant QAccessibleProgressBar::minimumStepSize() const
{
    // This is arbitrary since any value between min and max is valid.
    // Some screen readers (orca use it to calculate how many digits to display though,
    // so it makes sense to return a "sensible" value. Providing 100 increments seems ok.
    return (progressBar()->maximum() - progressBar()->minimum()) / 100.0;
}

QProgressBar *QAccessibleProgressBar::progressBar() const
{
    return qobject_cast<QProgressBar *>(object());
}
#endif


QAccessibleWindowContainer::QAccessibleWindowContainer(QWidget *w)
    : QAccessibleWidgetV2(w)
{
}

int QAccessibleWindowContainer::childCount() const
{
    if (container()->containedWindow() && QAccessible::queryAccessibleInterface(container()->containedWindow()))
        return 1;
    return 0;
}

int QAccessibleWindowContainer::indexOfChild(const QAccessibleInterface *child) const
{
    if (child->object() == container()->containedWindow())
        return 0;
    return -1;
}

QAccessibleInterface *QAccessibleWindowContainer::child(int i) const
{
    if (i == 0)
        return QAccessible::queryAccessibleInterface(container()->containedWindow());
    return nullptr;
}

QWindowContainer *QAccessibleWindowContainer::container() const
{
    return static_cast<QWindowContainer *>(widget());
}

#if QT_CONFIG(messagebox)
/*!
    \internal
    Implements QAccessibleWidget for QMessageBox
*/
QAccessibleMessageBox::QAccessibleMessageBox(QWidget *widget)
    : QAccessibleWidgetV2(widget, QAccessible::AlertMessage)
{
    Q_ASSERT(qobject_cast<QMessageBox *>(widget));
}

QMessageBox *QAccessibleMessageBox::messageBox() const
{
    return static_cast<QMessageBox *>(widget());
}

QString QAccessibleMessageBox::text(QAccessible::Text t) const
{
    QString str;

    switch (t) {
    case QAccessible::Name:
        str = QAccessibleWidgetV2::text(t);
        if (str.isEmpty()) // implies no title text is set
            str = messageBox()->text();
        break;
    case QAccessible::Value:
        str = messageBox()->text();
        break;
    case QAccessible::Help:
        str = messageBox()->informativeText();
        break;
    default:
        str = QAccessibleWidgetV2::text(t);
        break;
    }

    return str;
}
#endif

#endif // QT_CONFIG(accessibility)

QT_END_NAMESPACE
