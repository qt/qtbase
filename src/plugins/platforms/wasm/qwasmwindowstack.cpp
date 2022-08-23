// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmwindowstack.h"

QT_BEGIN_NAMESPACE

QWasmWasmWindowStack::QWasmWasmWindowStack(TopWindowChangedCallbackType topWindowChangedCallback)
    : m_topWindowChangedCallback(std::move(topWindowChangedCallback))
{
}

QWasmWasmWindowStack::~QWasmWasmWindowStack() = default;

void QWasmWasmWindowStack::pushWindow(QWasmWindow *window)
{
    Q_ASSERT(m_windowStack.count(window) == 0);

    m_windowStack.push_back(window);

    m_topWindowChangedCallback();
}

void QWasmWasmWindowStack::removeWindow(QWasmWindow *window)
{
    Q_ASSERT(m_windowStack.count(window) == 1);

    auto it = std::find(m_windowStack.begin(), m_windowStack.end(), window);
    const bool removingBottom = m_windowStack.begin() == it;
    const bool removingTop = m_windowStack.end() - 1 == it;
    if (removingBottom)
        m_firstWindowTreatment = FirstWindowTreatment::Regular;

    m_windowStack.erase(it);

    if (removingTop)
        m_topWindowChangedCallback();
}

void QWasmWasmWindowStack::raise(QWasmWindow *window)
{
    Q_ASSERT(m_windowStack.count(window) == 1);

    if (window == rootWindow() || window == topWindow())
        return;

    auto it = std::find(regularWindowsBegin(), m_windowStack.end(), window);
    std::rotate(it, it + 1, m_windowStack.end());
    m_topWindowChangedCallback();
}

void QWasmWasmWindowStack::lower(QWasmWindow *window)
{
    Q_ASSERT(m_windowStack.count(window) == 1);

    if (window == rootWindow())
        return;

    const bool loweringTopWindow = topWindow() == window;
    auto it = std::find(regularWindowsBegin(), m_windowStack.end(), window);
    std::rotate(regularWindowsBegin(), it, it + 1);
    if (loweringTopWindow && topWindow() != window)
        m_topWindowChangedCallback();
}

QWasmWasmWindowStack::iterator QWasmWasmWindowStack::begin()
{
    return m_windowStack.rbegin();
}

QWasmWasmWindowStack::iterator QWasmWasmWindowStack::end()
{
    return m_windowStack.rend();
}

QWasmWasmWindowStack::const_iterator QWasmWasmWindowStack::begin() const
{
    return m_windowStack.rbegin();
}

QWasmWasmWindowStack::const_iterator QWasmWasmWindowStack::end() const
{
    return m_windowStack.rend();
}

QWasmWasmWindowStack::const_reverse_iterator QWasmWasmWindowStack::rbegin() const
{
    return m_windowStack.begin();
}

QWasmWasmWindowStack::const_reverse_iterator QWasmWasmWindowStack::rend() const
{
    return m_windowStack.end();
}

bool QWasmWasmWindowStack::empty() const
{
    return m_windowStack.empty();
}

size_t QWasmWasmWindowStack::size() const
{
    return m_windowStack.size();
}

QWasmWindow *QWasmWasmWindowStack::rootWindow() const
{
    return m_firstWindowTreatment == FirstWindowTreatment::AlwaysAtBottom ? m_windowStack.first()
                                                                          : nullptr;
}

QWasmWindow *QWasmWasmWindowStack::topWindow() const
{
    return m_windowStack.last();
}

QWasmWasmWindowStack::StorageType::iterator QWasmWasmWindowStack::regularWindowsBegin()
{
    return m_windowStack.begin()
            + (m_firstWindowTreatment == FirstWindowTreatment::AlwaysAtBottom ? 1 : 0);
}

QT_END_NAMESPACE
