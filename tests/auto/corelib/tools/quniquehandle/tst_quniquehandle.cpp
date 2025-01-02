// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/quniquehandle_p.h>

#include <QTest>

QT_USE_NAMESPACE;

// clang-format off
namespace GlobalResource {

std::array<bool, 3> s_resources = { false, false, false };

using handle = size_t;
constexpr handle s_invalidHandle = static_cast<handle>(-1);

handle open()
{
    const auto it = std::find_if(s_resources.begin(), s_resources.end(),
                                 [](bool resource) {
                                     return !resource;
                                 });

    if (it == s_resources.end())
        return s_invalidHandle;

    *it = true;

    return std::distance(s_resources.begin(), it);
}

bool open(handle* dest)
{
    const handle resource = open();

    if (resource == s_invalidHandle)
        return false;

    *dest = resource;
    return true;
}

bool close(handle h)
{
    if (h >= s_resources.size())
        return false; // Invalid handle

    if (!s_resources[h])
        return false; // Un-allocated resource

    s_resources[h] = false;
    return true;
}

bool isOpen(handle h)
{
    return s_resources[h];
}

void reset()
{
    std::fill(s_resources.begin(), s_resources.end(), false);
}

bool isReset()
{
    return std::all_of(s_resources.begin(), s_resources.end(), [](bool res) {
        return !res;
    });
}

} // namespace GlobalResource

struct TestTraits
{
    using Type = GlobalResource::handle;

    static bool close(Type handle) noexcept
    {
        return GlobalResource::close(handle);
    }

    static Type invalidValue() noexcept
    {
        return GlobalResource::s_invalidHandle;
    }
};

using Handle = QUniqueHandle<TestTraits>;

class tst_QUniqueHandle : public QObject
{
    Q_OBJECT

private slots:

    void init() const
    {
        GlobalResource::reset();
    }

    void cleanup() const
    {
        QVERIFY(GlobalResource::isReset());
    }

    void defaultConstructor_initializesToInvalidHandle() const
    {
        const Handle h;
        QCOMPARE_EQ(h.get(), TestTraits::invalidValue());
    }

    void constructor_initializesToValid_whenCalledWithValidHandle() const
    {
        const auto res = GlobalResource::open();

        const Handle h{ res };

        QCOMPARE_EQ(h.get(), res);
    }

    void copyConstructor_and_assignmentOperator_areDeleted() const
    {
        static_assert(!std::is_copy_constructible_v<Handle> && !std::is_copy_assignable_v<Handle>);
    }

    void moveConstructor_movesOwnershipAndResetsSource() const
    {
        Handle source{ GlobalResource::open() };
        const Handle dest{ std::move(source) };

        QVERIFY(!source.isValid());
        QVERIFY(dest.isValid());
        QVERIFY(GlobalResource::isOpen(dest.get()));
    }

    void moveAssignment_movesOwnershipAndResetsSource() const
    {
        Handle source{ GlobalResource::open() };
        Handle dest;
        dest = { std::move(source) };

        QVERIFY(!source.isValid());
        QVERIFY(dest.isValid());
        QVERIFY(GlobalResource::isOpen(dest.get()));
    }

    void moveAssignment_maintainsOwnershipWhenSelfAssigning() const
    {
        Handle resource{ GlobalResource::open() };

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wself-move")
QT_WARNING_DISABLE_CLANG("-Wself-move")
        resource = std::move(resource);
QT_WARNING_POP

        QVERIFY(resource.isValid());
        QVERIFY(GlobalResource::isOpen(resource.get()));
    }

    void isValid_returnsFalse_onlyWhenHandleIsInvalid() const
    {
        const Handle invalid;
        QVERIFY(!invalid.isValid());

        const Handle valid{ GlobalResource::open() };
        QVERIFY(valid.isValid());
    }

