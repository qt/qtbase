// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file tests Q_FOREACH over containers (centralize in a tst_qforeach?)

// test the container forwards
#include <QtContainerFwd>

static QCache<int, int> *cacheX;
static QHash<int, int> *hashX;
static QList<int> *listX;
static QMap<int, int> *mapX;
static QMultiHash<int, int> *multiHashX;
static QMultiMap<int, int> *multiMapX;
static QPair<int, int> *pairX;
static QQueue<int> *queueX;
static QSet<int> *setX;
static QStack<int> *stackX;
static QVarLengthArray<int> *varLengthArrayX;
static QVarLengthArray<int, 512> *varLengthArrayY;
static QList<int> *vectorX;

void foo()
{
    cacheX = 0;
    hashX = 0;
    listX = 0;
    mapX = 0;
    multiHashX = 0;
    multiMapX = 0;
    pairX = 0;
    queueX = 0;
    setX = 0;
    stackX = 0;
    varLengthArrayX = 0;
    varLengthArrayY = 0;
    vectorX = 0;
}

#include <QTest>
#include <QVector>
#include <QScopedPointer>
#include <QThread>
#include <QSemaphore>

#include <algorithm>

#include "qbitarray.h"
#include "qbytearray.h"
#include "qcache.h"
#include "qhash.h"
#include "qlist.h"
#include "qmap.h"
#include "qpair.h"
#include "qregularexpression.h"
#include "qset.h"
#include "qstack.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qvarlengtharray.h"
#include "qqueue.h"

class tst_Collections : public QObject
{
    Q_OBJECT

private slots:
    void typeinfo();
    void qstring();
    void list();
    void vector();
    void byteArray();
    void stack();
    void hash();
    void map();
    void bitArray();
    void cache();
#if QT_CONFIG(regularexpression)
    void regexp();
#endif
    void pair();
    void sharableQList();
    void sharableQVector();
    void sharableQMap();
    void sharableQHash();
    void q_foreach();
    void conversions();
    void javaStyleIterators();
    void constAndNonConstStlIterators();
    void vector_stl_data();
    void vector_stl();
    void list_stl_data();
    void list_stl();
    void q_init();
    void pointersize();
    void containerInstantiation();
    void qtimerList();
    void containerTypedefs();
    void forwardDeclared();
    void alignment();
    void QTBUG13079_collectionInsideCollection();

    void foreach_2();
    void insert_remove_loop();

    void detachAssociativeContainerQMap() { detachAssociativeContainerImpl<QMap>(); }
    void detachAssociativeContainerQMultiMap() { detachAssociativeContainerImpl<QMultiMap>(); }
    void detachAssociativeContainerQHash() { detachAssociativeContainerImpl<QHash>(); }
    void detachAssociativeContainerQMultiHash() { detachAssociativeContainerImpl<QMultiHash>(); }

private:
    template <template<typename, typename> typename Container>
    void detachAssociativeContainerImpl();
};

struct LargeStatic {
    static int count;
    LargeStatic():c(count) { ++count; }
    LargeStatic(const LargeStatic& o):c(o.c) { ++count; }
    LargeStatic &operator=(const LargeStatic &o)
    {
        c = o.c;
        ++count;
        return *this;
    };
    ~LargeStatic() { --count; }
    int c;
    int data[8];
};

int LargeStatic::count = 0;

struct Movable {
    static int count;
    Movable():c(count) { ++count; }
    Movable(const Movable& o):c(o.c) { ++count; }
    ~Movable() { --count; }
    int c;
};

int Movable::count = 0;
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Movable, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE


struct Pod {
    int i1, i2;
};

// Compile-time checks for recursive containers
struct Dummy
{
    bool operator==(const Dummy &) const { return false; }
    bool operator<(const Dummy &) const { return false; }
};

struct RecursiveList : public QList<RecursiveList>
{
    friend bool operator<(const RecursiveList &lhs, const RecursiveList &rhs)
    {
        using Base = QList<RecursiveList>;
        // compare some non-QList members here
        return static_cast<const Base &>(lhs) < static_cast<const Base &>(rhs);
    }
};
struct RecursiveSet : public QSet<RecursiveSet> {};
struct RecursiveMapV : public QMap<Dummy, RecursiveMapV> {};
struct RecursiveMapK : public QMap<RecursiveMapK, Dummy> {};
struct RecursiveMultiMapV : public QMultiMap<Dummy, RecursiveMultiMapV> {};
struct RecursiveMultiMapK : public QMultiMap<RecursiveMultiMapK, Dummy> {};
struct RecursiveHashV : public QHash<Dummy, RecursiveHashV> {};
struct RecursiveHashK : public QHash<RecursiveHashK, Dummy> {};
struct RecursiveMultiHashV : public QMultiHash<Dummy, RecursiveMultiHashV> {};
struct RecursiveMultiHashK : public QMultiHash<RecursiveMultiHashK, Dummy> {};

struct Empty {};
struct NoCmpParamRecursiveMapV : public QMap<Empty, NoCmpParamRecursiveMapV> {};
struct NoCmpParamRecursiveMapK : public QMap<NoCmpParamRecursiveMapK, Empty> {};
struct NoCmpParamRecursiveMultiMapV : public QMultiMap<Empty, NoCmpParamRecursiveMultiMapV> {};
struct NoCmpParamRecursiveMultiMapK : public QMultiMap<NoCmpParamRecursiveMultiMapK, Empty> {};
struct NoCmpParamRecursiveHashV : public QHash<Empty, NoCmpParamRecursiveHashV> {};
struct NoCmpParamRecursiveHashK : public QHash<NoCmpParamRecursiveHashK, Empty> {};
struct NoCmpParamRecursiveMultiHashV : public QMultiHash<Empty, NoCmpParamRecursiveMultiHashV> {};
struct NoCmpParamRecursiveMultiHashK : public QMultiHash<NoCmpParamRecursiveMultiHashK, Empty> {};

struct NoCmpRecursiveList : public QList<NoCmpRecursiveList>
{
    bool operator==(const NoCmpRecursiveList &) const = delete;
    bool operator<(const NoCmpRecursiveList &) const = delete;
};
struct NoCmpRecursiveSet : public QSet<NoCmpRecursiveSet>
{
    bool operator==(const NoCmpRecursiveSet &) const = delete;
};
struct NoCmpRecursiveMapV : public QMap<Dummy, NoCmpRecursiveMapV>
{
    bool operator==(const NoCmpRecursiveMapV &) const = delete;
};
struct NoCmpRecursiveMapK : public QMap<NoCmpRecursiveMapK, Dummy>
{
    bool operator==(const NoCmpRecursiveMapK &) const = delete;
};
struct NoCmpRecursiveMultiMapV : public QMultiMap<Dummy, NoCmpRecursiveMultiMapV>
{
    bool operator==(const NoCmpRecursiveMultiMapV &) const = delete;
};
struct NoCmpRecursiveMultiMapK : public QMultiMap<NoCmpRecursiveMultiMapK, Dummy>
{
    bool operator==(const NoCmpRecursiveMultiMapK &) const = delete;
};
struct NoCmpRecursiveHashV : public QHash<Dummy, NoCmpRecursiveHashV>
{
    bool operator==(const NoCmpRecursiveHashV &) const = delete;
};
struct NoCmpRecursiveHashK : public QHash<NoCmpRecursiveHashK, Dummy>
{
    bool operator==(const NoCmpRecursiveHashK &) const = delete;
};
struct NoCmpRecursiveMultiHashV : public QMultiHash<Dummy, NoCmpRecursiveMultiHashV>
{
    bool operator==(const NoCmpRecursiveMultiHashV &) const = delete;
};
struct NoCmpRecursiveMultiHashK : public QMultiHash<NoCmpRecursiveMultiHashK, Dummy>
{
    bool operator==(const NoCmpRecursiveMultiHashK &) const = delete;
};

uint qHash(const Dummy &) { return 0; }
uint qHash(const RecursiveSet &) { return 0; }
uint qHash(const RecursiveHashK &) { return 0; }
uint qHash(const RecursiveHashV &) { return 0; }
uint qHash(const RecursiveMultiHashK &) { return 0; }
uint qHash(const RecursiveMultiHashV &) { return 0; }

Q_DECLARE_METATYPE(RecursiveList);
Q_DECLARE_METATYPE(RecursiveSet);
Q_DECLARE_METATYPE(RecursiveMapV);
Q_DECLARE_METATYPE(RecursiveMapK);
Q_DECLARE_METATYPE(RecursiveMultiMapV);
Q_DECLARE_METATYPE(RecursiveMultiMapK);
Q_DECLARE_METATYPE(RecursiveHashV);
Q_DECLARE_METATYPE(RecursiveHashK);
Q_DECLARE_METATYPE(RecursiveMultiHashV);
Q_DECLARE_METATYPE(RecursiveMultiHashK);

Q_DECLARE_METATYPE(NoCmpParamRecursiveMapV);
Q_DECLARE_METATYPE(NoCmpParamRecursiveMapK);
Q_DECLARE_METATYPE(NoCmpParamRecursiveMultiMapV);
Q_DECLARE_METATYPE(NoCmpParamRecursiveMultiMapK);
Q_DECLARE_METATYPE(NoCmpParamRecursiveHashK);
Q_DECLARE_METATYPE(NoCmpParamRecursiveHashV);
Q_DECLARE_METATYPE(NoCmpParamRecursiveMultiHashK);
Q_DECLARE_METATYPE(NoCmpParamRecursiveMultiHashV);

Q_DECLARE_METATYPE(NoCmpRecursiveList);
Q_DECLARE_METATYPE(NoCmpRecursiveMapV);
Q_DECLARE_METATYPE(NoCmpRecursiveMapK);
Q_DECLARE_METATYPE(NoCmpRecursiveMultiMapV);
Q_DECLARE_METATYPE(NoCmpRecursiveMultiMapK);
Q_DECLARE_METATYPE(NoCmpRecursiveHashV);
Q_DECLARE_METATYPE(NoCmpRecursiveHashK);
Q_DECLARE_METATYPE(NoCmpRecursiveMultiHashV);
Q_DECLARE_METATYPE(NoCmpRecursiveMultiHashK);

static_assert(QTypeTraits::has_operator_equal_v<RecursiveList>);
static_assert(QTypeTraits::has_operator_less_than_v<RecursiveList>);
static_assert(QTypeTraits::has_operator_equal_v<RecursiveSet>);
static_assert(QTypeTraits::has_operator_equal_v<RecursiveMapV>);
static_assert(QTypeTraits::has_operator_equal_v<RecursiveMapK>);
static_assert(QTypeTraits::has_operator_equal_v<RecursiveMultiMapV>);
static_assert(QTypeTraits::has_operator_equal_v<RecursiveMultiMapK>);
static_assert(QTypeTraits::has_operator_equal_v<RecursiveHashV>);
static_assert(QTypeTraits::has_operator_equal_v<RecursiveHashK>);
static_assert(QTypeTraits::has_operator_equal_v<RecursiveMultiHashV>);
static_assert(QTypeTraits::has_operator_equal_v<RecursiveMultiHashK>);

static_assert(!QTypeTraits::has_operator_equal_v<NoCmpParamRecursiveMapV>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpParamRecursiveMapK>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpParamRecursiveMultiMapV>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpParamRecursiveMultiMapK>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpParamRecursiveHashV>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpParamRecursiveHashK>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpParamRecursiveMultiHashV>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpParamRecursiveMultiHashK>);

static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveList>);
static_assert(!QTypeTraits::has_operator_less_than_v<NoCmpRecursiveList>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveSet>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveMapV>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveMapK>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveMultiMapV>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveMultiMapK>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveHashV>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveHashK>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveMultiHashV>);
static_assert(!QTypeTraits::has_operator_equal_v<NoCmpRecursiveMultiHashK>);

template <typename T>
constexpr inline bool has_prepend_v = true;
template <typename T, qsizetype N>
constexpr inline bool has_prepend_v<QVarLengthArray<T,N>> = false; // deprecated in Qt 6.3

void tst_Collections::typeinfo()
{
    QVERIFY(std::is_pointer_v<int*>);
    QVERIFY(!std::is_pointer_v<int>);
    QVERIFY(QTypeInfo<QString>::isComplex);
    QVERIFY(!QTypeInfo<int>::isComplex);
}

