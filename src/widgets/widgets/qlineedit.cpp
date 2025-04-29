// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlineedit.h"
#include "qlineedit_p.h"

#if QT_CONFIG(action)
#  include "qaction.h"
#endif
#include "qapplication.h"
#include "qclipboard.h"
#if QT_CONFIG(draganddrop)
#include <qdrag.h>
#endif
#include "qdrawutil.h"
#include "qevent.h"
#include "qfontmetrics.h"
#include "qstylehints.h"
#if QT_CONFIG(menu)
#include "qmenu.h"
#endif
#include "qpainter.h"
#include "qpixmap.h"
#include "qpointer.h"
#include "qstringlist.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qtimer.h"
#include "qvalidator.h"
#include "qvariant.h"
#include "qdebug.h"
#if QT_CONFIG(textedit)
#include "qtextedit.h"
#include <private/qtextedit_p.h>
#endif
#include <private/qwidgettextcontrol_p.h>

#if QT_CONFIG(accessibility)
#include "qaccessible.h"
#endif
#if QT_CONFIG(itemviews)
#include "qabstractitemview.h"
#endif
#if QT_CONFIG(style_stylesheet)
#include "private/qstylesheetstyle_p.h"
#endif
#if QT_CONFIG(shortcut)
#include "private/qapplication_p.h"
#include "private/qshortcutmap_p.h"
#include "qkeysequence.h"
#define ACCEL_KEY(k) (!QCoreApplication::testAttribute(Qt::AA_DontShowShortcutsInContextMenus) \
                      && !QGuiApplicationPrivate::instance()->shortcutMap.hasShortcutForKeySequence(k) ? \
                      u'\t' + QKeySequence(k).toString(QKeySequence::NativeText) : QString())
#else
#define ACCEL_KEY(k) QString()
#endif

#include <limits.h>
#ifdef DrawText
#undef DrawText
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    Initialize \a option with the values from this QLineEdit. This method
    is useful for subclasses when they need a QStyleOptionFrame, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QLineEdit::initStyleOption(QStyleOptionFrame *option) const
{
    if (!option)
        return;

    Q_D(const QLineEdit);
    option->initFrom(this);
    option->rect = contentsRect();
    option->lineWidth = d->frame ? style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this)
                                 : 0;
    option->midLineWidth = 0;
    option->state |= QStyle::State_Sunken;
    if (d->control->isReadOnly())
        option->state |= QStyle::State_ReadOnly;
#ifdef QT_KEYPAD_NAVIGATION
    if (hasEditFocus())
        option->state |= QStyle::State_HasEditFocus;
#endif
    option->features = QStyleOptionFrame::None;
}

/*!
    \class QLineEdit
    \brief The QLineEdit widget is a one-line text editor.

    \ingroup basicwidgets
    \inmodule QtWidgets

    \image fusion-lineedit.png

    A line edit allows users to enter and edit a single line of
    plain text with useful editing functions, including undo and redo, cut and
    paste, and drag and drop.

    By changing the echoMode() of a line edit, it can also be used as
    a write-only field for inputs such as passwords.

    QTextEdit is a related class that allows multi-line, rich text
    editing.

    \section1 Constraining Text

    Use \l maxLength to define the maximum permitted length of a text. You can
    use a \l inputMask and \l setValidator() to further constrain the text
    content.

    \section1 Editing Text

    You can change the text with setText() or insert(). Use text() to retrieve
    the text and displayText() to retrieve the displayed text (which may be
    different, see \l{EchoMode}). You can select the text with setSelection() or
    selectAll(), and you can cut(), copy(), and paste() the selection. To align
    the text, use setAlignment().

    When the text changes, the textChanged() signal is emitted. When the text
    changes in some other way than by calling setText(), the textEdited() signal
    is emitted. When the cursor is moved, the cursorPositionChanged() signal is
    emitted. And when the Return or Enter key is selected, the returnPressed()
    signal is emitted.

    When text editing is finished, either because the line edit lost focus
    or Return/Enter was selected, the editingFinished() signal is emitted.

    If the line edit focus is lost without any text changes, the
    editingFinished() signal won't be emitted.

    If there is a validator set on the line edit, the
    returnPressed()/editingFinished() signals will only be emitted if the
    validator returns QValidator::Acceptable.

    For more information on the many ways that QLineEdit can be used, see
    \l {Line Edits Example}, which also provides a selection of line edit
    examples that show the effects of various properties and validators on the
    input and output supplied by the user.

    \section1 Setting a Frame

    By default, QLineEdits have a frame as specified in the platform
    style guides. You can turn the frame off by calling setFrame(false).

    \section1 Default Key Bindings

    The table below describes the default key bindings.

    \note The line edit also provides a context menu (usually invoked by a
    right-click) that presents some of the editing options listed below.

    \target desc
    \table
    \header \li Keystroke \li Action
    \row \li Left Arrow \li Moves the cursor one character to the left.
    \row \li Shift+Left Arrow \li Moves and selects text one character to the left.
    \row \li Right Arrow \li Moves the cursor one character to the right.
    \row \li Shift+Right Arrow \li Moves and selects text one character to the right.
    \row \li Home \li Moves the cursor to the beginning of the line.
    \row \li End \li Moves the cursor to the end of the line.
    \row \li Backspace \li Deletes the character to the left of the cursor.
    \row \li Ctrl+Backspace \li Deletes the word to the left of the cursor.
    \row \li Delete \li Deletes the character to the right of the cursor.
    \row \li Ctrl+Delete \li Deletes the word to the right of the cursor.
    \row \li Ctrl+A \li Selects all.
    \row \li Ctrl+C \li Copies the selected text to the clipboard.
    \row \li Ctrl+Insert \li Copies the selected text to the clipboard.
    \row \li Ctrl+K \li Deletes to the end of the line.
    \row \li Ctrl+V \li Pastes the clipboard text into line edit.
    \row \li Shift+Insert \li Pastes the clipboard text into line edit.
    \row \li Ctrl+X \li Deletes the selected text and copies it to the clipboard.
    \row \li Shift+Delete \li Deletes the selected text and copies it to the clipboard.
    \row \li Ctrl+Z \li Undoes the last operation.
    \row \li Ctrl+Y \li Redoes the last undone operation.
    \endtable

    Any other keystroke that represents a valid character, will cause the
    character to be inserted into the line edit.

    \sa QTextEdit, QLabel, QComboBox, {Line Edits Example}
*/


/*!
    \fn void QLineEdit::textChanged(const QString &text)

    This signal is emitted whenever the text changes. The \a text
    argument is the new text.

    Unlike textEdited(), this signal is also emitted when the text is
    changed programmatically, for example, by calling setText().
*/

/*!
    \fn void QLineEdit::textEdited(const QString &text)

    This signal is emitted whenever the text is edited. The \a text
    argument is the new text.

    Unlike textChanged(), this signal is not emitted when the text is
    changed programmatically, for example, by calling setText().
*/

/*!
    \fn void QLineEdit::cursorPositionChanged(int oldPos, int newPos)

    This signal is emitted whenever the cursor moves. The previous
    position is given by \a oldPos, and the new position by \a newPos.

    \sa setCursorPosition(), cursorPosition()
*/

/*!
    \fn void QLineEdit::selectionChanged()

    This signal is emitted whenever the selection changes.

    \sa hasSelectedText(), selectedText()
*/

/*!
    Constructs a line edit with no text.

    The maximum text length is set to 32767 characters.

    The \a parent argument is sent to the QWidget constructor.

    \sa setText(), setMaxLength()
*/
QLineEdit::QLineEdit(QWidget* parent)
    : QLineEdit(QString(), parent)
{
}

/*!
    Constructs a line edit containing the text \a contents as a child of
    \a parent.

    The cursor position is set to the end of the line and the maximum text
    length to 32767 characters.

    \sa text(), setMaxLength()
*/
QLineEdit::QLineEdit(const QString& contents, QWidget* parent)
    : QWidget(*new QLineEditPrivate, parent, { })
{
    Q_D(QLineEdit);
    d->init(contents);
}



/*!
    Destroys the line edit.
*/

QLineEdit::~QLineEdit()
{
}


/*!
    \property QLineEdit::text
    \brief The line edit's text.

    Setting this property clears the selection, clears the undo/redo
    history, moves the cursor to the end of the line, and resets the
    \l modified property to false. The text is not validated when
    inserted with setText().

    The text is truncated to maxLength() length.

    By default, this property contains an empty string.

    \sa insert(), clear()
*/
QString QLineEdit::text() const
{
    Q_D(const QLineEdit);
    return d->control->text();
}

