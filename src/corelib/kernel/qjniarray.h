// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJNIARRAY_H
#define QJNIARRAY_H

#include <QtCore/qlist.h>

#if defined(Q_QDOC) || defined(Q_OS_ANDROID)
#include <QtCore/qbytearray.h>
#include <QtCore/qjniobject.h>

#include <iterator>
#include <QtCore/q26numeric.h>
#include <QtCore/q20type_traits.h>
#include <QtCore/q20utility.h>

#if defined(Q_QDOC)
using jsize = qint32;
using jarray = jobject;
#endif

QT_BEGIN_NAMESPACE

template <typename T> class QJniArray;
template <typename T> struct QJniArrayMutableIterator;

// forward declare here so that we don't have to include the private header
namespace QtAndroidPrivate
{
    Q_CORE_EXPORT jclass findClass(const char *className, JNIEnv *env);
}

template <typename T>
struct QJniArrayIterator
{
private:
    using VT = std::remove_const_t<T>;
    friend class QJniArray<VT>;
    friend struct QJniArrayMutableIterator<VT>;

    // Since QJniArray doesn't hold values, we need a wrapper to be able to hand
    // out a pointer to a value.
    struct QJniArrayValueRef {
        T ref;
        const T *operator->() const { return &ref; }
    };

public:
    QJniArrayIterator() = default;
    constexpr QJniArrayIterator(const QJniArrayMutableIterator<VT> &other) noexcept
        : m_index(other.m_index), m_array(other.m_array)
    {}
    constexpr QJniArrayIterator(QJniArrayMutableIterator<VT> &&other) noexcept
        : m_index(std::exchange(other.m_index, -1)), m_array(std::exchange(other.m_array, nullptr))
    {}

    constexpr QJniArrayIterator(const QJniArrayIterator &other) noexcept = default;
    constexpr QJniArrayIterator(QJniArrayIterator &&other) noexcept = default;
    constexpr QJniArrayIterator &operator=(const QJniArrayIterator &other) noexcept = default;
    constexpr QJniArrayIterator &operator=(QJniArrayIterator &&other) noexcept = default;

    using difference_type = jsize;
    using value_type = T;
    using pointer = QJniArrayValueRef;
    using reference = T; // difference to container requirements
    using const_reference = reference;
    using iterator_category = std::random_access_iterator_tag;

    const_reference operator*() const
    {
        return m_array->at(m_index);
    }

    QJniArrayValueRef operator->() const
    {
        return {m_array->at(m_index)};
    }

    const_reference operator[](difference_type n) const
    {
        return m_array->at(m_index + n);
    }

    friend QJniArrayIterator &operator++(QJniArrayIterator &that) noexcept
    {
        ++that.m_index;
        return that;
    }
    friend QJniArrayIterator operator++(QJniArrayIterator &that, int) noexcept
    {
        auto copy = that;
        ++that;
        return copy;
    }
    friend QJniArrayIterator operator+(const QJniArrayIterator &that, difference_type n) noexcept
    {
        return {that.m_index + n, that.m_array};
    }
    friend QJniArrayIterator operator+(difference_type n, const QJniArrayIterator &that) noexcept
    {
        return that + n;
    }
    friend QJniArrayIterator &operator+=(QJniArrayIterator &that, difference_type n) noexcept
    {
        that.m_index += n;
        return that;
    }
    friend QJniArrayIterator &operator--(QJniArrayIterator &that) noexcept
    {
        --that.m_index;
        return that;
    }
    friend QJniArrayIterator operator--(QJniArrayIterator &that, int) noexcept
    {
        auto copy = that;
        --that;
        return copy;
    }
    friend QJniArrayIterator operator-(const QJniArrayIterator &that, difference_type n) noexcept
    {
        return {that.m_index - n, that.m_array};
    }
    friend QJniArrayIterator operator-(difference_type n, const QJniArrayIterator &that) noexcept
    {
        return {n - that.m_index, that.m_array};
    }
    friend QJniArrayIterator &operator-=(QJniArrayIterator &that, difference_type n) noexcept
    {
        that.m_index -= n;
        return that;
    }
    friend difference_type operator-(const QJniArrayIterator &lhs, const QJniArrayIterator &rhs)
    {
        Q_ASSERT(lhs.m_array == rhs.m_array);
        return lhs.m_index - rhs.m_index;
    }
    void swap(QJniArrayIterator &other) noexcept
    {
        std::swap(m_index, other.m_index);
        qt_ptr_swap(m_array, other.m_array);
    }

private:
    friend constexpr bool comparesEqual(const QJniArrayIterator &lhs,
                                        const QJniArrayIterator &rhs) noexcept
    {
        Q_ASSERT(lhs.m_array == rhs.m_array);
        return lhs.m_index == rhs.m_index;
    }
    friend constexpr Qt::strong_ordering compareThreeWay(const QJniArrayIterator &lhs,
                                                         const QJniArrayIterator &rhs) noexcept
    {
        Q_ASSERT(lhs.m_array == rhs.m_array);
        return Qt::compareThreeWay(lhs.m_index, rhs.m_index);
    }
    Q_DECLARE_STRONGLY_ORDERED(QJniArrayIterator)

