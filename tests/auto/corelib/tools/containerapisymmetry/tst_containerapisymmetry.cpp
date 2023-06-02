// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include "qbytearray.h"
#include "qdebug.h"
#include "qhash.h"
#include "qmap.h"
#include "qset.h"
#include "qlist.h"
#include "qstring.h"
#include "qvarlengtharray.h"

#include <algorithm>
#include <functional>
#include <vector> // for reference
#include <iostream>
#include <list>
#include <set>
#include <sstream>
#include <map>
#include <forward_list>
#include <unordered_set>
#include <unordered_map>

#if defined(__cpp_lib_erase_if) && __cpp_lib_erase_if >= 202002L
#  define STDLIB_HAS_UNIFORM_ERASURE
#endif

QT_BEGIN_NAMESPACE
std::ostream &operator<<(std::ostream &os, const QChar &c)
{
    Q_ASSERT(c == QLatin1Char{c.toLatin1()});
    return os << c.toLatin1();
}
std::istream &operator>>(std::istream &os, QChar &c)
{
    char cL1;
    os >> cL1;
    c = QLatin1Char{cL1};
    return os;
}
QT_END_NAMESPACE

struct Movable
{
    explicit Movable(int i = 0) noexcept
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

    friend std::ostream &operator<<(std::ostream &os, const Movable &m)
    { return os << m.i; }
    friend std::istream &operator>>(std::istream &os, Movable &m)
    { return os >> m.i; }
};

int Movable::instanceCount = 0;
bool operator==(Movable lhs, Movable rhs) noexcept { return lhs.i == rhs.i; }
bool operator!=(Movable lhs, Movable rhs) noexcept { return lhs.i != rhs.i; }
bool operator<(Movable lhs, Movable rhs) noexcept { return lhs.i < rhs.i; }

size_t qHash(Movable m, size_t seed = 0) noexcept { return qHash(m.i, seed); }
QDebug &operator<<(QDebug &d, Movable m)
{
    const QDebugStateSaver saver(d);
    return d.nospace() << "Movable(" << m.i << ")";
}

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Movable, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE

struct Complex
{
    explicit Complex(int i = 0) noexcept
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
    constexpr Complex &operator=(const Complex &o) noexcept
    { i = o.i; return *this; }

    int i;
    static int instanceCount;

    friend std::ostream &operator<<(std::ostream &os, const Complex &c)
    { return os << c.i; }
    friend std::istream &operator>>(std::istream &os, Complex &c)
    { return os >> c.i; }
};

int Complex::instanceCount = 0;
bool operator==(Complex lhs, Complex rhs) noexcept { return lhs.i == rhs.i; }
bool operator!=(Complex lhs, Complex rhs) noexcept { return lhs.i != rhs.i; }
bool operator<(Complex lhs, Complex rhs) noexcept { return lhs.i < rhs.i; }

size_t qHash(Complex c, size_t seed = 0) noexcept { return qHash(c.i, seed); }
QDebug &operator<<(QDebug &d, Complex c)
{
    const QDebugStateSaver saver(d);
    return d.nospace() << "Complex(" << c.i << ")";
}


struct DuplicateStrategyTestType
{
    explicit DuplicateStrategyTestType(int i = 0) noexcept
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
bool operator==(DuplicateStrategyTestType lhs, DuplicateStrategyTestType rhs) noexcept
{
    return lhs.i == rhs.i;
}

bool operator!=(DuplicateStrategyTestType lhs, DuplicateStrategyTestType rhs) noexcept
{
    return lhs.i != rhs.i;
}

bool operator<(DuplicateStrategyTestType lhs, DuplicateStrategyTestType rhs) noexcept
{
    return lhs.i < rhs.i;
}

size_t qHash(DuplicateStrategyTestType c, size_t seed = 0) noexcept
{
    return qHash(c.i, seed);
}

bool reallyEqual(DuplicateStrategyTestType lhs, DuplicateStrategyTestType rhs) noexcept
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
    std::size_t operator()(Movable m) const noexcept
    {
        return hash<int>()(m.i);
    }
};

template<>
struct hash<Complex>
{
    std::size_t operator()(Complex m) const noexcept
    {
        return hash<int>()(m.i);
    }
};

