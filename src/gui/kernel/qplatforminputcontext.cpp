// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatforminputcontext.h"
#include <qguiapplication.h>
#include <QRect>
#include "private/qkeymapper_p.h"
#include "private/qhighdpiscaling_p.h"
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
    // Delay initialization of cached input direction
    // until super class has finished constructing.
    QMetaObject::invokeMethod(this, [this]{
        m_inputDirection = inputDirection();
    }, Qt::QueuedConnection);
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
    Q_UNUSED(capability);
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
    Called when the word currently being composed in the input item is tapped by
    the user. Input methods often use this information to offer more word
    suggestions to the user.
 */
void QPlatformInputContext::invokeAction(QInputMethod::Action action, int cursorPosition)
{
    Q_UNUSED(cursorPosition);
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
    Q_UNUSED(event);
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
    return QLocale::system();
}

void QPlatformInputContext::emitLocaleChanged()
{
    emit QGuiApplication::inputMethod()->localeChanged();

    // Changing the locale might have updated the input direction
    emitInputDirectionChanged(inputDirection());
}

Qt::LayoutDirection QPlatformInputContext::inputDirection() const
{
    return locale().textDirection();
}

void QPlatformInputContext::emitInputDirectionChanged(Qt::LayoutDirection newDirection)
{
    if (newDirection == m_inputDirection)
        return;

    emit QGuiApplication::inputMethod()->inputDirectionChanged(newDirection);
    m_inputDirection = newDirection;
}

/*!
    This virtual method gets called to notify updated focus to \a object.
    \warning Input methods must not call this function directly.
 */
void QPlatformInputContext::setFocusObject(QObject *object)
{
    Q_UNUSED(object);
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
   \brief QPlatformInputContext::setSelectionOnFocusObject
   \param anchorPos Beginning of selection in currently active window native coordinates
   \param cursorPos End of selection in currently active window native coordinates
*/
void QPlatformInputContext::setSelectionOnFocusObject(const QPointF &nativeAnchorPos, const QPointF &nativeCursorPos)
{
    QObject *focus = qApp->focusObject();
    if (!focus)
        return;

    QWindow *window = qApp->focusWindow();
    const QPointF &anchorPos = QHighDpi::fromNativePixels(nativeAnchorPos, window);
    const QPointF &cursorPos = QHighDpi::fromNativePixels(nativeCursorPos, window);

    QInputMethod *im = QGuiApplication::inputMethod();
    const QTransform mapToLocal = im->inputItemTransform().inverted();
    bool success;
    int anchor = QInputMethod::queryFocusObject(Qt::ImCursorPosition, anchorPos * mapToLocal).toInt(&success);
    if (success) {
        int cursor = QInputMethod::queryFocusObject(Qt::ImCursorPosition, cursorPos * mapToLocal).toInt(&success);
        if (success) {
            if (anchor == cursor && anchorPos != cursorPos)
                return;
            QList<QInputMethodEvent::Attribute> imAttributes;
            imAttributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection, anchor, cursor - anchor, QVariant()));
            QInputMethodEvent event(QString(), imAttributes);
            QGuiApplication::sendEvent(focus, &event);
        }
    }
}

/*!
   \brief QPlatformInputContext::queryFocusObject

    Queries the current foucus object with a window position in native pixels.
*/
QVariant QPlatformInputContext::queryFocusObject(Qt::InputMethodQuery query, QPointF nativePosition)
{
    const QPointF position = QHighDpi::fromNativePixels(nativePosition, QGuiApplication::focusWindow());
    const QInputMethod *im = QGuiApplication::inputMethod();
    const QTransform mapToLocal = im->inputItemTransform().inverted();
    return im->queryFocusObject(query, mapToLocal.map(position));
}

/*!
   \brief QPlatformInputContext::inputItemRectangle

    Returns the input item rectangle for the currently active window
    and input methiod in native window coordinates.
*/
QRectF QPlatformInputContext::inputItemRectangle()
{
    QInputMethod *im = QGuiApplication::inputMethod();
    const QRectF deviceIndependentRectangle = im->inputItemTransform().mapRect(im->inputItemRectangle());
    return QHighDpi::toNativePixels(deviceIndependentRectangle, QGuiApplication::focusWindow());
}

/*!
   \brief QPlatformInputContext::inputItemClipRectangle

    Returns the input item clip rectangle for the currently active window
    and input methiod in native window coordinates.
*/
QRectF QPlatformInputContext::inputItemClipRectangle()
{
    return QHighDpi::toNativePixels(
        QGuiApplication::inputMethod()->inputItemClipRectangle(), QGuiApplication::focusWindow());
}

/*!
   \brief QPlatformInputContext::cursorRectangle

    Returns the cursor rectangle for the currently active window
    and input methiod in native window coordinates.
*/
QRectF QPlatformInputContext::cursorRectangle()
{
    return QHighDpi::toNativePixels(
        QGuiApplication::inputMethod()->cursorRectangle(), QGuiApplication::focusWindow());
}

/*!
   \brief QPlatformInputContext::anchorRectangle

    Returns the anchor rectangle for the currently active window
    and input methiod in native window coordinates.
*/
QRectF QPlatformInputContext::anchorRectangle()
{
    return QHighDpi::toNativePixels(
        QGuiApplication::inputMethod()->anchorRectangle(), QGuiApplication::focusWindow());
}

/*!
   \brief QPlatformInputContext::keyboardRectangle

    Returns the keyboard rectangle for the currently active window
    and input methiod in native window coordinates.
*/
QRectF QPlatformInputContext::keyboardRectangle()
{
    return QHighDpi::toNativePixels(
        QGuiApplication::inputMethod()->keyboardRectangle(), QGuiApplication::focusWindow());
}

QT_END_NAMESPACE

#include "moc_qplatforminputcontext.cpp"
