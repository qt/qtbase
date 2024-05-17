// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SLOTS_WITH_VOID_TEMPLATE_H
#define SLOTS_WITH_VOID_TEMPLATE_H
#include <QObject>

template <typename T>
struct TestTemplate
{
    T *blah;
};

class SlotsWithVoidTemplateTest : public QObject
{
    Q_OBJECT
public slots:
    inline void dummySlot() {}
    inline void dummySlot2(void) {}
    inline void anotherSlot(const TestTemplate<void> &) {}
    inline TestTemplate<void> mySlot() { return TestTemplate<void>(); }
signals:
    void mySignal(const TestTemplate<void> &);
    void myVoidSignal();
    void myVoidSignal2(void);
};
#endif // SLOTS_WITH_VOID_TEMPLATE_H