void QLineEdit::setText(const QString& text)
{
    Q_D(QLineEdit);
    d->setText(text);
}

/*!
    \since 4.7

    \property QLineEdit::placeholderText
    \brief The line edit's placeholder text.

    Setting this property makes the line edit display a grayed-out
    placeholder text as long as the line edit is empty.

    Normally, an empty line edit shows the placeholder text even
    when it has focus. However, if the content is horizontally
    centered, the placeholder text is not displayed under
    the cursor when the line edit has focus.

    By default, this property contains an empty string.

    \sa text()
*/
QString QLineEdit::placeholderText() const
{
    Q_D(const QLineEdit);
    return d->placeholderText;
}

void QLineEdit::setPlaceholderText(const QString& placeholderText)
{
    Q_D(QLineEdit);
    if (d->placeholderText != placeholderText) {
        d->placeholderText = placeholderText;
        if (d->shouldShowPlaceholderText())
            update();
    }
}

/*!
    \property QLineEdit::displayText
    \brief The displayed text.

    If \l echoMode is \l Normal, this returns the same as text(). If
    \l EchoMode is \l Password or \l PasswordEchoOnEdit, it returns a string of
    platform-dependent password mask characters (e.g. "******"). If \l EchoMode
    is \l NoEcho, it returns an empty string.

    By default, this property contains an empty string.

    \sa setEchoMode(), text(), EchoMode
*/

QString QLineEdit::displayText() const
{
    Q_D(const QLineEdit);
    return d->control->displayText();
}


/*!
    \property QLineEdit::maxLength
    \brief The maximum permitted length of the text.

    If the text is too long, it is truncated at the limit.

    If truncation occurs, any selected text will be unselected, the
    cursor position is set to 0, and the first part of the string is
    shown.

    If the line edit has an input mask, the mask defines the maximum
    string length.

    By default, this property contains a value of 32767.

    \sa inputMask
*/

int QLineEdit::maxLength() const
{
    Q_D(const QLineEdit);
    return d->control->maxLength();
}

void QLineEdit::setMaxLength(int maxLength)
{
    Q_D(QLineEdit);
    d->control->setMaxLength(maxLength);
}

/*!
    \property QLineEdit::frame
    \brief Whether the line edit draws itself with a frame.

    If enabled (the default), the line edit draws itself inside a
    frame. Otherwise, the line edit draws itself without any frame.
*/
bool QLineEdit::hasFrame() const
{
    Q_D(const QLineEdit);
    return d->frame;
}

/*!
    \enum QLineEdit::ActionPosition

    This enum type describes how a line edit should display the action widgets to be
    added.

    \value LeadingPosition  The widget is displayed to the left of the text
                            when using layout direction \c Qt::LeftToRight or to
                            the right when using \c Qt::RightToLeft, respectively.

    \value TrailingPosition The widget is displayed to the right of the text
                            when using layout direction \c Qt::LeftToRight or to
                            the left when using \c Qt::RightToLeft, respectively.

    \sa addAction(), removeAction(), QWidget::layoutDirection

    \since 5.2
*/

#if QT_CONFIG(action)
/*!
    Adds the \a action to the list of actions at the \a position.

    \since 5.2
*/

void QLineEdit::addAction(QAction *action, ActionPosition position)
{
    Q_D(QLineEdit);
    QWidget::addAction(action);
    d->addAction(action, nullptr, position);
}

/*!
    \overload

    Creates a new action with the given \a icon at the \a position.

    \since 5.2
*/

QAction *QLineEdit::addAction(const QIcon &icon, ActionPosition position)
{
    QAction *result = new QAction(icon, QString(), this);
    addAction(result, position);
    return result;
}
#endif // QT_CONFIG(action)
/*!
    \property QLineEdit::clearButtonEnabled
    \brief Whether the line edit displays a clear button when it is not empty.

    If enabled, the line edit displays a trailing \uicontrol clear button when
    it contains some text. Otherwise, the line edit does not show a
    \uicontrol clear button (the default).

    \sa addAction(), removeAction()
    \since 5.2
*/

static const char clearButtonActionNameC[] = "_q_qlineeditclearaction";

void QLineEdit::setClearButtonEnabled(bool enable)
{
#if QT_CONFIG(action)
    Q_D(QLineEdit);
    if (enable == isClearButtonEnabled())
        return;
    if (enable) {
        QAction *clearAction = new QAction(d->clearButtonIcon(), QString(), this);
        clearAction->setEnabled(!isReadOnly());
        clearAction->setObjectName(QLatin1StringView(clearButtonActionNameC));

        int flags = QLineEditPrivate::SideWidgetClearButton | QLineEditPrivate::SideWidgetFadeInWithText;
        auto widgetAction = d->addAction(clearAction, nullptr, QLineEdit::TrailingPosition, flags);
        widgetAction->setVisible(!text().isEmpty());
    } else {
        QAction *clearAction = findChild<QAction *>(QLatin1StringView(clearButtonActionNameC));
        Q_ASSERT(clearAction);
        d->removeAction(clearAction);
        delete clearAction;
    }
#else
    Q_UNUSED(enable);
#endif // QT_CONFIG(action)
}

bool QLineEdit::isClearButtonEnabled() const
{
#if QT_CONFIG(action)
    return findChild<QAction *>(QLatin1StringView(clearButtonActionNameC));
#else
    return false;
#endif
}

void QLineEdit::setFrame(bool enable)
{
    Q_D(QLineEdit);
    d->frame = enable;
    update();
    updateGeometry();
}


/*!
    \enum QLineEdit::EchoMode

    This enum type describes how a line edit should display its
    contents.

    \value Normal   Display characters as they are entered. This is the
                    default.
    \value NoEcho   Do not display anything. This may be appropriate
                    for passwords where even the length of the
                    password should be kept secret.
    \value Password  Display platform-dependent password mask characters instead
                    of the characters actually entered.
    \value PasswordEchoOnEdit Display characters only while they are entered.
                    Otherwise, display characters as with \c Password.

    \sa setEchoMode(), echoMode()
*/


/*!
    \property QLineEdit::echoMode
    \brief The line edit's echo mode.

    The echo mode determines how the text entered in the line edit is
    displayed (or echoed) to the user.

    The most common setting is \l Normal, in which the text entered by the
    user is displayed verbatim. QLineEdit also supports modes that allow
    the entered text to be suppressed or obscured: these include \l NoEcho,
    \l Password and \l PasswordEchoOnEdit.

    The widget's display and the ability to copy or drag the text is
    affected by this setting.

    By default, this property is set to \l Normal.

    \sa EchoMode, displayText()
*/

QLineEdit::EchoMode QLineEdit::echoMode() const
{
    Q_D(const QLineEdit);
    return (EchoMode) d->control->echoMode();
}

void QLineEdit::setEchoMode(EchoMode mode)
{
    Q_D(QLineEdit);
    if (mode == (EchoMode)d->control->echoMode())
        return;
    Qt::InputMethodHints imHints = inputMethodHints();
    imHints.setFlag(Qt::ImhHiddenText, mode == Password || mode == NoEcho);
    imHints.setFlag(Qt::ImhNoAutoUppercase, mode != Normal);
    imHints.setFlag(Qt::ImhNoPredictiveText, mode != Normal);
    imHints.setFlag(Qt::ImhSensitiveData, mode != Normal);
    setInputMethodHints(imHints);
    d->control->setEchoMode(mode);
    update();
}


#ifndef QT_NO_VALIDATOR
/*!
    Returns a pointer to the current input validator, or \nullptr if no
    validator has been set.

    \sa setValidator()
*/

const QValidator * QLineEdit::validator() const
{
    Q_D(const QLineEdit);
    return d->control->validator();
}

/*!
    Sets the validator for values of line edit to \a v.

    The line edit's returnPressed() and editingFinished() signals will only
    be emitted if \a v validates the line edit's content as \l{QValidator::}{Acceptable}.
    The user may change the content to any \l{QValidator::}{Intermediate}
    value during editing, but will be prevented from editing the text to a
    value that \a v validates as \l{QValidator::}{Invalid}.

    This allows you to constrain the text that will be stored when editing is
    done while leaving users with enough freedom to edit the text from one valid
    state to another.

    To remove the current input validator, pass \c nullptr. The initial setting
    is to have no input validator (any input is accepted up to maxLength()).

    \sa validator(), hasAcceptableInput(), QIntValidator, QDoubleValidator,
    QRegularExpressionValidator
*/

void QLineEdit::setValidator(const QValidator *v)
{
    Q_D(QLineEdit);
    d->control->setValidator(v);
}
#endif // QT_NO_VALIDATOR

