/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qwidgetbaselinetest.h"

#include <qbaselinetest.h>
#include <QApplication>
#include <QStyle>
#include <QStyleHints>
#include <QScreen>

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
    window = new QWidget;
    window->setWindowTitle(QTest::currentDataTag());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    window->setScreen(QGuiApplication::primaryScreen());
#endif
    window->move(QGuiApplication::primaryScreen()->availableGeometry().topLeft());

    doInit();
}

void QWidgetBaselineTest::cleanup()
{
    doCleanup();

    delete window;
    window = nullptr;
}

void QWidgetBaselineTest::makeVisible()
{
    Q_ASSERT(window);
    window->show();
    QApplication::setActiveWindow(window);
    QVERIFY(QTest::qWaitForWindowActive(window));
    // explicitly unset focus, the test needs to control when focus is shown
    if (window->focusWidget())
        window->focusWidget()->clearFocus();
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

/*!
    Sets standard widget properties on the test window and its children,
    and uploads snapshots. The widgets are returned in the same state
    that they had before.

    Call this helper after setting up the test window.
*/
void QWidgetBaselineTest::takeStandardSnapshots()
{
    makeVisible();
    struct PublicWidget : QWidget {
        bool focusNextPrevChild(bool next) override { return QWidget::focusNextPrevChild(next); }
    };

    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "default");

    // try hard to set focus
    static_cast<PublicWidget*>(window)->focusNextPrevChild(true);
    if (!window->focusWidget()) {
        QWidget *firstChild = window->findChild<QWidget*>();
        if (firstChild)
            firstChild->setFocus();
    }

    if (testWindow()->focusWidget()) {
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), "focused");
        testWindow()->focusWidget()->clearFocus();
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
