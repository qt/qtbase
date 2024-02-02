// Copyright (C) 2023 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qxptype_traits.h>

#include <QTest>

class tst_qxp_is_virtual_base_of : public QObject
{
    Q_OBJECT
};

class Base {
public:
    virtual ~Base() {}
};

// Only works with classes
static_assert(!qxp::is_virtual_base_of_v<int, int>);
static_assert(!qxp::is_virtual_base_of_v<int, Base>);
static_assert(!qxp::is_virtual_base_of_v<Base, int>);

// A class isn't a virtual base of itself
static_assert(!qxp::is_virtual_base_of_v<Base, Base>);

// Non-virtual bases
class NonVirtualDerived : public Base {};
class NonVirtualPrivateDerived : private Base {};

static_assert(!qxp::is_virtual_base_of_v<Base, NonVirtualDerived>);
static_assert(!qxp::is_virtual_base_of_v<Base, NonVirtualPrivateDerived>);

static_assert(!qxp::is_virtual_base_of_v<NonVirtualPrivateDerived, NonVirtualDerived>);
static_assert(!qxp::is_virtual_base_of_v<NonVirtualDerived, NonVirtualPrivateDerived>);

static_assert(!qxp::is_virtual_base_of_v<tst_qxp_is_virtual_base_of, QObject>);

// Virtual bases
class VirtualDerived1 : public virtual Base {};
class VirtualDerived2 : public virtual Base {};
class VirtualDerived3 : public VirtualDerived1, public VirtualDerived2 {};
class VirtualDerived4 : public VirtualDerived3, public virtual Base {};
class VirtualPrivateDerived : private virtual Base {};

static_assert(qxp::is_virtual_base_of_v<Base, VirtualDerived1>);
static_assert(qxp::is_virtual_base_of_v<Base, VirtualDerived2>);
static_assert(qxp::is_virtual_base_of_v<Base, VirtualDerived3>);
static_assert(!qxp::is_virtual_base_of_v<VirtualDerived1, VirtualDerived3>);
static_assert(qxp::is_virtual_base_of_v<Base, VirtualDerived4>);
static_assert(qxp::is_virtual_base_of_v<Base, VirtualPrivateDerived>);

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Winaccessible-base")
QT_WARNING_DISABLE_CLANG("-Winaccessible-base")
// Ambiguous non-virtual base
class IntermediateDerived : public Base {};
class AmbiguousBase1 : public IntermediateDerived, public Base {};
class AmbiguousBase2 : public IntermediateDerived, public virtual Base {};

static_assert(!qxp::is_virtual_base_of_v<Base, AmbiguousBase1>);
#ifndef Q_CC_MSVC_ONLY // https://developercommunity.visualstudio.com/t/c-templates-multiple-inheritance-ambiguous-access/185674
static_assert(!qxp::is_virtual_base_of_v<Base, AmbiguousBase2>);
#endif
QT_WARNING_POP

// Const
static_assert(!qxp::is_virtual_base_of_v<      Base, const NonVirtualDerived>);
static_assert(!qxp::is_virtual_base_of_v<const Base,       NonVirtualDerived>);
static_assert(!qxp::is_virtual_base_of_v<const Base, const NonVirtualDerived>);

static_assert(!qxp::is_virtual_base_of_v<      Base, const NonVirtualPrivateDerived>);
static_assert(!qxp::is_virtual_base_of_v<const Base,       NonVirtualPrivateDerived>);
static_assert(!qxp::is_virtual_base_of_v<const Base, const NonVirtualPrivateDerived>);

static_assert(qxp::is_virtual_base_of_v<      Base, const VirtualDerived1>);
static_assert(qxp::is_virtual_base_of_v<const Base,       VirtualDerived1>);
static_assert(qxp::is_virtual_base_of_v<const Base, const VirtualDerived1>);

static_assert(qxp::is_virtual_base_of_v<      Base, const VirtualDerived2>);
static_assert(qxp::is_virtual_base_of_v<const Base,       VirtualDerived2>);
static_assert(qxp::is_virtual_base_of_v<const Base, const VirtualDerived2>);

static_assert(qxp::is_virtual_base_of_v<      Base, const VirtualDerived3>);
static_assert(qxp::is_virtual_base_of_v<const Base,       VirtualDerived3>);
static_assert(qxp::is_virtual_base_of_v<const Base, const VirtualDerived3>);

static_assert(qxp::is_virtual_base_of_v<      Base, const VirtualDerived4>);
static_assert(qxp::is_virtual_base_of_v<const Base,       VirtualDerived4>);
static_assert(qxp::is_virtual_base_of_v<const Base, const VirtualDerived4>);

static_assert(qxp::is_virtual_base_of_v<      Base, const VirtualDerived4>);
static_assert(qxp::is_virtual_base_of_v<const Base,       VirtualDerived4>);
static_assert(qxp::is_virtual_base_of_v<const Base, const VirtualDerived4>);

static_assert(qxp::is_virtual_base_of_v<      Base, const VirtualPrivateDerived>);
static_assert(qxp::is_virtual_base_of_v<const Base,       VirtualPrivateDerived>);
static_assert(qxp::is_virtual_base_of_v<const Base, const VirtualPrivateDerived>);


QTEST_APPLESS_MAIN(tst_qxp_is_virtual_base_of);

#include "tst_is_virtual_base_of.moc"
