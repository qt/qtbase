// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCborMap>
#include <QCborValue>

#include <QTest>

template <typename Char>
struct SampleStrings
{
    static constexpr char key[] = "hello";
};

template <>
struct SampleStrings<char16_t>
{
    static constexpr char16_t key[] = u"hello";
};

template <>
struct SampleStrings<QChar>
{
    static const QChar *const key;
};
const QChar *const SampleStrings<QChar>::key =
        reinterpret_cast<const QChar *>(SampleStrings<char16_t>::key);

template <typename T, typename = void>
constexpr bool hasValueType = false;

template <typename T>
constexpr bool hasValueType<T, std::void_t<typename T::value_type>> = true;

class tst_QCborValue : public QObject
{
    Q_OBJECT
private:
    template <typename Type>
    void doKeyLookup();

    template <typename Type>
    void doConstruct();

private slots:
    void keyLookupLatin1() { doKeyLookup<QLatin1StringView>(); }
    void keyLookupString() { doKeyLookup<QString>(); }
    void keyLookupConstCharPtr() { doKeyLookup<char>(); };

    void constructLatin1() { doConstruct<QLatin1StringView>(); }
    void constructString() { doConstruct<QString>(); }
    void constructStringView() { doConstruct<QStringView>(); }
    void constructConstCharPtr() { doConstruct<char>(); }
};

template <typename Type>
void tst_QCborValue::doKeyLookup()
{
    const QCborMap m{{"hello", "world"}, {1, 2}};
    const QCborValue v = m;

    if constexpr (hasValueType<Type>) {
        using Char = std::remove_cv_t<typename Type::value_type>;
        using Strings = SampleStrings<Char>;
        const Type s(Strings::key);
        QBENCHMARK {
            [[maybe_unused]] const QCborValue r = v[s];
        }
    } else {
        QBENCHMARK {
            [[maybe_unused]] const QCborValue r = v[SampleStrings<Type>::key];
        }
    }
}

template<typename Type>
void tst_QCborValue::doConstruct()
{
    if constexpr (hasValueType<Type>) {
        using Char = std::remove_cv_t<typename Type::value_type>;
        using Strings = SampleStrings<Char>;
        const Type s(Strings::key);
        QBENCHMARK {
            [[maybe_unused]] const auto v = QCborValue{s};
        }
    } else {
        QBENCHMARK {
            [[maybe_unused]] const auto v = QCborValue{SampleStrings<Type>::key};
        }
    }
}

QTEST_MAIN(tst_QCborValue)

#include "tst_bench_qcborvalue.moc"
