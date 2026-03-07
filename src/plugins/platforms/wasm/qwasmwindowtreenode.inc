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
    if (previousParent) {
        previousParent->m_childStack.removeWindow(window);
        previousParent->onSubtreeChanged(QWasmWindowTreeNodeChangeType::NodeRemoval, previousParent,
                                         window);
    }

    if (currentParent) {
        currentParent->m_childStack.pushWindow(window, positionPreference);
        currentParent->onSubtreeChanged(QWasmWindowTreeNodeChangeType::NodeInsertion, currentParent,
                                        window);
    }
}

template<class Window>
Window *QWasmWindowTreeNode<Window>::asWasmWindow()
{
    return nullptr;
}

template<class Window>
void QWasmWindowTreeNode<Window>::onSubtreeChanged(QWasmWindowTreeNodeChangeType changeType,
                                           QWasmWindowTreeNode *parent, Window *child)
{
    if (changeType == QWasmWindowTreeNodeChangeType::NodeInsertion && parent == this
        && m_childStack.topWindow()
        && m_childStack.topWindow()->window()) {

        const auto flags = m_childStack.topWindow()->window()->flags();
        const bool notToolOrPopup = ((flags & Qt::ToolTip) != Qt::ToolTip) && ((flags & Qt::Popup) != Qt::Popup);
        const QVariant showWithoutActivating = m_childStack.topWindow()->window()->property("_q_showWithoutActivating");
        if (!showWithoutActivating.isValid() || !showWithoutActivating.toBool()) {
            if (notToolOrPopup)
                m_childStack.topWindow()->requestActivateWindow();
        }
    }

    if (parentNode())
        parentNode()->onSubtreeChanged(changeType, parent, child);
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
    m_activeChild = activeChild;

    auto it = m_childStack.begin();
    if (it == m_childStack.end())
        return;
    for (; it != m_childStack.end(); ++it)
        (*it)->onActivationChanged(*it == m_activeChild);

    setAsActiveNode();
}

#endif /* QWASMWINDOWTREENODE_INC */
