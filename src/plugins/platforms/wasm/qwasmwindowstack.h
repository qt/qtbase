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
Q_AUTOTEST_EXPORT class QWasmWindowStack
{
public:
    using TopWindowChangedCallbackType = std::function<void()>;

    using StorageType = QList<QWasmWindow *>;

    using iterator = StorageType::reverse_iterator;
    using const_iterator = StorageType::const_reverse_iterator;
    using const_reverse_iterator = StorageType::const_iterator;

    explicit QWasmWindowStack(TopWindowChangedCallbackType topWindowChangedCallback);
    ~QWasmWindowStack();

    void pushWindow(QWasmWindow *window);
    void removeWindow(QWasmWindow *window);
    void raise(QWasmWindow *window);
    void lower(QWasmWindow *window);

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
    enum class FirstWindowTreatment { AlwaysAtBottom, Regular };

    QWasmWindow *rootWindow() const;
    StorageType::iterator regularWindowsBegin();

    TopWindowChangedCallbackType m_topWindowChangedCallback;
    QList<QWasmWindow *> m_windowStack;
    FirstWindowTreatment m_firstWindowTreatment = FirstWindowTreatment::AlwaysAtBottom;
};

QT_END_NAMESPACE

#endif // QWASMWINDOWSTACK_H
