// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWASMWINDOWTREENODE_H
#define QWASMWINDOWTREENODE_H

#include "qwasmwindowstack.h"

#include <QVariant>

namespace emscripten {
class val;
}

class QWasmWindow;

enum class QWasmWindowTreeNodeChangeType {
    NodeInsertion,
    NodeRemoval,
};

class QWasmWindowTreeNodeBase
{
protected:
    static uint64_t s_nextActiveIndex;
};

template<class Window = QWasmWindow>
class QWasmWindowTreeNode : public QWasmWindowTreeNodeBase
{
public:
    QWasmWindowTreeNode();
    virtual ~QWasmWindowTreeNode();

    virtual emscripten::val containerElement() = 0;
    virtual QWasmWindowTreeNode *parentNode() = 0;

protected:
    virtual void onParentChanged(QWasmWindowTreeNode *previous, QWasmWindowTreeNode *current,
                                 typename QWasmWindowStack<Window>::PositionPreference positionPreference);
    virtual Window *asWasmWindow();
    virtual void onSubtreeChanged(QWasmWindowTreeNodeChangeType changeType,
                                  QWasmWindowTreeNode *parent, Window *child);
    virtual void setWindowZOrder(Window *window, int z);

    void onPositionPreferenceChanged(typename QWasmWindowStack<Window>::PositionPreference positionPreference);
    void setAsActiveNode();
    void bringToTop();
    void sendToBottom();

    const QWasmWindowStack<Window> &childStack() const { return m_childStack; }
    QWasmWindowStack<Window> &childStack() { return m_childStack; }
    Window *activeChild() const { return m_activeChild; }

    uint64_t getActiveIndex() const {
        return m_activeIndex;
    }

private:
    void onTopWindowChanged();
    void setActiveChildNode(Window *activeChild);

    uint64_t m_activeIndex = 0;

    QWasmWindowStack<Window> m_childStack;
    Window *m_activeChild = nullptr;
};
#endif

#include "qwasmwindowtreenode.inc"