    void destructor_callsClose_whenHandleIsValid()
    {
        {
            const Handle h0{ GlobalResource::open() };
            const Handle h1{ GlobalResource::open() };
            const Handle h2{ GlobalResource::open() };
            QVERIFY(!GlobalResource::isReset());
        }

        QVERIFY(GlobalResource::isReset());
    }

    void operatorBool_returnsFalse_onlyWhenHandleIsInvalid() const
    {
        const Handle invalid;
        QVERIFY(!invalid);

        const Handle valid{ GlobalResource::open() };
        QVERIFY(valid);
    }

    void get_returnsValue() const
    {
        const Handle invalid;
        QCOMPARE_EQ(invalid.get(), GlobalResource::s_invalidHandle);

        const auto resource = GlobalResource::open();
        const Handle valid{ resource };
        QCOMPARE_EQ(valid.get(), resource);
    }

    void reset_resetsPreviousValueAndTakesOwnership_whenCalledWithHandle() const
    {
        const auto resource0 = GlobalResource::open();
        const auto resource1 = GlobalResource::open();

        Handle h1{ resource0 };
        h1.reset(resource1);

        QVERIFY(!GlobalResource::isOpen(resource0));
        QVERIFY(GlobalResource::isOpen(resource1));
    }

    void reset_resetsOwningHandleToInvalid_whenCalledWithNoArguments() const
    {
        const auto resource0 = GlobalResource::open();

        Handle h{ resource0 };
        h.reset();

        QVERIFY(!GlobalResource::isOpen(resource0));
        QVERIFY(!h);
    }

    void reset_maintainsOwnership_whenCalledWithOwnedHandle() const
    {
        const auto resource = GlobalResource::open();

        Handle h{ resource };
        h.reset(resource);

        QVERIFY(GlobalResource::isOpen(resource));
        QVERIFY(h);
    }

    void reset_doesNothing_whenCalledOnInvalidHandle() const
    {
        Handle h{ };
        h.reset();

        QVERIFY(!h);
    }

    void release_returnsInvalidResource_whenCalledOnInvalidHandle() const
    {
        Handle h;
        QCOMPARE_EQ(h.release(), GlobalResource::s_invalidHandle);
    }

    void release_releasesOwnershipAndReturnsResource_whenHandleOwnsObject() const
    {
        GlobalResource::handle resource{ GlobalResource::open() };
        GlobalResource::handle released{};
        {
            Handle h{ resource };
            released = h.release();
        }
        QVERIFY(GlobalResource::isOpen(resource));
        QCOMPARE_EQ(resource, released);

        GlobalResource::close(resource);
    }

    void swap_swapsOwnership() const
    {
         { // Swapping valid and invalid handle
            Handle h0{ GlobalResource::open() };
            Handle h1;

            h0.swap(h1);

            QVERIFY(!h0.isValid());
            QVERIFY(h1.isValid());
        }
        { // Swapping valid handles
            const auto resource0 = GlobalResource::open();
            const auto resource1 = GlobalResource::open();

            Handle h0{ resource0 };
            Handle h1{ resource1 };

            h0.swap(h1);

            QCOMPARE_EQ(h0.get(), resource1);
            QCOMPARE_EQ(h1.get(), resource0);
        }
        { // std::swap
            const auto resource0 = GlobalResource::open();
            const auto resource1 = GlobalResource::open();

            Handle h0{ resource0 };
            Handle h1{ resource1 };

            std::swap(h0, h1);

            QCOMPARE_EQ(h0.get(), resource1);
            QCOMPARE_EQ(h1.get(), resource0);
        }
        { // swap
            const auto resource0 = GlobalResource::open();
            const auto resource1 = GlobalResource::open();

            Handle h0{ resource0 };
            Handle h1{ resource1 };

            swap(h0, h1);

            QCOMPARE_EQ(h0.get(), resource1);
            QCOMPARE_EQ(h1.get(), resource0);
        }
    }

