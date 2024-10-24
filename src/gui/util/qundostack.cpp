// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qdebug.h>
#include "qundostack.h"
#if QT_CONFIG(undogroup)
#include "qundogroup.h"
#endif
#include "qundostack_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QUndoCommand
    \brief The QUndoCommand class is the base class of all commands stored on a QUndoStack.
    \since 4.2

    \inmodule QtGui

    For an overview of Qt's Undo Framework, see the
    \l{Overview of Qt's Undo Framework}{overview document}.

    A QUndoCommand represents a single editing action on a document; for example,
    inserting or deleting a block of text in a text editor. QUndoCommand can apply
    a change to the document with redo() and undo the change with undo(). The
    implementations for these functions must be provided in a derived class.

    \snippet code/src_gui_util_qundostack.cpp 0

    A QUndoCommand has an associated text(). This is a short string
    describing what the command does. It is used to update the text
    properties of the stack's undo and redo actions; see
    QUndoStack::createUndoAction() and QUndoStack::createRedoAction().

    QUndoCommand objects are owned by the stack they were pushed on.
    QUndoStack deletes a command if it has been undone and a new command is pushed. For example:

\snippet code/src_gui_util_qundostack.cpp 1

    In effect, when a command is pushed, it becomes the top-most command
    on the stack.

    To support command compression, QUndoCommand has an id() and the virtual function
    mergeWith(). These functions are used by QUndoStack::push().

    To support command macros, a QUndoCommand object can have any number of child
    commands. Undoing or redoing the parent command will cause the child
    commands to be undone or redone. A command can be assigned
    to a parent explicitly in the constructor. In this case, the command
    will be owned by the parent.

    The parent in this case is usually an empty command, in that it doesn't
    provide its own implementation of undo() and redo(). Instead, it uses
    the base implementations of these functions, which simply call undo() or
    redo() on all its children. The parent should, however, have a meaningful
    text().

    \snippet code/src_gui_util_qundostack.cpp 2

    Another way to create macros is to use the convenience functions
    QUndoStack::beginMacro() and QUndoStack::endMacro().

    \sa QUndoStack
*/

/*!
    Constructs a QUndoCommand object with the given \a parent and \a text.

    If \a parent is not \nullptr, this command is appended to parent's
    child list. The parent command then owns this command and will delete
    it in its destructor.

    \sa ~QUndoCommand()
*/

QUndoCommand::QUndoCommand(const QString &text, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    setText(text);
}

/*!
    Constructs a QUndoCommand object with parent \a parent.

    If \a parent is not \nullptr, this command is appended to parent's
    child list. The parent command then owns this command and will delete
    it in its destructor.

    \sa ~QUndoCommand()
*/

QUndoCommand::QUndoCommand(QUndoCommand *parent)
{
    d = new QUndoCommandPrivate;
    if (parent != nullptr)
        parent->d->child_list.append(this);
}

/*!
    Destroys the QUndoCommand object and all child commands.

    \sa QUndoCommand()
*/

QUndoCommand::~QUndoCommand()
{
    qDeleteAll(d->child_list);
    delete d;
}

/*!
    \since 5.9

    Returns whether the command is obsolete.

    The boolean is used for the automatic removal of commands that are not necessary in the
    stack anymore. The isObsolete function is checked in the functions QUndoStack::push(),
    QUndoStack::undo(), QUndoStack::redo(), and QUndoStack::setIndex().

    \sa setObsolete(), mergeWith(), QUndoStack::push(), QUndoStack::undo(), QUndoStack::redo()
*/

bool QUndoCommand::isObsolete() const
{
    return d->obsolete;
}

/*!
    \since 5.9

    Sets whether the command is obsolete to \a obsolete.

    \sa isObsolete(), mergeWith(), QUndoStack::push(), QUndoStack::undo(), QUndoStack::redo()
*/

void QUndoCommand::setObsolete(bool obsolete)
{
    d->obsolete = obsolete;
}

/*!
    Returns the ID of this command.

    A command ID is used in command compression. It must be an integer unique to
    this command's class, or -1 if the command doesn't support compression.

    If the command supports compression this function must be overridden in the
    derived class to return the correct ID. The base implementation returns -1.

    QUndoStack::push() will only try to merge two commands if they have the
    same ID, and the ID is not -1.

    \sa mergeWith(), QUndoStack::push()
*/

int QUndoCommand::id() const
{
    return -1;
}

/*!
    Attempts to merge this command with \a command. Returns \c true on
    success; otherwise returns \c false.

    If this function returns \c true, calling this command's redo() must have the same
    effect as redoing both this command and \a command.
    Similarly, calling this command's undo() must have the same effect as undoing
    \a command and this command.

    QUndoStack will only try to merge two commands if they have the same id, and
    the id is not -1.

    The default implementation returns \c false.

    \snippet code/src_gui_util_qundostack.cpp 3

    \sa id(), QUndoStack::push()
*/

bool QUndoCommand::mergeWith(const QUndoCommand *command)
{
    Q_UNUSED(command);
    return false;
}

/*!
    Applies a change to the document. This function must be implemented in
    the derived class. Calling QUndoStack::push(),
    QUndoStack::undo() or QUndoStack::redo() from this function leads to
    undefined beahavior.

    The default implementation calls redo() on all child commands.

    \sa undo()
*/

void QUndoCommand::redo()
{
    for (int i = 0; i < d->child_list.size(); ++i)
        d->child_list.at(i)->redo();
}

/*!
    Reverts a change to the document. After undo() is called, the state of
    the document should be the same as before redo() was called. This function must
    be implemented in the derived class. Calling QUndoStack::push(),
    QUndoStack::undo() or QUndoStack::redo() from this function leads to
    undefined beahavior.

    The default implementation calls undo() on all child commands in reverse order.

    \sa redo()
*/

void QUndoCommand::undo()
{
    for (int i = d->child_list.size() - 1; i >= 0; --i)
        d->child_list.at(i)->undo();
}

/*!
    Returns a short text string describing what this command does; for example,
    "insert text".

    The text is used for names of items in QUndoView.

    \sa actionText(), setText(), QUndoStack::createUndoAction(), QUndoStack::createRedoAction()
*/

QString QUndoCommand::text() const
{
    return d->text;
}

/*!
    \since 4.8

    Returns a short text string describing what this command does; for example,
    "insert text".

    The text is used when the text properties of the stack's undo and redo
    actions are updated.

    \sa text(), setText(), QUndoStack::createUndoAction(), QUndoStack::createRedoAction()
*/

QString QUndoCommand::actionText() const
{
    return d->actionText;
}

/*!
    Sets the command's text to be the \a text specified.

    The specified text should be a short user-readable string describing what this
    command does.

    If you need to have two different strings for text() and actionText(), separate
    them with "\\n" and pass into this function. Even if you do not use this feature
    for English strings during development, you can still let translators use two
    different strings in order to match specific languages' needs.
    The described feature and the function actionText() are available since Qt 4.8.

    \sa text(), actionText(), QUndoStack::createUndoAction(), QUndoStack::createRedoAction()
*/

void QUndoCommand::setText(const QString &text)
{
    int cdpos = text.indexOf(u'\n');
    if (cdpos > 0) {
        d->text = text.left(cdpos);
        d->actionText = text.mid(cdpos + 1);
    } else {
        d->text = text;
        d->actionText = text;
    }
}

/*!
    \since 4.4

    Returns the number of child commands in this command.

    \sa child()
*/

int QUndoCommand::childCount() const
{
    return d->child_list.size();
}

/*!
    \since 4.4

    Returns the child command at \a index.

    \sa childCount(), QUndoStack::command()
*/

const QUndoCommand *QUndoCommand::child(int index) const
{
    if (index < 0 || index >= d->child_list.size())
        return nullptr;
    return d->child_list.at(index);
}

#if QT_CONFIG(undostack)

/*!
    \class QUndoStack
    \brief The QUndoStack class is a stack of QUndoCommand objects.
    \since 4.2

    \inmodule QtGui

    For an overview of Qt's Undo Framework, see the
    \l{Overview of Qt's Undo Framework}{overview document}.

    An undo stack maintains a stack of commands that have been applied to a
    document.

    New commands are pushed on the stack using push(). Commands can be
    undone and redone using undo() and redo(), or by triggering the
    actions returned by createUndoAction() and createRedoAction().

    QUndoStack keeps track of the \a current command. This is the command
    which will be executed by the next call to redo(). The index of this
    command is returned by index(). The state of the edited object can be
    rolled forward or back using setIndex(). If the top-most command on the
    stack has already been redone, index() is equal to count().

    QUndoStack provides support for undo and redo actions, command
    compression, command macros, and supports the concept of a
    \e{clean state}.

    \section1 Undo and Redo Actions

    QUndoStack provides convenient undo and redo QAction objects, which
    can be inserted into a menu or a toolbar. When commands are undone or
    redone, QUndoStack updates the text properties of these actions
    to reflect what change they will trigger. The actions are also disabled
    when no command is available for undo or redo. These actions
    are returned by QUndoStack::createUndoAction() and QUndoStack::createRedoAction().

    \section1 Command Compression and Macros

    Command compression is useful when several commands can be compressed
    into a single command that can be undone and redone in a single operation.
    For example, when a user types a character in a text editor, a new command
    is created. This command inserts the character into the document at the
    cursor position. However, it is more convenient for the user to be able
    to undo or redo typing of whole words, sentences, or paragraphs.
    Command compression allows these single-character commands to be merged
    into a single command which inserts or deletes sections of text.
    For more information, see QUndoCommand::mergeWith() and push().

    A command macro is a sequence of commands, all of which are undone and
    redone in one go. Command macros are created by giving a command a list
    of child commands.
    Undoing or redoing the parent command will cause the child commands to
    be undone or redone. Command macros may be created explicitly
    by specifying a parent in the QUndoCommand constructor, or by using the
    convenience functions beginMacro() and endMacro().

    Although command compression and macros appear to have the same effect to the
    user, they often have different uses in an application. Commands that
    perform small changes to a document may be usefully compressed if there is
    no need to individually record them, and if only larger changes are relevant
    to the user.
    However, for commands that need to be recorded individually, or those that
    cannot be compressed, it is useful to use macros to provide a more convenient
    user experience while maintaining a record of each command.

    \section1 Clean State

    QUndoStack supports the concept of a clean state. When the
    document is saved to disk, the stack can be marked as clean using
    setClean(). Whenever the stack returns to this state through undoing and
    redoing commands, it emits the signal cleanChanged(). This signal
    is also emitted when the stack leaves the clean state. This signal is
    usually used to enable and disable the save actions in the application,
    and to update the document's title to reflect that it contains unsaved
    changes.

    \section1 Obsolete Commands

    QUndoStack is able to delete commands from the stack if the command is no
    longer needed. One example may be to delete a command when two commands are
    merged together in such a way that the merged command has no function. This
    can be seen with move commands where the user moves their mouse to one part
    of the screen and then moves it to the original position. The merged command
    results in a mouse movement of 0. This command can be deleted since it serves
    no purpose. Another example is with networking commands that fail due to connection
    issues. In this case, the command is to be removed from the stack because the redo()
    and undo() functions have no function since there was connection issues.

    A command can be marked obsolete with the QUndoCommand::setObsolete() function.
    The QUndoCommand::isObsolete() flag is checked in QUndoStack::push(),
    QUndoStack::undo(), QUndoStack::redo(), and QUndoStack::setIndex() after calling
    QUndoCommand::undo(), QUndoCommand::redo() and QUndoCommand:mergeWith() where
    applicable.

    If a command is set obsolete and the clean index is greater than or equal to the
    current command index, then the clean index will be reset when the command is
    deleted from the stack.

    \sa QUndoCommand, QUndoView
*/

/*! \internal
    Sets the current index to \a idx, emitting appropriate signals. If \a clean is true,
    makes \a idx the clean index as well.
*/

void QUndoStackPrivate::setIndex(int idx, bool clean)
{
    Q_Q(QUndoStack);

    bool was_clean = index == clean_index;

    if (idx != index) {
        index = idx;
        emit q->indexChanged(index);
        emit q->canUndoChanged(q->canUndo());
        emit q->undoTextChanged(q->undoText());
        emit q->canRedoChanged(q->canRedo());
        emit q->redoTextChanged(q->redoText());
    }

    if (clean)
        clean_index = index;

    bool is_clean = index == clean_index;
    if (is_clean != was_clean)
        emit q->cleanChanged(is_clean);
}

/*! \internal
    If the number of commands on the stack exceedes the undo limit, deletes commands from
    the bottom of the stack.

    Returns \c true if commands were deleted.
*/

bool QUndoStackPrivate::checkUndoLimit()
{
    if (undo_limit <= 0 || !macro_stack.isEmpty() || undo_limit >= command_list.size())
        return false;

    int del_count = command_list.size() - undo_limit;

    for (int i = 0; i < del_count; ++i)
        delete command_list.takeFirst();

    index -= del_count;
    if (clean_index != -1) {
        if (clean_index < del_count)
            clean_index = -1; // we've deleted the clean command
        else
            clean_index -= del_count;
    }

    return true;
}

/*!
    Constructs an empty undo stack with the parent \a parent. The
    stack will initially be in the clean state. If \a parent is a
    QUndoGroup object, the stack is automatically added to the group.

    \sa push()
*/

QUndoStack::QUndoStack(QObject *parent)
    : QObject(*(new QUndoStackPrivate), parent)
{
#if QT_CONFIG(undogroup)
    if (QUndoGroup *group = qobject_cast<QUndoGroup*>(parent))
        group->addStack(this);
#endif
}

/*!
    Destroys the undo stack, deleting any commands that are on it. If the
    stack is in a QUndoGroup, the stack is automatically removed from the group.

    \sa QUndoStack()
*/

QUndoStack::~QUndoStack()
{
#if QT_CONFIG(undogroup)
    Q_D(QUndoStack);
    if (d->group != nullptr)
        d->group->removeStack(this);
#endif
    clear();
}

/*!
    Clears the command stack by deleting all commands on it, and returns the stack
    to the clean state.

    Commands are not undone or redone; the state of the edited object remains
    unchanged.

    This function is usually used when the contents of the document are
    abandoned.

    \sa QUndoStack()
*/

void QUndoStack::clear()
{
    Q_D(QUndoStack);

    if (d->command_list.isEmpty())
        return;

    bool was_clean = isClean();

    d->macro_stack.clear();
    qDeleteAll(d->command_list);
    d->command_list.clear();

    d->index = 0;
    d->clean_index = 0;

    emit indexChanged(0);
    emit canUndoChanged(false);
    emit undoTextChanged(QString());
    emit canRedoChanged(false);
    emit redoTextChanged(QString());

    if (!was_clean)
        emit cleanChanged(true);
}

/*!
    Pushes \a cmd on the stack or merges it with the most recently executed command.
    In either case, executes \a cmd by calling its redo() function.

    If \a cmd's id is not -1, and if the id is the same as that of the
    most recently executed command, QUndoStack will attempt to merge the two
    commands by calling QUndoCommand::mergeWith() on the most recently executed
    command. If QUndoCommand::mergeWith() returns \c true, \a cmd is deleted.

    After calling QUndoCommand::redo() and, if applicable, QUndoCommand::mergeWith(),
    QUndoCommand::isObsolete() will be called for \a cmd or the merged command.
    If QUndoCommand::isObsolete() returns \c true, then \a cmd or the merged command
    will be deleted from the stack.

    In all other cases \a cmd is simply pushed on the stack.

    If commands were undone before \a cmd was pushed, the current command and
    all commands above it are deleted. Hence \a cmd always ends up being the
    top-most on the stack.

    Once a command is pushed, the stack takes ownership of it. There
    are no getters to return the command, since modifying it after it has
    been executed will almost always lead to corruption of the document's
    state.

    \sa QUndoCommand::id(), QUndoCommand::mergeWith()
*/

void QUndoStack::push(QUndoCommand *cmd)
{
    Q_D(QUndoStack);
    if (!cmd->isObsolete())
        cmd->redo();

    bool macro = !d->macro_stack.isEmpty();

    QUndoCommand *cur = nullptr;
    if (macro) {
        QUndoCommand *macro_cmd = d->macro_stack.constLast();
        if (!macro_cmd->d->child_list.isEmpty())
            cur = macro_cmd->d->child_list.constLast();
    } else {
        if (d->index > 0)
            cur = d->command_list.at(d->index - 1);
        while (d->index < d->command_list.size())
            delete d->command_list.takeLast();
        if (d->clean_index > d->index)
            d->clean_index = -1; // we've deleted the clean state
    }

    bool try_merge = cur != nullptr
                        && cur->id() != -1
                        && cur->id() == cmd->id()
                        && (macro || d->index != d->clean_index);

    if (try_merge && cur->mergeWith(cmd)) {
        delete cmd;

        if (macro) {
            if (cur->isObsolete())
                delete d->macro_stack.constLast()->d->child_list.takeLast();
        } else {
            if (cur->isObsolete()) {
                delete d->command_list.takeLast();

                d->setIndex(d->index - 1, false);
            } else {
                emit indexChanged(d->index);
                emit canUndoChanged(canUndo());
                emit undoTextChanged(undoText());
                emit canRedoChanged(canRedo());
                emit redoTextChanged(redoText());
            }
        }
    } else if (cmd->isObsolete()) {
        delete cmd; // command should be deleted and NOT added to the stack
    } else {
        if (macro) {
            d->macro_stack.constLast()->d->child_list.append(cmd);
        } else {
            d->command_list.append(cmd);
            d->checkUndoLimit();
            d->setIndex(d->index + 1, false);
        }
    }
}

/*!
    Marks the stack as clean and emits cleanChanged() if the stack was
    not already clean.

    This is typically called when a document is saved, for example.

    Whenever the stack returns to this state through the use of undo/redo
    commands, it emits the signal cleanChanged(). This signal is also
    emitted when the stack leaves the clean state.

    \sa isClean(), resetClean(), cleanIndex()
*/

void QUndoStack::setClean()
{
    Q_D(QUndoStack);
    if (Q_UNLIKELY(!d->macro_stack.isEmpty())) {
        qWarning("QUndoStack::setClean(): cannot set clean in the middle of a macro");
        return;
    }

    d->setIndex(d->index, true);
}

/*!
    \since 5.8

    Leaves the clean state and emits cleanChanged() if the stack was clean.
    This method resets the clean index to -1.

    This is typically called in the following cases, when a document has been:
    \list
    \li created basing on some template and has not been saved,
        so no filename has been associated with the document yet.
    \li restored from a backup file.
    \li changed outside of the editor and the user did not reload it.
    \endlist

    \sa isClean(), setClean(), cleanIndex()
*/

void QUndoStack::resetClean()
{
    Q_D(QUndoStack);
    const bool was_clean = isClean();
    d->clean_index = -1;
    if (was_clean)
        emit cleanChanged(false);
}

/*!
    \since 5.12
    \property QUndoStack::clean
    \brief the clean status of this stack.

    This property indicates whether or not the stack is clean. For example, a
    stack is clean when a document has been saved.

    \sa isClean(), setClean(), resetClean(), cleanIndex()
*/

/*!
    If the stack is in the clean state, returns \c true; otherwise returns \c false.

    \sa setClean(), cleanIndex()
*/

bool QUndoStack::isClean() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return false;
    return d->clean_index == d->index;
}

