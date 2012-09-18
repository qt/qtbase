/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

    static char argv0[] = "tst_QProcessNoApplication";
    char *argv[] = { argv0, 0 };
    int argc = 1;
    QCoreApplication app(argc, argv);
    MyThread thread;
    thread.start();
    QVERIFY(thread.wait(10000));
}

QTEST_APPLESS_MAIN(tst_QProcessNoApplication)

#include "tst_qprocessnoapplication.moc"
