/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <qpainter.h>
#include <qrasterwindow.h>
#include <qscreen.h>
#include <qpa/qwindowsysteminterface.h>
#include <QProcess>

#include <QtTest/QtTest>

class tst_QScreen_Xrandr: public QObject
{
    Q_OBJECT

private slots:
    void xrandr_15();
};

// this test requires an X11 desktop with at least two screens
void tst_QScreen_Xrandr::xrandr_15()
{
    QStringList originalScreenNames;
    QStringList mergedScreenNames;
    {
        QList<QScreen *> screens = QGuiApplication::screens();
        QVERIFY(screens.size() >= 2);
        qDebug() << "initial set of screens:" << screens.size();
        for (QScreen *s : screens) {
            qDebug() << "screen: " << s->name();
            originalScreenNames << s->name();
            if (mergedScreenNames.size() < 2)
                mergedScreenNames << s->name();
        }
    }

    QSignalSpy addedSpy(qApp, &QGuiApplication::screenAdded);
    QSignalSpy removedSpy(qApp, &QGuiApplication::screenRemoved);

    // combine the first two screens into one monitor
    // e.g. "xrandr --setmonitor Qt-merged auto eDP,HDMI-0"
    QString prog1 = "xrandr";
    QStringList args1;
    args1 << "--setmonitor" << "Qt-merged" << "auto" << mergedScreenNames.join(',');
    qDebug() << prog1 << args1;
    QProcess *myProcess1 = new QProcess(this);
    myProcess1->start(prog1, args1);
    QVERIFY(myProcess1->waitForFinished());

    QTRY_COMPARE(removedSpy.count(), 2);
    QVERIFY(QGuiApplication::screens().size() != originalScreenNames.size());
    auto combinedScreens = QGuiApplication::screens();
    qDebug() << "added" << addedSpy.count() << "removed" << removedSpy.count() << "combined screen(s):" << combinedScreens.size();
    QScreen *merged = nullptr;
    for (QScreen *s : combinedScreens) {
        qDebug() << "screen: " << s->name();
        if (s->name() == QLatin1String("Qt-merged"))
            merged = s;
    }
    // the screen that we created must be in the list now
    QVERIFY(merged);
    QCOMPARE(QGuiApplication::screens().size(), originalScreenNames.size() - 1);
    addedSpy.clear();
    removedSpy.clear();

    // "xrandr --delmonitor Qt-merged"
    QString prog2 = "xrandr";
    QStringList args2;
    args2 << "--delmonitor" << "Qt-merged";
    QProcess *myProcess2 = new QProcess(this);
    myProcess2->start(prog2, args2);
    QVERIFY(myProcess2->waitForFinished());

    QTRY_COMPARE(removedSpy.count(), 1);
    QVERIFY(QGuiApplication::screens().size() != combinedScreens.size());
    auto separatedScreens = QGuiApplication::screens();
    qDebug() << "added" << addedSpy.count() << "removed" << removedSpy.count() << "separated screen(s):" << separatedScreens.size();
    for (QScreen *s : separatedScreens)
        qDebug() << "screen: " << s->name();

    QCOMPARE(QGuiApplication::screens().size(), originalScreenNames.size());
}

#include <tst_qscreen_xrandr.moc>
QTEST_MAIN(tst_QScreen_Xrandr);
