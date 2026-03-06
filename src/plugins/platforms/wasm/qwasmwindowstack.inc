// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMWINDOWSTACK_INC
#define QWASMWINDOWSTACK_INC

template <typename Window>
QWasmWindowStack<Window>::QWasmWindowStack(
        WindowOrderChangedCallbackType windowOrderChangedCallback)
    : m_windowOrderChangedCallback(std::move(windowOrderChangedCallback)),
      m_regularWindowsBegin(m_windowStack.begin()),
      m_alwaysOnTopWindowsBegin(m_windowStack.begin())
{
    invariant();
}

template <typename Window>
QWasmWindowStack<Window>::~QWasmWindowStack()
{
    invariant();
}

template <typename Window>
void QWasmWindowStack<Window>::invariant()
{
    Q_ASSERT(m_regularWindowsBegin >= m_windowStack.begin());
    Q_ASSERT(m_regularWindowsBegin <= m_alwaysOnTopWindowsBegin);
    Q_ASSERT(m_alwaysOnTopWindowsBegin <= m_windowStack.end());
}

/* insert a window at the correct location.
 *
 * There are three groups
 *   StayOnBottom
 *   Regular
 *   StayOnTop
 *
 * In addition there is StayAboveParent which
 * can place the window in either of the
 * three above groups, depending on which
 * group the transient parent resides in.
 *
 * insertAtRegionBegin controls the placement
 * within each of the groups. Either at the end
 * or at the beginning.
 *
*/
template <typename Window>
void QWasmWindowStack<Window>::pushWindow(Window *window, PositionPreference position,
                                              bool insertAtRegionBegin, bool callCallbacks)
{
    invariant();
    Q_ASSERT(m_windowStack.count(window) == 0);

    auto regularDistance = std::distance(m_windowStack.begin(), m_regularWindowsBegin);
    auto stayOnTopDistance = std::distance(m_windowStack.begin(), m_alwaysOnTopWindowsBegin);

    if (position == PositionPreference::StayAboveTransientParent) {
        Q_ASSERT(window->transientParent());
        auto it =
                std::find(m_windowStack.begin(), m_windowStack.end(), window->transientParent());
        if (it == m_windowStack.end()) {
            qWarning() << "QWasmWindowStack<Window>::pushWindow - missing parent"
                       << window->transientParent();
            pushWindow(window, PositionPreference::Regular, insertAtRegionBegin, callCallbacks);
            return;
        } else {
            if (insertAtRegionBegin) {
                if (it >= m_alwaysOnTopWindowsBegin) {
                    ;
                } else if (it >= m_regularWindowsBegin) {
                    ++stayOnTopDistance;
                } else {
                    ++regularDistance;
                    ++stayOnTopDistance;
                }
                ++it;
            } else {
                if (it >= m_alwaysOnTopWindowsBegin) {
                    it = m_windowStack.end();
                } else if (it >= m_regularWindowsBegin) {
                    it = m_alwaysOnTopWindowsBegin;
                    ++stayOnTopDistance;
                } else {
                    it = m_regularWindowsBegin;
                    ++regularDistance;
                    ++stayOnTopDistance;
                }
            }

            m_windowStack.insert(it, window);
        }
    } else if (position == PositionPreference::StayOnTop) {
        if (insertAtRegionBegin)
            m_windowStack.insert(m_alwaysOnTopWindowsBegin, window);
        else
            m_windowStack.insert(m_windowStack.end(), window);

    } else if (position == PositionPreference::Regular) {
        ++stayOnTopDistance;
        if (insertAtRegionBegin)
            m_windowStack.insert(m_regularWindowsBegin, window);
        else
            m_windowStack.insert(m_alwaysOnTopWindowsBegin, window);

    } else {
        // StayOnBottom
        ++regularDistance;
        ++stayOnTopDistance;

        if (insertAtRegionBegin)
            m_windowStack.insert(m_windowStack.begin(), window);
        else
            m_windowStack.insert(m_regularWindowsBegin, window);
    }

    m_regularWindowsBegin = m_windowStack.begin() + regularDistance;
    m_alwaysOnTopWindowsBegin = m_windowStack.begin() + stayOnTopDistance;

    if (callCallbacks)
        m_windowOrderChangedCallback();

    Q_ASSERT(m_windowStack.count(window) == 1);
    invariant();
}

