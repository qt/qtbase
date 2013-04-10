/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qaccessible2_p.h"
#include <QtGui/QGuiApplication>
#include "qclipboard.h"
#include "qtextboundaryfinder.h"

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

/*!
    \class QAccessibleTextInterface
    \internal
    \inmodule QtGui

    \ingroup accessibility

    \brief The QAccessibleTextInterface class implements support for text handling.

    This interface corresponds to the IAccessibleText interface.
    It should be implemented for widgets that display more text than a plain label.
    Labels should be represented by only \l QAccessibleInterface
    and return their text as name (\l QAccessibleInterface::text() with \l QAccessible::Name as type).
    The QAccessibleTextInterface is typically for text that a screen reader
    might want to read line by line, and for widgets that support text selection and input.
    This interface is, for example, implemented for QLineEdit.

    Editable text objects should also implement \l QAccessibleEditableTextInterface.
    \l{IAccessible2 Specification}
*/

/*!
    \fn QAccessibleTextInterface::~QAccessibleTextInterface()
    Destructor.
*/

/*!
    \fn void QAccessibleTextInterface::addSelection(int startOffset, int endOffset)
    Select the text from \a startOffset to \a endOffset.
    The \a startOffset is the first character that will be selected.
    The \a endOffset is the first character that will not be selected.

    When the object supports multiple selections (e.g. in a word processor),
    this adds a new selection, otherwise it replaces the previous selection.

    The selection will be \a endOffset - \a startOffset characters long.
*/

/*!
    \fn QString QAccessibleTextInterface::attributes(int offset, int *startOffset, int *endOffset) const
*/

/*!
    \fn int QAccessibleTextInterface::cursorPosition() const

    Returns the current cursor position.
*/

/*!
    \fn QRect QAccessibleTextInterface::characterRect(int offset) const
*/

/*!
    \fn int QAccessibleTextInterface::selectionCount() const

    Returns the number of selections in this text.
*/

/*!
    \fn int QAccessibleTextInterface::offsetAtPoint(const QPoint &point) const
*/

/*!
    \fn void QAccessibleTextInterface::selection(int selectionIndex, int *startOffset, int *endOffset) const
*/

/*!
    \fn QString QAccessibleTextInterface::text(int startOffset, int endOffset) const

    Returns the text from \a startOffset to \a endOffset.
    The \a startOffset is the first character that will be returned.
    The \a endOffset is the first character that will not be returned.
*/

/*!
    Returns the text item of type \a boundaryType that is close to offset \a offset
    and sets \a startOffset and \a endOffset values to the start and end positions
    of that item; returns an empty string if there is no such an item.
    Sets \a startOffset and \a endOffset values to -1 on error.
*/
QString QAccessibleTextInterface::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                                   int *startOffset, int *endOffset) const
{
    const QString txt = text(0, characterCount());

    if (txt.isEmpty() || offset < 0 || offset > txt.length()) {
        *startOffset = *endOffset = -1;
        return QString();
    }
    if (offset == 0) {
        *startOffset = *endOffset = offset;
        return QString();
    }

    QTextBoundaryFinder::BoundaryType type;
    switch (boundaryType) {
    case QAccessible::CharBoundary:
        type = QTextBoundaryFinder::Grapheme;
        break;
    case QAccessible::WordBoundary:
        type = QTextBoundaryFinder::Word;
        break;
    case QAccessible::SentenceBoundary:
        type = QTextBoundaryFinder::Sentence;
        break;
    default:
        // in any other case return the whole line
        *startOffset = 0;
        *endOffset = txt.length();
        return txt;
    }

    // keep behavior in sync with QTextCursor::movePosition()!

    QTextBoundaryFinder boundary(type, txt);
    boundary.setPosition(offset);

    do {
        if ((boundary.boundaryReasons() & (QTextBoundaryFinder::StartOfItem | QTextBoundaryFinder::EndOfItem)))
            break;
    } while (boundary.toPreviousBoundary() > 0);
    Q_ASSERT(boundary.position() >= 0);
    *endOffset = boundary.position();

    while (boundary.toPreviousBoundary() > 0) {
        if ((boundary.boundaryReasons() & (QTextBoundaryFinder::StartOfItem | QTextBoundaryFinder::EndOfItem)))
            break;
    }
    Q_ASSERT(boundary.position() >= 0);
    *startOffset = boundary.position();

    return txt.mid(*startOffset, *endOffset - *startOffset);
}

