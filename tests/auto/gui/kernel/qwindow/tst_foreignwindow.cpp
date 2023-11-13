// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    // Top level windows may not have 0 as their winId, e.g. on
    // XCB the root window of the screen is used.
    const auto originalParentWinId = nativeWindow.parentWinId();

    // As a prerequisite to that, we must be able to reparent the foreign window
    std::unique_ptr<QWindow> foreignWindow(QWindow::fromWinId(nativeWindow));
    foreignWindow->setParent(&parentWindow);
    QTRY_COMPARE(nativeWindow.parentWinId(), parentWindow.winId());

    foreignWindow->setParent(nullptr);
    QTRY_COMPARE(nativeWindow.parentWinId(), originalParentWinId);
}

#include <tst_foreignwindow.moc>
QTEST_MAIN(tst_ForeignWindow)
