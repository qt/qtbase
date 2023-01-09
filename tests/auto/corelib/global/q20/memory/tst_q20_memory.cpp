// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <q20memory.h>
#include <QtCore/q20memory.h>

#include <QTest>
#include <QObject>
#include <QExplicitlySharedDataPointer>
#include <QSharedDataPointer>

struct Private : QSharedData {};

class tst_q20_memory : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void raw();
    void smart();
    void recursion();
    void usesPointerTraits();
    void prefersPointerTraits();

private Q_SLOTS:
    void to_address_broken_const_propagation_QExplicitlySharedDataPointer()
    { to_address_broken_const_propagation<QExplicitlySharedDataPointer<Private>>(); }
    void to_address_broken_const_propagation_QSharedDataPointer()
    { to_address_broken_const_propagation<QSharedDataPointer<Private>>(); }
    void to_address_broken_const_propagation_shared_ptr()
    { to_address_broken_const_propagation<std::shared_ptr<Private>>(); }
    void to_address_broken_const_propagation_unique_ptr()
    { to_address_broken_const_propagation<std::unique_ptr<Private>>(); }

private:
    template <typename Pointer>
    void to_address_broken_const_propagation();
};

void tst_q20_memory::raw()
{
    auto i = 0;
    auto p = &i;
    QVERIFY(q20::to_address(p) == &i);
}

template <typename T>
class MinimalPtr {
public:
    using element_type = T;

    explicit MinimalPtr(T *d) : d(d) {}

    T *operator->() const noexcept { return d; }

private:
    T *d;
};

void tst_q20_memory::smart()
{
    int i;
    MinimalPtr ptr(&i);
    QCOMPARE_EQ(q20::to_address(ptr), &i);
}

template <typename T>
class RecursivePtr {
public:
    using element_type = T;

    explicit RecursivePtr(T *d) : d(d) {}

    MinimalPtr<T> operator->() const noexcept { return d; }

private:
    MinimalPtr<T> d;
};

void tst_q20_memory::recursion()
{
    int i;
    RecursivePtr ptr(&i);
    QCOMPARE_EQ(q20::to_address(ptr), &i);
}

template <typename T>
class NoDerefOperatorPtr {
public:
    using element_type = T;

    explicit NoDerefOperatorPtr(T *d) : d(d) {}

    T *get() const noexcept { return d; }

private:
    T *d;
};

namespace std {
template <typename T>
struct pointer_traits<NoDerefOperatorPtr<T>>
{
    static T *to_address(const NoDerefOperatorPtr<T> &ptr) noexcept { return ptr.get(); }
};
} // namespace std

void tst_q20_memory::usesPointerTraits()
{
    int i;
    NoDerefOperatorPtr ptr(&i);
    QCOMPARE_EQ(q20::to_address(ptr), &i);
}

template <typename T>
class PrefersPointerTraitsPtr
{
public:
    using element_type = T;

    explicit PrefersPointerTraitsPtr(T *d) : d(d) {}

    T *operator->() const noexcept { return nullptr; }

    T *get() const noexcept { return d; }

private:
    T *d;
};

namespace std {
template <typename T>
struct pointer_traits<PrefersPointerTraitsPtr<T>>
{
    static T *to_address(const PrefersPointerTraitsPtr<T> &p) noexcept { return p.get(); }
};
} // namespace std

void tst_q20_memory::prefersPointerTraits()
{
    int i;
    PrefersPointerTraitsPtr ptr(&i);
    QCOMPARE_EQ(q20::to_address(ptr), &i);
}

template <typename Pointer>
void tst_q20_memory::to_address_broken_const_propagation()
{
    Pointer p(nullptr);
    QCOMPARE_EQ(q20::to_address(p), nullptr);
    p = Pointer{new Private()};
    QCOMPARE_EQ(q20::to_address(p), p.operator->());
    static_assert(std::is_same_v<decltype(q20::to_address(p)),
                                 decltype(std::as_const(p).operator->())>);
}

QTEST_GUILESS_MAIN(tst_q20_memory)
#include "tst_q20_memory.moc"

