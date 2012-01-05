/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qaccessible2.h"
#include <QtGui/QGuiApplication>
#include "qclipboard.h"
#include "qtextboundaryfinder.h"

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

/*!
    \namespace QAccessible2
    \ingroup accessibility
    \internal
    \preliminary

    \brief The QAccessible2 namespace defines constants relating to
    IAccessible2-based interfaces

    \link http://www.linux-foundation.org/en/Accessibility/IAccessible2 IAccessible2 Specification \endlink
*/

/*!
    \class QAccessibleTextInterface

    \ingroup accessibility
    \internal
    \preliminary

    \brief The QAccessibleTextInterface class implements support for
    the IAccessibleText interface.

    \link http://www.linux-foundation.org/en/Accessibility/IAccessible2 IAccessible2 Specification \endlink
*/

/*!
    \class QAccessibleEditableTextInterface
    \ingroup accessibility
    \internal
    \preliminary

    \brief The QAccessibleEditableTextInterface class implements support for
    the IAccessibleEditableText interface.

    \link http://www.linux-foundation.org/en/Accessibility/IAccessible2 IAccessible2 Specification \endlink
*/

/*!
    \class QAccessibleSimpleEditableTextInterface
    \ingroup accessibility
    \internal
    \preliminary

    \brief The QAccessibleSimpleEditableTextInterface class is a convenience class for
    text-based widgets.

    \link http://www.linux-foundation.org/en/Accessibility/IAccessible2 IAccessible2 Specification \endlink
*/

/*!
    \class QAccessibleValueInterface
    \ingroup accessibility
    \internal
    \preliminary

    \brief The QAccessibleValueInterface class implements support for
    the IAccessibleValue interface.

    \link http://www.linux-foundation.org/en/Accessibility/IAccessible2 IAccessible2 Specification \endlink
*/

/*!
    \class QAccessibleImageInterface
    \ingroup accessibility
    \internal
    \preliminary

    \brief The QAccessibleImageInterface class implements support for
    the IAccessibleImage interface.

    \link http://www.linux-foundation.org/en/Accessibility/IAccessible2 IAccessible2 Specification \endlink
*/

/*!
    \class QAccessibleActionInterface
    \ingroup accessibility
    \internal
    \preliminary

    \brief The QAccessibleActionInterface class implements support for
    invocable actions in the interface.

    Each accessible should implement the action interface if it supports any actions.
    The supported actions should adhere to the naming scheme inside the QAccessible2 namespace.
    Custom actions can be added.

    When subclassing QAccessibleActionInterface you need to provide a list of actionNames which
    is the primary means to discover the available actions. Action names are never localized.
    In order to present actions to the user there are two functions that need to return localized versions
    of the name and give a description of the action.

    In order to invoke the action, doAction is called with an action name.

    Most widgets will simply implement the PressAction. This is what happens when the widget is activated by
    being clicked on, space pressed or similar.

    \link http://www.linux-foundation.org/en/Accessibility/IAccessible2 IAccessible2 Specification \endlink
*/

/*!
    \fn QStringList QAccessibleActionInterface::actionNames() const

    Returns a list of valid actions. The actions returned should be in preferred order,
    i.e. the action that the user most likely wants to trigger should be returned first,
    while the least likely action should be returned last.

    The list does only contain actions that *can* be invoked. Therefore it,
    won't return disabled actions, or actions associated with disabled UI
    controls.

    The list can also be empty.

    \sa localizedActionName(), doAction()
*/

/*!
    \fn QString QAccessibleActionInterface::localizedActionName(const QString &name) const

    Returns a localized action name of \a name.

    \sa actionNames(), localizedActionDescription()
*/

/*!
    \fn QString QAccessibleActionInterface::localizedActionDescription(const QString &name) const

    Returns a localized action description of \a name.

    This is what should be presented to the user. The actionNames should always
    be untranslated to make them consistent for screen readers.

    \sa actionNames(), localizedActionName()
*/

/*!
    \fn void QAccessibleActionInterface::doAction(const QString &actionName)

    Invokes the action specified by \a actionName

    \sa actionNames()
*/

/*!
    \fn QStringList QAccessibleActionInterface::keyBindingsForAction(const QString &actionName) const

    Returns a list of the keyboard shortcuts available for invoking the action named \a actionName

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
        checkAction(QStringLiteral(QT_TRANSLATE_NOOP("QAccessibleActionInterface", "Check"))),
        uncheckAction(QStringLiteral(QT_TRANSLATE_NOOP("QAccessibleActionInterface", "Uncheck"))) {}

    const QString pressAction;
    const QString increaseAction;
    const QString decreaseAction;
    const QString showMenuAction;
    const QString setFocusAction;
    const QString checkAction;
    const QString uncheckAction;
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
    else if (actionName == strings->checkAction)
        return tr("Checks the checkbox");
    else if (actionName == strings->uncheckAction)
        return tr("Unchecks the checkbox");

    return QString();
}

const QString &QAccessibleActionInterface::pressAction()
{
    return accessibleActionStrings()->pressAction;
}

const QString &QAccessibleActionInterface::increaseAction()
{
    return accessibleActionStrings()->increaseAction;
}

const QString &QAccessibleActionInterface::decreaseAction()
{
    return accessibleActionStrings()->decreaseAction;
}

const QString &QAccessibleActionInterface::showMenuAction()
{
    return accessibleActionStrings()->showMenuAction;
}

const QString &QAccessibleActionInterface::setFocusAction()
{
    return accessibleActionStrings()->setFocusAction;
}

const QString &QAccessibleActionInterface::checkAction()
{
    return accessibleActionStrings()->checkAction;
}

const QString &QAccessibleActionInterface::uncheckAction()
{
    return accessibleActionStrings()->uncheckAction;
}

/*!
  \internal
*/
QString Q_GUI_EXPORT qTextBeforeOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text)
{
    QTextBoundaryFinder::BoundaryType type;
    switch (boundaryType) {
    case QAccessible2::CharBoundary:
        type = QTextBoundaryFinder::Grapheme;
        break;
    case QAccessible2::WordBoundary:
        type = QTextBoundaryFinder::Word;
        break;
    case QAccessible2::SentenceBoundary:
        type = QTextBoundaryFinder::Sentence;
        break;
    default:
        // in any other case return the whole line
        *startOffset = 0;
        *endOffset = text.length();
        return text;
    }

    QTextBoundaryFinder boundary(type, text);
    boundary.setPosition(offset);

    if (!boundary.isAtBoundary()) {
        boundary.toPreviousBoundary();
    }
    boundary.toPreviousBoundary();
    *startOffset = boundary.position();
    boundary.toNextBoundary();
    *endOffset = boundary.position();

    return text.mid(*startOffset, *endOffset - *startOffset);
}