/*!
    Returns the text item of type \a boundaryType that is right after offset \a offset
    and sets \a startOffset and \a endOffset values to the start and end positions
    of that item; returns an empty string if there is no such an item.
    Sets \a startOffset and \a endOffset values to -1 on error.
*/
QString QAccessibleTextInterface::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                                  int *startOffset, int *endOffset) const
{
    const QString txt = text(0, characterCount());

    if (txt.isEmpty() || offset < 0 || offset > txt.length()) {
        *startOffset = *endOffset = -1;
        return QString();
    }
    if (offset == txt.length()) {
        *startOffset = *endOffset = offset;
        return QString();
    }

    QTextBoundaryFinder::BoundaryType type;
    switch (boundaryType) {
    case QAccessible::CharBoundary:
        type = QTextBoundaryFinder::Grapheme;
        break;
    case QAccessible::WordBoundary:
        type = QTextBoundaryFinder::Word;
        break;
    case QAccessible::SentenceBoundary:
        type = QTextBoundaryFinder::Sentence;
        break;
    default:
        // in any other case return the whole line
        *startOffset = 0;
        *endOffset = txt.length();
        return txt;
    }

    // keep behavior in sync with QTextCursor::movePosition()!

    QTextBoundaryFinder boundary(type, txt);
    boundary.setPosition(offset);

    while (boundary.toNextBoundary() < txt.length()) {
        if ((boundary.boundaryReasons() & (QTextBoundaryFinder::StartOfItem | QTextBoundaryFinder::EndOfItem)))
            break;
    }
    Q_ASSERT(boundary.position() <= txt.length());
    *startOffset = boundary.position();

    while (boundary.toNextBoundary() < txt.length()) {
        if ((boundary.boundaryReasons() & (QTextBoundaryFinder::StartOfItem | QTextBoundaryFinder::EndOfItem)))
            break;
    }
    Q_ASSERT(boundary.position() <= txt.length());
    *endOffset = boundary.position();

    return txt.mid(*startOffset, *endOffset - *startOffset);
}

/*!
    Returns the text item of type \a boundaryType at offset \a offset
    and sets \a startOffset and \a endOffset values to the start and end positions
    of that item; returns an empty string if there is no such an item.
    Sets \a startOffset and \a endOffset values to -1 on error.
*/
QString QAccessibleTextInterface::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                               int *startOffset, int *endOffset) const
{
    const QString txt = text(0, characterCount());

    if (txt.isEmpty() || offset < 0 || offset > txt.length()) {
        *startOffset = *endOffset = -1;
        return QString();
    }
    if (offset == txt.length()) {
        *startOffset = *endOffset = offset;
        return QString();
    }

    QTextBoundaryFinder::BoundaryType type;
    switch (boundaryType) {
    case QAccessible::CharBoundary:
        type = QTextBoundaryFinder::Grapheme;
        break;
    case QAccessible::WordBoundary:
        type = QTextBoundaryFinder::Word;
        break;
    case QAccessible::SentenceBoundary:
        type = QTextBoundaryFinder::Sentence;
        break;
    default:
        // in any other case return the whole line
        *startOffset = 0;
        *endOffset = txt.length();
        return txt;
    }

    // keep behavior in sync with QTextCursor::movePosition()!

    QTextBoundaryFinder boundary(type, txt);
    boundary.setPosition(offset);

    do {
        if ((boundary.boundaryReasons() & (QTextBoundaryFinder::StartOfItem | QTextBoundaryFinder::EndOfItem)))
            break;
    } while (boundary.toPreviousBoundary() > 0);
    Q_ASSERT(boundary.position() >= 0);
    *startOffset = boundary.position();

    while (boundary.toNextBoundary() < txt.length()) {
        if ((boundary.boundaryReasons() & (QTextBoundaryFinder::StartOfItem | QTextBoundaryFinder::EndOfItem)))
            break;
    }
    Q_ASSERT(boundary.position() <= txt.length());
    *endOffset = boundary.position();

    return txt.mid(*startOffset, *endOffset - *startOffset);
}

