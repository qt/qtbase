// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUASIVIRTUAL_IMPL_H
#define QQUASIVIRTUAL_IMPL_H

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qscopeguard.h>
#include <QtCore/qassert.h>
#include <QtCore/qtclasshelpermacros.h>

#include <algorithm>
#include <QtCore/q20memory.h>
#include <type_traits>
#include <tuple>

#ifndef Q_QDOC

QT_BEGIN_NAMESPACE

namespace QtPrivate {

template <typename Applier, size_t ...Is>
void applyIndexSwitch(size_t index, Applier&& applier, std::index_sequence<Is...>)
{
    // Performance considerations:
    // The folding expression used here represents the same logic as a sequence of
    // linear if/else if/... statements. Experiments show that Clang, GCC, and MSVC
    // optimize it to essentially the same bytecode as a normal C++ switch,
    // ensuring O(1) lookup complexity.
    static_cast<void>(((Is == index ? (applier(std::integral_constant<size_t, Is>{}), true) : false)
                       || ...));
}

template <size_t IndexCount, typename Applier>
void applyIndexSwitch(size_t index, Applier&& applier)
{
    QtPrivate::applyIndexSwitch(index, std::forward<Applier>(applier),
                                std::make_index_sequence<IndexCount>());
}

template <typename Interface>
class QQuasiVirtualInterface
{
private:
    template <typename Arg>
    static constexpr bool passArgAsValue = sizeof(Arg) <= sizeof(size_t)
                                        && std::is_trivially_destructible_v<Arg>;

    template <typename C = Interface> using Methods = typename C::template MethodTemplates<C>;

    template <typename ...>
    struct MethodImpl;

    template <typename M, typename R, typename I, typename... Args>
    struct MethodImpl<M, R, I, Args...>
    {
        static_assert(std::is_base_of_v<I, Interface>, "The method must belong to the interface");
        using return_type = R;
        using call_args = std::tuple<std::conditional_t<passArgAsValue<Args>, Args, Args&&>...>;

        static constexpr size_t index()
        {
            return index(std::make_index_sequence<std::tuple_size_v<Methods<>>>());
        }

    private:
        template <size_t Ix>
        static constexpr bool matchesAt()
        {
            return std::is_base_of_v<M, std::tuple_element_t<Ix, Methods<>>>;
        }

        template <size_t... Is>
        static constexpr size_t index(std::index_sequence<Is...>)
        {
            constexpr size_t matchesCount = (size_t(matchesAt<Is>()) + ...);
            static_assert(matchesCount == 1, "Expected exactly one match");
            return ((size_t(matchesAt<Is>()) * Is) + ...);
        }

        static R invoke(I &intf /*const validation*/, Args... args)
        {
            Q_ASSERT(intf.m_callFN);

            auto& baseIntf = static_cast<base_interface&>(const_cast<std::remove_const_t<I>&>(intf));
            call_args callArgs(std::forward<Args>(args)...);
            if constexpr (std::is_void_v<R>) {
                intf.m_callFN(index(), baseIntf, nullptr, &callArgs);
            } else {
                alignas(R) std::byte buf[sizeof(R)];
                intf.m_callFN(index(), baseIntf, buf, &callArgs);

                R* result = std::launder(reinterpret_cast<R*>(buf));
                QScopeGuard destroyBuffer([result]() { std::destroy_at(result); });
                return std::forward<R>(*result);
            }
        }

        friend class QQuasiVirtualInterface<Interface>;
    };

    template <typename M, typename R, typename I, typename... Args>
    struct MethodImpl<M, R(I::*)(Args...)> : MethodImpl<M, R, I, Args...> {
        template <typename Subclass>
        using Overridden = R(Subclass::*)(Args...);
    };

    template <typename M, typename R, typename I, typename... Args>
    struct MethodImpl<M, R(I::*)(Args...) const> : MethodImpl<M, R, const I, Args...> {
        template <typename Subclass>
        using Overridden = R(Subclass::*)(Args...) const;
    };

public:
    template <auto prototype>
    struct Method : MethodImpl<Method<prototype>, decltype(prototype)> {};

