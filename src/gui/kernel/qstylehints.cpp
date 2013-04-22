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

#include <qstylehints.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

static inline QVariant hint(QPlatformIntegration::StyleHint h)
{
    return QGuiApplicationPrivate::platformIntegration()->styleHint(h);
}

static inline QVariant themeableHint(QPlatformTheme::ThemeHint th,
                                     QPlatformIntegration::StyleHint ih)
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(th);
        if (themeHint.isValid())
            return themeHint;
    }
    return QGuiApplicationPrivate::platformIntegration()->styleHint(ih);
}

/*!
    \class QStyleHints
    \since 5.0
    \brief The QStyleHints class contains platform specific hints and settings.
    \inmodule QtGui

    An object of this class, obtained from QGuiApplication, provides access to certain global
    user interface parameters of the current platform.

    Access is read only; typically the platform itself provides the user a way to tune these
    parameters.

    Access to these parameters are useful when implementing custom user interface components, in that
    they allow the components to exhibit the same behaviour and feel as other components.

    \sa QGuiApplication::styleHints(), QPlatformTheme
 */
QStyleHints::QStyleHints()
    : QObject()
{
}

/*!
    Returns the time limit in milliseconds that distinguishes a double click
    from two consecutive mouse clicks.
*/
int QStyleHints::mouseDoubleClickInterval() const
{
    return themeableHint(QPlatformTheme::MouseDoubleClickInterval, QPlatformIntegration::MouseDoubleClickInterval).toInt();
}

/*!
    Returns the distance, in pixels, that the mouse must be moved with a button
    held down before a drag and drop operation will begin.

    If you support drag and drop in your application, and want to start a drag
    and drop operation after the user has moved the cursor a certain distance
    with a button held down, you should use this property's value as the
    minimum distance required.

    For example, if the mouse position of the click is stored in \c startPos
    and the current position (e.g. in the mouse move event) is \c currentPos,
    you can find out if a drag should be started with code like this:

    \snippet code/src_gui_kernel_qapplication.cpp 7

    \sa startDragTime(), QPoint::manhattanLength(), {Drag and Drop}
*/
int QStyleHints::startDragDistance() const
{
    return themeableHint(QPlatformTheme::StartDragDistance, QPlatformIntegration::StartDragDistance).toInt();
}

/*!
    Returns the time, in milliseconds, that a mouse button must be held down
    before a drag and drop operation will begin.

    If you support drag and drop in your application, and want to start a drag
    and drop operation after the user has held down a mouse button for a
    certain amount of time, you should use this property's value as the delay.

    \sa startDragDistance(), {Drag and Drop}
*/
int QStyleHints::startDragTime() const
{
    return themeableHint(QPlatformTheme::StartDragTime, QPlatformIntegration::StartDragTime).toInt();
}

/*!
    Returns the limit for the velocity, in pixels per second, that the mouse may
    be moved, with a button held down, for a drag and drop operation to begin.
    A value of 0 means there is no such limit.

    \sa startDragDistance(), {Drag and Drop}
*/
int QStyleHints::startDragVelocity() const
{
    return themeableHint(QPlatformTheme::StartDragVelocity, QPlatformIntegration::StartDragVelocity).toInt();
}

/*!
    Returns the time limit, in milliseconds, that distinguishes a key press
    from two consecutive key presses.
*/
int QStyleHints::keyboardInputInterval() const
{
    return themeableHint(QPlatformTheme::KeyboardInputInterval, QPlatformIntegration::KeyboardInputInterval).toInt();
}

/*!
    Returns the rate, in events per second,  in which additional repeated key
    presses will automatically be generated if a key is being held down.
*/
int QStyleHints::keyboardAutoRepeatRate() const
{
    return themeableHint(QPlatformTheme::KeyboardAutoRepeatRate, QPlatformIntegration::KeyboardAutoRepeatRate).toInt();
}

/*!
    Returns the text cursor's flash (blink) time in milliseconds.

    The flash time is the time used to display, invert and restore the
    caret display. Usually the text cursor is displayed for half the cursor
    flash time, then hidden for the same amount of time.
*/
int QStyleHints::cursorFlashTime() const
{
    return themeableHint(QPlatformTheme::CursorFlashTime, QPlatformIntegration::CursorFlashTime).toInt();
}

/*!
    Returns \c true if the platform defaults to windows being fullscreen,
    otherwise \c false.

    \sa QWindow::show()
*/
bool QStyleHints::showIsFullScreen() const
{
    return hint(QPlatformIntegration::ShowIsFullScreen).toBool();
}

/*!
    Returns the time, in milliseconds, a typed letter is displayed unshrouded
    in a text input field in password mode.
*/
int QStyleHints::passwordMaskDelay() const
{
    return themeableHint(QPlatformTheme::PasswordMaskDelay, QPlatformIntegration::PasswordMaskDelay).toInt();
}

/*!
    Returns the character used to mask the characters typed into text input
    fields in password mode.
*/
QChar QStyleHints::passwordMaskCharacter() const
{
    return themeableHint(QPlatformTheme::PasswordMaskCharacter, QPlatformIntegration::PasswordMaskCharacter).toChar();
}

/*!
    Returns the gamma value used in font smoothing.
*/
qreal QStyleHints::fontSmoothingGamma() const
{
    return hint(QPlatformIntegration::FontSmoothingGamma).toReal();
}

/*!
    Returns \c true if right-to-left writing direction is enabled,
    otherwise \c false.
*/
bool QStyleHints::useRtlExtensions() const
{
    return hint(QPlatformIntegration::UseRtlExtensions).toBool();
}

QT_END_NAMESPACE
