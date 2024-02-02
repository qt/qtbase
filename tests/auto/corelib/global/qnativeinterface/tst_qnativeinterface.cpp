// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qnativeinterface.h>
#include <QtCore/private/qnativeinterface_p.h>

class tst_QNativeInterface: public QObject
{
    Q_OBJECT
private slots:
    void typeInfo() const;
    void resolve() const;
    void accessor() const;

    friend struct PublicClass;
};

struct InterfaceImplementation;

struct PublicClass
{
    PublicClass();
    QT_DECLARE_NATIVE_INTERFACE_ACCESSOR(PublicClass)
    std::unique_ptr<InterfaceImplementation> m_implementation;

    friend void tst_QNativeInterface::resolve() const;
};

QT_BEGIN_NAMESPACE
namespace QNativeInterface {
struct Interface
{
    QT_DECLARE_NATIVE_INTERFACE(Interface, 10, PublicClass)
    virtual int foo() = 0;
};

struct OtherInterface
{
    QT_DECLARE_NATIVE_INTERFACE(OtherInterface, 10, PublicClass)
};
}

QT_DEFINE_NATIVE_INTERFACE(Interface);
QT_DEFINE_NATIVE_INTERFACE(OtherInterface);
QT_END_NAMESPACE

struct NotInterface {};

struct AlmostInterface
{
    struct TypeInfo {
        // Missing required members
    };
};

using namespace QNativeInterface;

struct InterfaceImplementation : public Interface
{
    int foo() override { return 123; }
};

PublicClass::PublicClass() : m_implementation(new InterfaceImplementation) {}

void* PublicClass::resolveInterface(char const* name, int revision) const
{
    auto *implementation = m_implementation.get();
    QT_NATIVE_INTERFACE_RETURN_IF(Interface, implementation);
    QT_NATIVE_INTERFACE_RETURN_IF(OtherInterface, implementation);
    return nullptr;
}

void tst_QNativeInterface::typeInfo() const
{
    using namespace QNativeInterface::Private;

    QCOMPARE(TypeInfo<Interface>::haveTypeInfo, true);
    QCOMPARE(TypeInfo<NotInterface>::haveTypeInfo, false);
    QCOMPARE(TypeInfo<AlmostInterface>::haveTypeInfo, false);

    QCOMPARE(TypeInfo<Interface>::isCompatibleWith<PublicClass>, true);
    QCOMPARE(TypeInfo<Interface>::isCompatibleWith<QObject>, false);
    QCOMPARE(TypeInfo<Interface>::isCompatibleWith<int>, false);

    QCOMPARE(TypeInfo<Interface>::revision(), 10);
    QCOMPARE(TypeInfo<Interface>::name(), "Interface");
}

void tst_QNativeInterface::resolve() const
{
    using namespace QNativeInterface::Private;

    PublicClass foo;

    QVERIFY(foo.resolveInterface("Interface", 10));

    QTest::ignoreMessage(QtWarningMsg, "Native interface revision mismatch "
                    "(requested 5 / available 10) for interface Interface");

    QCOMPARE(foo.resolveInterface("Interface", 5), nullptr);
    QCOMPARE(foo.resolveInterface("NotInterface", 10), nullptr);
    QCOMPARE(foo.resolveInterface("OtherInterface", 10), nullptr);
}

void tst_QNativeInterface::accessor() const
{
    PublicClass foo;
    QVERIFY(foo.nativeInterface<Interface>());
    QCOMPARE(foo.nativeInterface<Interface>()->foo(), 123);
}

QTEST_MAIN(tst_QNativeInterface)
#include "tst_qnativeinterface.moc"