#if QT_CONFIG(completer)
/*!
    \since 4.2

    Sets this line edit to provide auto completions from the completer, \a c.
    The completion mode is set using QCompleter::setCompletionMode().

    To use a QCompleter with a QValidator or QLineEdit::inputMask, you need to
    ensure that the model provided to QCompleter contains valid entries. You can
    use the QSortFilterProxyModel to ensure that the QCompleter's model contains
    only valid entries.

    To remove the completer and disable auto-completion, pass a \c nullptr.

    \sa QCompleter
*/
void QLineEdit::setCompleter(QCompleter *c)
{
    Q_D(QLineEdit);
    if (c == d->control->completer())
        return;
    if (d->control->completer()) {
        d->disconnectCompleter();
        d->control->completer()->setWidget(nullptr);
        if (d->control->completer()->parent() == this)
            delete d->control->completer();
    }
    d->control->setCompleter(c);
    if (!c)
        return;
    if (c->widget() == nullptr)
        c->setWidget(this);
    if (hasFocus())
        d->connectCompleter();
}

/*!
    \since 4.2

    Returns the current QCompleter that provides completions.
*/
QCompleter *QLineEdit::completer() const
{
    Q_D(const QLineEdit);
    return d->control->completer();
}

#endif // QT_CONFIG(completer)

/*!
    Returns a recommended size for the widget.

    The width returned, in pixels, is usually enough for about 15 to
    20 characters.
*/

QSize QLineEdit::sizeHint() const
{
    Q_D(const QLineEdit);
    ensurePolished();
    QFontMetrics fm(font());
    const int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
    const QMargins tm = d->effectiveTextMargins();
    int h = qMax(fm.height(), qMax(14, iconSize - 2)) + 2 * QLineEditPrivate::verticalMargin
            + tm.top() + tm.bottom()
            + d->topmargin + d->bottommargin;
    int w = fm.horizontalAdvance(u'x') * 17 + 2 * QLineEditPrivate::horizontalMargin
            + tm.left() + tm.right()
            + d->leftmargin + d->rightmargin; // "some"
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    return style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h), this);
}


/*!
    Returns a minimum size for the line edit.

    The width returned is usually enough for at least one character.
*/

QSize QLineEdit::minimumSizeHint() const
{
    Q_D(const QLineEdit);
    ensurePolished();
    QFontMetrics fm = fontMetrics();
    const QMargins tm = d->effectiveTextMargins();
    int h = fm.height() + qMax(2 * QLineEditPrivate::verticalMargin, fm.leading())
            + tm.top() + tm.bottom()
            + d->topmargin + d->bottommargin;
    int w = fm.maxWidth() + 2 * QLineEditPrivate::horizontalMargin
            + tm.left() + tm.right()
            + d->leftmargin + d->rightmargin;
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    return style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h), this);
}


/*!
    \property QLineEdit::cursorPosition
    \brief The current cursor position for this line edit.

    Setting the cursor position causes a repaint when appropriate.

    By default, this property contains a value of 0.
*/

int QLineEdit::cursorPosition() const
{
    Q_D(const QLineEdit);
    return d->control->cursorPosition();
}

void QLineEdit::setCursorPosition(int pos)
{
    Q_D(QLineEdit);
    d->control->setCursorPosition(pos);
}

// ### What should this do if the point is outside of contentsRect? Currently returns 0.
/*!
    Returns the cursor position under the point \a pos.
*/
int QLineEdit::cursorPositionAt(const QPoint &pos)
{
    Q_D(QLineEdit);
    return d->xToPos(pos.x());
}



/*!
    \property QLineEdit::alignment
    \brief The alignment of the line edit.

    Both horizontal and vertical alignment is allowed here, Qt::AlignJustify
    will map to Qt::AlignLeft.

    By default, this property contains a combination of Qt::AlignLeft and Qt::AlignVCenter.

    \sa Qt::Alignment
*/

Qt::Alignment QLineEdit::alignment() const
{
    Q_D(const QLineEdit);
    return QFlag(d->alignment);
}

void QLineEdit::setAlignment(Qt::Alignment alignment)
{
    Q_D(QLineEdit);
    d->alignment = alignment;
    update();
}


/*!
    Moves the cursor forward \a steps characters. If \a mark is true,
    each character moved over is added to the selection. If \a mark is
    false, the selection is cleared.

    \sa cursorBackward()
*/

void QLineEdit::cursorForward(bool mark, int steps)
{
    Q_D(QLineEdit);
    d->control->cursorForward(mark, steps);
}


/*!
    Moves the cursor back \a steps characters. If \a mark is true, each
    character moved over is added to the selection. If \a mark is
    false, the selection is cleared.

    \sa cursorForward()
*/
void QLineEdit::cursorBackward(bool mark, int steps)
{
    cursorForward(mark, -steps);
}

/*!
    Moves the cursor one word forward. If \a mark is true, the word is
    also selected.

    \sa cursorWordBackward()
*/
void QLineEdit::cursorWordForward(bool mark)
{
    Q_D(QLineEdit);
    d->control->cursorWordForward(mark);
}

/*!
    Moves the cursor one word backward. If \a mark is true, the word
    is also selected.

    \sa cursorWordForward()
*/

void QLineEdit::cursorWordBackward(bool mark)
{
    Q_D(QLineEdit);
    d->control->cursorWordBackward(mark);
}


/*!
    If no text is selected, deletes the character to the left of the
    text cursor, and moves the cursor one position to the left. If any
    text is selected, the cursor is moved to the beginning of the
    selected text, and the selected text is deleted.

    \sa del()
*/
void QLineEdit::backspace()
{
    Q_D(QLineEdit);
    d->control->backspace();
}

/*!
    If no text is selected, deletes the character to the right of the
    text cursor. If any text is selected, the cursor is moved to the
    beginning of the selected text, and the selected text is deleted.

    \sa backspace()
*/

void QLineEdit::del()
{
    Q_D(QLineEdit);
    d->control->del();
}

/*!
    Moves the text cursor to the beginning of the line unless it is
    already there. If \a mark is true, text is selected towards the
    first position. Otherwise, any selected text is unselected if the
    cursor is moved.

    \sa end()
*/

void QLineEdit::home(bool mark)
{
    Q_D(QLineEdit);
    d->control->home(mark);
}

/*!
    Moves the text cursor to the end of the line unless it is already
    there. If \a mark is true, text is selected towards the last
    position. Otherwise, any selected text is unselected if the cursor
    is moved.

    \sa home()
*/

void QLineEdit::end(bool mark)
{
    Q_D(QLineEdit);
    d->control->end(mark);
}


/*!
    \property QLineEdit::modified
    \brief Whether the line edit's contents has been modified by the user.

    The modified flag is never read by QLineEdit; it has a default value
    of false and is changed to true whenever the user changes the line
    edit's contents.

    This is useful for things that need to provide a default value but
    do not start out knowing what the default should be (for example, it
    depends on other fields on the form). Start the line edit without
    the best default, and when the default is known, if modified()
    returns \c false (the user hasn't entered any text), insert the
    default value.

    Calling setText() resets the modified flag to false.
*/

bool QLineEdit::isModified() const
{
    Q_D(const QLineEdit);
    return d->control->isModified();
}

void QLineEdit::setModified(bool modified)
{
    Q_D(QLineEdit);
    d->control->setModified(modified);
}

/*!
    \property QLineEdit::hasSelectedText
    \brief Whether there is any text selected.

    hasSelectedText() returns \c true if some or all of the text has been
    selected by the user. Otherwise, it returns \c false.

    By default, this property is \c false.

    \sa selectedText()
*/


bool QLineEdit::hasSelectedText() const
{
    Q_D(const QLineEdit);
    return d->control->hasSelectedText();
}

/*!
    \property QLineEdit::selectedText
    \brief The selected text.

    If there is no selected text, this property's value is
    an empty string.

    By default, this property contains an empty string.

    \sa hasSelectedText()
*/

QString QLineEdit::selectedText() const
{
    Q_D(const QLineEdit);
    return d->control->selectedText();
}

/*!
    Returns the index of the first selected character in the
    line edit (or -1 if no text is selected).

    \sa selectedText()
    \sa selectionEnd()
    \sa selectionLength()
*/

int QLineEdit::selectionStart() const
{
    Q_D(const QLineEdit);
    return d->control->selectionStart();
}

/*!
    Returns the index of the character directly after the selection
    in the line edit (or -1 if no text is selected).
    \since 5.10

    \sa selectedText()
    \sa selectionStart()
    \sa selectionLength()
*/
int QLineEdit::selectionEnd() const
{
   Q_D(const QLineEdit);
   return d->control->selectionEnd();
}

