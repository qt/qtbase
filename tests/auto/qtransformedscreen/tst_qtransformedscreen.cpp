/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#if defined(Q_WS_QWS)

//TESTED_CLASS=
//TESTED_FILES=

#include <qdesktopwidget.h>
#include <qwindowsystem_qws.h>
#include <qscreen_qws.h>
#include <qscreendriverfactory_qws.h>
#include <qwsdisplay_qws.h>

#ifdef QT_NO_QWS_TRANSFORMED
# undef QT_NO_QWS_TRANSFORMED
#endif

#include <qscreentransformed_qws.h>

class tst_QTransformedScreen : public QObject
{
    Q_OBJECT

public:
    tst_QTransformedScreen() {}
    ~tst_QTransformedScreen() { }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void setTransformation_data();
    void setTransformation();
    void qwsDisplay_setTransformation();

private:
    QTransformedScreen *screen;
    QScreen *oldScreen;
    int id;
};

Q_DECLARE_METATYPE(QTransformedScreen::Transformation);

void tst_QTransformedScreen::initTestCase()
{
    oldScreen = qt_screen;

    QVERIFY(QScreenDriverFactory::keys().contains(QLatin1String("Transformed")));
    QVERIFY(QScreenDriverFactory::keys().contains(QLatin1String("VNC")));

    id = 10;
    screen = static_cast<QTransformedScreen*>(QScreenDriverFactory::create("Transformed", id));
    QVERIFY(screen);
    QVERIFY(screen->connect(QString("Transformed:Rot90:VNC:%1").arg(id)));
    QVERIFY(screen->initDevice());
}

void tst_QTransformedScreen::cleanupTestCase()
{
    screen->shutdownDevice();
    screen->disconnect();
    delete screen;
    screen = 0;

    qt_screen = oldScreen;
}

void tst_QTransformedScreen::setTransformation_data()
{
    QTest::addColumn<QTransformedScreen::Transformation>("transformation");
    QTest::addColumn<bool>("swap");

    QTest::newRow("Rot0") << QTransformedScreen::None << false;
    QTest::newRow("Rot90") << QTransformedScreen::Rot90 << true;
    QTest::newRow("Rot180") << QTransformedScreen::Rot180 << false;
    QTest::newRow("Rot270") << QTransformedScreen::Rot270 << true;
}

void tst_QTransformedScreen::setTransformation()
{
    // Not really failures but equal height and width makes this test useless
    QVERIFY(screen->deviceWidth() != screen->deviceHeight());

    screen->setTransformation(QTransformedScreen::None);
    int dw = screen->deviceWidth();
    int dh = screen->deviceHeight();
    int mmw = screen->physicalWidth();
    int mmh = screen->physicalHeight();

    QFETCH(QTransformedScreen::Transformation, transformation);
    QFETCH(bool, swap);

    screen->setTransformation(transformation);
    QCOMPARE(screen->deviceWidth(), dw);
    QCOMPARE(screen->deviceHeight(), dh);

    if (swap) {
        QCOMPARE(screen->width(), dh);
        QCOMPARE(screen->height(), dw);
        QCOMPARE(screen->physicalWidth(), mmh);
        QCOMPARE(screen->physicalHeight(), mmw);
    } else {
        QCOMPARE(screen->width(), dw);
        QCOMPARE(screen->height(), dh);
        QCOMPARE(screen->physicalWidth(), mmw);
        QCOMPARE(screen->physicalHeight(), mmh);
    }
}

void tst_QTransformedScreen::qwsDisplay_setTransformation()
{
    QDesktopWidget *desktop = QApplication::desktop();

    // test maximized windows
    {
        QWidget w;
        w.showMaximized();
        QApplication::processEvents();

        const int screen = desktop->screenNumber(&w);
        QCOMPARE(desktop->availableGeometry(screen), w.frameGeometry());

        for (int i = QTransformedScreen::None; i <= QTransformedScreen::Rot270; ++i) {
            QWSDisplay::instance()->setTransformation(i, screen);
            QApplication::processEvents();
            QCOMPARE(desktop->availableGeometry(screen), w.frameGeometry());
        }
    }

    // test fullscreen windows
    {
        QWidget w;
        w.showFullScreen();
        QApplication::processEvents();

        const int screen = desktop->screenNumber(&w);
        QCOMPARE(desktop->screenGeometry(screen), w.geometry());

        for (int i = QTransformedScreen::None; i <= QTransformedScreen::Rot270; ++i) {
            QWSDisplay::instance()->setTransformation(i, screen);
            QApplication::processEvents();
            QCOMPARE(desktop->screenGeometry(screen), w.geometry());
        }
    }
}

QTEST_MAIN(tst_QTransformedScreen)

#include "tst_qtransformedscreen.moc"

#else // Q_WS_QWS
QTEST_NOOP_MAIN
#endif