    qsizetype m_index = 0;
    const QJniArray<VT> *m_array = nullptr;

    QJniArrayIterator(qsizetype index, const QJniArray<VT> *array)
        : m_index(index), m_array(array)
    {}
};

template <typename T> // need to specialize traits for it, so can't be nested
struct QJniArrayMutableValueRef;

template <typename T>
struct QJniArrayMutableIterator
{
private:
    friend struct QJniArrayIterator<const T>;
    friend struct QJniArrayMutableValueRef<T>;

public:
    constexpr QJniArrayMutableIterator() noexcept = default;
    constexpr QJniArrayMutableIterator(const QJniArrayIterator<const T> &other) noexcept
        : m_index(other.m_index), m_array(other.m_array)
    {}
    constexpr QJniArrayMutableIterator(QJniArrayIterator<const T> &&other) noexcept
        : m_index(std::exchange(other.m_index, -1)), m_array(std::exchange(other.m_array, nullptr))
    {}

    constexpr QJniArrayMutableIterator(const QJniArrayMutableIterator &other) noexcept = default;
    constexpr QJniArrayMutableIterator(QJniArrayMutableIterator &&other) noexcept = default;
    constexpr QJniArrayMutableIterator &operator=(const QJniArrayMutableIterator &other) noexcept = default;
    constexpr QJniArrayMutableIterator &operator=(QJniArrayMutableIterator &&other) noexcept = default;

    using difference_type = jsize;
    using value_type = T;
    using pointer = QJniArrayMutableValueRef<T>;
    using reference = QJniArrayMutableValueRef<T>; // difference to container requirements
    using const_reference = T;
    using iterator_category = std::random_access_iterator_tag;

    const_reference operator*() const
    {
        return m_array->at(m_index);
    }

    reference operator*()
    {
        return {m_array->at(m_index), *this};
    }

    const pointer operator->() const
    {
        return {m_array->at(m_index)};
    }

    pointer operator->()
    {
        return {m_array->at(m_index), *this};
    }

    const_reference operator[](difference_type n) const
    {
        return m_array->at(m_index + n);
    }
    reference operator[](difference_type n)
    {
        return {m_array->at(m_index + n), *this};
    }

    friend QJniArrayMutableIterator &operator++(QJniArrayMutableIterator &that) noexcept
    {
        ++that.m_index;
        return that;
    }
    friend QJniArrayMutableIterator operator++(QJniArrayMutableIterator &that, int) noexcept
    {
        auto copy = that;
        ++that;
        return copy;
    }
    friend QJniArrayMutableIterator operator+(const QJniArrayMutableIterator &that, difference_type n) noexcept
    {
        return {that.m_index + n, that.m_array};
    }
    friend QJniArrayMutableIterator operator+(difference_type n, const QJniArrayMutableIterator &that) noexcept
    {
        return that + n;
    }
    friend QJniArrayMutableIterator &operator+=(QJniArrayMutableIterator &that, difference_type n) noexcept
    {
        that.m_index += n;
        return that;
    }
    friend QJniArrayMutableIterator &operator--(QJniArrayMutableIterator &that) noexcept
    {
        --that.m_index;
        return that;
    }
    friend QJniArrayMutableIterator operator--(QJniArrayMutableIterator &that, int) noexcept
    {
        auto copy = that;
        --that;
        return copy;
    }
    friend QJniArrayMutableIterator operator-(const QJniArrayMutableIterator &that, difference_type n) noexcept
    {
        return {that.m_index - n, that.m_array};
    }
    friend QJniArrayMutableIterator operator-(difference_type n, const QJniArrayMutableIterator &that) noexcept
    {
        return {n - that.m_index, that.m_array};
    }
    friend QJniArrayMutableIterator &operator-=(QJniArrayMutableIterator &that, difference_type n) noexcept
    {
        that.m_index -= n;
        return that;
    }
    friend difference_type operator-(const QJniArrayMutableIterator &lhs,
                                     const QJniArrayMutableIterator &rhs)
    {
        Q_ASSERT(lhs.m_array == rhs.m_array);
        return lhs.m_index - rhs.m_index;
    }
    void swap(QJniArrayMutableIterator &other) noexcept
    {
        std::swap(m_index, other.m_index);
        qt_ptr_swap(m_array, other.m_array);
    }

private:
    friend constexpr bool comparesEqual(const QJniArrayMutableIterator &lhs,
                                        const QJniArrayMutableIterator &rhs)
    {
        Q_ASSERT(lhs.m_array == rhs.m_array);
        return lhs.m_index == rhs.m_index;
    }
    friend constexpr bool comparesEqual(const QJniArrayMutableIterator &lhs,
                                        const QJniArrayIterator<const T> &rhs)
    {
        Q_ASSERT(lhs.m_array == rhs.m_array);
        return lhs.m_index == rhs.m_index;
    }
    friend constexpr Qt::strong_ordering compareThreeWay(const QJniArrayMutableIterator &lhs,
                                                         const QJniArrayMutableIterator &rhs)
    {
        Q_ASSERT(lhs.m_array == rhs.m_array);
        return Qt::compareThreeWay(lhs.m_index, rhs.m_index);
    }
    friend constexpr Qt::strong_ordering compareThreeWay(const QJniArrayMutableIterator &lhs,
                                                         const QJniArrayIterator<const T> &rhs)
    {
        Q_ASSERT(lhs.m_array == rhs.m_array);
        return Qt::compareThreeWay(lhs.m_index, rhs.m_index);
    }
    Q_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT(QJniArrayMutableIterator)
    Q_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT(QJniArrayMutableIterator, QJniArrayIterator<const T>)

