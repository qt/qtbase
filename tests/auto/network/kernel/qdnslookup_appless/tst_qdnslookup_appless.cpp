/****************************************************************************
**
** Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <QtNetwork/QDnsLookup>
#include <QtTest/QtTest>

class tst_QDnsLookup_Appless : public QObject
{
    Q_OBJECT

private slots:
    void noApplication();
    void recreateApplication();
    void destroyApplicationDuringLookup();
};

void tst_QDnsLookup_Appless::noApplication()
{
    QTest::ignoreMessage(QtWarningMsg, "QDnsLookup requires a QCoreApplication");
    QDnsLookup dns(QDnsLookup::A, "a-single.test.qt-project.org");
    dns.lookup();
}

void tst_QDnsLookup_Appless::recreateApplication()
{
    int argc = 0;
    char **argv = 0;
    for (int i = 0; i < 10; ++i) {
        QCoreApplication app(argc, argv);
        QDnsLookup dns(QDnsLookup::A, "a-single.test.qt-project.org");
        dns.lookup();
        if (!dns.isFinished()) {
            QObject::connect(&dns, SIGNAL(finished()),
                             &QTestEventLoop::instance(), SLOT(exitLoop()));
            QTestEventLoop::instance().enterLoop(10);
        }
        QVERIFY(dns.isFinished());
    }
}

void tst_QDnsLookup_Appless::destroyApplicationDuringLookup()
{
    int argc = 0;
    char **argv = 0;
    for (int i = 0; i < 10; ++i) {
        QCoreApplication app(argc, argv);
        QDnsLookup dns(QDnsLookup::A, "a-single.test.macieira.info");
        dns.lookup();
    }
}

QTEST_APPLESS_MAIN(tst_QDnsLookup_Appless)
#include "tst_qdnslookup_appless.moc"
