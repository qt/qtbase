// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwidgetbaselinetest.h"

#include <qbaselinetest.h>
#include <QApplication>
#include <QStyle>
#include <QStyleHints>
#include <QScreen>

#include <QtWidgets/private/qapplication_p.h>

QT_BEGIN_NAMESPACE

QWidgetBaselineTest::QWidgetBaselineTest()
{
    QBaselineTest::setProject("Widgets");

    // Set key platform properties that are relevant for the appearance of widgets
    const QString platformName = QGuiApplication::platformName() + "-" + QSysInfo::productType();
    QBaselineTest::addClientProperty("PlatformName", platformName);
    QBaselineTest::addClientProperty("OSVersion", QSysInfo::productVersion());

    // Encode a number of parameters that impact the UI
    QPalette palette;
    QFont font;
    const QString styleName =
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QApplication::style()->metaObject()->className();
#else
            QApplication::style()->name();
#endif
    // turn off animations and make the cursor flash time really long to avoid blinking
    QApplication::style()->setProperty("_qt_animation_time", QTime());
    QGuiApplication::styleHints()->setCursorFlashTime(50000);

    QByteArray appearanceBytes;
    {
        QDataStream appearanceStream(&appearanceBytes, QIODevice::WriteOnly);
        appearanceStream << palette << font;
        const qreal screenDpr = QApplication::primaryScreen()->devicePixelRatio();
        if (screenDpr != 1.0) {
            qWarning() << "DPR is" << screenDpr << "- images will not be compared to 1.0 baseline!";
            appearanceStream << screenDpr;
        }
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const quint16 appearanceId = qChecksum(appearanceBytes, appearanceBytes.size());
#else
    const quint16 appearanceId = qChecksum(appearanceBytes);
#endif

    // Assume that text that's darker than the background means we run in light mode
    // This results in a more meaningful appearance ID between different runs than
    // just the checksum of the various attributes.
    const QColor windowColor = palette.window().color();
    const QColor textColor = palette.text().color();
    const QString appearanceIdString = (windowColor.value() > textColor.value()
                                        ? QString("light-%1-%2") : QString("dark-%1-%2"))
                                       .arg(styleName).arg(appearanceId, 0, 16);
    QBaselineTest::addClientProperty("AppearanceID", appearanceIdString);

    // let users know where they can find the results
    qDebug() << "PlatformName computed to be:" << platformName;
    qDebug() << "Appearance ID computed as:" << appearanceIdString;
}

void QWidgetBaselineTest::initTestCase()
 {
    // Check and setup the environment. Failure to do so skips the test.
    QByteArray msg;
    if (!QBaselineTest::connectToBaselineServer(&msg))
        QSKIP(msg);
}

void QWidgetBaselineTest::init()
{
    QVERIFY(!window);
    background = new QWidget(nullptr, Qt::FramelessWindowHint);
    window = new QWidget(background, Qt::Window);
    window->setWindowTitle(QTest::currentDataTag());
    window->setFocusPolicy(Qt::StrongFocus);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    background->setScreen(QGuiApplication::primaryScreen());
    window->setScreen(QGuiApplication::primaryScreen());
#endif
    background->move(QGuiApplication::primaryScreen()->availableGeometry().topLeft());
    window->move(QGuiApplication::primaryScreen()->availableGeometry().topLeft());

    doInit();
}

void QWidgetBaselineTest::cleanup()
{
    doCleanup();

    delete background;
    background = nullptr;
    window = nullptr;
}

void QWidgetBaselineTest::makeVisible()
{
    Q_ASSERT(window);
    background->showMaximized();
    window->show();
    QApplicationPrivate::setActiveWindow(window);
    QVERIFY(QTest::qWaitForWindowActive(window));
    // explicitly set focus on the window so that the test widget doesn't have it
    window->setFocus(Qt::OtherFocusReason);
    QTRY_COMPARE(window->focusWidget(), window);
}

/*
    Grabs the test window and returns the resulting QImage, without
    compensating for DPR differences.
*/
QImage QWidgetBaselineTest::takeSnapshot()
{
    // make sure all effects are done
    QTest::qWait(250);
    return window->grab().toImage();
}

/*
    Grabs the test window screen and returns the resulting QImage, without
    compensating for DPR differences.
    This can be used for popup windows.
*/
QImage QWidgetBaselineTest::takeScreenSnapshot(const QRect& windowRect)
{
    // make sure all effects are done - wait longer here because entire
    // windows might be fading in and out.
    QTest::qWait(750);
    return window->screen()->grabWindow(0, windowRect.x(), windowRect.y(),
                                        windowRect.width(), windowRect.height()).toImage();
}

/*!
    Sets standard widget properties on the test window and its children,
    and uploads snapshots. The widgets are returned in the same state
    that they had before.

    Call this helper after setting up the test window.
*/
void QWidgetBaselineTest::takeStandardSnapshots()
{
    makeVisible();

    QWidget *oldFocusWidget = testWindow()->focusWidget();
    QCOMPARE(oldFocusWidget, testWindow());
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "default");

    // try hard to set focus
    QWidget *testWidget = window->nextInFocusChain();
    if (!testWidget)
        testWidget = window->findChild<QWidget*>();
    QVERIFY(testWidget);
    // use TabFocusReason, some widgets handle that specifically to e.g. select
    testWidget->setFocus(Qt::TabFocusReason);

    if (testWindow()->focusWidget() != oldFocusWidget) {
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), "focused");
        // set focus back
        oldFocusWidget->setFocus(Qt::OtherFocusReason);
    } else {
        qWarning() << "Couldn't set focus on tested widget" << testWidget;
    }

    // this disables all children
    window->setEnabled(false);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "disabled");
    window->setEnabled(true);

    // show and activate another window so that our test window becomes inactive
    QWidget otherWindow;
    otherWindow.move(window->geometry().bottomRight() + QPoint(10, 10));
    otherWindow.resize(50, 50);
    otherWindow.setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    otherWindow.show();
    otherWindow.windowHandle()->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&otherWindow));
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "inactive");

    window->windowHandle()->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    if (window->focusWidget())
        window->focusWidget()->clearFocus();
}

QT_END_NAMESPACE
