// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/private/qcomparisontesthelper_p.h>
#include <QtCore/qcontainerinfo.h>
#include <QtCore/qmetacontainer.h>
#include <QtCore/QMap>
#include <QtCore/QHash>

#include <QtCore/qvector.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>

#include <vector>
#include <set>
#include <forward_list>
#include <unordered_map>

namespace CheckContainerTraits
{
struct NotAContainer {};

static_assert(QContainerInfo::has_size_v<QVector<int>>);
static_assert(QContainerInfo::has_size_v<QSet<int>>);
static_assert(!QContainerInfo::has_size_v<NotAContainer>);
static_assert(QContainerInfo::has_size_v<std::vector<int>>);
static_assert(QContainerInfo::has_size_v<std::set<int>>);
static_assert(!QContainerInfo::has_size_v<std::forward_list<int>>);

static_assert(QContainerInfo::has_clear_v<QVector<int>>);
static_assert(QContainerInfo::has_clear_v<QSet<int>>);
static_assert(!QContainerInfo::has_clear_v<NotAContainer>);
static_assert(QContainerInfo::has_clear_v<std::vector<int>>);
static_assert(QContainerInfo::has_clear_v<std::set<int>>);
static_assert(QContainerInfo::has_clear_v<std::forward_list<int>>);

static_assert(QContainerInfo::has_at_index_v<QVector<int>>);
static_assert(!QContainerInfo::has_at_index_v<QSet<int>>);
static_assert(!QContainerInfo::has_at_index_v<NotAContainer>);
static_assert(QContainerInfo::has_at_index_v<std::vector<int>>);
static_assert(!QContainerInfo::has_at_index_v<std::set<int>>);
static_assert(!QContainerInfo::has_at_index_v<std::forward_list<int>>);

static_assert(QContainerInfo::can_get_at_index_v<QVector<int>>);
static_assert(!QContainerInfo::can_get_at_index_v<QSet<int>>);
static_assert(!QContainerInfo::can_get_at_index_v<NotAContainer>);
static_assert(QContainerInfo::can_get_at_index_v<std::vector<int>>);
static_assert(!QContainerInfo::can_get_at_index_v<std::set<int>>);
static_assert(!QContainerInfo::can_get_at_index_v<std::forward_list<int>>);

static_assert(QContainerInfo::can_set_at_index_v<QVector<int>>);
static_assert(!QContainerInfo::can_set_at_index_v<QSet<int>>);
static_assert(!QContainerInfo::can_set_at_index_v<NotAContainer>);
static_assert(QContainerInfo::can_set_at_index_v<std::vector<int>>);
static_assert(!QContainerInfo::can_set_at_index_v<std::set<int>>);
static_assert(!QContainerInfo::can_set_at_index_v<std::forward_list<int>>);

static_assert(QContainerInfo::has_push_back_v<QVector<int>>);
static_assert(!QContainerInfo::has_push_back_v<QSet<int>>);
static_assert(!QContainerInfo::has_push_back_v<NotAContainer>);
static_assert(QContainerInfo::has_push_back_v<std::vector<int>>);
static_assert(!QContainerInfo::has_push_back_v<std::set<int>>);
static_assert(!QContainerInfo::has_push_back_v<std::forward_list<int>>);

static_assert(QContainerInfo::has_push_front_v<QVector<int>>);
static_assert(!QContainerInfo::has_push_front_v<QSet<int>>);
static_assert(!QContainerInfo::has_push_front_v<NotAContainer>);
static_assert(!QContainerInfo::has_push_front_v<std::vector<int>>);
static_assert(!QContainerInfo::has_push_front_v<std::set<int>>);
static_assert(QContainerInfo::has_push_front_v<std::forward_list<int>>);

static_assert(!QContainerInfo::has_insert_v<QVector<int>>);
static_assert(QContainerInfo::has_insert_v<QSet<int>>);
static_assert(!QContainerInfo::has_insert_v<NotAContainer>);
static_assert(!QContainerInfo::has_insert_v<std::vector<int>>);
static_assert(QContainerInfo::has_insert_v<std::set<int>>);
static_assert(!QContainerInfo::has_insert_v<std::forward_list<int>>);

static_assert(QContainerInfo::has_pop_back_v<QVector<int>>);
static_assert(!QContainerInfo::has_pop_back_v<QSet<int>>);
static_assert(!QContainerInfo::has_pop_back_v<NotAContainer>);
static_assert(QContainerInfo::has_pop_back_v<std::vector<int>>);
static_assert(!QContainerInfo::has_pop_back_v<std::set<int>>);
static_assert(!QContainerInfo::has_pop_back_v<std::forward_list<int>>);

static_assert(QContainerInfo::has_pop_front_v<QVector<int>>);
static_assert(!QContainerInfo::has_pop_front_v<QSet<int>>);
static_assert(!QContainerInfo::has_pop_front_v<NotAContainer>);
static_assert(!QContainerInfo::has_pop_front_v<std::vector<int>>);
static_assert(!QContainerInfo::has_pop_front_v<std::set<int>>);
static_assert(QContainerInfo::has_pop_front_v<std::forward_list<int>>);

static_assert(QContainerInfo::has_iterator_v<QVector<int>>);
static_assert(QContainerInfo::has_iterator_v<QSet<int>>);
static_assert(!QContainerInfo::has_iterator_v<NotAContainer>);
static_assert(QContainerInfo::has_iterator_v<std::vector<int>>);
static_assert(QContainerInfo::has_iterator_v<std::set<int>>);
static_assert(QContainerInfo::has_iterator_v<std::forward_list<int>>);

static_assert(QContainerInfo::has_const_iterator_v<QVector<int>>);
static_assert(QContainerInfo::has_const_iterator_v<QSet<int>>);
static_assert(!QContainerInfo::has_const_iterator_v<NotAContainer>);
static_assert(QContainerInfo::has_const_iterator_v<std::vector<int>>);
static_assert(QContainerInfo::has_const_iterator_v<std::set<int>>);
static_assert(QContainerInfo::has_const_iterator_v<std::forward_list<int>>);

static_assert(QContainerInfo::iterator_dereferences_to_value_v<QVector<int>>);
static_assert(QContainerInfo::iterator_dereferences_to_value_v<QSet<int>>);
static_assert(!QContainerInfo::iterator_dereferences_to_value_v<NotAContainer>);
static_assert(QContainerInfo::iterator_dereferences_to_value_v<std::vector<int>>);
static_assert(QContainerInfo::iterator_dereferences_to_value_v<std::set<int>>);
static_assert(QContainerInfo::iterator_dereferences_to_value_v<std::forward_list<int>>);

static_assert(QContainerInfo::can_set_value_at_iterator_v<QVector<int>>);
static_assert(!QContainerInfo::can_set_value_at_iterator_v<QSet<int>>);
static_assert(!QContainerInfo::can_set_value_at_iterator_v<NotAContainer>);
static_assert(QContainerInfo::can_set_value_at_iterator_v<std::vector<int>>);
static_assert(!QContainerInfo::can_set_value_at_iterator_v<std::set<int>>);
static_assert(QContainerInfo::can_set_value_at_iterator_v<std::forward_list<int>>);

static_assert(QContainerInfo::can_insert_value_at_iterator_v<QVector<int>>);
static_assert(QContainerInfo::can_insert_value_at_iterator_v<QSet<int>>);
static_assert(!QContainerInfo::can_insert_value_at_iterator_v<NotAContainer>);
static_assert(QContainerInfo::can_insert_value_at_iterator_v<std::vector<int>>);
static_assert(!QContainerInfo::can_insert_value_at_iterator_v<std::forward_list<int>>);

// The iterator is only a hint, but syntactically indistinguishable from others.
// It's explicitly there to be signature compatible with std::vector::insert, though.
// Also, inserting into a set is not guaranteed to actually do anything.
static_assert(QContainerInfo::can_insert_value_at_iterator_v<std::set<int>>);

static_assert(QContainerInfo::can_erase_at_iterator_v<QVector<int>>);
static_assert(QContainerInfo::can_erase_at_iterator_v<QSet<int>>);
static_assert(!QContainerInfo::can_erase_at_iterator_v<NotAContainer>);
static_assert(QContainerInfo::can_erase_at_iterator_v<std::vector<int>>);
static_assert(QContainerInfo::can_erase_at_iterator_v<std::set<int>>);
static_assert(!QContainerInfo::can_erase_at_iterator_v<std::forward_list<int>>);

}

