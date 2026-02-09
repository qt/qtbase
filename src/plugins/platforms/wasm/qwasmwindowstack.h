// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWASMWINDOWSTACK_H
#define QWASMWINDOWSTACK_H

#include <qglobal.h>
#include <QtCore/qlist.h>
#include <QDebug>

#include <vector>

QT_BEGIN_NAMESPACE

class QWasmWindow;

// Maintains a z-order hierarchy for a set of windows. The first added window is always treated as
// the 'root', which always stays at the bottom. Other windows are 'regular', which means they are
// subject to z-order changes via |raise| and |lower|/
// If the root is ever removed, all of the current and future windows in the stack are treated as
// regular.
// Access to the top element is facilitated by |topWindow|.
// Changes to the top element are signaled via the |topWindowChangedCallback| supplied at
// construction.

// Requirement Window
//
// type Window {
//    Window *transientParent() const;
//    Qt::WindowFlags windowFlags() const;
//    bool isModal() const;
// };

template <typename Window=QWasmWindow>
class Q_AUTOTEST_EXPORT QWasmWindowStack
{
private:
    QWasmWindowStack(const QWasmWindowStack &) = delete;
    QWasmWindowStack(QWasmWindowStack &&) = delete;

    QWasmWindowStack &operator=(const QWasmWindowStack &) = delete;
    QWasmWindowStack &&operator=(QWasmWindowStack &&) = delete;

public:
    enum class PositionPreference {
        StayOnBottom,
        Regular,
        StayOnTop,
        StayAboveTransientParent // Parent is transientParent()
    };

    using WindowOrderChangedCallbackType = std::function<void()>;
    using StorageType = QList<Window *>;

    using iterator = typename StorageType::reverse_iterator;
    using const_iterator = typename StorageType::const_reverse_iterator;
    using const_reverse_iterator = typename StorageType::const_iterator;

    explicit QWasmWindowStack(WindowOrderChangedCallbackType topWindowChangedCallback);
    ~QWasmWindowStack();

    void pushWindow(Window *window, PositionPreference position, bool insertAtRegionBegin = false,
                    bool callCallbacks = true);
    void removeWindow(Window *window, bool callCallbacks = true);
    void raise(Window *window);
    void lower(Window *window);
    void windowPositionPreferenceChanged(Window *window, PositionPreference position);

    // Iterates top-to-bottom
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    // Iterates bottom-to-top
    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;

    bool empty() const;
    size_t size() const;
    Window *topWindow() const;

    PositionPreference getWindowPositionPreference(typename StorageType::const_iterator windowIt,
                                                   bool testStayAbove = true) const;
private:
    bool raiseImpl(Window *window);
    bool lowerImpl(Window *window);
    bool shouldBeAboveTransientParent(const Window *window) const;
    bool shouldBeAboveTransientParentFlags(Qt::WindowFlags flags) const;
    void invariant();
    WindowOrderChangedCallbackType m_windowOrderChangedCallback;

    StorageType m_windowStack;
    typename StorageType::iterator m_regularWindowsBegin;
    typename StorageType::iterator m_alwaysOnTopWindowsBegin;
};

#include "qwasmwindowstack.inc"

QT_END_NAMESPACE

#endif // QWASMWINDOWSTACK_H