template<>
struct hash<DuplicateStrategyTestType>
{
    std::size_t operator()(DuplicateStrategyTestType m) const noexcept
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
    using QVarLengthArray<T>::QVarLengthArray;
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

    void ranged_ctor_QVarLengthArray_int() { ranged_ctor_non_associative_impl<QVarLengthArray<int>>(); }
    void ranged_ctor_QVarLengthArray_Movable() { ranged_ctor_non_associative_impl<QVarLengthArray<Movable>>(); }
    void ranged_ctor_QVarLengthArray_Complex() { ranged_ctor_non_associative_impl<QVarLengthArray<Complex>>(); }
    void ranged_ctor_QVarLengthArray_duplicates_strategy() { non_associative_container_duplicates_strategy<VarLengthArray>(); } // note the VarLengthArray passed

    void ranged_ctor_QList_int() { ranged_ctor_non_associative_impl<QList<int>>(); }
    void ranged_ctor_QList_char() { ranged_ctor_non_associative_impl<QList<char>>(); }
    void ranged_ctor_QList_QChar() { ranged_ctor_non_associative_impl<QList<QChar>>(); }
    void ranged_ctor_QList_Movable() { ranged_ctor_non_associative_impl<QList<Movable>>(); }
    void ranged_ctor_QList_Complex() { ranged_ctor_non_associative_impl<QList<Complex>>(); }
    void ranged_ctor_QList_duplicates_strategy() { non_associative_container_duplicates_strategy<QList>(); }

    void ranged_ctor_std_list_int() { ranged_ctor_non_associative_impl<std::list<int>>(); }
    void ranged_ctor_std_list_Movable() { ranged_ctor_non_associative_impl<std::list<Movable>>(); }
    void ranged_ctor_std_list_Complex() { ranged_ctor_non_associative_impl<std::list<Complex>>(); }
    void ranged_ctor_std_list_duplicates_strategy() { non_associative_container_duplicates_strategy<std::list>(); }

    void ranged_ctor_std_forward_list_int() { ranged_ctor_non_associative_impl<std::forward_list<int>>(); }
    void ranged_ctor_std_forward_list_Movable() {ranged_ctor_non_associative_impl<std::forward_list<Movable>>(); }
    void ranged_ctor_std_forward_list_Complex() { ranged_ctor_non_associative_impl<std::forward_list<Complex>>(); }
    void ranged_ctor_std_forward_list_duplicates_strategy() { non_associative_container_duplicates_strategy<std::forward_list>(); }

    void ranged_ctor_std_set_int() { ranged_ctor_non_associative_impl<std::set<int>>(); }
    void ranged_ctor_std_set_Movable() { ranged_ctor_non_associative_impl<std::set<Movable>>(); }
    void ranged_ctor_std_set_Complex() { ranged_ctor_non_associative_impl<std::set<Complex>>(); }
    void ranged_ctor_std_set_duplicates_strategy() { non_associative_container_duplicates_strategy<std::set>(); }

    void ranged_ctor_std_multiset_int() { ranged_ctor_non_associative_impl<std::multiset<int>>(); }
    void ranged_ctor_std_multiset_Movable() { ranged_ctor_non_associative_impl<std::multiset<Movable>>(); }
    void ranged_ctor_std_multiset_Complex() { ranged_ctor_non_associative_impl<std::multiset<Complex>>(); }
    void ranged_ctor_std_multiset_duplicates_strategy() { non_associative_container_duplicates_strategy<std::multiset>(); }

    void ranged_ctor_std_unordered_set_int() { ranged_ctor_non_associative_impl<std::unordered_set<int>>(); }
    void ranged_ctor_std_unordered_set_Movable() { ranged_ctor_non_associative_impl<std::unordered_set<Movable>>(); }
    void ranged_ctor_std_unordered_set_Complex() { ranged_ctor_non_associative_impl<std::unordered_set<Complex>>(); }
    void ranged_ctor_std_unordered_set_duplicates_strategy() { non_associative_container_duplicates_strategy<std::unordered_set>(); }