/*!
    Returns the clean index. This is the index at which setClean() was called.

    A stack may not have a clean index. This happens if a document is saved,
    some commands are undone, then a new command is pushed. Since
    push() deletes all the undone commands before pushing the new command, the stack
    can't return to the clean state again. In this case, this function returns -1.
    The -1 may also be returned after an explicit call to resetClean().

    \sa isClean(), setClean()
*/

int QUndoStack::cleanIndex() const
{
    Q_D(const QUndoStack);
    return d->clean_index;
}

/*!
    Undoes the command below the current command by calling QUndoCommand::undo().
    Decrements the current command index.

    If the stack is empty, or if the bottom command on the stack has already been
    undone, this function does nothing.

    After the command is undone, if QUndoCommand::isObsolete() returns \c true,
    then the command will be deleted from the stack. Additionally, if the clean
    index is greater than or equal to the current command index, then the clean
    index is reset.

    \sa redo(), index()
*/

void QUndoStack::undo()
{
    Q_D(QUndoStack);
    if (d->index == 0)
        return;

    if (Q_UNLIKELY(!d->macro_stack.isEmpty())) {
        qWarning("QUndoStack::undo(): cannot undo in the middle of a macro");
        return;
    }

    int idx = d->index - 1;
    QUndoCommand *cmd = d->command_list.at(idx);

    if (!cmd->isObsolete())
        cmd->undo();

    if (cmd->isObsolete()) { // A separate check is done b/c the undo command may set obsolete flag
        delete d->command_list.takeAt(idx);

        if (d->clean_index > idx)
            resetClean();
    }

    d->setIndex(idx, false);
}

