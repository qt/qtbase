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

#include <qstylehints.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static inline QVariant hint(QPlatformIntegration::StyleHint h)
{
    return QGuiApplicationPrivate::platformIntegration()->styleHint(h);
}

static inline QVariant themeableHint(QPlatformTheme::ThemeHint th,
                                     QPlatformIntegration::StyleHint ih)
{
    if (!QCoreApplication::instance()) {
        qWarning("Must construct a QGuiApplication before accessing a platform theme hint.");
        return QVariant();
    }
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(th);
        if (themeHint.isValid())
            return themeHint;
    }
    return QGuiApplicationPrivate::platformIntegration()->styleHint(ih);
}

class QStyleHintsPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QStyleHints)
public:
    inline QStyleHintsPrivate()
        : m_mouseDoubleClickInterval(-1)
        , m_mousePressAndHoldInterval(-1)
        , m_startDragDistance(-1)
        , m_startDragTime(-1)
        , m_keyboardInputInterval(-1)
        , m_cursorFlashTime(-1)
        , m_tabFocusBehavior(-1)
        , m_uiEffects(-1)
        , m_wheelScrollLines(-1)
        , m_mouseQuickSelectionThreshold(-1)
        {}

    int m_mouseDoubleClickInterval;
    int m_mousePressAndHoldInterval;
    int m_startDragDistance;
    int m_startDragTime;
    int m_keyboardInputInterval;
    int m_cursorFlashTime;
    int m_tabFocusBehavior;
    int m_uiEffects;
    int m_wheelScrollLines;
    int m_mouseQuickSelectionThreshold;
};

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

    \sa QGuiApplication::styleHints()
 */
QStyleHints::QStyleHints()
    : QObject(*new QStyleHintsPrivate(), 0)
{
}

/*!
    Sets the \a mouseDoubleClickInterval.
    \internal
    \sa mouseDoubleClickInterval()
    \since 5.3
*/
void  QStyleHints::setMouseDoubleClickInterval(int mouseDoubleClickInterval)
{
    Q_D(QStyleHints);
    if (d->m_mouseDoubleClickInterval == mouseDoubleClickInterval)
        return;
    d->m_mouseDoubleClickInterval = mouseDoubleClickInterval;
    emit mouseDoubleClickIntervalChanged(mouseDoubleClickInterval);
}

/*!
    \property QStyleHints::mouseDoubleClickInterval
    \brief the time limit in milliseconds that distinguishes a double click
    from two consecutive mouse clicks.
*/
int QStyleHints::mouseDoubleClickInterval() const
{
    Q_D(const QStyleHints);
    return d->m_mouseDoubleClickInterval >= 0 ?
           d->m_mouseDoubleClickInterval :
           themeableHint(QPlatformTheme::MouseDoubleClickInterval, QPlatformIntegration::MouseDoubleClickInterval).toInt();
}

/*!
    Sets the \a mousePressAndHoldInterval.
    \internal
    \sa mousePressAndHoldInterval()
    \since 5.7
*/
void QStyleHints::setMousePressAndHoldInterval(int mousePressAndHoldInterval)
{
    Q_D(QStyleHints);
    if (d->m_mousePressAndHoldInterval == mousePressAndHoldInterval)
        return;
    d->m_mousePressAndHoldInterval = mousePressAndHoldInterval;
    emit mousePressAndHoldIntervalChanged(mousePressAndHoldInterval);
}

/*!
    \property QStyleHints::mousePressAndHoldInterval
    \brief the time limit in milliseconds that activates
    a press and hold.

    \since 5.3
*/
int QStyleHints::mousePressAndHoldInterval() const
{
    Q_D(const QStyleHints);
    return d->m_mousePressAndHoldInterval >= 0 ?
           d->m_mousePressAndHoldInterval :
           themeableHint(QPlatformTheme::MousePressAndHoldInterval, QPlatformIntegration::MousePressAndHoldInterval).toInt();
}

/*!
    Sets the \a startDragDistance.
    \internal
    \sa startDragDistance()
    \since 5.3
*/
void QStyleHints::setStartDragDistance(int startDragDistance)
{
    Q_D(QStyleHints);
    if (d->m_startDragDistance == startDragDistance)
        return;
    d->m_startDragDistance = startDragDistance;
    emit startDragDistanceChanged(startDragDistance);
}

