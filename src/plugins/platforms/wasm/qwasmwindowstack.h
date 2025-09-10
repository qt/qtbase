// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMWINDOWSTACK_H
#define QWASMWINDOWSTACK_H

#include <qglobal.h>
#include <QtCore/qlist.h>

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
class Q_AUTOTEST_EXPORT QWasmWindowStack
{
public:
    using WindowOrderChangedCallbackType = std::function<void()>;

    using StorageType = QList<QWasmWindow *>;

    using iterator = StorageType::reverse_iterator;
    using const_iterator = StorageType::const_reverse_iterator;
    using const_reverse_iterator = StorageType::const_iterator;

    enum class PositionPreference {
        StayOnBottom,
        Regular,
        StayOnTop,
    };

    explicit QWasmWindowStack(WindowOrderChangedCallbackType topWindowChangedCallback);
    ~QWasmWindowStack();

    void pushWindow(QWasmWindow *window, PositionPreference position);
    void removeWindow(QWasmWindow *window);
    void raise(QWasmWindow *window);
    void lower(QWasmWindow *window);
    void windowPositionPreferenceChanged(QWasmWindow *window, PositionPreference position);

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
    QWasmWindow *topWindow() const;

private:
    PositionPreference getWindowPositionPreference(StorageType::iterator windowIt) const;

    WindowOrderChangedCallbackType m_windowOrderChangedCallback;
    QList<QWasmWindow *> m_windowStack;
    StorageType::iterator m_regularWindowsBegin;
    StorageType::iterator m_alwaysOnTopWindowsBegin;
};

QT_END_NAMESPACE

#endif // QWASMWINDOWSTACK_H