    void ranged_ctor_std_unordered_multiset_int() { ranged_ctor_non_associative_impl<std::unordered_multiset<int>>(); }
    void ranged_ctor_std_unordered_multiset_Movable() { ranged_ctor_non_associative_impl<std::unordered_multiset<Movable>>(); }
    void ranged_ctor_std_unordered_multiset_Complex() { ranged_ctor_non_associative_impl<std::unordered_multiset<Complex>>(); }
    void ranged_ctor_std_unordered_multiset_duplicates_strategy() { non_associative_container_duplicates_strategy<std::unordered_multiset>(); }

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

    void ranged_ctor_unordered_map_int() { ranged_ctor_associative_impl<std::unordered_map<int, int>>(); }
    void ranged_ctor_unordered_map_Movable() { ranged_ctor_associative_impl<std::unordered_map<Movable, Movable>>(); }
    void ranged_ctor_unordered_map_Complex() { ranged_ctor_associative_impl<std::unordered_map<Complex, Complex>>(); }

    void ranged_ctor_QHash_int() { ranged_ctor_associative_impl<QHash<int, int>>(); }
    void ranged_ctor_QHash_Movable() { ranged_ctor_associative_impl<QHash<Movable, int>>(); }
    void ranged_ctor_QHash_Complex() { ranged_ctor_associative_impl<QHash<Complex, int>>(); }

    void ranged_ctor_unordered_multimap_int() { ranged_ctor_associative_impl<std::unordered_multimap<int, int>>(); }
    void ranged_ctor_unordered_multimap_Movable() { ranged_ctor_associative_impl<std::unordered_multimap<Movable, Movable>>(); }
    void ranged_ctor_unordered_multimap_Complex() { ranged_ctor_associative_impl<std::unordered_multimap<Complex, Complex>>(); }

    void ranged_ctor_QMultiHash_int() { ranged_ctor_associative_impl<QMultiHash<int, int>>(); }
    void ranged_ctor_QMultiHash_Movable() { ranged_ctor_associative_impl<QMultiHash<Movable, int>>(); }
    void ranged_ctor_QMultiHash_Complex() { ranged_ctor_associative_impl<QMultiHash<Complex, int>>(); }

private:
    template <typename Container>
    void resize_impl() const;

private Q_SLOTS:
    void resize_std_vector() { resize_impl<std::vector<int>>(); }
    void resize_QList() { resize_impl<QList<qintptr>>(); }
    void resize_QVarLengthArray() { resize_impl<QVarLengthArray<int>>(); }
    void resize_QString() { resize_impl<QString>(); }
    void resize_QByteArray() { resize_impl<QByteArray>(); }

private:
    template <typename Container>
    void assign_impl() const;

private Q_SLOTS:
    void assign_std_vector() { assign_impl<std::vector<int>>(); };
    void assign_std_string() { assign_impl<std::string>(); }
    void assign_QVarLengthArray() { assign_impl<QVarLengthArray<int, 4>>(); };
    void assign_QList() { assign_impl<QList<int>>(); }
    void assign_QByteArray() { assign_impl<QByteArray>(); }
    void assign_QString() { assign_impl<QString>(); }

private:
    template <typename Container>
    void front_back_impl() const;

private Q_SLOTS:
    void front_back_std_vector() { front_back_impl<std::vector<int>>(); }
    void front_back_QList() { front_back_impl<QList<qintptr>>(); }
    void front_back_QVarLengthArray() { front_back_impl<QVarLengthArray<int>>(); }
    void front_back_QString() { front_back_impl<QString>(); }
    void front_back_QStringView() { front_back_impl<QStringView>(); }
    void front_back_QLatin1String() { front_back_impl<QLatin1String>(); }
    void front_back_QByteArray() { front_back_impl<QByteArray>(); }

private:
    template <typename Container>
    void erase_impl() const;

    template <typename Container>
    void erase_if_impl() const;

    template <typename Container>
    void erase_if_associative_impl() const;

    template <typename Container>
    void member_erase_impl() const;

    template <typename Container>
    void member_erase_associative_impl() const;