template <typename Window>
void QWasmWindowStack<Window>::removeWindow(Window *window, bool callCallbacks)
{
    invariant();
    Q_ASSERT(m_windowStack.count(window) == 1);

    auto regularDistance = std::distance(m_windowStack.begin(), m_regularWindowsBegin);
    auto stayOnTopDistance = std::distance(m_windowStack.begin(), m_alwaysOnTopWindowsBegin);

    auto it = std::find(m_windowStack.begin(), m_windowStack.end(), window);

    Q_ASSERT(it != m_windowStack.end());

    if (it < m_regularWindowsBegin)
        --regularDistance;
    if (it < m_alwaysOnTopWindowsBegin)
        --stayOnTopDistance;

    m_windowStack.erase(it);

    m_regularWindowsBegin = m_windowStack.begin() + regularDistance;
    m_alwaysOnTopWindowsBegin = m_windowStack.begin() + stayOnTopDistance;

    if (callCallbacks)
        m_windowOrderChangedCallback();

    Q_ASSERT(m_windowStack.count(window) == 0);
    invariant();
}

template <typename Window>
void QWasmWindowStack<Window>::raise(Window *window)
{
    if (raiseImpl(window))
        m_windowOrderChangedCallback();
}

template <typename Window>
bool QWasmWindowStack<Window>::raiseImpl(Window *window)
{
    invariant();
    Q_ASSERT(m_windowStack.count(window) == 1);

    {
        const auto it = std::find(m_windowStack.begin(), m_windowStack.end(), window);
        const auto itEnd = ([this, it]() {
            if (it < m_regularWindowsBegin)
                return m_regularWindowsBegin;
            if (it < m_alwaysOnTopWindowsBegin)
                return m_alwaysOnTopWindowsBegin;
            return m_windowStack.end();
        })();

        if (it + 1 == itEnd)
            return false;

        std::rotate(it, it + 1, itEnd);
    }

    std::vector<Window *> windowsToRaise;
    {
        for (auto trit = m_windowStack.begin(); trit != m_windowStack.end(); ++trit) {
            const auto w = *trit;
            if ((w != window) &&
                (getWindowPositionPreference(trit) == PositionPreference::StayAboveTransientParent) &&
                (w->transientParent() == window)) {
                windowsToRaise.push_back(w);
            }
        }
    }

    for (const auto w : windowsToRaise)
    {
        raiseImpl(w);
    }
    invariant();
    return true;
}

template <typename Window>
void QWasmWindowStack<Window>::lower(Window *window)
{
    if (lowerImpl(window))
        m_windowOrderChangedCallback();
}

template <typename Window>
bool QWasmWindowStack<Window>::lowerImpl(Window *window)
{
    invariant();
    Q_ASSERT(m_windowStack.count(window) == 1);

    {
        const auto it = std::find(m_windowStack.begin(), m_windowStack.end(), window);
        const auto itBegin = ([this, it]() {
            if (it >= m_alwaysOnTopWindowsBegin)
                return m_alwaysOnTopWindowsBegin;
            if (it >= m_regularWindowsBegin)
                return m_regularWindowsBegin;
            return m_windowStack.begin();
        })();

        if (itBegin == it)
            return false;

        std::rotate(itBegin, it, it + 1);
    }

    std::vector<Window *> windowsToLower;
    {
        for (auto trit = m_windowStack.begin(); trit != m_windowStack.end(); ++trit) {
            const auto w = *trit;
            if ((w != window) &&
                (getWindowPositionPreference(trit) == PositionPreference::StayAboveTransientParent) &&
                (w->transientParent() == window)) {
                windowsToLower.push_back(w);
            }
        }
    }

    for (const auto w : windowsToLower)
    {
        lowerImpl(w);
    }
    invariant();
    return true;
}