/*!
    Returns the length of the selection.
    \since 5.10

    \sa selectedText()
    \sa selectionStart()
    \sa selectionEnd()
*/
int QLineEdit::selectionLength() const
{
   return selectionEnd() - selectionStart();
}

/*!
    Selects text from position \a start and for \a length characters.
    Negative lengths are allowed.

    \sa deselect(), selectAll(), selectedText()
*/

void QLineEdit::setSelection(int start, int length)
{
    Q_D(QLineEdit);
    if (Q_UNLIKELY(start < 0 || start > (int)d->control->end())) {
        qWarning("QLineEdit::setSelection: Invalid start position (%d)", start);
        return;
    }

    d->control->setSelection(start, length);

    if (d->control->hasSelectedText()){
        QStyleOptionFrame opt;
        initStyleOption(&opt);
        if (!style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, this))
            d->setCursorVisible(false);
    }
}


/*!
    \property QLineEdit::undoAvailable
    \brief Whether undo is available.

    Undo becomes available once the user has modified the text in the line edit.

    By default, this property is \c false.
*/

bool QLineEdit::isUndoAvailable() const
{
    Q_D(const QLineEdit);
    return d->control->isUndoAvailable();
}

/*!
    \property QLineEdit::redoAvailable
    \brief Whether redo is available.

    Redo becomes available once the user has performed one or more undo operations
    on the text in the line edit.

    By default, this property is \c false.
*/

bool QLineEdit::isRedoAvailable() const
{
    Q_D(const QLineEdit);
    return d->control->isRedoAvailable();
}

/*!
    \property QLineEdit::dragEnabled
    \brief Whether the line edit starts a drag if the user presses and
    moves the mouse on some selected text.

    Dragging is disabled by default.
*/

bool QLineEdit::dragEnabled() const
{
    Q_D(const QLineEdit);
    return d->dragEnabled;
}

void QLineEdit::setDragEnabled(bool b)
{
    Q_D(QLineEdit);
    d->dragEnabled = b;
}

/*!
  \property QLineEdit::cursorMoveStyle
  \brief The movement style of the cursor in this line edit.
  \since 4.8

  When this property is set to Qt::VisualMoveStyle, the line edit will use a
  visual movement style. Using the left arrow key will always cause the
  cursor to move left, regardless of the text's writing direction. The same
  behavior applies to the right arrow key.

  When the property is set to Qt::LogicalMoveStyle (the default), within a
  left-to-right (LTR) text block, using the left arrow key will increase
  the cursor position, whereas using the right arrow key will decrease the
  cursor position. If the text block is right-to-left (RTL), the opposite
  behavior applies.
*/

Qt::CursorMoveStyle QLineEdit::cursorMoveStyle() const
{
    Q_D(const QLineEdit);
    return d->control->cursorMoveStyle();
}

void QLineEdit::setCursorMoveStyle(Qt::CursorMoveStyle style)
{
    Q_D(QLineEdit);
    d->control->setCursorMoveStyle(style);
}

/*!
    \property QLineEdit::acceptableInput
    \brief Whether the input satisfies the inputMask and the
    validator.

    By default, this property is \c true.

    \sa setInputMask(), setValidator()
*/
bool QLineEdit::hasAcceptableInput() const
{
    Q_D(const QLineEdit);
    return d->control->hasAcceptableInput();
}

/*!
    \since 4.5
    Sets the margins around the text inside the frame to have the
    sizes \a left, \a top, \a right, and \a bottom.

    \sa textMargins()
*/
void QLineEdit::setTextMargins(int left, int top, int right, int bottom)
{
    setTextMargins({left, top, right, bottom});
}

/*!
    \since 4.6
    Sets the \a margins around the text inside the frame.

    \sa textMargins()
*/
void QLineEdit::setTextMargins(const QMargins &margins)
{
    Q_D(QLineEdit);
    d->textMargins = margins;
    updateGeometry();
    update();
}

/*!
    \since 4.6
    Returns the widget's text margins.

    \sa setTextMargins()
*/
QMargins QLineEdit::textMargins() const
{
    Q_D(const QLineEdit);
    return d->textMargins;
}

/*!
    \property QLineEdit::inputMask
    \brief The validation input mask.

    Sets the QLineEdit's validation mask. Validators can be used
    instead of, or in conjunction with masks; see setValidator(). The default is
    an empty string, which means that no input mask is used.

    To unset the mask and return to normal QLineEdit operation, pass an empty
    string.

    The input mask is an input template string. It can contain the following
    elements:
    \table
    \row \li Mask Characters \li Defines the \l {QChar::} {Category} of input
    characters that are considered valid in this position.
    \row \li Meta Characters \li Various special meanings (see details below).
    \row \li Separators \li All other characters are regarded as immutable
    separators.
    \endtable

    The following table shows the mask and meta characters that can be used in
    an input mask.

    \table
    \header \li Mask Character \li Meaning
    \row \li \c A \li Character of the Letter category required, such as A-Z,
                      a-z.
    \row \li \c a \li Character of the Letter category permitted but not
                      required.
    \row \li \c N \li Character of the Letter or Number category required, such
                      as A-Z, a-z, 0-9.
    \row \li \c n \li Character of the Letter or Number category permitted but
                      not required.
    \row \li \c X \li Any non-blank character required.
    \row \li \c x \li Any non-blank character permitted but not required.
    \row \li \c 9 \li Character of the Number category required, such as 0-9.
    \row \li \c 0 \li Character of the Number category permitted but not
                      required.
    \row \li \c D \li Character of the Number category and larger than zero
                      required, such as 1-9.
    \row \li \c d \li Character of the Number category and larger than zero
                      permitted but not required, such as 1-9.
    \row \li \c # \li Character of the Number category, or plus/minus sign
                      permitted but not required.
    \row \li \c H \li Hexadecimal character required. A-F, a-f, 0-9.
    \row \li \c h \li Hexadecimal character permitted but not required.
    \row \li \c B \li Binary character required. 0-1.
    \row \li \c b \li Binary character permitted but not required.
    \header \li Meta Character \li Meaning
    \row \li \c > \li All following alphabetic characters are uppercased.
    \row \li \c < \li All following alphabetic characters are lowercased.
    \row \li \c ! \li Switch off case conversion.
    \row \li \c {;c} \li Terminates the input mask and sets the \e{blank}
                      character to \e{c}.
    \row \li \c {[ ] { }} \li Reserved.
    \row \li \tt{\\} \li Use \tt{\\} to escape the special characters listed
                      above to use them as separators.
    \endtable

    When created or cleared, the line edit will be filled with a copy of the
    input mask string where the meta characters have been removed, and the mask
    characters have been replaced with the \e{blank} character (by default, a
    \c space).

    When an input mask is set, the text() method returns a modified copy of the
    line edit content where all the \e{blank} characters have been removed. The
    unmodified content can be read using displayText().

    The hasAcceptableInput() method returns false if the current content of the
    line edit does not fulfill the requirements of the input mask.

    Examples:
    \table
    \header \li Mask \li Notes
    \row \li \c 000.000.000.000;_ \li IP address; blanks are \c{_}.
    \row \li \c HH:HH:HH:HH:HH:HH;_ \li MAC address
    \row \li \c 0000-00-00 \li ISO Date; blanks are \c space
    \row \li \c >AAAAA-AAAAA-AAAAA-AAAAA-AAAAA;# \li License number;
    blanks are \c{#} and all (alphabetic) characters are converted to
    uppercase.
    \endtable

    To get range control (e.g., for an IP address) use masks together
    with \l{setValidator()}{validators}.

    \sa maxLength, QChar::isLetter(), QChar::isNumber(), QChar::digitValue()
*/
QString QLineEdit::inputMask() const
{
    Q_D(const QLineEdit);
    return d->control->inputMask();
}

void QLineEdit::setInputMask(const QString &inputMask)
{
    Q_D(QLineEdit);
    d->control->setInputMask(inputMask);
}

/*!
    Selects all the text (highlights it) and moves the cursor to
    the end.

    \note This is useful when a default value has been inserted
    because if the user types before clicking on the widget, the
    selected text will be deleted.

    \sa setSelection(), deselect()
*/

void QLineEdit::selectAll()
{
    Q_D(QLineEdit);
    d->control->selectAll();
}

/*!
    Deselects any selected text.

    \sa setSelection(), selectAll()
*/

void QLineEdit::deselect()
{
    Q_D(QLineEdit);
    d->control->deselect();
}


/*!
    Deletes any selected text, inserts \a newText, and validates the
    result. If it is valid, it sets the new text as the new contents of the line
    edit.

    \sa setText(), clear()
*/
void QLineEdit::insert(const QString &newText)
{
//     q->resetInputContext(); //#### FIX ME IN QT
    Q_D(QLineEdit);
    d->control->insert(newText);
}

