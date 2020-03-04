/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#include "qbytearray.h"
#include "qlinkedlist.h"
#include "qlist.h"
#include "qstring.h"
#include "qvarlengtharray.h"
#include "qvector.h"
#include "qhash.h"
#include "qdebug.h"

#include <algorithm>
#include <functional>
#include <vector> // for reference
#include <list>
#include <set>
#include <map>

// MSVC has these containers from the Standard Library, but it lacks
// a __has_include mechanism (that we need to use for other stdlibs).
// For the sake of increasing our test coverage, work around the issue.

#ifdef Q_CC_MSVC
#define COMPILER_HAS_STDLIB_INCLUDE(x) 1
#else
#define COMPILER_HAS_STDLIB_INCLUDE(x) __has_include(x)
#endif

#if COMPILER_HAS_STDLIB_INCLUDE(<forward_list>)
#include <forward_list>
#endif
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
#include <unordered_set>
#endif
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_map>)
#include <unordered_map>
#endif

struct Movable
{
    explicit Movable(int i = 0) Q_DECL_NOTHROW
        : i(i)
    {
        ++instanceCount;
    }

    Movable(const Movable &m)
        : i(m.i)
    {
        ++instanceCount;
    }

    ~Movable()
    {
        --instanceCount;
    }

    int i;
    static int instanceCount;
};

int Movable::instanceCount = 0;
bool operator==(Movable lhs, Movable rhs) Q_DECL_NOTHROW { return lhs.i == rhs.i; }
bool operator!=(Movable lhs, Movable rhs) Q_DECL_NOTHROW { return lhs.i != rhs.i; }
bool operator<(Movable lhs, Movable rhs) Q_DECL_NOTHROW { return lhs.i < rhs.i; }

uint qHash(Movable m, uint seed = 0) Q_DECL_NOTHROW { return qHash(m.i, seed); }
QDebug &operator<<(QDebug &d, Movable m)
{
    const QDebugStateSaver saver(d);
    return d.nospace() << "Movable(" << m.i << ")";
}

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Movable, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

struct Complex
{
    explicit Complex(int i = 0) Q_DECL_NOTHROW
        : i(i)
    {
        ++instanceCount;
    }

    Complex(const Complex &c)
        : i(c.i)
    {
        ++instanceCount;
    }

    ~Complex()
    {
        --instanceCount;
    }

    int i;
    static int instanceCount;
};

int Complex::instanceCount = 0;
bool operator==(Complex lhs, Complex rhs) Q_DECL_NOTHROW { return lhs.i == rhs.i; }
bool operator!=(Complex lhs, Complex rhs) Q_DECL_NOTHROW { return lhs.i != rhs.i; }
bool operator<(Complex lhs, Complex rhs) Q_DECL_NOTHROW { return lhs.i < rhs.i; }

uint qHash(Complex c, uint seed = 0) Q_DECL_NOTHROW { return qHash(c.i, seed); }
QDebug &operator<<(QDebug &d, Complex c)
{
    const QDebugStateSaver saver(d);
    return d.nospace() << "Complex(" << c.i << ")";
}


struct DuplicateStrategyTestType
{
    explicit DuplicateStrategyTestType(int i = 0) Q_DECL_NOTHROW
        : i(i),
          j(++counter)
    {
    }

    int i;
    int j;

    static int counter;
};

int DuplicateStrategyTestType::counter = 0;

// only look at the i member, not j. j allows us to identify which instance
// gets inserted in containers that don't allow for duplicates
bool operator==(DuplicateStrategyTestType lhs, DuplicateStrategyTestType rhs) Q_DECL_NOTHROW
{
    return lhs.i == rhs.i;
}

bool operator!=(DuplicateStrategyTestType lhs, DuplicateStrategyTestType rhs) Q_DECL_NOTHROW
{
    return lhs.i != rhs.i;
}

bool operator<(DuplicateStrategyTestType lhs, DuplicateStrategyTestType rhs) Q_DECL_NOTHROW
{
    return lhs.i < rhs.i;
}

uint qHash(DuplicateStrategyTestType c, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(c.i, seed);
}

bool reallyEqual(DuplicateStrategyTestType lhs, DuplicateStrategyTestType rhs) Q_DECL_NOTHROW
{
    return lhs.i == rhs.i && lhs.j == rhs.j;
}