    using VT = std::remove_const_t<T>;
    friend class QJniArray<VT>;

    qsizetype m_index = 0;
    QJniArray<VT> *m_array = nullptr;

    QJniArrayMutableIterator(qsizetype index, QJniArray<VT> *array)
        : m_index(index), m_array(array)
    {}
};

template <typename T> // need to specialize traits for it, so can't be nested
struct QJniArrayMutableValueRef {
    T value;
    QJniArrayMutableIterator<T> back = {-1, nullptr};

    operator T() const { return value; }
    const T &operator*() const { return value; }
    T &operator*() { return value; }

    const T *operator->() const { return &value; }
    T *operator->() = delete; // no write-back, so delete explicitly

    QJniArrayMutableValueRef &operator=(const QJniArrayMutableValueRef &other)
    {
        return *this = *other;
    }
    QJniArrayMutableValueRef &operator=(QJniArrayMutableValueRef &&other)
    {
        return *this = std::move(*other);
    }

    QJniArrayMutableValueRef &operator=(const T &v)
    {
        Q_ASSERT(back.m_array);
        value = v;
        back.m_array->setValue(back.m_index, value);
        return *this;
    }
    QJniArrayMutableValueRef &operator=(T &&v)
    {
        Q_ASSERT(back.m_array);
        value = std::move(v);
        back.m_array->setValue(back.m_index, value);
        return *this;
    }
};

class QJniArrayBase
{
    // for SFINAE'ing out the fromContainer named constructor
    template <typename C, typename = void> struct IsSequentialContainerHelper : std::false_type
    {
        static constexpr bool isForwardIterable = false;
    };
    template <typename C>
    struct IsSequentialContainerHelper<C, std::void_t<typename std::iterator_traits<typename C::const_iterator>::iterator_category,
                                                      typename C::value_type,
                                                      decltype(std::size(std::declval<C>()))
                                                     >
                                      > : std::true_type
    {
        static constexpr bool isForwardIterable = std::is_convertible_v<
            typename std::iterator_traits<typename C::const_iterator>::iterator_category,
            std::forward_iterator_tag
        >;
    };
    template <>
    struct IsSequentialContainerHelper<QByteArray, void>
    {
        static constexpr bool isForwardIterable = true;
    };

    template <typename C, typename = void> struct IsContiguousContainerHelper : std::false_type {};
    template <typename C>
    struct IsContiguousContainerHelper<C, std::void_t<decltype(std::data(std::declval<C>())),
                                                      decltype(std::size(std::declval<C>())),
                                                      typename C::value_type
                                                     >
                                      > : std::true_type {};

    template <typename C, typename = void> struct HasEmplaceBackTest : std::false_type {};
    template <typename C> struct HasEmplaceBackTest<C,
        std::void_t<decltype(std::declval<C>().emplace_back(std::declval<typename C::value_type>()))>
                                                        > : std::true_type
    {};


protected:
    // these are used in QJniArray
    template <typename C, typename = void>
    struct ElementTypeHelper
    {
        static constexpr bool isObject = false;
        static constexpr bool isPrimitive = false;
    };
    template <typename C>
    struct ElementTypeHelper<C, std::void_t<typename C::value_type>>
    {
        using E = typename C::value_type;
        static constexpr bool isObject = QtJniTypes::isObjectType<E>();
        static constexpr bool isPrimitive = QtJniTypes::isPrimitiveType<E>();
    };

    template <typename CRef, typename C = q20::remove_cvref_t<CRef>>
    static constexpr bool isContiguousContainer = IsContiguousContainerHelper<C>::value;

    template <typename From, typename To>
    using if_convertible = std::enable_if_t<QtPrivate::AreArgumentsConvertibleWithoutNarrowingBase<From, To>::value, bool>;
    template <typename From, typename To>
    using unless_convertible = std::enable_if_t<!QtPrivate::AreArgumentsConvertibleWithoutNarrowingBase<From, To>::value, bool>;

