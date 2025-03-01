// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <private/qcomvariant_p.h>
#include <private/qcomobject_p.h>
#include <QtCore/private/qcomptr_p.h>

using Microsoft::WRL::ComPtr;

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;

struct IUnknownStub : QComObject<IUnknown>
{
    ULONG refCount()
    {
        AddRef();
        return Release();
    }
};

extern "C" {
void __cdecl SetOaNoCache(void);
}

class tst_qcomvariant : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase()
    {
        SetOaNoCache();
    }

private slots:

    static void constructor_initializesToEmpty()
    {
        const QComVariant var;
        QCOMPARE_EQ(var.get().vt, VT_EMPTY);
    }

    static void constructor_initializes_withBool()
    {
        QComVariant falseVal{ false };
        QCOMPARE_EQ(falseVal.get().vt, VT_BOOL);
        QCOMPARE_EQ(falseVal.get().boolVal, VARIANT_FALSE);

        QComVariant trueVal{ true };
        QCOMPARE_EQ(trueVal.get().vt, VT_BOOL);
        QCOMPARE_EQ(trueVal.get().boolVal, VARIANT_TRUE);
    }

    static void constructor_initializes_withInt()
    {
        QComVariant var{ 3 };
        QCOMPARE_EQ(var.get().vt, VT_INT);
        QCOMPARE_EQ(var.get().intVal, 3);
    }

    static void constructor_initializes_withLong()
    {
        QComVariant var{ 4l };
        QCOMPARE_EQ(var.get().vt, VT_I4);
        QCOMPARE_EQ(var.get().lVal, 4);
    }

    static void constructor_initializes_withDouble()
    {
        QComVariant var{ 3.14 };
        QCOMPARE_EQ(var.get().vt, VT_R8);
        QVERIFY(qFuzzyCompare(var.get().dblVal, 3.14));
    }

    static void constructor_initializes_withQBStr()
    {
        QComVariant var{ QBStr{ L"Hello world!" } };
        QCOMPARE_EQ(var.get().vt, VT_BSTR);
        QCOMPARE_EQ(var.get().bstrVal, QStringView(L"Hello world!"));
    }

    static void constructor_initializesWithNullQBStr()
    {
        QComVariant var{ QBStr{ nullptr } };
        QCOMPARE_EQ(var.get().vt, VT_BSTR);
        QCOMPARE_EQ(var.get().bstrVal, nullptr);
    }

    static void constructor_initializes_withQString()
    {
        QComVariant var{ "Hello world!"_L1 };
        QCOMPARE_EQ(var.get().vt, VT_BSTR);
        QCOMPARE_EQ(var.get().bstrVal, QStringView(L"Hello world!"));
    }

    static void constructor_initializesWithNullQString()
    {
        QComVariant var{ QString{} };
        QCOMPARE_EQ(var.get().vt, VT_BSTR);
        QCOMPARE_EQ(var.get().bstrVal, nullptr);
    }

    static void constructor_initializesWithEmptyQString()
    {
        QComVariant var{ QString{ ""_L1 } };
        QCOMPARE_EQ(var.get().vt, VT_BSTR);
        QCOMPARE_EQ(var.get().bstrVal, ""_L1);
    }

    static void constructor_initializes_withIUnknown()
    {
        const ComPtr<IUnknownStub> value = makeComObject<IUnknownStub>();

        QComVariant var{ value };

        QCOMPARE_EQ(var.get().vt, VT_UNKNOWN);
        QCOMPARE_EQ(var.get().punkVal, value.Get());
    }

    static void constructor_initializes_withNullptrIUnknown()
    {
        ComPtr<IUnknown> ptr;
        QComVariant var{ ptr };

        QCOMPARE_EQ(var.get().vt, VT_UNKNOWN);
        QCOMPARE_EQ(var.get().punkVal, nullptr);
    }

    static void addressOfOperator_clearsVariant_whenVariantOwnsIUnknown()
    {
        // Arrange
        ComPtr<IUnknownStub> value = makeComObject<IUnknownStub>();

        QComVariant var{ value };
        QCOMPARE_EQ(value->refCount(), 2);

        // Act
        const VARIANT *ptr = &var;

        // Assert
        QCOMPARE_EQ(value->refCount(), 1);
        QCOMPARE_EQ(ptr->vt, VT_EMPTY);
        QCOMPARE_EQ(ptr->punkVal, nullptr);
    }

    static void destructor_clearsVariant_whenOwningIUnknown()
    {
        // Arrange
        const ComPtr<IUnknownStub> value = makeComObject<IUnknownStub>();

        {
            QComVariant var{ value };
            QCOMPARE_EQ(value->refCount(), 2);
            // Act (run destructor)
        }

        // Assert
        QCOMPARE_EQ(value->refCount(), 1);
    }

    static void destructor_doesNotCrash_whenOwningBSTR()
    {
        // Arrange
        QComVariant var{ QBStr{ L"Hello world!" } };

        // Act (run destructor)
    }

    static void release_releasesOwnershipAndReturnsVariant_whenOwningIUnknown()
    {
        // Arrange
        const ComPtr<IUnknownStub> value = makeComObject<IUnknownStub>();

        VARIANT detached{};
        {
            QComVariant var{ value };
            QCOMPARE_EQ(value->refCount(), 2);

            // Act (Release and run destructor)
            detached = var.release();
        }

        // Assert
        QCOMPARE_EQ(value->refCount(), 2);
        QCOMPARE_EQ(detached.vt, VT_UNKNOWN);
        QCOMPARE_EQ(detached.punkVal, value.Get());

        // Cleanup
        QCOMPARE_EQ(VariantClear(&detached), S_OK);
    }

};

QTEST_MAIN(tst_qcomvariant)
#include "tst_qcomvariant.moc"