QDebug &operator<<(QDebug &d, DuplicateStrategyTestType c)
{
    const QDebugStateSaver saver(d);
    return d.nospace() << "DuplicateStrategyTestType(" << c.i << "," << c.j << ")";
}


namespace std {
template<>
struct hash<Movable>
{
    std::size_t operator()(Movable m) const Q_DECL_NOTHROW
    {
        return hash<int>()(m.i);
    }
};

template<>
struct hash<Complex>
{
    std::size_t operator()(Complex m) const Q_DECL_NOTHROW
    {
        return hash<int>()(m.i);
    }
};

template<>
struct hash<DuplicateStrategyTestType>
{
    std::size_t operator()(DuplicateStrategyTestType m) const Q_DECL_NOTHROW
    {
        return hash<int>()(m.i);
    }
};
}

// work around the fact that QVarLengthArray has a non-type
// template parameter, and that breaks non_associative_container_duplicates_strategy
template<typename T>
class VarLengthArray : public QVarLengthArray<T>
{
public:
#ifdef Q_COMPILER_INHERITING_CONSTRUCTORS
    using QVarLengthArray<T>::QVarLengthArray;
#else
    template<typename InputIterator>
    VarLengthArray(InputIterator first, InputIterator last)
        : QVarLengthArray<T>(first, last)
    {
    }

    VarLengthArray(std::initializer_list<T> args)
        : QVarLengthArray<T>(args)
    {
    }
#endif
};

class tst_ContainerApiSymmetry : public QObject
{
    Q_OBJECT

    int m_movableInstanceCount;
    int m_complexInstanceCount;

private Q_SLOTS:
    void init();
    void cleanup();

private:
    template <typename Container>
    void ranged_ctor_non_associative_impl() const;

    template<template<typename ... T> class Container>
    void non_associative_container_duplicates_strategy() const;

    template <typename Container>
    void ranged_ctor_associative_impl() const;

private Q_SLOTS:
    // non associative
    void ranged_ctor_std_vector_int() { ranged_ctor_non_associative_impl<std::vector<int>>(); }
    void ranged_ctor_std_vector_char() { ranged_ctor_non_associative_impl<std::vector<char>>(); }
    void ranged_ctor_std_vector_QChar() { ranged_ctor_non_associative_impl<std::vector<QChar>>(); }
    void ranged_ctor_std_vector_Movable() { ranged_ctor_non_associative_impl<std::vector<Movable>>(); }
    void ranged_ctor_std_vector_Complex() { ranged_ctor_non_associative_impl<std::vector<Complex>>(); }
    void ranged_ctor_std_vector_duplicates_strategy() { non_associative_container_duplicates_strategy<std::vector>(); }

    void ranged_ctor_QVector_int() { ranged_ctor_non_associative_impl<QVector<int>>(); }
    void ranged_ctor_QVector_char() { ranged_ctor_non_associative_impl<QVector<char>>(); }
    void ranged_ctor_QVector_QChar() { ranged_ctor_non_associative_impl<QVector<QChar>>(); }
    void ranged_ctor_QVector_Movable() { ranged_ctor_non_associative_impl<QVector<Movable>>(); }
    void ranged_ctor_QVector_Complex() { ranged_ctor_non_associative_impl<QVector<Complex>>(); }
    void ranged_ctor_QVector_duplicates_strategy() { non_associative_container_duplicates_strategy<QVector>(); }

    void ranged_ctor_QVarLengthArray_int() { ranged_ctor_non_associative_impl<QVarLengthArray<int>>(); }
    void ranged_ctor_QVarLengthArray_Movable() { ranged_ctor_non_associative_impl<QVarLengthArray<Movable>>(); }
    void ranged_ctor_QVarLengthArray_Complex() { ranged_ctor_non_associative_impl<QVarLengthArray<Complex>>(); }
    void ranged_ctor_QVarLengthArray_duplicates_strategy() { non_associative_container_duplicates_strategy<VarLengthArray>(); } // note the VarLengthArray passed

    void ranged_ctor_QList_int() { ranged_ctor_non_associative_impl<QList<int>>(); }
    void ranged_ctor_QList_Movable() { ranged_ctor_non_associative_impl<QList<Movable>>(); }
    void ranged_ctor_QList_Complex() { ranged_ctor_non_associative_impl<QList<Complex>>(); }
    void ranged_ctor_QList_duplicates_strategy() { non_associative_container_duplicates_strategy<QList>(); }