/*!
    Clears the contents of the line edit.

    \sa setText(), insert()
*/
void QLineEdit::clear()
{
    Q_D(QLineEdit);
    d->resetInputMethod();
    d->control->clear();
}

/*!
    Undoes the last operation if undo is \l{QLineEdit::undoAvailable}{available}. Deselects any current
    selection, and updates the selection start to the current cursor
    position.
*/
void QLineEdit::undo()
{
    Q_D(QLineEdit);
    d->resetInputMethod();
    d->control->undo();
}

/*!
    Redoes the last operation if redo is \l{QLineEdit::redoAvailable}{available}.
*/
void QLineEdit::redo()
{
    Q_D(QLineEdit);
    d->resetInputMethod();
    d->control->redo();
}


/*!
    \property QLineEdit::readOnly
    \brief Whether the line edit is read-only.

    In read-only mode, the user can still copy the text to the
    clipboard, or drag and drop the text (if echoMode() is \l Normal),
    but cannot edit it.

    QLineEdit does not show a cursor in read-only mode.

    By default, this property is \c false.

    \sa setEnabled()
*/

bool QLineEdit::isReadOnly() const
{
    Q_D(const QLineEdit);
    return d->control->isReadOnly();
}

void QLineEdit::setReadOnly(bool enable)
{
    Q_D(QLineEdit);
    if (d->control->isReadOnly() != enable) {
        d->control->setReadOnly(enable);
        d->setClearButtonEnabled(!enable);
        setAttribute(Qt::WA_MacShowFocusRect, !enable);
        setAttribute(Qt::WA_InputMethodEnabled, d->shouldEnableInputMethod());
#ifndef QT_NO_CURSOR
        setCursor(enable ? Qt::ArrowCursor : Qt::IBeamCursor);
#endif
        QEvent event(QEvent::ReadOnlyChange);
        QCoreApplication::sendEvent(this, &event);
        update();
#if QT_CONFIG(accessibility)
        QAccessible::State changedState;
        changedState.readOnly = true;
        QAccessibleStateChangeEvent ev(this, changedState);
        QAccessible::updateAccessibility(&ev);
#endif
    }
}


#ifndef QT_NO_CLIPBOARD
/*!
    Copies the selected text to the clipboard and deletes it, if there
    is any, and if echoMode() is \l Normal.

    If the current validator disallows deleting the selected text,
    cut() will copy without deleting.

    \sa copy(), paste(), setValidator()
*/

void QLineEdit::cut()
{
    if (hasSelectedText()) {
        copy();
        del();
    }
}


/*!
    Copies the selected text to the clipboard, if there is any, and if
    echoMode() is \l Normal.

    \sa cut(), paste()
*/

void QLineEdit::copy() const
{
    Q_D(const QLineEdit);
    d->control->copy();
}

/*!
    Inserts the clipboard's text at the cursor position, deleting any
    selected text, providing the line edit is not \l{QLineEdit::readOnly}{read-only}.

    If the end result would be invalid to the current
    \l{setValidator()}{validator}, nothing happens.

    \sa copy(), cut()
*/

void QLineEdit::paste()
{
    Q_D(QLineEdit);
    d->control->paste();
}

#endif // !QT_NO_CLIPBOARD

/*!
    \reimp
*/
void QLineEdit::timerEvent(QTimerEvent *e)
{
    Q_D(QLineEdit);
    int timerId = ((QTimerEvent*)e)->timerId();
    if (false) {
#if QT_CONFIG(draganddrop)
    } else if (timerId == d->dndTimer.timerId()) {
        d->drag();
#endif
    }
    else if (timerId == d->tripleClickTimer.timerId())
        d->tripleClickTimer.stop();
}

/*! \reimp
*/
bool QLineEdit::event(QEvent * e)
{
    Q_D(QLineEdit);
    if (e->type() == QEvent::ContextMenu) {
#ifndef QT_NO_IM
        if (d->control->composeMode())
            return true;
#endif
        //d->separate();
    } else if (e->type() == QEvent::WindowActivate) {
        QTimer::singleShot(0, this, [this]() {
            Q_D(QLineEdit);
            d->handleWindowActivate();
        });
#ifndef QT_NO_SHORTCUT
    } else if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        d->control->processShortcutOverrideEvent(ke);
#endif
    } else if (e->type() == QEvent::Show) {
        //In order to get the cursor blinking if QComboBox::setEditable is called when the combobox has focus
        if (hasFocus()) {
            d->control->setBlinkingCursorEnabled(true);
            QStyleOptionFrame opt;
            initStyleOption(&opt);
            if ((!hasSelectedText() && d->control->preeditAreaText().isEmpty())
                || style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, this))
                d->setCursorVisible(true);
        }
    } else if (e->type() == QEvent::Hide) {
        d->control->setBlinkingCursorEnabled(false);
#if QT_CONFIG(action)
    } else if (e->type() == QEvent::ActionRemoved) {
        d->removeAction(static_cast<QActionEvent *>(e)->action());
#endif
    } else if (e->type() == QEvent::Resize) {
        d->positionSideWidgets();
    } else if (e->type() == QEvent::StyleChange) {
        d->initMouseYThreshold();
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplicationPrivate::keypadNavigationEnabled()) {
        if (e->type() == QEvent::EnterEditFocus) {
            end(false);
            d->setCursorVisible(true);
            d->control->setCursorBlinkEnabled(true);
        } else if (e->type() == QEvent::LeaveEditFocus) {
            d->setCursorVisible(false);
            d->control->setCursorBlinkEnabled(false);
            if (d->edited && (d->control->hasAcceptableInput()
                              || d->control->fixup())) {
                emit editingFinished();
                d->edited = false;
            }
        }
    }
#endif
    return QWidget::event(e);
}

/*! \reimp
*/
void QLineEdit::mousePressEvent(QMouseEvent* e)
{
    Q_D(QLineEdit);

    d->mousePressPos = e->position().toPoint();

    if (d->sendMouseEventToInputContext(e))
        return;
    if (e->button() == Qt::RightButton) {
        e->ignore();
        return;
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::QApplicationPrivate() && !hasEditFocus()) {
        setEditFocus(true);
        // Get the completion list to pop up.
        if (d->control->completer())
            d->control->completer()->complete();
    }
#endif
    if (d->tripleClickTimer.isActive() && (e->position().toPoint() - d->tripleClick).manhattanLength() <
         QApplication::startDragDistance()) {
        selectAll();
        return;
    }
    bool mark = e->modifiers() & Qt::ShiftModifier;
#ifdef Q_OS_ANDROID
    mark = mark && (d->imHints & Qt::ImhNoPredictiveText);
#endif // Q_OS_ANDROID
    int cursor = d->xToPos(e->position().toPoint().x());
#if QT_CONFIG(draganddrop)
    if (!mark && d->dragEnabled && d->control->echoMode() == Normal &&
         e->button() == Qt::LeftButton && d->inSelection(e->position().toPoint().x())) {
        if (!d->dndTimer.isActive())
            d->dndTimer.start(QApplication::startDragTime(), this);
    } else
#endif
    {
        d->control->moveCursor(cursor, mark);
    }
}

/*! \reimp
*/
void QLineEdit::mouseMoveEvent(QMouseEvent * e)
{
    Q_D(QLineEdit);

    if (e->buttons() & Qt::LeftButton) {
#if QT_CONFIG(draganddrop)
        if (d->dndTimer.isActive()) {
            if ((d->mousePressPos - e->position().toPoint()).manhattanLength() > QApplication::startDragDistance())
                d->drag();
        } else
#endif
        {
#ifndef Q_OS_ANDROID
            const bool select = true;
#else
            const bool select = (d->imHints & Qt::ImhNoPredictiveText);
#endif
#ifndef QT_NO_IM
            if (d->mouseYThreshold > 0 && e->position().toPoint().y() > d->mousePressPos.y() + d->mouseYThreshold) {
                if (layoutDirection() == Qt::RightToLeft)
                    d->control->home(select);
                else
                    d->control->end(select);
            } else if (d->mouseYThreshold > 0 && e->position().toPoint().y() + d->mouseYThreshold < d->mousePressPos.y()) {
                if (layoutDirection() == Qt::RightToLeft)
                    d->control->end(select);
                else
                    d->control->home(select);
            } else if (d->control->composeMode() && select) {
                int startPos = d->xToPos(d->mousePressPos.x());
                int currentPos = d->xToPos(e->position().toPoint().x());
                if (startPos != currentPos)
                    d->control->setSelection(startPos, currentPos - startPos);

            } else
#endif
            {
                d->control->moveCursor(d->xToPos(e->position().toPoint().x()), select);
            }
        }
    }

    d->sendMouseEventToInputContext(e);
}

