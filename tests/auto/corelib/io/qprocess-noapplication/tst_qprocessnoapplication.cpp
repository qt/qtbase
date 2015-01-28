/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QtTest>

class tst_QProcessNoApplication : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initializationDeadlock();
};

void tst_QProcessNoApplication::initializationDeadlock()
{
    // see QTBUG-27260
    // QProcess on Unix uses (or used to, at the time of the writing of this test)
    // a global class called QProcessManager.
    // This class is instantiated (or was) only in the main thread, which meant that
    // blocking the main thread while waiting for QProcess could mean a deadlock.

    struct MyThread : public QThread
    {
        void run()
        {
            // what we execute does not matter, as long as we try to
            // and that the process exits
            QProcess::execute("true");
        }
    };

    char *argv[] = { const_cast<char*>(QTest::currentAppName()), 0 };
    int argc = 1;
    QCoreApplication app(argc, argv);
    MyThread thread;
    thread.start();
    QVERIFY(thread.wait(10000));
}

QTEST_APPLESS_MAIN(tst_QProcessNoApplication)

#include "tst_qprocessnoapplication.moc"
