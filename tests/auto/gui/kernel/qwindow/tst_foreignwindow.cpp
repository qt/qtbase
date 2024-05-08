// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qloggingcategory.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include "../../../../shared/nativewindow.h"

class tst_ForeignWindow: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        auto *platformIntegration = QGuiApplicationPrivate::platformIntegration();
        if (!platformIntegration->hasCapability(QPlatformIntegration::ForeignWindows))
            QSKIP("This platform does not support foreign windows");
    }

    void fromWinId();
    void initialState();

    void embedForeignWindow();
    void embedInForeignWindow();

    void destroyExplicitly();
    void destroyWhenParentIsDestroyed();
};

void tst_ForeignWindow::fromWinId()
{
    NativeWindow nativeWindow;
    QVERIFY(nativeWindow);

    std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(nativeWindow));
    QVERIFY(foreignWindow);
    QVERIFY(foreignWindow->flags().testFlag(Qt::ForeignWindow));
    QVERIFY(foreignWindow->handle());

    // fromWinId does not take (exclusive) ownership of the native window,
    // so deleting the foreign window should not be a problem/cause crashes.
    foreignWindow.reset();
}

void tst_ForeignWindow::initialState()
{
    NativeWindow nativeWindow;
    QVERIFY(nativeWindow);

    // A foreign window can be used to embed a Qt UI in a foreign window hierarchy,
    // in which case the foreign window merely acts as a parent and should not be
    // modified, or to embed a foreign window in a Qt UI, in which case the foreign
    // window must to be able to re-parent, move, resize, show, etc, so that the
    // containing Qt UI can treat it as any other window.

    // At the point of creation though, we don't know what the foreign window
    // will be used for, so the platform should not assume it can modify the
    // window. Any properties set on the native window should persist past
    // creation of the foreign window.

    const QRect initialGeometry(123, 456, 321, 654);
    nativeWindow.setGeometry(initialGeometry);
    QTRY_COMPARE(nativeWindow.geometry(), initialGeometry);

    std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(nativeWindow));
    QCOMPARE(nativeWindow.geometry(), initialGeometry);

    // For extra bonus points, the foreign window should actually
    // reflect the state of the native window.
    QCOMPARE(foreignWindow->geometry(), initialGeometry);
}

void tst_ForeignWindow::embedForeignWindow()
{
    // A foreign window embedded into a Qt UI requires that the rest of Qt
    // is to be able to treat the foreign child window as any other window
    // that it can show, hide, stack, and move around.

    QWindow parentWindow;

    NativeWindow nativeWindow;
    QVERIFY(nativeWindow);

    // As a prerequisite to that, we must be able to reparent the foreign window
    std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(nativeWindow));
    foreignWindow->setParent(&parentWindow);
    QTRY_COMPARE(nativeWindow.parentWinId(), parentWindow.winId());

    // FIXME: This test is flakey on Linux. Figure out why
#if !defined(Q_OS_LINUX)
    foreignWindow->setParent(nullptr);
    QTRY_VERIFY(nativeWindow.parentWinId() != parentWindow.winId());
#endif
}

void tst_ForeignWindow::embedInForeignWindow()
{
    // When a foreign window is used as a container to embed a Qt UI
    // in a foreign window hierarchy, the foreign window merely acts
    // as a parent, and should not be modified.

    {
        // At a minimum, we must be able to reparent into the window
        NativeWindow nativeWindow;
        QVERIFY(nativeWindow);

        std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(nativeWindow));

        QWindow embeddedWindow;
        embeddedWindow.setParent(foreignWindow.get());
        QTRY_VERIFY(nativeWindow.isParentOf(embeddedWindow.winId()));
    }

    {
        // The foreign window's native window should not be reparent as a
        // result of creating the foreign window, adding and removing children,
        // or destroying the foreign window.

        NativeWindow topLevelNativeWindow;
        NativeWindow childNativeWindow;
        childNativeWindow.setParent(topLevelNativeWindow);
        QVERIFY(topLevelNativeWindow.isParentOf(childNativeWindow));

        std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(childNativeWindow));
        QVERIFY(topLevelNativeWindow.isParentOf(childNativeWindow));

        QWindow embeddedWindow;
        embeddedWindow.setParent(foreignWindow.get());
        QTRY_VERIFY(childNativeWindow.isParentOf(embeddedWindow.winId()));
        QVERIFY(topLevelNativeWindow.isParentOf(childNativeWindow));

        embeddedWindow.setParent(nullptr);
        QVERIFY(topLevelNativeWindow.isParentOf(childNativeWindow));

        foreignWindow.reset();
        QVERIFY(topLevelNativeWindow.isParentOf(childNativeWindow));
    }
}

void tst_ForeignWindow::destroyExplicitly()
{
    NativeWindow nativeWindow;
    QVERIFY(nativeWindow);

    std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(nativeWindow));
    QVERIFY(foreignWindow->handle());

    // Explicitly destroying a foreign window is a no-op, as
    // the documentation claims that it "releases the native
    // platform resources associated with this window.", which
    // is not technically true for foreign windows.
    auto *windowHandleBeforeDestroy = foreignWindow->handle();
    foreignWindow->destroy();
    QCOMPARE(foreignWindow->handle(), windowHandleBeforeDestroy);
}

void tst_ForeignWindow::destroyWhenParentIsDestroyed()
{
    QWindow parentWindow;

    NativeWindow nativeWindow;
    QVERIFY(nativeWindow);

    std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(nativeWindow));
    foreignWindow->setParent(&parentWindow);
    QTRY_COMPARE(nativeWindow.parentWinId(), parentWindow.winId());

    // Reparenting into a window will result in creating it
    QVERIFY(parentWindow.handle());

    parentWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parentWindow));

    // Destroying the parent window of the foreign window results
    // in destroying the foreign window as well, as the foreign
    // window no longer has a parent it can be embedded in.
    QVERIFY(foreignWindow->handle());
    parentWindow.destroy();
    QVERIFY(!foreignWindow->handle());

    // But the foreign window can be recreated again, and will
    // continue to be a native child of the parent window.
    foreignWindow->create();
    QVERIFY(foreignWindow->handle());
    QTRY_COMPARE(nativeWindow.parentWinId(), parentWindow.winId());

    parentWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parentWindow));
}

#include <tst_foreignwindow.moc>
QTEST_MAIN(tst_ForeignWindow)