void tst_Collections::list()
{
    {
        QList<int> list;
        QVERIFY(list.isEmpty());
        list.append(1);
        QVERIFY(list.size() == 1);

        QVERIFY(*list.begin() == 1);

        list.push_back(2);
        list += (3);
        list << 4 << 5 << 6;
        QVERIFY(!list.isEmpty());
        QVERIFY(list.size() == 6);
        QVERIFY(list.end() - list.begin() == list.size());

        {
            int sum = 0;
            QListIterator<int> i(list);
            while (i.hasNext())
                sum += i.next();
            QVERIFY(sum == 21);
        }

        {
            QList<int> list1;
            list1 << 1 << 2 << 3 << 5 << 7 << 8 << 9;
            QList<int> list2 = list1;

            QMutableListIterator<int> i1(list1);
            while (i1.hasNext()) {
                if (i1.next() % 2 != 0)
                    i1.remove();
            }

            QMutableListIterator<int> i2(list2);
            i2.toBack();
            while (i2.hasPrevious()) {
                if (i2.previous() % 2 != 0)
                    i2.remove();
            }
            QVERIFY(list1.size() == 2);
            QVERIFY(list2.size() == 2);
            QVERIFY(list1 == list2);
        }

        {
            int sum = 0;
            for (int i = 0; i < list.size(); ++i)
                sum += list[i];
            QVERIFY(sum == 21);
        }
        {
            int sum = 0;
            QList<int>::const_iterator i = list.begin();
            while (i != list.end())
                sum += *i++;
            QVERIFY(sum == 21);
        }
        {
            int sum = 0;
            QList<int>::ConstIterator i = list.begin();
            while (i != list.end())
                sum += *i++;
            QVERIFY(sum == 21);
        }
        {
            QList<int>::Iterator i = list.begin();
            i += 2;
            QCOMPARE(*i, 3);
            i -= 1;
            QCOMPARE(*i, 2);
        }
        {
            QList<int>::ConstIterator i = list.begin();
            i += 2;
            QCOMPARE(*i, 3);
            i -= 1;
            QCOMPARE(*i, 2);
        }
        {
            int sum = 0;
            int i;
            for (i = 0; i < list.size(); ++i)
                list[i] = list[i] +1;
            for (i = 0; i < list.size(); ++i)
                sum += list[i];
            QVERIFY(sum == 21 + list.size());
        }
        {
            int sum = 0;
            int i;
            for (i = 0; i < list.size(); ++i)
                --list[i];
            for (i = 0; i < list.size(); ++i)
                sum += list[i];
            QVERIFY(sum == 21);
        }
        {
            QMutableListIterator<int> i(list);
            while (i.hasNext())
                i.setValue(2*i.next());
        }
        {
            int sum = 0;
            QListIterator<int> i(list);
            i.toBack();
            while (i.hasPrevious())
                sum += i.previous();
            QVERIFY(sum == 2*21);
        }
        {
            QMutableListIterator<int> i(list);
            i.toBack();
            while (i.hasPrevious())
                i.setValue(2*i.previous());
        }
        {
            int sum = 0;
            QListIterator<int> i(list);
            i.toBack();
            while (i.hasPrevious())
                sum += i.previous();
            QVERIFY(sum == 2*2*21);
        }
        {
            QMutableListIterator<int> i(list);
            while (i.hasNext()) {
                int a = i.next();
                i.insert(a);
            }
        }
        {
            int sum = 0;
            QList<int>::iterator i = list.begin();
            while (i != list.end())
                sum += *i++;
            QVERIFY(sum == 2*2*2*21);
        }
        {
            int duplicates = 0;
            QListIterator<int> i(list);
            while (i.hasNext()) {
                int a = i.next();
                if (i.hasNext() && a == i.peekNext())
                    duplicates++;
            }
            QVERIFY(duplicates == 6);
        }
        {
            int duplicates = 0;
            QListIterator<int> i(list);
            i.toBack();
            while (i.hasPrevious()) {
                int a = i.previous();
                if (i.hasPrevious() && a == i.peekPrevious())
                    duplicates++;
            }
            QVERIFY(duplicates == 6);
        }
        {
            QMutableListIterator<int> i(list);
            while (i.hasNext()) {
                int a = i.next();
                if (i.hasNext() &&
                     i.peekNext() == a)
                    i.remove();
            }
        }
        {
            int duplicates = 0;
            QMutableListIterator<int> i = list;
            i.toBack();
            while (i.hasPrevious()) {
                int a = i.previous();
                if (i.hasPrevious() && a == i.peekPrevious())
                    duplicates++;
            }
            QVERIFY(duplicates == 0);
        }
        {
            QVERIFY(list.size() == 6);
            QMutableListIterator<int> i = list;
            while (i.hasNext()) {
                int a = i.peekNext();
                i.insert(42);
                QVERIFY(i.peekPrevious() == 42 && i.peekNext() == a);
                i.next();
            }
            QVERIFY(list.size() == 12);
            i.toFront();
            while (i.findNext(42))
                i.remove();
        }
        {
            QList<int> l;
            l << 4 << 8 << 12 << 16 << 20 << 24;
            QVERIFY(l == list);
            QList<int> copy = list;
            list += list;
            QVERIFY(l != list && l.size() == list.size()/2 && l == copy);
            l += copy;
            QVERIFY(l == list);
            list = copy;
        }
        {
            QList<int> copy = list;
            list << 8;
            QVERIFY(list.indexOf(8) == 1);
            QVERIFY(list.indexOf(8, list.indexOf(8)+1) == 6);
            int a = list.indexOf(8);
            QVERIFY(list.count(8) == 2);
            int r = list.removeAll(8);
            QVERIFY(r == 2);
            list.insert(a, 8);
            QVERIFY(list == copy);
        }
        {
            QList<QString> list;
            list << "one" << "two" << "three" << "four" << "five" << "six";
            while (!list.isEmpty())
                list.removeAll(list.first());
        }
        {
            QList<QString> list;
            list << "one" << "two" << "one" << "two";
            QVERIFY(!list.removeOne("three"));
            QVERIFY(list.removeOne("two"));
            QCOMPARE(list, QList<QString>() << "one" << "one" << "two");
            QVERIFY(list.removeOne("two"));
            QCOMPARE(list, QList<QString>() << "one" << "one");
            QVERIFY(!list.removeOne("two"));
            QCOMPARE(list, QList<QString>() << "one" << "one");
            QVERIFY(list.removeOne("one"));
            QCOMPARE(list, QList<QString>() << "one");
            QVERIFY(list.removeOne("one"));
            QVERIFY(list.isEmpty());
            QVERIFY(!list.removeOne("one"));
            QVERIFY(list.isEmpty());
        }
        {
            QList<int> copy = list;
            list << 8;
            QVERIFY(list.lastIndexOf(8) == 6);
            QVERIFY(list.lastIndexOf(8, list.lastIndexOf(8)-1) == 1);
            list = copy;
        }
        {
            QList<int> copy = list;
            list.insert(3, 999);
            QVERIFY(list[3] == 999);
            list.replace(3, 222);
            QVERIFY(list[3] == 222);
            QVERIFY(list.contains(222) && ! list.contains(999));
            list.removeAt(3);
            list = copy;
            QVERIFY(list == copy);
        }
        {
            list.clear();
            QVERIFY(list.isEmpty());
            QVERIFY(list.begin() == list.end());
            QListIterator<int> i(list);
            QVERIFY(!i.hasNext() && !i.hasPrevious());
        }
        {
            QList<int> l1;
            QList<int> l2;
            l1 << 1 << 2 << 3;
            l2 << 4 << 5 << 6;
            QList<int> l3 = l1 + l2;
            l1 += l2;
            QVERIFY(l3 == l1);
        }
        {
            QList<int> list;
            QVERIFY(list.isEmpty());
            list.append(1);
            QList<int> list2;
            list2 = list;
            list2.clear();
            QVERIFY(list2.size() == 0);
            QVERIFY(list.size() == 1);
        }
        {
            QList<int> list;
            list.append(1);
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wself-assign-overloaded")
            list = list;
QT_WARNING_POP
            QVERIFY(list.size() == 1);
        }
    }
    {
        QList<void*> list;
        list.append(0);
        list.append((void*)42);
        QCOMPARE(list.size(), 2);
        QCOMPARE(list.at(0), (void*)0);
        QCOMPARE(list.at(1), (void*)42);
    }

    {
        QList<QString> vector(5);
        vector[0] = "99";
        vector[4] ="100";
        QList<QString> list = vector.toList();

        QVERIFY(list.size() == 5);
        QCOMPARE(list.at(0), QLatin1String("99"));
        QCOMPARE(list.at(4), QLatin1String("100"));
        list[0] = "10";
        QCOMPARE(list.at(0), QLatin1String("10"));
        QCOMPARE(vector.at(0), QLatin1String("99"));

    }

    {
        QList<QString> list;
        list.append("Hello");

        QList<QString>::iterator it = list.begin();
        QVERIFY((*it)[0] == QChar('H'));
        QVERIFY(it->constData()[0] == QChar('H'));
        it->replace(QChar('H'), QChar('X'));
        QCOMPARE(list.first(), QLatin1String("Xello"));

        QList<QString>::const_iterator cit = list.constBegin();
        QCOMPARE((*cit).toLower(), QLatin1String("xello"));
        QCOMPARE(cit->toUpper(), QLatin1String("XELLO"));

        cit = list.cbegin();
        QCOMPARE((*cit).toLower(),QLatin1String("xello"));
        QCOMPARE(cit->toUpper(), QLatin1String("XELLO"));
    }

    {
        QList<int *> list;
        QVERIFY(list.value(0) == 0);
        int i;
        list.append(&i);
        QVERIFY(list.value(0) == &i);
    }
    {
        QList<const int *> list;
        QVERIFY(list.value(0) == 0);
        int i;
        list.append(&i);
        QVERIFY(list.value(0) == &i);
    }
    {
        QList<int> list;
        QVERIFY(list.value(0) == 0);
        list.append(10);
        QVERIFY(list.value(0) == 10);
    }
    {
        QList<Pod> list;
        QCOMPARE(list.value(0).i1, 0);
        QCOMPARE(list.value(0).i2, 0);
    }

    {
        QList<QString> list;
        list << "alpha" << "beta";
        list += list;
        QVERIFY(list.size() == 4);
        QCOMPARE(list.at(0), QLatin1String("alpha"));
        QCOMPARE(list.at(1), QLatin1String("beta"));
        QCOMPARE(list.at(2), QLatin1String("alpha"));
        QCOMPARE(list.at(3), QLatin1String("beta"));
    }

    // test endcases for inserting into a qlist
    {
        QList<QString> list;
        list << "foo" << "bar";
        QVERIFY(!list.isEmpty());

        list.insert(0, "atzero");
        QCOMPARE(list.at(0), QString("atzero"));

        int listCount = list.size();
        list.insert(listCount, "atcount");
        QCOMPARE(list.at(listCount), QString("atcount"));
    }

    {
        QList<int> list1;
        list1 << 0 << 1 << 2 << 3;
        list1.removeFirst();

        list1.swapItemsAt(0, 0);
        QVERIFY(list1 == QList<int>() << 1 << 2 << 3);

        list1.swapItemsAt(1, 1);
        QVERIFY(list1 == QList<int>() << 1 << 2 << 3);

        list1.swapItemsAt(2, 2);
        QVERIFY(list1 == QList<int>() << 1 << 2 << 3);

        list1.swapItemsAt(0, 1);
        QVERIFY(list1 == QList<int>() << 2 << 1 << 3);

        list1.swapItemsAt(0, 2);
        QVERIFY(list1 == QList<int>() << 3 << 1 << 2);

        list1.swapItemsAt(1, 2);
        QVERIFY(list1 == QList<int>() << 3 << 2 << 1);

        list1.swapItemsAt(1, 2);
        QVERIFY(list1 == QList<int>() << 3 << 1 << 2);

        QList<QString> list2;
        list2 << "1" << "2" << "3";

        list2.swapItemsAt(0, 0);
        QVERIFY(list2 == QList<QString>() << "1" << "2" << "3");

        list2.swapItemsAt(1, 1);
        QVERIFY(list2 == QList<QString>() << "1" << "2" << "3");

        list2.swapItemsAt(2, 2);
        QVERIFY(list2 == QList<QString>() << "1" << "2" << "3");

        list2.swapItemsAt(0, 1);
        QVERIFY(list2 == QList<QString>() << "2" << "1" << "3");

        list2.swapItemsAt(0, 2);
        QVERIFY(list2 == QList<QString>() << "3" << "1" << "2");

        list2.swapItemsAt(1, 2);
        QVERIFY(list2 == QList<QString>() << "3" << "2" << "1");

        list2.swapItemsAt(1, 2);
        QVERIFY(list2 == QList<QString>() << "3" << "1" << "2");

        QList<double> list3;
        list3 << 1.0 << 2.0 << 3.0;

        list3.swapItemsAt(0, 0);
        QVERIFY(list3 == QList<double>() << 1.0 << 2.0 << 3.0);

        list3.swapItemsAt(1, 1);
        QVERIFY(list3 == QList<double>() << 1.0 << 2.0 << 3.0);

        list3.swapItemsAt(2, 2);
        QVERIFY(list3 == QList<double>() << 1.0 << 2.0 << 3.0);

        list3.swapItemsAt(0, 1);
        QVERIFY(list3 == QList<double>() << 2.0 << 1.0 << 3.0);

        list3.swapItemsAt(0, 2);
        QVERIFY(list3 == QList<double>() << 3.0 << 1.0 << 2.0);

        list3.swapItemsAt(1, 2);
        QVERIFY(list3 == QList<double>() << 3.0 << 2.0 << 1.0);

        list3.swapItemsAt(1, 2);
        QVERIFY(list3 == QList<double>() << 3.0 << 1.0 << 2.0);
    }

    // Check what happens when using references to own items.
    // Ideally we should run valgrind on this.
    {
        int i;

        QList<void *> list1;
        list1.append(reinterpret_cast<void *>(50));

        for (i = 1; i < 100; ++i) {
            list1.append(list1.at(i - 1));
            list1.prepend(list1.at(i));
            list1.insert(i, list1.at(i - 1));
            list1.insert(i, list1.at(i));
            list1.insert(i, list1.at(i + 1));
            list1.replace(i, list1.at(i - 1));
            list1.replace(i, list1.at(i));
            list1.replace(i, list1.at(i + 1));
        }
        QCOMPARE(list1.size(), 496);
        for (i = 0; i < list1.size(); ++i) {
            QCOMPARE(list1.at(i), reinterpret_cast<void *>(50));
        }

        QList<QString> list2;
        list2.append("50");

        for (i = 1; i < 100; ++i) {
            list2.append(list2.at(i - 1));
            list2.prepend(list2.at(i));
            list2.insert(i, list2.at(i - 1));
            list2.insert(i, list2.at(i));
            list2.insert(i, list2.at(i + 1));
            list2.replace(i, list2.at(i - 1));
            list2.replace(i, list2.at(i));
            list2.replace(i, list2.at(i + 1));
        }
        QCOMPARE(list2.size(), 496);
        for (i = 0; i < list2.size(); ++i) {
            QCOMPARE(list2.at(i), QString::fromLatin1("50"));
        }

        QList<double> list3;
        list3.append(50.0);

        for (i = 1; i < 100; ++i) {
            list3.append(list3.at(i - 1));
            list3.prepend(list3.at(i));
            list3.insert(i, list3.at(i - 1));
            list3.insert(i, list3.at(i));
            list3.insert(i, list3.at(i + 1));
            list3.replace(i, list3.at(i - 1));
            list3.replace(i, list3.at(i));
            list3.replace(i, list3.at(i + 1));
        }
        QCOMPARE(list3.size(), 496);
        for (i = 0; i < list3.size(); ++i) {
            QCOMPARE(list3.at(i), 50.0);
        }

        QList<QTime> list4;
        list4.append(QTime(12, 34, 56));

        for (i = 1; i < 100; ++i) {
            list4.append(list4.at(i - 1));
            list4.prepend(list4.at(i));
            list4.insert(i, list4.at(i - 1));
            list4.insert(i, list4.at(i));
            list4.insert(i, list4.at(i + 1));
            list4.replace(i, list4.at(i - 1));
            list4.replace(i, list4.at(i));
            list4.replace(i, list4.at(i + 1));
        }
        QCOMPARE(list4.size(), 496);
        for (i = 0; i < list4.size(); ++i) {
            QVERIFY(list4.at(i) == QTime(12, 34, 56));
        }

    }
    {
        QList<int> a;
        QCOMPARE(a.startsWith(1), false);
        QCOMPARE(a.endsWith(1), false);
        a.append(1);
        QCOMPARE(a.startsWith(1), true);
        QCOMPARE(a.startsWith(2), false);
        QCOMPARE(a.endsWith(1), true);
        QCOMPARE(a.endsWith(2), false);
        a.append(2);
        QCOMPARE(a.startsWith(1), true);
        QCOMPARE(a.startsWith(2), false);
        QCOMPARE(a.endsWith(1), false);
        QCOMPARE(a.endsWith(2), true);
    }
}

