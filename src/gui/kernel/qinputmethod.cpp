/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qinputmethod.h>
#include <private/qinputmethod_p.h>
#include <qguiapplication.h>
#include <qtimer.h>
#include <qpa/qplatforminputcontext_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal
*/
QInputMethod::QInputMethod()
    : QObject(*new QInputMethodPrivate)
{
    // might be instantiated before QGuiApplication is fully done, need to connect later
    QTimer::singleShot(0, this, SLOT(_q_connectFocusObject()));
}

/*!
    \internal
*/
QInputMethod::~QInputMethod()
{
}

/*!
    \class QInputMethod
    \brief The QInputMethod class provides access to the active text input method.
    \inmodule QtGui

    QInputMethod is used by the text editors for integrating to the platform text input
    methods and more commonly by application views for querying various text input method-related
    information like virtual keyboard visibility and keyboard dimensions.

    Qt Quick also provides access to QInputMethod in QML through \l{QmlGlobalQtObject}{Qt global object}
    as \c Qt.inputMethod property.
*/

/*!
    \property QInputMethod::inputItem
    \brief Focused item that accepts text input
    \obsolete

    Input item is set and unset by the focused window. In QML Scene Graph this is done by
    QQuickCanvas and the input item is either TextInput or TextEdit element. Any QObject can
    behave as an input item as long as it responds to QInputMethodQueryEvent and QInputMethodEvent
    events sent by the input methods.

    \sa inputItemTransform, inputWindow, QInputMethodQueryEvent, QInputMethodEvent
*/
QObject *QInputMethod::inputItem() const
{
    Q_D(const QInputMethod);
    return d->inputItem.data();
}

void QInputMethod::setInputItem(QObject *inputItem)
{
    Q_D(QInputMethod);
    d->setInputItem(inputItem);
}

/*!
    Returns the currently focused window containing the input item.

    \obsolete
*/
QWindow *QInputMethod::inputWindow() const
{
    return qApp->focusWindow();
}

/*!
    Returns the transformation from input item coordinates to the window coordinates.
*/
QTransform QInputMethod::inputItemTransform() const
{
    Q_D(const QInputMethod);
    return d->inputItemTransform;
}

/*!
    Sets the transformation from input item coordinates to window coordinates to be \a transform.
    Item transform needs to be updated by the focused window like QQuickCanvas whenever
    item is moved inside the scene.
*/
void QInputMethod::setInputItemTransform(const QTransform &transform)
{
    Q_D(QInputMethod);
    if (d->inputItemTransform == transform)
        return;

    d->inputItemTransform = transform;
    emit cursorRectangleChanged();
}

/*!
    \property QInputMethod::cursorRectangle
    \brief Input item's cursor rectangle in window coordinates.

    Cursor rectangle is often used by various text editing controls
    like text prediction popups for following the text being typed.
*/
QRectF QInputMethod::cursorRectangle() const
{
    Q_D(const QInputMethod);

    if (!d->inputItem)
        return QRectF();

    QInputMethodQueryEvent query(Qt::ImCursorRectangle);
    QGuiApplication::sendEvent(d->inputItem.data(), &query);
    QRectF r = query.value(Qt::ImCursorRectangle).toRectF();
    if (!r.isValid())
        return QRectF();

    return d->inputItemTransform.mapRect(r);
}