    template <typename Container>
    void member_erase_set_impl() const;

private Q_SLOTS:
    void erase_QList() { erase_impl<QList<int>>(); }
    void erase_QVarLengthArray() { erase_impl<QVarLengthArray<int>>(); }
    void erase_QString() { erase_impl<QString>(); }
    void erase_QByteArray() { erase_impl<QByteArray>(); }
    void erase_std_vector() {
#ifdef STDLIB_HAS_UNIFORM_ERASURE
        erase_impl<std::vector<int>>();
#endif
    }

    void erase_if_QList() { erase_if_impl<QList<int>>(); }
    void erase_if_QVarLengthArray() { erase_if_impl<QVarLengthArray<int>>(); }
    void erase_if_QSet() { erase_if_impl<QSet<int>>(); }
    void erase_if_QString() { erase_if_impl<QString>(); }
    void erase_if_QByteArray() { erase_if_impl<QByteArray>(); }
    void erase_if_std_vector() {
#ifdef STDLIB_HAS_UNIFORM_ERASURE
        erase_if_impl<std::vector<int>>();
#endif
    }
    void erase_if_QMap() { erase_if_associative_impl<QMap<int, int>>(); }
    void erase_if_QMultiMap() {erase_if_associative_impl<QMultiMap<int, int>>(); }
    void erase_if_QHash() { erase_if_associative_impl<QHash<int, int>>(); }
    void erase_if_QMultiHash() { erase_if_associative_impl<QMultiHash<int, int>>(); }

    void member_erase_QList() { member_erase_impl<QList<int>>(); }
    void member_erase_QVarLengthArray() { member_erase_impl<QVarLengthArray<int>>(); }
    void member_erase_QString() { member_erase_impl<QString>(); }
    void member_erase_QByteArray() { member_erase_impl<QByteArray>(); }
    void member_erase_QSet() { member_erase_set_impl<QSet<int>>(); }

    void member_erase_QMap() { member_erase_associative_impl<QMap<int, int>>(); }
    void member_erase_QMultiMap() {member_erase_associative_impl<QMultiMap<int, int>>(); }
    void member_erase_QHash() { member_erase_associative_impl<QHash<int, int>>(); }
    void member_erase_QMultiHash() { member_erase_associative_impl<QMultiHash<int, int>>(); }

private:
    template <typename Container>
    void keyValueRange_impl() const;

private Q_SLOTS:
    void keyValueRange_QMap() { keyValueRange_impl<QMap<int, int>>(); }
    void keyValueRange_QMultiMap() { keyValueRange_impl<QMultiMap<int, int>>(); }
    void keyValueRange_QHash() { keyValueRange_impl<QHash<int, int>>(); }
    void keyValueRange_QMultiHash() { keyValueRange_impl<QMultiHash<int, int>>(); }
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

    // from stringsteam (= pure input_iterator)
    const Container c5 = [&] {
        {
            std::stringstream ss;
            for (auto &v : values1)
                ss << v << ' ';
            ss.seekg(0);
            return Container(std::istream_iterator<V>{ss},
                             std::istream_iterator<V>{});
        }
    }();

    QCOMPARE(c1,  reference);
    QCOMPARE(c2a, reference);
    QCOMPARE(c2b, reference);
    QCOMPARE(c3a, reference);
    QCOMPARE(c3b, reference);
    QCOMPARE(c4,  reference);
    QCOMPARE(c5,  reference);
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
struct ContainerDuplicatedValuesStrategy<QList<T...>> : ContainerAcceptsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<QVarLengthArray<T...>> : ContainerAcceptsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<VarLengthArray<T...>> : ContainerAcceptsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::list<T...>> : ContainerAcceptsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::forward_list<T...>> : ContainerAcceptsDuplicateValues {};

// assuming https://cplusplus.github.io/LWG/lwg-active.html#2844 resolution
template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::set<T...>> : ContainerRejectsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::multiset<T...>> : ContainerAcceptsDuplicateValues {};

// assuming https://cplusplus.github.io/LWG/lwg-active.html#2844 resolution
template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::unordered_set<T...>> : ContainerRejectsDuplicateValues {};

template<typename ... T>
struct ContainerDuplicatedValuesStrategy<std::unordered_multiset<T...>> : ContainerAcceptsDuplicateValues {};

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
    c.reserve(size);
    using V = typename Container::value_type;
    int i = 0;
    std::generate_n(std::inserter(c, c.end()), size, [&i] { return V(++i); });
    return c;
}