class tst_QMetaContainer: public QObject
{
    Q_OBJECT

private:
    QVector<QMetaType> qvector;
    std::vector<QString> stdvector;
    QSet<QByteArray> qset;
    std::set<int> stdset;
    std::forward_list<QMetaSequence> forwardList;

    QHash<int, QMetaType> qhash;
    QMap<QByteArray, bool> qmap;
    std::map<QString, int> stdmap;
    std::unordered_map<int, QMetaAssociation> stdunorderedmap;

private slots:
    void init();
    void compareCompiles();
    void testSequence_data();
    void testSequence();

    void testAssociation_data();
    void testAssociation();

    void cleanup();
};

void tst_QMetaContainer::init()
{
    qvector = { QMetaType(), QMetaType::fromType<QString>(), QMetaType::fromType<int>() };
    stdvector = { QStringLiteral("foo"), QStringLiteral("bar"), QStringLiteral("baz") };
    qset = { "aaa", "bbb", "ccc" };
    stdset = { 1, 2, 3, 42, 45, 11 };
    forwardList = {
        QMetaSequence::fromContainer<QVector<QMetaType>>(),
        QMetaSequence::fromContainer<std::vector<QString>>(),
        QMetaSequence::fromContainer<QSet<QByteArray>>(),
        QMetaSequence::fromContainer<std::set<int>>(),
        QMetaSequence::fromContainer<std::forward_list<QMetaSequence>>()
    };
    qhash = {
        { 233, QMetaType() },
        { 11, QMetaType::fromType<QByteArray>() },
        { 6626, QMetaType::fromType<bool>() }
    };
    qmap = {
        { "eins", true },
        { "zwei", false },
        { "elfundvierzig", true }
    };

    stdmap = {
        { QStringLiteral("dkdkdkd"), 58583 },
        { QStringLiteral("ooo30393"), 12 },
        { QStringLiteral("2dddd30393"), 999999 },
    };
    stdunorderedmap = {
        { 11, QMetaAssociation::fromContainer<QHash<int, QMetaType>>() },
        { 12, QMetaAssociation::fromContainer<QMap<QByteArray, bool>>() },
        { 393, QMetaAssociation::fromContainer<std::map<QString, int>>() },
        { 293, QMetaAssociation::fromContainer<std::unordered_map<int, QMetaAssociation>>() }
    };
}