/*! \reimp
*/
void QLineEdit::mouseReleaseEvent(QMouseEvent* e)
{
    Q_D(QLineEdit);
    if (d->sendMouseEventToInputContext(e))
        return;
#if QT_CONFIG(draganddrop)
    if (e->button() == Qt::LeftButton) {
        if (d->dndTimer.isActive()) {
            d->dndTimer.stop();
            deselect();
            return;
        }
    }
#endif
#ifndef QT_NO_CLIPBOARD
    if (QGuiApplication::clipboard()->supportsSelection()) {
        if (e->button() == Qt::LeftButton) {
            d->control->copy(QClipboard::Selection);
        } else if (!d->control->isReadOnly() && e->button() == Qt::MiddleButton) {
            deselect();
            d->control->paste(QClipboard::Selection);
        }
    }
#endif

    if (!isReadOnly() && rect().contains(e->position().toPoint()))
        d->handleSoftwareInputPanel(e->button(), d->clickCausedFocus);
    d->clickCausedFocus = 0;
}

/*! \reimp
*/
void QLineEdit::mouseDoubleClickEvent(QMouseEvent* e)
{
    Q_D(QLineEdit);

    if (e->button() == Qt::LeftButton) {
        int position = d->xToPos(e->position().toPoint().x());

        // exit composition mode
#ifndef QT_NO_IM
        if (d->control->composeMode()) {
            int preeditPos = d->control->cursor();
            int posInPreedit = position - d->control->cursor();
            int preeditLength = d->control->preeditAreaText().size();
            bool positionOnPreedit = false;

            if (posInPreedit >= 0 && posInPreedit <= preeditLength)
                positionOnPreedit = true;

            int textLength = d->control->end();
            d->control->commitPreedit();
            int sizeChange = d->control->end() - textLength;

            if (positionOnPreedit) {
                if (sizeChange == 0)
                    position = -1; // cancel selection, word disappeared
                else
                    // ensure not selecting after preedit if event happened there
                    position = qBound(preeditPos, position, preeditPos + sizeChange);
            } else if (position > preeditPos) {
                // adjust positions after former preedit by how much text changed
                position += (sizeChange - preeditLength);
            }
        }
#endif

        if (position >= 0)
            d->control->selectWordAtPos(position);

        d->tripleClickTimer.start(QApplication::doubleClickInterval(), this);
        d->tripleClick = e->position().toPoint();
    } else {
        d->sendMouseEventToInputContext(e);
    }
}

/*!
    \fn void  QLineEdit::returnPressed()

    This signal is emitted when the Return or Enter key is used.

    \note If there is a validator() or inputMask() set on the line
    edit, the returnPressed() signal will only be emitted if the input
    follows the inputMask() and the validator() returns
    QValidator::Acceptable.
*/

/*!
    \fn void  QLineEdit::editingFinished()

    This signal is emitted when the Return or Enter key is used, or if the line
    edit loses focus and its contents have changed since the last time this
    signal was emitted.

    \note If there is a validator() or inputMask() set on the line edit and
    enter/return is used, the editingFinished() signal will only be emitted
    if the input follows the inputMask() and the validator() returns
    QValidator::Acceptable.
*/

/*!
    \fn void QLineEdit::inputRejected()
    \since 5.12

    This signal is emitted when the user uses a key that is not
    considered to be valid input. For example, if using a key results in a
    validator's \l {QValidator::validate()}{validate()} call to return
    \l {QValidator::Invalid}{Invalid}. Another case is when trying
    to enter more characters beyond the maximum length of the line edit.

    \note This signal will still be emitted when only a part of the text is
    accepted. For example, if there is a maximum length set and the clipboard
    text is longer than the maximum length when it is pasted.
*/

/*!
    Converts the given key press \a event into a line edit action.

    If Return or Enter is used and the current text is valid (or
    can be \l{QValidator::fixup()}{made valid} by the
    validator), the signal returnPressed() is emitted.

    \sa {Default Key Bindings}
*/

void QLineEdit::keyPressEvent(QKeyEvent *event)
{
    Q_D(QLineEdit);
    #ifdef QT_KEYPAD_NAVIGATION
    bool select = false;
    switch (event->key()) {
        case Qt::Key_Select:
            if (QApplicationPrivate::keypadNavigationEnabled()) {
                if (hasEditFocus()) {
                    setEditFocus(false);
                    if (d->control->completer() && d->control->completer()->popup()->isVisible())
                        d->control->completer()->popup()->hide();
                    select = true;
                }
            }
            break;
        case Qt::Key_Back:
        case Qt::Key_No:
            if (!QApplicationPrivate::keypadNavigationEnabled() || !hasEditFocus()) {
                event->ignore();
                return;
            }
            break;
        default:
            if (QApplicationPrivate::keypadNavigationEnabled()) {
                if (!hasEditFocus() && !(event->modifiers() & Qt::ControlModifier)) {
                    if (!event->text().isEmpty() && event->text().at(0).isPrint()
                        && !isReadOnly())
                        setEditFocus(true);
                    else {
                        event->ignore();
                        return;
                    }
                }
            }
    }



    if (QApplicationPrivate::keypadNavigationEnabled() && !select && !hasEditFocus()) {
        setEditFocus(true);
        if (event->key() == Qt::Key_Select)
            return; // Just start. No action.
    }
#endif
    d->control->processKeyEvent(event);
    if (event->isAccepted())
        d->control->updateCursorBlinking();
}

/*!
    \reimp
*/
void QLineEdit::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QLineEdit);
    if (!isReadOnly())
        d->handleSoftwareInputPanel();
    d->control->updateCursorBlinking();
    QWidget::keyReleaseEvent(e);
}

/*!
  \since 4.4

  Returns a rectangle that includes the line edit cursor.
*/
QRect QLineEdit::cursorRect() const
{
    Q_D(const QLineEdit);
    return d->cursorRect();
}

/*! \reimp
 */
void QLineEdit::inputMethodEvent(QInputMethodEvent *e)
{
    Q_D(QLineEdit);

    if (echoMode() == PasswordEchoOnEdit && !d->control->passwordEchoEditing()) {
        // Clear the edit and reset to normal echo mode while entering input
        // method data; the echo mode switches back when the edit loses focus.
        // ### changes a public property, resets current content.
        d->updatePasswordEchoEditing(true);
        clear();
    }

#ifdef QT_KEYPAD_NAVIGATION
    // Focus in if currently in navigation focus on the widget
    // Only focus in on preedits, to allow input methods to
    // commit text as they focus out without interfering with focus
    if (QApplicationPrivate::keypadNavigationEnabled()
        && hasFocus() && !hasEditFocus()
        && !e->preeditString().isEmpty())
        setEditFocus(true);
#endif

    d->control->processInputMethodEvent(e);

#if QT_CONFIG(completer)
    if (!e->commitString().isEmpty())
        d->control->complete(Qt::Key_unknown);
#endif
}

/*!\reimp
*/
QVariant QLineEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
#ifdef Q_OS_ANDROID
    // QTBUG-61652
    if (property == Qt::ImEnterKeyType) {
        QWidget *next = nextInFocusChain();
        while (next && next != this && next->focusPolicy() == Qt::NoFocus)
            next = next->nextInFocusChain();
        if (next) {
            const auto nextYPos = next->mapToGlobal(QPoint(0, 0)).y();
            const auto currentYPos = mapToGlobal(QPoint(0, 0)).y();
            if (currentYPos < nextYPos)
                // Set EnterKey to KeyNext type only if the next widget
                // in the focus chain is below current QLineEdit
                return Qt::EnterKeyNext;
        }
    }
#endif
    return inputMethodQuery(property, QVariant());
}