template <typename Container>
Container makeAssociative(int size)
{
    using K = typename Container::key_type;
    using V = typename Container::mapped_type;
    Container c;
    for (int i = 1; i <= size; ++i)
        c.insert(K(i), V(i));
    return c;
}

static QString s_string = QStringLiteral("\1\2\3\4\5\6\7");

template <> QString       make(int size) { return s_string.left(size); }
template <> QStringView   make(int size) { return QStringView(s_string).left(size); }
template <> QLatin1String make(int size) { return QLatin1String("\1\2\3\4\5\6\7", size); }
template <> QByteArray    make(int size) { return QByteArray("\1\2\3\4\5\6\7", size); }

template <typename T> T clean(T &&t) { return std::forward<T>(t); }
inline char clean(QLatin1Char ch) { return ch.toLatin1(); }

template <typename Container>
void tst_ContainerApiSymmetry::resize_impl() const
{
    using V = typename Container::value_type;
    using S = typename Container::size_type;
    auto c = make<Container>(3);
    QCOMPARE(c.size(), S(3));
    c.resize(4, V(5));
    QCOMPARE(c.size(), S(4));
    QCOMPARE(c.back(), V(5));

    // ctor/resize symmetry:
    {
        Container c1(S(5), V(4));
        QCOMPARE(c1.size(), S(5));

        Container c2;
        c2.resize(S(5), V(4));
        QCOMPARE(c2.size(), S(5));

        QCOMPARE(c1, c2);
    }
}

template <typename Container>
void tst_ContainerApiSymmetry::assign_impl() const
{
#define CHECK(Arr, ComparisonData, Sz_n, Sz_e)               \
    QCOMPARE(Sz_n, Sz_e);                                    \
    for (const auto &e : Arr)                                \
        QCOMPARE(e, ComparisonData)                          \
    /*end*/
#define RET_CHECK(...)                                           \
    do {                                                         \
        if constexpr (std::is_void_v<decltype( __VA_ARGS__ )>) { \
            /* e.g. std::vector */                               \
            __VA_ARGS__ ;                                        \
        } else {                                                 \
            /* e.g. std::basic_string */                         \
            auto &&r = __VA_ARGS__ ;                             \
            QCOMPARE_EQ(&r, &c);                                 \
        }                                                        \
    } while (false)                                              \
    /* end */
    using V = typename Container::value_type;
    using S = typename Container::size_type;
    auto tData = V(65);
    {
        // fill version
        auto c = make<Container>(4);
        const S oldCapacity = c.capacity();
        RET_CHECK(c.assign(4, tData));
        CHECK(c, tData, c.size(), S(4));
        QCOMPARE_EQ(c.capacity(), oldCapacity);

        tData = V(66);
        c.assign(8, tData); // may reallocate
        CHECK(c, tData, c.size(), S(8));

        const S grownCapacity = c.capacity();
        c.assign(0, tData);
        CHECK(c, tData, c.size(), S(0));
        QCOMPARE_EQ(c.capacity(), grownCapacity);
    }
    {
        // range version for non input iterator
        auto c = make<Container>(4);
        auto iter = make<Container>(1);

        iter.assign(8, tData);
        RET_CHECK(c.assign(iter.begin(), iter.end())); // may reallocate
        CHECK(c, tData, c.size(), S(8));

        const S oldCapacity = c.capacity();
        c.assign(iter.begin(), iter.begin());
        CHECK(c, tData, c.size(), S(0));
        QCOMPARE_EQ(c.capacity(), oldCapacity);
    }
    {
        // range version for input iterator
        auto c = make<Container>(4);
        const S oldCapacity = c.capacity();

        std::stringstream ss;
        ss << tData << ' ' << tData << ' ';
        RET_CHECK(c.assign(std::istream_iterator<V>{ss}, std::istream_iterator<V>{}));
        CHECK(c, tData, c.size(), S(2));
        QCOMPARE_EQ(c.capacity(), oldCapacity);

        ss.str("");
        ss.clear();
        tData = V(66);
        ss << tData << ' ' << tData << ' ' << tData << ' ' << tData << ' ';
        c.assign(std::istream_iterator<V>{ss}, std::istream_iterator<V>{});
        CHECK(c, tData, c.size(), S(4));
        QCOMPARE_EQ(c.capacity(), oldCapacity);

        ss.str("");
        ss.clear();
        tData = V(67);
        ss << tData << ' ' << tData << ' ' << tData << ' ' << tData << ' '
           << tData << ' ' << tData << ' ' << tData << ' ';
        c.assign(std::istream_iterator<V>{ss}, std::istream_iterator<V>{}); // may reallocate
        CHECK(c, tData, c.size(), S(7));
    }
    {
        // initializer-list version
        auto c = make<Container>(4);
        const S oldCapacity = c.capacity();
        std::initializer_list<V> list = {tData, tData, tData};
        RET_CHECK(c.assign(list));
        CHECK(c, tData, c.size(), S(3));
        QCOMPARE_EQ(c.capacity(), oldCapacity);
    }

#undef RET_CHECK
#undef CHECK
}