/*!
    Redoes the current command by calling QUndoCommand::redo(). Increments the current
    command index.

    If the stack is empty, or if the top command on the stack has already been
    redone, this function does nothing.

    If QUndoCommand::isObsolete() returns true for the current command, then
    the command will be deleted from the stack. Additionally, if the clean
    index is greater than or equal to the current command index, then the clean
    index is reset.

    \sa undo(), index()
*/

void QUndoStack::redo()
{
    Q_D(QUndoStack);
    if (d->index == d->command_list.size())
        return;

    if (Q_UNLIKELY(!d->macro_stack.isEmpty())) {
        qWarning("QUndoStack::redo(): cannot redo in the middle of a macro");
        return;
    }

    int idx = d->index;
    QUndoCommand *cmd = d->command_list.at(idx);

    if (!cmd->isObsolete())
        cmd->redo(); // A separate check is done b/c the undo command may set obsolete flag

    if (cmd->isObsolete()) {
        delete d->command_list.takeAt(idx);

        if (d->clean_index > idx)
            resetClean();
    } else {
        d->setIndex(d->index + 1, false);
    }
}

/*!
    Returns the number of commands on the stack. Macro commands are counted as
    one command.

    \sa index(), setIndex(), command()
*/

int QUndoStack::count() const
{
    Q_D(const QUndoStack);
    return d->command_list.size();
}

