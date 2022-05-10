// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    void xrandr_15_off_and_on();
    void xrandr_15_primary();
    void xrandr_15_rotate();

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

    QSignalSpy geometryChangedSpy2(screen2, &QScreen::geometryChanged);

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

void tst_QScreen_Xrandr::xrandr_15_off_and_on()
{
    QList<QScreen *> screens = QGuiApplication::screens();
    int ss = screens.size();
    if (ss < 1)
        QSKIP("This test requires at least one screen.");

    QStringList names;
    for (QScreen *s : screens)
        names << s->name();

    QStringList args;
    for (int i = ss; i > 0; i--) {
        QString name = names.at(i-1);
        qDebug() << "i=" << i << ", name:" << name;
        args.clear();
        args << "--output" << name << "--off";
        xrandr_process(args);
        QTest::qWait(500);

        QVERIFY(QGuiApplication::screens().size() == (i != 1 ? i - 1 : i));
    }

    for (int i = ss; i > 0; i--) {
        QString name = names.at(i-1);
        qDebug() << "i=" << i << ", name:" << name;
        args.clear();
        args << "--output" << name << "--auto";
        xrandr_process(args);
        QTest::qWait(500);

        QVERIFY(QGuiApplication::screens().size() == (i == ss ? 1 : ss - i + 1));
    }
}

void tst_QScreen_Xrandr::xrandr_15_primary()
{
    QList<QScreen *> screens = QGuiApplication::screens();
    int ss = screens.size();
    if (ss < 2)
        QSKIP("This test requires at least two screens.");

    QStringList names;
    for (QScreen *s : screens)
        names << s->name();

    qDebug() << "All screens: " << names;
    QScreen *ps = qGuiApp->primaryScreen();
    qDebug() << "Current primary screen: " << ps;

    QStringList args;
    for (QString name : names) {
        qDebug() << "Trying to set primary screen:" << name;
        args.clear();
        args << "--output" << name << "--primary";
        xrandr_process(args);
        QTest::qWait(500);

        QScreen *ps = qGuiApp->primaryScreen();
        qDebug() << "Current primary screen: " << ps;
        if (ps) {
            qDebug() << "primary screen name: " << ps->name();
            QVERIFY(ps->name() == name);
        }
    }
}

void tst_QScreen_Xrandr::xrandr_15_rotate()
{
    QList<QScreen *> screens = QGuiApplication::screens();
    int ss = screens.size();
    if (ss < 1)
        QSKIP("This test requires at least one screen.");

    QScreen *screen1 = screens.at(0);
    QString name1 = screen1->name();
    int height1 = screen1->size().height();
    int width1 = screen1->size().width();
    qDebug() << "screen " << name1 << ": height=" << height1 << ", width=" << width1;

    int expectedHeight = width1;
    int expectedWidth = height1;

    QSignalSpy geometryChangedSpy1(screen1, &QScreen::geometryChanged);

    // "xrandr --output name1 --rotate left"
    QStringList args;
    args << "--output" << name1 << "--rotate" << "left";
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

    QSignalSpy geometryChangedSpy2(screen2, &QScreen::geometryChanged);

    // "xrandr --output name1 --rotate normal"
    args.clear();
    args << "--output" << name1 << "--rotate" << "normal";
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