template<typename Container>
void tst_ContainerApiSymmetry::front_back_impl() const
{
    using V = typename Container::value_type;
    auto c1 = make<Container>(1);
    QCOMPARE(clean(c1.front()), V(1));
    QCOMPARE(clean(c1.back()), V(1));
    QCOMPARE(clean(std::as_const(c1).front()), V(1));
    QCOMPARE(clean(std::as_const(c1).back()), V(1));

    auto c2 = make<Container>(2);
    QCOMPARE(clean(c2.front()), V(1));
    QCOMPARE(clean(c2.back()), V(2));
    QCOMPARE(clean(std::as_const(c2).front()), V(1));
    QCOMPARE(clean(std::as_const(c2).back()), V(2));
}

namespace {
struct Conv {
    template <typename T>
    static int toInt(T i) { return i; }
    static int toInt(QChar ch) { return ch.unicode(); }
};
}

template <typename Container>
void tst_ContainerApiSymmetry::erase_impl() const
{
    using S = typename Container::size_type;
    using V = typename Container::value_type;
    auto c = make<Container>(7); // {1, 2, 3, 4, 5, 6, 7}
    QCOMPARE(c.size(), S(7));

    auto result = erase(c, V(1));
    QCOMPARE(result, S(1));
    QCOMPARE(c.size(), S(6));

    result = erase(c, V(5));
    QCOMPARE(result, S(1));
    QCOMPARE(c.size(), S(5));

    result = erase(c, V(123));
    QCOMPARE(result, S(0));
    QCOMPARE(c.size(), S(5));
}

template <typename Container>
void tst_ContainerApiSymmetry::erase_if_impl() const
{
    using S = typename Container::size_type;
    using V = typename Container::value_type;
    auto c = make<Container>(7); // {1, 2, 3, 4, 5, 6, 7}
    QCOMPARE(c.size(), S(7));

    decltype(c.size()) oldSize, count;

    oldSize = c.size();
    count = 0;
    auto result = erase_if(c, [&](V i) { ++count; return Conv::toInt(i) % 2 == 0; });
    QCOMPARE(result, S(3));
    QCOMPARE(c.size(), S(4));
    QCOMPARE(count, oldSize);

    oldSize = c.size();
    count = 0;
    result = erase_if(c, [&](V i) { ++count; return Conv::toInt(i) % 123 == 0; });
    QCOMPARE(result, S(0));
    QCOMPARE(c.size(), S(4));
    QCOMPARE(count, oldSize);

    oldSize = c.size();
    count = 0;
    result = erase_if(c, [&](V i) { ++count; return Conv::toInt(i) % 3 == 0; });
    QCOMPARE(result, S(1));
    QCOMPARE(c.size(), S(3));
    QCOMPARE(count, oldSize);

    oldSize = c.size();
    count = 0;
    result = erase_if(c, [&](V i) { ++count; return Conv::toInt(i) % 2 == 1; });
    QCOMPARE(result, S(3));
    QCOMPARE(c.size(), S(0));
    QCOMPARE(count, oldSize);
}