void tst_QMetaContainer::compareCompiles()
{
    QTestPrivate::testEqualityOperatorsCompile<QMetaSequence>();
    QTestPrivate::testEqualityOperatorsCompile<QMetaAssociation>();
}

void tst_QMetaContainer::cleanup()
{
    qvector.clear();
    stdvector.clear();
    qset.clear();
    stdset.clear();
    forwardList.clear();
    qhash.clear();
    qmap.clear();
    stdmap.clear();
    stdunorderedmap.clear();
}

void tst_QMetaContainer::testSequence_data()
{
    QTest::addColumn<void *>("container");
    QTest::addColumn<QMetaSequence>("metaSequence");
    QTest::addColumn<QMetaType>("metaType");
    QTest::addColumn<bool>("hasSize");
    QTest::addColumn<bool>("isIndexed");
    QTest::addColumn<bool>("canRemove");
    QTest::addColumn<bool>("hasBidirectionalIterator");
    QTest::addColumn<bool>("hasRandomAccessIterator");
    QTest::addColumn<bool>("canInsertAtIterator");
    QTest::addColumn<bool>("canEraseAtIterator");
    QTest::addColumn<bool>("isSortable");

    QTest::addRow("QVector")
            << static_cast<void *>(&qvector)
            << QMetaSequence::fromContainer<QVector<QMetaType>>()
            << QMetaType::fromType<QMetaType>()
            << true << true << true << true << true << true << true << true;
    QTest::addRow("std::vector")
            << static_cast<void *>(&stdvector)
            << QMetaSequence::fromContainer<std::vector<QString>>()
            << QMetaType::fromType<QString>()
            << true << true << true << true << true << true << true << true;
    QTest::addRow("QSet")
            << static_cast<void *>(&qset)
            << QMetaSequence::fromContainer<QSet<QByteArray>>()
            << QMetaType::fromType<QByteArray>()
            << true << false << false << false << false << true << true << false;
    QTest::addRow("std::set")
            << static_cast<void *>(&stdset)
            << QMetaSequence::fromContainer<std::set<int>>()
            << QMetaType::fromType<int>()
            << true << false << false << true << false << true << true << false;
    QTest::addRow("std::forward_list")
            << static_cast<void *>(&forwardList)
            << QMetaSequence::fromContainer<std::forward_list<QMetaSequence>>()
            << QMetaType::fromType<QMetaSequence>()
            << false << false << true << false << false << false << false << true;
}

