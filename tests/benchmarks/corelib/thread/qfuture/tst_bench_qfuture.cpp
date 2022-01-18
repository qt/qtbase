/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QTest>

#include <qexception.h>
#include <qfuture.h>
#include <qpromise.h>
#include <qsemaphore.h>

class tst_QFuture : public QObject
{
    Q_OBJECT

private slots:
    void makeReadyfuture();
#ifndef QT_NO_EXCEPTIONS
    void makeExceptionalFuture();
#endif
    void result();
    void results();
    void takeResult();
    void reportResult();
    void reportResults();
    void reportResultsManualProgress();
#ifndef QT_NO_EXCEPTIONS
    void reportException();
#endif
    void then();
    void thenVoid();
    void onCanceled();
    void onCanceledVoid();
#ifndef QT_NO_EXCEPTIONS
    void onFailed();
    void onFailedVoid();
    void thenOnFailed();
    void thenOnFailedVoid();
#endif
    void reportProgress();
    void progressMinimum();
    void progressMaximum();
    void progressValue();
    void progressText();
};

void tst_QFuture::makeReadyfuture()
{
    QBENCHMARK {
        auto future = QtFuture::makeReadyFuture(42);
        Q_UNUSED(future);
    }
}

#ifndef QT_NO_EXCEPTIONS
void tst_QFuture::makeExceptionalFuture()
{
    QException e;
    QBENCHMARK {
        auto future = QtFuture::makeExceptionalFuture(e);
        Q_UNUSED(future);
    }
}
#endif

void tst_QFuture::result()
{
    auto future = QtFuture::makeReadyFuture(42);

    QBENCHMARK {
        auto value = future.result();
        Q_UNUSED(value);
    }
}

void tst_QFuture::results()
{
    QFutureInterface<int> fi;
    fi.reportStarted();

    for (int i = 0; i < 1000; ++i)
        fi.reportResult(i);

    fi.reportFinished();

    auto future = fi.future();
    QBENCHMARK {
        auto values = future.results();
        Q_UNUSED(values);
    }
}

void tst_QFuture::takeResult()
{
    QBENCHMARK {
        auto future = QtFuture::makeReadyFuture(42);
        auto value = future.takeResult();
        Q_UNUSED(value);
    }
}

void tst_QFuture::reportResult()
{
    QFutureInterface<int> fi;
    QBENCHMARK {
        fi.reportResult(42);
    }
}

void tst_QFuture::reportResults()
{
    QFutureInterface<int> fi;
    QList<int> values(1000);
    std::iota(values.begin(), values.end(), 0);
    QBENCHMARK {
        fi.reportResults(values);
    }
}

void tst_QFuture::reportResultsManualProgress()
{
    QFutureInterface<int> fi;
    const int resultCount = 1000;
    fi.setProgressRange(0, resultCount);
    QBENCHMARK {
        for (int i = 0; i < resultCount; ++i)
            fi.reportResult(i);
    }
}

#ifndef QT_NO_EXCEPTIONS
void tst_QFuture::reportException()
{
    QException e;
    QBENCHMARK {
        QFutureInterface<int> fi;
        fi.reportException(e);
    }
}
#endif

void tst_QFuture::then()
{
    auto f = QtFuture::makeReadyFuture(42);
    QBENCHMARK {
        auto future = f.then([](int value) { return value; });
        Q_UNUSED(future);
    }
}

void tst_QFuture::thenVoid()
{
    auto f = QtFuture::makeReadyFuture();
    QBENCHMARK {
        auto future = f.then([] {});
        Q_UNUSED(future);
    }
}

void tst_QFuture::onCanceled()
{
    QFutureInterface<int> fi;
    fi.reportStarted();
    fi.reportCanceled();
    fi.reportFinished();

    QBENCHMARK {
        auto future = fi.future().onCanceled([] { return 0; });
        Q_UNUSED(future);
    }
}

void tst_QFuture::onCanceledVoid()
{
    QFutureInterface<void> fi;
    fi.reportStarted();
    fi.reportCanceled();
    fi.reportFinished();

    QBENCHMARK {
        auto future = fi.future().onCanceled([] {});
        Q_UNUSED(future);
    }
}

#ifndef QT_NO_EXCEPTIONS
void tst_QFuture::onFailed()
{
    QException e;
    auto f = QtFuture::makeExceptionalFuture<int>(e);
    QBENCHMARK {
        auto future = f.onFailed([] { return 0; });
        Q_UNUSED(future);
    }
}

void tst_QFuture::onFailedVoid()
{
    QException e;
    auto f = QtFuture::makeExceptionalFuture(e);
    QBENCHMARK {
        auto future = f.onFailed([] {});
        Q_UNUSED(future);
    }
}

void tst_QFuture::thenOnFailed()
{
    auto f = QtFuture::makeReadyFuture(42);
    QBENCHMARK {
        auto future =
                f.then([](int) { throw std::runtime_error("error"); }).onFailed([] { return 0; });
        Q_UNUSED(future);
    }
}

void tst_QFuture::thenOnFailedVoid()
{
    auto f = QtFuture::makeReadyFuture();
    QBENCHMARK {
        auto future = f.then([] { throw std::runtime_error("error"); }).onFailed([] {});
        Q_UNUSED(future);
    }
}

#endif

void tst_QFuture::reportProgress()
{
    QFutureInterface<void> fi;
    fi.reportStarted();
    fi.reportFinished();
    QBENCHMARK {
        for (int i = 0; i < 100; ++i) {
            fi.setProgressValue(i);
        }
    }
}

void tst_QFuture::progressMinimum()
{
    QFutureInterface<void> fi;
    fi.setProgressRange(0, 100);
    fi.reportStarted();
    fi.reportFinished();
    auto future = fi.future();

    QBENCHMARK {
        auto value = future.progressMinimum();
        Q_UNUSED(value);
    }
}

void tst_QFuture::progressMaximum()
{
    QFutureInterface<void> fi;
    fi.setProgressRange(0, 100);
    fi.reportStarted();
    fi.reportFinished();
    auto future = fi.future();

    QBENCHMARK {
        auto value = future.progressMaximum();
        Q_UNUSED(value);
    }
}

void tst_QFuture::progressValue()
{
    QFutureInterface<void> fi;
    fi.setProgressValue(50);
    fi.reportStarted();
    fi.reportFinished();
    auto future = fi.future();

    QBENCHMARK {
        auto value = future.progressValue();
        Q_UNUSED(value);
    }
}

void tst_QFuture::progressText()
{
    QFutureInterface<void> fi;
    fi.setProgressValueAndText(50, "text");
    fi.reportStarted();
    fi.reportFinished();
    auto future = fi.future();

    QBENCHMARK {
        auto text = future.progressText();
        Q_UNUSED(text);
    }
}

QTEST_MAIN(tst_QFuture)

#include "tst_bench_qfuture.moc"