template <typename Container>
void tst_ContainerApiSymmetry::erase_if_associative_impl() const
{
    using S = typename Container::size_type;
    using K = typename Container::key_type;
    using V = typename Container::mapped_type;
    using I = typename Container::iterator;
    using P = std::pair<const K &, V &>;

    auto c = makeAssociative<Container>(20);
    QCOMPARE(c.size(), S(20));

    auto result = erase_if(c, [](const P &p) { return Conv::toInt(p.first) % 2 == 0; });
    QCOMPARE(result, S(10));
    QCOMPARE(c.size(), S(10));

    result = erase_if(c, [](const P &p) { return Conv::toInt(p.first) % 3 == 0; });
    QCOMPARE(result, S(3));
    QCOMPARE(c.size(), S(7));

    result = erase_if(c, [](const P &p) { return Conv::toInt(p.first) % 42 == 0; });
    QCOMPARE(result, S(0));
    QCOMPARE(c.size(), S(7));

    result = erase_if(c, [](const P &p) { return Conv::toInt(p.first) % 2 == 1; });
    QCOMPARE(result, S(7));
    QCOMPARE(c.size(), S(0));

    // same, but with a predicate taking a Qt iterator
    c = makeAssociative<Container>(20);
    QCOMPARE(c.size(), S(20));

    result = erase_if(c, [](const I &it) { return Conv::toInt(it.key()) % 2 == 0; });
    QCOMPARE(result, S(10));
    QCOMPARE(c.size(), S(10));

    result = erase_if(c, [](const I &it) { return Conv::toInt(it.key()) % 3 == 0; });
    QCOMPARE(result, S(3));
    QCOMPARE(c.size(), S(7));

    result = erase_if(c, [](const I &it) { return Conv::toInt(it.key()) % 42 == 0; });
    QCOMPARE(result, S(0));
    QCOMPARE(c.size(), S(7));

    result = erase_if(c, [](const I &it) { return Conv::toInt(it.key()) % 2 == 1; });
    QCOMPARE(result, S(7));
    QCOMPARE(c.size(), S(0));
}

template <typename Container>
void tst_ContainerApiSymmetry::member_erase_impl() const
{
    using S = typename Container::size_type;
    using V = typename Container::value_type;
    const S size = 7;
    auto c = make<Container>(size); // {1, 2, 3, 4, 5, 6, 7}
    QCOMPARE(c.size(), size);

    auto copy = c;
    // Container::erase() returns an iterator, not const_iterator
    auto it = c.erase(c.cbegin(), c.cbegin());
    static_assert(std::is_same_v<decltype(it), typename Container::iterator>);
    QCOMPARE(c.size(), size);
    const V newVal{100};
    QCOMPARE_NE(*it, newVal);
    *it = newVal;
    QCOMPARE(it, c.cbegin());
    QCOMPARE(*c.cbegin(), newVal);

    QCOMPARE(std::find(copy.cbegin(), copy.cend(), newVal), copy.cend());
}

template <typename Container>
void tst_ContainerApiSymmetry::member_erase_associative_impl() const
{
    using S = typename Container::size_type;
    using V = typename Container::mapped_type;

    const S size = 20;
    auto c = makeAssociative<Container>(size);
    QCOMPARE(c.size(), size);

    // Verify Container::erase() returns iterator, not const_iterator
    auto it = c.erase(c.cbegin());
    static_assert(std::is_same_v<decltype(it), typename Container::iterator>);
    QCOMPARE(c.size(), size - 1);
    QCOMPARE(it, c.cbegin());
    const auto current = it.value();
    it.value() = current + V(5);
    QCOMPARE(c.cbegin().value(),current + V(5));
}

template <typename Container>
void tst_ContainerApiSymmetry::member_erase_set_impl() const
{
    using S = typename Container::size_type;

    const S size = 20;
    auto c = make<Container>(size);
    QCOMPARE(c.size(), size);

    // Verify Container::erase() returns iterator, not const_iterator
    auto it = c.erase(c.cbegin());
    static_assert(std::is_same_v<decltype(it), typename Container::iterator>);
    QCOMPARE(c.size(), size - 1);
    QCOMPARE(it, c.cbegin());
}

