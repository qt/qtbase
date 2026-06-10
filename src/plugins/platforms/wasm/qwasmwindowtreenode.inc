// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMWINDOWTREENODE_INC
#define QWASMWINDOWTREENODE_INC

template<class Window>
QWasmWindowTreeNode<Window>::QWasmWindowTreeNode()
    : m_childStack(std::bind(&QWasmWindowTreeNode<Window>::onTopWindowChanged, this))
{
}

template<class Window>
QWasmWindowTreeNode<Window>::~QWasmWindowTreeNode() = default;

template<class Window>
void QWasmWindowTreeNode<Window>::onParentChanged(QWasmWindowTreeNode *previousParent,
                                          QWasmWindowTreeNode *currentParent,
                                          typename QWasmWindowStack<Window>::PositionPreference positionPreference)
{
    auto *window = asWasmWindow();
    if (previousParent)
        previousParent->m_childStack.removeWindow(window);

    if (currentParent)
        currentParent->m_childStack.pushWindow(window, positionPreference);
}

template<class Window>
Window *QWasmWindowTreeNode<Window>::asWasmWindow()
{
    return nullptr;
}

template<class Window>
void QWasmWindowTreeNode<Window>::setWindowZOrder(Window *window, int z)
{
    window->setZOrder(z);
}

template<class Window>
void QWasmWindowTreeNode<Window>::onPositionPreferenceChanged(
        typename QWasmWindowStack<Window>::PositionPreference positionPreference)
{
    if (parentNode()) {
        parentNode()->m_childStack.windowPositionPreferenceChanged(asWasmWindow(),
                                                                   positionPreference);
    }
}

template<class Window>
void QWasmWindowTreeNode<Window>::setAsActiveNode()
{
    if (parentNode())
        parentNode()->setActiveChildNode(asWasmWindow());

    // At the end, this is a recursive function
    m_activeIndex = ++s_nextActiveIndex;
}

template<class Window>
void QWasmWindowTreeNode<Window>::bringToTop()
{
    if (!parentNode())
        return;
    parentNode()->m_childStack.raise(asWasmWindow());
    parentNode()->bringToTop();
}

template<class Window>
void QWasmWindowTreeNode<Window>::sendToBottom()
{
    if (!parentNode())
        return;
    parentNode()->m_childStack.lower(asWasmWindow());
}

template<class Window>
void QWasmWindowTreeNode<Window>::onTopWindowChanged()
{
    constexpr int zOrderForElementInFrontOfScreen = 3;
    int z = zOrderForElementInFrontOfScreen;
    std::for_each(m_childStack.rbegin(), m_childStack.rend(),
                  [this, &z](Window *window) { setWindowZOrder(window, z++); });
}

template<class Window>
void QWasmWindowTreeNode<Window>::setActiveChildNode(Window *activeChild)
{
    for (auto *child : m_childStack)
        child->onActivationChanged(child == activeChild);

    setAsActiveNode();
}

#endif /* QWASMWINDOWTREENODE_INC */