    void ranged_ctor_std_list_int() { ranged_ctor_non_associative_impl<std::list<int>>(); }
    void ranged_ctor_std_list_Movable() { ranged_ctor_non_associative_impl<std::list<Movable>>(); }
    void ranged_ctor_std_list_Complex() { ranged_ctor_non_associative_impl<std::list<Complex>>(); }
    void ranged_ctor_std_list_duplicates_strategy() { non_associative_container_duplicates_strategy<std::list>(); }

    void ranged_ctor_std_forward_list_int() {
#if COMPILER_HAS_STDLIB_INCLUDE(<forward_list>)
        ranged_ctor_non_associative_impl<std::forward_list<int>>();
#else
        QSKIP("<forward_list> is needed for this test");
#endif
    }

    void ranged_ctor_std_forward_list_Movable() {
#if COMPILER_HAS_STDLIB_INCLUDE(<forward_list>)
        ranged_ctor_non_associative_impl<std::forward_list<Movable>>();
#else
        QSKIP("<forward_list> is needed for this test");
#endif
    }

    void ranged_ctor_std_forward_list_Complex() {
#if COMPILER_HAS_STDLIB_INCLUDE(<forward_list>)
        ranged_ctor_non_associative_impl<std::forward_list<Complex>>();
#else
        QSKIP("<forward_list> is needed for this test");
#endif
    }

    void ranged_ctor_std_forward_list_duplicates_strategy() {
#if COMPILER_HAS_STDLIB_INCLUDE(<forward_list>)
        non_associative_container_duplicates_strategy<std::forward_list>();
#else
        QSKIP("<forward_list> is needed for this test");
#endif
    }

#if QT_DEPRECATED_SINCE(5, 15)
    void ranged_ctor_QLinkedList_int();
    void ranged_ctor_QLinkedList_Movable();
    void ranged_ctor_QLinkedList_Complex();
    void ranged_ctor_QLinkedList_duplicates_strategy();
#endif
    void ranged_ctor_std_set_int() { ranged_ctor_non_associative_impl<std::set<int>>(); }
    void ranged_ctor_std_set_Movable() { ranged_ctor_non_associative_impl<std::set<Movable>>(); }
    void ranged_ctor_std_set_Complex() { ranged_ctor_non_associative_impl<std::set<Complex>>(); }
    void ranged_ctor_std_set_duplicates_strategy() { non_associative_container_duplicates_strategy<std::set>(); }

    void ranged_ctor_std_multiset_int() { ranged_ctor_non_associative_impl<std::multiset<int>>(); }
    void ranged_ctor_std_multiset_Movable() { ranged_ctor_non_associative_impl<std::multiset<Movable>>(); }
    void ranged_ctor_std_multiset_Complex() { ranged_ctor_non_associative_impl<std::multiset<Complex>>(); }
    void ranged_ctor_std_multiset_duplicates_strategy() { non_associative_container_duplicates_strategy<std::multiset>(); }

    void ranged_ctor_std_unordered_set_int() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
        ranged_ctor_non_associative_impl<std::unordered_set<int>>();
#else
        QSKIP("<unordered_set> is needed for this test");
#endif
    }

    void ranged_ctor_std_unordered_set_Movable() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
        ranged_ctor_non_associative_impl<std::unordered_set<Movable>>();
#else
        QSKIP("<unordered_set> is needed for this test");
#endif
    }

    void ranged_ctor_std_unordered_set_Complex() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
        ranged_ctor_non_associative_impl<std::unordered_set<Complex>>();
#else
        QSKIP("<unordered_set> is needed for this test");
#endif
    }

    void ranged_ctor_unordered_set_duplicates_strategy() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
        non_associative_container_duplicates_strategy<std::unordered_set>();
#else
        QSKIP("<unordered_set> is needed for this test");
#endif
    }


    void ranged_ctor_std_unordered_multiset_int() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
        ranged_ctor_non_associative_impl<std::unordered_multiset<int>>();
#else
        QSKIP("<unordered_set> is needed for this test");
#endif
    }

    void ranged_ctor_std_unordered_multiset_Movable() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
        ranged_ctor_non_associative_impl<std::unordered_multiset<Movable>>();
#else
        QSKIP("<unordered_set> is needed for this test");
#endif
    }

    void ranged_ctor_std_unordered_multiset_Complex() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
        ranged_ctor_non_associative_impl<std::unordered_multiset<Complex>>();