/*!
    \property QStyleHints::startDragDistance
    \brief the distance, in pixels, that the mouse must be moved with a button
    held down before a drag and drop operation will begin.

    If you support drag and drop in your application, and want to start a drag
    and drop operation after the user has moved the cursor a certain distance
    with a button held down, you should use this property's value as the
    minimum distance required.

    For example, if the mouse position of the click is stored in \c startPos
    and the current position (e.g. in the mouse move event) is \c currentPos,
    you can find out if a drag should be started with code like this:

    \snippet code/src_gui_kernel_qapplication.cpp 6

    \sa startDragTime, QPoint::manhattanLength(), {Drag and Drop}
*/
int QStyleHints::startDragDistance() const
{
    Q_D(const QStyleHints);
    return d->m_startDragDistance >= 0 ?
           d->m_startDragDistance :
           themeableHint(QPlatformTheme::StartDragDistance, QPlatformIntegration::StartDragDistance).toInt();
}

/*!
    Sets the \a startDragDragTime.
    \internal
    \sa startDragTime()
    \since 5.3
*/
void QStyleHints::setStartDragTime(int startDragTime)
{
    Q_D(QStyleHints);
    if (d->m_startDragTime == startDragTime)
        return;
    d->m_startDragTime = startDragTime;
    emit startDragTimeChanged(startDragTime);
}

/*!
    \property QStyleHints::startDragTime
    \brief the time, in milliseconds, that a mouse button must be held down
    before a drag and drop operation will begin.

    If you support drag and drop in your application, and want to start a drag
    and drop operation after the user has held down a mouse button for a
    certain amount of time, you should use this property's value as the delay.

    \sa startDragDistance, {Drag and Drop}
*/
int QStyleHints::startDragTime() const
{
    Q_D(const QStyleHints);
    return d->m_startDragTime >= 0 ?
           d->m_startDragTime :
           themeableHint(QPlatformTheme::StartDragTime, QPlatformIntegration::StartDragTime).toInt();
}

/*!
    \property QStyleHints::startDragVelocity
    \brief the limit for the velocity, in pixels per second, that the mouse may
    be moved, with a button held down, for a drag and drop operation to begin.
    A value of 0 means there is no such limit.

    \sa startDragDistance, {Drag and Drop}
*/
int QStyleHints::startDragVelocity() const
{
    return themeableHint(QPlatformTheme::StartDragVelocity, QPlatformIntegration::StartDragVelocity).toInt();
}

/*!
    Sets the \a keyboardInputInterval.
    \internal
    \sa keyboardInputInterval()
    \since 5.3
*/
void QStyleHints::setKeyboardInputInterval(int keyboardInputInterval)
{
    Q_D(QStyleHints);
    if (d->m_keyboardInputInterval == keyboardInputInterval)
        return;
    d->m_keyboardInputInterval = keyboardInputInterval;
    emit keyboardInputIntervalChanged(keyboardInputInterval);
}

/*!
    \property QStyleHints::keyboardInputInterval
    \brief the time limit, in milliseconds, that distinguishes a key press
    from two consecutive key presses.
*/
int QStyleHints::keyboardInputInterval() const
{
    Q_D(const QStyleHints);
    return d->m_keyboardInputInterval >= 0 ?
           d->m_keyboardInputInterval :
           themeableHint(QPlatformTheme::KeyboardInputInterval, QPlatformIntegration::KeyboardInputInterval).toInt();
}

/*!
    \property QStyleHints::keyboardAutoRepeatRate
    \brief the rate, in events per second,  in which additional repeated key
    presses will automatically be generated if a key is being held down.
*/
int QStyleHints::keyboardAutoRepeatRate() const
{
    return themeableHint(QPlatformTheme::KeyboardAutoRepeatRate, QPlatformIntegration::KeyboardAutoRepeatRate).toInt();
}

/*!
    Sets the \a cursorFlashTime.
    \internal
    \sa cursorFlashTime()
    \since 5.3
*/
void QStyleHints::setCursorFlashTime(int cursorFlashTime)
{
    Q_D(QStyleHints);
    if (d->m_cursorFlashTime == cursorFlashTime)
        return;
    d->m_cursorFlashTime = cursorFlashTime;
    emit cursorFlashTimeChanged(cursorFlashTime);
}

