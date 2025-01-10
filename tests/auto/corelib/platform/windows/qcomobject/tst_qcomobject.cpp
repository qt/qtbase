// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#ifdef Q_OS_WIN

#  include <private/qcomobject_p.h>

#  include <wrl/client.h>

using Microsoft::WRL::ComPtr;

QT_BEGIN_NAMESPACE

template <typename T, typename... Args>
ComPtr<T> makeComObject(Args &&...args)
{
    ComPtr<T> p;
    // Don't use Attach because of MINGW64 bug
    // #892 Microsoft::WRL::ComPtr::Attach leaks references
    *p.GetAddressOf() = new T(std::forward<Args>(args)...);
    return p;
}

MIDL_INTERFACE("878fab04-7da0-41ea-9c49-058c7fa0d80a")
IIntermediate : public IUnknown{};

MIDL_INTERFACE("65a29ce9-191c-4182-9185-06dd70aafc5d")
IDirect : public IIntermediate{};

class ComImplementation : public QComObject<IDirect>
{
};

MIDL_INTERFACE("d05397e0-da7f-4055-8563-a5b80f095e6c")
IMultipleA : public IUnknown{};

MIDL_INTERFACE("67e298c5-ec5f-4c45-a779-bfba2484e142")
IMultipleB : public IUnknown{};

class MultipleComImplementation : public QComObject<IMultipleA, IMultipleB>
{
};

MIDL_INTERFACE("b8278a1b-0c3b-4bbd-99db-1e8a141483fa")
IOther : public IUnknown{};

#  ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IIntermediate, 0x878fab04, 0x7da0, 0x41ea, 0x9c, 0x49, 0x05, 0x8c, 0x7f, 0xa0, 0xd8,
                0x0a)
__CRT_UUID_DECL(IDirect, 0x65a29ce9, 0x191c, 0x4182, 0x91, 0x85, 0x06, 0xdd, 0x70, 0xaa, 0xfc, 0x5d)
__CRT_UUID_DECL(IMultipleA, 0xd05397e0, 0xda7f, 0x4055, 0x85, 0x63, 0xa5, 0xb8, 0x0f, 0x09, 0x5e,
                0x6c)
__CRT_UUID_DECL(IMultipleB, 0x67e298c5, 0xec5f, 0x4c45, 0xa7, 0x79, 0xbf, 0xba, 0x24, 0x84, 0xe1,
                0x42)
__CRT_UUID_DECL(IOther, 0xb8278a1b, 0x0c3b, 0x4bbd, 0x99, 0xdb, 0x1e, 0x8a, 0x14, 0x14, 0x83, 0xfa)
#  endif

namespace QtPrivate {

template <>
struct QComObjectTraits<IDirect>
{
    static constexpr bool isGuidOf(REFIID riid) noexcept
    {
        return QComObjectTraits<IDirect, IIntermediate>::isGuidOf(riid);
    }
};

} // namespace QtPrivate

class tst_QComObject : public QObject
{
    Q_OBJECT
private slots:
    void QueryInterface_returnsConvertedPointer_whenIUnknownIsRequested();
    void QueryInterface_returnsConvertedPointer_whenDirectParentIsRequested();
    void QueryInterface_returnsConvertedPointer_whenDirectIntermediateIsRequested();
    void QueryInterface_returnsConvertedPointer_whenIUnknownOfMultipleParentsIsRequested();
    void QueryInterface_returnsConvertedPointer_whenFirstOfMultipleParentsIsRequested();
    void QueryInterface_returnsConvertedPointer_whenSecondOfMultipleParentsIsRequested();
    void QueryInterface_returnsNullPointer_whenNonParentIsRequested();
    void QueryInterface_returnsNullPointer_whenNullPointerIsPassedForReceivingObject();
    void QueryInterface_incrementsReferenceCount_whenConvertedPointerIsReturned();
    void AddRef_incrementsReferenceCountByOne();
    void Release_decrementsReferenceCountByOne();
};

void tst_QComObject::QueryInterface_returnsConvertedPointer_whenIUnknownIsRequested()
{
    // Arrange
    const ComPtr<ComImplementation> implementation = makeComObject<ComImplementation>();

    ComPtr<IUnknown> unknown;

    // Act
    const HRESULT queryResult = implementation->QueryInterface(__uuidof(IUnknown), &unknown);

    // Assert
    QCOMPARE(queryResult, S_OK);
    QCOMPARE(unknown.Get(), implementation.Get());
}

void tst_QComObject::QueryInterface_returnsConvertedPointer_whenDirectParentIsRequested()
{
    // Arrange
    const ComPtr<ComImplementation> implementation = makeComObject<ComImplementation>();

    ComPtr<IDirect> direct;

    // Act
    const HRESULT queryResult = implementation->QueryInterface(__uuidof(IDirect), &direct);

    // Assert
    QCOMPARE(queryResult, S_OK);
    QCOMPARE(direct.Get(), implementation.Get());
}

