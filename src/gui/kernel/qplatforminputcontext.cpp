/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatforminputcontext.h"
#include <qguiapplication.h>
#include <QRect>
#include "private/qkeymapper_p.h"
#include <qpa/qplatforminputcontext_p.h>

#include <QtGui/qtransform.h>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformInputContext
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \brief The QPlatformInputContext class abstracts the input method dependent data and composing state.

    An input method is responsible for inputting complex text that cannot
    be inputted via simple keymap. It converts a sequence of input
    events (typically key events) into a text string through the input
    method specific converting process. The class of the processes are
    widely ranging from simple finite state machine to complex text
    translator that pools a whole paragraph of a text with text
    editing capability to perform grammar and semantic analysis.

    To abstract such different input method specific intermediate
    information, Qt offers the QPlatformInputContext as base class. The
    concept is well known as 'input context' in the input method
    domain. An input context is created for a text widget in response
    to a demand. It is ensured that an input context is prepared for
    an input method before input to a text widget.

    QPlatformInputContext provides an interface the actual input methods
    can derive from by reimplementing methods.

    \sa QInputMethod
*/

/*!
    \internal
 */
QPlatformInputContext::QPlatformInputContext()
    : QObject(*(new QPlatformInputContextPrivate))
{
}

/*!
    \internal
 */
QPlatformInputContext::~QPlatformInputContext()
{
}

/*!
    Returns input context validity. Deriving implementations should return true.
 */
bool QPlatformInputContext::isValid() const
{
    return false;
}

/*!
    Returns whether the implementation supports \a capability.
    \internal
    \since 5.4
 */
bool QPlatformInputContext::hasCapability(Capability capability) const
{
    Q_UNUSED(capability)
    return true;
}

/*!
    Method to be called when input method needs to be reset. Called by QInputMethod::reset().
    No further QInputMethodEvents should be sent as response.
 */
void QPlatformInputContext::reset()
{
}

void QPlatformInputContext::commit()
{
}

/*!
    Notification on editor updates. Called by QInputMethod::update().
 */
void QPlatformInputContext::update(Qt::InputMethodQueries)
{
}

/*!
    Called when when the word currently being composed in input item is tapped by
    the user. Input methods often use this information to offer more word
    suggestions to the user.
 */
void QPlatformInputContext::invokeAction(QInputMethod::Action action, int cursorPosition)
{
    Q_UNUSED(cursorPosition)
    // Default behavior for simple ephemeral input contexts. Some
    // complex input contexts should not be reset here.
    if (action == QInputMethod::Click)
        reset();
}

/*!
    This function can be reimplemented to filter input events.
    Return true if the event has been consumed. Otherwise, the unfiltered event will
    be forwarded to widgets as ordinary way. Although the input events have accept()
    and ignore() methods, leave it untouched.
*/
bool QPlatformInputContext::filterEvent(const QEvent *event)
{
    Q_UNUSED(event)
    return false;
}

/*!
    This function can be reimplemented to return virtual keyboard rectangle in currently active
    window coordinates. Default implementation returns invalid rectangle.
 */
QRectF QPlatformInputContext::keyboardRect() const
{
    return QRectF();
}

/*!
    Active QPlatformInputContext is responsible for providing keyboardRectangle property to QInputMethod.
    In addition of providing the value in keyboardRect function, it also needs to call this emit
    function whenever the property changes.
 */
void QPlatformInputContext::emitKeyboardRectChanged()
{
    emit QGuiApplication::inputMethod()->keyboardRectangleChanged();
}

/*!
    This function can be reimplemented to return true whenever input method is animating
    shown or hidden. Default implementation returns \c false.
 */
bool QPlatformInputContext::isAnimating() const
{
    return false;
}

/*!
    Active QPlatformInputContext is responsible for providing animating property to QInputMethod.
    In addition of providing the value in isAnimation function, it also needs to call this emit
    function whenever the property changes.
 */
void QPlatformInputContext::emitAnimatingChanged()
{
    emit QGuiApplication::inputMethod()->animatingChanged();
}

/*!
    Request to show input panel.
 */
void QPlatformInputContext::showInputPanel()
{
}

/*!
    Request to hide input panel.
 */
void QPlatformInputContext::hideInputPanel()
{
}

/*!
    Returns input panel visibility status. Default implementation returns \c false.
 */
bool QPlatformInputContext::isInputPanelVisible() const
{
    return false;
}

/*!
    Active QPlatformInputContext is responsible for providing visible property to QInputMethod.
    In addition of providing the value in isInputPanelVisible function, it also needs to call this emit
    function whenever the property changes.
 */
void QPlatformInputContext::emitInputPanelVisibleChanged()
{
    emit QGuiApplication::inputMethod()->visibleChanged();
}

QLocale QPlatformInputContext::locale() const
{
    return qt_keymapper_private()->keyboardInputLocale;
}

void QPlatformInputContext::emitLocaleChanged()
{
    emit QGuiApplication::inputMethod()->localeChanged();
}

Qt::LayoutDirection QPlatformInputContext::inputDirection() const
{
    return qt_keymapper_private()->keyboardInputDirection;
}

void QPlatformInputContext::emitInputDirectionChanged(Qt::LayoutDirection newDirection)
{
    emit QGuiApplication::inputMethod()->inputDirectionChanged(newDirection);
}

/*!
    This virtual method gets called to notify updated focus to \a object.
    \warning Input methods must not call this function directly.
 */
void QPlatformInputContext::setFocusObject(QObject *object)
{
    Q_UNUSED(object)
}

/*!
    Returns \c true if current focus object supports input method events.
 */
bool QPlatformInputContext::inputMethodAccepted() const
{
    return QPlatformInputContextPrivate::s_inputMethodAccepted;
}

bool QPlatformInputContextPrivate::s_inputMethodAccepted = false;

void QPlatformInputContextPrivate::setInputMethodAccepted(bool accepted)
{
    QPlatformInputContextPrivate::s_inputMethodAccepted = accepted;
}

/*!
 * \brief QPlatformInputContext::setSelectionOnFocusObject
 * \param anchorPos Beginning of selection in currently active window coordinates
 * \param cursorPos End of selection in currently active window coordinates
 */
void QPlatformInputContext::setSelectionOnFocusObject(const QPointF &anchorPos, const QPointF &cursorPos)
{
    QObject *focus = qApp->focusObject();
    if (!focus)
        return;

    QInputMethod *im = QGuiApplication::inputMethod();
    const QTransform mapToLocal = im->inputItemTransform().inverted();
    bool success;
    int anchor = QInputMethod::queryFocusObject(Qt::ImCursorPosition, anchorPos * mapToLocal).toInt(&success);
    if (success) {
        int cursor = QInputMethod::queryFocusObject(Qt::ImCursorPosition, cursorPos * mapToLocal).toInt(&success);
        if (success) {
            QList<QInputMethodEvent::Attribute> imAttributes;
            imAttributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection, anchor, cursor - anchor, QVariant()));
            QInputMethodEvent event(QString(), imAttributes);
            QGuiApplication::sendEvent(focus, &event);
        }
    }
}

QT_END_NAMESPACE