/*!
    \property QStyleHints::cursorFlashTime
    \brief the text cursor's flash (blink) time in milliseconds.

    The flash time is the time used to display, invert and restore the
    caret display. Usually the text cursor is displayed for half the cursor
    flash time, then hidden for the same amount of time.
*/
int QStyleHints::cursorFlashTime() const
{
    Q_D(const QStyleHints);
    return d->m_cursorFlashTime >= 0 ?
           d->m_cursorFlashTime :
           themeableHint(QPlatformTheme::CursorFlashTime, QPlatformIntegration::CursorFlashTime).toInt();
}

/*!
    \property QStyleHints::showIsFullScreen
    \brief whether the platform defaults to fullscreen windows.

    This property is \c true if the platform defaults to windows being fullscreen,
    otherwise \c false.

    \note The platform may still choose to show certain windows non-fullscreen,
    such as popups or dialogs. This property only reports the default behavior.

    \sa QWindow::show(), showIsMaximized()
*/
bool QStyleHints::showIsFullScreen() const
{
    return hint(QPlatformIntegration::ShowIsFullScreen).toBool();
}

/*!
    \property QStyleHints::showIsMaximized
    \brief whether the platform defaults to maximized windows.

    This property is \c true if the platform defaults to windows being maximized,
    otherwise \c false.

    \note The platform may still choose to show certain windows non-maximized,
    such as popups or dialogs. This property only reports the default behavior.

    \sa QWindow::show(), showIsFullScreen()
    \since 5.6
*/
bool QStyleHints::showIsMaximized() const
{
    return hint(QPlatformIntegration::ShowIsMaximized).toBool();
}

/*!
    \property QStyleHints::showShortcutsInContextMenus
    \since 5.10
    \brief \c true if the platform normally shows shortcut key sequences in
    context menus, otherwise \c false.
*/
bool QStyleHints::showShortcutsInContextMenus() const
{
    return themeableHint(QPlatformTheme::ShowShortcutsInContextMenus, QPlatformIntegration::ShowShortcutsInContextMenus).toBool();
}

/*!
    \property QStyleHints::passwordMaskDelay
    \brief the time, in milliseconds, a typed letter is displayed unshrouded
    in a text input field in password mode.
*/
int QStyleHints::passwordMaskDelay() const
{
    return themeableHint(QPlatformTheme::PasswordMaskDelay, QPlatformIntegration::PasswordMaskDelay).toInt();
}

/*!
    \property QStyleHints::passwordMaskCharacter
    \brief the character used to mask the characters typed into text input
    fields in password mode.
*/
QChar QStyleHints::passwordMaskCharacter() const
{
    return themeableHint(QPlatformTheme::PasswordMaskCharacter, QPlatformIntegration::PasswordMaskCharacter).toChar();
}

/*!
    \property QStyleHints::fontSmoothingGamma
    \brief the gamma value used in font smoothing.
*/
qreal QStyleHints::fontSmoothingGamma() const
{
    return hint(QPlatformIntegration::FontSmoothingGamma).toReal();
}

/*!
    \property QStyleHints::useRtlExtensions
    \brief the writing direction.

    This property is \c true if right-to-left writing direction is enabled,
    otherwise \c false.
*/
bool QStyleHints::useRtlExtensions() const
{
    return hint(QPlatformIntegration::UseRtlExtensions).toBool();
}

/*!
    \property QStyleHints::setFocusOnTouchRelease
    \brief the event that should set input focus on focus objects.

    This property is \c true if focus objects (line edits etc) should receive
    input focus after a touch/mouse release. This is normal behavior on
    touch platforms. On desktop platforms, the standard is to set
    focus already on touch/mouse press.
*/
bool QStyleHints::setFocusOnTouchRelease() const
{
    return hint(QPlatformIntegration::SetFocusOnTouchRelease).toBool();
}

/*!
    \property QStyleHints::tabFocusBehavior
    \since 5.5
    \brief The focus behavior on press of the tab key.

    \note Do not bind this value in QML because the change notifier
    signal is not implemented yet.
*/

Qt::TabFocusBehavior QStyleHints::tabFocusBehavior() const
{
    Q_D(const QStyleHints);
    return Qt::TabFocusBehavior(d->m_tabFocusBehavior >= 0 ?
                                d->m_tabFocusBehavior :
                                themeableHint(QPlatformTheme::TabFocusBehavior, QPlatformIntegration::TabFocusBehavior).toInt());
}

