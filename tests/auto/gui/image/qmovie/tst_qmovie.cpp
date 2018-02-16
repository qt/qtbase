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


#include <QIODevice>
#ifndef QT_NO_WIDGETS
#include <QLabel>
#endif
#include <QMovie>

class tst_QMovie : public QObject
{
    Q_OBJECT

public:
    tst_QMovie();
    virtual ~tst_QMovie();

public slots:
    void init();
    void cleanup();

protected slots:
    void exitLoopSlot();

private slots:
    void getSetCheck();
    void construction();
    void playMovie_data();
    void playMovie();
    void jumpToFrame_data();
    void jumpToFrame();
    void changeMovieFile();
#ifndef QT_NO_WIDGETS
    void infiniteLoop();
#endif
};

// Testing get/set functions
void tst_QMovie::getSetCheck()
{
    QMovie obj1;
    // QIODevice * QMovie::device()
    // void QMovie::setDevice(QIODevice *)
    QFile *var1 = new QFile;
    obj1.setDevice(var1);
    QCOMPARE(obj1.device(), (QIODevice *)var1);
    obj1.setDevice((QIODevice *)0);
    QCOMPARE(obj1.device(), (QIODevice *)0);
    delete var1;

    // CacheMode QMovie::cacheMode()
    // void QMovie::setCacheMode(CacheMode)
    obj1.setCacheMode(QMovie::CacheMode(QMovie::CacheNone));
    QCOMPARE(QMovie::CacheMode(QMovie::CacheNone), obj1.cacheMode());
    obj1.setCacheMode(QMovie::CacheMode(QMovie::CacheAll));
    QCOMPARE(QMovie::CacheMode(QMovie::CacheAll), obj1.cacheMode());

    // int QMovie::speed()
    // void QMovie::setSpeed(int)
    obj1.setSpeed(0);
    QCOMPARE(0, obj1.speed());
    obj1.setSpeed(INT_MIN);
    QCOMPARE(INT_MIN, obj1.speed());
    obj1.setSpeed(INT_MAX);
    QCOMPARE(INT_MAX, obj1.speed());
}

tst_QMovie::tst_QMovie()
{
}

tst_QMovie::~tst_QMovie()
{

}

void tst_QMovie::init()
{
}

void tst_QMovie::cleanup()
{
}

void tst_QMovie::exitLoopSlot()
{
    QTestEventLoop::instance().exitLoop();
}

void tst_QMovie::construction()
{
    QMovie movie;
    QCOMPARE(movie.device(), (QIODevice *)0);
    QCOMPARE(movie.fileName(), QString());
    QCOMPARE(movie.state(), QMovie::NotRunning);
}

void tst_QMovie::playMovie_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("frameCount");
#ifdef QTEST_HAVE_GIF
    QTest::newRow("comicsecard") << QString("animations/comicsecard.gif") << 5;
    QTest::newRow("trolltech") << QString("animations/trolltech.gif") << 34;
#endif
}

void tst_QMovie::playMovie()
{
    QFETCH(QString, fileName);
    QFETCH(int, frameCount);

    QMovie movie(QFINDTESTDATA(fileName));

    QCOMPARE(movie.state(), QMovie::NotRunning);
    movie.setSpeed(1000);
    movie.start();
    QCOMPARE(movie.state(), QMovie::Running);
    movie.setPaused(true);
    QCOMPARE(movie.state(), QMovie::Paused);
    movie.start();
    QCOMPARE(movie.state(), QMovie::Running);
    movie.stop();
    QCOMPARE(movie.state(), QMovie::NotRunning);
    movie.jumpToFrame(0);
    QCOMPARE(movie.state(), QMovie::NotRunning);
    movie.start();
    QCOMPARE(movie.state(), QMovie::Running);

    connect(&movie, SIGNAL(finished()), this, SLOT(exitLoopSlot()));

#ifndef QT_NO_WIDGETS
    QLabel label;
    label.setMovie(&movie);
    label.show();

    QTestEventLoop::instance().enterLoop(20);
    QVERIFY2(!QTestEventLoop::instance().timeout(),
            "Timed out while waiting for finished() signal");

    QCOMPARE(movie.state(), QMovie::NotRunning);
    QCOMPARE(movie.frameCount(), frameCount);
#endif

    movie.stop();
    QSignalSpy finishedSpy(&movie, &QMovie::finished);
    movie.setSpeed(0);
    movie.start();
    QCOMPARE(movie.state(), QMovie::Running);
    QTestEventLoop::instance().enterLoop(2);
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(movie.state(), QMovie::Running);
    QCOMPARE(movie.currentFrameNumber(), 0);
}

void tst_QMovie::jumpToFrame_data()
{
    playMovie_data();
}

void tst_QMovie::jumpToFrame()
{
    QFETCH(QString, fileName);
    QMovie movie(QFINDTESTDATA(fileName));
    movie.start();
    movie.stop();
    QVERIFY(!movie.jumpToFrame(-1));
    QCOMPARE(movie.currentFrameNumber(), 0);
}

void tst_QMovie::changeMovieFile()
{
    QMovie movie(QFINDTESTDATA("animations/comicsecard.gif"));
    movie.start();
    movie.stop();
    movie.setFileName(QFINDTESTDATA("animations/trolltech.gif"));
    QCOMPARE(movie.currentFrameNumber(), -1);
}

#ifndef QT_NO_WIDGETS
void tst_QMovie::infiniteLoop()
{
    QLabel label;
    label.show();
    QMovie *movie = new QMovie(QLatin1String(":animations/corrupt.gif"), QByteArray(), &label);
    label.setMovie(movie);
    movie->start();

    QTestEventLoop::instance().enterLoop(1);
    QTestEventLoop::instance().timeout();
}
#endif

QTEST_MAIN(tst_QMovie)
#include "tst_qmovie.moc"
