/****************************************************************************
**
** Copyright (C) 2019 Samuel Gaist <samuel.gaist@idiap.ch>
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
#include <QSignalSpy>
#include <QSessionManager>
#include <AppKit/AppKit.h>

// Q_DECLARE_METATYPE(QSessionManager)

class tst_SessionManagement_macOS : public QObject
{
    Q_OBJECT

private slots:
    void stopApplication();
};

/*
    Test that session handling code is properly called
*/
void tst_SessionManagement_macOS::stopApplication()
{
    int argc = 0;
    QGuiApplication app(argc, nullptr);
    QSignalSpy spy(&app, &QGuiApplication::commitDataRequest);
    QTimer::singleShot(1000, []() {
         [NSApp terminate:nil];
    });
    app.exec();
    QCOMPARE(spy.count(), 1);
}

QTEST_APPLESS_MAIN(tst_SessionManagement_macOS)
#include "tst_sessionmanagement_macos.moc"
