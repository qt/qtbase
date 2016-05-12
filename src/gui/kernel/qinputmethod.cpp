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

#include <qinputmethod.h>
#include <private/qinputmethod_p.h>
#include <qguiapplication.h>
#include <qtimer.h>
#include <qpa/qplatforminputcontext_p.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \internal
*/
QInputMethod::QInputMethod()
    : QObject(*new QInputMethodPrivate)
{
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
    emit anchorRectangleChanged();
}


/*!
    \since 5.1

    Returns the input item's geometry in input item coordinates.

    \sa setInputItemRectangle()
*/
QRectF QInputMethod::inputItemRectangle() const
{
    Q_D(const QInputMethod);
    return d->inputRectangle;
}

/*!
    \since 5.1

    Sets the input item's geometry to be \a rect, in input item coordinates.
    This needs to be updated by the focused window like QQuickCanvas whenever
    item is moved inside the scene, or focus is changed.
*/
void QInputMethod::setInputItemRectangle(const QRectF &rect)
{
    Q_D(QInputMethod);
    d->inputRectangle = rect;
}

static QRectF inputMethodQueryRectangle_helper(Qt::InputMethodQuery imquery, const QTransform &xform)
{
    QRectF r;
    if (QObject *focusObject = qGuiApp->focusObject()) {
        QInputMethodQueryEvent query(imquery);
        QGuiApplication::sendEvent(focusObject, &query);
        r = query.value(imquery).toRectF();
        if (r.isValid())
            r = xform.mapRect(r);
    }
    return r;
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
    return inputMethodQueryRectangle_helper(Qt::ImCursorRectangle, d->inputItemTransform);
}

/*!
    \property QInputMethod::anchorRectangle
    \brief Input item's anchor rectangle in window coordinates.

    Anchor rectangle is often used by various text editing controls
    like text prediction popups for following the text selection.
*/
QRectF QInputMethod::anchorRectangle() const
{
    Q_D(const QInputMethod);
    return inputMethodQueryRectangle_helper(Qt::ImAnchorRectangle, d->inputItemTransform);
}

/*!
    \property QInputMethod::keyboardRectangle
    \brief Virtual keyboard's geometry in window coordinates.

    This might be an empty rectangle if it is not possible to know the geometry
    of the keyboard. This is the case for a floating keyboard on android.
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
    \property QInputMethod::inputItemClipRectangle
    \brief Input item's clipped rectangle in window coordinates.

    The clipped input rectangle is often used by various input methods to determine
    how much screen real estate is available for the input method (e.g. Virtual Keyboard).
*/
QRectF QInputMethod::inputItemClipRectangle() const
{
    Q_D(const QInputMethod);
    return inputMethodQueryRectangle_helper(Qt::ImInputItemClipRectangle, d->inputItemTransform);
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
        QPlatformInputContextPrivate::setInputMethodAccepted(enabled);
    }

    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->update(queries);

    if (queries & Qt::ImCursorRectangle)
        emit cursorRectangleChanged();

    if (queries & (Qt::ImAnchorRectangle))
        emit anchorRectangleChanged();

    if (queries & (Qt::ImInputItemClipRectangle))
        emit inputItemClipRectangleChanged();
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

/*!
  Send \a query to the current focus object with parameters \a argument and return the result.
 */
QVariant QInputMethod::queryFocusObject(Qt::InputMethodQuery query, QVariant argument)
{
    QVariant retval;
    QObject *focusObject = qGuiApp->focusObject();
    if (!focusObject)
        return retval;

    bool newMethodWorks = QMetaObject::invokeMethod(focusObject, "inputMethodQuery",
                                                    Qt::DirectConnection,
                                                    Q_RETURN_ARG(QVariant, retval),
                                                    Q_ARG(Qt::InputMethodQuery, query),
                                                    Q_ARG(QVariant, argument));
    if (newMethodWorks)
        return retval;

    QInputMethodQueryEvent queryEvent(query);
    QCoreApplication::sendEvent(focusObject, &queryEvent);
    return queryEvent.value(query);
}

QT_END_NAMESPACE

#include "moc_qinputmethod.cpp"
