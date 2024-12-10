// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef NFA_H
#define NFA_H

#include <QByteArray>
#include <QDebug>
#include <QHash>
#include <QList>
#include <QMap>
#include <QStack>
#include <QString>

#include "global.h"

typedef QMultiHash<InputType, int> TransitionMap;

struct State
{
    QString symbol;
    TransitionMap transitions;
};

inline QDataStream &operator<<(QDataStream &stream, const State &state)
{
    return stream << state.symbol << state.transitions;
}

inline QDataStream &operator>>(QDataStream &stream, State &state)
{
    return stream >> state.symbol >> state.transitions;
}

struct DFA : public QList<State>
{
    void debug() const;
    DFA minimize() const;
};

class NFA
{
public:
    static NFA createSingleInputNFA(InputType input);
    static NFA createSymbolNFA(const QString &symbol);
    static NFA createAlternatingNFA(const NFA &a, const NFA &b);
    static NFA createConcatenatingNFA(const NFA &a, const NFA &b);
    static NFA createOptionalNFA(const NFA &a);

    // convenience
    static NFA createStringNFA(const QByteArray &str);
    static NFA createSetNFA(const QSet<InputType> &set);
    static NFA createZeroOrOneNFA(const NFA &a);
    static NFA applyQuantity(const NFA &a, int minOccurrences, int maxOccurrences);

    void setTerminationSymbol(const QString &symbol);

    DFA toDFA() const;

    inline bool isEmpty() const { return states.isEmpty(); }
    inline int stateCount() const { return states.count(); }

    void debug();

private:
    void initialize(int size);
    void addTransition(int from, InputType input, int to);
    void copyFrom(const NFA &other, int baseState);

    void initializeFromPair(const NFA &a, const NFA &b,
                            int *initialA, int *finalA,
                            int *initialB, int *finalB);

    QSet<int> epsilonClosure(const QSet<int> &initialClosure) const;

    inline void assertValidState(int state)
    { Q_UNUSED(state); Q_ASSERT(state >= 0); Q_ASSERT(state < states.count()); }

#if defined(AUTOTEST)
public:
#endif
    int initialState;
    int finalState;

    QList<State> states;
};

#endif // NFA_H