    void comparison_behavesAsInt_whenHandleTypeIsInt_data() const
    {
        QTest::addColumn<int>("lhs");
        QTest::addColumn<int>("rhs");

        QTest::addRow("lhs == rhs") << 1 << 1;
        QTest::addRow("lhs < rhs") << 0 << 1;
        QTest::addRow("lhs > rhs") << 1 << 0;
    }

    void comparison_behavesAsInt_whenHandleTypeIsInt() const
    {
        struct IntTraits
        {
            using Type = int;

            static bool close(Type) noexcept
            {
                return true;
            }

            static Type invalidValue() noexcept
            {
                return INT_MAX;
            }
        };

        using Handle = QUniqueHandle<IntTraits>;

        QFETCH(int, lhs);
        QFETCH(int, rhs);

        QCOMPARE_EQ(Handle{ lhs } == Handle{ rhs }, lhs == rhs);
        QCOMPARE_EQ(Handle{ lhs } != Handle{ rhs }, lhs != rhs);
        QCOMPARE_EQ(Handle{ lhs } < Handle{ rhs }, lhs < rhs);
        QCOMPARE_EQ(Handle{ lhs } <= Handle{ rhs }, lhs <= rhs);
        QCOMPARE_EQ(Handle{ lhs } > Handle{ rhs }, lhs > rhs);
        QCOMPARE_EQ(Handle{ lhs } >= Handle{ rhs },  lhs >= rhs);

        QCOMPARE_EQ(Handle{ }, Handle{ });
    }

    void comparison_behavesAsPointer_whenHandleTypeIsPointer_data() const
    {
        QTest::addColumn<int*>("lhs");
        QTest::addColumn<int*>("rhs");

        QTest::addRow("lhs == rhs") << reinterpret_cast<int*>(2) << reinterpret_cast<int*>(2);
        QTest::addRow("lhs < rhs") << reinterpret_cast<int*>(1) << reinterpret_cast<int*>(2);
        QTest::addRow("lhs > rhs") << reinterpret_cast<int*>(2) << reinterpret_cast<int*>(1);
    }

    void comparison_behavesAsPointer_whenHandleTypeIsPointer() const
    {
        struct IntPtrTraits
        {
            using Type = int*;

            static bool close(Type)
            {
                return true;
            }

            static Type invalidValue() noexcept
            {
                return nullptr;
            }
        };

        using Handle = QUniqueHandle<IntPtrTraits>;

        QFETCH(int*, lhs);
        QFETCH(int*, rhs);

        QCOMPARE_EQ(Handle{ lhs } == Handle{ rhs }, lhs == rhs);
        QCOMPARE_EQ(Handle{ lhs } != Handle{ rhs }, lhs != rhs);
        QCOMPARE_EQ(Handle{ lhs } < Handle{ rhs }, lhs < rhs);
        QCOMPARE_EQ(Handle{ lhs } <= Handle{ rhs }, lhs <= rhs);
        QCOMPARE_EQ(Handle{ lhs } > Handle{ rhs }, lhs > rhs);
        QCOMPARE_EQ(Handle{ lhs } >= Handle{ rhs },  lhs >= rhs);

        QCOMPARE_EQ(Handle{ }, Handle{ });
    }

    void sort_sortsHandles() const
    {
        const auto resource0 = GlobalResource::open();
        const auto resource1 = GlobalResource::open();

        QVERIFY(resource1 > resource0); // Precondition of underlying allocator

        Handle h0{ resource0 };
        Handle h1{ resource1 };

        std::vector<Handle> handles;
        handles.push_back(std::move(h1));
        handles.push_back(std::move(h0));

        std::sort(handles.begin(), handles.end());

        QCOMPARE_LT(handles.front(), handles.back());
        QCOMPARE_LT(handles.front().get(), handles.back().get());
    }

    void addressOf_returnsAddressOfHandle() const
    {
        Handle h;
        QVERIFY(GlobalResource::open(&h));
        QVERIFY(h.isValid());
    }

};

// clang-format on
QTEST_MAIN(tst_QUniqueHandle)
#include "tst_quniquehandle.moc"