    // helpers for toContainer
    template <typename E> struct ToContainerHelper { using type = QList<E>; };
    template <> struct ToContainerHelper<jstring> { using type = QStringList; };
    template <> struct ToContainerHelper<jbyte> { using type = QByteArray; };
    template <> struct ToContainerHelper<char> { using type = QByteArray; };

    template <typename E>
    using ToContainerType = typename ToContainerHelper<E>::type;

    template <typename E, typename CRef, typename C = q20::remove_cvref_t<CRef>>
    static constexpr bool isCompatibleTargetContainer =
        (QtPrivate::AreArgumentsConvertibleWithoutNarrowingBase<E, typename C::value_type>::value
         || QtPrivate::AreArgumentsConvertibleWithoutNarrowingBase<typename ToContainerType<E>::value_type,
                                                                   typename C::value_type>::value
         || (std::is_base_of_v<QtJniTypes::JObjectBase, E> && std::is_same_v<typename C::value_type, QString>))
        && (qxp::is_detected_v<HasEmplaceBackTest, C>
            || (isContiguousContainer<C> && ElementTypeHelper<C>::isPrimitive));

public:
    using size_type = jsize;
    using difference_type = size_type;

    operator QJniObject() const { return m_object; }

    template <typename T = jobject>
    T object() const { return m_object.object<T>(); }
    bool isValid() const { return m_object.isValid(); }
    bool isEmpty() const { return size() == 0; }

    size_type size() const
    {
        if (jarray array = m_object.object<jarray>())
            return jniEnv()->GetArrayLength(array);
        return 0;
    }

    // We can create an array from any forward-iterable container, and optimize
    // for contiguous containers holding primitive elements. QJniArray is a
    // forward-iterable container, so explicitly remove that from the overload
    // set so that the copy constructors get used instead.
    // Used also in the deduction guide, so must be public
    template <typename C>
    using IsSequentialOrContiguous = std::bool_constant<
                                        IsSequentialContainerHelper<C>::isForwardIterable
                                || (isContiguousContainer<C> && ElementTypeHelper<C>::isPrimitive)
                                >;
    template <typename CRef, typename C = q20::remove_cvref_t<CRef>>
    static constexpr bool isCompatibleSourceContainer = std::conjunction_v<
        std::negation<std::is_same<QString, C>>,
        IsSequentialOrContiguous<C>,
        std::negation<std::is_base_of<QJniArrayBase, C>>
    >;

    template <typename C>
    using if_compatible_source_container = std::enable_if_t<isCompatibleSourceContainer<C>, bool>;
    template <typename T, typename C>
    using if_compatible_target_container = std::enable_if_t<isCompatibleTargetContainer<T, C>, bool>;

    template <typename Container, if_compatible_source_container<Container> = true>
    static auto fromContainer(Container &&container)
    {
        verifySize(std::size(container));
        using ElementType = typename std::remove_reference_t<Container>::value_type;
        if constexpr (std::is_base_of_v<std::remove_pointer_t<jobject>,
                                        std::remove_pointer_t<ElementType>>) {
            return makeObjectArray(std::forward<Container>(container));
        } else if constexpr (std::disjunction_v<std::is_same<ElementType, QJniObject>,
                                                std::is_same<ElementType, QString>,
                                                std::is_base_of<QtJniTypes::JObjectBase, ElementType>
                             >) {
            return QJniArray<ElementType>(makeObjectArray(std::forward<Container>(container)).arrayObject());
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jfloat>) {
            return makeArray<jfloat>(std::forward<Container>(container), &JNIEnv::NewFloatArray,
                                                             &JNIEnv::SetFloatArrayRegion);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jdouble>) {
            return makeArray<jdouble>(std::forward<Container>(container), &JNIEnv::NewDoubleArray,
                                                              &JNIEnv::SetDoubleArrayRegion);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jboolean>) {
            return makeArray<jboolean>(std::forward<Container>(container), &JNIEnv::NewBooleanArray,
                                                               &JNIEnv::SetBooleanArrayRegion);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jbyte>
                             || std::is_same_v<ElementType, char>) {
            return makeArray<jbyte>(std::forward<Container>(container), &JNIEnv::NewByteArray,
                                                            &JNIEnv::SetByteArrayRegion);
        } else if constexpr (std::disjunction_v<std::is_same<ElementType, jchar>,
                                                std::is_same<ElementType, QChar>>) {
            return makeArray<jchar>(std::forward<Container>(container), &JNIEnv::NewCharArray,
                                                            &JNIEnv::SetCharArrayRegion);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jshort>) {
            return makeArray<jshort>(std::forward<Container>(container), &JNIEnv::NewShortArray,
                                                             &JNIEnv::SetShortArrayRegion);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jint>) {
            return makeArray<jint>(std::forward<Container>(container), &JNIEnv::NewIntArray,
                                                           &JNIEnv::SetIntArrayRegion);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jlong>) {
            return makeArray<jlong>(std::forward<Container>(container), &JNIEnv::NewLongArray,
                                                            &JNIEnv::SetLongArrayRegion);
        } else {
            static_assert(QtPrivate::type_dependent_false<ElementType>(),
                            "Don't know how to make QJniArray for this element type");
        }
    }

protected:
    QJniArrayBase() = default;
    ~QJniArrayBase() = default;