void tst_Collections::vector()
{
    QList<int> v1;
    v1 << 1 << 2 << 3;
    QVector<int> v2;
    v2 << 4 << 5;
    QVector<int> v3;
    v3 << 1 << 2 << 3 << 4 << 5;
    QVERIFY(v1 + v2 == v3);

    QVector<int> emptyVector;
    // emptyVector.remove(3, -3); // Q_ASSERT_X() triggered with "index out of range" message.
    QCOMPARE(emptyVector.size(), 0);

    emptyVector.remove(0, 0);
    QCOMPARE(emptyVector.size(), 0);

    QVector<int> v4;
    v4 << 1 << 2 << 3;
    QCOMPARE(v4.size(), 3);
    v4.remove(1, 0);
    QCOMPARE(v4.size(), 3);

    QVector<int> v;
    v.append(2);
    QVERIFY(*v.begin() == 2);
    v.prepend(1);
    QVERIFY(*v.begin() == 1);
    QVERIFY(*(v.begin() + 1) == 2);

    v.clear();
    v << 1 << 2 << 3;
    v.insert(v.begin(), 0);
    v.insert(v.end(), 4);
    v.insert(v.begin()+2, 9);

    QVector<int> result;
    result << 0 << 1 << 9 << 2 << 3 << 4;

    QVERIFY( v == result );

    v.clear();
    v << 1 << 2 << 3;
    v.insert(0, 0);
    v.insert(4, 4);
    v.insert(2, 9);

    QVERIFY( v == result );

    QVector<QString> vec;
    vec << "foo" << "bar";
    vec.reserve( 512 );
    QCOMPARE(vec[0], QLatin1String("foo"));
    QCOMPARE(vec[1], QLatin1String("bar"));

    int initialLargeStaticCount = LargeStatic::count;
    {
        QVector<LargeStatic> vector;
        vector.append(LargeStatic());
        vector.resize(0);
    }
    QCOMPARE(LargeStatic::count, initialLargeStaticCount);

    {
        QVector<QString> vector;
        vector << "alpha" << "beta";
        vector += vector;
        QVERIFY(vector.size() == 4);
        QCOMPARE(vector.at(0), QLatin1String("alpha"));
        QCOMPARE(vector.at(1), QLatin1String("beta"));
        QCOMPARE(vector.at(2), QLatin1String("alpha"));
        QCOMPARE(vector.at(3), QLatin1String("beta"));
    }

    int originalLargeStaticCount = LargeStatic::count;
    {
        QVector<LargeStatic> vector(5);
    }
    QVERIFY(LargeStatic::count == originalLargeStaticCount);
    {
        QVector<LargeStatic> vector(5);
        QList<LargeStatic> list = vector.toList();
    }
    QVERIFY(LargeStatic::count == originalLargeStaticCount);
    {
        QVector<LargeStatic> vector;
        LargeStatic *dummy = nullptr;
        for (int i = 0; i < 10000; ++i) {
            delete dummy;
            dummy = new LargeStatic;
            vector.append(LargeStatic());
        }
        delete dummy;
    }
    QVERIFY(LargeStatic::count == originalLargeStaticCount);

    int originalMovableCount = Movable::count;
    {
        QVector<Movable> vector(5);
    }
    QVERIFY(Movable::count == originalMovableCount);
    {
        QVector<Movable> vector(5);
        QList<Movable> list = vector.toList();
    }
    QVERIFY(Movable::count == originalMovableCount);
    {
        QVector<Movable> vector;
        Movable *dummy = nullptr;
        for (int i = 0; i < 10000; ++i) {
            delete dummy;
            dummy = new Movable;
            vector.append(Movable());
        }
        delete dummy;
    }
    QVERIFY(Movable::count == originalMovableCount);

    // Check what happens when using references to own items.
    // Ideally we should run valgrind on this.
    {
        int i;

        QVector<void *> vect1;
        vect1.append(reinterpret_cast<void *>(50));

        for (i = 1; i < 100; ++i) {
            vect1.append(vect1.at(i - 1));
            vect1.prepend(vect1.at(i));
            vect1.insert(i, vect1.at(i - 1));
            vect1.insert(i, vect1.at(i));
            vect1.insert(i, vect1.at(i + 1));
            vect1.replace(i, vect1.at(i - 1));
            vect1.replace(i, vect1.at(i));
            vect1.replace(i, vect1.at(i + 1));
        }
        QCOMPARE(vect1.size(), 496);
        for (i = 0; i < vect1.size(); ++i) {
            QCOMPARE(vect1.at(i), reinterpret_cast<void *>(50));
        }

        QVector<QString> vect2;
        vect2.append("50");

        for (i = 1; i < 100; ++i) {
            vect2.append(vect2.at(i - 1));
            vect2.prepend(vect2.at(i));
            vect2.insert(i, vect2.at(i - 1));
            vect2.insert(i, vect2.at(i));
            vect2.insert(i, vect2.at(i + 1));
            vect2.replace(i, vect2.at(i - 1));
            vect2.replace(i, vect2.at(i));
            vect2.replace(i, vect2.at(i + 1));
        }
        QCOMPARE(vect2.size(), 496);
        for (i = 0; i < vect2.size(); ++i) {
            QCOMPARE(vect2.at(i), QString::fromLatin1("50"));
        }

        QVector<double> vect3;
        vect3.append(50.0);

        for (i = 1; i < 100; ++i) {
            vect3.append(vect3.at(i - 1));
            vect3.prepend(vect3.at(i));
            vect3.insert(i, vect3.at(i - 1));
            vect3.insert(i, vect3.at(i));
            vect3.insert(i, vect3.at(i + 1));
            vect3.replace(i, vect3.at(i - 1));
            vect3.replace(i, vect3.at(i));
            vect3.replace(i, vect3.at(i + 1));
        }
        QCOMPARE(vect3.size(), 496);
        for (i = 0; i < vect3.size(); ++i) {
            QCOMPARE(vect3.at(i), 50.0);
        }

        QVector<QTime> vect4;
        vect4.append(QTime(12, 34, 56));

        for (i = 1; i < 100; ++i) {
            vect4.append(vect4.at(i - 1));
            vect4.prepend(vect4.at(i));
            vect4.insert(i, vect4.at(i - 1));
            vect4.insert(i, vect4.at(i));
            vect4.insert(i, vect4.at(i + 1));
            vect4.replace(i, vect4.at(i - 1));
            vect4.replace(i, vect4.at(i));
            vect4.replace(i, vect4.at(i + 1));
        }
        QCOMPARE(vect4.size(), 496);
        for (i = 0; i < vect4.size(); ++i) {
            QVERIFY(vect4.at(i) == QTime(12, 34, 56));
        }
    }

    // this used to trigger an uninitialized read in valgrind
    QVector<char> foo;
    foo.resize(144);

    {
        QVector<int> a;
        QCOMPARE(a.startsWith(1), false);
        QCOMPARE(a.endsWith(1), false);
        a.append(1);
        QCOMPARE(a.startsWith(1), true);
        QCOMPARE(a.startsWith(2), false);
        QCOMPARE(a.endsWith(1), true);
        QCOMPARE(a.endsWith(2), false);
        a.append(2);
        QCOMPARE(a.startsWith(1), true);
        QCOMPARE(a.startsWith(2), false);
        QCOMPARE(a.endsWith(1), false);
        QCOMPARE(a.endsWith(2), true);
    }
}

void tst_Collections::byteArray()
{
    QByteArray hello = "hello";
    QByteArray ello = "ello";
    QByteArray World = "World";
    QByteArray Wor = "Wor";
    QByteArray helloWorld = "helloWorld";
    QVERIFY(hello + World == helloWorld);
    QVERIFY(hello + "World" == helloWorld);
    QVERIFY("hello" + World == helloWorld);

    QVERIFY('h' + ello == hello);
    QVERIFY(Wor + 'l' + 'd' == "World");
    QVERIFY(hello + World == "helloWorld");
    QVERIFY(hello + "World" == "helloWorld");
    QVERIFY("hello" + World == "helloWorld");
    QVERIFY('h' + ello == "hello");
    QVERIFY(Wor + 'l' + 'd' == "World");
    QVERIFY("helloWorld" == hello + World);
    QVERIFY("helloWorld" == hello + "World");
    QVERIFY("helloWorld" == "hello" + World);
    QVERIFY("hello" == 'h' + ello);
    QVERIFY("World" == Wor + 'l' + 'd');

    QVERIFY(hello.contains('e'));
    QVERIFY (true == hello.contains('e'));
    QVERIFY (hello.contains('e') != false);

    QVERIFY(hello.indexOf('e') == 1);
    QVERIFY(hello.indexOf('e', -10) == 1);
    QVERIFY(hello.indexOf('l') == 2);
    QVERIFY(hello.indexOf('l',2) == 2);
    QVERIFY(hello.indexOf('l',3) == 3);

    QByteArray empty;
    QCOMPARE(empty.indexOf("x"), -1);
    QCOMPARE(empty.lastIndexOf("x"), -1);
    QCOMPARE(empty.lastIndexOf("x", 0), -1);
    QCOMPARE(empty.count("x"), 0);
    QCOMPARE(empty.indexOf(""), 0);
    QCOMPARE(empty.lastIndexOf(""), 0);
    QCOMPARE(empty.lastIndexOf("", -1), -1);
    QCOMPARE(empty.count(""), 1);

    QByteArray large = "000 100 200 300 400 500 600 700 800 900";

    QVERIFY(large.indexOf("700") == 28);
    QVERIFY(large.indexOf("700", 28) == 28);
    QVERIFY(large.indexOf("700", 29) == -1);
    QVERIFY(large.lastIndexOf("700") == 28);
    QVERIFY(large.lastIndexOf("700", 28) == 28);
    QVERIFY(large.lastIndexOf("700", 27) == -1);

    QVERIFY(large.contains("200"));
    QVERIFY(!large.contains("201"));
    QVERIFY(large.contains('3'));
    QVERIFY(!large.contains('a'));

    QVERIFY(large.count("00") == 11);
    QVERIFY(large.count('3') == 1);
    QVERIFY(large.count('0') == 21);
    QVERIFY(large.count("0") == 21);
    QVERIFY(large.count("200") == 1);
    QVERIFY(large.count("201") == 0);

    QVERIFY(hello.left(0) == "");
    QVERIFY(!hello.left(0).isNull());
    QVERIFY(hello.left(1) == "h");
    QVERIFY(hello.left(2) == "he");
    QVERIFY(hello.left(200) == "hello");
    QVERIFY(hello.left(hello.size()) == hello);
    QVERIFY(hello.left(hello.size()+1) == hello);

    QVERIFY(hello.right(0) == "");
    QVERIFY(!hello.right(0).isNull());
    QVERIFY(hello.right(1) == "o");
    QVERIFY(hello.right(2) == "lo");
    QVERIFY(hello.right(200) == "hello");
    QVERIFY(hello.right(hello.size()) == hello);
    QVERIFY(hello.right(hello.size()+1) == hello);

    QVERIFY(!hello.mid(0, 0).isNull());
    QVERIFY(hello.mid(0, 1) == "h");
    QVERIFY(hello.mid(0, 2) == "he");
    QVERIFY(hello.mid(0, 200) == "hello");
    QVERIFY(hello.mid(0) == "hello");
    QVERIFY(hello.mid(0, hello.size()) == hello);
    QVERIFY(hello.mid(0, hello.size()+1) == hello);

    QVERIFY(hello.mid(hello.size()-0) == "");
    QVERIFY(hello.mid(hello.size()-0).isEmpty());
    QVERIFY(!hello.mid(hello.size()-0).isNull());
    QVERIFY(hello.mid(hello.size()-1) == "o");
    QVERIFY(hello.mid(hello.size()-2) == "lo");
    QVERIFY(hello.mid(hello.size()-200) == "hello");

    QByteArray nullByteArray;
    QByteArray nonNullByteArray = "";
    QVERIFY(nullByteArray.left(10).isNull());
    QVERIFY(nullByteArray.mid(0).isNull());

    QVERIFY(nullByteArray.isEmpty() == nonNullByteArray.isEmpty());
    QVERIFY(nullByteArray.size() == nonNullByteArray.size());

    QVERIFY(nullByteArray == QByteArray()); // QByteArray() is both null and empty.
    QVERIFY(QByteArray()  == nullByteArray);

    QVERIFY(nonNullByteArray == QByteArray("")); // QByteArray("") is empty, but not null.
    QVERIFY(QByteArray("") == nonNullByteArray);

    QVERIFY(nullByteArray == nonNullByteArray);
    QVERIFY(QByteArray() == QByteArray(""));

    QByteArray str = "Hello";
    QByteArray cstr(str.data(), str.size());
    QVERIFY(str == "Hello");
    QVERIFY(cstr == "Hello");
    cstr.clear();
    QVERIFY(str == "Hello");
    QVERIFY(cstr.isEmpty());

    {
        QByteArray ba1("Foo");
        ba1.prepend(ba1);
        QCOMPARE(ba1, QByteArray("FooFoo"));
        ba1.append(ba1);
        QCOMPARE(ba1, QByteArray("FooFooFooFoo"));
        ba1.insert(2, ba1);
        QCOMPARE(ba1, QByteArray("FoFooFooFooFoooFooFooFoo"));
        ba1.replace(3, 3, ba1);
        QCOMPARE(ba1, QByteArray("FoFFoFooFooFooFoooFooFooFooooFooFoooFooFooFoo"));
        ba1 = "FooFoo";
        ba1.replace(char('F'), ba1);
        QCOMPARE(ba1, QByteArray("FooFooooFooFoooo"));
        ba1 = "FooFoo";
        ba1.replace(char('o'), ba1);
        QCOMPARE(ba1, QByteArray("FFooFooFooFooFFooFooFooFoo"));

        ba1.replace(ba1, "xxx");
        QCOMPARE(ba1, QByteArray("xxx"));
        ba1.replace(ba1, QByteArray("yyy"));
        QCOMPARE(ba1, QByteArray("yyy"));
        ba1 += ba1;
        QCOMPARE(ba1, QByteArray("yyyyyy"));

        ba1.remove(1, -1); // do nothing
        QCOMPARE(ba1, QByteArray("yyyyyy"));

        ba1.replace(0, -1, "ZZZ");
        QCOMPARE(ba1, QByteArray("ZZZyyyyyy"));
    }
};

void tst_Collections::stack()
{
    QStack<int> stack;
    stack.push(1);
    stack.push(2);
    stack.push(3);
    QListIterator<int> i = stack;
    i.toBack();
    int sum = 0;
    while (i.hasPrevious())
        sum += i.previous();
    QVERIFY(sum == 6);

    sum = 0;
    for (QStack<int>::iterator i = stack.begin(); i != stack.end(); ++i)
        sum += *i;
    QVERIFY(sum == 6);

    while (!stack.isEmpty())
        sum -= stack.pop();
    QVERIFY(sum == 0);
}

