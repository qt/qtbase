// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qcollator.h>
#include <QtCore/qlocale.h>
#include <private/qglobal_p.h> // For feature test

class tst_QCollator : public QObject
{
    Q_OBJECT

#if QT_CONFIG(icu) \
    || defined(Q_OS_WIN) || defined(Q_OS_MACOS) || defined(Q_OS_ANDROID) || defined(Q_OS_WASM)
    const QLocale m_locale = QLocale(QLocale::English, QLocale::UnitedStates);
#else
    // The POSIX backend only supports one locale:
    const QLocale m_locale = QLocale(QLocale::C);
    // (The macOS backend *doesn't* support the C locale, so we can't use it for
    // all. In any case, it's not a good choice as its sorting is too simple.)
#endif

private Q_SLOTS:
    // Same types as tst_QCollator::sorting() under tests/auto/.
    void sortStringList();
    void sortViewList();
    void sortKeyStringList();
    void sortKeyViewList();
};

static const char16_t *const s_elements[] = {
    u"Hydrogen", u"Helium",
    u"Lithium", u"Beryllium", u"Boron", u"Carbon",
    u"Nitrogen", u"Oxygen", u"Flourine", u"Neon",
    u"Sodium", u"Magnesium", u"Aluminium", u"Silicon",
    u"Phosphorous", u"Sulphur", u"Chlorine", u"Argon",
    u"Potassium", u"Calcium", u"Scandium", u"Titanium",
    u"Vanadium", u"Chromium", u"Manganese", u"Iron" };

static const char16_t *const s_sorted[] = {
    u"Aluminium", u"Argon", u"Beryllium", u"Boron", u"Calcium", u"Carbon",
    u"Chlorine", u"Chromium", u"Flourine", u"Helium", u"Hydrogen", u"Iron",
    u"Lithium", u"Magnesium", u"Manganese", u"Neon", u"Nitrogen", u"Oxygen",
    u"Phosphorous", u"Potassium", u"Scandium", u"Silicon", u"Sodium", u"Sulphur",
    u"Titanium", u"Vanadium" };

void tst_QCollator::sortStringList()
{
    QStringList test;
    for (const char16_t *el : s_elements)
        test << QStringView(el).toString();
    const QStringList elements = test;
    const QCollator collator(m_locale);
    QBENCHMARK {
        test = elements;
        std::sort(test.begin(), test.end(), collator);
    }
}

void tst_QCollator::sortViewList()
{
    const QList<QStringView> elements(std::begin(s_elements), std::end(s_elements));
    const QCollator collator(m_locale);
    QList<QStringView> test;
    QBENCHMARK {
        test = elements;
        std::sort(test.begin(), test.end(), collator);
    }
}

void tst_QCollator::sortKeyStringList()
{
    using Pair = QPair<QCollatorSortKey, QString>;
    const QCollator collator(m_locale);
    QList<Pair> test;
    for (const char16_t *el : s_elements) {
        const QString str = QStringView(el).toString();
        test << Pair(collator.sortKey(str), str);
    }
    const auto elements = test;
    QBENCHMARK {
        test = elements;
        std::sort(test.begin(), test.end(),
                  [](const Pair &lhs, const Pair &rhs) {
                      return lhs.first < rhs.first;
                  });
    }
}

void tst_QCollator::sortKeyViewList()
{
    using Pair = QPair<QCollatorSortKey, QStringView>;
    const QCollator collator(m_locale);
    QList<Pair> test;
    for (const char16_t *el : s_elements) {
        const QStringView str(el);
        test << Pair(collator.sortKey(str.toString()), str);
    }
    const auto elements = test;
    QBENCHMARK {
        test = elements;
        std::sort(test.begin(), test.end(),
                  [](const Pair &lhs, const Pair &rhs) {
                      return lhs.first < rhs.first;
                  });
    }
}

QTEST_MAIN(tst_QCollator)

#include "tst_bench_qcollator.moc"
