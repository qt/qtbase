// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../../../../src/plugins/platforms/wasm/qwasmwindowstack.h"

#include <QtGui/QWindow>
#include <QTest>
#include <emscripten/val.h>

class TestWindow
{
public:
    TestWindow *transientParent() const { return m_transientParent; }
    Qt::WindowFlags windowFlags() const { return m_windowFlags; }
    bool isModal() const { return m_isModal; }

public:
    TestWindow *m_transientParent = nullptr;
    Qt::WindowFlags m_windowFlags = {};
    bool m_isModal = false;
};

#define QWasmWindowStack QWasmWindowStack<TestWindow>
#define QWasmWindow TestWindow

namespace {

QDebug operator<<(QDebug d, const QWasmWindowStack::PositionPreference &pref)
{
    switch (pref) {
    case QWasmWindowStack::PositionPreference::StayOnBottom:
        d << "StayOnBottom";
        break;
    case QWasmWindowStack::PositionPreference::Regular:
        d << "Regular";
        break;
    case QWasmWindowStack::PositionPreference::StayOnTop:
        d << "StayOnTop";
        break;
    case QWasmWindowStack::PositionPreference::StayAboveTransientParent:
        d << "StayAboveParent";
        break;
    } /* end-switch */
    return d;
}

class LogWindows
{
public:
    LogWindows(QWasmWindow *window, const QWasmWindowStack &stack)
        : m_window(window), m_stack(&stack)
    {
    }

public:
    friend QDebug operator<<(QDebug d, const LogWindows &cl)
    {
        LogWindows &l = const_cast<LogWindows &>(cl);
        d << "\n";
        for (auto it = l.m_stack->rend(); it != l.m_stack->rbegin();) {
            --it;
            d << " Window " << (*it) - l.m_window
              << l.m_stack->getWindowPositionPreference(it, false)
              << l.m_stack->getWindowPositionPreference(it, true) << "\n";
        }
        return d;
    }

private:
    QWasmWindow *m_window;
    const QWasmWindowStack *m_stack;
};

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
    void raising();
    void raisingWithAlwaysOnBottom();
    void raisingWithAlwaysOnTop();
    void lowering();
    void loweringWithAlwaysOnBottom();
    void loweringWithAlwaysOnTop();
    void removing();
    void removingWithAlwaysOnBottom();
    void removingWithAlwaysOnTop();
    void positionPreferenceChanges();
    void clearing();
    void stayAboveParentOnBottom1();
    void stayAboveParentOnBottom2();
    void stayAboveParentOnBottom3();
    void stayAboveParentRegular();
    void stayAboveParentOnTop();

private:
    void onTopWindowChanged()
    {
        ++m_topLevelChangedCallCount;
        if (m_onTopLevelChangedAction)
            m_onTopLevelChangedAction();
    }

    void verifyWindowOrderChanged(int expected = 1)
    {
        QCOMPARE(expected, m_topLevelChangedCallCount);
        clearCallbackCounter();
    }

    void clearCallbackCounter() { m_topLevelChangedCallCount = 0; }

    QWasmWindowStack::WindowOrderChangedCallbackType m_mockCallback;
    QWasmWindowStack::WindowOrderChangedCallbackType m_onTopLevelChangedAction;
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
    m_onTopLevelChangedAction = QWasmWindowStack::WindowOrderChangedCallbackType();
    clearCallbackCounter();
}

void tst_QWasmWindowStack::insertion()
{
    QWasmWindowStack stack(m_mockCallback);

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_root); };
    stack.pushWindow(&m_root, QWasmWindowStack::PositionPreference::Regular);
    verifyWindowOrderChanged();

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window1); };
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    verifyWindowOrderChanged();

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window2); };
    stack.pushWindow(&m_window2, QWasmWindowStack::PositionPreference::Regular);
    verifyWindowOrderChanged();
}

