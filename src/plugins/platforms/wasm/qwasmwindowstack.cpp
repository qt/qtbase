// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmwindowstack.h"

QT_BEGIN_NAMESPACE

QWasmWindowStack::QWasmWindowStack(TopWindowChangedCallbackType topWindowChangedCallback)
    : m_topWindowChangedCallback(std::move(topWindowChangedCallback))
{
}

QWasmWindowStack::~QWasmWindowStack() = default;

void QWasmWindowStack::pushWindow(QWasmWindow *window)
{
    Q_ASSERT(m_windowStack.count(window) == 0);

    m_windowStack.push_back(window);

    m_topWindowChangedCallback();
}

void QWasmWindowStack::removeWindow(QWasmWindow *window)
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

void QWasmWindowStack::raise(QWasmWindow *window)
{
    Q_ASSERT(m_windowStack.count(window) == 1);

    if (window == rootWindow() || window == topWindow())
        return;

    auto it = std::find(regularWindowsBegin(), m_windowStack.end(), window);
    std::rotate(it, it + 1, m_windowStack.end());
    m_topWindowChangedCallback();
}

void QWasmWindowStack::lower(QWasmWindow *window)
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

QWasmWindowStack::iterator QWasmWindowStack::begin()
{
    return m_windowStack.rbegin();
}

QWasmWindowStack::iterator QWasmWindowStack::end()
{
    return m_windowStack.rend();
}

QWasmWindowStack::const_iterator QWasmWindowStack::begin() const
{
    return m_windowStack.rbegin();
}

QWasmWindowStack::const_iterator QWasmWindowStack::end() const
{
    return m_windowStack.rend();
}

QWasmWindowStack::const_reverse_iterator QWasmWindowStack::rbegin() const
{
    return m_windowStack.begin();
}

QWasmWindowStack::const_reverse_iterator QWasmWindowStack::rend() const
{
    return m_windowStack.end();
}

bool QWasmWindowStack::empty() const
{
    return m_windowStack.empty();
}

size_t QWasmWindowStack::size() const
{
    return m_windowStack.size();
}

QWasmWindow *QWasmWindowStack::rootWindow() const
{
    return m_firstWindowTreatment == FirstWindowTreatment::AlwaysAtBottom ? m_windowStack.first()
                                                                          : nullptr;
}

QWasmWindow *QWasmWindowStack::topWindow() const
{
    return m_windowStack.empty() ? nullptr : m_windowStack.last();
}

QWasmWindowStack::StorageType::iterator QWasmWindowStack::regularWindowsBegin()
{
    return m_windowStack.begin()
            + (m_firstWindowTreatment == FirstWindowTreatment::AlwaysAtBottom ? 1 : 0);
}

QT_END_NAMESPACE