/*!
    Returns the index of the current command. This is the command that will be
    executed on the next call to redo(). It is not always the top-most command
    on the stack, since a number of commands may have been undone.

    \sa undo(), redo(), count()
*/

int QUndoStack::index() const
{
    Q_D(const QUndoStack);
    return d->index;
}

/*!
    Repeatedly calls undo() or redo() until the current command index reaches
    \a idx. This function can be used to roll the state of the document forwards
    or backwards. indexChanged() is emitted only once.

    \sa index(), count(), undo(), redo()
*/

void QUndoStack::setIndex(int idx)
{
    Q_D(QUndoStack);
    if (Q_UNLIKELY(!d->macro_stack.isEmpty())) {
        qWarning("QUndoStack::setIndex(): cannot set index in the middle of a macro");
        return;
    }

    if (idx < 0)
        idx = 0;
    else if (idx > d->command_list.size())
        idx = d->command_list.size();

    int i = d->index;
    while (i < idx) {
        QUndoCommand *cmd = d->command_list.at(i);

        if (!cmd->isObsolete())
            cmd->redo();  // A separate check is done b/c the undo command may set obsolete flag

        if (cmd->isObsolete()) {
            delete d->command_list.takeAt(i);

            if (d->clean_index > i)
                resetClean();

            idx--; // Subtract from idx because we removed a command
        } else {
            i++;
        }
    }

    while (i > idx) {
        QUndoCommand *cmd = d->command_list.at(--i);

        cmd->undo();
        if (cmd->isObsolete()) {
            delete d->command_list.takeAt(i);

            if (d->clean_index > i)
                resetClean();
        }
    }

    d->setIndex(idx, false);
}

