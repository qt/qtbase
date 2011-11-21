/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <qinputpanel.h>
#include <private/qinputpanel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal
*/
QInputPanel::QInputPanel()
    : QObject(*new QInputPanelPrivate)
{
}

/*!
    \internal
*/
QInputPanel::~QInputPanel()
{
}

/*!
    \class QInputPanel
    \brief The QInputPanel class provides access to the active text input method.

    QInputPanel is used by the text editors for integrating to the platform text input
    methods and more commonly by application views for querying various text input method-related
    information like virtual keyboard visibility and keyboard dimensions.

    Qt Quick also provides access to QInputPanel in QML through \l{QmlGlobalQtObject}{Qt global object}
    as \c Qt.application.inputPanel property.
*/

/*!
    \property QInputPanel::inputItem
    \brief Focused item that accepts text input

    Input item is set and unset by the focused window. In QML Scene Graph this is done by
    QQuickCanvas and the input item is either TextInput or TextEdit element. Any QObject can
    behave as an input item as long as it responds to QInputMethodQueryEvent and QInputMethodEvent
    events sent by the input methods.

    \sa inputItemTransform, inputWindow, QInputMethodQueryEvent, QInputMethodEvent
*/
QObject *QInputPanel::inputItem() const
{
    Q_D(const QInputPanel);
    return d->inputItem.data();
}

void QInputPanel::setInputItem(QObject *inputItem)
{
    Q_D(QInputPanel);

    if (d->inputItem.data() == inputItem)
        return;

    d->inputItem = inputItem;
    emit inputItemChanged();
}

/*!
    Returns the currently focused window containing the input item.
*/
QWindow *QInputPanel::inputWindow() const
{
    return qApp->activeWindow();
}

/*!
    Returns the transformation from input item coordinates to the window coordinates.
*/
QTransform QInputPanel::inputItemTransform() const
{
    Q_D(const QInputPanel);
    return d->inputItemTransform;
}

/*!
    Sets the transformation from input item coordinates to the window coordinates.
    Item transform needs to be updated by the focused window like QQuickCanvas whenever
    item is moved inside the scene.
*/
void QInputPanel::setInputItemTransform(const QTransform &transform)
{
    Q_D(QInputPanel);
    if (d->inputItemTransform == transform)
        return;

    d->inputItemTransform = transform;
    emit cursorRectangleChanged();
}

/*!
    \property QInputPanel::cursorRectangle
    \brief Input item's cursor rectangle in window coordinates.

    Cursor rectangle is often used by various text editing controls
    like text prediction popups for following the text being typed.
*/
QRectF QInputPanel::cursorRectangle() const
{
    Q_D(const QInputPanel);

    if (!d->inputItem)
        return QRectF();

    QInputMethodQueryEvent query(Qt::ImCursorRectangle);
    QGuiApplication::sendEvent(d->inputItem.data(), &query);
    QRect r = query.value(Qt::ImCursorRectangle).toRect();
    if (!r.isValid())
        return QRect();

    return d->inputItemTransform.mapRect(r);
}

/*!
    \property QInputPanel::keyboardRectangle
    \brief Virtual keyboard's geometry in window coordinates.
*/
QRectF QInputPanel::keyboardRectangle()
{
    Q_D(QInputPanel);
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
void QInputPanel::show()
{
    Q_D(QInputPanel);
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
void QInputPanel::hide()
{
    Q_D(QInputPanel);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->hideInputPanel();
}

/*!
    \property QInputPanel::visible
    \brief Virtual keyboard's visibility on the screen

    Input panel visibility remains false for devices
    with no virtual keyboards.

    \sa show(), hide()
*/
bool QInputPanel::visible() const
{
    Q_D(const QInputPanel);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        return ic->isInputPanelVisible();
    return false;
}

/*!
    Controls the keyboard visibility. Equivalent
    to calling show() and hide() functions.

    \sa show(), hide()
*/
void QInputPanel::setVisible(bool visible)
{
    visible ? show() : hide();
}

/*!
    \property QInputPanel::animating
    \brief True when the virtual keyboard is being opened or closed.

    Animating is false when keyboard is fully open or closed.
    When \c animating is \c true and \c visibility is \c true keyboard
    is being opened. When \c animating is \c true and \c visibility is
    false keyboard is being closed.
*/

bool QInputPanel::isAnimating() const
{
    Q_D(const QInputPanel);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        return ic->isAnimating();
    return false;
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
void QInputPanel::update(Qt::InputMethodQueries queries)
{
    Q_D(QInputPanel);

    if (!d->inputItem)
        return;

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
void QInputPanel::reset()
{
    Q_D(QInputPanel);
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
void QInputPanel::commit()
{
    Q_D(QInputPanel);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->commit();
}

/*!
    Called by the input item when the word currently being composed is tapped by
    the user. Input methods often use this information to offer more word
    suggestions to the user.
*/
void QInputPanel::invokeAction(Action a, int cursorPosition)
{
    Q_D(QInputPanel);
    QPlatformInputContext *ic = d->platformInputContext();
    if (ic)
        ic->invokeAction(a, cursorPosition);
}

QT_END_NAMESPACE

#include "moc_qinputpanel.cpp"
