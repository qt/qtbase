// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMWINDOWTREENODE_H
#define QWASMWINDOWTREENODE_H

#include "qwasmwindowstack.h"

namespace emscripten {
class val;
}

class QWasmWindow;

enum class QWasmWindowTreeNodeChangeType {
    NodeInsertion,
    NodeRemoval,
};

class QWasmWindowTreeNode
{
public:
    QWasmWindowTreeNode();
    virtual ~QWasmWindowTreeNode();

    virtual emscripten::val containerElement() = 0;
    virtual QWasmWindowTreeNode *parentNode() = 0;

protected:
    virtual void onParentChanged(QWasmWindowTreeNode *previous, QWasmWindowTreeNode *current,
                                 QWasmWindowStack::PositionPreference positionPreference);
    virtual QWasmWindow *asWasmWindow();
    virtual void onSubtreeChanged(QWasmWindowTreeNodeChangeType changeType,
                                  QWasmWindowTreeNode *parent, QWasmWindow *child);
    virtual void setWindowZOrder(QWasmWindow *window, int z);

    void onPositionPreferenceChanged(QWasmWindowStack::PositionPreference positionPreference);
    void setAsActiveNode();
    void bringToTop();
    void sendToBottom();
    void shutdown();

    const QWasmWindowStack &childStack() const { return m_childStack; }
    QWasmWindow *activeChild() const { return m_activeChild; }

    uint64_t getActiveIndex() const {
        return m_activeIndex;
    }

private:
    void onTopWindowChanged();
    void setActiveChildNode(QWasmWindow *activeChild);

    uint64_t m_activeIndex = 0;
    static uint64_t s_nextActiveIndex;

    QWasmWindowStack m_childStack;
    QWasmWindow *m_activeChild = nullptr;
};

#endif // QWASMWINDOWTREENODE_H