/*!
    \fn void QAccessibleTextInterface::removeSelection(int selectionIndex)

    Clears the selection with \a index selectionIndex.
*/

/*!
    \fn void QAccessibleTextInterface::setCursorPosition(int position)

    Moves the cursor to \a position.
*/

/*!
    \fn void QAccessibleTextInterface::setSelection(int selectionIndex, int startOffset, int endOffset)

    Set the selection \a selectionIndex to the range from \a startOffset to \a endOffset.

    \sa addSelection(), removeSelection()
*/

/*!
    \fn int QAccessibleTextInterface::characterCount() const

    Returns the length of the text (total size including spaces).
*/

/*!
    \fn void QAccessibleTextInterface::scrollToSubstring(int startIndex, int endIndex)

    Ensures that the text between \a startIndex and \a endIndex is visible.
*/

/*!
    \class QAccessibleEditableTextInterface
    \ingroup accessibility
    \inmodule QtGui
    \internal

    \brief The QAccessibleEditableTextInterface class implements support for objects with editable text.

    When implementing this interface you will almost certainly also want to implement \l QAccessibleTextInterface.

    \sa QAccessibleInterface

    \l{IAccessible2 Specification}
*/

/*!
    \fn QAccessibleEditableTextInterface::~QAccessibleEditableTextInterface()


*/

/*!
    \fn void QAccessibleEditableTextInterface::deleteText(int startOffset, int endOffset)

    Deletes the text from \a startOffset to \a endOffset.
*/

/*!
    \fn void QAccessibleEditableTextInterface::insertText(int offset, const QString &text)

    Inserts \a text at position \a offset.
*/

/*!
    \fn void QAccessibleEditableTextInterface::replaceText(int startOffset, int endOffset, const QString &text)

    Removes the text from \a startOffset to \a endOffset and instead inserts \a text.
*/

/*!
    \class QAccessibleValueInterface
    \inmodule QtGui
    \ingroup accessibility
    \internal

    \brief The QAccessibleValueInterface class implements support for objects that manipulate a value.

    This interface should be implemented by accessible objects that represent a value.
    Examples are spinner, slider, dial and scroll bar.

    Instead of forcing the user to deal with the individual parts of the widgets, this interface
    gives an easier approach to the kind of widget it represents.

    Usually this interface is implemented by classes that also implement \l QAccessibleInterface.

    \l{IAccessible2 Specification}
*/

/*!
    \fn QAccessibleValueInterface::~QAccessibleValueInterface()
    Destructor.
*/

/*!
    \fn QVariant QAccessibleValueInterface::currentValue() const

    Returns the current value of the widget. This is usually a double or int.
    \sa setCurrentValue()
*/

/*!
    \fn void QAccessibleValueInterface::setCurrentValue(const QVariant &value)

    Sets the \a value. If the desired \a value is out of the range of permissible values,
    this call will be ignored.

    \sa currentValue(), minimumValue(), maximumValue()
*/

/*!
    \fn QVariant QAccessibleValueInterface::maximumValue() const

    Returns the maximum value this object accepts.
    \sa minimumValue(), currentValue()
*/

/*!
    \fn QVariant QAccessibleValueInterface::minimumValue() const

    Returns the minimum value this object accepts.
    \sa maximumValue(), currentValue()
*/

