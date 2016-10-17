/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QAction>
#include <QUndoStack>

/******************************************************************************
** Commands
*/

class InsertCommand : public QUndoCommand
{
public:
    InsertCommand(QString *str, int idx, const QString &text,
                    QUndoCommand *parent = 0);

    virtual void undo();
    virtual void redo();

private:
    QString *m_str;
    int m_idx;
    QString m_text;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(QString *str, int idx, int len, QUndoCommand *parent = 0);

    virtual void undo();
    virtual void redo();

private:
    QString *m_str;
    int m_idx;
    QString m_text;
};

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(QString *str, const QString &text, bool _fail_merge = false,
                    QUndoCommand *parent = 0);
    ~AppendCommand();

    virtual void undo();
    virtual void redo();
    virtual int id() const;
    virtual bool mergeWith(const QUndoCommand *other);

    bool merged;
    bool fail_merge;
    static int delete_cnt;

private:
    QString *m_str;
    QString m_text;
};

class IdleCommand : public QUndoCommand
{
public:
    IdleCommand(QUndoCommand *parent = 0);
    ~IdleCommand();

    virtual void undo();
    virtual void redo();
};

InsertCommand::InsertCommand(QString *str, int idx, const QString &text,
                            QUndoCommand *parent)
    : QUndoCommand(parent)
{
    QVERIFY(str->length() >= idx);

    setText("insert");

    m_str = str;
    m_idx = idx;
    m_text = text;
}

void InsertCommand::redo()
{
    QVERIFY(m_str->length() >= m_idx);

    m_str->insert(m_idx, m_text);
}

void InsertCommand::undo()
{
    QCOMPARE(m_str->mid(m_idx, m_text.length()), m_text);

    m_str->remove(m_idx, m_text.length());
}

RemoveCommand::RemoveCommand(QString *str, int idx, int len, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    QVERIFY(str->length() >= idx + len);

    setText("remove");

    m_str = str;
    m_idx = idx;
    m_text = m_str->mid(m_idx, len);
}

void RemoveCommand::redo()
{
    QCOMPARE(m_str->mid(m_idx, m_text.length()), m_text);

    m_str->remove(m_idx, m_text.length());
}

void RemoveCommand::undo()
{
    QVERIFY(m_str->length() >= m_idx);

    m_str->insert(m_idx, m_text);
}

int AppendCommand::delete_cnt = 0;

AppendCommand::AppendCommand(QString *str, const QString &text, bool _fail_merge,
                                QUndoCommand *parent)
    : QUndoCommand(parent)
{
    setText("append");

    m_str = str;
    m_text = text;
    merged = false;
    fail_merge = _fail_merge;
}

AppendCommand::~AppendCommand()
{
    ++delete_cnt;
}

void AppendCommand::redo()
{
    m_str->append(m_text);
}

void AppendCommand::undo()
{
    QCOMPARE(m_str->mid(m_str->length() - m_text.length()), m_text);

    m_str->truncate(m_str->length() - m_text.length());
}

int AppendCommand::id() const
{
    return 1;
}

bool AppendCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id())
        return false;
    if (fail_merge)
        return false;
    m_text += static_cast<const AppendCommand*>(other)->m_text;
    merged = true;
    return true;
}

IdleCommand::IdleCommand(QUndoCommand *parent)
    : QUndoCommand(parent)
{
    // "idle-item" goes to QUndoStack::{redo,undo}Text
    // "idle-action" goes to all other places (e.g. QUndoView)
    setText("idle-item\nidle-action");
}

IdleCommand::~IdleCommand()
{
}

void IdleCommand::redo()
{
}

void IdleCommand::undo()
{
}

/******************************************************************************
** tst_QUndoStack
*/

class tst_QUndoStack : public QObject
{
    Q_OBJECT
public:
    tst_QUndoStack();

private slots:
    void undoRedo();
    void setIndex();
    void setClean();
    void clear();
    void childCommand();
    void macroBeginEnd();
    void compression();
    void undoLimit();
    void commandTextFormat();
    void separateUndoText();
};

tst_QUndoStack::tst_QUndoStack()
{
}

static QString glue(const QString &s1, const QString &s2)
{
    QString result;

    result.append(s1);
    if (!s1.isEmpty() && !s2.isEmpty())
        result.append(' ');
    result.append(s2);

    return result;
}