    explicit QJniArrayBase(const QJniArrayBase &other) = default;
    explicit QJniArrayBase(QJniArrayBase &&other) noexcept = default;
    QJniArrayBase &operator=(const QJniArrayBase &other) = default;
    QJniArrayBase &operator=(QJniArrayBase &&other) noexcept = default;

    explicit QJniArrayBase(jarray array)
        : m_object(static_cast<jobject>(array))
    {
    }
    explicit QJniArrayBase(const QJniObject &object)
        : m_object(object)
    {}
    explicit QJniArrayBase(QJniObject &&object) noexcept
        : m_object(std::move(object))
    {}
    QJniArrayBase &operator=(const QJniObject &object)
    {
        m_object = object;
        return *this;
    }
    QJniArrayBase &operator=(QJniObject &&object) noexcept
    {
        m_object = std::move(object);
        return *this;
    }

    JNIEnv *jniEnv() const noexcept { return QJniEnvironment::getJniEnv(); }

    template <typename ElementType, typename List, typename NewFn, typename SetFn>
    static auto makeArray(List &&list, NewFn &&newArray, SetFn &&setRegion);
    template <typename List>
    static auto makeObjectArray(List &&list);
    template <typename ElementType>
    static auto makeEmptyArray(size_type size)
    {
        auto env = QJniEnvironment();
        if constexpr (std::disjunction_v<std::is_base_of<std::remove_pointer_t<jobject>,
                                                         std::remove_pointer_t<ElementType>>,
                                        std::is_same<ElementType, QJniObject>,
                                        std::is_same<ElementType, QString>,
                                        std::is_base_of<QtJniTypes::JObjectBase, ElementType>
                             >) {
            using ResultType = decltype(QtJniTypes::Traits<ElementType>::convertToJni(nullptr, {}));
            const auto className = QtJniTypes::Traits<ResultType>::className();
            jclass elementClass = env.findClass(className);
            if (!elementClass) {
                env.checkAndClearExceptions();
                return jobjectArray(nullptr);
            }
            return env->NewObjectArray(size, elementClass, nullptr);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jfloat>) {
            return env->NewFloatArray(size);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jdouble>) {
            return env->NewDoubleArray(size);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jboolean>) {
            return env->NewBooleanArray(size);
        } else if constexpr (std::disjunction_v<std::is_same<ElementType, jbyte>,
                                                std::is_same<ElementType, char>>) {
            return env->NewByteArray(size);
        } else if constexpr (std::disjunction_v<std::is_same<ElementType, jchar>,
                                                std::is_same<ElementType, QChar>>) {
            return env->NewCharArray(size);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jshort>) {
            return env->NewShortArray(size);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jint>) {
            return env->NewIntArray(size);
        } else if constexpr (QtJniTypes::sameTypeForJni<ElementType, jlong>) {
            return env->NewLongArray(size);
        } else {
            static_assert(QtPrivate::type_dependent_false<ElementType>(),
                          "Don't know how to make QJniArray for this element type");
        }
    }

    void swap(QJniArrayBase &other) noexcept { m_object.swap(other.m_object); }

private:
    template <typename container_size_type>
    static void verifySize(container_size_type size)
    {
        if (!q20::in_range<size_type>(size))
            qWarning("QJniArray::fromContainer: Container is too large for Java and will be truncated!");
    }

    QJniObject m_object;
};

template <typename T>
class QJniArray : public QJniArrayBase
{
    friend struct QJniArrayIterator<T>;

    template <typename C>
    using CanReserveTest = decltype(std::declval<C>().reserve(0));
    template <typename C>
    static constexpr bool canReserve = qxp::is_detected_v<CanReserveTest, C>;

public:
    using Type = T;

    using value_type = T;
    using iterator = QJniArrayMutableIterator<T>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_iterator = QJniArrayIterator<const T>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using reference = typename iterator::reference;
    using const_reference = typename const_iterator::const_reference;

    QJniArray() = default;
    explicit QJniArray(jarray array) : QJniArrayBase(array) {}
    explicit QJniArray(const QJniObject &object) : QJniArrayBase(object) {}
    explicit QJniArray(QJniObject &&object) noexcept : QJniArrayBase(std::move(object)) {}