template <typename Window>
void QWasmWindowStack<Window>::windowPositionPreferenceChanged(Window *window,
                                                                   PositionPreference position)
{
    invariant();

    auto it = std::find(m_windowStack.begin(), m_windowStack.end(), window);
    const auto currentPosition = getWindowPositionPreference(it);

    if (position == currentPosition) {
        ;
    } else if (currentPosition == PositionPreference::StayAboveTransientParent) {
        // Keep position if possible
        const bool isStayOnBottom ( it < m_regularWindowsBegin);
        const bool isRegular( !isStayOnBottom && (it < m_alwaysOnTopWindowsBegin));
        const bool isStayOnTop(!isStayOnBottom && !isRegular);

        if (isStayOnBottom && (position == PositionPreference::StayOnBottom))
            ;
        else if (isRegular && (position == PositionPreference::Regular))
            ;
        else if (isStayOnTop && (position == PositionPreference::StayOnTop))
            ;
        else {
            auto current = *it;
            removeWindow(current, false);
            pushWindow(current, position, false, false);
            m_windowOrderChangedCallback();
        }
    } else {
        const bool insertAtRegionBegin = (
            (currentPosition != PositionPreference::StayAboveTransientParent) &&
            (position != PositionPreference::StayAboveTransientParent) &&
            ((currentPosition == PositionPreference::StayOnBottom) ||
             (position == PositionPreference::StayOnTop)));

        auto current = *it;
        removeWindow(current, false);
        pushWindow(current,  position, insertAtRegionBegin, false);
        m_windowOrderChangedCallback();
    }
    invariant();
}

template <typename Window>
typename QWasmWindowStack<Window>::iterator QWasmWindowStack<Window>::begin()
{
    return m_windowStack.rbegin();
}

template <typename Window>
typename QWasmWindowStack<Window>::iterator QWasmWindowStack<Window>::end()
{
    return m_windowStack.rend();
}

template <typename Window>
typename QWasmWindowStack<Window>::const_iterator QWasmWindowStack<Window>::begin() const
{
    return m_windowStack.rbegin();
}

template <typename Window>
typename QWasmWindowStack<Window>::const_iterator QWasmWindowStack<Window>::end() const
{
    return m_windowStack.rend();
}

template <typename Window>
typename QWasmWindowStack<Window>::const_reverse_iterator
QWasmWindowStack<Window>::rbegin() const
{
    return m_windowStack.begin();
}

template <typename Window>
typename QWasmWindowStack<Window>::const_reverse_iterator
QWasmWindowStack<Window>::rend() const
{
    return m_windowStack.end();
}

template <typename Window>
bool QWasmWindowStack<Window>::empty() const
{
    return m_windowStack.empty();
}

template <typename Window>
size_t QWasmWindowStack<Window>::size() const
{
    return m_windowStack.size();
}

template <typename Window>
Window *QWasmWindowStack<Window>::topWindow() const
{
    return m_windowStack.empty() ? nullptr : m_windowStack.last();
}

template <typename Window>
bool QWasmWindowStack<Window>::shouldBeAboveTransientParentFlags(Qt::WindowFlags flags) const
{
    if (flags.testFlag(Qt::Tool) ||
        flags.testFlag(Qt::SplashScreen) ||
        flags.testFlag(Qt::ToolTip) ||
        flags.testFlag(Qt::Popup))
    {
        return true;
    }

    return false;
}

template <typename Window>
bool QWasmWindowStack<Window>::shouldBeAboveTransientParent(const Window *window) const
{
    if (!window->transientParent())
        return false;

    if (window->isModal())
        return true;

    if (shouldBeAboveTransientParentFlags(window->windowFlags()))
        return true;

    return false;
}

template <typename Window>
typename QWasmWindowStack<Window>::PositionPreference
QWasmWindowStack<Window>::getWindowPositionPreference(
    typename StorageType::const_iterator windowIt, bool testStayAbove) const
{
    Window *window = *windowIt;
    if (testStayAbove && shouldBeAboveTransientParent(window))
        return PositionPreference::StayAboveTransientParent;
    if (windowIt >= m_alwaysOnTopWindowsBegin)
        return PositionPreference::StayOnTop;
    if (windowIt >= m_regularWindowsBegin)
        return PositionPreference::Regular;
    return PositionPreference::StayOnBottom;
}

#endif /* QWASMWINDOWSTACK_INC */