void tst_Collections::hash()
{
    const char *hello = "hello";
    const char *world = "world";
    const char *allo = "allo";
    const char *monde = "monde";

    {
        typedef QHash<QString, QString> Hash;
        Hash hash;
        QString key = QLatin1String("  ");
        for (int i = 0; i < 10; ++i) {
            key[0] = QChar(i + '0');
            for (int j = 0; j < 10; ++j) {
                key[1] = QChar(j + '0');
                hash.insert(key, "V" + key);
            }
        }

        for (int i = 0; i < 10; ++i) {
            key[0] = QChar(i + '0');
            for (int j = 0; j < 10; ++j) {
                key[1] = QChar(j + '0');
                hash.remove(key);
            }
        }
    }

    {
        typedef QHash<int, const char *> Hash;
        Hash hash;
        hash.insert(1, hello);
        hash.insert(2, world);

        QVERIFY(hash.size() == 2);
        QVERIFY(!hash.isEmpty());

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wself-assign-overloaded")
        {
            Hash hash2 = hash;
            hash2 = hash;
            hash = hash2;
            hash2 = hash2;
            hash = hash;
            hash2.clear();
            hash2 = hash2;
            QVERIFY(hash2.size() == 0);
            QVERIFY(hash2.isEmpty());
        }
        QVERIFY(hash.size() == 2);
QT_WARNING_POP

        {
            Hash hash2 = hash;
            hash2[1] = allo;
            hash2[2] = monde;

            QVERIFY(hash2[1] == allo);
            QVERIFY(hash2[2] == monde);
            QVERIFY(hash[1] == hello);
            QVERIFY(hash[2] == world);

            hash2[1] = hash[1];
            hash2[2] = hash[2];

            QVERIFY(hash2[1] == hello);
            QVERIFY(hash2[2] == world);

            hash[1] = hash[1];
            QVERIFY(hash[1] == hello);
        }

        {
            Hash hash2 = hash;
            hash2.detach();
            hash2.remove(1);
            QVERIFY(hash2.size() == 1);
            hash2.remove(1);
            QVERIFY(hash2.size() == 1);
            hash2.remove(0);
            QVERIFY(hash2.size() == 1);
            hash2.remove(2);
            QVERIFY(hash2.size() == 0);
            QVERIFY(hash.size() == 2);
        }

        hash.detach();

        {
            Hash::iterator it1 = hash.find(1);
            QVERIFY(it1 != hash.end());

            Hash::iterator it2 = hash.find(0);
            QVERIFY(it2 != hash.begin());
            QVERIFY(it2 == hash.end());

            *it1 = monde;
            QVERIFY(*it1 == monde);
            QVERIFY(hash[1] == monde);

            *it1 = hello;
            QVERIFY(*it1 == hello);
            QVERIFY(hash[1] == hello);

            hash[1] = monde;
            QVERIFY(it1.key() == 1);
            QVERIFY(it1.value() == monde);
            QVERIFY(*it1 == monde);
            QVERIFY(hash[1] == monde);

            hash[1] = hello;
            QVERIFY(*it1 == hello);
            QVERIFY(hash[1] == hello);
        }

        {
            const Hash hash2 = hash;

            Hash::const_iterator it1 = hash2.find(1);
            QVERIFY(it1 != hash2.end());
            QVERIFY(it1.key() == 1);
            QVERIFY(it1.value() == hello);
            QVERIFY(*it1 == hello);

            Hash::const_iterator it2 = hash2.find(2);
            QVERIFY(it1 != it2);
            QVERIFY(it1 != hash2.end());
            QVERIFY(it2 != hash2.end());

            int count = 0;
            it1 = hash2.begin();
            while (it1 != hash2.end()) {
                count++;
                ++it1;
            }
            QVERIFY(count == 2);

            count = 0;
            it1 = hash.begin();
            while (it1 != hash.end()) {
                count++;
                ++it1;
            }
            QVERIFY(count == 2);
        }

        {
            QVERIFY(hash.contains(1));
            QVERIFY(hash.contains(2));
            QVERIFY(!hash.contains(0));
            QVERIFY(!hash.contains(3));
        }

        {
            QVERIFY(hash.value(1) == hello);
            QVERIFY(hash.value(2) == world);
            QVERIFY(hash.value(3) == 0);
            QVERIFY(hash.value(1, allo) == hello);
            QVERIFY(hash.value(2, allo) == world);
            QVERIFY(hash.value(3, allo) == allo);
            QVERIFY(hash.value(0, monde) == monde);
        }

        {
            QHash<int,LargeStatic> hash;
            for (int i = 0; i < 10; i++)
                hash.insert(i, LargeStatic());
            QVERIFY(LargeStatic::count == 10);
            hash.remove(7);
            QVERIFY(LargeStatic::count == 9);

        }
        QVERIFY(LargeStatic::count == 0);
        {
            QHash<int, int*> hash;
            QVERIFY(((const QHash<int,int*>*) &hash)->operator[](7) == 0);
        }

        {
            /*
                This test relies on a certain implementation of
                QHash. If you change the way QHash works internally,
                change this test as well.
            */
            QHash<int, int> hash;
            for (int i = 0; i < 1000; ++i)
                hash.insert(i, i);
            QVERIFY(hash.capacity() > 1000);

            for (int j = 0; j < 900; ++j)
                hash.remove(j);
            QVERIFY(hash.capacity() > 1000);
            hash.squeeze();
            QVERIFY(hash.capacity() < 200);
        }
    }

    {
        QHash<int, QString> hash;
        hash.insert(0, "Hello");

        QHash<int, QString>::iterator it = hash.begin();
        QVERIFY((*it)[0] == QChar('H'));
        QVERIFY(it->constData()[0] == QChar('H'));
        it->replace(QChar('H'), QChar('X'));
        QCOMPARE(*hash.begin(), QLatin1String("Xello"));

        QHash<int, QString>::const_iterator cit = hash.constBegin();
        QCOMPARE((*cit).toLower(), QLatin1String("xello"));
        QCOMPARE(cit->toUpper(), QLatin1String("XELLO"));

        cit = hash.cbegin();
        QCOMPARE((*cit).toLower(), QLatin1String("xello"));
        QCOMPARE(cit->toUpper(), QLatin1String("XELLO"));
    }

    {
        QMultiHash<int, QString> hash1, hash2;
        hash1.insert(1, "Alpha");
        hash1.insert(1, "Gamma");
        hash2.insert(1, "Beta");
        hash2.insert(1, "Gamma");
        hash2.insert(1, "Gamma");

        hash1.unite(hash2);
        QCOMPARE(hash1.size(), 5);
        auto values = hash1.values();
        std::sort(values.begin(), values.end());
        QList<QString> expected;
        expected << "Gamma" << "Gamma" << "Beta" << "Gamma" << "Alpha";
        std::sort(expected.begin(), expected.end());
        QCOMPARE(values, expected);

        hash2 = hash1;
        hash2.unite(hash2);
        QCOMPARE(hash2.size(), 10);
        values = hash2.values();
        std::sort(values.begin(), values.end());
        expected += expected;
        std::sort(expected.begin(), expected.end());
        QCOMPARE(values, expected);
    }
}

void tst_Collections::map()
{
    const char *hello = "hello";
    const char *world = "world";
    const char *allo = "allo";
    const char *monde = "monde";

    {
        typedef QMap<int, const char *> Map;
        Map map;
        map.insert(1, hello);
        map.insert(2, world);

        QVERIFY(*map.begin() == hello);

        QVERIFY(map.size() == 2);
        QVERIFY(!map.isEmpty());

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wself-assign-overloaded")
        {
            Map map2 = map;
            map2 = map;
            map = map2;
            map2 = map2;
            map = map;
            map2.clear();
            map2 = map2;
            QVERIFY(map2.size() == 0);
            QVERIFY(map2.isEmpty());
        }
        QVERIFY(map.size() == 2);
QT_WARNING_POP

        {
            Map map2 = map;
            map2[1] = allo;
            map2[2] = monde;

            QVERIFY(map2[1] == allo);
            QVERIFY(map2[2] == monde);
            QVERIFY(map[1] == hello);
            QVERIFY(map[2] == world);

            map2[1] = map[1];
            map2[2] = map[2];

            QVERIFY(map2[1] == hello);
            QVERIFY(map2[2] == world);

            map[1] = map[1];
            QVERIFY(map[1] == hello);
        }

        {
            Map map2 = map;
            map2.detach();
            map2.remove(1);
            QVERIFY(map2.size() == 1);
            map2.remove(1);
            QVERIFY(map2.size() == 1);
            map2.remove(0);
            QVERIFY(map2.size() == 1);
            map2.remove(2);
            QVERIFY(map2.size() == 0);
            QVERIFY(map.size() == 2);
        }

        map.detach();

        {
            Map::iterator it1 = map.find(1);
            QVERIFY(it1 == map.begin());
            QVERIFY(it1 != map.end());

            Map::iterator it2 = map.find(0);
            QVERIFY(it2 != map.begin());
            QVERIFY(it2 == map.end());

            *it1 = monde;
            QVERIFY(*it1 == monde);
            QVERIFY(map[1] == monde);

            *it1 = hello;
            QVERIFY(*it1 == hello);
            QVERIFY(map[1] == hello);

            map[1] = monde;
            QVERIFY(it1.key() == 1);
            QVERIFY(it1.value() == monde);
            QVERIFY(*it1 == monde);
            QVERIFY(map[1] == monde);

            map[1] = hello;
            QVERIFY(*it1 == hello);
            QVERIFY(map[1] == hello);

            *++it1 = allo;
            QVERIFY(*it1 == allo);
            QVERIFY(map[2] == allo);
            *it1 = world;

            ++it1;
            QVERIFY(it1 == map.end());

            int count = 0;
            it1 = map.begin();
            while (it1 != map.end()) {
                count++;
                ++it1;
            }
            QVERIFY(count == 2);
        }

        {
            const Map map2 = map;

            Map::const_iterator it1 = map2.find(1);
            QVERIFY(it1 != map2.end());
            QVERIFY(it1.key() == 1);
            QVERIFY(it1.value() == hello);
            QVERIFY(*it1 == hello);
            ++it1;

            Map::const_iterator it2 = map2.find(2);
            QVERIFY(it1 == it2);
            ++it1;
            QVERIFY(it1 == map2.end());
            QVERIFY(it2 != map2.end());
            QVERIFY(it1 != it2);

            int count = 0;
            it1 = map2.begin();
            while (it1 != map2.end()) {
                count++;
                ++it1;
            }
            QVERIFY(count == 2);

            count = 0;
            it1 = map.begin();
            while (it1 != map.end()) {
                count++;
                ++it1;
            }
            QVERIFY(count == 2);
        }

        {
            QVERIFY(map.contains(1));
            QVERIFY(map.contains(2));
            QVERIFY(!map.contains(0));
            QVERIFY(!map.contains(3));
        }

        {
            QVERIFY(map.value(1) == hello);
            QVERIFY(map.value(2) == world);
            QVERIFY(map.value(3) == 0);
            QVERIFY(map.value(1, allo) == hello);
            QVERIFY(map.value(2, allo) == world);
            QVERIFY(map.value(3, allo) == allo);
            QVERIFY(map.value(0, monde) == monde);
        }
        int originalLargeStaticCount = LargeStatic::count;
        {
            QMap<int,LargeStatic> map;
            for (int i = 0; i < 10; i++)
                map.insert(i, LargeStatic());
            QVERIFY(LargeStatic::count == (originalLargeStaticCount + 10));
            map.remove(7);
            QVERIFY(LargeStatic::count == (originalLargeStaticCount + 9));

        }
        QVERIFY(LargeStatic::count == originalLargeStaticCount);
        {
            QMap<int, int*> map;
            QVERIFY(((const QMap<int,int*>*) &map)->operator[](7) == 0);
        }

        {
            QMap<int, int> map;
            map[0] = 1;
            map[1] = 2;
            map[2] = 4;
            map[3] = 8;
            int sum = 0;
            int sumkey = 0;
            QMapIterator<int,int> i = map;
            while (i.hasNext()) {
                sum += i.next().value();
                sumkey += i.key();
            }
            QVERIFY(sum == 15);
            QVERIFY(sumkey == 6);
        }
        {
            QMap<int, int> map;
            map[0] = 1;
            map[1] = 2;
            map[2] = 4;
            map[3] = 8;
            int sum = 0;
            QMutableMapIterator<int,int> i = map;
            while (i.hasNext())
                if (i.next().key()  == 2)
                    i.remove();
            i.toFront();
            while (i.hasNext()) {
                sum += i.next().value();
                i.setValue(10);
                i.value() += 22;
                QVERIFY(i.value() == 32);
            }
            QVERIFY(sum == 11);
        }
        {
            QMap<int, int> map;
            map[0] = 1;
            QMutableMapIterator<int,int> i(map);
            i.toBack();
            while (i.hasPrevious()) {
                i.previous();
                QCOMPARE(i.key(), 0);
                QCOMPARE(i.value(), 1);
            }
        }
    }

    {
        QMultiMap<QString, int> map1;
        map1.insert("1", 2);
        map1.insert("1", 1);
        map1.insert("a", 3);
        map1.insert("a", 2);
        map1.insert("a", 1);
        map1.insert("b", 2);
        map1.insert("b", 1);

        QMultiMap<QString, int>::iterator j1, k1;

        j1 = map1.lowerBound("0"); k1 = map1.upperBound("0");
        QVERIFY(j1 == map1.begin() && k1 == j1);
        j1 = map1.lowerBound("00"); k1 = map1.upperBound("00");
        QVERIFY(j1 == map1.find("1") && k1 == j1);
        j1 = map1.lowerBound("1"); k1 = map1.upperBound("1");
        QVERIFY(j1 == map1.find("1") && --(--k1) == j1);
        j1 = map1.lowerBound("11"); k1 = map1.upperBound("11");
        QVERIFY(j1 == map1.find("a") && k1 == j1);
        j1 = map1.lowerBound("a"); k1 = map1.upperBound("a");
        QVERIFY(j1 == map1.find("a") && k1 == map1.find("b"));
        QVERIFY(j1.value() == 1 && j1.value() == 1);
        j1 = map1.lowerBound("aa"); k1 = map1.upperBound("aa");
        QVERIFY(j1 == map1.find("b") && k1 == j1);
        QVERIFY(j1.value() == 1);
        j1 = map1.lowerBound("b"); k1 = map1.upperBound("b");
        QVERIFY(j1 == map1.find("b") && k1 == map1.end());
        QVERIFY(j1.value() == 1);
        j1 = map1.lowerBound("bb"); k1 = map1.upperBound("bb");
        QVERIFY(j1 == map1.end() && k1 == j1);

        const QMultiMap<QString, int> map2 = map1;
        QMultiMap<QString, int>::const_iterator j2, k2;

        j2 = map2.lowerBound("0"); k2 = map2.upperBound("0");
        QVERIFY(j2 == map2.begin() && k2 == j2);
        j2 = map2.lowerBound("00"); k2 = map2.upperBound("00");
        QVERIFY(j2 == map2.find("1") && k2 == j2);
        j2 = map2.lowerBound("1"); k2 = map2.upperBound("1");
        QVERIFY(j2 == map2.find("1") && --(--k2) == j2);
        j2 = map2.lowerBound("11"); k2 = map2.upperBound("11");
        QVERIFY(j2 == map2.find("a") && k2 == j2);
        j2 = map2.lowerBound("a"); k2 = map2.upperBound("a");
        QVERIFY(j2 == map2.find("a") && k2 == map2.find("b"));
        QVERIFY(j2.value() == 1 && j2.value() == 1);
        j2 = map2.lowerBound("aa"); k2 = map2.upperBound("aa");
        QVERIFY(j2 == map2.find("b") && k2 == j2);
        QVERIFY(j2.value() == 1);
        j2 = map2.lowerBound("b"); k2 = map2.upperBound("b");
        QVERIFY(j2 == map2.find("b") && k2 == map2.end());
        QVERIFY(j2.value() == 1);
        j2 = map2.lowerBound("bb"); k2 = map2.upperBound("bb");
        QVERIFY(j2 == map2.end() && k2 == j2);
    }

    {
        QMap<int, QString> map;
        map.insert(0, "Hello");

        QMap<int, QString>::iterator it = map.begin();
        QVERIFY((*it)[0] == QChar('H'));
        QVERIFY(it->constData()[0] == QChar('H'));
        it->replace(QChar('H'), QChar('X'));
        QCOMPARE(*map.begin(), QLatin1String("Xello"));

        QMap<int, QString>::const_iterator cit = map.constBegin();
        QCOMPARE((*cit).toLower(), QLatin1String("xello"));
        QCOMPARE(cit->toUpper(), QLatin1String("XELLO"));

        cit = map.cbegin();
        QCOMPARE((*cit).toLower(), QLatin1String("xello"));
        QCOMPARE(cit->toUpper(), QLatin1String("XELLO"));
    }

    {
        QMultiMap<int, QString> map1, map2;
        map1.insert(1, "Alpha");
        map1.insert(1, "Gamma");
        map2.insert(1, "Beta");
        map2.insert(1, "Gamma");
        map2.insert(1, "Gamma");

        map1.unite(map2);
        QCOMPARE(map1.size(), 5);
        QCOMPARE(static_cast<QStringList>(map1.values()),
                (QStringList() << "Gamma" << "Gamma" << "Beta" << "Gamma" << "Alpha"));

        map2 = map1;
        map2.unite(map2);
        QCOMPARE(map2.size(), 10);
        QCOMPARE(map2.values(), map1.values() + map1.values());
    }
}