    template <typename Other, if_convertible<Other, T> = true>
    QJniArray(const QJniArray<Other> &other)
        : QJniArrayBase(other)
    {
    }
    template <typename Other, if_convertible<Other, T> = true>
    QJniArray(QJniArray<Other> &&other) noexcept
        : QJniArrayBase(std::move(other))
    {
    }
    template <typename Other, if_convertible<Other, T> = true>
    QJniArray &operator=(const QJniArray<Other> &other)
    {
        QJniArrayBase::operator=(QJniObject(other));
        return *this;
    }
    template <typename Other, if_convertible<Other, T> = true>
    QJniArray &operator=(QJniArray<Other> &&other) noexcept
    {
        QJniArray moved(std::move(other));
        swap(moved);
        return *this;
    }
    // explicitly delete to disable detour via operator QJniObject()
    template <typename Other, unless_convertible<Other, T> = true>
    QJniArray(const QJniArray<Other> &other) = delete;
    template <typename Other, unless_convertible<Other, T> = true>
    QJniArray(QJniArray<Other> &&other) noexcept = delete;

    template <typename Container, if_compatible_source_container<Container> = true>
    explicit QJniArray(Container &&container)
        : QJniArrayBase(QJniArrayBase::fromContainer(std::forward<Container>(container)))
    {
    }

    Q_IMPLICIT inline QJniArray(std::initializer_list<T> list)
        : QJniArrayBase(QJniArrayBase::fromContainer(list))
    {
    }

    explicit QJniArray(size_type size)
        : QJniArrayBase(makeEmptyArray<T>(size))
    {}

    ~QJniArray() = default;

    auto arrayObject() const
    {
        if constexpr (QtJniTypes::isObjectType<T>())
            return object<jobjectArray>();
        else if constexpr (QtJniTypes::sameTypeForJni<T, jbyte>)
            return object<jbyteArray>();
        else if constexpr (QtJniTypes::sameTypeForJni<T, jchar>)
            return object<jcharArray>();
        else if constexpr (QtJniTypes::sameTypeForJni<T, jboolean>)
            return object<jbooleanArray>();
        else if constexpr (QtJniTypes::sameTypeForJni<T, jshort>)
            return object<jshortArray>();
        else if constexpr (QtJniTypes::sameTypeForJni<T, jint>)
            return object<jintArray>();
        else if constexpr (QtJniTypes::sameTypeForJni<T, jlong>)
            return object<jlongArray>();
        else if constexpr (QtJniTypes::sameTypeForJni<T, jfloat>)
            return object<jfloatArray>();
        else if constexpr (QtJniTypes::sameTypeForJni<T, jdouble>)
            return object<jdoubleArray>();
        else
            return object<jarray>();
    }

