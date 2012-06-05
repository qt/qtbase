/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qscreen.h>

#include <QtTest/QtTest>

class tst_QScreen: public QObject
{
    Q_OBJECT

private slots:
    void angleBetween_data();
    void angleBetween();
    void transformBetween_data();
    void transformBetween();
    void orientationChange();
};

void tst_QScreen::angleBetween_data()
{
    QTest::addColumn<uint>("oa");
    QTest::addColumn<uint>("ob");
    QTest::addColumn<int>("expected");

    QTest::newRow("Portrait Portrait")
        << uint(Qt::PortraitOrientation)
        << uint(Qt::PortraitOrientation)
        << 0;

    QTest::newRow("Portrait Landscape")
        << uint(Qt::PortraitOrientation)
        << uint(Qt::LandscapeOrientation)
        << 270;

    QTest::newRow("Portrait InvertedPortrait")
        << uint(Qt::PortraitOrientation)
        << uint(Qt::InvertedPortraitOrientation)
        << 180;

    QTest::newRow("Portrait InvertedLandscape")
        << uint(Qt::PortraitOrientation)
        << uint(Qt::InvertedLandscapeOrientation)
        << 90;

    QTest::newRow("InvertedLandscape InvertedPortrait")
        << uint(Qt::InvertedLandscapeOrientation)
        << uint(Qt::InvertedPortraitOrientation)
        << 90;

    QTest::newRow("InvertedLandscape Landscape")
        << uint(Qt::InvertedLandscapeOrientation)
        << uint(Qt::LandscapeOrientation)
        << 180;

    QTest::newRow("Landscape Primary")
        << uint(Qt::LandscapeOrientation)
        << uint(Qt::PrimaryOrientation)
        << QGuiApplication::primaryScreen()->angleBetween(Qt::LandscapeOrientation, QGuiApplication::primaryScreen()->primaryOrientation());
}

void tst_QScreen::angleBetween()
{
    QFETCH( uint, oa );
    QFETCH( uint, ob );
    QFETCH( int, expected );

    Qt::ScreenOrientation a = Qt::ScreenOrientation(oa);
    Qt::ScreenOrientation b = Qt::ScreenOrientation(ob);

    QCOMPARE(QGuiApplication::primaryScreen()->angleBetween(a, b), expected);
    QCOMPARE(QGuiApplication::primaryScreen()->angleBetween(b, a), (360 - expected) % 360);
}

void tst_QScreen::transformBetween_data()
{
    QTest::addColumn<uint>("oa");
    QTest::addColumn<uint>("ob");
    QTest::addColumn<QRect>("rect");
    QTest::addColumn<QTransform>("expected");

    QRect rect(0, 0, 480, 640);

    QTest::newRow("Portrait Portrait")
        << uint(Qt::PortraitOrientation)
        << uint(Qt::PortraitOrientation)
        << rect
        << QTransform();

    QTest::newRow("Portrait Landscape")
        << uint(Qt::PortraitOrientation)
        << uint(Qt::LandscapeOrientation)
        << rect
        << QTransform(0, -1, 1, 0, 0, rect.height());

    QTest::newRow("Portrait InvertedPortrait")
        << uint(Qt::PortraitOrientation)
        << uint(Qt::InvertedPortraitOrientation)
        << rect
        << QTransform(-1, 0, 0, -1, rect.width(), rect.height());

    QTest::newRow("Portrait InvertedLandscape")
        << uint(Qt::PortraitOrientation)
        << uint(Qt::InvertedLandscapeOrientation)
        << rect
        << QTransform(0, 1, -1, 0, rect.width(), 0);

    QTest::newRow("InvertedLandscape InvertedPortrait")
        << uint(Qt::InvertedLandscapeOrientation)
        << uint(Qt::InvertedPortraitOrientation)
        << rect
        << QTransform(0, 1, -1, 0, rect.width(), 0);

    QTest::newRow("InvertedLandscape Landscape")
        << uint(Qt::InvertedLandscapeOrientation)
        << uint(Qt::LandscapeOrientation)
        << rect
        << QTransform(-1, 0, 0, -1, rect.width(), rect.height());

    QTest::newRow("Landscape Primary")
        << uint(Qt::LandscapeOrientation)
        << uint(Qt::PrimaryOrientation)
        << rect
        << QGuiApplication::primaryScreen()->transformBetween(Qt::LandscapeOrientation, QGuiApplication::primaryScreen()->primaryOrientation(), rect);
}

void tst_QScreen::transformBetween()
{
    QFETCH( uint, oa );
    QFETCH( uint, ob );
    QFETCH( QRect, rect );
    QFETCH( QTransform, expected );

    Qt::ScreenOrientation a = Qt::ScreenOrientation(oa);
    Qt::ScreenOrientation b = Qt::ScreenOrientation(ob);

    QCOMPARE(QGuiApplication::primaryScreen()->transformBetween(a, b, rect), expected);
}

void tst_QScreen::orientationChange()
{
    qRegisterMetaType<Qt::ScreenOrientation>("Qt::ScreenOrientation");

    QScreen *screen = QGuiApplication::primaryScreen();

    screen->setOrientationUpdateMask(Qt::LandscapeOrientation | Qt::PortraitOrientation);

    QWindowSystemInterface::handleScreenOrientationChange(screen, Qt::LandscapeOrientation);
    QTRY_COMPARE(screen->orientation(), Qt::LandscapeOrientation);

    QWindowSystemInterface::handleScreenOrientationChange(screen, Qt::PortraitOrientation);
    QTRY_COMPARE(screen->orientation(), Qt::PortraitOrientation);

    QSignalSpy spy(screen, SIGNAL(orientationChanged(Qt::ScreenOrientation)));

    QWindowSystemInterface::handleScreenOrientationChange(screen, Qt::InvertedLandscapeOrientation);
    QWindowSystemInterface::handleScreenOrientationChange(screen, Qt::InvertedPortraitOrientation);
    QWindowSystemInterface::handleScreenOrientationChange(screen, Qt::LandscapeOrientation);

    QTRY_COMPARE(screen->orientation(), Qt::LandscapeOrientation);
    QCOMPARE(spy.count(), 1);
}

#include <tst_qscreen.moc>
QTEST_MAIN(tst_QScreen);
