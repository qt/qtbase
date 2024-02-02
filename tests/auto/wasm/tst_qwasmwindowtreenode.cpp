// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../../../src/plugins/platforms/wasm/qwasmwindowtreenode.h"
#include <QtGui/QWindow>
#include <QTest>
#include <emscripten/val.h>

class QWasmWindow
{
};

using OnSubtreeChangedCallback = std::function<void(
        QWasmWindowTreeNodeChangeType changeType, QWasmWindowTreeNode *parent, QWasmWindow *child)>;
using SetWindowZOrderCallback = std::function<void(QWasmWindow *window, int z)>;

struct OnSubtreeChangedCallData
{
    QWasmWindowTreeNodeChangeType changeType;
    QWasmWindowTreeNode *parent;
    QWasmWindow *child;
};

struct SetWindowZOrderCallData
{
    QWasmWindow *window;
    int z;
};

class TestWindowTreeNode final : public QWasmWindowTreeNode, public QWasmWindow
{
public:
    TestWindowTreeNode(OnSubtreeChangedCallback onSubtreeChangedCallback,
                       SetWindowZOrderCallback setWindowZOrderCallback)
        : m_onSubtreeChangedCallback(std::move(onSubtreeChangedCallback)),
          m_setWindowZOrderCallback(std::move(setWindowZOrderCallback))
    {
    }
    ~TestWindowTreeNode() final { }

    void setParent(TestWindowTreeNode *parent)
    {
        auto *previous = m_parent;
        m_parent = parent;
        onParentChanged(previous, parent, QWasmWindowStack::PositionPreference::Regular);
    }

    void setContainerElement(emscripten::val container) { m_containerElement = container; }

    void bringToTop() { QWasmWindowTreeNode::bringToTop(); }

    void sendToBottom() { QWasmWindowTreeNode::sendToBottom(); }

    const QWasmWindowStack &childStack() { return QWasmWindowTreeNode::childStack(); }

    emscripten::val containerElement() final { return m_containerElement; }

    QWasmWindowTreeNode *parentNode() final { return m_parent; }

    QWasmWindow *asWasmWindow() final { return this; }

protected:
    void onSubtreeChanged(QWasmWindowTreeNodeChangeType changeType, QWasmWindowTreeNode *parent,
                          QWasmWindow *child) final
    {
        m_onSubtreeChangedCallback(changeType, parent, child);
    }

    void setWindowZOrder(QWasmWindow *window, int z) final { m_setWindowZOrderCallback(window, z); }

    TestWindowTreeNode *m_parent = nullptr;
    emscripten::val m_containerElement = emscripten::val::undefined();

    OnSubtreeChangedCallback m_onSubtreeChangedCallback;
    SetWindowZOrderCallback m_setWindowZOrderCallback;
};

class tst_QWasmWindowTreeNode : public QObject
{
    Q_OBJECT

public:
    tst_QWasmWindowTreeNode() { }

private slots:
    void init();

    void nestedWindowStacks();
    void settingChildWindowZOrder();
};

void tst_QWasmWindowTreeNode::init() { }

bool operator==(const OnSubtreeChangedCallData &lhs, const OnSubtreeChangedCallData &rhs)
{
    return lhs.changeType == rhs.changeType && lhs.parent == rhs.parent && lhs.child == rhs.child;
}

bool operator==(const SetWindowZOrderCallData &lhs, const SetWindowZOrderCallData &rhs)
{
    return lhs.window == rhs.window && lhs.z == rhs.z;
}