#else
        QSKIP("<unordered_set> is needed for this test");
#endif
    }

    void ranged_ctor_std_unordered_multiset_duplicates_strategy() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
        non_associative_container_duplicates_strategy<std::unordered_multiset>();
#else
        QSKIP("<unordered_set> is needed for this test");
#endif
    }

    void ranged_ctor_QSet_int() { ranged_ctor_non_associative_impl<QSet<int>>(); }
    void ranged_ctor_QSet_Movable() { ranged_ctor_non_associative_impl<QSet<Movable>>(); }
    void ranged_ctor_QSet_Complex() { ranged_ctor_non_associative_impl<QSet<Complex>>(); }
    void ranged_ctor_QSet_duplicates_strategy() { non_associative_container_duplicates_strategy<QSet>(); }

    // associative
    void ranged_ctor_std_map_int() { ranged_ctor_associative_impl<std::map<int, int>>(); }
    void ranged_ctor_std_map_Movable() { ranged_ctor_associative_impl<std::map<Movable, int>>(); }
    void ranged_ctor_std_map_Complex() { ranged_ctor_associative_impl<std::map<Complex, int>>(); }

    void ranged_ctor_std_multimap_int() { ranged_ctor_associative_impl<std::multimap<int, int>>(); }
    void ranged_ctor_std_multimap_Movable() { ranged_ctor_associative_impl<std::multimap<Movable, int>>(); }
    void ranged_ctor_std_multimap_Complex() { ranged_ctor_associative_impl<std::multimap<Complex, int>>(); }

    void ranged_ctor_unordered_map_int() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_map>)
        ranged_ctor_associative_impl<std::unordered_map<int, int>>();
#else
        QSKIP("<unordered_map> is needed for this test");
#endif
    }

    void ranged_ctor_unordered_map_Movable() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_map>)
        ranged_ctor_associative_impl<std::unordered_map<Movable, Movable>>();
#else
        QSKIP("<unordered_map> is needed for this test");
#endif
    }

    void ranged_ctor_unordered_map_Complex() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_map>)
        ranged_ctor_associative_impl<std::unordered_map<Complex, Complex>>();
#else
        QSKIP("<unordered_map> is needed for this test");
#endif
    }

    void ranged_ctor_QHash_int() { ranged_ctor_associative_impl<QHash<int, int>>(); }
    void ranged_ctor_QHash_Movable() { ranged_ctor_associative_impl<QHash<Movable, int>>(); }
    void ranged_ctor_QHash_Complex() { ranged_ctor_associative_impl<QHash<Complex, int>>(); }

    void ranged_ctor_unordered_multimap_int() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_map>)
        ranged_ctor_associative_impl<std::unordered_multimap<int, int>>();
#else
        QSKIP("<unordered_map> is needed for this test");
#endif
    }

    void ranged_ctor_unordered_multimap_Movable() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_map>)
        ranged_ctor_associative_impl<std::unordered_multimap<Movable, Movable>>();
#else
        QSKIP("<unordered_map> is needed for this test");
#endif
    }

    void ranged_ctor_unordered_multimap_Complex() {
#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_map>)
        ranged_ctor_associative_impl<std::unordered_multimap<Complex, Complex>>();
#else
        QSKIP("<unordered_map> is needed for this test");
#endif
    }

    void ranged_ctor_QMultiHash_int() { ranged_ctor_associative_impl<QMultiHash<int, int>>(); }
    void ranged_ctor_QMultiHash_Movable() { ranged_ctor_associative_impl<QMultiHash<Movable, int>>(); }
    void ranged_ctor_QMultiHash_Complex() { ranged_ctor_associative_impl<QMultiHash<Complex, int>>(); }

private:
    template <typename Container>
    void front_back_impl() const;

private Q_SLOTS:
    void front_back_std_vector() { front_back_impl<std::vector<int>>(); }
    void front_back_QVector() { front_back_impl<QVector<int>>(); }
    void front_back_QList() { front_back_impl<QList<qintptr>>(); }
#if QT_DEPRECATED_SINCE(5, 15)
    void front_back_QLinkedList();
#endif
    void front_back_QVarLengthArray() { front_back_impl<QVarLengthArray<int>>(); }
    void front_back_QString() { front_back_impl<QString>(); }
    void front_back_QStringRef() { front_back_impl<QStringRef>(); }
    void front_back_QStringView() { front_back_impl<QStringView>(); }
    void front_back_QLatin1String() { front_back_impl<QLatin1String>(); }
    void front_back_QByteArray() { front_back_impl<QByteArray>(); }
};