void tst_Collections::qstring()
{
    QString hello = "hello";
    QString ello = "ello";
    QString World = "World";
    QString Wor = "Wor";
    QString helloWorld = "helloWorld";

    QString s = hello + "World";
    QVERIFY(hello + World == helloWorld);
    QVERIFY(hello + "World" == helloWorld);
    QVERIFY("hello" + World == helloWorld);

    QVERIFY('h' + ello == hello);
    QVERIFY(Wor + 'l' + 'd' == "World");
    QVERIFY(hello + World == "helloWorld");
    QVERIFY(hello + "World" == "helloWorld");
    QVERIFY("hello" + World == "helloWorld");
    QVERIFY('h' + ello == "hello");
    QVERIFY(Wor + 'l' + 'd' == "World");
    QVERIFY("helloWorld" == hello + World);
    QVERIFY("helloWorld" == hello + "World");
    QVERIFY("helloWorld" == "hello" + World);
    QVERIFY("hello" == 'h' + ello);
    QVERIFY("World" == Wor + 'l' + 'd');

    QVERIFY(hello.contains('e'));
    QVERIFY (true == hello.contains('e'));
    QVERIFY (hello.contains('e') != false);

    QVERIFY(hello.indexOf('e') == 1);
    QVERIFY(hello.indexOf('e', -10) == -1);
    QVERIFY(hello.indexOf('l') == 2);
    QVERIFY(hello.indexOf('l',2) == 2);
    QVERIFY(hello.indexOf('l',3) == 3);

    QString empty;
    QCOMPARE(empty.indexOf("x"), -1);
    QCOMPARE(empty.lastIndexOf("x"), -1);
    QCOMPARE(empty.lastIndexOf("x", 0), -1);
    QCOMPARE(empty.count("x"), 0);
    QCOMPARE(empty.indexOf(""), 0);
    QCOMPARE(empty.lastIndexOf(""), 0);
    QCOMPARE(empty.lastIndexOf("", -1), -1);
    QCOMPARE(empty.count(""), 1);

    QString large = "000 100 200 300 400 500 600 700 800 900";

    QVERIFY(large.indexOf("700") == 28);
    QVERIFY(large.indexOf("700", 28) == 28);
    QVERIFY(large.indexOf("700", 29) == -1);
    QVERIFY(large.lastIndexOf("700") == 28);
    QVERIFY(large.lastIndexOf("700", 28) == 28);
    QVERIFY(large.lastIndexOf("700", 27) == -1);

    QCOMPARE(large.indexOf(""), 0);
    QCOMPARE(large.indexOf(QString()), 0);
    QCOMPARE(large.indexOf(QLatin1String()), 0);
    QCOMPARE(large.indexOf(QStringView()), 0);
    QCOMPARE(large.lastIndexOf(""), large.size());
    QCOMPARE(large.lastIndexOf("", -1), large.size() - 1);
    QCOMPARE(large.lastIndexOf(QString()), large.size());
    QCOMPARE(large.lastIndexOf(QLatin1String()), large.size());
    QCOMPARE(large.lastIndexOf(QStringView()), large.size());
    QCOMPARE(large.count(""), large.size() + 1);
    QCOMPARE(large.count(QString()), large.size() + 1);
    QCOMPARE(large.count(QLatin1String()), large.size() + 1);
    QCOMPARE(large.count(QStringView()), large.size() + 1);

    QVERIFY(large.contains("200"));
    QVERIFY(!large.contains("201"));
    QVERIFY(large.contains('3'));
    QVERIFY(!large.contains('a'));

    QVERIFY(large.count("00") == 11);
    QVERIFY(large.count('3') == 1);
    QVERIFY(large.count('0') == 21);
    QVERIFY(large.count("0") == 21);
    QVERIFY(large.count("200") == 1);
    QVERIFY(large.count("201") == 0);

    QVERIFY(hello.left(0) == "");
    QVERIFY(!hello.left(0).isNull());
    QCOMPARE(hello.left(1), QLatin1String("h"));
    QCOMPARE(hello.left(2), QLatin1String("he"));
    QCOMPARE(hello.left(200), QLatin1String("hello"));
    QVERIFY(hello.left(hello.size()) == hello);
    QVERIFY(hello.left(hello.size()+1) == hello);

    QVERIFY(hello.right(0) == "");
    QVERIFY(!hello.right(0).isNull());
    QCOMPARE(hello.right(1), QLatin1String("o"));
    QCOMPARE(hello.right(2), QLatin1String("lo"));
    QCOMPARE(hello.right(200), QLatin1String("hello"));
    QVERIFY(hello.right(hello.size()) == hello);
    QVERIFY(hello.right(hello.size()+1) == hello);

    QVERIFY(!hello.mid(0, 0).isNull());
    QVERIFY(hello.mid(0, 1) == "h");
    QVERIFY(hello.mid(0, 2) == "he");
    QVERIFY(hello.mid(0, 200) == "hello");
    QCOMPARE(hello.mid(0), QLatin1String("hello"));
    QVERIFY(hello.mid(0, hello.size()) == hello);
    QVERIFY(hello.mid(0, hello.size()+1) == hello);

    QVERIFY(hello.mid(hello.size()-0) == "");
    QVERIFY(hello.mid(hello.size()-0).isEmpty());
    QVERIFY(!hello.mid(hello.size()-0).isNull());
    QCOMPARE(hello.mid(hello.size()-1), QLatin1String("o"));
    QCOMPARE(hello.mid(hello.size()-2), QLatin1String("lo"));
    QCOMPARE(hello.mid(hello.size()-200), QLatin1String("hello"));

    QString null;
    QString nonNull = "";
    QVERIFY(null.left(10).isNull());
    QVERIFY(null.mid(0).isNull());
    QVERIFY(null.isNull());
    QVERIFY(!nonNull.isNull());

    QString fill = "123";
    fill.fill('a');
    QCOMPARE(fill, QLatin1String("aaa"));

    s.clear();
    s = hello;
    s.append(World);
    QVERIFY(s == helloWorld);
    s.clear();
    s = World;
    s.insert(0,hello);
    QVERIFY(s == helloWorld);
    s = "012345";
    s.insert(3, 'E');
    QCOMPARE(s, QLatin1String("012E345"));
    s.insert(3, "INSID");
    QCOMPARE(s, QLatin1String("012INSIDE345"));
    s = "short";
    s.insert(7, 'E');
    QCOMPARE(s, QLatin1String("short  E"));
    s = "short";
    s.insert(7, "END");
    QCOMPARE(s, QLatin1String("short  END"));

    QVERIFY(QString::fromLatin1("hello") == "hello");

    s = "first";
    QVERIFY(s.toLatin1() == "first");
    s = "second";
    QVERIFY(s.toLatin1() == "second");
    s.clear();
    QVERIFY(s.isNull());
    QVERIFY(s.toLatin1().size() == 0);
    QVERIFY(s.toLatin1().isEmpty());
    QVERIFY(s.toLatin1().isNull());

    s = "first-utf8";
    QVERIFY(s.toUtf8() == "first-utf8");
    s = "second-utf8";
    QVERIFY(s.toUtf8() == "second-utf8");
    s.clear();
    QVERIFY(s.isNull());
    QVERIFY(s.toUtf8().size() == 0);
    QVERIFY(s.toUtf8().isEmpty());
    QVERIFY(s.toUtf8().isNull());

    s = "first-utf8";
    QVERIFY(s.toUtf8() == "first-utf8");
    s = "second-utf8";
    QVERIFY(s.toUtf8() == "second-utf8");
    s.clear();
    QVERIFY(s.isNull());
    QVERIFY(s.toUtf8().size() == 0);
    QVERIFY(s.toUtf8().isEmpty());
    QVERIFY(s.toUtf8().isNull());

    s = "first-local8Bit";
    QVERIFY(s.toLocal8Bit() == "first-local8Bit");
    s = "second-local8Bit";
    QVERIFY(s.toLocal8Bit() == "second-local8Bit");
    s.clear();
    QVERIFY(s.isNull());
    QVERIFY(s.toLocal8Bit().size() == 0);
    QVERIFY(s.toLocal8Bit().isEmpty());

    s = "first-ascii";
    QVERIFY(s.toLatin1() == "first-ascii");
    s = "second-ascii";
    QVERIFY(s.toLatin1() == "second-ascii");
    s.clear();
    QVERIFY(s.isNull());
    QVERIFY(s.toLatin1().size() == 0);
    QVERIFY(s.toLatin1().isEmpty());
    QVERIFY(s.toLatin1().isNull());

    s = "ascii";
    s += QChar((uchar) 0xb0);
    QVERIFY(s.toUtf8() != s.toLatin1());
    QCOMPARE(s[s.size()-1].unicode(), char16_t(0xb0));
    QCOMPARE(s.left(s.size()-1), QLatin1String("ascii"));

    QVERIFY(s == QString::fromUtf8(s.toUtf8().constData()));

    s = "12";
    s.append('3');
    s += '4';
    QCOMPARE(s, QLatin1String("1234"));

    s = "repend";
    s.prepend('p');
    QCOMPARE(s, QLatin1String("prepend"));
    s.prepend("abc ");
    QCOMPARE(s, QLatin1String("abc prepend"));

    s = "   whitespace        ";
    QCOMPARE(s.trimmed(), QLatin1String("whitespace"));
    s = "    lots      of  stuff       ";
    QCOMPARE(s.simplified(), QLatin1String("lots of stuff"));

    s = "a hat, a stick, a ski";
    QVERIFY(s[2] == 'h');
    QVERIFY(s[1] < 'b');


    s = "12223";
    s.remove(1, 2);
    QCOMPARE(s, QLatin1String("123"));

    s = "(%1)(%2)";
    s = s.arg("foo").arg(7);
    QCOMPARE(s, QLatin1String("(foo)(7)"));

    s = "stl rocks";
    std::string stl_string = s.toStdString(); // TODO: std::string stl_string = s does not work.
    QCOMPARE(s, QLatin1String("stl rocks"));
    s = QString::fromStdString(stl_string); // TODO: s = stl_string does not work.
    QCOMPARE(s, QLatin1String("stl rocks"));

    {
        QString str("Bananas");
        QVERIFY(str.startsWith("Ban"));
        QVERIFY(false == str.startsWith("Car"));
    }
    {
        QString str("Bananas");
        QVERIFY(str.endsWith("anas"));
        QVERIFY(false == str.endsWith("pple"));
    }


    QString str = "Hello";
    QString cstr = QString::fromRawData(str.unicode(), str.size());
    QCOMPARE(str, QLatin1String("Hello"));
    QCOMPARE(cstr, QLatin1String("Hello"));
    cstr.clear();
    QCOMPARE(str, QLatin1String("Hello"));
    QVERIFY(cstr.isEmpty());

    {
        QString str1("Foo");
        str1.prepend(str1);
        QCOMPARE(str1, QString("FooFoo"));
        str1.append(str1);
        QCOMPARE(str1, QString("FooFooFooFoo"));
        str1.insert(2, str1);
        QCOMPARE(str1, QString("FoFooFooFooFoooFooFooFoo"));
        str1.replace(3, 3, str1);
        QCOMPARE(str1, QString("FoFFoFooFooFooFoooFooFooFooooFooFoooFooFooFoo"));
        str1 = "FooFoo";
        str1.replace(char('F'), str1);
        QCOMPARE(str1, QString("FooFooooFooFoooo"));
        str1 = "FooFoo";
        str1.replace(char('o'), str1);
        QCOMPARE(str1, QString("FFooFooFooFooFFooFooFooFoo"));

        str1 = "Foo";
        str1.replace("Foo", str1);
        QCOMPARE(str1, QString("Foo"));
        str1.replace(str1, str1);
        QCOMPARE(str1, QString("Foo"));

        str1 = "Foo";
        str1.replace("Foo", str1, Qt::CaseInsensitive);
        QCOMPARE(str1, QString("Foo"));
        str1.replace(str1, str1);
        QCOMPARE(str1, QString("Foo"));

        str1 = "FooFoo";
        str1.reserve(100);
        str1.replace("oo", str1);
        QCOMPARE(str1, QString("FFooFooFFooFoo"));

        str1 = "Bar";
        str1.replace("FooFoo", str1);
        QCOMPARE(str1, QString("Bar"));

        str1.replace(str1, "xxx");
        QCOMPARE(str1, QString("xxx"));
        str1.replace(str1, QString("yyy"));
        QCOMPARE(str1, QString("yyy"));
        str1 += str1;
        QCOMPARE(str1, QString("yyyyyy"));
    }
}


void tst_Collections::bitArray()
{
    QBitArray ba(20);
    QVERIFY(!ba.testBit(17));
    ba.setBit(17);
    QVERIFY(ba.size() == 20);
    QVERIFY(ba.testBit(17)==true);
    QVERIFY(!ba.testBit(16));
    ba[4] = true;
    QVERIFY(ba.testBit(4));
    QVERIFY(ba[4]);
    int sum = 0;
    for(int i = 0; i < 20; i++)
        sum += ba.testBit(i) ? 1 : 0;
    QVERIFY(sum == 2);

    ba = QBitArray(7, true);
    QVERIFY(ba.size() == 7);
    QVERIFY(ba[5]);

    ba = QBitArray(3);
    ba[0] = ba[2] = true;

    QBitArray nba(3);
    nba[1] = true;

    QVERIFY(~ba == nba);

};

struct CacheFoo
{
    CacheFoo(int v):val(v) { counter++; }
    ~CacheFoo() { counter--; }
    int val;
    static int counter;
    bool isDetached() const { return val != 2; }
};

int CacheFoo::counter = 0;

void tst_Collections::cache()
{
    {
        QCache<int, CacheFoo> cache(120);
        int i;
        for (i = 0; i < 30; i++) {
            cache.object(10);
            cache.insert(i, new CacheFoo(i), i);
        }

        QVERIFY(cache.contains(10));
        QVERIFY(!cache.contains(1));
        QVERIFY(!cache.contains(2));
        delete cache.take(10);
    }
    {
        QCache<int, QString> cache(120);
        int i;
        QString two;
        for (i = 0; i < 30; i++) {
            QString s = QString::number(i);
            cache.insert(i, new QString(s), i);
            if (i == 2)
                two = s;
        }
        QVERIFY(!cache.contains(3));
        QVERIFY(!cache.contains(2));
    }
    {
        QCache<int, int> cache(100);
        cache.insert(2, new int(2));
        *cache[2] = 3;
        QVERIFY(*cache.object(2) == 3);
    }

    QVERIFY(CacheFoo::counter == 0);

}

#if QT_CONFIG(regularexpression)
void tst_Collections::regexp()
{
    QRegularExpression rx("^\\d\\d?$");
    QVERIFY(!rx.match("123").hasMatch());
    QVERIFY(!rx.match("-6").hasMatch());
    QVERIFY(rx.match("6").hasMatch()) ;
}
#endif

