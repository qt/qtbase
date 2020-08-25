/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/qtest.h>
#include <QtCore/qcontainerinfo.h>
#include <QtCore/qmetacontainer.h>

#include <QtCore/qvector.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>

#include <vector>
#include <set>
#include <forward_list>

namespace CheckContainerTraits
{
struct NotAContainer {};

static_assert(QContainerTraits::has_size_v<QVector<int>>);
static_assert(QContainerTraits::has_size_v<QSet<int>>);
static_assert(!QContainerTraits::has_size_v<NotAContainer>);
static_assert(QContainerTraits::has_size_v<std::vector<int>>);
static_assert(QContainerTraits::has_size_v<std::set<int>>);
static_assert(!QContainerTraits::has_size_v<std::forward_list<int>>);

static_assert(QContainerTraits::has_clear_v<QVector<int>>);
static_assert(QContainerTraits::has_clear_v<QSet<int>>);
static_assert(!QContainerTraits::has_clear_v<NotAContainer>);
static_assert(QContainerTraits::has_clear_v<std::vector<int>>);
static_assert(QContainerTraits::has_clear_v<std::set<int>>);
static_assert(QContainerTraits::has_clear_v<std::forward_list<int>>);

static_assert(QContainerTraits::has_at_v<QVector<int>>);
static_assert(!QContainerTraits::has_at_v<QSet<int>>);
static_assert(!QContainerTraits::has_at_v<NotAContainer>);
static_assert(QContainerTraits::has_at_v<std::vector<int>>);
static_assert(!QContainerTraits::has_at_v<std::set<int>>);
static_assert(!QContainerTraits::has_at_v<std::forward_list<int>>);

static_assert(QContainerTraits::can_get_at_index_v<QVector<int>>);
static_assert(!QContainerTraits::can_get_at_index_v<QSet<int>>);
static_assert(!QContainerTraits::can_get_at_index_v<NotAContainer>);
static_assert(QContainerTraits::can_get_at_index_v<std::vector<int>>);
static_assert(!QContainerTraits::can_get_at_index_v<std::set<int>>);
static_assert(!QContainerTraits::can_get_at_index_v<std::forward_list<int>>);

static_assert(QContainerTraits::can_set_at_index_v<QVector<int>>);
static_assert(!QContainerTraits::can_set_at_index_v<QSet<int>>);
static_assert(!QContainerTraits::can_set_at_index_v<NotAContainer>);
static_assert(QContainerTraits::can_set_at_index_v<std::vector<int>>);
static_assert(!QContainerTraits::can_set_at_index_v<std::set<int>>);
static_assert(!QContainerTraits::can_set_at_index_v<std::forward_list<int>>);

static_assert(QContainerTraits::has_push_back_v<QVector<int>>);
static_assert(!QContainerTraits::has_push_back_v<QSet<int>>);
static_assert(!QContainerTraits::has_push_back_v<NotAContainer>);
static_assert(QContainerTraits::has_push_back_v<std::vector<int>>);
static_assert(!QContainerTraits::has_push_back_v<std::set<int>>);
static_assert(!QContainerTraits::has_push_back_v<std::forward_list<int>>);

static_assert(QContainerTraits::has_push_front_v<QVector<int>>);
static_assert(!QContainerTraits::has_push_front_v<QSet<int>>);
static_assert(!QContainerTraits::has_push_front_v<NotAContainer>);
static_assert(!QContainerTraits::has_push_front_v<std::vector<int>>);
static_assert(!QContainerTraits::has_push_front_v<std::set<int>>);
static_assert(QContainerTraits::has_push_front_v<std::forward_list<int>>);

static_assert(!QContainerTraits::has_insert_v<QVector<int>>);
static_assert(QContainerTraits::has_insert_v<QSet<int>>);
static_assert(!QContainerTraits::has_insert_v<NotAContainer>);
static_assert(!QContainerTraits::has_insert_v<std::vector<int>>);
static_assert(QContainerTraits::has_insert_v<std::set<int>>);
static_assert(!QContainerTraits::has_insert_v<std::forward_list<int>>);

static_assert(QContainerTraits::has_pop_back_v<QVector<int>>);
static_assert(!QContainerTraits::has_pop_back_v<QSet<int>>);
static_assert(!QContainerTraits::has_pop_back_v<NotAContainer>);
static_assert(QContainerTraits::has_pop_back_v<std::vector<int>>);
static_assert(!QContainerTraits::has_pop_back_v<std::set<int>>);
static_assert(!QContainerTraits::has_pop_back_v<std::forward_list<int>>);

static_assert(QContainerTraits::has_pop_front_v<QVector<int>>);
static_assert(!QContainerTraits::has_pop_front_v<QSet<int>>);
static_assert(!QContainerTraits::has_pop_front_v<NotAContainer>);
static_assert(!QContainerTraits::has_pop_front_v<std::vector<int>>);
static_assert(!QContainerTraits::has_pop_front_v<std::set<int>>);
static_assert(QContainerTraits::has_pop_front_v<std::forward_list<int>>);

static_assert(QContainerTraits::has_iterator_v<QVector<int>>);
static_assert(QContainerTraits::has_iterator_v<QSet<int>>);
static_assert(!QContainerTraits::has_iterator_v<NotAContainer>);
static_assert(QContainerTraits::has_iterator_v<std::vector<int>>);
static_assert(QContainerTraits::has_iterator_v<std::set<int>>);
static_assert(QContainerTraits::has_iterator_v<std::forward_list<int>>);

static_assert(QContainerTraits::has_const_iterator_v<QVector<int>>);
static_assert(QContainerTraits::has_const_iterator_v<QSet<int>>);
static_assert(!QContainerTraits::has_const_iterator_v<NotAContainer>);
static_assert(QContainerTraits::has_const_iterator_v<std::vector<int>>);
static_assert(QContainerTraits::has_const_iterator_v<std::set<int>>);
static_assert(QContainerTraits::has_const_iterator_v<std::forward_list<int>>);

static_assert(QContainerTraits::can_get_at_iterator_v<QVector<int>>);
static_assert(QContainerTraits::can_get_at_iterator_v<QSet<int>>);
static_assert(!QContainerTraits::can_get_at_iterator_v<NotAContainer>);
static_assert(QContainerTraits::can_get_at_iterator_v<std::vector<int>>);
static_assert(QContainerTraits::can_get_at_iterator_v<std::set<int>>);
static_assert(QContainerTraits::can_get_at_iterator_v<std::forward_list<int>>);

static_assert(QContainerTraits::can_set_at_iterator_v<QVector<int>>);
static_assert(!QContainerTraits::can_set_at_iterator_v<QSet<int>>);
static_assert(!QContainerTraits::can_get_at_iterator_v<NotAContainer>);
static_assert(QContainerTraits::can_set_at_iterator_v<std::vector<int>>);
static_assert(!QContainerTraits::can_set_at_iterator_v<std::set<int>>);
static_assert(QContainerTraits::can_set_at_iterator_v<std::forward_list<int>>);

static_assert(QContainerTraits::can_insert_at_iterator_v<QVector<int>>);
static_assert(!QContainerTraits::can_insert_at_iterator_v<QSet<int>>);
static_assert(!QContainerTraits::can_insert_at_iterator_v<NotAContainer>);
static_assert(QContainerTraits::can_insert_at_iterator_v<std::vector<int>>);
static_assert(!QContainerTraits::can_insert_at_iterator_v<std::forward_list<int>>);

// The iterator is only a hint, but syntactically indistinguishable from others.
// It's explicitly there to be signature compatible with std::vector::insert, though.
// Also, inserting into a set is not guaranteed to actually do anything.
static_assert(QContainerTraits::can_insert_at_iterator_v<std::set<int>>);

static_assert(QContainerTraits::can_erase_at_iterator_v<QVector<int>>);
static_assert(QContainerTraits::can_erase_at_iterator_v<QSet<int>>);
static_assert(!QContainerTraits::can_erase_at_iterator_v<NotAContainer>);
static_assert(QContainerTraits::can_erase_at_iterator_v<std::vector<int>>);
static_assert(QContainerTraits::can_erase_at_iterator_v<std::set<int>>);
static_assert(!QContainerTraits::can_erase_at_iterator_v<std::forward_list<int>>);

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

private slots:
    void init();
    void testSequence_data();
    void testSequence();
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
}