void tst_ContainerApiSymmetry::init()
{
    m_movableInstanceCount = Movable::instanceCount;
    m_complexInstanceCount = Complex::instanceCount;
}

void tst_ContainerApiSymmetry::cleanup()
{
    // very simple leak check
    QCOMPARE(Movable::instanceCount, m_movableInstanceCount);
    QCOMPARE(Complex::instanceCount, m_complexInstanceCount);
}

template <typename Container>
Container createContainerReference()
{
    using V = typename Container::value_type;

    return {V(0), V(1), V(2), V(0)};
}

template <typename Container>
void tst_ContainerApiSymmetry::ranged_ctor_non_associative_impl() const
{
    using V = typename Container::value_type;

    // the double V(0) is deliberate
    const auto reference = createContainerReference<Container>();

    // plain array
    const V values1[] = { V(0), V(1), V(2), V(0) };

    const Container c1(values1, values1 + sizeof(values1)/sizeof(values1[0]));

    // from QList
    QList<V> l2;
    l2 << V(0) << V(1) << V(2) << V(0);

    const Container c2a(l2.begin(), l2.end());
    const Container c2b(l2.cbegin(), l2.cend());

    // from std::list
    std::list<V> l3;
    l3.push_back(V(0));
    l3.push_back(V(1));
    l3.push_back(V(2));
    l3.push_back(V(0));
    const Container c3a(l3.begin(), l3.end());

    // from const std::list
    const std::list<V> l3c = l3;
    const Container c3b(l3c.begin(), l3c.end());

    // from itself
    const Container c4(reference.begin(), reference.end());

    QCOMPARE(c1,  reference);
    QCOMPARE(c2a, reference);
    QCOMPARE(c2b, reference);
    QCOMPARE(c3a, reference);
    QCOMPARE(c3b, reference);
    QCOMPARE(c4,  reference);
}


// type traits for detecting whether a non-associative container
// accepts duplicated values, and if it doesn't, whether construction/insertion
// prefer the new values (overwriting) or the old values (rejecting)

struct ContainerAcceptsDuplicateValues {};
struct ContainerOverwritesDuplicateValues {};
struct ContainerRejectsDuplicateValues {};

