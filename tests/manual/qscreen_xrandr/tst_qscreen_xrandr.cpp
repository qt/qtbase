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
    void xrandr_15_merge_and_unmerge();
    void xrandr_15_scale();

private:
    void xrandr_process(const QStringList &arguments = {});
};

// this test requires an X11 desktop with at least two screens
void tst_QScreen_Xrandr::xrandr_15_merge_and_unmerge()
{
    QStringList originalScreenNames;
    QStringList mergedScreenNames;
    {
        QList<QScreen *> screens = QGuiApplication::screens();
        if (screens.size() < 2)
            QSKIP("This test requires two or more screens.");
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
    QStringList args;
    args << "--setmonitor" << "Qt-merged" << "auto" << mergedScreenNames.join(',');
    xrandr_process(args);

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
    args.clear();
    args << "--delmonitor" << "Qt-merged";
    xrandr_process(args);

    QTRY_COMPARE(removedSpy.count(), 1);
    QVERIFY(QGuiApplication::screens().size() != combinedScreens.size());
    auto separatedScreens = QGuiApplication::screens();
    qDebug() << "added" << addedSpy.count() << "removed" << removedSpy.count() << "separated screen(s):" << separatedScreens.size();
    for (QScreen *s : separatedScreens)
        qDebug() << "screen: " << s->name();

    QCOMPARE(QGuiApplication::screens().size(), originalScreenNames.size());
}

// try to scale the first screen to 1.5x1.5 and scale back to 1x1
void tst_QScreen_Xrandr::xrandr_15_scale()
{
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.size() < 1)
        QSKIP("This test requires at least one screen.");

    QScreen *screen1 = screens.at(0);
    QString name1 = screen1->name();
    int height1 = screen1->size().height();
    int width1 = screen1->size().width();
    qDebug() << "screen " << name1 << ": height=" << height1 << ", width=" << width1;

    int expectedHeight = height1 * 1.5;
    int expectedWidth = width1 * 1.5;

    QSignalSpy geometryChangedSpy1(screen1, &QScreen::geometryChanged);

    // "xrandr --output name1 --scale 1.5x1.5"
    QStringList args;
    args << "--output" << name1 << "--scale" << "1.5x1.5";
    xrandr_process(args);
    QTRY_COMPARE(geometryChangedSpy1.count(), 1);

    QList<QScreen *> screens2 = QGuiApplication::screens();
    QVERIFY(screens2.size() >= 1);
    QScreen *screen2 = nullptr;
    for (QScreen *s : screens2) {
        if (s->name() == name1)
            screen2 = s;
    }
    int height2 = screen2->size().height();
    int width2 = screen2->size().width();
    qDebug() << "screen " << name1 << ": height=" << height2 << ", width=" << width2;
    QVERIFY(height2 == expectedHeight);
    QVERIFY(width2 == expectedWidth);

    QSignalSpy geometryChangedSpy2(screen1, &QScreen::geometryChanged);

    // "xrandr --output name1 --scale 1x1"
    args.clear();
    args << "--output" << name1 << "--scale" << "1x1";
    xrandr_process(args);
    QTRY_COMPARE(geometryChangedSpy2.count(), 1);

    QList<QScreen *> screens3 = QGuiApplication::screens();
    QVERIFY(screens3.size() >= 1);
    QScreen *screen3 = nullptr;
    for (QScreen *s : screens3) {
        if (s->name() == name1)
            screen3 = s;
    }
    int height3 = screen3->size().height();
    int width3 = screen3->size().width();
    qDebug() << "screen " << name1 << ": height=" << height3 << ", width=" << width3;
    QVERIFY(height3 == height1);
    QVERIFY(width3 == width1);
}

void tst_QScreen_Xrandr::xrandr_process(const QStringList &args)
{
    QString prog = "xrandr";
    QProcess *process = new QProcess(this);
    qDebug() << Q_FUNC_INFO << prog << args;
    process->start(prog, args);
    QVERIFY(process->waitForFinished());
}

#include <tst_qscreen_xrandr.moc>
QTEST_MAIN(tst_QScreen_Xrandr);
