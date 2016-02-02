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

#include <qapplication.h>
#include <qwindow.h>
#include <qwidget.h>

#include <qdockwidget.h>
#include <qmainwindow.h>
#include <qscreen.h>
#include <qscopedpointer.h>


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

public:
    tst_QWindowContainer() : m_availableGeometry(QGuiApplication::primaryScreen()->availableGeometry()) {}

private slots:
    void testShow();
    void testPositionAndSize();
    void testExposeObscure();
    void testOwnership();
    void testBehindTheScenesDeletion();
    void testUnparenting();
    void testActivation();
    void testAncestorChange();
    void testDockWidget();
    void testNativeContainerParent();
    void cleanup();

private:
    const QRect m_availableGeometry;
};

void tst_QWindowContainer::cleanup()
{
    QVERIFY(QGuiApplication::topLevelWindows().isEmpty());
}

void tst_QWindowContainer::testShow()
{
    QWidget root;
    root.setWindowTitle(QTest::currentTestFunction());
    root.setGeometry(m_availableGeometry.x() + 100, m_availableGeometry.y() + 100, 400, 400);

    Window *window = new Window();
    QWidget *container = QWidget::createWindowContainer(window, &root);

    container->setGeometry(50, 50, 200, 200);

    root.show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
}



void tst_QWindowContainer::testPositionAndSize()
{
    QWindow *window = new QWindow();
    window->setGeometry(m_availableGeometry.x() + 300, m_availableGeometry.y() + 400, 500, 600);

    QScopedPointer<QWidget> container(QWidget::createWindowContainer(window));
    container->setWindowTitle(QTest::currentTestFunction());
    container->setGeometry(50, 50, 200, 200);


    container->show();
    QVERIFY(QTest::qWaitForWindowExposed(container.data()));

    QCOMPARE(window->x(), 0);
    QCOMPARE(window->y(), 0);
    QCOMPARE(window->width(), container->width());
    QCOMPARE(window->height(), container->height());
}



void tst_QWindowContainer::testExposeObscure()
{
    Window *window = new Window();

    QScopedPointer<QWidget> container(QWidget::createWindowContainer(window));
    container->setWindowTitle(QTest::currentTestFunction());
    container->setGeometry(m_availableGeometry.x() + 50, m_availableGeometry.y() + 50, 200, 200);

    container->show();
    QVERIFY(QTest::qWaitForWindowExposed(container.data()));
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
    root.setWindowTitle(QTest::currentTestFunction());

    QWindow *window = new QWindow();
    QWidget *container = QWidget::createWindowContainer(window, &root);

    container->setGeometry(100, 100, 200, 100);
    root.setGeometry(m_availableGeometry.x() + 100, m_availableGeometry.y() + 100, 400, 300);

    root.show();
    root.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&root));

    QVERIFY(QTest::qWaitForWindowActive(root.windowHandle()));
    QCOMPARE(QGuiApplication::focusWindow(), root.windowHandle());

    // Verify that all states in the root widget indicate it is active
    QVERIFY(root.windowHandle()->isActive());
    QVERIFY(root.isActiveWindow());
    QCOMPARE(root.palette().currentColorGroup(), QPalette::Active);

    // Under KDE (ubuntu 12.10), we experience that doing two activateWindow in a row
    // does not work. The second gets ignored by the window manager, even though the
    // timestamp in the xcb connection is unique for both.
    if (!QGuiApplication::platformName().compare(QLatin1String("xcb"), Qt::CaseInsensitive))
        QTest::qWait(100);

    window->requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), window);

    // Verify that all states in the root widget still indicate it is active
    QVERIFY(root.windowHandle()->isActive());
    QVERIFY(root.isActiveWindow());
    QCOMPARE(root.palette().currentColorGroup(), QPalette::Active);
}



void tst_QWindowContainer::testUnparenting()
{
    QWindow *window = new QWindow();
    QScopedPointer<QWidget> container(QWidget::createWindowContainer(window));
    container->setWindowTitle(QTest::currentTestFunction());
    container->setGeometry(m_availableGeometry.x() + 100, m_availableGeometry.y() + 100, 200, 100);

    window->setParent(0);

    container->show();

    QVERIFY(QTest::qWaitForWindowExposed(container.data()));

    // Window should not be made visible by container..
    QVERIFY(!window->isVisible());
}