template <typename Container>
void tst_ContainerApiSymmetry::keyValueRange_impl() const
{
    constexpr int COUNT = 20;

    using K = typename Container::key_type;
    using V = typename Container::mapped_type;
    QVector<K> keys;
    keys.reserve(COUNT);
    QVector<V> values;
    values.reserve(COUNT);

    auto c = makeAssociative<Container>(COUNT);
    auto returnC = [&](){ return c; };

    const auto verify = [](QVector<K> v, int count, int offset = 0) -> bool {
        if (v.size() != count)
            return false;
        std::sort(v.begin(), v.end());
        for (int i = 0; i < count; ++i) {
            // vector is indexed from 0, but makeAssociative starts from 1
            if (v[i] != i + 1 + offset)
                return false;
        }
        return true;
    };

    // Check that the range has the right size
    auto range = c.asKeyValueRange();
    QCOMPARE(std::distance(range.begin(), range.end()), COUNT);

    auto constRange = std::as_const(c).asKeyValueRange();
    QCOMPARE(std::distance(constRange.begin(), constRange.end()), COUNT);

    auto rvalueRange = returnC().asKeyValueRange();
    QCOMPARE(std::distance(rvalueRange.begin(), rvalueRange.end()), COUNT);

    // auto, mutating
    keys.clear(); values.clear();
    for (auto [key, value] : c.asKeyValueRange()) {
        keys << key;
        values << value;
        QCOMPARE(key, value);
        QCOMPARE(c.value(key), value);
        ++value;
        QCOMPARE(key, value - 1);
        QCOMPARE(c.value(key), value);
    }
    QVERIFY(verify(keys, COUNT));
    QVERIFY(verify(values, COUNT));

    // auto, non-mutating
    keys.clear(); values.clear();
    for (auto [key, value] : c.asKeyValueRange()) {
        keys << key;
        values << value;
        QCOMPARE(key, value - 1);
        QCOMPARE(c.value(key), value);
    }
    QVERIFY(verify(keys, COUNT));
    QVERIFY(verify(values, COUNT, 1));

    // auto &&, mutating
    keys.clear(); values.clear();
    for (auto &&[key, value] : c.asKeyValueRange()) {
        keys << key;
        values << value;
        QCOMPARE(key, value - 1);
        QCOMPARE(c.value(key), value);
        ++value;
        QCOMPARE(key, value - 2);
        QCOMPARE(c.value(key), value);
    }
    QVERIFY(verify(keys, COUNT));
    QVERIFY(verify(values, COUNT, 1));

    // auto, non-mutating (const map)
    keys.clear(); values.clear();
    for (auto [key, value] : std::as_const(c).asKeyValueRange()) {
        keys << key;
        values << value;
        QCOMPARE(key, value - 2);
        QCOMPARE(c.value(key), value);
    }
    QVERIFY(verify(keys, COUNT));
    QVERIFY(verify(values, COUNT, 2));

    // auto &&, non-mutating (const map)
    keys.clear(); values.clear();
    for (auto &&[key, value] : std::as_const(c).asKeyValueRange()) {
        keys << key;
        values << value;
        QCOMPARE(key, value - 2);
        QCOMPARE(c.value(key), value);
    }
    QVERIFY(verify(keys, COUNT));
    QVERIFY(verify(values, COUNT, 2));

    // auto, non-mutating (rvalue map)
    keys.clear(); values.clear();
    for (auto [key, value] : returnC().asKeyValueRange()) {
        keys << key;
        values << value;
        QCOMPARE(key, value - 2);
        QCOMPARE(c.value(key), value);
    }
    QVERIFY(verify(keys, COUNT));
    QVERIFY(verify(values, COUNT, 2));

    // auto &&, non-mutating (rvalue map)
    keys.clear(); values.clear();
    for (auto &&[key, value] : returnC().asKeyValueRange()) {
        keys << key;
        values << value;
        QCOMPARE(key, value - 2);
        QCOMPARE(c.value(key), value);
    }
    QVERIFY(verify(keys, COUNT));
    QVERIFY(verify(values, COUNT, 2));
}

QTEST_APPLESS_MAIN(tst_ContainerApiSymmetry)
#include "tst_containerapisymmetry.moc"