    template <typename Method, typename... Args>
    auto call(Args &&... args) const
    {
        return Method::invoke(static_cast<const Interface &>(*this), std::forward<Args>(args)...);
    }

    template <typename Method, typename... Args>
    auto call(Args &&... args)
    {
        return Method::invoke(static_cast<Interface &>(*this), std::forward<Args>(args)...);
    }

    void destroy(); // quasi-virtual pure destructor
    using Destroy = Method<&QQuasiVirtualInterface::destroy>;

    struct Deleter
    {
        void operator () (QQuasiVirtualInterface* self) const { self->template call<Destroy>(); }
    };

protected:
    using base_interface = QQuasiVirtualInterface<Interface>;
    using CallFN = void (*)(size_t index, base_interface &intf, void *ret, void *args);
    void initCallFN(CallFN func) { m_callFN = func; }

    QQuasiVirtualInterface() = default;
    ~QQuasiVirtualInterface() = default;

private:
    Q_DISABLE_COPY_MOVE(QQuasiVirtualInterface)
    CallFN m_callFN = nullptr;
};

template <typename Subclass, typename Interface>
class QQuasiVirtualSubclass : public Interface
{
private:
    template <typename C = Subclass> using Methods = typename C::template MethodTemplates<C>;

    template <size_t OverriddenIndex>
    static constexpr size_t interfaceMethodIndex() {
        return std::tuple_element_t<OverriddenIndex, Methods<>>::index();
    }

    template <size_t... Is>
    static void callImpl(size_t index, Subclass &subclass, void *ret, void *args, std::index_sequence<Is...>)
    {
        constexpr auto methodIndexMask = []() {
            std::array<bool, sizeof...(Is)> result = {};
            (static_cast<void>(std::get<interfaceMethodIndex<Is>()>(result) = true), ...);
            return result;
        }();
        static_assert((methodIndexMask[Is] && ...),
                      "Mapping between base and overridden methods is not unique");

        auto doInvoke = [&](auto idxConstant) {
            std::tuple_element_t<idxConstant.value, Methods<>>::doInvoke(subclass, ret, args);
        };
        QtPrivate::applyIndexSwitch(index, doInvoke,
                                    std::index_sequence<interfaceMethodIndex<Is>()...>{});
    }

    static void callImpl(size_t index, typename Interface::base_interface &intf, void *ret, void *args)
    {
        constexpr auto seq = std::make_index_sequence<std::tuple_size_v<Methods<>>>();
        callImpl(index, static_cast<Subclass&>(intf), ret, args, seq);
    }

    template <typename BaseMethod>
    using OverridenSignature = typename BaseMethod::template Overridden<Subclass>;

protected:
    template <typename... Args>
    QQuasiVirtualSubclass(Args &&... args)
        : Interface(std::forward<Args>(args)...)
    {
        Interface::initCallFN(&QQuasiVirtualSubclass::callImpl);
    }

public:
    template <typename BaseMethod, OverridenSignature<BaseMethod> overridden>
    struct Override : BaseMethod
    {
    private:
        static constexpr void doInvoke(Subclass &subclass, void *ret, void *args)
        {
            using Return = typename BaseMethod::return_type;
            using PackedArgs = typename BaseMethod::call_args;

            Q_ASSERT(args);
            Q_ASSERT(std::is_void_v<Return> == !ret);

            auto invoke = [&subclass](auto &&...params)
            {
                return std::invoke(overridden, &subclass, std::forward<decltype(params)>(params)...);
            };

            if constexpr (std::is_void_v<Return>) {
                std::apply(invoke, std::move(*static_cast<PackedArgs *>(args)));
            } else {
                q20::construct_at(static_cast<Return *>(ret),
                                  std::apply(invoke, std::move(*static_cast<PackedArgs *>(args))));
            }
        }

        friend class QQuasiVirtualSubclass<Subclass, Interface>;
    };
};

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // Q_DOC

#endif // QQUASIVIRTUAL_IMPL_H