template<typename Container>
struct ContainerDuplicatedValuesStrategy {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::vector<T...>> : ContainerAcceptsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<QVector<T...>> : ContainerAcceptsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<QVarLengthArray<T...>> : ContainerAcceptsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<VarLengthArray<T...>> : ContainerAcceptsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<QList<T...>> : ContainerAcceptsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::list<T...>> : ContainerAcceptsDuplicateValues {};

#if COMPILER_HAS_STDLIB_INCLUDE(<forward_list>)
template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::forward_list<T...>> : ContainerAcceptsDuplicateValues {};
#endif

#if QT_DEPRECATED_SINCE(5, 15)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
template<typename ... T>
struct ContainerDuplicatedValuesStrategy<QLinkedList<T...>> : ContainerAcceptsDuplicateValues {};
QT_WARNING_POP
#endif

// assuming https://cplusplus.github.io/LWG/lwg-active.html#2844 resolution
template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::set<T...>> : ContainerRejectsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::multiset<T...>> : ContainerAcceptsDuplicateValues {};

#if COMPILER_HAS_STDLIB_INCLUDE(<unordered_set>)
// assuming https://cplusplus.github.io/LWG/lwg-active.html#2844 resolution
template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::unordered_set<T...>> : ContainerRejectsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::unordered_multiset<T...>> : ContainerAcceptsDuplicateValues {};
#endif

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<QSet<T...>> : ContainerRejectsDuplicateValues {};

template<typename Container>
void non_associative_container_check_duplicates_impl(const std::initializer_list<DuplicateStrategyTestType> &reference, const Container &c, ContainerAcceptsDuplicateValues)
{
    // do a deep check for equality, not ordering
    QVERIFY(std::distance(reference.begin(), reference.end()) == std::distance(c.begin(), c.end()));
    QVERIFY(std::is_permutation(reference.begin(), reference.end(), c.begin(), &reallyEqual));
}

enum class IterationOnReference
{
    ForwardIteration,
    ReverseIteration
};

template<typename Container>
void non_associative_container_check_duplicates_impl_no_duplicates(const std::initializer_list<DuplicateStrategyTestType> &reference, const Container &c, IterationOnReference ior)
{
    std::vector<DuplicateStrategyTestType> valuesAlreadySeen;

    // iterate on reference forward or backwards, depending on ior. this will give
    // us the expected semantics when checking for duplicated values into c
    auto it = [&reference, ior]() {
        switch (ior) {
        case IterationOnReference::ForwardIteration: return reference.begin();
        case IterationOnReference::ReverseIteration: return reference.end() - 1;
        };
        return std::initializer_list<DuplicateStrategyTestType>::const_iterator();
    }();

    const auto &end = [&reference, ior]() {
        switch (ior) {
        case IterationOnReference::ForwardIteration: return reference.end();
        case IterationOnReference::ReverseIteration: return reference.begin() - 1;
        };
        return std::initializer_list<DuplicateStrategyTestType>::const_iterator();
    }();

    while (it != end) {
        const auto &value = *it;

        // check that there is indeed the same value in the container (using operator==)
        const auto &valueInContainerIterator = std::find(c.begin(), c.end(), value);
        QVERIFY(valueInContainerIterator != c.end());
        QVERIFY(value == *valueInContainerIterator);

        // if the value is a duplicate, we don't expect to find it in the container
        // (when doing a deep comparison). otherwise it should be there

        const auto &valuesAlreadySeenIterator = std::find(valuesAlreadySeen.cbegin(), valuesAlreadySeen.cend(), value);
        const bool valueIsDuplicated = (valuesAlreadySeenIterator != valuesAlreadySeen.cend());

        const auto &reallyEqualCheck = [&value](const DuplicateStrategyTestType &v) { return reallyEqual(value, v); };
        QCOMPARE(std::find_if(c.begin(), c.end(), reallyEqualCheck) == c.end(), valueIsDuplicated);

        valuesAlreadySeen.push_back(value);

        switch (ior) {
        case IterationOnReference::ForwardIteration:
            ++it;
            break;
        case IterationOnReference::ReverseIteration:
            --it;
            break;
        };
    }

}

template<typename Container>
void non_associative_container_check_duplicates_impl(const std::initializer_list<DuplicateStrategyTestType> &reference, const Container &c, ContainerRejectsDuplicateValues)
{
    non_associative_container_check_duplicates_impl_no_duplicates(reference, c, IterationOnReference::ForwardIteration);
}

template<typename Container>
void non_associative_container_check_duplicates_impl(const std::initializer_list<DuplicateStrategyTestType> &reference, const Container &c, ContainerOverwritesDuplicateValues)
{
    non_associative_container_check_duplicates_impl_no_duplicates(reference, c, IterationOnReference::ReverseIteration);
}

template<typename Container>
void non_associative_container_check_duplicates(const std::initializer_list<DuplicateStrategyTestType> &reference, const Container &c)
{
    non_associative_container_check_duplicates_impl(reference, c, ContainerDuplicatedValuesStrategy<Container>());
}

template<template<class ... T> class Container>
void tst_ContainerApiSymmetry::non_associative_container_duplicates_strategy() const
{
    // first and last are "duplicates" -- they compare equal for operator==,
    // but they differ when using reallyEqual
    const std::initializer_list<DuplicateStrategyTestType> reference{ DuplicateStrategyTestType{0},
                                                                      DuplicateStrategyTestType{1},
                                                                      DuplicateStrategyTestType{2},
                                                                      DuplicateStrategyTestType{0} };
    Container<DuplicateStrategyTestType> c1{reference};
    non_associative_container_check_duplicates(reference, c1);

    Container<DuplicateStrategyTestType> c2{reference.begin(), reference.end()};
    non_associative_container_check_duplicates(reference, c2);
}

template <typename Container>
void tst_ContainerApiSymmetry::ranged_ctor_associative_impl() const
{
    using K = typename Container::key_type;
    using V = typename Container::mapped_type;

    // The double K(0) is deliberate. The order of the elements matters:
    // * for unique-key STL containers, the first one should be the one inserted (cf. LWG 2844)
    // * for unique-key Qt containers, the last one should be the one inserted
    // * for multi-key sorted containers, the order of insertion of identical keys is also the
    //   iteration order (which establishes the equality of the containers)
    // (although nothing of this is being tested here, that deserves its own testing)
    const Container reference{
        { K(0), V(1000) },
        { K(1), V(1001) },
        { K(2), V(1002) },
        { K(0), V(1003) }
    };

    // Note that using anything not convertible to std::pair doesn't work for
    // std containers. Their ranged construction is defined in terms of
    // insert(value_type), which for std associative containers is
    // std::pair<const K, T>.

    // plain array
    const std::pair<K, V> values1[] = {
        std::make_pair(K(0), V(1000)),
        std::make_pair(K(1), V(1001)),
        std::make_pair(K(2), V(1002)),
        std::make_pair(K(0), V(1003))
    };

    const Container c1(values1, values1 + sizeof(values1)/sizeof(values1[0]));

    // from QList
    QList<std::pair<K, V>> l2;
    l2 << std::make_pair(K(0), V(1000))
       << std::make_pair(K(1), V(1001))
       << std::make_pair(K(2), V(1002))
       << std::make_pair(K(0), V(1003));

    const Container c2a(l2.begin(), l2.end());
    const Container c2b(l2.cbegin(), l2.cend());

    // from std::list
    std::list<std::pair<K, V>> l3;
    l3.push_back(std::make_pair(K(0), V(1000)));
    l3.push_back(std::make_pair(K(1), V(1001)));
    l3.push_back(std::make_pair(K(2), V(1002)));
    l3.push_back(std::make_pair(K(0), V(1003)));
    const Container c3a(l3.begin(), l3.end());

    // from const std::list
    const std::list<std::pair<K, V>> l3c = l3;
    const Container c3b(l3c.begin(), l3c.end());

    // from itself
    const Container c4(reference.begin(), reference.end());

    QCOMPARE(c1,  reference);
    QCOMPARE(c2a, reference);
    QCOMPARE(c2b, reference);
    QCOMPARE(c3a, reference);
    QCOMPARE(c3b, reference);
    QCOMPARE(c4,  reference);
}

template <typename Container>
Container make(int size)
{
    Container c;
    int i = 1;
    while (size--)
        c.push_back(typename Container::value_type(i++));
    return c;
}

static QString s_string = QStringLiteral("\1\2\3\4\5\6\7");

template <> QStringRef    make(int size) { return s_string.leftRef(size); }
template <> QStringView   make(int size) { return QStringView(s_string).left(size); }
template <> QLatin1String make(int size) { return QLatin1String("\1\2\3\4\5\6\7", size); }

template <typename T> T clean(T &&t) { return std::forward<T>(t); }
inline QChar clean(QCharRef ch) { return ch; }
inline char clean(QByteRef ch) { return ch; }
inline char clean(QLatin1Char ch) { return ch.toLatin1(); }

template <typename Container>
void tst_ContainerApiSymmetry::front_back_impl() const
{
    using V = typename Container::value_type;
    auto c1 = make<Container>(1);
    QCOMPARE(clean(c1.front()), V(1));
    QCOMPARE(clean(c1.back()), V(1));
    QCOMPARE(clean(qAsConst(c1).front()), V(1));
    QCOMPARE(clean(qAsConst(c1).back()), V(1));

    auto c2 = make<Container>(2);
    QCOMPARE(clean(c2.front()), V(1));
    QCOMPARE(clean(c2.back()), V(2));
    QCOMPARE(clean(qAsConst(c2).front()), V(1));
    QCOMPARE(clean(qAsConst(c2).back()), V(2));
}

#if QT_DEPRECATED_SINCE(5, 15)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void tst_ContainerApiSymmetry::ranged_ctor_QLinkedList_int()
{
    ranged_ctor_non_associative_impl<QLinkedList<int>>();
}

void tst_ContainerApiSymmetry::ranged_ctor_QLinkedList_Movable()
{
    ranged_ctor_non_associative_impl<QLinkedList<Movable>>();
}

void tst_ContainerApiSymmetry::ranged_ctor_QLinkedList_Complex()
{
    ranged_ctor_non_associative_impl<QLinkedList<Complex>>();
}

void tst_ContainerApiSymmetry::ranged_ctor_QLinkedList_duplicates_strategy()
{
    non_associative_container_duplicates_strategy<QLinkedList>();
}

void tst_ContainerApiSymmetry::front_back_QLinkedList()
{
    front_back_impl<QLinkedList<int>>();
}
QT_WARNING_POP
#endif

QTEST_APPLESS_MAIN(tst_ContainerApiSymmetry)
#include "tst_containerapisymmetry.moc"
