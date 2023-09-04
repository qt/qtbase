// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

private slots:
    void keyLookupLatin1() { doKeyLookup<QLatin1StringView>(); }
    void keyLookupString() { doKeyLookup<QString>(); }
    void keyLookupConstCharPtr() { doKeyLookup<char>(); };
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
            const QCborValue r = v[s];
            Q_UNUSED(r);
        }
    } else {
        QBENCHMARK {
            const QCborValue r = v[SampleStrings<Type>::key];
            Q_UNUSED(r);
        }
    }
}

QTEST_MAIN(tst_QCborValue)

#include "tst_bench_qcborvalue.moc"