void tst_QMetaContainer::cleanup()
{
    qvector.clear();
    stdvector.clear();
    qset.clear();
    stdset.clear();
    forwardList.clear();
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
    QTest::addColumn<bool>("isOrdered");

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
            << true << false << false << false << false << false << true << false;
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
    QFETCH(bool, isOrdered);

    QVERIFY(metaSequence.canAddElement());
    QCOMPARE(metaSequence.hasSize(), hasSize);
    QCOMPARE(metaSequence.canGetElementAtIndex(), isIndexed);
    QCOMPARE(metaSequence.canSetElementAtIndex(), isIndexed);
    QCOMPARE(metaSequence.canRemoveElement(), canRemove);
    QCOMPARE(metaSequence.hasBidirectionalIterator(), hasBidirectionalIterator);
    QCOMPARE(metaSequence.hasRandomAccessIterator(), hasRandomAccessIterator);
    QCOMPARE(metaSequence.canInsertElementAtIterator(), canInsertAtIterator);
    QCOMPARE(metaSequence.canEraseElementAtIterator(), canEraseAtIterator);
    QCOMPARE(metaSequence.isOrdered(), isOrdered);

    QVariant var1(metaType);
    QVariant var2(metaType);
    QVariant var3(metaType);

    if (hasSize) {
        const qsizetype size = metaSequence.size(container);

        // var1 is invalid, and our sets do not contain an invalid value so far.
        metaSequence.addElement(container, var1.constData());
        QCOMPARE(metaSequence.size(container), size + 1);
        if (canRemove) {
            metaSequence.removeElement(container);
            QCOMPARE(metaSequence.size(container), size);
        }
    } else {
        metaSequence.addElement(container, var1.constData());
        if (canRemove)
            metaSequence.removeElement(container);
    }

    if (isIndexed) {
        QVERIFY(hasSize);
        const qsizetype size = metaSequence.size(container);
        for (qsizetype i = 0; i < size; ++i) {
            metaSequence.elementAtIndex(container, i, var1.data());
            metaSequence.elementAtIndex(container, size - i - 1, var2.data());

            metaSequence.setElementAtIndex(container, i, var2.constData());
            metaSequence.setElementAtIndex(container, size - i - 1, var1.constData());

            metaSequence.elementAtIndex(container, i, var3.data());
            QCOMPARE(var3, var2);

            metaSequence.elementAtIndex(container, size - i - 1, var3.data());
            QCOMPARE(var3, var1);
        }
    }

    QVERIFY(metaSequence.hasIterator());
    QVERIFY(metaSequence.hasConstIterator());
    QVERIFY(metaSequence.canGetElementAtIterator());
    QVERIFY(metaSequence.canGetElementAtConstIterator());

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
        metaSequence.elementAtIterator(it, var1.data());
        if (isIndexed) {
            metaSequence.elementAtIndex(container, count, var2.data());
            QCOMPARE(var1, var2);
        }
        metaSequence.elementAtConstIterator(constIt, var3.data());
        QCOMPARE(var3, var1);
        ++count;
    }

    QCOMPARE(count, size);
    QVERIFY(metaSequence.compareConstIterator(constIt, constEnd));

    metaSequence.destroyIterator(it);
    metaSequence.destroyIterator(end);
    metaSequence.destroyConstIterator(constIt);
    metaSequence.destroyConstIterator(constEnd);

    if (metaSequence.canSetElementAtIterator()) {
        void *it = metaSequence.begin(container);
        void *end = metaSequence.end(container);
        QVERIFY(it);
        QVERIFY(end);

        for (; !metaSequence.compareIterator(it, end); metaSequence.advanceIterator(it, 1)) {
            metaSequence.elementAtIterator(it, var1.data());
            metaSequence.setElementAtIterator(it, var2.constData());
            metaSequence.elementAtIterator(it, var3.data());
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

            metaSequence.elementAtIterator(it, var1.data());
            if (isIndexed) {
                metaSequence.elementAtIndex(container, count - size, var2.data());
                QCOMPARE(var1, var2);
            }
            metaSequence.elementAtConstIterator(constIt, var3.data());
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

        metaSequence.insertElementAtIterator(container, it, var1.constData());
        metaSequence.destroyIterator(it);
        it = metaSequence.begin(container);
        metaSequence.insertElementAtIterator(container, it, var2.constData());
        metaSequence.destroyIterator(it);
        it = metaSequence.begin(container);
        metaSequence.insertElementAtIterator(container, it, var3.constData());

        metaSequence.destroyIterator(it);

        it = metaSequence.begin(container);
        end = metaSequence.end(container);

        const qsizetype newSize = metaSequence.diffIterator(end, it);

        if (metaSequence.isOrdered()) {
            QCOMPARE(newSize, size + 3);
            QVariant var4(metaType);
            metaSequence.elementAtIterator(it, var4.data());
            QCOMPARE(var4, var3);
            metaSequence.advanceIterator(it, 1);
            metaSequence.elementAtIterator(it, var4.data());
            QCOMPARE(var4, var2);
            metaSequence.advanceIterator(it, 1);
            metaSequence.elementAtIterator(it, var4.data());
            QCOMPARE(var4, var1);
        } else {
            QVERIFY(newSize >= size);
        }

        if (canEraseAtIterator) {
            for (int i = 0; i < newSize; ++i) {
                metaSequence.destroyIterator(it);
                it = metaSequence.begin(container);
                metaSequence.eraseElementAtIterator(container, it);
            }

            metaSequence.destroyIterator(it);
            it = metaSequence.begin(container);
            metaSequence.destroyIterator(end);
            end = metaSequence.end(container);
            QVERIFY(metaSequence.compareIterator(it, end));

            metaSequence.addElement(container, var1.constData());
            metaSequence.addElement(container, var2.constData());
            metaSequence.addElement(container, var3.constData());
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
}

QTEST_MAIN(tst_QMetaContainer)
#include "tst_qmetacontainer.moc"
