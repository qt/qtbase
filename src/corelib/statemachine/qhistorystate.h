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

#ifndef QHISTORYSTATE_H
#define QHISTORYSTATE_H

#include <QtCore/qabstractstate.h>

QT_REQUIRE_CONFIG(statemachine);

QT_BEGIN_NAMESPACE

class QAbstractTransition;
class QHistoryStatePrivate;
class Q_CORE_EXPORT QHistoryState : public QAbstractState
{
    Q_OBJECT
    Q_PROPERTY(QAbstractState* defaultState READ defaultState WRITE setDefaultState NOTIFY defaultStateChanged)
    Q_PROPERTY(QAbstractTransition* defaultTransition READ defaultTransition WRITE setDefaultTransition NOTIFY defaultTransitionChanged)
    Q_PROPERTY(HistoryType historyType READ historyType WRITE setHistoryType NOTIFY historyTypeChanged)
public:
    enum HistoryType {
        ShallowHistory,
        DeepHistory
    };
    Q_ENUM(HistoryType)

    QHistoryState(QState *parent = nullptr);
    QHistoryState(HistoryType type, QState *parent = nullptr);
    ~QHistoryState();

    QAbstractTransition *defaultTransition() const;
    void setDefaultTransition(QAbstractTransition *transition);

    QAbstractState *defaultState() const;
    void setDefaultState(QAbstractState *state);

    HistoryType historyType() const;
    void setHistoryType(HistoryType type);

Q_SIGNALS:
    void defaultTransitionChanged(QPrivateSignal);
    void defaultStateChanged(QPrivateSignal);
    void historyTypeChanged(QPrivateSignal);

protected:
    void onEntry(QEvent *event) override;
    void onExit(QEvent *event) override;

    bool event(QEvent *e) override;

private:
    Q_DISABLE_COPY(QHistoryState)
    Q_DECLARE_PRIVATE(QHistoryState)
};

QT_END_NAMESPACE

#endif
