/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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


#include <QtTest/QtTest>
#include <qsslerror.h>

#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qnetworkproxy.h>

class tst_QSslError : public QObject
{
    Q_OBJECT

public:
    static void enterLoop(int secs)
    {
        ++loopLevel;
        QTestEventLoop::instance().enterLoop(secs);
        --loopLevel;
    }
    static void exitLoop()
    {
        // Safe exit - if we aren't in an event loop, don't
        // exit one.
        if (loopLevel > 0)
            QTestEventLoop::instance().exitLoop();
    }
    static bool timeout()
    {
        return QTestEventLoop::instance().timeout();
    }

#ifndef QT_NO_SSL
private slots:
    void constructing();
    void hash();
#endif

private:
    static int loopLevel;
};

int tst_QSslError::loopLevel = 0;

#ifndef QT_NO_SSL

void tst_QSslError::constructing()
{
    QSslError error;
}

void tst_QSslError::hash()
{
    // mostly a compile-only test, to check that qHash(QSslError) is found
    QSet<QSslError> errors;
    errors << QSslError();
    QCOMPARE(errors.size(), 1);
}

#endif // QT_NO_SSL

QTEST_MAIN(tst_QSslError)
#include "tst_qsslerror.moc"