/*!\internal
*/
QVariant QLineEdit::inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const
{
    Q_D(const QLineEdit);
    switch(property) {
    case Qt::ImEnabled:
        return isEnabled() && !isReadOnly();
    case Qt::ImCursorRectangle:
        return d->cursorRect();
    case Qt::ImAnchorRectangle:
        return d->adjustedControlRect(d->control->anchorRect());
    case Qt::ImFont:
        return font();
    case Qt::ImAbsolutePosition:
    case Qt::ImCursorPosition: {
        const QPointF pt = argument.toPointF();
        if (!pt.isNull())
            return QVariant(d->xToPos(pt.x(), QTextLine::CursorBetweenCharacters));
        return QVariant(d->control->cursor()); }
    case Qt::ImSurroundingText:
        return QVariant(d->control->surroundingText());
    case Qt::ImCurrentSelection:
        return QVariant(selectedText());
    case Qt::ImMaximumTextLength:
        return QVariant(maxLength());
    case Qt::ImAnchorPosition:
        if (d->control->selectionStart() == d->control->selectionEnd())
            return QVariant(d->control->cursor());
        else if (d->control->selectionStart() == d->control->cursor())
            return QVariant(d->control->selectionEnd());
        else
            return QVariant(d->control->selectionStart());
    case Qt::ImReadOnly:
        return isReadOnly();
    case Qt::ImTextBeforeCursor: {
        const QPointF pt = argument.toPointF();
        if (!pt.isNull())
            return d->textBeforeCursor(d->xToPos(pt.x(), QTextLine::CursorBetweenCharacters));
        else
            return d->textBeforeCursor(d->control->cursor()); }
    case Qt::ImTextAfterCursor: {
        const QPointF pt = argument.toPointF();
        if (!pt.isNull())
            return d->textAfterCursor(d->xToPos(pt.x(), QTextLine::CursorBetweenCharacters));
        else
            return d->textAfterCursor(d->control->cursor()); }
    default:
        return QWidget::inputMethodQuery(property);
    }
}

/*!\reimp
*/

void QLineEdit::focusInEvent(QFocusEvent *e)
{
    Q_D(QLineEdit);
    if (e->reason() == Qt::TabFocusReason ||
         e->reason() == Qt::BacktabFocusReason  ||
         e->reason() == Qt::ShortcutFocusReason) {
        if (!d->control->inputMask().isEmpty())
            d->control->moveCursor(d->control->nextMaskBlank(0));
        else if (!d->control->hasSelectedText())
            selectAll();
        else
            updateMicroFocus();
    } else if (e->reason() == Qt::MouseFocusReason) {
        d->clickCausedFocus = 1;
        updateMicroFocus();
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplicationPrivate::keypadNavigationEnabled() || (hasEditFocus() && ( e->reason() == Qt::PopupFocusReason))) {
#endif
    d->control->setBlinkingCursorEnabled(true);
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    if ((!hasSelectedText() && d->control->preeditAreaText().isEmpty())
       || style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, this))
        d->setCursorVisible(true);
#ifdef QT_KEYPAD_NAVIGATION
        d->control->setCancelText(d->control->text());
    }
#endif
#if QT_CONFIG(completer)
    if (d->control->completer()) {
        d->control->completer()->setWidget(this);
        d->connectCompleter();
    }
#endif
    update();
}

/*!\reimp
*/
void QLineEdit::focusOutEvent(QFocusEvent *e)
{
    Q_D(QLineEdit);
    if (d->control->passwordEchoEditing()) {
        // Reset the echomode back to PasswordEchoOnEdit when the widget loses
        // focus.
        d->updatePasswordEchoEditing(false);
    }

    Qt::FocusReason reason = e->reason();
    if (reason != Qt::ActiveWindowFocusReason &&
        reason != Qt::PopupFocusReason)
        deselect();

    d->setCursorVisible(false);
    d->control->setBlinkingCursorEnabled(false);
#ifdef QT_KEYPAD_NAVIGATION
    // editingFinished() is already emitted on LeaveEditFocus
    if (!QApplicationPrivate::keypadNavigationEnabled())
#endif
    if (reason != Qt::PopupFocusReason
        || !(QApplication::activePopupWidget() && QApplication::activePopupWidget()->parentWidget() == this)) {
            if (d->edited && (hasAcceptableInput() || d->control->fixup())) {
                emit editingFinished();
                d->edited = false;
            }
    }
#ifdef QT_KEYPAD_NAVIGATION
    d->control->setCancelText(QString());
#endif
#if QT_CONFIG(completer)
    if (d->control->completer())
        d->disconnectCompleter();
#endif
    QWidget::focusOutEvent(e);
}

/*!\reimp
*/
void QLineEdit::paintEvent(QPaintEvent *)
{
    Q_D(QLineEdit);
    QPainter p(this);
    QPalette pal = palette();

    QStyleOptionFrame panel;
    initStyleOption(&panel);
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);
    QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
    r = r.marginsRemoved(d->effectiveTextMargins());
    p.setClipRect(r);

    QFontMetrics fm = fontMetrics();
    int fmHeight = 0;
    if (d->shouldShowPlaceholderText())
        fmHeight = fm.boundingRect(d->placeholderText).height();
    else
        fmHeight = fm.boundingRect(d->control->text() + d->control->preeditAreaText()).height();
    fmHeight = qMax(fmHeight, fm.height());

    Qt::Alignment va = QStyle::visualAlignment(d->control->layoutDirection(), QFlag(d->alignment));
    switch (va & Qt::AlignVertical_Mask) {
     case Qt::AlignBottom:
         d->vscroll = r.y() + r.height() - fmHeight - QLineEditPrivate::verticalMargin;
         break;
     case Qt::AlignTop:
         d->vscroll = r.y() + QLineEditPrivate::verticalMargin;
         break;
     default:
         //center
         d->vscroll = r.y() + (r.height() - fmHeight + 1) / 2;
         break;
    }
    QRect lineRect(r.x() + QLineEditPrivate::horizontalMargin, d->vscroll,
                   r.width() - 2 * QLineEditPrivate::horizontalMargin, fmHeight);

    if (d->shouldShowPlaceholderText()) {
        if (!d->placeholderText.isEmpty()) {
            const Qt::LayoutDirection layoutDir = d->placeholderText.isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight;
            const Qt::Alignment alignPhText = QStyle::visualAlignment(layoutDir, QFlag(d->alignment));
            const QColor col = pal.placeholderText().color();
            QPen oldpen = p.pen();
            p.setPen(col);
            Qt::LayoutDirection oldLayoutDir = p.layoutDirection();
            p.setLayoutDirection(layoutDir);

            const QString elidedText = fm.elidedText(d->placeholderText, Qt::ElideRight, lineRect.width());
            p.drawText(lineRect, alignPhText, elidedText);
            p.setPen(oldpen);
            p.setLayoutDirection(oldLayoutDir);
        }
    }

    int cix = qRound(d->control->cursorToX());

    // horizontal scrolling. d->hscroll is the left indent from the beginning
    // of the text line to the left edge of lineRect. we update this value
    // depending on the delta from the last paint event; in effect this means
    // the below code handles all scrolling based on the textline (widthUsed),
    // the line edit rect (lineRect) and the cursor position (cix).
    int widthUsed = qRound(d->control->naturalTextWidth()) + 1;
    if (widthUsed <= lineRect.width()) {
        // text fits in lineRect; use hscroll for alignment
        switch (va & ~(Qt::AlignAbsolute|Qt::AlignVertical_Mask)) {
        case Qt::AlignRight:
            d->hscroll = widthUsed - lineRect.width() + 1;
            break;
        case Qt::AlignHCenter:
            d->hscroll = (widthUsed - lineRect.width()) / 2;
            break;
        default:
            // Left
            d->hscroll = 0;
            break;
        }
    } else if (cix - d->hscroll >= lineRect.width()) {
        // text doesn't fit, cursor is to the right of lineRect (scroll right)
        d->hscroll = cix - lineRect.width() + 1;
    } else if (cix - d->hscroll < 0 && d->hscroll < widthUsed) {
        // text doesn't fit, cursor is to the left of lineRect (scroll left)
        d->hscroll = cix;
    } else if (widthUsed - d->hscroll < lineRect.width()) {
        // text doesn't fit, text document is to the left of lineRect; align
        // right
        d->hscroll = widthUsed - lineRect.width() + 1;
    } else {
        //in case the text is bigger than the lineedit, the hscroll can never be negative
        d->hscroll = qMax(0, d->hscroll);
    }

    // the y offset is there to keep the baseline constant in case we have script changes in the text.
    // Needs to be kept in sync with QLineEditPrivate::adjustedControlRect
    QPoint topLeft = lineRect.topLeft() - QPoint(d->hscroll, d->control->ascent() - fm.ascent());

    // draw text, selections and cursors
#if QT_CONFIG(style_stylesheet)
    if (QStyleSheetStyle* cssStyle = qt_styleSheet(style())) {
        cssStyle->styleSheetPalette(this, &panel, &pal);
    }
#endif
    p.setPen(pal.text().color());

    int flags = QWidgetLineControl::DrawText;