void tst_Collections::pair()
{
    QPair<double, int> p;
    QVERIFY(p.first == 0.0);
    QVERIFY(p.second == 0);

    QPair<int, QString> a(1, "Zebra"), b(2, "Ying"), c(3, "Yang"), d(3, "Ying"), e(5, "Alabama");
    QVERIFY(a.first == 1);
    QCOMPARE(a.second, QLatin1String("Zebra"));
    QVERIFY(a == qMakePair(1, QString("Zebra")));

    QVERIFY(a == a && b == b && c == c && d == d && e == e);
    QVERIFY(a != b && a != c && a != d && a != e && b != c && b != d && b != e && c != d && c != e
           && d != e);

    QVERIFY(a < b && b < c && c < d && d < e);
    QVERIFY(a <= b && b <= c && c <= d && d <= e);

    QVERIFY(e > d && d > c && c > b && b > a);
    QVERIFY(e >= d && d >= c && c >= b && b >= a);

    QVERIFY(!(a > b || b > c || c > d || d > e));
    QVERIFY(!(a >= b || b >= c || c >= d || d >= e));

    QVERIFY(!(e < d || d < c || c < b || b < a));
    QVERIFY(!(e <= d || d <= c || c <= b || b <= a));

    QVERIFY(a <= a && b <= b && c <= c && d <= d && e <= e);
    QVERIFY(!(a < a || b < b || c < c || d < d || e < e));

    QVERIFY(a >= a && b >= b && c >= c && d >= d && e >= e);
    QVERIFY(!(a > a || b > b || c > c || d > d || e > e));
}

/*
    These test that Java-style mutable iterators don't trash shared
    copy (the setSharable() mechanism).
*/

template <class Container>
void populate(Container &);

template <>
void populate(QList<int> &container)
{
    container << 1 << 2 << 4 << 8;
}

template <>
void populate(QMap<int, int> &container)
{
    container.insert(1, 1);
    container.insert(2, 2);
    container.insert(4, 4);
    container.insert(8, 8);
}

template <>
void populate(QHash<int, int> &container)
{
    container.insert(1, 1);
    container.insert(2, 2);
    container.insert(4, 4);
    container.insert(8, 8);
}

template <class Container>
bool isSharable(const Container &container)
{
    Container copy = container;
    return !container.isDetached();
}

template <class Container> Container newInstance() {
    Container container;
    populate(container);
    if (!container.isEmpty())
        return container;
    return Container();
}

template <class Container, class ContainerMutableIterator>
void testContainer()
{
    /*
        Verify that shared_null's 'sharable' flag is set to true.
    */
    {
        Container c1;
        QVERIFY(!c1.isDetached());

        Container c2 = c1;
        QVERIFY(!c1.isDetached());
        QVERIFY(!c2.isDetached());
    }

    /*
        Verify that the 'sharable' flag is true in populated containers.
    */
    {
        Container c1;
        populate(c1);
        QVERIFY(c1.size() == 4);
        QVERIFY(c1.isDetached());

        Container c2 = c1;
        QVERIFY(c1.size() == 4);
        QVERIFY(c2.size() == 4);
        QVERIFY(!c1.isDetached());
        QVERIFY(!c2.isDetached());
    }

    /* test that the move operators work properly */
    {
        Container c1 = Container(newInstance<Container>());
        QVERIFY(c1.size() == 4);
        QVERIFY(c1 == newInstance<Container>());
        c1 = newInstance<Container>();
        QVERIFY(c1.size() == 4);
        QVERIFY(c1 == newInstance<Container>());
        Container c2 = std::move(c1);
        QVERIFY(c2.size() == 4);
        QVERIFY(c2 == newInstance<Container>());
    }
}

#define TEST_SEQUENTIAL_CONTAINER(Container) \
    testContainer<Q##Container<int>, QMutable##Container##Iterator<int> >() \

#define TEST_ASSOCIATIVE_CONTAINER(Container) \
    testContainer<Q##Container<int, int>, QMutable##Container##Iterator<int, int> >()

void tst_Collections::sharableQList()
{
    TEST_SEQUENTIAL_CONTAINER(List);
}

void tst_Collections::sharableQVector()
{
    TEST_SEQUENTIAL_CONTAINER(Vector);
}

void tst_Collections::sharableQMap()
{
    TEST_ASSOCIATIVE_CONTAINER(Map);
}

void tst_Collections::sharableQHash()
{
    TEST_ASSOCIATIVE_CONTAINER(Hash);
}

static int getList_calls = 0;
QList<int> getList()
{
    ++getList_calls;
    QList<int> list;
    list << 1 << 2 << 3 << 4 << 5 << 6;
    return list;
}


void tst_Collections::q_foreach()
{
    QList<int> list;
    list << -2 << -1 << 0 << 1 << 2;

    int sum = 0;
    int j = 0;
    foreach(int i, list) {
        QCOMPARE(i, list.at(j));
        sum += i;
        ++j;
    }
    QCOMPARE(sum, 0);

    // again, but without scope
    foreach(int i, list)
        sum += i;
    QCOMPARE(sum, 0);

    foreach(int i, list) {
        sum += i;
        if (i == 0)
            break;
    }
    QCOMPARE(sum, -3);

    sum = 0;
    foreach(int i, list) {
        if (i < 0)
            continue;
        sum += i;
    }
    QCOMPARE(sum, 3);

    sum = 0;
    getList_calls = 0;
    foreach(int i, getList())
        sum += i;
    QCOMPARE(sum, 21);
    QCOMPARE(getList_calls, 1);
}


void tst_Collections::conversions()
{
#define STUFF "A" << "C" << "B" << "A"

    {
        QList<QString> list1;
        list1 << STUFF;

        QVector<QString> vect1 = list1.toVector();
        QCOMPARE(list1.size(), 4);
        QVERIFY(vect1 == (QVector<QString>() << STUFF));

        QList<QString> list2 = vect1.toList();
        QCOMPARE(list2.size(), 4);
        QVERIFY(list2 == (QList<QString>() << STUFF));

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        QSet<QString> set1 = list1.toSet();
#else
        QSet<QString> set1(list1.begin(), list1.end());
#endif
        QCOMPARE(set1.size(), 3);
        QVERIFY(set1.contains("A"));
        QVERIFY(set1.contains("B"));
        QVERIFY(set1.contains("C"));
        QVERIFY(!set1.contains("D"));

        QList<QString> list3 = set1.values();
        QCOMPARE(list3.size(), 3);
        QVERIFY(list3.contains("A"));
        QVERIFY(list3.contains("B"));
        QVERIFY(list3.contains("C"));
        QVERIFY(!list3.contains("D"));

        QVERIFY(QList<int>().toVector().isEmpty());
        QVERIFY(QVector<int>().toList().isEmpty());
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        QVERIFY(QList<int>().toSet().isEmpty());
        QVERIFY(QSet<int>().toList().isEmpty());
#endif
    }

    {
        QList<QString> list1;
        list1 << STUFF;

        QVector<QString> vect1 = QVector<QString>::fromList(list1);
        QCOMPARE(list1.size(), 4);
        QVERIFY(vect1 == (QVector<QString>() << STUFF));

        QList<QString> list2 = QList<QString>::fromVector(vect1);
        QCOMPARE(list2.size(), 4);
        QVERIFY(list2 == (QList<QString>() << STUFF));

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        QSet<QString> set1 = QSet<QString>::fromList(list1);
#else
        QSet<QString> set1(list1.begin(), list1.end());
#endif
        QCOMPARE(set1.size(), 3);
        QVERIFY(set1.contains("A"));
        QVERIFY(set1.contains("B"));
        QVERIFY(set1.contains("C"));
        QVERIFY(!set1.contains("D"));

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        QList<QString> list3 = QList<QString>::fromSet(set1);
#else
        QList<QString> list3 = set1.values();
#endif
        QCOMPARE(list3.size(), 3);
        QVERIFY(list3.contains("A"));
        QVERIFY(list3.contains("B"));
        QVERIFY(list3.contains("C"));
        QVERIFY(!list3.contains("D"));

        QVERIFY(QVector<int>::fromList(QList<int>()).isEmpty());
        QVERIFY(QList<int>::fromVector(QVector<int>()).isEmpty());
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        QVERIFY(QSet<int>::fromList(QList<int>()).isEmpty());
        QVERIFY(QList<int>::fromSet(QSet<int>()).isEmpty());
#endif
    }
#undef STUFF
}

void tst_Collections::javaStyleIterators()
{
    QStringList list;
    list << "a" << "b" << "c";
    QMutableStringListIterator i(list);
    while (i.hasNext()) {
        i.next();
        i.setValue("");
    }
    while (i.hasPrevious()) {
        i.previous();
        QVERIFY(i.value().isEmpty());
        i.value() = "x";
        QCOMPARE(i.value(), QString("x"));
    }
}

template <class Container>
void testLinkedListLikeStlIterators()
{
    Container fake;
    typename Container::value_type t = {};
    fake << t;

    typename Container::iterator i1 = fake.begin(), i2 = i1 + 1;
    typename Container::const_iterator c1 = i1, c2 = c1 + 1;

    QVERIFY(i1 == i1);
    QVERIFY(i1 == c1);
    QVERIFY(c1 == i1);
    QVERIFY(c1 == c1);
    QVERIFY(i2 == i2);
    QVERIFY(i2 == c2);
    QVERIFY(c2 == i2);
    QVERIFY(c2 == c2);
    QVERIFY(1 + i1 == i1 + 1);
    QVERIFY(1 + c1 == c1 + 1);

    QVERIFY(i1 != i2);
    QVERIFY(i1 != c2);
    QVERIFY(c1 != i2);
    QVERIFY(c1 != c2);
    QVERIFY(i2 != i1);
    QVERIFY(i2 != c1);
    QVERIFY(c2 != i1);
    QVERIFY(c2 != c1);
}

template <class Container>
void testListLikeStlIterators()
{
    testLinkedListLikeStlIterators<Container>();

    Container fake;
    typename Container::value_type t = {};
    fake << t;

    typename Container::iterator i1 = fake.begin(), i2 = i1 + 1;
    typename Container::const_iterator c1 = i1, c2 = c1 + 1;

    QVERIFY(i1 < i2);
    QVERIFY(i1 < c2);
    QVERIFY(c1 < i2);
    QVERIFY(c1 < c2);
    QVERIFY(!(i2 < i1));
    QVERIFY(!(i2 < c1));
    QVERIFY(!(c2 < i1));
    QVERIFY(!(c2 < c1));
    QVERIFY(!(i1 < i1));
    QVERIFY(!(i1 < c1));
    QVERIFY(!(c1 < i1));
    QVERIFY(!(c1 < c1));
    QVERIFY(!(i2 < i2));
    QVERIFY(!(i2 < c2));
    QVERIFY(!(c2 < i2));
    QVERIFY(!(c2 < c2));

    QVERIFY(i2 > i1);
    QVERIFY(i2 > c1);
    QVERIFY(c2 > i1);
    QVERIFY(c2 > c1);
    QVERIFY(!(i1 > i2));
    QVERIFY(!(i1 > c2));
    QVERIFY(!(c1 > i2));
    QVERIFY(!(c1 > c2));
    QVERIFY(!(i1 > i1));
    QVERIFY(!(i1 > c1));
    QVERIFY(!(c1 > i1));
    QVERIFY(!(c1 > c1));
    QVERIFY(!(i2 > i2));
    QVERIFY(!(i2 > c2));
    QVERIFY(!(c2 > i2));
    QVERIFY(!(c2 > c2));

    QVERIFY(!(i1 >= i2));
    QVERIFY(!(i1 >= c2));
    QVERIFY(!(c1 >= i2));
    QVERIFY(!(c1 >= c2));
    QVERIFY(i2 >= i1);
    QVERIFY(i2 >= c1);
    QVERIFY(c2 >= i1);
    QVERIFY(c2 >= c1);
    QVERIFY(i1 >= i1);
    QVERIFY(i1 >= c1);
    QVERIFY(c1 >= i1);
    QVERIFY(c1 >= c1);
    QVERIFY(i2 >= i2);
    QVERIFY(i2 >= c2);
    QVERIFY(c2 >= i2);
    QVERIFY(c2 >= c2);

    QVERIFY(!(i2 <= i1));
    QVERIFY(!(i2 <= c1));
    QVERIFY(!(c2 <= i1));
    QVERIFY(!(c2 <= c1));
    QVERIFY(i1 <= i2);
    QVERIFY(i1 <= c2);
    QVERIFY(c1 <= i2);
    QVERIFY(c1 <= c2);
    QVERIFY(i1 <= i1);
    QVERIFY(i1 <= c1);
    QVERIFY(c1 <= i1);
    QVERIFY(c1 <= c1);
    QVERIFY(i2 <= i2);
    QVERIFY(i2 <= c2);
    QVERIFY(c2 <= i2);
    QVERIFY(c2 <= c2);
}

template <class Container>
void testMapLikeStlIterators()
{
    Container fake;
    QString k;
    QString t;
    fake.insert(k, t);

    typename Container::iterator i1 = fake.begin(), i2 = i1;
    ++i2;
    typename Container::const_iterator c1 = i1, c2 = c1;
    ++c2;

    QVERIFY(i1 == i1);
    QVERIFY(i1 == c1);
    QVERIFY(c1 == i1);
    QVERIFY(c1 == c1);
    QVERIFY(i2 == i2);
    QVERIFY(i2 == c2);
    QVERIFY(c2 == i2);
    QVERIFY(c2 == c2);

    QVERIFY(i1 != i2);
    QVERIFY(i1 != c2);
    QVERIFY(c1 != i2);
    QVERIFY(c1 != c2);
    QVERIFY(i2 != i1);
    QVERIFY(i2 != c1);
    QVERIFY(c2 != i1);
    QVERIFY(c2 != c1);
}

void tst_Collections::constAndNonConstStlIterators()
{
    testListLikeStlIterators<QList<int>>();
    testListLikeStlIterators<QStringList>();
    testMapLikeStlIterators<QMap<QString, QString>>();
    testMapLikeStlIterators<QMultiMap<QString, QString>>();
    testMapLikeStlIterators<QHash<QString, QString>>();
    testMapLikeStlIterators<QMultiHash<QString, QString>>();
}

void tst_Collections::vector_stl_data()
{
    QTest::addColumn<QStringList>("elements");

    QTest::newRow("empty") << QStringList();
    QTest::newRow("one") << (QStringList() << "Hei");
    QTest::newRow("two") << (QStringList() << "Hei" << "Hopp");
    QTest::newRow("three") << (QStringList() << "Hei" << "Hopp" << "Sann");
}

void tst_Collections::vector_stl()
{
    QFETCH(QStringList, elements);

    QList<QString> vector;
    for (int i = 0; i < elements.size(); ++i)
        vector << elements.at(i);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    std::vector<QString> stdVector = vector.toStdVector();
#else
    std::vector<QString> stdVector(vector.begin(), vector.end());
#endif
    QCOMPARE(int(stdVector.size()), elements.size());

    std::vector<QString>::const_iterator it = stdVector.begin();
    for (uint j = 0; j < stdVector.size() && it != stdVector.end(); ++j, ++it)
        QCOMPARE(*it, vector[j]);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QCOMPARE(QList<QString>::fromStdVector(stdVector), vector);
#endif
    QCOMPARE(QList<QString>(stdVector.begin(), stdVector.end()), vector);
}