/*!
    \fn QVariant QAccessibleValueInterface::minimumStepSize() const

    Returns the minimum step size for the accessible.
    This is the smalles increment that makes sense when changing the value.
    When programatically changing the value it should always be a multiple
    of the minimum step size.

    Some tools use this value even when the setCurrentValue does not
    perform any action. Progress bars for example are read-only but
    should return their range divided by 100.
*/

/*!
    \class QAccessibleImageInterface
    \inmodule QtGui
    \ingroup accessibility
    \internal
    \preliminary

    \brief The QAccessibleImageInterface class implements support for
    the IAccessibleImage interface.

    \l{IAccessible2 Specification}
*/

/*!
    \class QAccessibleTableCellInterface
    \inmodule QtGui
    \ingroup accessibility
    \internal

    \brief The QAccessibleTableCellInterface class implements support for
    the IAccessibleTable2 Cell interface.

    \l{IAccessible2 Specification}
*/

/*!
    \class QAccessibleTableInterface
    \ingroup accessibility
    \internal

    \brief The QAccessibleTableInterface class implements support for
    the IAccessibleTable2 interface.

    \l{IAccessible2 Specification}
*/


/*!
    \class QAccessibleActionInterface
    \ingroup accessibility
    \internal

    \brief The QAccessibleActionInterface class implements support for
    invocable actions in the interface.

    Accessible objects should implement the action interface if they support user interaction.
    Usually this interface is implemented by classes that also implement \l QAccessibleInterface.

    The supported actions should use the predefined actions offered in this class unless they do not
    fit a predefined action. In that case a custom action can be added.

    When subclassing QAccessibleActionInterface you need to provide a list of actionNames which
    is the primary means to discover the available actions. Action names are never localized.
    In order to present actions to the user there are two functions that need to return localized versions
    of the name and give a description of the action. For the predefined action names use
    \l QAccessibleActionInterface::localizedActionName() and \l QAccessibleActionInterface::localizedActionDescription()
    to return their localized counterparts.

    In general you should use one of the predefined action names, unless describing an action that does not fit these:
    \table
    \header \li Action name         \li Description
    \row    \li \l toggleAction()   \li toggles the item (checkbox, radio button, switch, ...)
    \row    \li \l decreaseAction() \li decrease the value of the accessible (e.g. spinbox)
    \row    \li \l increaseAction() \li increase the value of the accessible (e.g. spinbox)
    \row    \li \l pressAction()    \li press or click or activate the accessible (should correspont to clicking the object with the mouse)
    \row    \li \l setFocusAction() \li set the focus to this accessible
    \row    \li \l showMenuAction() \li show a context menu, corresponds to right-clicks
    \endtable

    In order to invoke the action, \l doAction() is called with an action name.

    Most widgets will simply implement \l pressAction(). This is what happens when the widget is activated by
    being clicked, space pressed or similar.

    \l{IAccessible2 Specification}
*/

/*!
    \fn QStringList QAccessibleActionInterface::actionNames() const

    Returns the list of actions supported by this accessible object.
    The actions returned should be in preferred order,
    i.e. the action that the user most likely wants to trigger should be returned first,
    while the least likely action should be returned last.

    The list does only contain actions that can be invoked.
    It won't return disabled actions, or actions associated with disabled UI controls.

    The list can be empty.

    Note that this list is not localized. For a localized representation re-implement \l localizedActionName()
    and \l localizedActionDescription()

    \sa doAction(), localizedActionName(), localizedActionDescription()
*/

/*!
    \fn QString QAccessibleActionInterface::localizedActionName(const QString &actionName) const

    Returns a localized action name of \a actionName.

    For custom actions this function has to be re-implemented.
    When using one of the default names, you can call this function in QAccessibleActionInterface
    to get the localized string.

    \sa actionNames(), localizedActionDescription()
*/

/*!
    \fn QString QAccessibleActionInterface::localizedActionDescription(const QString &actionName) const

    Returns a localized action description of the action \a actionName.

    When using one of the default names, you can call this function in QAccessibleActionInterface
    to get the localized string.

    \sa actionNames(), localizedActionName()
*/