/*!
    \since 5.12
    \property QUndoStack::canUndo
    \brief whether this stack can undo.

    This property indicates whether or not there is a command that can be
    undone.

    \sa canUndo(), index(), canRedo()
*/

/*!
    Returns \c true if there is a command available for undo; otherwise returns \c false.

    This function returns \c false if the stack is empty, or if the bottom command
    on the stack has already been undone.

    Synonymous with index() == 0.

    \sa index(), canRedo()
*/

bool QUndoStack::canUndo() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return false;
    return d->index > 0;
}

/*!
    \since 5.12
    \property QUndoStack::canRedo
    \brief whether this stack can redo.

    This property indicates whether or not there is a command that can be
    redone.

    \sa canRedo(), index(), canUndo()
*/

/*!
    Returns \c true if there is a command available for redo; otherwise returns \c false.

    This function returns \c false if the stack is empty or if the top command
    on the stack has already been redone.

    Synonymous with index() == count().

    \sa index(), canUndo()
*/

bool QUndoStack::canRedo() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return false;
    return d->index < d->command_list.size();
}

/*!
    \since 5.12
    \property QUndoStack::undoText
    \brief the undo text of the next command that is undone.

    This property holds the text of the command which will be undone in the
    next call to undo().

    \sa undoText(), QUndoCommand::actionText(), redoText()
*/