void tst_QMetaContainer::testSequence()
{
    QFETCH(void *, container);
    QFETCH(QMetaSequence, metaSequence);
    QFETCH(QMetaType, metaType);
    QFETCH(bool, hasSize);
    QFETCH(bool, isIndexed);
    QFETCH(bool, canRemove);
    QFETCH(bool, hasBidirectionalIterator);
    QFETCH(bool, hasRandomAccessIterator);
    QFETCH(bool, canInsertAtIterator);
    QFETCH(bool, canEraseAtIterator);
    QFETCH(bool, isSortable);

    QVERIFY(metaSequence.canAddValue());
    QCOMPARE(metaSequence.hasSize(), hasSize);
    QCOMPARE(metaSequence.canGetValueAtIndex(), isIndexed);
    QCOMPARE(metaSequence.canSetValueAtIndex(), isIndexed);
    QCOMPARE(metaSequence.canRemoveValue(), canRemove);
    QCOMPARE(metaSequence.hasBidirectionalIterator(), hasBidirectionalIterator);
    QCOMPARE(metaSequence.hasRandomAccessIterator(), hasRandomAccessIterator);
    QCOMPARE(metaSequence.canInsertValueAtIterator(), canInsertAtIterator);
    QCOMPARE(metaSequence.canEraseValueAtIterator(), canEraseAtIterator);
    QCOMPARE(metaSequence.isSortable(), isSortable);

    QVariant var1(metaType);
    QVariant var2(metaType);
    QVariant var3(metaType);

    if (hasSize) {
        const qsizetype size = metaSequence.size(container);

        // var1 is invalid, and our sets do not contain an invalid value so far.
        metaSequence.addValue(container, var1.constData());
        QCOMPARE(metaSequence.size(container), size + 1);
        if (canRemove) {
            metaSequence.removeValue(container);
            QCOMPARE(metaSequence.size(container), size);
        }
    } else {
        metaSequence.addValue(container, var1.constData());
        if (canRemove)
            metaSequence.removeValue(container);
    }

    if (isIndexed) {
        QVERIFY(hasSize);
        const qsizetype size = metaSequence.size(container);
        for (qsizetype i = 0; i < size; ++i) {
            metaSequence.valueAtIndex(container, i, var1.data());
            metaSequence.valueAtIndex(container, size - i - 1, var2.data());

            metaSequence.setValueAtIndex(container, i, var2.constData());
            metaSequence.setValueAtIndex(container, size - i - 1, var1.constData());

            metaSequence.valueAtIndex(container, i, var3.data());
            QCOMPARE(var3, var2);

            metaSequence.valueAtIndex(container, size - i - 1, var3.data());
            QCOMPARE(var3, var1);
        }
    }

    QVERIFY(metaSequence.hasIterator());
    QVERIFY(metaSequence.hasConstIterator());
    QVERIFY(metaSequence.canGetValueAtIterator());
    QVERIFY(metaSequence.canGetValueAtConstIterator());

    void *it = metaSequence.begin(container);
    void *end = metaSequence.end(container);
    QVERIFY(it);
    QVERIFY(end);

    void *constIt = metaSequence.constBegin(container);
    void *constEnd = metaSequence.constEnd(container);
    QVERIFY(constIt);
    QVERIFY(constEnd);

    const qsizetype size = metaSequence.diffIterator(end, it);
    QCOMPARE(size, metaSequence.diffConstIterator(constEnd, constIt));
    if (hasSize)
        QCOMPARE(size, metaSequence.size(container));

    qsizetype count = 0;
    for (; !metaSequence.compareIterator(it, end);
         metaSequence.advanceIterator(it, 1), metaSequence.advanceConstIterator(constIt, 1)) {
        metaSequence.valueAtIterator(it, var1.data());
        if (isIndexed) {
            metaSequence.valueAtIndex(container, count, var2.data());
            QCOMPARE(var1, var2);
        }
        metaSequence.valueAtConstIterator(constIt, var3.data());
        QCOMPARE(var3, var1);
        ++count;
    }

    QCOMPARE(count, size);
    QVERIFY(metaSequence.compareConstIterator(constIt, constEnd));

    metaSequence.destroyIterator(it);
    metaSequence.destroyIterator(end);
    metaSequence.destroyConstIterator(constIt);
    metaSequence.destroyConstIterator(constEnd);

    if (metaSequence.canSetValueAtIterator()) {
        void *it = metaSequence.begin(container);
        void *end = metaSequence.end(container);
        QVERIFY(it);
        QVERIFY(end);

        for (; !metaSequence.compareIterator(it, end); metaSequence.advanceIterator(it, 1)) {
            metaSequence.valueAtIterator(it, var1.data());
            metaSequence.setValueAtIterator(it, var2.constData());
            metaSequence.valueAtIterator(it, var3.data());
            QCOMPARE(var2, var3);
            var2 = var1;
        }

        metaSequence.destroyIterator(it);
        metaSequence.destroyIterator(end);
    }

    if (metaSequence.hasBidirectionalIterator()) {
        void *it = metaSequence.end(container);
        void *end = metaSequence.begin(container);
        QVERIFY(it);
        QVERIFY(end);

        void *constIt = metaSequence.constEnd(container);
        void *constEnd = metaSequence.constBegin(container);
        QVERIFY(constIt);
        QVERIFY(constEnd);

        qsizetype size = 0;
        if (metaSequence.hasRandomAccessIterator()) {
            size = metaSequence.diffIterator(end, it);
            QCOMPARE(size, metaSequence.diffConstIterator(constEnd, constIt));
        } else {
            size = -metaSequence.diffIterator(it, end);
        }

        if (hasSize)
            QCOMPARE(size, -metaSequence.size(container));

        qsizetype count = 0;
        do {
            metaSequence.advanceIterator(it, -1);
            metaSequence.advanceConstIterator(constIt, -1);
            --count;

            metaSequence.valueAtIterator(it, var1.data());
            if (isIndexed) {
                metaSequence.valueAtIndex(container, count - size, var2.data());
                QCOMPARE(var1, var2);
            }
            metaSequence.valueAtConstIterator(constIt, var3.data());
            QCOMPARE(var3, var1);
        } while (!metaSequence.compareIterator(it, end));

        QCOMPARE(count, size);
        QVERIFY(metaSequence.compareConstIterator(constIt, constEnd));

        metaSequence.destroyIterator(it);
        metaSequence.destroyIterator(end);
        metaSequence.destroyConstIterator(constIt);
        metaSequence.destroyConstIterator(constEnd);
    }

    if (canInsertAtIterator) {
        void *it = metaSequence.begin(container);
        void *end = metaSequence.end(container);

        const qsizetype size = metaSequence.diffIterator(end, it);
        metaSequence.destroyIterator(end);

        metaSequence.insertValueAtIterator(container, it, var1.constData());
        metaSequence.destroyIterator(it);
        it = metaSequence.begin(container);
        metaSequence.insertValueAtIterator(container, it, var2.constData());
        metaSequence.destroyIterator(it);
        it = metaSequence.begin(container);
        metaSequence.insertValueAtIterator(container, it, var3.constData());

        metaSequence.destroyIterator(it);

        it = metaSequence.begin(container);
        end = metaSequence.end(container);

        const qsizetype newSize = metaSequence.diffIterator(end, it);

        if (metaSequence.isSortable()) {
            QCOMPARE(newSize, size + 3);
            QVariant var4(metaType);
            metaSequence.valueAtIterator(it, var4.data());
            QCOMPARE(var4, var3);
            metaSequence.advanceIterator(it, 1);
            metaSequence.valueAtIterator(it, var4.data());
            QCOMPARE(var4, var2);
            metaSequence.advanceIterator(it, 1);
            metaSequence.valueAtIterator(it, var4.data());
            QCOMPARE(var4, var1);
        } else {
            QVERIFY(newSize >= size);
        }

        if (canEraseAtIterator) {
            for (int i = 0; i < newSize; ++i) {
                metaSequence.destroyIterator(it);
                it = metaSequence.begin(container);
                metaSequence.eraseValueAtIterator(container, it);
            }

            metaSequence.destroyIterator(it);
            it = metaSequence.begin(container);
            metaSequence.destroyIterator(end);
            end = metaSequence.end(container);
            QVERIFY(metaSequence.compareIterator(it, end));

            metaSequence.addValue(container, var1.constData());
            metaSequence.addValue(container, var2.constData());
            metaSequence.addValue(container, var3.constData());
        }

        metaSequence.destroyIterator(end);
        metaSequence.destroyIterator(it);
    }

    QVERIFY(metaSequence.canClear());
    constIt = metaSequence.constBegin(container);
    constEnd = metaSequence.constEnd(container);
    QVERIFY(!metaSequence.compareConstIterator(constIt, constEnd));
    metaSequence.destroyConstIterator(constIt);
    metaSequence.destroyConstIterator(constEnd);

    metaSequence.clear(container);
    constIt = metaSequence.constBegin(container);
    constEnd = metaSequence.constEnd(container);
    QVERIFY(metaSequence.compareConstIterator(constIt, constEnd));
    metaSequence.destroyConstIterator(constIt);
    metaSequence.destroyConstIterator(constEnd);

    QVERIFY(metaSequence.iface() != nullptr);
    QMetaSequence defaultConstructed;
    QVERIFY(defaultConstructed.iface() == nullptr);
    QT_TEST_EQUALITY_OPS(QMetaSequence(), defaultConstructed, true);
    QT_TEST_EQUALITY_OPS(QMetaSequence(), QMetaSequence(), true);
    QT_TEST_EQUALITY_OPS(defaultConstructed, metaSequence, false);
}

