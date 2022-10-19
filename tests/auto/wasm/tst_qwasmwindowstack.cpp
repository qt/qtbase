// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../../../src/plugins/platforms/wasm/qwasmwindowstack.h"
#include <QtGui/QWindow>
#include <QTest>
#include <emscripten/val.h>

class QWasmWindow
{
};

namespace {
std::vector<QWasmWindow *> getWindowsFrontToBack(const QWasmWindowStack *stack)
{
    return std::vector<QWasmWindow *>(stack->begin(), stack->end());
}
}

class tst_QWasmWindowStack : public QObject
{
    Q_OBJECT

public:
    tst_QWasmWindowStack()
        : m_mockCallback(std::bind(&tst_QWasmWindowStack::onTopWindowChanged, this))
    {
    }

private slots:
    void init();

    void insertion();
    void raisingTheRootIsImpossible();
    void raising();
    void lowering();
    void removing();
    void removingTheRoot();
    void clearing();

private:
    void onTopWindowChanged()
    {
        ++m_topLevelChangedCallCount;
        if (m_onTopLevelChangedAction)
            m_onTopLevelChangedAction();
    }

    void verifyTopWindowChangedCalled(int expected = 1)
    {
        QCOMPARE(expected, m_topLevelChangedCallCount);
        clearCallbackCounter();
    }

    void clearCallbackCounter() { m_topLevelChangedCallCount = 0; }

    QWasmWindowStack::TopWindowChangedCallbackType m_mockCallback;
    QWasmWindowStack::TopWindowChangedCallbackType m_onTopLevelChangedAction;
    int m_topLevelChangedCallCount = 0;

    QWasmWindow m_root;
    QWasmWindow m_window1;
    QWasmWindow m_window2;
    QWasmWindow m_window3;
    QWasmWindow m_window4;
    QWasmWindow m_window5;
};

void tst_QWasmWindowStack::init()
{
    m_onTopLevelChangedAction = QWasmWindowStack::TopWindowChangedCallbackType();
    clearCallbackCounter();
}

void tst_QWasmWindowStack::insertion()
{
    QWasmWindowStack stack(m_mockCallback);

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_root); };
    stack.pushWindow(&m_root);
    verifyTopWindowChangedCalled();

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window1); };
    stack.pushWindow(&m_window1);
    verifyTopWindowChangedCalled();

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window2); };
    stack.pushWindow(&m_window2);
    verifyTopWindowChangedCalled();
}

void tst_QWasmWindowStack::raisingTheRootIsImpossible()
{
    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(&m_root);
    stack.pushWindow(&m_window1);
    stack.pushWindow(&m_window2);
    stack.pushWindow(&m_window3);
    stack.pushWindow(&m_window4);
    stack.pushWindow(&m_window5);

    clearCallbackCounter();

    stack.raise(&m_root);
    verifyTopWindowChangedCalled(0);

    QCOMPARE(&m_window5, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window2); };
    stack.raise(&m_window2);
    verifyTopWindowChangedCalled();
}

void tst_QWasmWindowStack::raising()
{
    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(&m_root);
    stack.pushWindow(&m_window1);
    stack.pushWindow(&m_window2);
    stack.pushWindow(&m_window3);
    stack.pushWindow(&m_window4);
    stack.pushWindow(&m_window5);

    clearCallbackCounter();

    QCOMPARE(&m_window5, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window1); };
    stack.raise(&m_window1);
    verifyTopWindowChangedCalled();
    QCOMPARE(&m_window1, stack.topWindow());

    stack.raise(&m_window1);
    verifyTopWindowChangedCalled(0);
    QCOMPARE(&m_window1, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window3); };
    stack.raise(&m_window3);
    verifyTopWindowChangedCalled();
    QCOMPARE(&m_window3, stack.topWindow());
}

void tst_QWasmWindowStack::lowering()
{
    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(&m_root);
    stack.pushWindow(&m_window1);
    stack.pushWindow(&m_window2);
    stack.pushWindow(&m_window3);
    stack.pushWindow(&m_window4);
    stack.pushWindow(&m_window5);
    // Window order: 5 4 3 2 1 R

    clearCallbackCounter();

    QCOMPARE(&m_window5, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window4); };
    stack.lower(&m_window5);
    // Window order: 4 3 2 1 5 R
    verifyTopWindowChangedCalled();
    QCOMPARE(&m_window4, stack.topWindow());

    stack.lower(&m_window3);
    // Window order: 4 2 1 5 3 R
    verifyTopWindowChangedCalled(0);
    std::vector<QWasmWindow *> expectedWindowOrder = { &m_window4, &m_window2, &m_window1,
                                                       &m_window5, &m_window3, &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
}

void tst_QWasmWindowStack::removing()
{
    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(&m_root);
    stack.pushWindow(&m_window1);
    stack.pushWindow(&m_window2);
    stack.pushWindow(&m_window3);
    stack.pushWindow(&m_window4);
    stack.pushWindow(&m_window5);
    // Window order: 5 4 3 2 1 R

    clearCallbackCounter();

    QCOMPARE(&m_window5, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window4); };
    stack.removeWindow(&m_window5);
    // Window order: 4 3 2 1 R
    verifyTopWindowChangedCalled();
    QCOMPARE(&m_window4, stack.topWindow());

    stack.removeWindow(&m_window2);
    // Window order: 4 3 1 R
    verifyTopWindowChangedCalled(0);
    std::vector<QWasmWindow *> expectedWindowOrder = { &m_window4, &m_window3, &m_window1,
                                                       &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
}

void tst_QWasmWindowStack::removingTheRoot()
{
    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(&m_root);
    stack.pushWindow(&m_window1);
    stack.pushWindow(&m_window2);
    stack.pushWindow(&m_window3);
    // Window order: 3 2 1 R

    clearCallbackCounter();

    QCOMPARE(&m_window3, stack.topWindow());

    stack.removeWindow(&m_root);
    // Window order: 3 2 1
    verifyTopWindowChangedCalled(0);
    QCOMPARE(&m_window3, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window1); };
    // Check that the new bottom window is not treated specially as a root
    stack.raise(&m_window1);
    // Window order: 1 3 2
    verifyTopWindowChangedCalled();
    std::vector<QWasmWindow *> expectedWindowOrder = { &m_window1, &m_window3, &m_window2 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window3); };
    // Check that the new bottom window is not treated specially as a root
    stack.lower(&m_window1);
    // Window order: 3 2 1
    verifyTopWindowChangedCalled();
    expectedWindowOrder = { &m_window3, &m_window2, &m_window1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
}

void tst_QWasmWindowStack::clearing()
{
    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(&m_root);
    stack.pushWindow(&m_window1);
    // Window order: 1 R

    clearCallbackCounter();

    QCOMPARE(&m_window1, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_root); };
    stack.removeWindow(&m_window1);
    // Window order: R
    verifyTopWindowChangedCalled();
    QCOMPARE(&m_root, stack.topWindow());

    m_onTopLevelChangedAction = [&stack]() { QVERIFY(stack.topWindow() == nullptr); };
    stack.removeWindow(&m_root);
    // Window order: <empty>
    verifyTopWindowChangedCalled();
    QCOMPARE(nullptr, stack.topWindow());
    QCOMPARE(0u, stack.size());
}

QTEST_MAIN(tst_QWasmWindowStack)
#include "tst_qwasmwindowstack.moc"
