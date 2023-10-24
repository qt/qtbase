// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmwindowstack.h"

QT_BEGIN_NAMESPACE

QWasmWindowStack::QWasmWindowStack(WindowOrderChangedCallbackType windowOrderChangedCallback)
    : m_windowOrderChangedCallback(std::move(windowOrderChangedCallback)),
      m_regularWindowsBegin(m_windowStack.begin()),
      m_alwaysOnTopWindowsBegin(m_windowStack.begin())
{
}

QWasmWindowStack::~QWasmWindowStack() = default;

void QWasmWindowStack::pushWindow(QWasmWindow *window, PositionPreference position)
{
    Q_ASSERT(m_windowStack.count(window) == 0);

    if (position == PositionPreference::StayOnTop) {
        const auto stayOnTopDistance =
                std::distance(m_windowStack.begin(), m_alwaysOnTopWindowsBegin);
        const auto regularDistance = std::distance(m_windowStack.begin(), m_regularWindowsBegin);
        m_windowStack.push_back(window);
        m_alwaysOnTopWindowsBegin = m_windowStack.begin() + stayOnTopDistance;
        m_regularWindowsBegin = m_windowStack.begin() + regularDistance;
    } else if (position == PositionPreference::Regular) {
        const auto regularDistance = std::distance(m_windowStack.begin(), m_regularWindowsBegin);
        m_alwaysOnTopWindowsBegin = m_windowStack.insert(m_alwaysOnTopWindowsBegin, window) + 1;
        m_regularWindowsBegin = m_windowStack.begin() + regularDistance;
    } else {
        const auto stayOnTopDistance =
                std::distance(m_windowStack.begin(), m_alwaysOnTopWindowsBegin);
        m_regularWindowsBegin = m_windowStack.insert(m_regularWindowsBegin, window) + 1;
        m_alwaysOnTopWindowsBegin = m_windowStack.begin() + stayOnTopDistance + 1;
    }

    m_windowOrderChangedCallback();
}

void QWasmWindowStack::removeWindow(QWasmWindow *window)
{
    Q_ASSERT(m_windowStack.count(window) == 1);

    auto it = std::find(m_windowStack.begin(), m_windowStack.end(), window);
    const auto position = getWindowPositionPreference(it);
    const auto stayOnTopDistance = std::distance(m_windowStack.begin(), m_alwaysOnTopWindowsBegin);
    const auto regularDistance = std::distance(m_windowStack.begin(), m_regularWindowsBegin);

    m_windowStack.erase(it);

    m_alwaysOnTopWindowsBegin = m_windowStack.begin() + stayOnTopDistance
            - (position != PositionPreference::StayOnTop ? 1 : 0);
    m_regularWindowsBegin = m_windowStack.begin() + regularDistance
            - (position == PositionPreference::StayOnBottom ? 1 : 0);

    m_windowOrderChangedCallback();
}

void QWasmWindowStack::raise(QWasmWindow *window)
{
    Q_ASSERT(m_windowStack.count(window) == 1);

    if (window == topWindow())
        return;

    auto it = std::find(m_windowStack.begin(), m_windowStack.end(), window);
    auto itEnd = ([this, position = getWindowPositionPreference(it)]() {
        switch (position) {
        case PositionPreference::StayOnTop:
            return m_windowStack.end();
        case PositionPreference::Regular:
            return m_alwaysOnTopWindowsBegin;
        case PositionPreference::StayOnBottom:
            return m_regularWindowsBegin;
        }
    })();
    std::rotate(it, it + 1, itEnd);
    m_windowOrderChangedCallback();
}

void QWasmWindowStack::lower(QWasmWindow *window)
{
    Q_ASSERT(m_windowStack.count(window) == 1);

    if (window == *m_windowStack.begin())
        return;

    auto it = std::find(m_windowStack.begin(), m_windowStack.end(), window);
    auto itBegin = ([this, position = getWindowPositionPreference(it)]() {
        switch (position) {
        case PositionPreference::StayOnTop:
            return m_alwaysOnTopWindowsBegin;
        case PositionPreference::Regular:
            return m_regularWindowsBegin;
        case PositionPreference::StayOnBottom:
            return m_windowStack.begin();
        }
    })();

    std::rotate(itBegin, it, it + 1);
    m_windowOrderChangedCallback();
}

void QWasmWindowStack::windowPositionPreferenceChanged(QWasmWindow *window,
                                                       PositionPreference position)
{
    auto it = std::find(m_windowStack.begin(), m_windowStack.end(), window);
    const auto currentPosition = getWindowPositionPreference(it);

    const auto zones = static_cast<int>(position) - static_cast<int>(currentPosition);
    Q_ASSERT(zones != 0);

    if (zones < 0) {
        // Perform right rotation so that the window lands on top of regular windows
        const auto begin = std::make_reverse_iterator(it + 1);
        const auto end = position == PositionPreference::Regular
                ? std::make_reverse_iterator(m_alwaysOnTopWindowsBegin)
                : std::make_reverse_iterator(m_regularWindowsBegin);
        std::rotate(begin, begin + 1, end);
        if (zones == -2) {
            ++m_alwaysOnTopWindowsBegin;
            ++m_regularWindowsBegin;
        } else if (position == PositionPreference::Regular) {
            ++m_alwaysOnTopWindowsBegin;
        } else {
            ++m_regularWindowsBegin;
        }
    } else {
        // Perform left rotation so that the window lands at the bottom of always on top windows
        const auto begin = it;
        const auto end = position == PositionPreference::Regular ? m_regularWindowsBegin
                                                                 : m_alwaysOnTopWindowsBegin;
        std::rotate(begin, begin + 1, end);
        if (zones == 2) {
            --m_alwaysOnTopWindowsBegin;
            --m_regularWindowsBegin;
        } else if (position == PositionPreference::Regular) {
            --m_regularWindowsBegin;
        } else {
            --m_alwaysOnTopWindowsBegin;
        }
    }
    m_windowOrderChangedCallback();
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

QWasmWindow *QWasmWindowStack::topWindow() const
{
    return m_windowStack.empty() ? nullptr : m_windowStack.last();
}

QWasmWindowStack::PositionPreference
QWasmWindowStack::getWindowPositionPreference(StorageType::iterator windowIt) const
{
    if (windowIt >= m_alwaysOnTopWindowsBegin)
        return PositionPreference::StayOnTop;
    if (windowIt >= m_regularWindowsBegin)
        return PositionPreference::Regular;
    return PositionPreference::StayOnBottom;
}

QT_END_NAMESPACE