void tst_QMetaContainer::testAssociation_data()
{
    QTest::addColumn<void *>("container");
    QTest::addColumn<QMetaAssociation>("metaAssociation");
    QTest::addColumn<QMetaType>("keyType");
    QTest::addColumn<QMetaType>("mappedType");
    QTest::addColumn<bool>("hasSize");
    QTest::addColumn<bool>("canRemove");
    QTest::addColumn<bool>("canSetMapped");
    QTest::addColumn<bool>("hasBidirectionalIterator");
    QTest::addColumn<bool>("hasRandomAccessIterator");

    QTest::addRow("QHash")
            << static_cast<void *>(&qhash)
            << QMetaAssociation::fromContainer<QHash<int, QMetaType>>()
            << QMetaType::fromType<int>()
            << QMetaType::fromType<QMetaType>()
            << true << true << true << false << false;
    QTest::addRow("QMap")
            << static_cast<void *>(&qmap)
            << QMetaAssociation::fromContainer<QMap<QByteArray, bool>>()
            << QMetaType::fromType<QByteArray>()
            << QMetaType::fromType<bool>()
            << true << true << true << true << false;
    QTest::addRow("std::map")
            << static_cast<void *>(&stdmap)
            << QMetaAssociation::fromContainer<std::map<QString, int>>()
            << QMetaType::fromType<QString>()
            << QMetaType::fromType<int>()
            << true << true << true << true << false;
    QTest::addRow("std::unorderedmap")
            << static_cast<void *>(&stdunorderedmap)
            << QMetaAssociation::fromContainer<std::unordered_map<int, QMetaAssociation>>()
            << QMetaType::fromType<int>()
            << QMetaType::fromType<QMetaAssociation>()
            << true << true << true << false << false;
    QTest::addRow("QSet")
            << static_cast<void *>(&qset)
            << QMetaAssociation::fromContainer<QSet<QByteArray>>()
            << QMetaType::fromType<QByteArray>()
            << QMetaType()
            << true << true << false << false << false;
    QTest::addRow("std::set")
            << static_cast<void *>(&stdset)
            << QMetaAssociation::fromContainer<std::set<int>>()
            << QMetaType::fromType<int>()
            << QMetaType()
            << true << true << false << true << false;
}

