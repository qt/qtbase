// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTMOCHELPERS_H
#define QTMOCHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists to be used by the code that
// moc generates. This file will not change quickly, but it over the long term,
// it will likely change or even be removed.
//
// We mean it.
//

#include <QtCore/qmetatype.h>
#include <QtCore/qtmocconstants.h>

#include <QtCore/q20algorithm.h>    // std::min, std::copy_n
#include <QtCore/q23type_traits.h>  // std::is_scoped_enum
#include <limits>

#if 0
#pragma qt_no_master_include
#endif

QT_BEGIN_NAMESPACE
namespace QtMocHelpers {
// The maximum Size of a string literal is 2 GB on 32-bit and 4 GB on 64-bit
// (but the compiler is likely to give up before you get anywhere near that much)
static constexpr size_t MaxStringSize =
        (std::min)(size_t((std::numeric_limits<uint>::max)()),
                   size_t((std::numeric_limits<qsizetype>::max)()));

template <uint... Nx> constexpr size_t stringDataSizeHelper(std::integer_sequence<uint, Nx...>)
{
    // same as:
    //   return (0 + ... + Nx);
    // but not using the fold expression to avoid exceeding compiler limits
    size_t total = 0;
    uint sizes[] = { Nx... };
    for (uint n : sizes)
        total += n;
    return total;
}

template <int Count, size_t StringSize> struct StringData
{
    static_assert(StringSize <= MaxStringSize, "Meta Object data is too big");
    uint offsetsAndSizes[Count] = {};
    char stringdata0[StringSize] = {};
    constexpr StringData() = default;
};

template <uint... Nx> constexpr auto stringData(const char (&...strings)[Nx])
{
    constexpr size_t StringSize = stringDataSizeHelper<Nx...>({});
    constexpr size_t Count = 2 * sizeof...(Nx);

    StringData<Count, StringSize> result;
    const char *inputs[] = { strings... };
    uint sizes[] = { Nx... };

    uint offset = 0;
    char *output = result.stringdata0;
    for (size_t i = 0; i < sizeof...(Nx); ++i) {
        // copy the input string, including the terminating null
        uint len = sizes[i];
        for (uint j = 0; j < len; ++j)
            output[offset + j] = inputs[i][j];
        result.offsetsAndSizes[2 * i] = offset + sizeof(result.offsetsAndSizes);
        result.offsetsAndSizes[2 * i + 1] = len - 1;
        offset += len;
    }

    return result;
}

#  define QT_MOC_HAS_STRINGDATA       1

struct NoType {};

namespace detail {
template<typename Enum> constexpr int payloadSizeForEnum()
{
    // How many uint blocks do we need to store the values of this enum and the
    // string indices for the enumeration labels? We only support 8- 16-, 32-
    // and 64-bit enums at the time of this writing, so this code is extra
    // pedantic allowing for 48-, 96-, 128-bit, etc.
    int n = int(sizeof(Enum) + sizeof(uint)) - 1;
    return 1 + n / sizeof(uint);
}

template <uint H, uint P> struct UintDataBlock
{
    static constexpr uint headerSize() { return H; }
    static constexpr uint payloadSize() { return P; }
    uint header[H ? H : 1] = {};
    uint payload[P ? P : 1] = {};
};

template <int Idx, typename T> struct UintDataEntry
{
    T entry;
    constexpr UintDataEntry(T &&entry_) : entry(std::move(entry_)) {}
};

// This storage type is designed similar to libc++'s std::tuple, in that it
// derives from a type unique to each of the types in the template parameter
// pack (even if they are the same type). That way, we can refer to each of
// entries uniquely by just casting *this to that unique type.
//
// Testing reveals this to compile MUCH faster than recursive approaches and
// avoids compiler constexpr-time limits.
template <typename Idx, typename... T> struct UintDataStorage;
template <int... Idx, typename... T> struct UintDataStorage<std::integer_sequence<int, Idx...>, T...>
        : UintDataEntry<Idx, T>...
{
    constexpr UintDataStorage(T &&... data)
        : UintDataEntry<Idx, T>(std::move(data))...
    {}

    template <typename F> constexpr void forEach(F &&f) const
    {
        [[maybe_unused]] auto invoke = [&f](const auto &entry) { f(entry.entry); return 0; };
        int dummy[] = {
            0,
            invoke(static_cast<const UintDataEntry<Idx, T> &>(*this))...
        };
        (void) dummy;
    }
};
} // namespace detail

template <typename... Block> struct UintData
{
    constexpr UintData(Block &&... data_)
        : data(std::move(data_)...)
    {}

    static constexpr uint count() { return sizeof...(Block); }
    static constexpr uint headerSize()
    {
        // same as:
        //   return (0 + ... + Block::headerSize());
        // but not using the fold expression to avoid exceeding compiler limits
        // (calculation done using int to get compile-time overflow checking)
        int total = 0;
        int sizes[] = { 0, Block::headerSize()... };
        for (int n : sizes)
            total += n;
        return total;
    }
    static constexpr uint payloadSize()
    {
        // ditto
        int total = 0;
        int sizes[] = { 0, Block::payloadSize()... };
        for (int n : sizes)
            total += n;
        return total;
    }
    static constexpr uint dataSize() { return headerSize() + payloadSize(); }

    template <typename Result>
    constexpr void copyTo(Result &result, size_t dataoffset) const
    {
        uint *ptr = result.data.data();
        size_t payloadoffset = dataoffset + headerSize();
        data.forEach([&](const auto &input) {
            // copy the uint data
            q20::copy_n(input.header, input.headerSize(), ptr + dataoffset);
            q20::copy_n(input.payload, input.payloadSize(), ptr + payloadoffset);
            input.adjustOffset(ptr, uint(dataoffset), uint(payloadoffset));

            dataoffset += input.headerSize();
            payloadoffset += input.payloadSize();
        });
    }

    template <typename F> constexpr void forEach(F &&f) const
    {
        data.forEach(std::forward<F>(f));
    }

private:
    detail::UintDataStorage<std::make_integer_sequence<int, count()>, Block...> data;
};

template <int N> struct ClassInfos : detail::UintDataBlock<2 * N, 0>
{
    constexpr ClassInfos() = default;
    constexpr ClassInfos(const std::array<uint, 2> (&infos)[N])
    {
        uint *out = this->header;
        for (int i = 0; i < N; ++i) {
            *out++ = infos[i][0];
            *out++ = infos[i][1];
        }
    }
};

struct PropertyData : detail::UintDataBlock<5, 0>
{
    constexpr PropertyData(uint nameIndex, uint typeIndex, uint flags, uint notifyId = uint(-1), uint revision = 0)
    {
        this->header[0] = nameIndex;
        this->header[1] = typeIndex;
        this->header[2] = flags;
        this->header[3] = notifyId;
        this->header[4] = revision;
    }

