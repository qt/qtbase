// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJNIARRAY_H
#define QJNIARRAY_H

#include <QtCore/qlist.h>

#if defined(Q_QDOC) || defined(Q_OS_ANDROID)
#include <QtCore/qbytearray.h>
#include <QtCore/qjniobject.h>

#include <utility>
#include <type_traits>

QT_BEGIN_NAMESPACE

template <typename T> class QJniArray;
template <typename T>
struct QJniArrayIterator
{
    constexpr QJniArrayIterator(const QJniArrayIterator &other) noexcept = default;
    constexpr QJniArrayIterator(QJniArrayIterator &&other) noexcept = default;
    constexpr QJniArrayIterator &operator=(const QJniArrayIterator &other) noexcept = default;
    constexpr QJniArrayIterator &operator=(QJniArrayIterator &&other) noexcept = default;

    friend bool operator==(const QJniArrayIterator &lhs, const QJniArrayIterator &rhs) noexcept
    {
        return lhs.m_array == rhs.m_array && lhs.m_index == rhs.m_index;
    }
    friend bool operator!=(const QJniArrayIterator &lhs, const QJniArrayIterator &rhs) noexcept
    {
        return !(lhs == rhs);
    }
    T operator*() const
    {
        return m_array->at(m_index);
    }
    friend QJniArrayIterator &operator++(QJniArrayIterator &that) noexcept
    {
        ++that.m_index;
        return that;
    }

    void swap(QJniArrayIterator &other) noexcept
    {
        std::swap(m_index, other.m_index);
        qt_ptr_swap(m_array, other.m_array);
    }

private:
    friend class QJniArray<T>;

    qsizetype m_index = 0;
    const QJniArray<T> *m_array = nullptr;

    QJniArrayIterator() = delete;
    QJniArrayIterator(qsizetype index, const QJniArray<T> *array)
        : m_index(index), m_array(array)
    {}
};

class QJniArrayBase : public QJniObject
{
    // for SFINAE'ing out the fromContainer named constructor
    template <typename Container, typename = void> struct CanConvertHelper : std::false_type {};
    template <typename Container>
    struct CanConvertHelper<Container, std::void_t<decltype(std::declval<Container>().data()),
                                                   decltype(std::declval<Container>().size())
                                              >
                           > : std::true_type {};

public:

    qsizetype size() const
    {
        if (jarray array = object<jarray>())
            return jniEnv()->GetArrayLength(array);
        return 0;
    }

    template <typename Container>
    static constexpr bool canConvert = CanConvertHelper<Container>::value;
    template <typename Container>
    using IfCanConvert = std::enable_if_t<canConvert<Container>, bool>;
    template <typename Container
        , IfCanConvert<Container> = true
    >
    static auto fromContainer(Container &&container)
    {
        using ElementType = typename std::remove_reference_t<Container>::value_type;
        if constexpr (std::disjunction_v<std::is_same<ElementType, jobject>,
                                         std::is_same<ElementType, QJniObject>>) {
            return makeObjectArray(std::forward<Container>(container));
        } else if constexpr (std::is_same_v<ElementType, jfloat>) {
            return makeArray<jfloat>(std::forward<Container>(container), &JNIEnv::NewFloatArray,
                                                             &JNIEnv::SetFloatArrayRegion);
        } else if constexpr (std::is_same_v<ElementType, jdouble>) {
            return makeArray<jdouble>(std::forward<Container>(container), &JNIEnv::NewDoubleArray,
                                                              &JNIEnv::SetDoubleArrayRegion);
        } else if constexpr (std::disjunction_v<std::is_same<ElementType, jboolean>,
                                                std::is_same<ElementType, bool>>) {
            return makeArray<jboolean>(std::forward<Container>(container), &JNIEnv::NewBooleanArray,
                                                               &JNIEnv::SetBooleanArrayRegion);
        } else if constexpr (std::disjunction_v<std::is_same<ElementType, jbyte>,
                                                std::is_same<ElementType, char>>) {
            return makeArray<jbyte>(std::forward<Container>(container), &JNIEnv::NewByteArray,
                                                            &JNIEnv::SetByteArrayRegion);
        } else if constexpr (std::disjunction_v<std::is_same<ElementType, jchar>,
                                                std::is_same<ElementType, QChar>>) {
            return makeArray<jchar>(std::forward<Container>(container), &JNIEnv::NewCharArray,
                                                            &JNIEnv::SetCharArrayRegion);
        } else if constexpr (std::is_same_v<ElementType, jshort>
                          || sizeof(ElementType) == sizeof(jshort)) {
            return makeArray<jshort>(std::forward<Container>(container), &JNIEnv::NewShortArray,
                                                             &JNIEnv::SetShortArrayRegion);
        } else if constexpr (std::is_same_v<ElementType, jint>
                          || sizeof(ElementType) == sizeof(jint)) {
            return makeArray<jint>(std::forward<Container>(container), &JNIEnv::NewIntArray,
                                                           &JNIEnv::SetIntArrayRegion);
        } else if constexpr (std::is_same_v<ElementType, jlong>
                          || sizeof(ElementType) == sizeof(jlong)) {
            return makeArray<jlong>(std::forward<Container>(container), &JNIEnv::NewLongArray,
                                                            &JNIEnv::SetLongArrayRegion);
        }
    }

protected:
    QJniArrayBase() = default;
    ~QJniArrayBase() = default;

