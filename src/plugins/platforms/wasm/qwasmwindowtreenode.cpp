// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmwindowtreenode.h"

#include "qwasmwindow.h"

QWasmWindowTreeNode::QWasmWindowTreeNode()
    : m_childStack(std::bind(&QWasmWindowTreeNode::onTopWindowChanged, this))
{
}

QWasmWindowTreeNode::~QWasmWindowTreeNode() = default;

void QWasmWindowTreeNode::onParentChanged(QWasmWindowTreeNode *previousParent,
                                          QWasmWindowTreeNode *currentParent,
                                          QWasmWindowStack::PositionPreference positionPreference)
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

QWasmWindow *QWasmWindowTreeNode::asWasmWindow()
{
    return nullptr;
}

void QWasmWindowTreeNode::onSubtreeChanged(QWasmWindowTreeNodeChangeType changeType,
                                           QWasmWindowTreeNode *parent, QWasmWindow *child)
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

void QWasmWindowTreeNode::setWindowZOrder(QWasmWindow *window, int z)
{
    window->setZOrder(z);
}

void QWasmWindowTreeNode::onPositionPreferenceChanged(
        QWasmWindowStack::PositionPreference positionPreference)
{
    if (parentNode()) {
        parentNode()->m_childStack.windowPositionPreferenceChanged(asWasmWindow(),
                                                                   positionPreference);
    }
}

void QWasmWindowTreeNode::setAsActiveNode()
{
    if (parentNode())
        parentNode()->setActiveChildNode(asWasmWindow());
}

void QWasmWindowTreeNode::bringToTop()
{
    if (!parentNode())
        return;
    parentNode()->m_childStack.raise(asWasmWindow());
    parentNode()->bringToTop();
}

void QWasmWindowTreeNode::sendToBottom()
{
    if (!parentNode())
        return;
    m_childStack.lower(asWasmWindow());
}

void QWasmWindowTreeNode::onTopWindowChanged()
{
    constexpr int zOrderForElementInFrontOfScreen = 3;
    int z = zOrderForElementInFrontOfScreen;
    std::for_each(m_childStack.rbegin(), m_childStack.rend(),
                  [this, &z](QWasmWindow *window) { setWindowZOrder(window, z++); });
}

void QWasmWindowTreeNode::setActiveChildNode(QWasmWindow *activeChild)
{
    m_activeChild = activeChild;

    auto it = m_childStack.begin();
    if (it == m_childStack.end())
        return;
    for (; it != m_childStack.end(); ++it)
        (*it)->onActivationChanged(*it == m_activeChild);

    setAsActiveNode();
}
