// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/private/qcomptr_p.h>
#include <QtCore/private/qcomobject_p.h>

MIDL_INTERFACE("8B06D1DA-94F2-45A5-9E86-77E89A51FAC7")
IMyInterface : IUnknown{};

#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IMyInterface, 0x8b06d1da, 0x94f2, 0x45a5, 0x9e, 0x86, 0x77, 0xe8, 0x9a, 0x51, 0xfa,
                0xc7);
#endif

QT_USE_NAMESPACE

class IMyInterfaceImpl : public QComObject<IMyInterface>
{
};

qsizetype s_instanceCount = 0;

class IUnknownImpl : public QComObject<IUnknown>
{
public:
    IUnknownImpl()
    { //
        ++s_instanceCount;
    }

    ~IUnknownImpl()
    { //
        --s_instanceCount;
    }
};

class IDispatchImpl : public QComObject<IDispatch>
{
public:
    HRESULT GetTypeInfoCount(UINT *) override { return E_FAIL; }
    HRESULT GetTypeInfo(UINT, LCID, ITypeInfo **) override { return E_FAIL; }
    HRESULT GetIDsOfNames(const IID &, LPOLESTR *, UINT, LCID, DISPID *) override { return E_FAIL; }
    HRESULT Invoke(DISPID, const IID &, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *,
                   UINT *) override
    {
        return E_FAIL;
    }
};

class tst_qcomptr : public QObject
{
    Q_OBJECT
private slots:
    void makeComObject_createsComObject()
    {
        // See bug report https://sourceforge.net/p/mingw-w64/bugs/892/
        {
            ComPtr<IUnknown> o = makeComObject<IUnknownImpl>();
            QCOMPARE_EQ(o->AddRef(), 2ul);
            QCOMPARE_EQ(o->Release(), 1ul);
            QVERIFY(o);
        }
        QCOMPARE_EQ(s_instanceCount, 0);
    }

    void comparison_returnsTrue_withEqualInstances()
    {
        {
            // MINGW may lack comparison operator
            ComPtr<IUnknown> o1 = makeComObject<IUnknownImpl>();
            ComPtr<IUnknown> o2 = o1;
            QCOMPARE_EQ(o1, o2);
            QVERIFY(!(o1 != o2));
        }
        {
            //ComPtr<IMyInterface> o1 = makeComObject<IMyInterfaceImpl>();
            //ComPtr<IDispatch> o2 = makeComObject<IDispatchImpl>();
            //QVERIFY(!(o1 == o2)); // Intentionally fails to compile
        }
    }

    void comparison_returnsFalse_withUnequalInstances()
    {
        // MINGW may lack comparison operator
        {
            ComPtr<IUnknown> o1 = makeComObject<IUnknownImpl>();
            ComPtr<IUnknown> o2 = makeComObject<IUnknownImpl>();
            QCOMPARE_NE(o1, o2);
            QVERIFY(!(o1 == o2));
        }
        {
            ComPtr<IUnknown> o1 = makeComObject<IUnknownImpl>();
            ComPtr<IMyInterface> o2 = makeComObject<IMyInterfaceImpl>();
            QCOMPARE_NE(o1, o2);
            QVERIFY(!(o1 == o2));
        }
        {
            //ComPtr<IMyInterface> o1 = makeComObject<IMyInterfaceImpl>();
            //ComPtr<IDispatch> o2 = makeComObject<IDispatchImpl>();
            //QCOMPARE_NE(o1, o2); // Intentionally fails to compile
        }
    }

    void comparison_returnsTrue_whenComparingNullWithNullptr()
    {
        // MINGW may lack comparison operator
        ComPtr<IUnknown> o;
        QCOMPARE_EQ(o, nullptr);
        QCOMPARE_EQ(nullptr, o);
    }

    void comparison_returnsTrue_whenComparingNotNullWithNullptr()
    {
        // MINGW may lack comparison operator
        ComPtr<IUnknown> o = makeComObject<IUnknownImpl>();
        QCOMPARE_NE(o, nullptr);
        QCOMPARE_NE(nullptr, o);
    }

    void lessThan_returnsEqualToRawPointerComparison()
    {
        // MINGW may lack comparison operator
        {
            ComPtr<IUnknown> o1 = makeComObject<IUnknownImpl>();
            ComPtr<IUnknown> o2 = makeComObject<IUnknownImpl>();
            QCOMPARE_EQ(o1 < o2, o1.Get() < o2.Get());
        }
        {
            ComPtr<IUnknown> o1 = makeComObject<IUnknownImpl>();
            ComPtr<IMyInterface> o2 = makeComObject<IMyInterfaceImpl>();
            QCOMPARE_EQ(o1 < o2, o1.Get() < o2.Get());
        }
        {
            //ComPtr<IMyInterface> o1 = makeComObject<IMyInterfaceImpl>();
            //ComPtr<IDispatch> o2 = makeComObject<IDispatchImpl>();
            //QCOMPARE_EQ(o1 < o2, o1.Get() < o2.Get()); // Intentionally fails to compile
        }
    }
};

QTEST_MAIN(tst_qcomptr)
#include "tst_qcomptr.moc"
