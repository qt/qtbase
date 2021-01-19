/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QSTATE_H
#define QSTATE_H

#include <QtCore/qabstractstate.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetaobject.h>

QT_REQUIRE_CONFIG(statemachine);

QT_BEGIN_NAMESPACE

class QAbstractTransition;
class QSignalTransition;

class QStatePrivate;
class Q_CORE_EXPORT QState : public QAbstractState
{
    Q_OBJECT
    Q_PROPERTY(QAbstractState* initialState READ initialState WRITE setInitialState NOTIFY initialStateChanged)
    Q_PROPERTY(QAbstractState* errorState READ errorState WRITE setErrorState NOTIFY errorStateChanged)
    Q_PROPERTY(ChildMode childMode READ childMode WRITE setChildMode NOTIFY childModeChanged)
public:
    enum ChildMode {
        ExclusiveStates,
        ParallelStates
    };
    Q_ENUM(ChildMode)

    enum RestorePolicy {
        DontRestoreProperties,
        RestoreProperties
    };
    Q_ENUM(RestorePolicy)

    QState(QState *parent = nullptr);
    QState(ChildMode childMode, QState *parent = nullptr);
    ~QState();

    QAbstractState *errorState() const;
    void setErrorState(QAbstractState *state);

    void addTransition(QAbstractTransition *transition);
    QSignalTransition *addTransition(const QObject *sender, const char *signal, QAbstractState *target);
#ifdef Q_QDOC
    template<typename PointerToMemberFunction>
    QSignalTransition *addTransition(const QObject *sender, PointerToMemberFunction signal,
                       QAbstractState *target);
#else
    template <typename Func>
    QSignalTransition *addTransition(const typename QtPrivate::FunctionPointer<Func>::Object *obj,
                      Func signal, QAbstractState *target)
    {
        const QMetaMethod signalMetaMethod = QMetaMethod::fromSignal(signal);
        return addTransition(obj, signalMetaMethod.methodSignature().constData(), target);
    }
#endif // Q_QDOC
    QAbstractTransition *addTransition(QAbstractState *target);
    void removeTransition(QAbstractTransition *transition);
    QList<QAbstractTransition*> transitions() const;

    QAbstractState *initialState() const;
    void setInitialState(QAbstractState *state);

    ChildMode childMode() const;
    void setChildMode(ChildMode mode);

#ifndef QT_NO_PROPERTIES
    void assignProperty(QObject *object, const char *name,
                        const QVariant &value);
#endif

Q_SIGNALS:
    void finished(QPrivateSignal);
    void propertiesAssigned(QPrivateSignal);
    void childModeChanged(QPrivateSignal);
    void initialStateChanged(QPrivateSignal);
    void errorStateChanged(QPrivateSignal);

protected:
    void onEntry(QEvent *event) override;
    void onExit(QEvent *event) override;

    bool event(QEvent *e) override;

protected:
    QState(QStatePrivate &dd, QState *parent);

private:
    Q_DISABLE_COPY(QState)
    Q_DECLARE_PRIVATE(QState)
};

QT_END_NAMESPACE

#endif