/*!
    \fn void QAccessibleActionInterface::doAction(const QString &actionName)

    Invokes the action specified by \a actionName.
    Note that \a actionName is the non-localized name as returned by \l actionNames()
    This function is usually implemented by calling the same functions
    that other user interaction, such as clicking the object, would trigger.

    \sa actionNames()
*/

/*!
    \fn QStringList QAccessibleActionInterface::keyBindingsForAction(const QString &actionName) const

    Returns a list of the keyboard shortcuts available for invoking the action named \a actionName.

    This is important to let users learn alternative ways of using the application by emphasizing the keyboard.

    \sa actionNames()
*/


struct QAccessibleActionStrings
{
    QAccessibleActionStrings() :
        pressAction(QStringLiteral(QT_TRANSLATE_NOOP("QAccessibleActionInterface", "Press"))),
        increaseAction(QStringLiteral(QT_TRANSLATE_NOOP("QAccessibleActionInterface", "Increase"))),
        decreaseAction(QStringLiteral(QT_TRANSLATE_NOOP("QAccessibleActionInterface", "Decrease"))),
        showMenuAction(QStringLiteral(QT_TRANSLATE_NOOP("QAccessibleActionInterface", "ShowMenu"))),
        setFocusAction(QStringLiteral(QT_TRANSLATE_NOOP("QAccessibleActionInterface", "SetFocus"))),
        toggleAction(QStringLiteral(QT_TRANSLATE_NOOP("QAccessibleActionInterface", "Toggle"))) {}

    const QString pressAction;
    const QString increaseAction;
    const QString decreaseAction;
    const QString showMenuAction;
    const QString setFocusAction;
    const QString toggleAction;
};

Q_GLOBAL_STATIC(QAccessibleActionStrings, accessibleActionStrings)

QString QAccessibleActionInterface::localizedActionName(const QString &actionName) const
{
    return QAccessibleActionInterface::tr(qPrintable(actionName));
}

QString QAccessibleActionInterface::localizedActionDescription(const QString &actionName) const
{
    const QAccessibleActionStrings *strings = accessibleActionStrings();
    if (actionName == strings->pressAction)
        return tr("Triggers the action");
    else if (actionName == strings->increaseAction)
        return tr("Increase the value");
    else if (actionName == strings->decreaseAction)
        return tr("Decrease the value");
    else if (actionName == strings->showMenuAction)
        return tr("Shows the menu");
    else if (actionName == strings->setFocusAction)
        return tr("Sets the focus");
    else if (actionName == strings->toggleAction)
        return tr("Toggles the state");

    return QString();
}

/*!
    Returns the name of the press default action.
    \sa actionNames(), localizedActionName()
  */
const QString &QAccessibleActionInterface::pressAction()
{
    return accessibleActionStrings()->pressAction;
}

/*!
    Returns the name of the increase default action.
    \sa actionNames(), localizedActionName()
  */
const QString &QAccessibleActionInterface::increaseAction()
{
    return accessibleActionStrings()->increaseAction;
}

/*!
    Returns the name of the decrease default action.
    \sa actionNames(), localizedActionName()
  */
const QString &QAccessibleActionInterface::decreaseAction()
{
    return accessibleActionStrings()->decreaseAction;
}

/*!
    Returns the name of the show menu default action.
    \sa actionNames(), localizedActionName()
  */
const QString &QAccessibleActionInterface::showMenuAction()
{
    return accessibleActionStrings()->showMenuAction;
}

/*!
    Returns the name of the set focus default action.
    \sa actionNames(), localizedActionName()
  */
const QString &QAccessibleActionInterface::setFocusAction()
{
    return accessibleActionStrings()->setFocusAction;
}

/*!
    Returns the name of the toggle default action.
    \sa actionNames(), localizedActionName()
  */
const QString &QAccessibleActionInterface::toggleAction()
{
    return accessibleActionStrings()->toggleAction;
}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
