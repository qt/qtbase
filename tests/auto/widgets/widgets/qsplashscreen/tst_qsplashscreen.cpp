// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <QSplashScreen>
#include <QTimer>

class tst_QSplashScreen : public QObject
{
    Q_OBJECT

private slots:
    void checkCloseTime();
    void checkConstructorAndShow();
};

class CloseEventSplash : public QSplashScreen
{
public:
    CloseEventSplash(const QPixmap &pix) : QSplashScreen(pix), receivedCloseEvent(false) {}
    bool receivedCloseEvent;
protected:
    void closeEvent(QCloseEvent *event) override
    {
        receivedCloseEvent = true;
        QSplashScreen::closeEvent(event);
    }
};

void tst_QSplashScreen::checkCloseTime()
{
    QPixmap pix(100, 100);
    pix.fill(Qt::red);
    CloseEventSplash splash(pix);
    QVERIFY(!splash.receivedCloseEvent);
    QWidget w;
    splash.show();
    QTimer::singleShot(10, &w, &QWidget::show);
    QVERIFY(!splash.receivedCloseEvent);
    splash.finish(&w);
    QVERIFY(splash.receivedCloseEvent);
    // We check the window handle because if this is not valid, then
    // it can't have been exposed
    QVERIFY(w.windowHandle());
    QVERIFY(w.windowHandle()->isVisible());
}

void tst_QSplashScreen::checkConstructorAndShow()
{
    QPixmap pix(100, 100);
    pix.fill(Qt::red);
    for (auto *screen : QGuiApplication::screens()) {
        QSplashScreen splash(screen, pix);
        splash.show();
        QCOMPARE(splash.screen(), screen);
        QVERIFY(splash.windowHandle());
        QVERIFY(splash.windowHandle()->isVisible());
        QCOMPARE(splash.windowHandle()->screen(), screen);
    }
}

QTEST_MAIN(tst_QSplashScreen)
#include "tst_qsplashscreen.moc"
