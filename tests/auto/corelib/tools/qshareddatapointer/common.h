// Copyright (C) 2026 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QSHAREDDATAPOINTER_COMMON_H
#define TST_QSHAREDDATAPOINTER_COMMON_H

#include <QTest>

#include <QtCore/QSharedData>

class Payload : public QSharedData
{
public:
    static inline int instanceCount = 0;
    int value = 0;

    Payload() { ++instanceCount; }
    explicit Payload(int v) : value(v) { ++instanceCount; }
    Payload(const Payload &other) : QSharedData(other), value(other.value) { ++instanceCount; }
    ~Payload() { --instanceCount; }

    void clear() { value = 0; }
};

// Access the payload through constData() throughout: on the implicit pointer
// operator->() would detach and defeat the sharing the test is exercising.
template <template <typename> class Ptr>
void clearPayloadTest()
{
    QCOMPARE(Payload::instanceCount, 0);

    // Unshared: the storage is reused and cleared in place.
    {
        Ptr<Payload> p(new Payload(42));
        const Payload *before = p.constData();
        QCOMPARE(Payload::instanceCount, 1);

        p.clearPayload();
        QCOMPARE(p.constData(), before);
        QCOMPARE(p.constData()->value, 0);
        QCOMPARE(p.constData()->ref.loadRelaxed(), 1);
        QCOMPARE(Payload::instanceCount, 1);
    }
    QCOMPARE(Payload::instanceCount, 0);

    // Shared: the other owner is left untouched and a fresh, singly-referenced
    // object is installed.
    {
        Ptr<Payload> p(new Payload(42));
        Ptr<Payload> other(p);
        QCOMPARE(p.constData(), other.constData());
        QCOMPARE(p.constData()->ref.loadRelaxed(), 2);
        QCOMPARE(Payload::instanceCount, 1);

        p.clearPayload();
        QCOMPARE(other.constData()->value, 42);
        QCOMPARE(other.constData()->ref.loadRelaxed(), 1);
        QVERIFY(p.constData() != other.constData());
        QCOMPARE(p.constData()->value, 0);
        QCOMPARE(p.constData()->ref.loadRelaxed(), 1);
        QCOMPARE(Payload::instanceCount, 2);

        {
            Ptr<Payload> copy(p);
            QCOMPARE(p.constData()->ref.loadRelaxed(), 2);
        }
        QCOMPARE(p.constData()->ref.loadRelaxed(), 1);
        QCOMPARE(p.constData()->value, 0);
        QCOMPARE(Payload::instanceCount, 2);
    }
    QCOMPARE(Payload::instanceCount, 0);
}

#endif // TST_QSHAREDDATAPOINTER_COMMON_H