void tst_QWindowContainer::testAncestorChange()
{
    QWidget root;
    root.setWindowTitle(QStringLiteral("Root ") + QTest::currentTestFunction());
    QWidget *left = new QWidget(&root);
    QWidget *right = new QWidget(&root);


    root.setGeometry(m_availableGeometry.x() + 50, m_availableGeometry.y() + 50, 200, 100);
    left->setGeometry(0, 0, 100, 100);
    right->setGeometry(100, 0, 100, 100);

    QWindow *window = new QWindow();
    QWidget *container = QWidget::createWindowContainer(window, left);
    container->setGeometry(0, 0, 100, 100);

    //      Root
    //      + left
    //      | + container
    //      |   + window
    //      + right
    root.show();
    QVERIFY(QTest::qWaitForWindowExposed(&root));
    QCOMPARE(window->geometry(), QRect(0, 0, 100, 100));

    container->setParent(right);
    //      Root
    //      + left
    //      + right
    //        + container
    //          + window
    QCOMPARE(window->geometry(), QRect(100, 0, 100, 100));

    QWidget *newRoot = new QWidget(&root);
    newRoot->setWindowTitle(QStringLiteral("newRoot ") + QTest::currentTestFunction());
    newRoot->setGeometry(50, 50, 200, 200);
    right->setParent(newRoot);
    //      Root
    //      + left
    //      + newRoot
    //        + right
    //          + container
    //            + window
    QCOMPARE(window->geometry(), QRect(150, 50, 100, 100));
    newRoot->move(0, 0);
    QCOMPARE(window->geometry(), QRect(100, 0, 100, 100));

    newRoot->setParent(0);
    QScopedPointer<QWidget> newRootGuard(newRoot);
    newRoot->setGeometry(m_availableGeometry.x() + 100, m_availableGeometry.y() + 100, 200, 200);
    newRoot->show();
    QVERIFY(QTest::qWaitForWindowExposed(newRoot));
    QCOMPARE(newRoot->windowHandle(), window->parent());
    //      newRoot
    //      + right
    //        + container
    //          + window
    QCOMPARE(window->geometry(), QRect(100, 0, 100, 100));
}


void tst_QWindowContainer::testDockWidget()
{
    QMainWindow mainWindow;
    mainWindow.setWindowTitle(QTest::currentTestFunction());
    mainWindow.resize(200, 200);
    mainWindow.move(m_availableGeometry.center() - QPoint(100, 100));

    QDockWidget *dock = new QDockWidget(QStringLiteral("Dock ") + QTest::currentTestFunction());
    QWindow *window = new QWindow();
    QWidget *container = QWidget::createWindowContainer(window);
    dock->setWidget(container);
    mainWindow.addDockWidget(Qt::RightDockWidgetArea, dock);

    mainWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));
    QCOMPARE(window->parent(), mainWindow.window()->windowHandle());

    QTest::qWait(1000);
    dock->setFloating(true);
    QTRY_VERIFY(window->parent() != mainWindow.window()->windowHandle());

    QTest::qWait(1000);
    dock->setFloating(false);
    QTRY_COMPARE(window->parent(), mainWindow.window()->windowHandle());
}

void tst_QWindowContainer::testNativeContainerParent()
{
    QWidget root;
    root.setWindowTitle(QTest::currentTestFunction());
    root.setGeometry(m_availableGeometry.x() + 50, m_availableGeometry.y() + 50, 200, 200);

    Window *window = new Window();
    QWidget *container = QWidget::createWindowContainer(window, &root);
    container->setAttribute(Qt::WA_NativeWindow);
    container->setGeometry(50, 50, 150, 150);

    root.show();

    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTRY_COMPARE(window->parent(), container->windowHandle());
}

QTEST_MAIN(tst_QWindowContainer)

#include "tst_qwindowcontainer.moc"