void tst_Collections::list_stl_data()
{
    QTest::addColumn<QStringList>("elements");

    QTest::newRow("empty") << QStringList();
    QTest::newRow("one") << (QStringList() << "Hei");
    QTest::newRow("two") << (QStringList() << "Hei" << "Hopp");
    QTest::newRow("three") << (QStringList() << "Hei" << "Hopp" << "Sann");
}

void tst_Collections::list_stl()
{
    QFETCH(QStringList, elements);

    QList<QString> list;
    for (int i = 0; i < elements.size(); ++i)
        list << elements.at(i);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    std::list<QString> stdList = list.toStdList();
#else
    std::list<QString> stdList(list.begin(), list.end());
#endif

    QCOMPARE(int(stdList.size()), elements.size());

    std::list<QString>::const_iterator it = stdList.begin();
    for (uint j = 0; j < stdList.size() && it != stdList.end(); ++j, ++it)
        QCOMPARE(*it, list[j]);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QCOMPARE(QList<QString>::fromStdList(stdList), list);
#endif
    QCOMPARE(QList<QString>(stdList.begin(), stdList.end()), list);
}

template <typename T>
T qtInit(T * = nullptr)
{
    return T();
}

void tst_Collections::q_init()
{
    QCOMPARE(qtInit<int>(), 0);
    QCOMPARE(qtInit<double>(), 0.0);
    QCOMPARE(qtInit<QString>(), QString());
    QCOMPARE(qtInit<int *>(), static_cast<int *>(nullptr));
    QCOMPARE(qtInit<double *>(), static_cast<double *>(nullptr));
    QCOMPARE(qtInit<QString *>(), static_cast<QString *>(nullptr));
    QCOMPARE(qtInit<Pod>().i1, 0);
    QCOMPARE(qtInit<Pod>().i2, 0);
}

void tst_Collections::pointersize()
{
    QCOMPARE(int(sizeof(void *)), QT_POINTER_SIZE);
}

class LessThanComparable
{
public:
    bool operator<(const LessThanComparable &) const { return true; }
    bool operator==(const LessThanComparable &) const { return true; }
};

class EqualsComparable
{
public:
    bool operator==(const EqualsComparable &) const { return true; }
};

size_t qHash(const EqualsComparable &)
{
    return 0;
}

/*
    The following functions instatiates every member functions in the
    Qt containers that requires either operator== or operator<.
    They are ordered in a concept inheritance tree:

    Container
        MutableIterationContainer
            Sequence (QLinkedList)
                Random Access (QVector, QList, QQueue, QStack)
            Pair Associative (QHash, QMap)
        Associative (QSet)
*/
template <typename ContainerType, typename ValueType>
void instantiateContainer()
{
    const ValueType value = ValueType();
    ContainerType container;
    const ContainerType constContainer(container);

    typename ContainerType::const_iterator constIt;
    constIt = constContainer.begin();
    constIt = container.cbegin();
    container.constBegin();

    constIt = constContainer.end();
    constIt = constContainer.cend();
    container.constEnd();
    Q_UNUSED(constIt);

    container.clear();
    container.contains(value);
    container.size();
    Q_UNUSED(container.empty());
    Q_UNUSED(container.isEmpty());
    container.size();

    Q_UNUSED((container != constContainer));
    Q_UNUSED((container == constContainer));
    container = constContainer;
}

template <typename ContainerType, typename ValueType>
void instantiateMutableIterationContainer()
{
    instantiateContainer<ContainerType, ValueType>();
    ContainerType container;

    typename ContainerType::iterator it;
    it = container.begin();
    it = container.end();
    Q_UNUSED(it);
}

template <typename ContainerType, typename ValueType>
void instantiateSequence()
{
    instantiateMutableIterationContainer<ContainerType, ValueType>();

    ValueType value = ValueType();
    ContainerType container;
    container.removeAll(value);
}

template <typename ContainerType, typename ValueType>
void instantiateRandomAccess()
{
    instantiateSequence<ContainerType, ValueType>();

    ValueType value = ValueType();
    ContainerType container;
    container.indexOf(value);
    container.lastIndexOf(value);
}

template <typename ContainerType, typename ValueType>
void instantiateAssociative()
{
    instantiateContainer<ContainerType, ValueType>();

    const ValueType value = ValueType();
    ContainerType container;
    const ContainerType constContainer(container);

    container.reserve(1);
    container.capacity();
    container.squeeze();

    container.remove(value);
    container.values();

    container.unite(constContainer);
    container.intersect(constContainer);
    container.subtract(constContainer);

    Q_UNUSED((container != constContainer));
    Q_UNUSED((container == constContainer));
    container & constContainer;
    container &= constContainer;
    container &= value;
    container + constContainer;
    container += constContainer;
    container += value;
    container - constContainer;
    container -= constContainer;
    container -= value;
    container | constContainer;
    container |= constContainer;
    container |= value;
}

template <typename ContainerType, typename KeyType, typename ValueType>
void instantiatePairAssociative()
{
    instantiateMutableIterationContainer<ContainerType, KeyType>();

    const KeyType key = KeyType();
    const ValueType value = ValueType();
    ContainerType container;
    const ContainerType constContainer(container);

    auto it = container.insert(key, value);
    container.erase(it);
    container.find(key);
    container.constFind(key);
    constContainer.find(key);

    container.key(value);
    container.keys();
    constContainer.keys();
    container.remove(key);
    container.take(key);
    container.insert(constContainer);
    container.value(key);
    container.value(key, value);
    container.values();
    container[key];
    const int foo = constContainer[key];
    Q_UNUSED(foo);
}

/*
    Instantiate all Qt containers using a datatype that
    defines the minimum amount of operators.
*/
void tst_Collections::containerInstantiation()
{
    // Instantiate QHash member functions.
    typedef QHash<EqualsComparable, int> Hash;
    instantiatePairAssociative<Hash, EqualsComparable, int>();

    Hash hash;
    hash.reserve(1);
    hash.capacity();
    hash.squeeze();

    // Instantiate QMap member functions.
    typedef QMap<LessThanComparable, int> Map;
    instantiatePairAssociative<Map, LessThanComparable, int>();

    // Instantiate QSet member functions.
    typedef QSet<EqualsComparable> Set;
    instantiateAssociative<Set, EqualsComparable>();

    //Instantiate QList member functions.
    typedef QList<EqualsComparable> List;
    instantiateRandomAccess<List, EqualsComparable>();
    {
        EqualsComparable value;
        List list;
        list.removeAll(value);
    }

    //Instantiate QQueue member functions.
    typedef QQueue<EqualsComparable> Queue;
    instantiateRandomAccess<Queue, EqualsComparable>();

    //Instantiate QStack member functions.
    typedef QStack<EqualsComparable> Stack;
    instantiateRandomAccess<Stack, EqualsComparable>();
}

void tst_Collections::qtimerList()
{
    QList<double> foo;
    const int N = 10000;

    foo.append(99.9);
    foo.append(99.9);
    foo.append(99.9);

    for(int i = 0; i < N; i++) {
        foo.removeFirst();
        foo.insert(1, 99.9);
    }

    QList<double>::Iterator end = foo.end();
    for (int i = 0; i < (N / 2) - 10; ++i) {
        foo.prepend(99.9);
        if (foo.end() != end)
            return;
    }
    QFAIL("QList preallocates too much memory");
}

#define QVERIFY_TYPE(Type) QVERIFY(sizeof(Type) > 0)

template <typename Container>
void testContainerTypedefs(Container container)
{
    Q_UNUSED(container);
    { QVERIFY_TYPE(typename Container::value_type); }
    { QVERIFY_TYPE(typename Container::iterator); }
    { QVERIFY_TYPE(typename Container::const_iterator); }
    { QVERIFY_TYPE(typename Container::reference); }
    { QVERIFY_TYPE(typename Container::const_reference); }
    { QVERIFY_TYPE(typename Container::pointer); }
    { QVERIFY_TYPE(typename Container::difference_type); }
    { QVERIFY_TYPE(typename Container::size_type); }
}

template <typename Container>
void testPairAssociativeContainerTypedefs(Container container)
{
    Q_UNUSED(container);

//  TODO: Not sure how to define value_type for our associative containers
//    { QVERIFY_TYPE(typename Container::value_type); }
//    { QVERIFY_TYPE(typename Container::const_iterator); }
//    { QVERIFY_TYPE(typename Container::reference); }
//    { QVERIFY_TYPE(typename Container::const_reference); }
//    { QVERIFY_TYPE(typename Container::pointer); }

    { QVERIFY_TYPE(typename Container::difference_type); }
    { QVERIFY_TYPE(typename Container::size_type); }
    { QVERIFY_TYPE(typename Container::iterator); }
    { QVERIFY_TYPE(typename Container::key_type); }
    { QVERIFY_TYPE(typename Container::mapped_type); }
// TODO
//    { QVERIFY_TYPE(typename Container::key_compare); }
//    { QVERIFY_TYPE(typename Container::value_comare); }
}

template <typename Container>
void testSetContainerTypedefs(Container container)
{
    Q_UNUSED(container);
    { QVERIFY_TYPE(typename Container::iterator); }
    { QVERIFY_TYPE(typename Container::const_iterator); }
    { QVERIFY_TYPE(typename Container::reference); }
    { QVERIFY_TYPE(typename Container::const_reference); }
    { QVERIFY_TYPE(typename Container::pointer); }
    { QVERIFY_TYPE(typename Container::difference_type); }
    { QVERIFY_TYPE(typename Container::size_type); }
    { QVERIFY_TYPE(typename Container::key_type); }
}

/*
    Compile-time test that verifies that the Qt containers
    have STL-compatable typedefs.
*/
void tst_Collections::containerTypedefs()
{
    testContainerTypedefs(QStack<int>());
    testContainerTypedefs(QList<int>());
    testContainerTypedefs(QQueue<int>());

    testPairAssociativeContainerTypedefs(QMap<int, int>());
    testPairAssociativeContainerTypedefs(QMultiMap<int, int>());
    testPairAssociativeContainerTypedefs(QHash<int, int>());

    testSetContainerTypedefs(QSet<int>());
}

class Key1;
class T1;
class T2;

void tst_Collections::forwardDeclared()
{
#define TEST(...) do { \
        using C = __VA_ARGS__; \
        C *x = nullptr; \
        C::iterator i; \
        C::const_iterator j; \
        Q_UNUSED(x); \
        Q_UNUSED(i); \
        Q_UNUSED(j); \
    } while (false)

    TEST(QHash<Key1, T1>);
    TEST(QMap<Key1, T1>);
    TEST(QMultiMap<Key1, T1>);
    TEST(QList<T1>);
    TEST(QVector<T1>);
    TEST(QStack<T1>);
    TEST(QQueue<T1>);
    TEST(QSet<T1>);
#undef TEST

    {
        using C = QPair<T1, T2>;
        C *x = nullptr;
        Q_UNUSED(x);
    }

    {
        using C = QSet<T1>;
        C *x = nullptr;
        C::const_iterator j;
        Q_UNUSED(x);
        Q_UNUSED(j);
    }
}

#if defined(Q_PROCESSOR_ARM)
#  if defined(Q_COMPILER_ALIGNAS) && defined(__BIGGEST_ALIGNMENT__)
     // On ARM __BIGGEST_ALIGNMENT__ must be multiplied by 8 to
     // get the same limit as enforced by alignas()
#    define BIGGEST_ALIGNMENT_TO_TEST (__BIGGEST_ALIGNMENT__ << 3)
#  endif
#endif

#if !defined(BIGGEST_ALIGNMENT_TO_TEST)
#  define BIGGEST_ALIGNMENT_TO_TEST 128
#endif

template <size_t Alignment>
class alignas(Alignment) AlignedClass
{
    char i;

public:
    AlignedClass(int i = 0) : i(i) {}

    enum { PreferredAlignment = Alignment };

    inline bool operator==(const AlignedClass &other) const { return i == other.i; }
    inline bool operator<(const AlignedClass &other) const { return i < other.i; }
    friend inline size_t qHash(const AlignedClass &a) { return qHash(a.i); }
};

using Aligned4 = AlignedClass<4>;
static_assert(alignof(Aligned4) % 4 == 0);

using AlignedStdMax = AlignedClass<alignof(std::max_align_t)>;
static_assert(alignof(AlignedStdMax) % alignof(std::max_align_t) == 0);

using AlignedBiggest = AlignedClass<BIGGEST_ALIGNMENT_TO_TEST>;
static_assert(BIGGEST_ALIGNMENT_TO_TEST > alignof(std::max_align_t), "Not overly aligned");
static_assert(alignof(AlignedBiggest) % BIGGEST_ALIGNMENT_TO_TEST == 0);

template<typename C>
void testVectorAlignment()
{
    typedef typename C::value_type Aligned;
    C container;
    container.append(Aligned());
    QCOMPARE(quintptr(&container[0]) % Aligned::PreferredAlignment, quintptr(0));

    for (int i = 0; i < 200; ++i)
        container.append(Aligned());

    for (int i = 0; i < container.size(); ++i)
        QCOMPARE(quintptr(&container.at(i)) % Aligned::PreferredAlignment, quintptr(0));
}

template<typename C>
void testContiguousCacheAlignment()
{
    typedef typename C::value_type Aligned;
    C container(150);
    container.append(Aligned());
    QCOMPARE(quintptr(&container[container.firstIndex()]) % Aligned::PreferredAlignment, quintptr(0));

    for (int i = 0; i < 200; ++i)
        container.append(Aligned());

    for (int i = container.firstIndex(); i < container.lastIndex(); ++i)
        QCOMPARE(quintptr(&container.at(i)) % Aligned::PreferredAlignment, quintptr(0));
}

template<typename C>
void testAssociativeContainerAlignment()
{
    typedef typename C::key_type Key;
    typedef typename C::mapped_type Value;
    C container;
    container.insert(Key(), Value());

    typename C::const_iterator it = container.constBegin();
    QCOMPARE(quintptr(&it.key()) % Key::PreferredAlignment, quintptr(0));
    QCOMPARE(quintptr(&it.value()) % Value::PreferredAlignment, quintptr(0));

    // add some more elements
    for (int i = 0; i < 200; ++i)
        container.insert(Key(i), Value(i));

    it = container.constBegin();
    for ( ; it != container.constEnd(); ++it) {
        QCOMPARE(quintptr(&it.key()) % Key::PreferredAlignment, quintptr(0));
        QCOMPARE(quintptr(&it.value()) % Value::PreferredAlignment, quintptr(0));
    }
}