    const_iterator begin() const noexcept { return {0, this}; }
    const_iterator constBegin() const noexcept { return begin(); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator end() const noexcept { return {size(), this}; }
    const_iterator constEnd() const noexcept { return {end()}; }
    const_iterator cend() const noexcept { return {end()}; }

    iterator begin() noexcept { return {0, this}; }
    iterator end() noexcept { return {size(), this}; }

    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end());  }
    reverse_iterator rend() noexcept { return reverse_iterator(begin());  }

    const_reference operator[](size_type i) const { return at(i); }
    reference operator[](size_type i) { return reference{at(i), iterator{i, this}}; }
    const_reference at(size_type i) const
    {
        JNIEnv *env = jniEnv();
        if constexpr (QtJniTypes::isObjectType<T>()) {
            jobject element = env->GetObjectArrayElement(object<jobjectArray>(), i);
            if constexpr (std::is_base_of_v<std::remove_pointer_t<jobject>, std::remove_pointer_t<T>>)
                return static_cast<T>(element);
            else
                return QtJniTypes::Traits<T>::convertFromJni(QJniObject::fromLocalRef(element));
        } else {
            T res = {};
            if constexpr (QtJniTypes::sameTypeForJni<T, jbyte>)
                env->GetByteArrayRegion(object<jbyteArray>(), i, 1, &res);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jchar>)
                env->GetCharArrayRegion(object<jcharArray>(), i, 1, &res);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jboolean>)
                env->GetBooleanArrayRegion(object<jbooleanArray>(), i, 1, &res);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jshort>)
                env->GetShortArrayRegion(object<jshortArray>(), i, 1, &res);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jint>)
                env->GetIntArrayRegion(object<jintArray>(), i, 1, &res);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jlong>)
                env->GetLongArrayRegion(object<jlongArray>(), i, 1, &res);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jfloat>)
                env->GetFloatArrayRegion(object<jfloatArray>(), i, 1, &res);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jdouble>)
                env->GetDoubleArrayRegion(object<jdoubleArray>(), i, 1, &res);
            return res;
        }
    }

    void setValue(size_type i, const_reference &val)
    {
        JNIEnv *env = jniEnv();

        if constexpr (QtJniTypes::isObjectType<T>()) {
            QtJniTypes::Detail::LocalFrame<T> frame(env);
            jobject element = frame.convertToJni(val);
            env->SetObjectArrayElement(object<jobjectArray>(), i, element);
        } else { // primitive types
            if constexpr (QtJniTypes::sameTypeForJni<T, jbyte>)
                env->SetByteArrayRegion(object<jbyteArray>(), i, 1, &val);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jchar>)
                env->SetCharArrayRegion(object<jcharArray>(), i, 1, &val);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jboolean>)
                env->SetBooleanArrayRegion(object<jbooleanArray>(), i, 1, &val);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jshort>)
                env->SetShortArrayRegion(object<jshortArray>(), i, 1, &val);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jint>)
                env->SetIntArrayRegion(object<jintArray>(), i, 1, &val);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jlong>)
                env->SetLongArrayRegion(object<jlongArray>(), i, 1, &val);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jfloat>)
                env->SetFloatArrayRegion(object<jfloatArray>(), i, 1, &val);
            else if constexpr (QtJniTypes::sameTypeForJni<T, jdouble>)
                env->SetDoubleArrayRegion(object<jdoubleArray>(), i, 1, &val);
        }
    }

    template <typename Container = ToContainerType<T>, if_compatible_target_container<T, Container> = true>
    Container toContainer(Container &&container = {}) const
    {
        const qsizetype sz = size();
        if (!sz)
            return std::forward<Container>(container);
        JNIEnv *env = jniEnv();

        using ContainerType = q20::remove_cvref_t<Container>;

        if constexpr (canReserve<ContainerType>)
            container.reserve(sz);
        if constexpr (std::is_same_v<typename ContainerType::value_type, QString>) {
            for (auto element : *this) {
                if constexpr (std::is_same_v<decltype(element), QString>) {
                    container.emplace_back(element);
                } else if constexpr (std::is_same_v<decltype(element), jstring>) {
                    container.emplace_back(element ? QtJniTypes::Detail::toQString(element, env)
                                                   : QString{});
                } else {
                    container.emplace_back(QJniObject(element).toString());
                }
            }
        } else if constexpr (std::is_base_of_v<std::remove_pointer_t<jobject>, std::remove_pointer_t<T>>) {
            for (auto element : *this)
                container.emplace_back(element);
        } else if constexpr (isContiguousContainer<ContainerType>) {
            container.resize(sz);
            if constexpr (QtJniTypes::sameTypeForJni<T, jbyte>) {
                env->GetByteArrayRegion(object<jbyteArray>(),
                                        0, sz,
                                        reinterpret_cast<jbyte *>(container.data()));
            } else if constexpr (QtJniTypes::sameTypeForJni<T, jchar>) {
                env->GetCharArrayRegion(object<jcharArray>(),
                                        0, sz, container.data());
            } else if constexpr (QtJniTypes::sameTypeForJni<T, jboolean>) {
                env->GetBooleanArrayRegion(object<jbooleanArray>(),
                                        0, sz, container.data());
            } else if constexpr (QtJniTypes::sameTypeForJni<T, jshort>) {
                env->GetShortArrayRegion(object<jshortArray>(),
                                        0, sz, container.data());
            } else if constexpr (QtJniTypes::sameTypeForJni<T, jint>) {
                env->GetIntArrayRegion(object<jintArray>(),
                                        0, sz, container.data());
            } else if constexpr (QtJniTypes::sameTypeForJni<T, jlong>) {
                env->GetLongArrayRegion(object<jlongArray>(),
                                        0, sz, container.data());
            } else if constexpr (QtJniTypes::sameTypeForJni<T, jfloat>) {
                env->GetFloatArrayRegion(object<jfloatArray>(),
                                        0, sz, container.data());
            } else if constexpr (QtJniTypes::sameTypeForJni<T, jdouble>) {
                env->GetDoubleArrayRegion(object<jdoubleArray>(),
                                        0, sz, container.data());
            } else {
                static_assert(QtPrivate::type_dependent_false<T>(),
                              "Don't know how to copy data from a QJniArray of this type");
            }
        } else {
            for (auto e : *this)
                container.emplace_back(e);
        }
        return std::forward<Container>(container);
    }
};

// Deduction guide so that we can construct as 'QJniArray list(Container<T>)'. Since
// fromContainer() maps several C++ types to the same JNI type (e.g. both jboolean and
// bool become QJniArray<jboolean>), we have to deduce to what fromContainer() would
// give us.
template <typename Container, QJniArrayBase::if_compatible_source_container<Container> = true>
QJniArray(Container) -> QJniArray<typename decltype(QJniArrayBase::fromContainer(std::declval<Container>()))::value_type>;

template <typename ElementType, typename List, typename NewFn, typename SetFn>
auto QJniArrayBase::makeArray(List &&list, NewFn &&newArray, SetFn &&setRegion)
{
    const size_type length = size_type(std::size(list));
    JNIEnv *env = QJniEnvironment::getJniEnv();
    auto localArray = (env->*newArray)(length);
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        if (localArray)
            env->DeleteLocalRef(localArray);
        return QJniArray<ElementType>();
    }

    if (length) {
        // can't use static_cast here because we have signed/unsigned mismatches
        if constexpr (isContiguousContainer<List>) {
            (env->*setRegion)(localArray, 0, length,
                            reinterpret_cast<const ElementType *>(std::data(std::as_const(list))));
        } else {
            size_type i = 0;
            for (const auto &e : std::as_const(list))
                (env->*setRegion)(localArray, i++, 1, reinterpret_cast<const ElementType *>(&e));
        }
    }
    return QJniArray<ElementType>(QJniObject::fromLocalRef(localArray));
};