/*!
  \internal
*/
QString Q_GUI_EXPORT qTextAfterOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text)
{
    QTextBoundaryFinder::BoundaryType type;
    switch (boundaryType) {
    case QAccessible2::CharBoundary:
        type = QTextBoundaryFinder::Grapheme;
        break;
    case QAccessible2::WordBoundary:
        type = QTextBoundaryFinder::Word;
        break;
    case QAccessible2::SentenceBoundary:
        type = QTextBoundaryFinder::Sentence;
        break;
    default:
        // in any other case return the whole line
        *startOffset = 0;
        *endOffset = text.length();
        return text;
    }

    QTextBoundaryFinder boundary(type, text);
    boundary.setPosition(offset);

    boundary.toNextBoundary();
    *startOffset = boundary.position();
    boundary.toNextBoundary();
    *endOffset = boundary.position();

    return text.mid(*startOffset, *endOffset - *startOffset);
}

/*!
  \internal
*/
QString Q_GUI_EXPORT qTextAtOffsetFromString(int offset, QAccessible2::BoundaryType boundaryType,
        int *startOffset, int *endOffset, const QString& text)
{
    QTextBoundaryFinder::BoundaryType type;
    switch (boundaryType) {
    case QAccessible2::CharBoundary:
        type = QTextBoundaryFinder::Grapheme;
        break;
    case QAccessible2::WordBoundary:
        type = QTextBoundaryFinder::Word;
        break;
    case QAccessible2::SentenceBoundary:
        type = QTextBoundaryFinder::Sentence;
        break;
    default:
        // in any other case return the whole line
        *startOffset = 0;
        *endOffset = text.length();
        return text;
    }

    QTextBoundaryFinder boundary(type, text);
    boundary.setPosition(offset);

    if (!boundary.isAtBoundary()) {
        boundary.toPreviousBoundary();
    }
    *startOffset = boundary.position();
    boundary.toNextBoundary();
    *endOffset = boundary.position();

    return text.mid(*startOffset, *endOffset - *startOffset);
}

QAccessibleSimpleEditableTextInterface::QAccessibleSimpleEditableTextInterface(
                QAccessibleInterface *accessibleInterface)
    : iface(accessibleInterface)
{
    Q_ASSERT(iface);
}

#ifndef QT_NO_CLIPBOARD
static QString textForRange(QAccessibleInterface *iface, int startOffset, int endOffset)
{
    return iface->text(QAccessible::Value).mid(startOffset, endOffset - startOffset);
}
#endif

void QAccessibleSimpleEditableTextInterface::copyText(int startOffset, int endOffset) const
{
#ifdef QT_NO_CLIPBOARD
    Q_UNUSED(startOffset);
    Q_UNUSED(endOffset);
#else
    QGuiApplication::clipboard()->setText(textForRange(iface, startOffset, endOffset));
#endif
}

void QAccessibleSimpleEditableTextInterface::deleteText(int startOffset, int endOffset)
{
    QString txt = iface->text(QAccessible::Value);
    txt.remove(startOffset, endOffset - startOffset);
    iface->setText(QAccessible::Value, txt);
}

void QAccessibleSimpleEditableTextInterface::insertText(int offset, const QString &text)
{
    QString txt = iface->text(QAccessible::Value);
    txt.insert(offset, text);
    iface->setText(QAccessible::Value, txt);
}

void QAccessibleSimpleEditableTextInterface::cutText(int startOffset, int endOffset)
{
#ifdef QT_NO_CLIPBOARD
    Q_UNUSED(startOffset);
    Q_UNUSED(endOffset);
#else
    QString sub = textForRange(iface, startOffset, endOffset);
    deleteText(startOffset, endOffset);
    QGuiApplication::clipboard()->setText(sub);
#endif
}

void QAccessibleSimpleEditableTextInterface::pasteText(int offset)
{
#ifdef QT_NO_CLIPBOARD
    Q_UNUSED(offset);
#else
    QString txt = iface->text(QAccessible::Value);
    txt.insert(offset, QGuiApplication::clipboard()->text());
    iface->setText(QAccessible::Value, txt);
#endif
}

void QAccessibleSimpleEditableTextInterface::replaceText(int startOffset, int endOffset, const QString &text)
{
    QString txt = iface->text(QAccessible::Value);
    txt.replace(startOffset, endOffset - startOffset, text);
    iface->setText(QAccessible::Value, txt);
}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