void tst_Collections::alignment()
{
    testVectorAlignment<QList<Aligned4> >();
    testVectorAlignment<QList<AlignedStdMax> >();
    testVectorAlignment<QList<AlignedBiggest> >();

    testContiguousCacheAlignment<QContiguousCache<Aligned4> >();
    testContiguousCacheAlignment<QContiguousCache<AlignedStdMax> >();
    testContiguousCacheAlignment<QContiguousCache<AlignedBiggest> >();

    // there's no guarentee that std::map supports over-aligned types
    testAssociativeContainerAlignment<QMap<Aligned4, Aligned4> >();
    testAssociativeContainerAlignment<QMap<Aligned4, AlignedStdMax> >();
    testAssociativeContainerAlignment<QMap<AlignedStdMax, Aligned4> >();
    testAssociativeContainerAlignment<QMap<AlignedStdMax, AlignedStdMax> >();

    testAssociativeContainerAlignment<QHash<Aligned4, Aligned4> >();
    testAssociativeContainerAlignment<QHash<Aligned4, AlignedStdMax> >();
    testAssociativeContainerAlignment<QHash<Aligned4, AlignedBiggest> >();
    testAssociativeContainerAlignment<QHash<AlignedStdMax, Aligned4> >();
    testAssociativeContainerAlignment<QHash<AlignedStdMax, AlignedStdMax> >();
    testAssociativeContainerAlignment<QHash<AlignedStdMax, AlignedBiggest> >();
    testAssociativeContainerAlignment<QHash<AlignedBiggest, Aligned4> >();
    testAssociativeContainerAlignment<QHash<AlignedBiggest, AlignedStdMax> >();
    testAssociativeContainerAlignment<QHash<AlignedBiggest, AlignedBiggest> >();
}

#ifndef QT_NO_TEMPLATE_TEMPLATE_PARAMETERS

template<template<class> class C>
struct QTBUG13079_Node {
    C<QTBUG13079_Node> children;
    QString s;

    ~QTBUG13079_Node() {
        children.begin(); //play with memory
    }
};
template<template<class> class C> void QTBUG13079_collectionInsideCollectionImpl()
{
    C<QTBUG13079_Node<C> > nodeList;
    nodeList << QTBUG13079_Node<C>();
    nodeList.first().s = "parent";
    nodeList.first().children << QTBUG13079_Node<C>();
    nodeList.first().children.first().s = "child";

    nodeList = nodeList.first().children;
    QCOMPARE(nodeList.first().s, QString::fromLatin1("child"));

    nodeList = nodeList.first().children;
    QCOMPARE(nodeList.size(), 0);
    nodeList << QTBUG13079_Node<C>();
}

template<template<class, class> class C>
struct QTBUG13079_NodeAssoc {
    C<int, QTBUG13079_NodeAssoc> children;
    QString s;

    ~QTBUG13079_NodeAssoc() {
        children.begin(); //play with memory
    }
};
template<template<class, class> class C> void QTBUG13079_collectionInsideCollectionAssocImpl()
{
    C<int, QTBUG13079_NodeAssoc<C> > nodeMap;
    nodeMap[18] = QTBUG13079_NodeAssoc<C>();
    nodeMap[18].s = "parent";
    nodeMap[18].children[12] = QTBUG13079_NodeAssoc<C>();
    nodeMap[18].children[12].s = "child";

    nodeMap = nodeMap[18].children;
    QCOMPARE(nodeMap[12].s, QString::fromLatin1("child"));

    nodeMap = nodeMap[12].children;
    QCOMPARE(nodeMap.size(), 0);
    nodeMap[42] = QTBUG13079_NodeAssoc<C>();
}


size_t qHash(const QTBUG13079_Node<QSet> &)
{
    return 0;
}

bool operator==(const QTBUG13079_Node<QSet> &a, const QTBUG13079_Node<QSet> &b)
{
    return a.s == b.s && a.children == b.children;
}

template<template<class> class C>
struct QTBUG13079_NodePtr : QSharedData {
    C<QTBUG13079_NodePtr> child;
    QTBUG13079_NodePtr *next;
    QString s;

    QTBUG13079_NodePtr() : next(0) {}
    ~QTBUG13079_NodePtr() {
        next = child.data(); //play with memory
    }
};
template<template<class> class C> void QTBUG13079_collectionInsidePtrImpl()
{
    typedef C<QTBUG13079_NodePtr<C> > Ptr;
    {
        Ptr nodePtr;
        nodePtr = Ptr(new QTBUG13079_NodePtr<C>());
        nodePtr->s = "parent";
        nodePtr->child = Ptr(new QTBUG13079_NodePtr<C>());
        nodePtr->child->s = "child";
        nodePtr = nodePtr->child;
        QCOMPARE(nodePtr->s, QString::fromLatin1("child"));
        nodePtr = nodePtr->child;
        QVERIFY(!nodePtr);
    }
    {
        Ptr nodePtr;
        nodePtr = Ptr(new QTBUG13079_NodePtr<C>());
        nodePtr->s = "parent";
        nodePtr->next = new QTBUG13079_NodePtr<C>();
        nodePtr->next->s = "next";
        nodePtr = Ptr(nodePtr->next);
        QCOMPARE(nodePtr->s, QString::fromLatin1("next"));
        nodePtr = Ptr(nodePtr->next);
        QVERIFY(!nodePtr);
    }
}

#endif

void tst_Collections::QTBUG13079_collectionInsideCollection()
{
#ifndef QT_NO_TEMPLATE_TEMPLATE_PARAMETERS
    QTBUG13079_collectionInsideCollectionImpl<QVector>();
    QTBUG13079_collectionInsideCollectionImpl<QStack>();
    QTBUG13079_collectionInsideCollectionImpl<QList>();
    QTBUG13079_collectionInsideCollectionImpl<QQueue>();

    {
        QSet<QTBUG13079_Node<QSet> > nodeSet;
        nodeSet << QTBUG13079_Node<QSet>();
        nodeSet = nodeSet.begin()->children;
        QCOMPARE(nodeSet.size(), 0);
    }

    QTBUG13079_collectionInsideCollectionAssocImpl<QMap>();
    QTBUG13079_collectionInsideCollectionAssocImpl<QHash>();

    QTBUG13079_collectionInsidePtrImpl<QSharedPointer>();
    QTBUG13079_collectionInsidePtrImpl<QExplicitlySharedDataPointer>();
    QTBUG13079_collectionInsidePtrImpl<QSharedDataPointer>();
#endif
}

template<class Container> void foreach_test_arrays(const Container &container)
{
    typedef typename Container::value_type T;
    int i = 0;
    QSet <T> set;
    foreach(const T & val, container) {
        QVERIFY( val == container[i] );
        set << val;
        i++;
    }
    QCOMPARE(set.size(), container.size());

    //modify the container while iterating.
    Container c2 = container;
    Container c3;
    i = 0;
    foreach (T val, c2) {
        c3 << val;
        c2.insert((i * 89) % c2.size(), T() );
        QVERIFY( val == container.at(i) );
        val = T();
        i++;
    }
    QVERIFY(c3 == container);
}


void tst_Collections::foreach_2()
{
    QStringList strlist = QString::fromLatin1("a,b,c,d,e,f,g,h,ih,kl,mn,op,qr,st,uvw,xyz").split(QLatin1Char(','));
    foreach_test_arrays(strlist);
    foreach_test_arrays(QList<QString>(strlist));
    foreach_test_arrays(strlist.toVector());

    QList<int> intlist;
    intlist << 1 << 2 << 3 << 4 <<5 << 6 << 7 << 8 << 9;
    foreach_test_arrays(intlist);
    foreach_test_arrays(intlist.toVector());

    QVarLengthArray<int> varl1;
    QVarLengthArray<int, 3> varl2;
    QVarLengthArray<int, 10> varl3;
    foreach(int i, intlist) {
        varl1 << i;
        varl2 << i;
        varl3 << i;
    }
    QCOMPARE(varl1.size(), intlist.size());
    QCOMPARE(varl2.size(), intlist.size());
    QCOMPARE(varl3.size(), intlist.size());

    QVarLengthArray<QString> varl4;
    QVarLengthArray<QString, 3> varl5;
    QVarLengthArray<QString, 18> varl6;
    foreach(const QString &str, strlist) {
        varl4 << str;
        varl5 << str;
        varl6 << str;
    }
    QCOMPARE(varl4.size(), strlist.size());
    QCOMPARE(varl5.size(), strlist.size());
    QCOMPARE(varl6.size(), strlist.size());
}

struct IntOrString
{
    int val;
    IntOrString(int v) : val(v) { }
    IntOrString(const QString &v) : val(v.toInt()) { }
    operator int() { return val; }
    operator QString() { return QString::number(val); }
    operator std::string() { return QString::number(val).toStdString(); }
    IntOrString(const std::string &v) : val(QString::fromStdString(v).toInt()) { }
};

template<class Container> void insert_remove_loop_impl()
{
    typedef typename Container::value_type T;
    Container t;
    t.append(T(IntOrString(1)));
    t << (T(IntOrString(2)));
    t += (T(IntOrString(3)));
    if constexpr (has_prepend_v<Container>)
        t.prepend(T(IntOrString(4)));
    else
        t.insert(t.cbegin(), T(IntOrString(4)));
    t.insert(2, 3 , T(IntOrString(5)));
    t.insert(4, T(IntOrString(6)));
    t.insert(t.begin() + 2, T(IntOrString(7)));
    t.insert(t.begin() + 5, 3,  T(IntOrString(8)));
    int expect1[] = { 4 , 1 , 7, 5 , 5 , 8, 8, 8, 6, 5, 2 , 3 };
    QCOMPARE(size_t(t.size()), sizeof(expect1)/sizeof(int));
    for (int i = 0; i < t.size(); i++) {
        QCOMPARE(t[i], T(IntOrString(expect1[i])));
    }

    Container compare_test1 = t;
    t.replace(5, T(IntOrString(9)));
    Container compare_test2 = t;
    QVERIFY(!(compare_test1 == t));
    QVERIFY( (compare_test1 != t));
    QVERIFY( (compare_test2 == t));
    QVERIFY(!(compare_test2 != t));
    t.remove(7);
    t.remove(2, 3);
    int expect2[] = { 4 , 1 , 9, 8, 6, 5, 2 , 3 };
    QCOMPARE(size_t(t.size()), sizeof(expect2)/sizeof(int));
    for (int i = 0; i < t.size(); i++) {
        QCOMPARE(t[i], T(IntOrString(expect2[i])));
    }

    for (typename Container::iterator it = t.begin(); it != t.end(); ) {
        if ( int(IntOrString(*it)) % 2 )
            ++it;
        else
            it = t.erase(it);
    }

    int expect3[] = { 1 , 9, 5, 3 };
    QCOMPARE(size_t(t.size()), sizeof(expect3)/sizeof(int));
    for (int i = 0; i < t.size(); i++) {
        QCOMPARE(t[i], T(IntOrString(expect3[i])));
    }

    t.erase(t.begin() + 1, t.end() - 1);

    int expect4[] = { 1 , 3 };
    QCOMPARE(size_t(t.size()), sizeof(expect4)/sizeof(int));
    for (int i = 0; i < t.size(); i++) {
        QCOMPARE(t[i], T(IntOrString(expect4[i])));
    }

    t << T(IntOrString(10)) << T(IntOrString(11)) << T(IntOrString(12)) << T(IntOrString(13));
    t << T(IntOrString(14)) << T(IntOrString(15)) << T(IntOrString(16)) << T(IntOrString(17));
    t << T(IntOrString(18)) << T(IntOrString(19)) << T(IntOrString(20)) << T(IntOrString(21));
    for (typename Container::iterator it = t.begin(); it != t.end(); ++it) {
        int iv = int(IntOrString(*it));
        if ( iv % 2 ) {
            it = t.insert(it, T(IntOrString(iv * iv)));
            it = t.insert(it + 2, T(IntOrString(iv * iv + 1)));
        }
    }

    int expect5[] = { 1, 1, 2, 3*3, 3, 3*3+1, 10, 11*11, 11, 11*11+1, 12 , 13*13, 13, 13*13+1, 14,
                      15*15, 15, 15*15+1, 16 , 17*17, 17, 17*17+1 ,18 , 19*19, 19, 19*19+1, 20, 21*21, 21, 21*21+1 };
    QCOMPARE(size_t(t.size()), sizeof(expect5)/sizeof(int));
    for (int i = 0; i < t.size(); i++) {
        QCOMPARE(t[i], T(IntOrString(expect5[i])));
    }

    t.clear();
    t << T(IntOrString(1)) << T(IntOrString(2)) << T(IntOrString(3)) << T(IntOrString(4));
    t.insert(2, 4, T(IntOrString(9)));
    t.insert(2, 4, T(IntOrString(7)));

    int expect6[] = { 1, 2, 7, 7, 7, 7, 9, 9, 9, 9, 3, 4 };
    QCOMPARE(size_t(t.size()), sizeof(expect6)/sizeof(int));
    for (int i = 0; i < t.size(); i++) {
        QCOMPARE(t[i], T(IntOrString(expect6[i])));
    }

}


template<typename T>
using ExtList = QList<T>;

void tst_Collections::insert_remove_loop()
{
    insert_remove_loop_impl<ExtList<int>>();
    insert_remove_loop_impl<ExtList<QString>>();
    insert_remove_loop_impl<QList<int>>();
    insert_remove_loop_impl<QList<QString>>();
    insert_remove_loop_impl<QVarLengthArray<int>>();
    insert_remove_loop_impl<QVarLengthArray<QString>>();
    insert_remove_loop_impl<QVarLengthArray<int, 10>>();
    insert_remove_loop_impl<QVarLengthArray<QString, 10>>();
    insert_remove_loop_impl<QVarLengthArray<int, 3>>();
    insert_remove_loop_impl<QVarLengthArray<QString, 3>>();
    insert_remove_loop_impl<QVarLengthArray<int, 15>>();
    insert_remove_loop_impl<QVarLengthArray<QString, 15>>();

    insert_remove_loop_impl<ExtList<std::string>>();
    insert_remove_loop_impl<QList<std::string>>();
    insert_remove_loop_impl<QVarLengthArray<std::string>>();
    insert_remove_loop_impl<QVarLengthArray<std::string, 10>>();
    insert_remove_loop_impl<QVarLengthArray<std::string, 3>>();
    insert_remove_loop_impl<QVarLengthArray<std::string, 15>>();
}

template <template<typename, typename> typename Container>
void tst_Collections::detachAssociativeContainerImpl()
{
    constexpr int RUNS = 50;

    for (int run = 0; run < RUNS; ++run) {
        Container<int, int> container;

        for (int i = 0; i < 1'000; ++i) {
            container.insert(i, i);
            container.insert(i, i); // for multi-keyed containers
        }

        const auto it = container.constBegin();
        const auto &key = it.key();
        const auto &value = it.value();
        const auto keyCopy = key;
        const auto valueCopy = value;

        QSemaphore sem1, sem2;
        auto detachInAnotherThread = [&sem1, &sem2, copy = container]() mutable {
            sem1.release();
            sem2.acquire();
            copy.clear(); // <==
        };

        QScopedPointer thread(QThread::create(std::move(detachInAnotherThread)));
        thread->start();

        sem2.release();
        sem1.acquire();

        // The following call may detach (because the container is
        // shared), and then use key/value to search+insert.
        //
        // This means that key/value, as references, have to be valid
        // throughout the insertion procedure. Note that they are
        // references into the container *itself*; and that the
        // insertion procedure is working on a new (detached) copy of
        // the container's payload.
        //
        // There is now a possible scenario in which the clear() above
        // finds the copy's refcount at 1, hence not perform a detach,
        // and destroy its payload. But key/value were references into
        // *that* payload (it's the payload that `container` itself
        // used to share). If inside insert() we don't take extra
        // measures to keep the payload alive, now they're dangling and
        // the insertion will malfunction.

        container.insert(key, value);

        QVERIFY(container.contains(keyCopy));
        QCOMPARE(container.value(keyCopy), valueCopy);

        thread->wait();
    }
}

QTEST_APPLESS_MAIN(tst_Collections)
#include "tst_collections.moc"