static void checkState(QSignalSpy &redoTextChangedSpy,
                       QSignalSpy &canRedoChangedSpy,
                       QSignalSpy &undoTextChangedSpy,
                       const QScopedPointer<QAction> &redoAction,
                       const QScopedPointer<QAction> &undoAction,
                       QSignalSpy &canUndoChangedSpy,
                       QSignalSpy &cleanChangedSpy,
                       QSignalSpy &indexChangedSpy,
                       QUndoStack &stack,
                       const bool _clean,
                       const int _count,
                       const int _index,
                       const bool _canUndo,
                       const QString &_undoText,
                       const bool _canRedo,
                       const QString &_redoText,
                       const bool _cleanChanged,
                       const bool _indexChanged,
                       const bool _undoChanged,
                       const bool _redoChanged)
{
    QCOMPARE(stack.count(), _count);
    QCOMPARE(stack.isClean(), _clean);
    QCOMPARE(stack.index(), _index);
    QCOMPARE(stack.canUndo(), _canUndo);
    QCOMPARE(stack.undoText(), QString(_undoText));
    QCOMPARE(stack.canRedo(), _canRedo);
    QCOMPARE(stack.redoText(), QString(_redoText));
    if (_indexChanged) {
        QCOMPARE(indexChangedSpy.count(), 1);
        QCOMPARE(indexChangedSpy.at(0).at(0).toInt(), _index);
        indexChangedSpy.clear();
    } else {
        QCOMPARE(indexChangedSpy.count(), 0);
    }
    if (_cleanChanged) {
        QCOMPARE(cleanChangedSpy.count(), 1);
        QCOMPARE(cleanChangedSpy.at(0).at(0).toBool(), _clean);
        cleanChangedSpy.clear();
    } else {
        QCOMPARE(cleanChangedSpy.count(), 0);
    }
    if (_undoChanged) {
        QCOMPARE(canUndoChangedSpy.count(), 1);
        QCOMPARE(canUndoChangedSpy.at(0).at(0).toBool(), _canUndo);
        QCOMPARE(undoAction->isEnabled(), _canUndo);
        QCOMPARE(undoTextChangedSpy.count(), 1);
        QCOMPARE(undoTextChangedSpy.at(0).at(0).toString(), QString(_undoText));
        QCOMPARE(undoAction->text(), glue("foo", _undoText));
        canUndoChangedSpy.clear();
        undoTextChangedSpy.clear();
    } else {
        QCOMPARE(canUndoChangedSpy.count(), 0);
        QCOMPARE(undoTextChangedSpy.count(), 0);
    }
    if (_redoChanged) {
        QCOMPARE(canRedoChangedSpy.count(), 1);
        QCOMPARE(canRedoChangedSpy.at(0).at(0).toBool(), _canRedo);
        QCOMPARE(redoAction->isEnabled(), _canRedo);
        QCOMPARE(redoTextChangedSpy.count(), 1);
        QCOMPARE(redoTextChangedSpy.at(0).at(0).toString(), QString(_redoText));
        QCOMPARE(redoAction->text(), glue("bar", _redoText));
        canRedoChangedSpy.clear();
        redoTextChangedSpy.clear();
    } else {
        QCOMPARE(canRedoChangedSpy.count(), 0);
        QCOMPARE(redoTextChangedSpy.count(), 0);
    }
}