/*!
    \property QInputMethod::keyboardRectangle
    \brief Virtual keyboard's geometry in window coordinates.
*/
QRectF QInputMethod::keyboardRectangle() const
{
    Q_D(const QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        return ic->keyboardRect();
    return QRectF();
}

/*!
    Requests virtual keyboard to open. If the platform
    doesn't provide virtual keyboard the visibility
    remains false.

    Normally applications should not need to call this
    function, keyboard should automatically open when
    the text editor gains focus.
*/
void QInputMethod::show()
{
    Q_D(QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->showInputPanel();
}

/*!
    Requests virtual keyboard to close.

    Normally applications should not need to call this function,
    keyboard should automatically close when the text editor loses
    focus, for example when the parent view is closed.
*/
void QInputMethod::hide()
{
    Q_D(QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->hideInputPanel();
}

/*!
    \fn bool QInputMethod::visible() const
    \obsolete

    Use isVisible() instead.
*/

/*!
    \property QInputMethod::visible
    \brief Virtual keyboard's visibility on the screen

    Input method visibility remains false for devices
    with no virtual keyboards.

    \sa show(), hide()
*/
bool QInputMethod::isVisible() const
{
    Q_D(const QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        return ic->isInputPanelVisible();
    return false;
}

/*!
    Controls the keyboard visibility. Equivalent
    to calling show() (if \a visible is \c true)
    or hide() (if \a visible is \c false).

    \sa show(), hide()
*/
void QInputMethod::setVisible(bool visible)
{
    visible ? show() : hide();
}

/*!
    \property QInputMethod::animating
    \brief True when the virtual keyboard is being opened or closed.

    Animating is false when keyboard is fully open or closed.
    When \c animating is \c true and \c visibility is \c true keyboard
    is being opened. When \c animating is \c true and \c visibility is
    false keyboard is being closed.
*/

bool QInputMethod::isAnimating() const
{
    Q_D(const QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        return ic->isAnimating();
    return false;
}

/*!
    \property QInputMethod::locale
    \brief Current input locale.
*/
QLocale QInputMethod::locale() const
{
    Q_D(const QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        return ic->locale();
    return QLocale::c();
}

/*!
    \property QInputMethod::inputDirection
    \brief Current input direction.
*/
Qt::LayoutDirection QInputMethod::inputDirection() const
{
    Q_D(const QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        return ic->inputDirection();
    return Qt::LeftToRight;
}

/*!
    Called by the input item to inform the platform input methods when there has been
    state changes in editor's input method query attributes. When calling the function
    \a queries parameter has to be used to tell what has changes, which input method
    can use to make queries for attributes it's interested with QInputMethodQueryEvent.

    In particular calling update whenever the cursor position changes is important as
    that often causes other query attributes like surrounding text and text selection
    to change as well. The attributes that often change together with cursor position
    have been grouped in Qt::ImQueryInput value for convenience.
*/
void QInputMethod::update(Qt::InputMethodQueries queries)
{
    Q_D(QInputMethod);

    if (queries & Qt::ImEnabled) {
        QObject *focus = qApp->focusObject();
        bool enabled = d->objectAcceptsInputMethod(focus);
        d->setInputItem(enabled ? focus : 0);
        QPlatformInputContextPrivate::setInputMethodAccepted(enabled);
    }

    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->update(queries);

    if (queries & Qt::ImCursorRectangle)
        emit cursorRectangleChanged();
}

/*!
    Resets the input method state. For example, a text editor normally calls
    this method before inserting a text to make widget ready to accept a text.

    Input method resets automatically when the focused editor changes.
*/
void QInputMethod::reset()
{
    Q_D(QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->reset();
}

/*!
    Commits the word user is currently composing to the editor. The function is
    mostly needed by the input methods with text prediction features and by the
    methods where the script used for typing characters is different from the
    script that actually gets appended to the editor. Any kind of action that
    interrupts the text composing needs to flush the composing state by calling the
    commit() function, for example when the cursor is moved elsewhere.
*/
void QInputMethod::commit()
{
    Q_D(QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->commit();
}

/*!
    \enum QInputMethod::Action

    Indicates the kind of action performed by the user.

    \value Click        A normal click/tap
    \value ContextMenu  A context menu click/tap (e.g. right-button or tap-and-hold)

    \sa invokeAction()
*/

/*!
    Called by the input item when the word currently being composed is tapped by
    the user, as indicated by the action \a a and the given \a cursorPosition.
    Input methods often use this information to offer more word suggestions to the user.
*/
void QInputMethod::invokeAction(Action a, int cursorPosition)
{
    Q_D(QInputMethod);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->invokeAction(a, cursorPosition);
}

// temporary handlers for updating focus item based on application focus
void QInputMethodPrivate::_q_connectFocusObject()
{
    Q_Q(QInputMethod);
    QObject::connect(qApp, SIGNAL(focusObjectChanged(QObject*)),
                     q, SLOT(_q_checkFocusObject(QObject*)));
    _q_checkFocusObject(qApp->focusObject());
}

void QInputMethodPrivate::_q_checkFocusObject(QObject *object)
{
    bool enabled = objectAcceptsInputMethod(object);
    setInputItem(enabled ? object : 0);
}

bool QInputMethodPrivate::objectAcceptsInputMethod(QObject *object)
{
    bool enabled = false;
    if (object) {
        QInputMethodQueryEvent query(Qt::ImEnabled);
        QGuiApplication::sendEvent(object, &query);
        enabled = query.value(Qt::ImEnabled).toBool();
    }

    return enabled;
}

QT_END_NAMESPACE

#include "moc_qinputmethod.cpp"
