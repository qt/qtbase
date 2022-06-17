/****************************************************************************
**
** Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QObject>
#include <QScopeGuard>
#include <QSharedPointer>
#include <QTest>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#if __has_include(<boost/shared_ptr.hpp>)
#  include <boost/shared_ptr.hpp>
#  include <boost/make_shared.hpp>

#  ifdef BOOST_NO_EXCEPTIONS
// https://stackoverflow.com/a/9530546/134841
// https://www.boost.org/doc/libs/1_79_0/libs/throw_exception/doc/html/throw_exception.html#throw_exception
BOOST_NORETURN void boost::throw_exception(const std::exception &) { std::terminate(); }
#    if BOOST_VERSION >= 107300
// https://www.boost.org/doc/libs/1_79_0/libs/throw_exception/doc/html/throw_exception.html#changes_in_1_73_0
BOOST_NORETURN void boost::throw_exception(const std::exception &, const boost::source_location &)
{ std::terminate(); }
#    endif // Boost v1.73
#  endif // BOOST_NO_EXCEPTIONS

#  define ONLY_IF_BOOST(x) x
#else
#  define ONLY_IF_BOOST(x) QSKIP("This benchmark requires Boost.SharedPtr.")
#endif

class tst_QSharedPointer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void refAndDeref_null_QSP_int() { refAndDeref<QSharedPointer<int>>(); }
    void refAndDeref_null_SSP_int() { refAndDeref<std::shared_ptr<int>>(); }
    void refAndDeref_null_BSP_int() { ONLY_IF_BOOST(refAndDeref<boost::shared_ptr<int>>()); }

    void refAndDeref_null_QSP_QString() { refAndDeref<QSharedPointer<QString>>(); }
    void refAndDeref_null_SSP_QString() { refAndDeref<std::shared_ptr<QString>>(); }
    void refAndDeref_null_BSP_QString() { ONLY_IF_BOOST(refAndDeref<boost::shared_ptr<QString>>()); }

    void refAndDeref_nonnull_QSP_int() { refAndDeref(QSharedPointer<int>::create(42)); }
    void refAndDeref_nonnull_SSP_int() { refAndDeref(std::make_shared<int>(42)); }
    void refAndDeref_nonnull_BSP_int() { ONLY_IF_BOOST(refAndDeref(boost::make_shared<int>(42))); }

    void refAndDeref_nonnull_QSP_QString() { refAndDeref(QSharedPointer<QString>::create(QStringLiteral("Hello"))); }
    void refAndDeref_nonnull_SSP_QString() { refAndDeref(std::make_shared<QString>(QStringLiteral("Hello"))); }
    void refAndDeref_nonnull_BSP_QString() { ONLY_IF_BOOST(refAndDeref(boost::make_shared<QString>(QStringLiteral("Hello")))); }

private:
    template <typename SP>
    void refAndDeref(SP sp = {})
    {
        QBENCHMARK {
            [[maybe_unused]] auto copy = sp;
        }
    }

private Q_SLOTS:
    void threadedRefAndDeref_null_QSP_int() { threadedRefAndDeref<QSharedPointer<int>>(); }
    void threadedRefAndDeref_null_SSP_int() { threadedRefAndDeref<std::shared_ptr<int>>(); }
    void threadedRefAndDeref_null_BSP_int() { ONLY_IF_BOOST(threadedRefAndDeref<boost::shared_ptr<int>>()); }

    void threadedRefAndDeref_null_QSP_QString() { threadedRefAndDeref<QSharedPointer<QString>>(); }
    void threadedRefAndDeref_null_SSP_QString() { threadedRefAndDeref<std::shared_ptr<QString>>(); }
    void threadedRefAndDeref_null_BSP_QString() { ONLY_IF_BOOST(threadedRefAndDeref<boost::shared_ptr<QString>>()); }

    void threadedRefAndDeref_nonnull_QSP_int() { threadedRefAndDeref(QSharedPointer<int>::create(42)); }
    void threadedRefAndDeref_nonnull_SSP_int() { threadedRefAndDeref(std::make_shared<int>(42)); }
    void threadedRefAndDeref_nonnull_BSP_int() { ONLY_IF_BOOST(threadedRefAndDeref(boost::make_shared<int>(42))); }

    void threadedRefAndDeref_nonnull_QSP_QString() { threadedRefAndDeref(QSharedPointer<QString>::create(QStringLiteral("Hello"))); }
    void threadedRefAndDeref_nonnull_SSP_QString() { threadedRefAndDeref(std::make_shared<QString>(QStringLiteral("Hello"))); }
    void threadedRefAndDeref_nonnull_BSP_QString() { ONLY_IF_BOOST(threadedRefAndDeref(boost::make_shared<QString>(QStringLiteral("Hello")))); }

private:
    template <typename SP>
    void threadedRefAndDeref(SP sp = {})
    {
        std::atomic<bool> cancel = false;
        std::vector<std::thread> threads;
        const auto numCores = std::max(2U, std::thread::hardware_concurrency());
        for (uint i = 0; i < numCores - 1; ++i) {
            threads.emplace_back([sp, &cancel] {
                    while (!cancel.load(std::memory_order_relaxed)) {
                        for (int i = 0; i < 100; ++i)
                            [[maybe_unused]] auto copy = sp;
                    }
                });
        }
        const auto join = qScopeGuard([&] {
            cancel.store(true, std::memory_order_relaxed);
            for (auto &t : threads)
                t.join();
        });

        QBENCHMARK {
            [[maybe_unused]] auto copy = sp;
        }
    }
};

QTEST_MAIN(tst_QSharedPointer)

#include "tst_bench_shared_ptr.moc"
