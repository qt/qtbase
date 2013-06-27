/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qapplication.h>
#include <qwindow.h>
#include <qwidget.h>



class Window : public QWindow
{
public:
    Window()
        : numberOfExposes(0)
        , numberOfObscures(0)
    {
    }

    void exposeEvent(QExposeEvent *) {
        if (isExposed())
            ++numberOfExposes;
        else
            ++numberOfObscures;
    }

    int numberOfExposes;
    int numberOfObscures;
};



class tst_QWindowContainer: public QObject
{
    Q_OBJECT
private slots:
    void testShow();
    void testPositionAndSize();
    void testExposeObscure();
    void testOwnership();
    void testBehindTheScenesDeletion();
    void testUnparenting();
    void testActivation();
};



void tst_QWindowContainer::testShow()
{
    QWidget root;
    root.setGeometry(100, 100, 400, 400);

    Window *window = new Window();
    QWidget *container = QWidget::createWindowContainer(window, &root);

    container->setGeometry(50, 50, 200, 200);

    root.show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
}



void tst_QWindowContainer::testPositionAndSize()
{
    QWindow *window = new QWindow();
    window->setGeometry(300, 400, 500, 600);

    QWidget *container = QWidget::createWindowContainer(window);
    container->setGeometry(50, 50, 200, 200);


    container->show();
    QVERIFY(QTest::qWaitForWindowExposed(container));

    QCOMPARE(window->x(), 0);
    QCOMPARE(window->y(), 0);
    QCOMPARE(window->width(), container->width());
    QCOMPARE(window->height(), container->height());
}



void tst_QWindowContainer::testExposeObscure()
{
    Window *window = new Window();

    QWidget *container = QWidget::createWindowContainer(window);
    container->setGeometry(50, 50, 200, 200);

    container->show();
    QVERIFY(QTest::qWaitForWindowExposed(container));
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QVERIFY(window->numberOfExposes > 0);

    container->hide();

    QElapsedTimer timer;
    timer.start();
    while (window->numberOfObscures == 0 && timer.elapsed() < 5000) {
        QTest::qWait(10);
    }

    QVERIFY(window->numberOfObscures > 0);
}



void tst_QWindowContainer::testOwnership()
{
    QPointer<QWindow> window(new QWindow());
    QWidget *container = QWidget::createWindowContainer(window);

    delete container;

    QCOMPARE(window.data(), (QWindow *) 0);
}



void tst_QWindowContainer::testBehindTheScenesDeletion()
{
    QWindow *window = new QWindow();
    QWidget *container = QWidget::createWindowContainer(window);

    delete window;

    // The child got removed, showing not should not have any side effects,
    // such as for instance, crashing...
    container->show();
    QVERIFY(QTest::qWaitForWindowExposed(container));
    delete container;
}



void tst_QWindowContainer::testActivation()
{
    QWidget root;

    QWindow *window = new QWindow();
    QWidget *container = QWidget::createWindowContainer(window, &root);

    container->setGeometry(100, 100, 200, 100);
    root.setGeometry(100, 100, 400, 300);

    root.show();
    root.activateWindow();

    QVERIFY(QTest::qWaitForWindowActive(root.windowHandle()));
    QVERIFY(QGuiApplication::focusWindow() == root.windowHandle());

    // Verify that all states in the root widget indicate it is active
    QVERIFY(root.windowHandle()->isActive());
    QVERIFY(root.isActiveWindow());
    QCOMPARE(root.palette().currentColorGroup(), QPalette::Active);

    // Under KDE (ubuntu 12.10), we experience that doing two activateWindow in a row
    // does not work. The second gets ignored by the window manager, even though the
    // timestamp in the xcb connection is unique for both.
    if (QGuiApplication::platformName() == "xcb")
        QTest::qWait(100);

    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);

    // Verify that all states in the root widget still indicate it is active
    QVERIFY(root.windowHandle()->isActive());
    QVERIFY(root.isActiveWindow());
    QCOMPARE(root.palette().currentColorGroup(), QPalette::Active);
}



void tst_QWindowContainer::testUnparenting()
{
    QWindow *window = new QWindow();
    QWidget *container = QWidget::createWindowContainer(window);
    container->setGeometry(100, 100, 200, 100);

    window->setParent(0);

    container->show();

    QVERIFY(QTest::qWaitForWindowExposed(container));

    // Window should not be made visible by container..
    QVERIFY(!window->isVisible());
}

QTEST_MAIN(tst_QWindowContainer)

#include "tst_qwindowcontainer.moc"