    static constexpr void adjustOffset(uint *, uint, uint) noexcept {}
};

template <typename Enum, int N = 0>
struct EnumData : detail::UintDataBlock<5, N * detail::payloadSizeForEnum<Enum>()>
{
private:
    static_assert(sizeof(Enum) <= 2 * sizeof(uint), "Cannot store enumeration of this size");
    template <typename T> struct RealEnum { using Type = T; };
    template <typename T> struct RealEnum<QFlags<T>> { using Type = T; };
public:
    struct EnumEntry {
        int nameIndex;
        typename RealEnum<Enum>::Type value;
    };

    constexpr EnumData(uint nameOffset, uint aliasOffset, uint flags)
    {
        this->header[0] = nameOffset;
        this->header[1] = aliasOffset;
        this->header[2] = flags;
        this->header[3] = N;
        this->header[4] = 0;        // will be set in adjustOffsets()

        if (nameOffset != aliasOffset || QtPrivate::IsQFlags<Enum>::value)
            this->header[2] |= QtMocConstants::EnumIsFlag;
        if constexpr (q23::is_scoped_enum_v<Enum>)
            this->header[2] |= QtMocConstants::EnumIsScoped;
    }

    template <int Added> constexpr auto add(const EnumEntry (&entries)[Added]) const
    {
        EnumData<Enum, N + Added> result(this->header[0], this->header[1], this->header[2]);

        q20::copy_n(this->payload, this->payloadSize(), result.payload);
        uint o = this->payloadSize();
        for (auto entry : entries) {
            result.payload[o++] = uint(entry.nameIndex);
            auto value = qToUnderlying(entry.value);
            result.payload[o++] = uint(value);
        }

        if constexpr (sizeof(Enum) > sizeof(uint)) {
            static_assert(N == 0, "Unimplemented: merging with non-empty EnumData");
            result.header[2] |= QtMocConstants::EnumIs64Bit;
            for (auto entry : entries) {
                auto value = qToUnderlying(entry.value);
                result.payload[o++] = uint(value >> 32);
            }
        }
        return result;
    }