void tst_QWasmWindowTreeNode::nestedWindowStacks()
{
    QList<OnSubtreeChangedCallData> calls;
    OnSubtreeChangedCallback mockOnSubtreeChanged =
            [&calls](QWasmWindowTreeNodeChangeType changeType, QWasmWindowTreeNode *parent,
                     QWasmWindow *child) {
                calls.push_back(OnSubtreeChangedCallData{ changeType, parent, child });
            };
    SetWindowZOrderCallback ignoreSetWindowZOrder = [](QWasmWindow *, int) {};
    TestWindowTreeNode node(mockOnSubtreeChanged, ignoreSetWindowZOrder);
    node.bringToTop();

    OnSubtreeChangedCallback ignoreSubtreeChanged = [](QWasmWindowTreeNodeChangeType,
                                                       QWasmWindowTreeNode *, QWasmWindow *) {};
    TestWindowTreeNode node2(ignoreSubtreeChanged, ignoreSetWindowZOrder);
    node2.setParent(&node);

    QCOMPARE(node.childStack().size(), 1u);
    QCOMPARE(node2.childStack().size(), 0u);
    QCOMPARE(node.childStack().topWindow(), &node2);
    QCOMPARE(calls.size(), 1u);
    {
        OnSubtreeChangedCallData expected{ QWasmWindowTreeNodeChangeType::NodeInsertion, &node,
                                           &node2 };
        QCOMPARE(calls[0], expected);
        calls.clear();
    }

    TestWindowTreeNode node3(ignoreSubtreeChanged, ignoreSetWindowZOrder);
    node3.setParent(&node);

    QCOMPARE(node.childStack().size(), 2u);
    QCOMPARE(node2.childStack().size(), 0u);
    QCOMPARE(node3.childStack().size(), 0u);
    QCOMPARE(node.childStack().topWindow(), &node3);
    {
        OnSubtreeChangedCallData expected{ QWasmWindowTreeNodeChangeType::NodeInsertion, &node,
                                           &node3 };
        QCOMPARE(calls[0], expected);
        calls.clear();
    }

    TestWindowTreeNode node4(ignoreSubtreeChanged, ignoreSetWindowZOrder);
    node4.setParent(&node);

    QCOMPARE(node.childStack().size(), 3u);
    QCOMPARE(node2.childStack().size(), 0u);
    QCOMPARE(node3.childStack().size(), 0u);
    QCOMPARE(node4.childStack().size(), 0u);
    QCOMPARE(node.childStack().topWindow(), &node4);
    {
        OnSubtreeChangedCallData expected{ QWasmWindowTreeNodeChangeType::NodeInsertion, &node,
                                           &node4 };
        QCOMPARE(calls[0], expected);
        calls.clear();
    }

    node3.bringToTop();
    QCOMPARE(node.childStack().topWindow(), &node3);

    node4.setParent(nullptr);
    QCOMPARE(node.childStack().size(), 2u);
    QCOMPARE(node.childStack().topWindow(), &node3);
    {
        OnSubtreeChangedCallData expected{ QWasmWindowTreeNodeChangeType::NodeRemoval, &node,
                                           &node4 };
        QCOMPARE(calls[0], expected);
        calls.clear();
    }

    node2.setParent(nullptr);
    QCOMPARE(node.childStack().size(), 1u);
    QCOMPARE(node.childStack().topWindow(), &node3);
    {
        OnSubtreeChangedCallData expected{ QWasmWindowTreeNodeChangeType::NodeRemoval, &node,
                                           &node2 };
        QCOMPARE(calls[0], expected);
        calls.clear();
    }

    node3.setParent(nullptr);
    QVERIFY(node.childStack().empty());
    QCOMPARE(node.childStack().topWindow(), nullptr);
    {
        OnSubtreeChangedCallData expected{ QWasmWindowTreeNodeChangeType::NodeRemoval, &node,
                                           &node3 };
        QCOMPARE(calls[0], expected);
        calls.clear();
    }
}

void tst_QWasmWindowTreeNode::settingChildWindowZOrder()
{
    QList<SetWindowZOrderCallData> calls;
    OnSubtreeChangedCallback ignoreSubtreeChanged = [](QWasmWindowTreeNodeChangeType,
                                                       QWasmWindowTreeNode *, QWasmWindow *) {};
    SetWindowZOrderCallback onSetWindowZOrder = [&calls](QWasmWindow *window, int z) {
        calls.push_back(SetWindowZOrderCallData{ window, z });
    };
    SetWindowZOrderCallback ignoreSetWindowZOrder = [](QWasmWindow *, int) {};
    TestWindowTreeNode node(ignoreSubtreeChanged, onSetWindowZOrder);

    TestWindowTreeNode node2(ignoreSubtreeChanged, ignoreSetWindowZOrder);
    node2.setParent(&node);

    {
        QCOMPARE(calls.size(), 1u);
        SetWindowZOrderCallData expected{ &node2, 3 };
        QCOMPARE(calls[0], expected);
        calls.clear();
    }

    TestWindowTreeNode node3(ignoreSubtreeChanged, ignoreSetWindowZOrder);
    node3.setParent(&node);

    {
        QCOMPARE(calls.size(), 2u);
        SetWindowZOrderCallData expected{ &node2, 3 };
        QCOMPARE(calls[0], expected);
        expected = SetWindowZOrderCallData{ &node3, 4 };
        QCOMPARE(calls[1], expected);
        calls.clear();
    }

    TestWindowTreeNode node4(ignoreSubtreeChanged, ignoreSetWindowZOrder);
    node4.setParent(&node);

    {
        QCOMPARE(calls.size(), 3u);
        SetWindowZOrderCallData expected{ &node2, 3 };
        QCOMPARE(calls[0], expected);
        expected = SetWindowZOrderCallData{ &node3, 4 };
        QCOMPARE(calls[1], expected);
        expected = SetWindowZOrderCallData{ &node4, 5 };
        QCOMPARE(calls[2], expected);
        calls.clear();
    }

    node2.bringToTop();

    {
        QCOMPARE(calls.size(), 3u);
        SetWindowZOrderCallData expected{ &node3, 3 };
        QCOMPARE(calls[0], expected);
        expected = SetWindowZOrderCallData{ &node4, 4 };
        QCOMPARE(calls[1], expected);
        expected = SetWindowZOrderCallData{ &node2, 5 };
        QCOMPARE(calls[2], expected);
        calls.clear();
    }
}

QTEST_MAIN(tst_QWasmWindowTreeNode)
#include "tst_qwasmwindowtreenode.moc"