void tst_QComObject::QueryInterface_returnsConvertedPointer_whenDirectIntermediateIsRequested()
{
    // Arrange
    const ComPtr<ComImplementation> implementation = makeComObject<ComImplementation>();

    ComPtr<IIntermediate> intermediate;

    // Act
    const HRESULT queryResult =
            implementation->QueryInterface(__uuidof(IIntermediate), &intermediate);

    // Assert
    QCOMPARE(queryResult, S_OK);
    QCOMPARE(intermediate.Get(), implementation.Get());
}

void tst_QComObject::
        QueryInterface_returnsConvertedPointer_whenIUnknownOfMultipleParentsIsRequested()
{
    // Arrange
    const ComPtr<MultipleComImplementation> implementation =
            makeComObject<MultipleComImplementation>();

    ComPtr<IUnknown> unknown;

    // Act
    const HRESULT queryResult = implementation->QueryInterface(__uuidof(IUnknown), &unknown);

    // Assert
    QCOMPARE(queryResult, S_OK);

    // Cast MultipleComImplementation to IMultipleA to prevent ambiguity
    QCOMPARE(unknown.Get(), static_cast<IMultipleA *>(implementation.Get()));
}

void tst_QComObject::QueryInterface_returnsConvertedPointer_whenFirstOfMultipleParentsIsRequested()
{
    // Arrange
    const ComPtr<MultipleComImplementation> implementation =
            makeComObject<MultipleComImplementation>();

    ComPtr<IMultipleA> multiple;

    // Act
    const HRESULT queryResult = implementation->QueryInterface(__uuidof(IMultipleA), &multiple);

    // Assert
    QCOMPARE(queryResult, S_OK);
    QCOMPARE(multiple.Get(), implementation.Get());
}

void tst_QComObject::QueryInterface_returnsConvertedPointer_whenSecondOfMultipleParentsIsRequested()
{
    // Arrange
    const ComPtr<MultipleComImplementation> implementation =
            makeComObject<MultipleComImplementation>();

    ComPtr<IMultipleB> multiple;

    // Act
    const HRESULT queryResult = implementation->QueryInterface(__uuidof(IMultipleB), &multiple);

    // Assert
    QCOMPARE(queryResult, S_OK);
    QCOMPARE(multiple.Get(), implementation.Get());
}

void tst_QComObject::QueryInterface_returnsNullPointer_whenNonParentIsRequested()
{
    // Arrange
    const ComPtr<ComImplementation> implementation = makeComObject<ComImplementation>();

    ComPtr<IOther> other;

    // Act
    const HRESULT queryResult = implementation->QueryInterface(__uuidof(IOther), &other);

    // Assert
    QCOMPARE(queryResult, E_NOINTERFACE);
    QCOMPARE(other.Get(), nullptr);
}

void tst_QComObject::QueryInterface_returnsNullPointer_whenNullPointerIsPassedForReceivingObject()
{
    // Arrange
    const ComPtr<ComImplementation> implementation = makeComObject<ComImplementation>();

    // Act
    const HRESULT queryResult = implementation->QueryInterface(__uuidof(IUnknown), nullptr);

    // Assert
    QCOMPARE(queryResult, E_POINTER);
}

void tst_QComObject::QueryInterface_incrementsReferenceCount_whenConvertedPointerIsReturned()
{
    // Arrange
    const ComPtr<ComImplementation> implementation = makeComObject<ComImplementation>();

    ComPtr<IUnknown> unknown;

    // Act
    implementation->QueryInterface(__uuidof(IUnknown), &unknown);

    // As there's no any way to get the current reference count of an object, just add one more
    // reference and assert against cumulative reference count value
    const ULONG referenceCount = implementation->AddRef();

    // Assert
    QCOMPARE(referenceCount, 3);
}

void tst_QComObject::AddRef_incrementsReferenceCountByOne()
{
    // Arrange
    const ComPtr<ComImplementation> implementation = makeComObject<ComImplementation>();

    // Act
    const ULONG referenceCount1 = implementation->AddRef();
    const ULONG referenceCount2 = implementation->AddRef();

    // Assert
    QCOMPARE(referenceCount1, 2);
    QCOMPARE(referenceCount2, 3);
}

void tst_QComObject::Release_decrementsReferenceCountByOne()
{
    // Arrange
    const ComPtr<ComImplementation> implementation = makeComObject<ComImplementation>();

    implementation->AddRef();
    implementation->AddRef();

    // Act
    const ULONG referenceCount1 = implementation->Release();
    const ULONG referenceCount2 = implementation->Release();

    // Assert
    QCOMPARE(referenceCount1, 2);
    QCOMPARE(referenceCount2, 1);
}

QTEST_MAIN(tst_QComObject)
#  include "tst_qcomobject.moc"

QT_END_NAMESPACE

#endif // Q_OS_WIN