    static constexpr void adjustOffset(uint *ptr, uint dataoffset, uint payloadoffset) noexcept
    {
        ptr[dataoffset + 4] += uint(payloadoffset);
    }
};

template <typename F, uint ExtraFlags> struct FunctionData;
template <typename Ret, typename... Args, uint ExtraFlags>
struct FunctionData<Ret (Args...), ExtraFlags>
    : detail::UintDataBlock<6, 2 * sizeof...(Args) + 1 + (ExtraFlags & QtMocConstants::MethodRevisioned ? 1 : 0)>
{
    static constexpr bool IsRevisioned = (ExtraFlags & QtMocConstants::MethodRevisioned) != 0;
    struct FunctionParameter {
        uint typeIdx;   // or static meta type ID
        uint nameIdx;
    };
    using ParametersArray = std::array<FunctionParameter, sizeof...(Args)>;

    static constexpr void adjustOffset(uint *ptr, uint dataoffset, uint payloadoffset) noexcept
    {
        if constexpr (IsRevisioned)
            ++payloadoffset;
        ptr[dataoffset + 2] += uint(payloadoffset);
    }

    constexpr
    FunctionData(uint nameIndex, uint tagIndex, uint metaTypesIndex, uint flags,
                 uint returnType, ParametersArray params = {})
    {
        this->header[0] = nameIndex;
        this->header[1] = sizeof...(Args);
        this->header[2] = 0;        // will be set in adjustOffsets()
        this->header[3] = tagIndex;
        this->header[4] = flags | ExtraFlags;
        this->header[5] = metaTypesIndex;

        uint *p = this->payload;
        if constexpr (ExtraFlags & QtMocConstants::MethodRevisioned)
            ++p;
        *p++ = returnType;
        for (uint i = 0; i < sizeof...(Args); ++i)
            *p++ = params[i].typeIdx;
        for (uint i = 0; i < sizeof...(Args); ++i)
            *p++ = params[i].nameIdx;
    }

    constexpr
    FunctionData(uint nameIndex, uint tagIndex, uint metaTypesIndex, uint flags, uint revision,
                 uint returnType, ParametersArray params = {})
#ifdef __cpp_concepts
            requires(IsRevisioned)
#endif
        : FunctionData(nameIndex, tagIndex, metaTypesIndex, flags, returnType, params)
    {
        // note: we place the revision differently from meta object revision 12
        this->payload[0] = revision;
    }
};
template <typename Ret, typename... Args, uint ExtraFlags>
struct FunctionData<Ret (Args...) const, ExtraFlags>
    : FunctionData<Ret (Args...), ExtraFlags | QtMocConstants::MethodIsConst>
{
    using FunctionData<Ret (Args...), ExtraFlags | QtMocConstants::MethodIsConst>::FunctionData;
};

template <typename F> struct MethodData : FunctionData<F, QtMocConstants::MethodMethod>
{
    using FunctionData<F, QtMocConstants::MethodMethod>::FunctionData;
};

template <typename F> struct SignalData : FunctionData<F, QtMocConstants::MethodSignal>
{
    using FunctionData<F, QtMocConstants::MethodSignal>::FunctionData;
};

template <typename F> struct SlotData : FunctionData<F, QtMocConstants::MethodSlot>
{
    using FunctionData<F, QtMocConstants::MethodSlot>::FunctionData;
};

template <typename F> struct ConstructorData : FunctionData<F, QtMocConstants::MethodConstructor>
{
    using FunctionData<F, QtMocConstants::MethodConstructor>::FunctionData;
};

template <typename F> struct RevisionedMethodData :
        FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodMethod>
{
    using FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodMethod>::FunctionData;
};

template <typename F> struct RevisionedSignalData :
        FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodSignal>
{
    using FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodSignal>::FunctionData;
};

template <typename F> struct RevisionedSlotData :
        FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodSlot>
{
    using FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodSlot>::FunctionData;
};

template <typename F> struct RevisionedConstructorData :
        FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodConstructor>
{
    using FunctionData<F, QtMocConstants::MethodRevisioned | QtMocConstants::MethodConstructor>::FunctionData;
};



template <uint N> struct UintDataResult
{
    std::array<uint, N> data;
};

template <typename Methods, typename Properties, typename Enums,
          typename Constructors = UintData<>, typename ClassInfo = detail::UintDataBlock<0, 0>>
constexpr auto metaObjectData(uint flags, const Methods &methods, const Properties &properties,
                              const Enums &enums, const Constructors &constructors = {},
                              const ClassInfo &classInfo = {})
{
    constexpr uint HeaderSize = 14;
    constexpr uint TotalSize = HeaderSize
            + Properties::dataSize()
            + Enums::dataSize()
            + Methods::dataSize()
            + Constructors::dataSize()
            + ClassInfo::headerSize() // + ClassInfo::payloadSize()
            + 1;    // empty EOD
    UintDataResult<TotalSize> result = {};
    uint dataoffset = HeaderSize;

    result.data[0] = QtMocConstants::OutputRevision;
    result.data[1] = 0;     // class name index (it's always 0)

    result.data[2] = ClassInfo::headerSize() / 2;
    result.data[3] = ClassInfo::headerSize() ? dataoffset : 0;
    q20::copy_n(classInfo.header, classInfo.headerSize(), result.data.data() + dataoffset);
    dataoffset += ClassInfo::headerSize();

    result.data[6] = properties.count();
    result.data[7] = properties.count() ? dataoffset : 0;
    properties.copyTo(result, dataoffset);
    dataoffset += properties.dataSize();

    result.data[8] = enums.count();
    result.data[9] = enums.count() ? dataoffset : 0;
    enums.copyTo(result, dataoffset);
    dataoffset += enums.dataSize();

    result.data[4] = methods.count();
    result.data[5] = methods.count() ? dataoffset : 0;
    methods.copyTo(result, dataoffset);
    dataoffset += methods.dataSize();

    result.data[10] = constructors.count();
    result.data[11] = constructors.count() ? dataoffset : 0;
    constructors.copyTo(result, dataoffset);
    dataoffset += constructors.dataSize();

    result.data[12] = flags;

    // count the number of signals
    if constexpr (Methods::count()) {
        constexpr uint MethodHeaderSize = Methods::headerSize() / Methods::count();
        const uint *ptr = &result.data[result.data[5]];
        const uint *end = &result.data[result.data[5] + MethodHeaderSize * Methods::count()];
        for ( ; ptr < end; ptr += MethodHeaderSize) {
            if ((ptr[4] & QtMocConstants::MethodSignal) == 0)
                break;
            ++result.data[13];
        }
    }

    return result;
}

#define QT_MOC_HAS_UINTDATA    1

template <typename T> inline std::enable_if_t<std::is_enum_v<T>> assignFlags(void *v, T t) noexcept
{
    *static_cast<T *>(v) = t;
}

template <typename T> inline std::enable_if_t<QtPrivate::IsQFlags<T>::value> assignFlags(void *v, T t) noexcept
{
    *static_cast<T *>(v) = t;
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
template <typename T>
Q_DECL_DEPRECATED_X("Returning int/uint from a Q_PROPERTY that is a Q_FLAG is deprecated; "
                    "please update to return the actual property's type")
inline void assignFlagsFromInteger(QFlags<T> &f, int i) noexcept
{
     f = QFlag(i);
}

template <typename T, typename I>
inline std::enable_if_t<QtPrivate::IsQFlags<T>::value && sizeof(T) == sizeof(int) && std::is_integral_v<I>>
assignFlags(void *v, I i) noexcept
{
    assignFlagsFromInteger(*static_cast<T *>(v), i);
}
#endif  // Qt 7

} // namespace QtMocHelpers
QT_END_NAMESPACE

QT_USE_NAMESPACE

#endif // QTMOCHELPERS_H
