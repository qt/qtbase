/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QUNDOSTACK_H
#define QUNDOSTACK_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

QT_REQUIRE_CONFIG(undocommand);

QT_BEGIN_NAMESPACE

class QAction;
class QUndoCommandPrivate;
class QUndoStackPrivate;

class Q_WIDGETS_EXPORT QUndoCommand
{
    QUndoCommandPrivate *d;

public:
    explicit QUndoCommand(QUndoCommand *parent = nullptr);
    explicit QUndoCommand(const QString &text, QUndoCommand *parent = nullptr);
    virtual ~QUndoCommand();

    virtual void undo();
    virtual void redo();

    QString text() const;
    QString actionText() const;
    void setText(const QString &text);

    bool isObsolete() const;
    void setObsolete(bool obsolete);

    virtual int id() const;
    virtual bool mergeWith(const QUndoCommand *other);

    int childCount() const;
    const QUndoCommand *child(int index) const;

private:
    Q_DISABLE_COPY(QUndoCommand)
    friend class QUndoStack;
};

#if QT_CONFIG(undostack)

class Q_WIDGETS_EXPORT QUndoStack : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QUndoStack)
    Q_PROPERTY(bool active READ isActive WRITE setActive)
    Q_PROPERTY(int undoLimit READ undoLimit WRITE setUndoLimit)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)
    Q_PROPERTY(QString undoText READ undoText NOTIFY undoTextChanged)
    Q_PROPERTY(QString redoText READ redoText NOTIFY redoTextChanged)
    Q_PROPERTY(bool clean READ isClean NOTIFY cleanChanged)

public:
    explicit QUndoStack(QObject *parent = nullptr);
    ~QUndoStack();
    void clear();

    void push(QUndoCommand *cmd);

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;

    int count() const;
    int index() const;
    QString text(int idx) const;

#ifndef QT_NO_ACTION
    QAction *createUndoAction(QObject *parent,
                                const QString &prefix = QString()) const;
    QAction *createRedoAction(QObject *parent,
                                const QString &prefix = QString()) const;
#endif // QT_NO_ACTION

    bool isActive() const;
    bool isClean() const;
    int cleanIndex() const;

    void beginMacro(const QString &text);
    void endMacro();

    void setUndoLimit(int limit);
    int undoLimit() const;

    const QUndoCommand *command(int index) const;

public Q_SLOTS:
    void setClean();
    void resetClean();
    void setIndex(int idx);
    void undo();
    void redo();
    void setActive(bool active = true);

Q_SIGNALS:
    void indexChanged(int idx);
    void cleanChanged(bool clean);
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);
    void undoTextChanged(const QString &undoText);
    void redoTextChanged(const QString &redoText);

private:
    Q_DISABLE_COPY(QUndoStack)
    friend class QUndoGroup;
};

#endif // QT_CONFIG(undostack)

QT_END_NAMESPACE

#endif // QUNDOSTACK_H
