/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPROMISE_H
#define QPROMISE_H

#include <QtCore/qglobal.h>
#include <QtCore/qfuture.h>

#include <utility>

QT_REQUIRE_CONFIG(future);

QT_BEGIN_NAMESPACE

template<typename T>
class QPromise
{
    static_assert (std::is_copy_constructible_v<T>
                   || std::is_move_constructible_v<T>
                   || std::is_same_v<T, void>,
                   "Type with copy or move constructors or type void is required");
public:
    QPromise() = default;
    Q_DISABLE_COPY(QPromise)
    QPromise(QPromise<T> &&other) : d(other.d)
    {
        other.d = QFutureInterface<T>();
    }
    QPromise(QFutureInterface<T> &other) : d(other) {}
    QPromise& operator=(QPromise<T> &&other)
    {
        QPromise<T> tmp(std::move(other));
        tmp.swap(*this);
        return *this;
    }
    ~QPromise()
    {
        const int state = d.loadState();
        // If QFutureInterface has no state, there is nothing to be done
        if (state == static_cast<int>(QFutureInterfaceBase::State::NoState))
            return;
        // Otherwise, if computation is not finished at this point, cancel
        // potential waits
        if (!(state & QFutureInterfaceBase::State::Finished)) {
            d.cancel();
            finish();  // required to finalize the state
        }
    }

    // Core QPromise APIs
    QFuture<T> future() const { return d.future(); }
    template<typename U, typename = QtPrivate::EnableIfSameOrConvertible<U, T>>
    bool addResult(U &&result, int index = -1)
    {
        return d.reportResult(std::forward<U>(result), index);
    }
#ifndef QT_NO_EXCEPTIONS
    void setException(const QException &e) { d.reportException(e); }
    void setException(std::exception_ptr e) { d.reportException(e); }
#endif
    void start() { d.reportStarted(); }
    void finish() { d.reportFinished(); }

    void suspendIfRequested() { d.suspendIfRequested(); }

    bool isCanceled() const { return d.isCanceled(); }

    // Progress methods
    void setProgressRange(int minimum, int maximum) { d.setProgressRange(minimum, maximum); }
    void setProgressValue(int progressValue) { d.setProgressValue(progressValue); }
    void setProgressValueAndText(int progressValue, const QString &progressText)
    {
        d.setProgressValueAndText(progressValue, progressText);
    }

    void swap(QPromise<T> &other) noexcept
    {
        qSwap(this->d, other.d);
    }

#if defined(Q_CLANG_QDOC)  // documentation-only simplified signatures
    bool addResult(const T &result, int index = -1) { }
    bool addResult(T &&result, int index = -1) { }
#endif
private:
    mutable QFutureInterface<T> d = QFutureInterface<T>();
};

template<typename T>
inline void swap(QPromise<T> &a, QPromise<T> &b) noexcept
{
    a.swap(b);
}

QT_END_NAMESPACE

#endif  // QPROMISE_H
