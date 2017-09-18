/****************************************************************************
**
** Copyright (C) 2013 David Faure <david.faure@kdab.com>
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

#include <qtest.h>
#include <QtCore>

class tst_QThreadPool : public QObject
{
    Q_OBJECT

public:
    tst_QThreadPool();
    ~tst_QThreadPool();

private slots:
    void startRunnables();
    void activeThreadCount();
};

tst_QThreadPool::tst_QThreadPool()
{
}

tst_QThreadPool::~tst_QThreadPool()
{
}

class NoOpRunnable : public QRunnable
{
public:
    void run() override {
    }
};

void tst_QThreadPool::startRunnables()
{
    QThreadPool threadPool;
    threadPool.setMaxThreadCount(10);
    QBENCHMARK {
        threadPool.start(new NoOpRunnable());
    }
}

void tst_QThreadPool::activeThreadCount()
{
    QThreadPool threadPool;
    threadPool.start(new NoOpRunnable());
    QBENCHMARK {
        QVERIFY(threadPool.activeThreadCount() <= 10);
    }
}

QTEST_MAIN(tst_QThreadPool)
#include "tst_qthreadpool.moc"