#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplicationPrivate::keypadNavigationEnabled() || hasEditFocus())
#endif
    if (d->control->hasSelectedText() || (d->cursorVisible && !d->control->inputMask().isEmpty() && !d->control->isReadOnly())){
        flags |= QWidgetLineControl::DrawSelections;
        // Palette only used for selections/mask and may not be in sync
        if (d->control->palette() != pal
           || d->control->palette().currentColorGroup() != pal.currentColorGroup())
            d->control->setPalette(pal);
    }

    // Asian users see an IM selection text as cursor on candidate
    // selection phase of input method, so the ordinary cursor should be
    // invisible if we have a preedit string. another condition is when inputmask
    // isn't empty,we don't need draw cursor,because cursor and character overlapping
    // area is white.
    if (d->cursorVisible && !d->control->isReadOnly() && d->control->inputMask().isEmpty())
        flags |= QWidgetLineControl::DrawCursor;

    d->control->setCursorWidth(style()->pixelMetric(QStyle::PM_TextCursorWidth, &panel, this));
    d->control->draw(&p, topLeft, r, flags);

}


#if QT_CONFIG(draganddrop)
/*!\reimp
*/
void QLineEdit::dragMoveEvent(QDragMoveEvent *e)
{
    Q_D(QLineEdit);
    if (!d->control->isReadOnly() && e->mimeData()->hasFormat("text/plain"_L1)) {
        e->acceptProposedAction();
        d->control->moveCursor(d->xToPos(e->position().toPoint().x()), false);
        d->cursorVisible = true;
        update();
    }
}

/*!\reimp */
void QLineEdit::dragEnterEvent(QDragEnterEvent * e)
{
    QLineEdit::dragMoveEvent(e);
}

/*!\reimp */
void QLineEdit::dragLeaveEvent(QDragLeaveEvent *)
{
    Q_D(QLineEdit);
    if (d->cursorVisible) {
        d->cursorVisible = false;
        update();
    }
}

/*!\reimp */
void QLineEdit::dropEvent(QDropEvent* e)
{
    Q_D(QLineEdit);
    QString str = e->mimeData()->text();

    if (!str.isNull() && !d->control->isReadOnly()) {
        if (e->source() == this && e->dropAction() == Qt::CopyAction)
            deselect();
        int cursorPos = d->xToPos(e->position().toPoint().x());
        int selStart = cursorPos;
        int oldSelStart = d->control->selectionStart();
        int oldSelEnd = d->control->selectionEnd();
        d->control->moveCursor(cursorPos, false);
        d->cursorVisible = false;
        e->acceptProposedAction();
        insert(str);
        if (e->source() == this) {
            if (e->dropAction() == Qt::MoveAction) {
                if (selStart > oldSelStart && selStart <= oldSelEnd)
                    setSelection(oldSelStart, str.size());
                else if (selStart > oldSelEnd)
                    setSelection(selStart - str.size(), str.size());
                else
                    setSelection(selStart, str.size());
            } else {
                setSelection(selStart, str.size());
            }
        }
    } else {
        e->ignore();
        update();
    }
}

#endif // QT_CONFIG(draganddrop)

#ifndef QT_NO_CONTEXTMENU
/*!
    Shows the standard context menu created with
    createStandardContextMenu().

    If you do not want the line edit to have a context menu, you can set
    its \l contextMenuPolicy to Qt::NoContextMenu. To customize the context
    menu, reimplement this function. To extend the standard context menu,
    reimplement this function, call createStandardContextMenu(), and extend the
    menu returned.

    \snippet code/src_gui_widgets_qlineedit.cpp 0

    The \a event parameter is used to obtain the position where
    the mouse cursor was when the event was generated.

    \sa setContextMenuPolicy()
*/
void QLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    if (QMenu *menu = createStandardContextMenu()) {
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(event->globalPos());
    }
}

/*!  Creates the standard context menu, which is shown
        when the user clicks on the line edit with the right mouse
        button. It is called from the default contextMenuEvent() handler.
        The popup menu's ownership is transferred to the caller.
*/

QMenu *QLineEdit::createStandardContextMenu()
{
    Q_D(QLineEdit);
    QMenu *popup = new QMenu(this);
    popup->setObjectName("qt_edit_menu"_L1);
    QAction *action = nullptr;

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("&Undo") + ACCEL_KEY(QKeySequence::Undo));
        action->setEnabled(d->control->isUndoAvailable());
        action->setObjectName(QStringLiteral("edit-undo"));
        setActionIcon(action, QStringLiteral("edit-undo"));
        connect(action, &QAction::triggered, this, &QLineEdit::undo);

        action = popup->addAction(QLineEdit::tr("&Redo") + ACCEL_KEY(QKeySequence::Redo));
        action->setEnabled(d->control->isRedoAvailable());
        action->setObjectName(QStringLiteral("edit-redo"));
        setActionIcon(action, QStringLiteral("edit-redo"));
        connect(action, &QAction::triggered, this, &QLineEdit::redo);

        popup->addSeparator();
    }

#ifndef QT_NO_CLIPBOARD
    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("Cu&t") + ACCEL_KEY(QKeySequence::Cut));
        action->setEnabled(!d->control->isReadOnly() && d->control->hasSelectedText()
                && d->control->echoMode() == QLineEdit::Normal);
        action->setObjectName(QStringLiteral("edit-cut"));
        setActionIcon(action, QStringLiteral("edit-cut"));
        connect(action, &QAction::triggered, this, &QLineEdit::cut);
    }

    action = popup->addAction(QLineEdit::tr("&Copy") + ACCEL_KEY(QKeySequence::Copy));
    action->setEnabled(d->control->hasSelectedText()
            && d->control->echoMode() == QLineEdit::Normal);
    action->setObjectName(QStringLiteral("edit-copy"));
    setActionIcon(action, QStringLiteral("edit-copy"));
    connect(action, &QAction::triggered, this, &QLineEdit::copy);

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("&Paste") + ACCEL_KEY(QKeySequence::Paste));
        action->setEnabled(!d->control->isReadOnly() && !QGuiApplication::clipboard()->text().isEmpty());
        action->setObjectName(QStringLiteral("edit-paste"));
        setActionIcon(action, QStringLiteral("edit-paste"));
        connect(action, &QAction::triggered, this, &QLineEdit::paste);
    }
#endif

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("Delete"));
        action->setEnabled(!d->control->isReadOnly() && !d->control->text().isEmpty() && d->control->hasSelectedText());
        action->setObjectName(QStringLiteral("edit-delete"));
        setActionIcon(action, QStringLiteral("edit-delete"));
        connect(action, &QAction::triggered,
                d->control, &QWidgetLineControl::_q_deleteSelected);
    }

    if (!popup->isEmpty())
        popup->addSeparator();

    action = popup->addAction(QLineEdit::tr("Select All") + ACCEL_KEY(QKeySequence::SelectAll));
    action->setEnabled(!d->control->text().isEmpty() && !d->control->allSelected());
    action->setObjectName(QStringLiteral("select-all"));
    setActionIcon(action, QStringLiteral("edit-select-all"));
    d->selectAllAction = action;
    connect(action, &QAction::triggered, this, &QLineEdit::selectAll);

    if (!d->control->isReadOnly() && QGuiApplication::styleHints()->useRtlExtensions()) {
        popup->addSeparator();
        QUnicodeControlCharacterMenu *ctrlCharacterMenu = new QUnicodeControlCharacterMenu(this, popup);
        popup->addMenu(ctrlCharacterMenu);
    }
    return popup;
}
#endif // QT_NO_CONTEXTMENU

/*! \reimp */
void QLineEdit::changeEvent(QEvent *ev)
{
    Q_D(QLineEdit);
    switch(ev->type())
    {
    case QEvent::ActivationChange:
        if (!palette().isEqual(QPalette::Active, QPalette::Inactive))
            update();
        break;
    case QEvent::FontChange:
        d->control->setFont(font());
        break;
    case QEvent::StyleChange:
        {
            QStyleOptionFrame opt;
            initStyleOption(&opt);
            d->control->setPasswordCharacter(char16_t(style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, this)));
            d->control->setPasswordMaskDelay(style()->styleHint(QStyle::SH_LineEdit_PasswordMaskDelay, &opt, this));
        }
        update();
        break;
    case QEvent::LayoutDirectionChange:
#if QT_CONFIG(toolbutton)
        for (const auto &e : d->trailingSideWidgets) { // Refresh icon to show arrow in right direction.
            if (e.flags & QLineEditPrivate::SideWidgetClearButton)
                static_cast<QLineEditIconButton *>(e.widget)->setIcon(d->clearButtonIcon());
        }
#endif
        d->positionSideWidgets();
        break;
    default:
        break;
    }
    QWidget::changeEvent(ev);
}

QT_END_NAMESPACE

#include "moc_qlineedit.cpp"