void tst_QMetaContainer::testAssociation()
{
    QFETCH(void *, container);
    QFETCH(QMetaAssociation, metaAssociation);
    QFETCH(QMetaType, keyType);
    QFETCH(QMetaType, mappedType);
    QFETCH(bool, hasSize);
    QFETCH(bool, canRemove);
    QFETCH(bool, canSetMapped);
    QFETCH(bool, hasBidirectionalIterator);
    QFETCH(bool, hasRandomAccessIterator);

    QCOMPARE(metaAssociation.hasSize(), hasSize);
    QCOMPARE(metaAssociation.canRemoveKey(), canRemove);
    QCOMPARE(metaAssociation.canSetMappedAtKey(), canSetMapped);
    QCOMPARE(metaAssociation.canSetMappedAtIterator(), canSetMapped);

    // Apparently implementations can choose to provide "better" iterators than required by the std.
    if (hasBidirectionalIterator)
        QCOMPARE(metaAssociation.hasBidirectionalIterator(), hasBidirectionalIterator);
    if (hasRandomAccessIterator)
        QCOMPARE(metaAssociation.hasRandomAccessIterator(), hasRandomAccessIterator);

    QVariant key1(keyType);
    QVariant key2(keyType);
    QVariant key3(keyType);

    QVariant mapped1(mappedType);
    QVariant mapped2(mappedType);
    QVariant mapped3(mappedType);

    if (hasSize) {
        const qsizetype size = metaAssociation.size(container);

        QVERIFY(metaAssociation.canInsertKey());

        // var1 is invalid, and our containers do not contain an invalid key so far.
        metaAssociation.insertKey(container, key1.constData());
        QCOMPARE(metaAssociation.size(container), size + 1);
        metaAssociation.removeKey(container, key1.constData());
        QCOMPARE(metaAssociation.size(container), size);
    } else {
        metaAssociation.insertKey(container, key1.constData());
        metaAssociation.removeKey(container, key1.constData());
    }

    QVERIFY(metaAssociation.hasIterator());
    QVERIFY(metaAssociation.hasConstIterator());
    QVERIFY(metaAssociation.canGetKeyAtIterator());
    QVERIFY(metaAssociation.canGetKeyAtConstIterator());

    void *it = metaAssociation.begin(container);
    void *end = metaAssociation.end(container);
    QVERIFY(it);
    QVERIFY(end);

    void *constIt = metaAssociation.constBegin(container);
    void *constEnd = metaAssociation.constEnd(container);
    QVERIFY(constIt);
    QVERIFY(constEnd);

    const qsizetype size = metaAssociation.diffIterator(end, it);
    QCOMPARE(size, metaAssociation.diffConstIterator(constEnd, constIt));
    if (hasSize)
        QCOMPARE(size, metaAssociation.size(container));

    qsizetype count = 0;
    for (; !metaAssociation.compareIterator(it, end);
         metaAssociation.advanceIterator(it, 1), metaAssociation.advanceConstIterator(constIt, 1)) {
        metaAssociation.keyAtIterator(it, key1.data());
        metaAssociation.keyAtConstIterator(constIt, key3.data());
        QCOMPARE(key3, key1);
        ++count;
    }

    QCOMPARE(count, size);
    QVERIFY(metaAssociation.compareConstIterator(constIt, constEnd));

    metaAssociation.destroyIterator(it);
    metaAssociation.destroyIterator(end);
    metaAssociation.destroyConstIterator(constIt);
    metaAssociation.destroyConstIterator(constEnd);

    if (metaAssociation.canSetMappedAtIterator()) {
        void *it = metaAssociation.begin(container);
        void *end = metaAssociation.end(container);
        QVERIFY(it);
        QVERIFY(end);

        for (; !metaAssociation.compareIterator(it, end); metaAssociation.advanceIterator(it, 1)) {
            metaAssociation.mappedAtIterator(it, mapped1.data());
            metaAssociation.setMappedAtIterator(it, mapped2.constData());
            metaAssociation.mappedAtIterator(it, mapped3.data());
            QCOMPARE(mapped2, mapped3);
            mapped2 = mapped1;
        }

        metaAssociation.destroyIterator(it);
        metaAssociation.destroyIterator(end);

        it = metaAssociation.constBegin(container);
        end = metaAssociation.constEnd(container);
        QVERIFY(it);
        QVERIFY(end);

        for (; !metaAssociation.compareConstIterator(it, end); metaAssociation.advanceConstIterator(it, 1)) {
            metaAssociation.mappedAtConstIterator(it, mapped1.data());
            metaAssociation.keyAtConstIterator(it, key1.data());
            metaAssociation.setMappedAtKey(container, key1.constData(), mapped2.constData());
            metaAssociation.mappedAtConstIterator(it, mapped3.data());
            QCOMPARE(mapped2, mapped3);
            mapped2 = mapped1;
        }

        metaAssociation.destroyConstIterator(it);
        metaAssociation.destroyConstIterator(end);
    }

    if (metaAssociation.hasBidirectionalIterator()) {
        void *it = metaAssociation.end(container);
        void *end = metaAssociation.begin(container);
        QVERIFY(it);
        QVERIFY(end);

        void *constIt = metaAssociation.constEnd(container);
        void *constEnd = metaAssociation.constBegin(container);
        QVERIFY(constIt);
        QVERIFY(constEnd);

        qsizetype size = 0;
        if (metaAssociation.hasRandomAccessIterator()) {
            size = metaAssociation.diffIterator(end, it);
            QCOMPARE(size, metaAssociation.diffConstIterator(constEnd, constIt));
        } else {
            size = -metaAssociation.diffIterator(it, end);
        }

        if (hasSize)
            QCOMPARE(size, -metaAssociation.size(container));

        qsizetype count = 0;
        do {
            metaAssociation.advanceIterator(it, -1);
            metaAssociation.advanceConstIterator(constIt, -1);
            --count;

            metaAssociation.keyAtIterator(it, key1.data());
            metaAssociation.keyAtConstIterator(constIt, key3.data());
            QCOMPARE(key3, key1);
        } while (!metaAssociation.compareIterator(it, end));

        QCOMPARE(count, size);
        QVERIFY(metaAssociation.compareConstIterator(constIt, constEnd));

        metaAssociation.destroyIterator(it);
        metaAssociation.destroyIterator(end);
        metaAssociation.destroyConstIterator(constIt);
        metaAssociation.destroyConstIterator(constEnd);
    }

    QVERIFY(metaAssociation.canClear());
    constIt = metaAssociation.constBegin(container);
    constEnd = metaAssociation.constEnd(container);
    QVERIFY(!metaAssociation.compareConstIterator(constIt, constEnd));
    metaAssociation.destroyConstIterator(constIt);
    metaAssociation.destroyConstIterator(constEnd);

    metaAssociation.clear(container);
    constIt = metaAssociation.constBegin(container);
    constEnd = metaAssociation.constEnd(container);
    QVERIFY(metaAssociation.compareConstIterator(constIt, constEnd));
    metaAssociation.destroyConstIterator(constIt);
    metaAssociation.destroyConstIterator(constEnd);

    QVERIFY(metaAssociation.iface() != nullptr);
    QMetaAssociation defaultConstructed;
    QVERIFY(defaultConstructed.iface() == nullptr);
    QT_TEST_EQUALITY_OPS(QMetaAssociation(), QMetaAssociation(), true);
    QT_TEST_EQUALITY_OPS(QMetaAssociation(), metaAssociation, false);
}

QTEST_MAIN(tst_QMetaContainer)
#include "tst_qmetacontainer.moc"
