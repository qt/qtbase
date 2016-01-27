/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QDate>
#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtGui>
#ifdef Q_OS_WINCE_WM
#include <windows.h>
#include "ddhelper.h"
#endif



class tst_WindowsMobile : public QObject
{
    Q_OBJECT
public:
    tst_WindowsMobile()
    {
       qApp->setCursorFlashTime (24 * 3600 * 1000); // once a day
       // qApp->setCursorFlashTime (INT_MAX);
#ifdef Q_OS_WINCE_WM
        q_initDD();
#endif
    }

#if defined(Q_OS_WINCE_WM) && defined(_WIN32_WCE) && _WIN32_WCE <= 0x501
    private slots:
        void testMainWindowAndMenuBar();
        void testSimpleWidget();
#endif
};

#if defined(Q_OS_WINCE_WM) && defined(_WIN32_WCE) && _WIN32_WCE <= 0x501

bool qt_wince_is_platform(const QString &platformString) {
    wchar_t tszPlatform[64];
    if (SystemParametersInfo(SPI_GETPLATFORMTYPE,
                             sizeof(tszPlatform)/sizeof(*tszPlatform),tszPlatform,0))
      if (0 == _tcsicmp(reinterpret_cast<const wchar_t *> (platformString.utf16()), tszPlatform))
            return true;
    return false;
}

bool qt_wince_is_smartphone() {
       return qt_wince_is_platform(QString::fromLatin1("Smartphone"));
}

void openMenu()
{
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,450,630,0,0);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,450,630,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,65535,65535,0,0);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,65535,65535,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,55535,55535,0,0);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,55535,55535,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,55535,58535,0,0);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,55535,58535,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,40535,55535,0,0);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,40535,55535,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,32535,55535,0,0);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,32535,55535,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,65535,65535,0,0);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,65535,65535,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,55535,50535,0,0);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,55535,50535,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,55535,40535,0,0);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,55535,40535,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE,48535,45535,0,0);
    QTest::qWait(2000);
    ::mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE,48535,45535,0,0);
}

void compareScreenshots(const QString &image1, const QString &image2)
{
    QImage screenShot(image1);
    QImage original(image2);

    // cut away the title bar before comparing
    QDesktopWidget desktop;
    QRect desktopFrameRect  = desktop.frameGeometry();
    QRect desktopClientRect = desktop.availableGeometry();

    QPainter p1(&screenShot);
    QPainter p2(&original);

    //screenShot.save("scr1.png", "PNG");
    p1.fillRect(0, 0, desktopFrameRect.width(), desktopClientRect.y(), Qt::black);
    p2.fillRect(0, 0, desktopFrameRect.width(), desktopClientRect.y(), Qt::black);

    //screenShot.save("scr2.png", "PNG");
    //original.save("orig1.png", "PNG");

    QCOMPARE(original, screenShot);
}

void takeScreenShot(const QString filename)
{
    q_lock();
    QImage image = QImage(( uchar *) q_frameBuffer(), q_screenWidth(),
        q_screenHeight(), q_screenWidth() * q_screenDepth() / 8, QImage::Format_RGB16);
    image.save(filename, "PNG");
    q_unlock();
}

void tst_WindowsMobile::testMainWindowAndMenuBar()
{
    if (qt_wince_is_smartphone())
        QSKIP("This test is only for Windows Mobile");

    QProcess process;
    process.start("testQMenuBar.exe");
    QCOMPARE(process.state(), QProcess::Running);
    QTest::qWait(6000);
    openMenu();
    QTest::qWait(1000);
    takeScreenShot("testQMenuBar_current.png");
    process.close();
    compareScreenshots("testQMenuBar_current.png", ":/testQMenuBar_current.png");
}

void tst_WindowsMobile::testSimpleWidget()
{
    if (qt_wince_is_smartphone())
        QSKIP("This test is only for Windows Mobile");

    QMenuBar menubar;
    menubar.show();
    QWidget maximized;
    QPalette pal = maximized.palette();
    pal.setColor(QPalette::Background, Qt::red);
    maximized.setPalette(pal);
    maximized.showMaximized();
    QWidget widget;
    widget.setGeometry(100, 100, 200, 200);
    widget.setWindowTitle("Widget");
    widget.show();
    qApp->processEvents();
    QTest::qWait(1000);

    QWidget widget2;
    widget2.setGeometry(100, 380, 300, 200);
    widget2.setWindowTitle("Widget 2");
    widget2.setWindowFlags(Qt::Popup);
    widget2.show();

    qApp->processEvents();
    QTest::qWait(1000);
    takeScreenShot("testSimpleWidget_current.png");
    compareScreenshots("testSimpleWidget_current.png", ":/testSimpleWidget_current.png");
}


#endif //Q_OS_WINCE_WM


QTEST_MAIN(tst_WindowsMobile)
#include "tst_windowsmobile.moc"