void tst_QWasmWindowStack::raising()
{
    QWasmWindowStack stack(m_mockCallback);
    stack.pushWindow(&m_root, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window4, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window5, QWasmWindowStack::PositionPreference::Regular);

    clearCallbackCounter();
    QCOMPARE(&m_window5, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window1); };
    stack.raise(&m_window1);
    verifyWindowOrderChanged();
    QCOMPARE(&m_window1, stack.topWindow());

    stack.raise(&m_window1);
    verifyWindowOrderChanged(0);
    QCOMPARE(&m_window1, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window3); };
    stack.raise(&m_window3);
    verifyWindowOrderChanged();
    QCOMPARE(&m_window3, stack.topWindow());
}

void tst_QWasmWindowStack::raisingWithAlwaysOnBottom()
{
    QWasmWindowStack stack(m_mockCallback);

    QWasmWindow alwaysOnBottomWindow1;
    QWasmWindow alwaysOnBottomWindow2;
    QWasmWindow alwaysOnBottomWindow3;

    stack.pushWindow(&alwaysOnBottomWindow1, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&alwaysOnBottomWindow2, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&alwaysOnBottomWindow3, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::Regular);
    // Window order: 3 2 1 | B3 B2 B1

    clearCallbackCounter();

    std::vector<QWasmWindow *> expectedWindowOrder = { &m_window3,
                                                       &m_window2,
                                                       &m_window1,
                                                       &alwaysOnBottomWindow3,
                                                       &alwaysOnBottomWindow2,
                                                       &alwaysOnBottomWindow1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    QCOMPARE(&m_window3, stack.topWindow());

    // Window order: 1 3 2 | B3 B2 B1
    stack.raise(&m_window1);

    expectedWindowOrder = { &m_window1,
                            &m_window3,
                            &m_window2,
                            &alwaysOnBottomWindow3,
                            &alwaysOnBottomWindow2,
                            &alwaysOnBottomWindow1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&m_window1, stack.topWindow());

    // Window order: 1 3 2 | B1 B3 B2
    stack.raise(&alwaysOnBottomWindow1);

    expectedWindowOrder = { &m_window1,
                            &m_window3,
                            &m_window2,
                            &alwaysOnBottomWindow1,
                            &alwaysOnBottomWindow3,
                            &alwaysOnBottomWindow2 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&m_window1, stack.topWindow());

    // Window order: 1 3 2 | B3 B1 B2
    stack.raise(&alwaysOnBottomWindow3);

    expectedWindowOrder = { &m_window1,
                            &m_window3,
                            &m_window2,
                            &alwaysOnBottomWindow3,
                            &alwaysOnBottomWindow1,
                            &alwaysOnBottomWindow2 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&m_window1, stack.topWindow());
}

void tst_QWasmWindowStack::raisingWithAlwaysOnTop()
{
    QWasmWindowStack stack(m_mockCallback);

    QWasmWindow alwaysOnTopWindow1;
    QWasmWindow alwaysOnTopWindow2;
    QWasmWindow alwaysOnTopWindow3;

    stack.pushWindow(&m_root, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&alwaysOnTopWindow1, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&alwaysOnTopWindow2, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(&m_window5, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&alwaysOnTopWindow3, QWasmWindowStack::PositionPreference::StayOnTop);
    // Window order: T3 T2 T1 | 5 3 1 | R

    clearCallbackCounter();

    std::vector<QWasmWindow *> expectedWindowOrder = { &alwaysOnTopWindow3,
                                                       &alwaysOnTopWindow2,
                                                       &alwaysOnTopWindow1,
                                                       &m_window5,
                                                       &m_window3,
                                                       &m_window1,
                                                       &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: T3 T2 T1 | 1 5 3 | R
    stack.raise(&m_window1);

    expectedWindowOrder = { &alwaysOnTopWindow3,
                            &alwaysOnTopWindow2,
                            &alwaysOnTopWindow1,
                            &m_window1,
                            &m_window5,
                            &m_window3,
                            &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&alwaysOnTopWindow3, stack.topWindow());

    // Window order: T3 T2 T1 3 1 5 R
    stack.raise(&m_window3);

    expectedWindowOrder = { &alwaysOnTopWindow3,
                            &alwaysOnTopWindow2,
                            &alwaysOnTopWindow1,
                            &m_window3,
                            &m_window1,
                            &m_window5,
                            &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&alwaysOnTopWindow3, stack.topWindow());

    // Window order: T1 T3 T2 3 1 5 R
    stack.raise(&alwaysOnTopWindow1);

    expectedWindowOrder = { &alwaysOnTopWindow1,
                            &alwaysOnTopWindow3,
                            &alwaysOnTopWindow2,
                            &m_window3,
                            &m_window1,
                            &m_window5,
                            &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&alwaysOnTopWindow1, stack.topWindow());
}

void tst_QWasmWindowStack::lowering()
{
    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(&m_root, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window4, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window5, QWasmWindowStack::PositionPreference::Regular);
    // Window order: 5 4 3 2 1 | R

    clearCallbackCounter();

    QCOMPARE(&m_window5, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window4); };
    stack.lower(&m_window5);

    // Window order: 4 3 2 1 5 R
    verifyWindowOrderChanged();
    QCOMPARE(&m_window4, stack.topWindow());

    stack.lower(&m_window3);
    // Window order: 4 2 1 5 3 R
    verifyWindowOrderChanged();
    std::vector<QWasmWindow *> expectedWindowOrder = { &m_window4, &m_window2, &m_window1,
                                                       &m_window5, &m_window3, &m_root };

    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
}

void tst_QWasmWindowStack::loweringWithAlwaysOnBottom()
{
    QWasmWindowStack stack(m_mockCallback);

    QWasmWindow alwaysOnBottomWindow1;
    QWasmWindow alwaysOnBottomWindow2;
    QWasmWindow alwaysOnBottomWindow3;

    stack.pushWindow(&alwaysOnBottomWindow1, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&alwaysOnBottomWindow2, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&alwaysOnBottomWindow3, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::Regular);
    // Window order: 3 2 1 | B3 B2 B1

    clearCallbackCounter();

    std::vector<QWasmWindow *> expectedWindowOrder = { &m_window3,
                                                       &m_window2,
                                                       &m_window1,
                                                       &alwaysOnBottomWindow3,
                                                       &alwaysOnBottomWindow2,
                                                       &alwaysOnBottomWindow1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    QCOMPARE(&m_window3, stack.topWindow());

    // Window order: 2 1 3 | B3 B2 B1
    stack.lower(&m_window3);

    expectedWindowOrder = { &m_window2,
                            &m_window1,
                            &m_window3,
                            &alwaysOnBottomWindow3,
                            &alwaysOnBottomWindow2,
                            &alwaysOnBottomWindow1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&m_window2, stack.topWindow());

    // Window order: 2 1 3 | B2 B1 B3
    stack.lower(&alwaysOnBottomWindow3);

    expectedWindowOrder = { &m_window2,
                            &m_window1,
                            &m_window3,
                            &alwaysOnBottomWindow2,
                            &alwaysOnBottomWindow1,
                            &alwaysOnBottomWindow3 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&m_window2, stack.topWindow());

    // Window order: 2 1 3 | B2 B3 B1
    stack.lower(&alwaysOnBottomWindow1);

    expectedWindowOrder = { &m_window2,
                            &m_window1,
                            &m_window3,
                            &alwaysOnBottomWindow2,
                            &alwaysOnBottomWindow3,
                            &alwaysOnBottomWindow1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&m_window2, stack.topWindow());
}

void tst_QWasmWindowStack::loweringWithAlwaysOnTop()
{
    QWasmWindowStack stack(m_mockCallback);

    QWasmWindow alwaysOnTopWindow1;
    QWasmWindow alwaysOnTopWindow2;

    stack.pushWindow(&m_root, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&alwaysOnTopWindow1, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&alwaysOnTopWindow2, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(&m_window5, QWasmWindowStack::PositionPreference::Regular);
    // Window order: T2 T1 5 3 1 R

    clearCallbackCounter();

    std::vector<QWasmWindow *> expectedWindowOrder = { &alwaysOnTopWindow2, &alwaysOnTopWindow1,
                                                       &m_window5,          &m_window3,
                                                       &m_window1,          &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: T1 T2 5 3 1 R
    stack.lower(&alwaysOnTopWindow2);

    expectedWindowOrder = { &alwaysOnTopWindow1, &alwaysOnTopWindow2, &m_window5,
                            &m_window3,          &m_window1,          &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&alwaysOnTopWindow1, stack.topWindow());

    // Window order: T2 T1 5 3 1 R
    stack.lower(&alwaysOnTopWindow1);

    expectedWindowOrder = { &alwaysOnTopWindow2, &alwaysOnTopWindow1, &m_window5,
                            &m_window3,          &m_window1,          &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&alwaysOnTopWindow2, stack.topWindow());

    // Window order: T2 T1 3 1 5 R
    stack.lower(&m_window5);

    expectedWindowOrder = { &alwaysOnTopWindow2, &alwaysOnTopWindow1, &m_window3,
                            &m_window1,          &m_window5,          &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&alwaysOnTopWindow2, stack.topWindow());

    // Window order: T2 T1 3 5 1 R
    stack.lower(&m_window1);

    expectedWindowOrder = { &alwaysOnTopWindow2, &alwaysOnTopWindow1, &m_window3,
                            &m_window5,          &m_window1,          &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&alwaysOnTopWindow2, stack.topWindow());
}

void tst_QWasmWindowStack::removing()
{
    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(&m_root, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window4, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window5, QWasmWindowStack::PositionPreference::Regular);
    // Window order: 5 4 3 2 1 R

    clearCallbackCounter();

    QCOMPARE(&m_window5, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_window4); };
    stack.removeWindow(&m_window5);
    // Window order: 4 3 2 1 R
    verifyWindowOrderChanged();
    QCOMPARE(&m_window4, stack.topWindow());

    stack.removeWindow(&m_window2);
    // Window order: 4 3 1 R
    verifyWindowOrderChanged();
    std::vector<QWasmWindow *> expectedWindowOrder = { &m_window4, &m_window3, &m_window1,
                                                       &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
}

void tst_QWasmWindowStack::positionPreferenceChanges()
{
    QWasmWindowStack stack(m_mockCallback);

    QWasmWindow window6;
    QWasmWindow window7;
    QWasmWindow window8;
    QWasmWindow window9;

    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window2, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window4, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window5, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&window6, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&window7, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(&window8, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(&window9, QWasmWindowStack::PositionPreference::StayOnTop);

    // Window order: 9 8 7 | 6 5 4 | 3 2 1

    clearCallbackCounter();

    std::vector<QWasmWindow *> expectedWindowOrder = { &window9,   &window8,   &window7,
                                                       &window6,   &m_window5, &m_window4,
                                                       &m_window3, &m_window2, &m_window1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: 9 8 7 1 | 6 5 4 | 3 2
    stack.windowPositionPreferenceChanged(&m_window1,
                                          QWasmWindowStack::PositionPreference::StayOnTop);

    expectedWindowOrder = {
        &window9,   &window8,   &window7,   &m_window1, &window6,
        &m_window5, &m_window4, &m_window3, &m_window2,
    };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: 9 8 7 1 5 | 6 4 | 3 2
    stack.windowPositionPreferenceChanged(&m_window5,
                                          QWasmWindowStack::PositionPreference::StayOnTop);

    expectedWindowOrder = {
        &window9, &window8,   &window7,   &m_window1, &m_window5,
        &window6, &m_window4, &m_window3, &m_window2,
    };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: 9 7 1 5 | 8 6 4 | 3 2
    stack.windowPositionPreferenceChanged(&window8, QWasmWindowStack::PositionPreference::Regular);

    expectedWindowOrder = {
        &window9, &window7,   &m_window1, &m_window5, &window8,
        &window6, &m_window4, &m_window3, &m_window2,
    };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: 9 7 1 5 | 8 6 4 2 | 3
    stack.windowPositionPreferenceChanged(&m_window2,
                                          QWasmWindowStack::PositionPreference::Regular);

    expectedWindowOrder = {
        &window9, &window7,   &m_window1, &m_window5, &window8,
        &window6, &m_window4, &m_window2, &m_window3,
    };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: 7 1 5 | 8 6 4 2 | 9 3
    stack.windowPositionPreferenceChanged(&window9,
                                          QWasmWindowStack::PositionPreference::StayOnBottom);

    expectedWindowOrder = {
        &window7,   &m_window1, &m_window5, &window8,   &window6,
        &m_window4, &m_window2, &window9,   &m_window3,
    };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: 7 1 5 | 6 4 2 | 8 9 3
    stack.windowPositionPreferenceChanged(&window8,
                                          QWasmWindowStack::PositionPreference::StayOnBottom);

    expectedWindowOrder = {
        &window7,   &m_window1, &m_window5, &window6,   &m_window4,
        &m_window2, &window8,   &window9,   &m_window3,
    };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
}

void tst_QWasmWindowStack::removingWithAlwaysOnBottom()
{
    QWasmWindowStack stack(m_mockCallback);

    QWasmWindow alwaysOnBottomWindow1;
    QWasmWindow alwaysOnBottomWindow2;
    QWasmWindow alwaysOnBottomWindow3;

    stack.pushWindow(&alwaysOnBottomWindow1, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&alwaysOnBottomWindow2, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&alwaysOnBottomWindow3, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::Regular);
    // Window order: 3 2 1 | B3 B2 B1

    clearCallbackCounter();

    std::vector<QWasmWindow *> expectedWindowOrder = { &m_window3,
                                                       &m_window2,
                                                       &m_window1,
                                                       &alwaysOnBottomWindow3,
                                                       &alwaysOnBottomWindow2,
                                                       &alwaysOnBottomWindow1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    QCOMPARE(&m_window3, stack.topWindow());

    // Window order: 3 1 | B3 B2 B1
    stack.removeWindow(&m_window2);

    expectedWindowOrder = { &m_window3, &m_window1, &alwaysOnBottomWindow3, &alwaysOnBottomWindow2,
                            &alwaysOnBottomWindow1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&m_window3, stack.topWindow());

    // Window order: 1 3 2 | B1 B3 B2
    stack.removeWindow(&alwaysOnBottomWindow2);

    expectedWindowOrder = { &m_window3, &m_window1, &alwaysOnBottomWindow3,
                            &alwaysOnBottomWindow1 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&m_window3, stack.topWindow());

    // Window order: 1 3 2 | B3 B1 B2
    stack.removeWindow(&alwaysOnBottomWindow1);

    expectedWindowOrder = { &m_window3, &m_window1, &alwaysOnBottomWindow3 };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    verifyWindowOrderChanged();
    QCOMPARE(&m_window3, stack.topWindow());
}

void tst_QWasmWindowStack::removingWithAlwaysOnTop()
{
    QWasmWindowStack stack(m_mockCallback);

    QWasmWindow alwaysOnTopWindow1;
    QWasmWindow alwaysOnTopWindow2;

    stack.pushWindow(&m_root, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&alwaysOnTopWindow1, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(&m_window3, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(&alwaysOnTopWindow2, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(&m_window5, QWasmWindowStack::PositionPreference::Regular);
    // Window order: T2 T1 5 3 1 R

    clearCallbackCounter();

    std::vector<QWasmWindow *> expectedWindowOrder = { &alwaysOnTopWindow2, &alwaysOnTopWindow1,
                                                       &m_window5,          &m_window3,
                                                       &m_window1,          &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: T2 T1 5 1 R
    stack.removeWindow(&m_window3);
    verifyWindowOrderChanged();

    expectedWindowOrder = { &alwaysOnTopWindow2, &alwaysOnTopWindow1, &m_window5, &m_window1,
                            &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: T2 5 1 R
    stack.removeWindow(&alwaysOnTopWindow1);
    verifyWindowOrderChanged();

    expectedWindowOrder = { &alwaysOnTopWindow2, &m_window5, &m_window1, &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: T2 1 R
    stack.removeWindow(&m_window5);
    verifyWindowOrderChanged();

    expectedWindowOrder = { &alwaysOnTopWindow2, &m_window1, &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: T2 R
    stack.removeWindow(&m_window1);
    verifyWindowOrderChanged();

    expectedWindowOrder = { &alwaysOnTopWindow2, &m_root };
    QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));

    // Window order: R
    stack.removeWindow(&alwaysOnTopWindow2);
    verifyWindowOrderChanged();
    QCOMPARE(&m_root, stack.topWindow());
}

void tst_QWasmWindowStack::clearing()
{
    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(&m_root, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(&m_window1, QWasmWindowStack::PositionPreference::Regular);
    // Window order: 1 R

    clearCallbackCounter();

    QCOMPARE(&m_window1, stack.topWindow());

    m_onTopLevelChangedAction = [this, &stack]() { QVERIFY(stack.topWindow() == &m_root); };
    stack.removeWindow(&m_window1);
    // Window order: R
    verifyWindowOrderChanged();
    QCOMPARE(&m_root, stack.topWindow());

    m_onTopLevelChangedAction = [&stack]() { QVERIFY(stack.topWindow() == nullptr); };
    stack.removeWindow(&m_root);
    // Window order: <empty>
    verifyWindowOrderChanged();
    QCOMPARE(nullptr, stack.topWindow());
    QCOMPARE(0u, stack.size());
}

void tst_QWasmWindowStack::stayAboveParentOnBottom1()
{
    QWasmWindow windows[5];
    windows[4].m_transientParent = &windows[0];
    windows[4].m_windowFlags = Qt::Tool;

    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(windows + 0, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(windows + 1, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(windows + 2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(windows + 3, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(windows + 4, QWasmWindowStack::PositionPreference::StayAboveTransientParent, true);

    {
        const std::vector expectedWindowOrder = {
            windows + 3,
            windows + 2,
            windows + 1,
            windows + 4,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);

        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                           getWindowsFrontToBack(&stack).begin()));
    }
    {
        // Check that window is moved to correct group:
        //   it was: StayAboveParent, in group StayOnBottom
        //   it is: StayOnTop
        stack.windowPositionPreferenceChanged(
            windows + 4,
            QWasmWindowStack::PositionPreference::StayOnTop);

        const std::vector expectedWindowOrder = {
            windows + 4,
            windows + 3,
            windows + 2,
            windows + 1,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);

        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                           getWindowsFrontToBack(&stack).begin()));
    }
}

void tst_QWasmWindowStack::stayAboveParentOnBottom2()
{
    QWasmWindow windows[5];
    windows[4].m_transientParent = &windows[0];
    windows[4].m_windowFlags = Qt::Tool;

    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(windows + 0, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(windows + 1, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(windows + 2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(windows + 3, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(windows + 4, QWasmWindowStack::PositionPreference::StayAboveTransientParent, true);

    {
        const std::vector expectedWindowOrder = {
            windows + 3,
            windows + 2,
            windows + 1,
            windows + 4,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);

        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                           getWindowsFrontToBack(&stack).begin()));
    }
    {
        // Check that order does not change:
        //   it was: StayAboveParent, in group StayOnBottom
        //   it is: StayOnBottom
        stack.windowPositionPreferenceChanged(
            windows + 4,
            QWasmWindowStack::PositionPreference::StayOnBottom);

        const std::vector expectedWindowOrder = {
            windows + 3,
            windows + 2,
            windows + 1,
            windows + 4,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);

        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                           getWindowsFrontToBack(&stack).begin()));
    }
}

void tst_QWasmWindowStack::stayAboveParentOnBottom3()
{
    QWasmWindow windows[5];
    windows[4].m_transientParent = &windows[0];
    windows[4].m_windowFlags = Qt::Tool;

    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(windows + 0, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(windows + 1, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(windows + 2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(windows + 3, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(windows + 4, QWasmWindowStack::PositionPreference::StayAboveTransientParent, true);

    {
        const std::vector expectedWindowOrder = {
            windows + 3,
            windows + 2,
            windows + 1,
            windows + 4,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);

        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                           getWindowsFrontToBack(&stack).begin()));
    }
    {
        // Check that windows is moved to correct group
        //   it was: StayAboveParent, in group StayOnBottom
        //   it is: Regular
        stack.windowPositionPreferenceChanged(
            windows + 4,
            QWasmWindowStack::PositionPreference::Regular);

        const std::vector expectedWindowOrder = {
            windows + 3,
            windows + 4,
            windows + 2,
            windows + 1,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);

        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                           getWindowsFrontToBack(&stack).begin()));
    }
}

void tst_QWasmWindowStack::stayAboveParentRegular()
{
    QWasmWindow windows[5];
    windows[4].m_transientParent = &windows[1];
    windows[4].m_windowFlags = Qt::Tool;

    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(windows + 0, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(windows + 1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(windows + 2, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(windows + 3, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(windows + 4, QWasmWindowStack::PositionPreference::StayAboveTransientParent, true);

    {
        const std::vector expectedWindowOrder = {
            windows + 3,
            windows + 2,
            windows + 4,
            windows + 1,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);

        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                           getWindowsFrontToBack(&stack).begin()));
    }
    {
        stack.windowPositionPreferenceChanged(
            windows + 4,
            QWasmWindowStack::PositionPreference::StayOnTop);

        const std::vector expectedWindowOrder = {
            windows + 4,
            windows + 3,
            windows + 2,
            windows + 1,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);

        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                           getWindowsFrontToBack(&stack).begin()));
    }
}

void tst_QWasmWindowStack::stayAboveParentOnTop()
{
    QWasmWindow windows[5];
    windows[3].m_transientParent = &windows[2];
    windows[3].m_windowFlags = Qt::Tool;

    QWasmWindowStack stack(m_mockCallback);

    stack.pushWindow(windows + 0, QWasmWindowStack::PositionPreference::StayOnBottom);
    stack.pushWindow(windows + 1, QWasmWindowStack::PositionPreference::Regular);
    stack.pushWindow(windows + 2, QWasmWindowStack::PositionPreference::StayOnTop);
    stack.pushWindow(windows + 3, QWasmWindowStack::PositionPreference::StayAboveTransientParent, true);
    stack.pushWindow(windows + 4, QWasmWindowStack::PositionPreference::StayOnTop);

    {
        const std::vector expectedWindowOrder = {
            windows + 4,
            windows + 3,
            windows + 2,
            windows + 1,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);
        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                       getWindowsFrontToBack(&stack).begin()));
    }
    {
        stack.windowPositionPreferenceChanged(
            windows + 3,
            QWasmWindowStack::PositionPreference::StayOnTop);

        const std::vector expectedWindowOrder = {
            windows + 4,
            windows + 3,
            windows + 2,
            windows + 1,
            windows + 0
        };

        qDebug() << LogWindows(windows + 0, stack);

        QVERIFY(std::equal(expectedWindowOrder.begin(), expectedWindowOrder.end(),
                           getWindowsFrontToBack(&stack).begin()));
    }
}


QTEST_MAIN(tst_QWasmWindowStack)
#include "tst_qwasmwindowstack.moc"