template <typename List>
auto QJniArrayBase::makeObjectArray(List &&list)
{
    using ElementType = typename q20::remove_cvref_t<List>::value_type;
    using ResultType = QJniArray<decltype(QtJniTypes::Traits<ElementType>::convertToJni(nullptr,
                                                                                        {}))>;

    if (std::size(list) == 0)
        return ResultType();

    JNIEnv *env = QJniEnvironment::getJniEnv();
    const size_type length = q26::saturate_cast<size_type>(std::size(list));

    // this assumes that all objects in the list have the same class
    jclass elementClass = nullptr;
    if constexpr (std::disjunction_v<std::is_same<ElementType, QJniObject>,
                                     std::is_base_of<QtJniTypes::JObjectBase, ElementType>>) {
        elementClass = std::begin(list)->objectClass();
    } else if constexpr (std::is_same_v<ElementType, QString>) {
        elementClass = QtAndroidPrivate::findClass("java/lang/String", env);
    } else {
        elementClass = env->GetObjectClass(*std::begin(list));
    }
    auto localArray = env->NewObjectArray(length, elementClass, nullptr);
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        if (localArray)
            env->DeleteLocalRef(localArray);
        return ResultType();
    }

    // explicitly manage the frame for local references in chunks of 100
    constexpr jint frameCapacity = 100;
    qsizetype i = 0;
    for (const auto &element : std::as_const(list)) {
        if (i % frameCapacity == 0) {
            if (i)
                env->PopLocalFrame(nullptr);
            if (env->PushLocalFrame(frameCapacity) != 0)
                return ResultType{};
        }
        jobject object = QtJniTypes::Traits<ElementType>::convertToJni(env, element);
        env->SetObjectArrayElement(localArray, i, object);
        ++i;
    }
    if (i)
        env->PopLocalFrame(nullptr);
    return ResultType(QJniObject::fromLocalRef(localArray));
}

namespace QtJniTypes
{
template <typename T> struct Traits<QJniArray<T>>
{
    template <IfValidFieldType<T> = true>
    static constexpr auto signature()
    {
        return CTString("[") + Traits<T>::signature();
    }
    static auto convertToJni(JNIEnv *, const QJniArray<T> &value)
    {
        return value.arrayObject();
    }
    static auto convertFromJni(QJniObject &&object)
    {
        return QJniArray<T>(std::move(object));
    }
};

template <typename T> struct Traits<QJniArrayMutableValueRef<T>> : public Traits<T> {};

template<typename T> struct Traits<T, std::enable_if_t<QJniArrayBase::isCompatibleSourceContainer<T>>>
{
    // QByteArray::value_type is char, which maps to 'C'; we need 'B', i.e. jbyte
    using ElementType = std::conditional_t<std::is_same_v<T, QByteArray>,
                                           jbyte, typename T::value_type>;

    template <typename U = ElementType, IfValidFieldType<U> = true>
    static constexpr auto signature()
    {
        return CTString("[") + Traits<ElementType>::signature();
    }

    static auto convertToJni(JNIEnv *env, const T &value)
    {
        using QJniArrayType = decltype(QJniArrayBase::fromContainer(value));
        using ArrayType = decltype(std::declval<QJniArrayType>().arrayObject());
        return static_cast<ArrayType>(env->NewLocalRef(QJniArray(value).arrayObject()));
    }

    static auto convertFromJni(QJniObject &&object)
    {
        // if we were to create a QJniArray from Type...
        using QJniArrayType = decltype(QJniArrayBase::fromContainer(std::declval<T>()));
        // then that QJniArray would have elements of type
        using ArrayType = typename QJniArrayType::Type;
        // construct a QJniArray from a jobject pointer of that type
        return QJniArray<ArrayType>(object.template object<jarray>()).toContainer();
    }
};

template<typename T> struct Traits<T, std::enable_if_t<std::is_array_v<T>>>
{
    using ElementType = std::remove_extent_t<T>;

    template <typename U = ElementType, IfValidFieldType<U> = true>
    static constexpr auto signature()
    {
        static_assert(!std::is_array_v<ElementType>,
                    "Traits::signature() does not handle multi-dimensional arrays");
        return CTString("[") + Traits<U>::signature();
    }

    static constexpr auto convertFromJni(QJniObject &&object)
    {
        return QJniArray<ElementType>(std::move(object));
    }
};
}

QT_END_NAMESPACE

#endif

#endif // QJNIARRAY_H
