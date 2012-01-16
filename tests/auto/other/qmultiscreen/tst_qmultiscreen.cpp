/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qdesktopwidget.h>
#include <qscreen_qws.h>
#include <qscreendriverfactory_qws.h>
#include <qlabel.h>

class tst_QMultiScreen : public QObject
{
    Q_OBJECT

public:
    tst_QMultiScreen() : screen(0), oldScreen(0) {}
    ~tst_QMultiScreen() {}

private slots:
    void initTestCase();
    void cleanupTestCase();
    void widgetSetFixedSize();
    void grabWindow();

private:
    QScreen *screen;
    QScreen *oldScreen;
};

void tst_QMultiScreen::cleanupTestCase()
{
    screen->shutdownDevice();
    screen->disconnect();
    delete screen;
    screen = 0;

    qt_screen = oldScreen;
}

void tst_QMultiScreen::initTestCase()
{
    oldScreen = qt_screen;

    QVERIFY(QScreenDriverFactory::keys().contains(QLatin1String("Multi")));
    QVERIFY(QScreenDriverFactory::keys().contains(QLatin1String("VNC")));

    const int id = 10;
    screen = QScreenDriverFactory::create("Multi", id);
    QVERIFY(screen);
    QVERIFY(screen->connect(QString("Multi: "
                                    "VNC:size=640x480:depth=32:offset=0,0:%1 "
                                    "VNC:size=640x480:depth=32:offset=640,0:%2 "
                                    "VNC:size=640x480:depth=16:offset=0,480:%3 "
                                    ":%4")
                            .arg(id+1).arg(id+2).arg(id+3).arg(id)));
    QVERIFY(screen->initDevice());

    QDesktopWidget desktop;
    QCOMPARE(desktop.numScreens(), 3);
}

void tst_QMultiScreen::widgetSetFixedSize()
{
    QDesktopWidget desktop;
    QRect maxRect;
    for (int i = 0; i < desktop.numScreens(); ++i)
        maxRect |= desktop.availableGeometry(i);

    maxRect = maxRect.adjusted(50, 50, -50, -50);

    // make sure we can set a size larger than a single screen (task 166368)
    QWidget w;
    w.setFixedSize(maxRect.size());
    w.show();
    QApplication::processEvents();
    QCOMPARE(w.geometry().size(), maxRect.size());
}

void tst_QMultiScreen::grabWindow()
{
    QDesktopWidget desktop;

    QVERIFY(desktop.numScreens() >= 2);

    const QRect r0 = desktop.availableGeometry(0).adjusted(50, 50, -50, -50);
    const QRect r1 = desktop.availableGeometry(1).adjusted(60, 60, -60, -60);

    QWidget w;
    w.setGeometry(r0);
    w.show();

    QLabel l("hi there");
    l.setGeometry(r1);
    l.show();

    QApplication::processEvents();
    QApplication::sendPostedEvents(); // workaround for glib event loop
    QVERIFY(desktop.screenNumber(&w) == 0);
    QVERIFY(desktop.screenNumber(&l) == 1);

    const QPixmap p0 = QPixmap::grabWindow(w.winId());
    const QPixmap p1 = QPixmap::grabWindow(l.winId());

//     p0.save("w.png", "PNG");
//     p1.save("l.png", "PNG");
    QCOMPARE(p0.size(), w.size());
    QCOMPARE(p1.size(), l.size());

    const QImage img0 = p0.toImage();
    const QImage img1 = p1.toImage();

//     QPixmap::grabWidget(&w).toImage().convertToFormat(img0.format()).save("w_img.png", "PNG");
//     QPixmap::grabWidget(&l).toImage().convertToFormat(img1.format()).save("l_img.png", "PNG");

    QImage::Format format = QImage::Format_RGB16;
    QCOMPARE(img0.convertToFormat(format),
             QPixmap::grabWidget(&w).toImage().convertToFormat(format));
    QCOMPARE(img1.convertToFormat(format),
             QPixmap::grabWidget(&l).toImage().convertToFormat(format));
}

QTEST_MAIN(tst_QMultiScreen)

#include "tst_qmultiscreen.moc"