    explicit QJniArrayBase(jarray array)
        : QJniObject(static_cast<jobject>(array))
    {
        static_assert(sizeof(QJniArrayBase) == sizeof(QJniObject),
                      "QJniArrayBase must have the same size as QJniObject!");
    }
    explicit QJniArrayBase(const QJniObject &object)
        : QJniObject(object)
    {}
    explicit QJniArrayBase(QJniObject &&object) noexcept
        : QJniObject(std::move(object))
    {}

    template <typename ElementType, typename List, typename NewFn, typename SetFn>
    static auto makeArray(List &&list, NewFn &&newArray, SetFn &&setRegion);
    template <typename List>
    static auto makeObjectArray(List &&list);
};

template <typename T>
class QJniArray : public QJniArrayBase
{
    friend struct QJniArrayIterator<T>;
public:
    using Type = T;
    using const_iterator = const QJniArrayIterator<T>;

    QJniArray() = default;
    explicit QJniArray(jarray array) : QJniArrayBase(array) {}
    explicit QJniArray(const QJniObject &object) : QJniArrayBase(object) {}
    explicit QJniArray(QJniObject &&object) noexcept : QJniArrayBase(std::move(object)) {}

    // base class destructor is protected, so need to provide all SMFs
    QJniArray(const QJniArray &other) = default;
    QJniArray(QJniArray &&other) noexcept = default;
    QJniArray &operator=(const QJniArray &other) = default;
    QJniArray &operator=(QJniArray &&other) noexcept = default;

    template <typename Container
        , IfCanConvert<Container> = true
    >
    explicit QJniArray(Container &&container);

    template <typename Other, std::enable_if_t<std::is_convertible_v<Other, Type>, bool> = true>
    QJniArray(QJniArray<Other> &&other)
        : QJniArrayBase(std::forward<QJniArray<Other>>(other))
    {
    }
    ~QJniArray() = default;

    auto arrayObject() const
    {
        if constexpr (std::is_convertible_v<jobject, T>)
            return object<jobjectArray>();
        else if constexpr (std::is_same_v<T, jbyte>)
            return object<jbyteArray>();
        else if constexpr (std::is_same_v<T, jchar>)
            return object<jcharArray>();
        else if constexpr (std::is_same_v<T, jboolean>)
            return object<jbooleanArray>();
        else if constexpr (std::is_same_v<T, jshort>)
            return object<jshortArray>();
        else if constexpr (std::is_same_v<T, jint>)
            return object<jintArray>();
        else if constexpr (std::is_same_v<T, jlong>)
            return object<jlongArray>();
        else if constexpr (std::is_same_v<T, jfloat>)
            return object<jfloatArray>();
        else if constexpr (std::is_same_v<T, jdouble>)
            return object<jdoubleArray>();
        else
            return object<jarray>();
    }

    const_iterator begin() const { return {0, this}; }
    const_iterator constBegin() const { return begin(); }
    const_iterator cbegin() const { return begin(); }
    const_iterator end() const { return {size(), this}; }
    const_iterator constEnd() const { return {end()}; }
    const_iterator cend() const { return {end()}; }