/*!
    Returns the text of the command which will be undone in the next call to undo().

    \sa QUndoCommand::actionText(), redoText()
*/

QString QUndoStack::undoText() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return QString();
    if (d->index > 0)
        return d->command_list.at(d->index - 1)->actionText();
    return QString();
}

/*!
    \since 5.12
    \property QUndoStack::redoText
    \brief the redo text of the next command that is redone.

    This property holds the text of the command which will be redone in the
    next call to redo().

    \sa redoText(), QUndoCommand::actionText(), undoText()
*/

/*!
    Returns the text of the command which will be redone in the next call to redo().

    \sa QUndoCommand::actionText(), undoText()
*/

QString QUndoStack::redoText() const
{
    Q_D(const QUndoStack);
    if (!d->macro_stack.isEmpty())
        return QString();
    if (d->index < d->command_list.size())
        return d->command_list.at(d->index)->actionText();
    return QString();
}

#ifndef QT_NO_ACTION

/*!
    \internal

    Sets the text property of \a action to \a text, applying \a prefix, and falling back to \a defaultText if \a text is empty.
*/
void QUndoStackPrivate::setPrefixedText(QAction *action, const QString &prefix, const QString &defaultText, const QString &text)
{
    if (defaultText.isEmpty()) {
        QString s = prefix;
        if (!prefix.isEmpty() && !text.isEmpty())
            s.append(u' ');
        s.append(text);
        action->setText(s);
    } else {
        if (text.isEmpty())
            action->setText(defaultText);
        else
            action->setText(prefix.arg(text));
    }
};

/*!
    Creates an undo QAction object with the given \a parent.

    Triggering this action will cause a call to undo(). The text of this action
    is the text of the command which will be undone in the next call to undo(),
    prefixed by the specified \a prefix. If there is no command available for undo,
    this action will be disabled.

    If \a prefix is empty, the default template "Undo %1" is used instead of prefix.
    Before Qt 4.8, the prefix "Undo" was used by default.

    \sa createRedoAction(), canUndo(), QUndoCommand::text()
*/