void tst_QUndoStack::undoRedo()
{
    QUndoStack stack;
    const QScopedPointer<QAction> undoAction(stack.createUndoAction(0, QString("foo")));
    const QScopedPointer<QAction> redoAction(stack.createRedoAction(0, QString("bar")));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    // push, undo, redo

    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.undo(); // nothing to undo
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new InsertCommand(&str, 2, "123"));
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged


    stack.undo();
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.redo();
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.redo(); // nothing to redo
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.undo();
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo(); // nothing to undo
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    // push after undo - check that undone commands get deleted

    stack.redo();
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new RemoveCommand(&str, 2, 2));
    QCOMPARE(str, QString("heo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count - still 2, last command got deleted
                2,          // index
                true,       // canUndo
                "remove",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "remove",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new InsertCommand(&str, 0, "goodbye"));
    QCOMPARE(str, QString("goodbye"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count - two commands got deleted
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
}

void tst_QUndoStack::setIndex()
{
    QUndoStack stack;
    const QScopedPointer<QAction> undoAction(stack.createUndoAction(0, QString("foo")));
    const QScopedPointer<QAction> redoAction(stack.createRedoAction(0, QString("bar")));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    stack.setIndex(10); // should do nothing
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.setIndex(0); // should do nothing
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.setIndex(-10); // should do nothing
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new InsertCommand(&str, 2, "123"));
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setIndex(2);
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.setIndex(0);
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setIndex(10); // should set index to 2
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setIndex(-10); // should set index to 0
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setIndex(1);
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setIndex(2);
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
}

void tst_QUndoStack::setClean()
{
    QUndoStack stack;
    const QScopedPointer<QAction> undoAction(stack.createUndoAction(0, QString("foo")));
    const QScopedPointer<QAction> redoAction(stack.createRedoAction(0, QString("bar")));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    QCOMPARE(stack.cleanIndex(), 0);
    stack.setClean();
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged
    QCOMPARE(stack.cleanIndex(), 0);

    stack.push(new InsertCommand(&str, 0, "goodbye"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), 0);

    stack.setClean();
    QCOMPARE(str, QString("goodbye"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.push(new AppendCommand(&str, " cowboy"));
    QCOMPARE(str, QString("goodbye cowboy"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.undo(); // reaching clean state from above
    QCOMPARE(str, QString("goodbye"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.undo();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.redo(); // reaching clean state from below
    QCOMPARE(str, QString("goodbye"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.undo();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.push(new InsertCommand(&str, 0, "foo")); // the clean state gets deleted!
    QCOMPARE(str, QString("foo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), -1);

    stack.undo();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), -1);

    stack.setClean();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged
    QCOMPARE(stack.cleanIndex(), 0);

    stack.resetClean();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged
    QCOMPARE(stack.cleanIndex(), -1);

    stack.redo();
    QCOMPARE(str, QString("foo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), -1);

    stack.setClean();
    QCOMPARE(str, QString("foo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.undo();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.resetClean();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                false,       // indexChanged
                false,       // undoChanged
                false);      // redoChanged
    QCOMPARE(stack.cleanIndex(), -1);
}

void tst_QUndoStack::clear()
{
    QUndoStack stack;
    const QScopedPointer<QAction> undoAction(stack.createUndoAction(0, QString("foo")));
    const QScopedPointer<QAction> redoAction(stack.createRedoAction(0, QString("bar")));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    stack.clear();
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new InsertCommand(&str, 2, "123"));
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.clear();
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    str.clear();
    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new InsertCommand(&str, 2, "123"));
    QCOMPARE(str, QString("he123llo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setIndex(0);
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.clear();
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
}

void tst_QUndoStack::childCommand()
{
    QUndoStack stack;
    const QScopedPointer<QAction> undoAction(stack.createUndoAction(0, QString("foo")));
    const QScopedPointer<QAction> redoAction(stack.createRedoAction(0, QString("bar")));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    QUndoCommand *cmd = new QUndoCommand();
    cmd->setText("ding");
    new InsertCommand(&str, 5, "world", cmd);
    new RemoveCommand(&str, 4, 1, cmd);
    stack.push(cmd);
    QCOMPARE(str, QString("hellworld"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "ding",     // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.redo();
    QCOMPARE(str, QString("hellworld"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
}

void tst_QUndoStack::macroBeginEnd()
{
    QUndoStack stack;
    const QScopedPointer<QAction> undoAction(stack.createUndoAction(0, QString("foo")));
    const QScopedPointer<QAction> redoAction(stack.createRedoAction(0, QString("bar")));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    stack.beginMacro("ding");
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setClean(); // should do nothing
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.undo(); // should do nothing
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.redo(); // should do nothing
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.setIndex(0); // should do nothing
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.endMacro();
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index - endMacro() increments index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new InsertCommand(&str, 0, "h"));
    QCOMPARE(str, QString("h"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new InsertCommand(&str, 1, "owdy"));
    QCOMPARE(str, QString("howdy"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setIndex(2);
    QCOMPARE(str, QString("h"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.beginMacro("dong"); // the "owdy" command gets deleted
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new InsertCommand(&str, 1, "ello"));
    QCOMPARE(str, QString("hello"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.push(new RemoveCommand(&str, 1, 2));
    QCOMPARE(str, QString("hlo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.beginMacro("dong2");
    QCOMPARE(str, QString("hlo"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.push(new RemoveCommand(&str, 1, 1));
    QCOMPARE(str, QString("ho"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.endMacro();
    QCOMPARE(str, QString("ho"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.endMacro();
    QCOMPARE(str, QString("ho"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "dong",     // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("h"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "insert",     // undoText
                true,       // canRedo
                "dong",     // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString(""));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                1,          // index
                true,       // canUndo
                "ding",     // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setIndex(3);
    QCOMPARE(str, QString("ho"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "dong",     // undoText
                false,      // canRedo
                "",         // redoText
                false,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setIndex(1);
    QCOMPARE(str, QString());
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                1,          // index
                true,       // canUndo
                "ding",     // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
}

void tst_QUndoStack::compression()
{
    QUndoStack stack;
    const QScopedPointer<QAction> undoAction(stack.createUndoAction(0, QString("foo")));
    const QScopedPointer<QAction> redoAction(stack.createRedoAction(0, QString("bar")));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    AppendCommand::delete_cnt = 0;

    stack.push(new InsertCommand(&str, 0, "ene"));
    QCOMPARE(str, QString("ene"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, " due")); // #1
    QCOMPARE(str, QString("ene due"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, " rike")); // #2 should merge
    QCOMPARE(str, QString("ene due rike"));
    QCOMPARE(AppendCommand::delete_cnt, 1); // #2 should be deleted
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setClean();
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.push(new AppendCommand(&str, " fake")); // #3 should NOT merge, since the stack was clean
    QCOMPARE(str, QString("ene due rike fake"));  // and we want to be able to return to this state
    QCOMPARE(AppendCommand::delete_cnt, 1); // #3 should not be deleted
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("ene due rike"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("ene"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "ma", true)); // #4 clean state gets deleted!
    QCOMPARE(str, QString("enema"));
    QCOMPARE(AppendCommand::delete_cnt, 3); // #1 got deleted
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "trix")); // #5 should NOT merge
    QCOMPARE(str, QString("enematrix"));
    QCOMPARE(AppendCommand::delete_cnt, 3);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("enema"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    // and now for command compression inside macros

    stack.setClean();
    QCOMPARE(str, QString("enema"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.beginMacro("ding");
    QCOMPARE(str, QString("enema"));
    QCOMPARE(AppendCommand::delete_cnt, 4); // #5 gets deleted
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    AppendCommand *merge_cmd = new AppendCommand(&str, "top");
    stack.push(merge_cmd); // #6
    QCOMPARE(merge_cmd->merged, false);
    QCOMPARE(str, QString("enematop"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.push(new AppendCommand(&str, "eja")); // #7 should merge
    QCOMPARE(str, QString("enematopeja"));
    QCOMPARE(merge_cmd->merged, true);
    QCOMPARE(AppendCommand::delete_cnt, 5); // #7 gets deleted
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged
    merge_cmd->merged = false;

    stack.push(new InsertCommand(&str, 2, "123")); // should not merge
    QCOMPARE(str, QString("en123ematopeja"));
    QCOMPARE(merge_cmd->merged, false);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.endMacro();
    QCOMPARE(str, QString("en123ematopeja"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("enema"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,      // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "ding",     // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.redo();
    QCOMPARE(str, QString("en123ematopeja"));
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
}

void tst_QUndoStack::undoLimit()
{
    QUndoStack stack;
    const QScopedPointer<QAction> undoAction(stack.createUndoAction(0, QString("foo")));
    const QScopedPointer<QAction> redoAction(stack.createRedoAction(0, QString("bar")));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    AppendCommand::delete_cnt = 0;
    QString str;

    QCOMPARE(stack.undoLimit(), 0);
    stack.setUndoLimit(2);
    QCOMPARE(stack.undoLimit(), 2);

    stack.push(new AppendCommand(&str, "1", true));
    QCOMPARE(str, QString("1"));
    QCOMPARE(AppendCommand::delete_cnt, 0);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "2", true));
    QCOMPARE(str, QString("12"));
    QCOMPARE(AppendCommand::delete_cnt, 0);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.setClean();
    QCOMPARE(str, QString("12"));
    QCOMPARE(AppendCommand::delete_cnt, 0);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.push(new AppendCommand(&str, "3", true));
    QCOMPARE(str, QString("123"));
    QCOMPARE(AppendCommand::delete_cnt, 1);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "4", true));
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 2);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("123"));
    QCOMPARE(AppendCommand::delete_cnt, 2);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("12"));
    QCOMPARE(AppendCommand::delete_cnt, 2);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "3", true));
    QCOMPARE(str, QString("123"));
    QCOMPARE(AppendCommand::delete_cnt, 4);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "4", true));
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 4);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "5", true));
    QCOMPARE(str, QString("12345"));
    QCOMPARE(AppendCommand::delete_cnt, 5);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 5);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("123"));
    QCOMPARE(AppendCommand::delete_cnt, 5);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "4", true));
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 7);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "5"));
    QCOMPARE(str, QString("12345"));
    QCOMPARE(AppendCommand::delete_cnt, 7);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "6", true)); // should be merged
    QCOMPARE(str, QString("123456"));
    QCOMPARE(AppendCommand::delete_cnt, 8);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.beginMacro("foo");
    QCOMPARE(str, QString("123456"));
    QCOMPARE(AppendCommand::delete_cnt, 8);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.push(new AppendCommand(&str, "7", true));
    QCOMPARE(str, QString("1234567"));
    QCOMPARE(AppendCommand::delete_cnt, 8);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.push(new AppendCommand(&str, "8"));
    QCOMPARE(str, QString("12345678"));
    QCOMPARE(AppendCommand::delete_cnt, 8);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false);     // redoChanged

    stack.endMacro();
    QCOMPARE(str, QString("12345678"));
    QCOMPARE(AppendCommand::delete_cnt, 9);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "foo",      // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("123456"));
    QCOMPARE(AppendCommand::delete_cnt, 9);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "foo",      // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 9);
    checkState(redoTextChangedSpy,
                canRedoChangedSpy,
                undoTextChangedSpy,
                redoAction,
                undoAction,
                canUndoChangedSpy,
                cleanChangedSpy,
                indexChangedSpy,
                stack,
                false,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true);      // redoChanged
}

void tst_QUndoStack::commandTextFormat()
{
#ifdef QT_NO_PROCESS
    QSKIP("No QProcess available");
#else
    QString binDir = QLibraryInfo::location(QLibraryInfo::BinariesPath);

    if (QProcess::execute(binDir + "/lrelease -version") != 0)
        QSKIP("lrelease is missing or broken");

    const QString tsFile = QFINDTESTDATA("testdata/qundostack.ts");
    QVERIFY(!tsFile.isEmpty());
    QFile::remove("qundostack.qm"); // Avoid confusion by strays.
    QVERIFY(!QProcess::execute(binDir + "/lrelease -silent " + tsFile + " -qm qundostack.qm"));

    QTranslator translator;
    QVERIFY(translator.load("qundostack.qm"));
    QFile::remove("qundostack.qm");
    qApp->installTranslator(&translator);

    QUndoStack stack;
    const QScopedPointer<QAction> undo_action(stack.createUndoAction(0));
    const QScopedPointer<QAction> redo_action(stack.createRedoAction(0));

    QCOMPARE(undo_action->text(), QString("Undo-default-text"));
    QCOMPARE(redo_action->text(), QString("Redo-default-text"));

    QString str;

    stack.push(new AppendCommand(&str, "foo"));
    QCOMPARE(undo_action->text(), QString("undo-prefix append undo-suffix"));
    QCOMPARE(redo_action->text(), QString("Redo-default-text"));

    stack.push(new InsertCommand(&str, 0, "bar"));
    stack.undo();
    QCOMPARE(undo_action->text(), QString("undo-prefix append undo-suffix"));
    QCOMPARE(redo_action->text(), QString("redo-prefix insert redo-suffix"));

    stack.undo();
    QCOMPARE(undo_action->text(), QString("Undo-default-text"));
    QCOMPARE(redo_action->text(), QString("redo-prefix append redo-suffix"));

    qApp->removeTranslator(&translator);
#endif
}

void tst_QUndoStack::separateUndoText()
{
    QUndoStack stack;
    const QScopedPointer<QAction> undo_action(stack.createUndoAction(0));
    const QScopedPointer<QAction> redo_action(stack.createRedoAction(0));

    QUndoCommand *command1 = new IdleCommand();
    QUndoCommand *command2 = new IdleCommand();
    stack.push(command1);
    stack.push(command2);
    stack.undo();

    QCOMPARE(undo_action->text(), QString("Undo idle-action"));
    QCOMPARE(redo_action->text(), QString("Redo idle-action"));
    QCOMPARE(command1->actionText(), QString("idle-action"));

    QCOMPARE(command1->text(), QString("idle-item"));
    QCOMPARE(stack.text(0), QString("idle-item"));

    command1->setText("idle");
    QCOMPARE(command1->actionText(), QString("idle"));
    QCOMPARE(command1->text(), QString("idle"));

    command1->setText("idle-item\nidle-action");
    QCOMPARE(command1->actionText(), QString("idle-action"));
    QCOMPARE(command1->text(), QString("idle-item"));
}

QTEST_MAIN(tst_QUndoStack)

#include "tst_qundostack.moc"