    T operator[](qsizetype i) const { return at(i); }
    T at(qsizetype i) const
    {
        JNIEnv *env = jniEnv();
        if constexpr (std::is_convertible_v<jobject, T>) {
            jobject element = env->GetObjectArrayElement(object<jobjectArray>(), i);
            if constexpr (std::is_base_of_v<QJniObject, T>)
                return QJniObject::fromLocalRef(element);
            else if constexpr (std::is_base_of_v<QtJniTypes::JObjectBase, T>)
                return T::fromLocalRef(element);
            else
                return T{element};
        } else {
            T res = {};
            if constexpr (std::is_same_v<T, jbyte>)
                env->GetByteArrayRegion(object<jbyteArray>(), i, 1, &res);
            else if constexpr (std::is_same_v<T, jchar>)
                env->GetCharArrayRegion(object<jcharArray>(), i, 1, &res);
            else if constexpr (std::is_same_v<T, jboolean>)
                env->GetBooleanArrayRegion(object<jbooleanArray>(), i, 1, &res);
            else if constexpr (std::is_same_v<T, jshort>)
                env->GetShortArrayRegion(object<jshortArray>(), i, 1, &res);
            else if constexpr (std::is_same_v<T, jint>)
                env->GetIntArrayRegion(object<jbyteArray>(), i, 1, &res);
            else if constexpr (std::is_same_v<T, jlong>)
                env->GetLongArrayRegion(object<jlongArray>(), i, 1, &res);
            else if constexpr (std::is_same_v<T, jfloat>)
                env->GetFloatArrayRegion(object<jfloatArray>(), i, 1, &res);
            else if constexpr (std::is_same_v<T, jdouble>)
                env->GetDoubleArrayRegion(object<jdoubleArray>(), i, 1, &res);
            return res;
        }
    }
    auto toContainer() const
    {
        JNIEnv *env = jniEnv();
        if constexpr (std::is_same_v<T, jobject>) {
            QList<jobject> res;
            res.reserve(size());
            for (auto &&element : *this)
                res.append(element);
            return res;
        } else if constexpr (std::is_same_v<T, jbyte>) {
            const qsizetype bytecount = size();
            QByteArray res(bytecount, Qt::Initialization::Uninitialized);
            env->GetByteArrayRegion(object<jbyteArray>(),
                                    0, bytecount, reinterpret_cast<jbyte *>(res.data()));
            return res;
        } else {
            QList<T> res;
            res.resize(size());
            if constexpr (std::is_same_v<T, jchar>) {
                env->GetCharArrayRegion(object<jcharArray>(),
                                        0, res.size(), res.data());
            } else if constexpr (std::is_same_v<T, jboolean>) {
                env->GetBooleanArrayRegion(object<jbooleanArray>(),
                                           0, res.size(), res.data());
            } else if constexpr (std::is_same_v<T, jshort>) {
                env->GetShortArrayRegion(object<jshortArray>(),
                                         0, res.size(), res.data());
            } else if constexpr (std::is_same_v<T, jint>) {
                env->GetIntArrayRegion(object<jintArray>(),
                                       0, res.size(), res.data());
            } else if constexpr (std::is_same_v<T, jlong>) {
                env->GetLongArrayRegion(object<jlongArray>(),
                                        0, res.size(), res.data());
            } else if constexpr (std::is_same_v<T, jfloat>) {
                env->GetFloatArrayRegion(object<jfloatArray>(),
                                         0, res.size(), res.data());
            } else if constexpr (std::is_same_v<T, jdouble>) {
                env->GetDoubleArrayRegion(object<jdoubleArray>(),
                                          0, res.size(), res.data());
            } else {
                res.clear();
            }
            return res;
        }
    }
};

template <typename ElementType, typename List, typename NewFn, typename SetFn>
auto QJniArrayBase::makeArray(List &&list, NewFn &&newArray, SetFn &&setRegion)
{
    const int length = int(list.size());
    JNIEnv *env = QJniEnvironment::getJniEnv();
    auto localArray = (env->*newArray)(length);
    if (QJniEnvironment::checkAndClearExceptions(env))
        return QJniArray<ElementType>();

    // can't use static_cast here because we have signed/unsigned mismatches
    if (length) {
        (env->*setRegion)(localArray, 0, length,
                          reinterpret_cast<const ElementType *>(std::as_const(list).data()));
    }
    return QJniArray<ElementType>(localArray);
};

template <typename List>
auto QJniArrayBase::makeObjectArray(List &&list)
{
    using ElementType = typename List::value_type;
    if (list.isEmpty())
        return QJniArray<jobject>();

    JNIEnv *env = QJniEnvironment::getJniEnv();
    const int length = int(list.size());

    // this assumes that all objects in the list have the same class
    jclass elementClass = nullptr;
    if constexpr (std::is_same_v<ElementType, QJniObject>)
        elementClass = list.first().objectClass();
    else
        elementClass = env->GetObjectClass(list.first());
    auto localArray = env->NewObjectArray(length, elementClass, nullptr);
    if (QJniEnvironment::checkAndClearExceptions(env))
        return QJniArray<jobject>();
    for (int i = 0; i < length; ++i) {
        jobject object;
        if constexpr (std::is_same_v<ElementType, QJniObject>)
            object = list.at(i).object();
        else
            object = list.at(i);
        env->SetObjectArrayElement(localArray, i, object);
    }
    return QJniArray<jobject>(localArray);
}


template <typename T>
template <typename Container
    , QJniArrayBase::IfCanConvert<Container>
>
QJniArray<T>::QJniArray(Container &&container)
    : QJniArrayBase(QJniArrayBase::fromContainer(std::forward<Container>(container)))
{
}

namespace QtJniTypes
{
template <typename T> struct IsJniArray: std::false_type {};
template <typename T> struct IsJniArray<QJniArray<T>> : std::true_type {};
template <typename T> struct Traits<QJniArray<T>> {
    template <IfValidFieldType<T> = true>
    static constexpr auto signature()
    {
        return CTString("[") + Traits<T>::signature();
    }
};
template <typename T> struct Traits<QList<T>> {
    template <IfValidFieldType<T> = true>
    static constexpr auto signature()
    {
        return CTString("[") + Traits<T>::signature();
    }
};
template <> struct Traits<QByteArray> {
    static constexpr auto signature()
    {
        return CTString("[B");
    }
};
}

QT_END_NAMESPACE

#endif

#endif // QJNIARRAY_H