QAction *QUndoStack::createUndoAction(QObject *parent, const QString &prefix) const
{
    QAction *action = new QAction(parent);
    action->setEnabled(canUndo());

    QString effectivePrefix = prefix;
    QString defaultText;
    if (prefix.isEmpty()) {
        effectivePrefix = tr("Undo %1");
        defaultText = tr("Undo", "Default text for undo action");
    }

    QUndoStackPrivate::setPrefixedText(action, effectivePrefix, defaultText, undoText());

    connect(this, &QUndoStack::canUndoChanged, action, &QAction::setEnabled);
    connect(this, &QUndoStack::undoTextChanged, action, [=](const QString &text) {
        QUndoStackPrivate::setPrefixedText(action, effectivePrefix, defaultText, text);
    });
    connect(action, &QAction::triggered, this, &QUndoStack::undo);

    return action;
}

/*!
    Creates an redo QAction object with the given \a parent.

    Triggering this action will cause a call to redo(). The text of this action
    is the text of the command which will be redone in the next call to redo(),
    prefixed by the specified \a prefix. If there is no command available for redo,
    this action will be disabled.

    If \a prefix is empty, the default template "Redo %1" is used instead of prefix.
    Before Qt 4.8, the prefix "Redo" was used by default.

    \sa createUndoAction(), canRedo(), QUndoCommand::text()
*/

QAction *QUndoStack::createRedoAction(QObject *parent, const QString &prefix) const
{
    QAction *action = new QAction(parent);
    action->setEnabled(canRedo());

    QString effectivePrefix = prefix;
    QString defaultText;
    if (prefix.isEmpty()) {
        effectivePrefix = tr("Redo %1");
        defaultText = tr("Redo", "Default text for redo action");
    }

    QUndoStackPrivate::setPrefixedText(action, effectivePrefix, defaultText, redoText());

    connect(this, &QUndoStack::canRedoChanged, action, &QAction::setEnabled);
    connect(this, &QUndoStack::redoTextChanged, action, [=](const QString &text) {
        QUndoStackPrivate::setPrefixedText(action, effectivePrefix, defaultText, text);
    });
    connect(action, &QAction::triggered, this, &QUndoStack::redo);

    return action;
}

#endif // QT_NO_ACTION

/*!
    Begins composition of a macro command with the given \a text description.

    An empty command described by the specified \a text is pushed on the stack.
    Any subsequent commands pushed on the stack will be appended to the empty
    command's children until endMacro() is called.

    Calls to beginMacro() and endMacro() may be nested, but every call to
    beginMacro() must have a matching call to endMacro().

    While a macro is being composed, the stack is disabled. This means that:
    \list
    \li indexChanged() and cleanChanged() are not emitted,
    \li canUndo() and canRedo() return false,
    \li calling undo() or redo() has no effect,
    \li the undo/redo actions are disabled.
    \endlist

    The stack becomes enabled and appropriate signals are emitted when endMacro()
    is called for the outermost macro.

    \snippet code/src_gui_util_qundostack.cpp 4

    This code is equivalent to:

    \snippet code/src_gui_util_qundostack.cpp 5

    \sa endMacro()
*/

void QUndoStack::beginMacro(const QString &text)
{
    Q_D(QUndoStack);
    QUndoCommand *cmd = new QUndoCommand();
    cmd->setText(text);

    if (d->macro_stack.isEmpty()) {
        while (d->index < d->command_list.size())
            delete d->command_list.takeLast();
        if (d->clean_index > d->index)
            d->clean_index = -1; // we've deleted the clean state
        d->command_list.append(cmd);
    } else {
        d->macro_stack.constLast()->d->child_list.append(cmd);
    }
    d->macro_stack.append(cmd);

    if (d->macro_stack.size() == 1) {
        emit canUndoChanged(false);
        emit undoTextChanged(QString());
        emit canRedoChanged(false);
        emit redoTextChanged(QString());
    }
}

/*!
    Ends composition of a macro command.

    If this is the outermost macro in a set nested macros, this function emits
    indexChanged() once for the entire macro command.

    \sa beginMacro()
*/

void QUndoStack::endMacro()
{
    Q_D(QUndoStack);
    if (Q_UNLIKELY(d->macro_stack.isEmpty())) {
        qWarning("QUndoStack::endMacro(): no matching beginMacro()");
        return;
    }

    d->macro_stack.removeLast();

    if (d->macro_stack.isEmpty()) {
        d->checkUndoLimit();
        d->setIndex(d->index + 1, false);
    }
}