/*!
    Sets the \a tabFocusBehavior.
    \internal
    \sa tabFocusBehavior()
    \since 5.7
*/
void QStyleHints::setTabFocusBehavior(Qt::TabFocusBehavior tabFocusBehavior)
{
    Q_D(QStyleHints);
    if (d->m_tabFocusBehavior == tabFocusBehavior)
        return;
    d->m_tabFocusBehavior = tabFocusBehavior;
    emit tabFocusBehaviorChanged(tabFocusBehavior);
}

/*!
    \property QStyleHints::singleClickActivation
    \brief whether items are activated by single or double click.

    This property is \c true if items should be activated by single click, \c false
    if they should be activated by double click instead.

    \since 5.5
*/
bool QStyleHints::singleClickActivation() const
{
    return themeableHint(QPlatformTheme::ItemViewActivateItemOnSingleClick, QPlatformIntegration::ItemViewActivateItemOnSingleClick).toBool();
}

/*!
    \property QStyleHints::useHoverEffects
    \brief whether UI elements use hover effects.

    This property is \c true if UI elements should use hover effects. This is the
    standard behavior on desktop platforms with a mouse pointer, whereas
    on touch platforms the overhead of hover event delivery can be avoided.

    \since 5.8
*/
bool QStyleHints::useHoverEffects() const
{
    Q_D(const QStyleHints);
    return (d->m_uiEffects >= 0 ?
            d->m_uiEffects :
            themeableHint(QPlatformTheme::UiEffects, QPlatformIntegration::UiEffects).toInt()) & QPlatformTheme::HoverEffect;
}

void QStyleHints::setUseHoverEffects(bool useHoverEffects)
{
    Q_D(QStyleHints);
    if (d->m_uiEffects >= 0 && useHoverEffects == bool(d->m_uiEffects & QPlatformTheme::HoverEffect))
        return;
    if (d->m_uiEffects == -1)
        d->m_uiEffects = 0;
    if (useHoverEffects)
        d->m_uiEffects |= QPlatformTheme::HoverEffect;
    else
        d->m_uiEffects &= ~QPlatformTheme::HoverEffect;
    emit useHoverEffectsChanged(useHoverEffects);
}

/*!
    \property QStyleHints::wheelScrollLines
    \brief Number of lines to scroll by default for each wheel click.

    \since 5.9
*/
int QStyleHints::wheelScrollLines() const
{
    Q_D(const QStyleHints);
    if (d->m_wheelScrollLines > 0)
        return d->m_wheelScrollLines;
    return themeableHint(QPlatformTheme::WheelScrollLines, QPlatformIntegration::WheelScrollLines).toInt();
}

/*!
    Sets the \a wheelScrollLines.
    \internal
    \sa wheelScrollLines()
    \since 5.9
*/
void QStyleHints::setWheelScrollLines(int scrollLines)
{
    Q_D(QStyleHints);
    if (d->m_wheelScrollLines == scrollLines)
        return;
    d->m_wheelScrollLines = scrollLines;
    emit wheelScrollLinesChanged(scrollLines);
}

/*!
    Sets the mouse quick selection threshold.
    \internal
    \sa mouseQuickSelectionThreshold()
    \since 5.11
*/
void QStyleHints::setMouseQuickSelectionThreshold(int threshold)
{
    Q_D(QStyleHints);
    if (d->m_mouseQuickSelectionThreshold == threshold)
        return;
    d->m_mouseQuickSelectionThreshold = threshold;
    emit mouseQuickSelectionThresholdChanged(threshold);
}

/*!
    \property QStyleHints::mouseQuickSelectionThreshold
    \brief Quick selection mouse threshold in QLineEdit.

    This property defines how much the mouse cursor should be moved along the y axis
    to trigger a quick selection during a normal QLineEdit text selection.

    If the property value is less than or equal to 0, the quick selection feature is disabled.

    \since 5.11
*/
int QStyleHints::mouseQuickSelectionThreshold() const
{
    Q_D(const QStyleHints);
    if (d->m_mouseQuickSelectionThreshold >= 0)
        return d->m_mouseQuickSelectionThreshold;
    return themeableHint(QPlatformTheme::MouseQuickSelectionThreshold, QPlatformIntegration::MouseQuickSelectionThreshold).toInt();
}

QT_END_NAMESPACE