/*!
  \since 4.4

  Returns a const pointer to the command at \a index.

  This function returns a const pointer, because modifying a command,
  once it has been pushed onto the stack and executed, almost always
  causes corruption of the state of the document, if the command is
  later undone or redone.

  \sa QUndoCommand::child()
*/
const QUndoCommand *QUndoStack::command(int index) const
{
    Q_D(const QUndoStack);

    if (index < 0 || index >= d->command_list.size())
        return nullptr;
    return d->command_list.at(index);
}

/*!
    Returns the text of the command at index \a idx.

    \sa beginMacro()
*/

QString QUndoStack::text(int idx) const
{
    Q_D(const QUndoStack);

    if (idx < 0 || idx >= d->command_list.size())
        return QString();
    return d->command_list.at(idx)->text();
}

/*!
    \property QUndoStack::undoLimit
    \brief the maximum number of commands on this stack.
    \since 4.3

    When the number of commands on a stack exceedes the stack's undoLimit, commands are
    deleted from the bottom of the stack. Macro commands (commands with child commands)
    are treated as one command. The default value is 0, which means that there is no
    limit.

    This property may only be set when the undo stack is empty, since setting it on a
    non-empty stack might delete the command at the current index. Calling setUndoLimit()
    on a non-empty stack prints a warning and does nothing.
*/

void QUndoStack::setUndoLimit(int limit)
{
    Q_D(QUndoStack);

    if (Q_UNLIKELY(!d->command_list.isEmpty())) {
        qWarning("QUndoStack::setUndoLimit(): an undo limit can only be set when the stack is empty");
        return;
    }

    if (limit == d->undo_limit)
        return;
    d->undo_limit = limit;
    d->checkUndoLimit();
}

int QUndoStack::undoLimit() const
{
    Q_D(const QUndoStack);

    return d->undo_limit;
}

/*!
    \property QUndoStack::active
    \brief the active status of this stack.

    An application often has multiple undo stacks, one for each opened document. The active
    stack is the one associated with the currently active document. If the stack belongs
    to a QUndoGroup, calls to QUndoGroup::undo() or QUndoGroup::redo() will be forwarded
    to this stack when it is active. If the QUndoGroup is watched by a QUndoView, the view
    will display the contents of this stack when it is active. If the stack does not belong to
    a QUndoGroup, making it active has no effect.

    It is the programmer's responsibility to specify which stack is active by
    calling setActive(), usually when the associated document window receives focus.

    \sa QUndoGroup
*/

void QUndoStack::setActive(bool active)
{
#if !QT_CONFIG(undogroup)
    Q_UNUSED(active);
#else
    Q_D(QUndoStack);

    if (d->group != nullptr) {
        if (active)
            d->group->setActiveStack(this);
        else if (d->group->activeStack() == this)
            d->group->setActiveStack(nullptr);
    }
#endif
}

bool QUndoStack::isActive() const
{
#if !QT_CONFIG(undogroup)
    return true;
#else
    Q_D(const QUndoStack);
    return d->group == nullptr || d->group->activeStack() == this;
#endif
}

/*!
    \fn void QUndoStack::indexChanged(int idx)

    This signal is emitted whenever a command modifies the state of the document.
    This happens when a command is undone or redone. When a macro
    command is undone or redone, or setIndex() is called, this signal
    is emitted only once.

    \a idx specifies the index of the current command, ie. the command which will be
    executed on the next call to redo().

    \sa index(), setIndex()
*/

/*!
    \fn void QUndoStack::cleanChanged(bool clean)

    This signal is emitted whenever the stack enters or leaves the clean state.
    If \a clean is true, the stack is in a clean state; otherwise this signal
    indicates that it has left the clean state.

    \sa isClean(), setClean()
*/

/*!
    \fn void QUndoStack::undoTextChanged(const QString &undoText)

    This signal is emitted whenever the value of undoText() changes. It is
    used to update the text property of the undo action returned by createUndoAction().
    \a undoText specifies the new text.
*/

/*!
    \fn void QUndoStack::canUndoChanged(bool canUndo)

    This signal is emitted whenever the value of canUndo() changes. It is
    used to enable or disable the undo action returned by createUndoAction().
    \a canUndo specifies the new value.
*/

/*!
    \fn void QUndoStack::redoTextChanged(const QString &redoText)

    This signal is emitted whenever the value of redoText() changes. It is
    used to update the text property of the redo action returned by createRedoAction().
    \a redoText specifies the new text.
*/

/*!
    \fn void QUndoStack::canRedoChanged(bool canRedo)

    This signal is emitted whenever the value of canRedo() changes. It is
    used to enable or disable the redo action returned by createRedoAction().
    \a canRedo specifies the new value.
*/

QT_END_NAMESPACE

#include "moc_qundostack.cpp"

#endif // QT_CONFIG(undostack)
