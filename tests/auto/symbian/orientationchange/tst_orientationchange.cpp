/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifdef Q_OS_SYMBIAN

#include <eikenv.h>
#include <aknappui.h>
#include <private/qcore_symbian_p.h>
#include <QDesktopWidget>

class tst_orientationchange : public QObject
{
    Q_OBJECT
public:
    tst_orientationchange(){};
    ~tst_orientationchange(){};

private slots:
    void resizeEventOnOrientationChange();
};

class TestWidget : public QWidget
{
public:
    TestWidget(QWidget *parent = 0);

    void reset();
public:
    void resizeEvent(QResizeEvent *event);

public:
    QSize resizeEventSize;
    int resizeEventCount;
};

TestWidget::TestWidget(QWidget *parent)
: QWidget(parent)
{
    reset();
}

void TestWidget::reset()
{
    resizeEventSize = QSize();
    resizeEventCount = 0;
}

void TestWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Size delivered in first resize event is stored.
    if (!resizeEventCount)
        resizeEventSize = event->size();

    resizeEventCount++;
}

void tst_orientationchange::resizeEventOnOrientationChange()
{
    // This will test that when orientation 'changes', then
    // at most one resize event is generated.

    TestWidget *normalWidget = new TestWidget();
    TestWidget *fullScreenWidget = new TestWidget();
    TestWidget *maximizedWidget = new TestWidget();

    fullScreenWidget->showFullScreen();
    maximizedWidget->showMaximized();
    normalWidget->show();

    QCoreApplication::sendPostedEvents();
    QCoreApplication::sendPostedEvents();

    QCOMPARE(fullScreenWidget->resizeEventCount, 1);
    QCOMPARE(fullScreenWidget->size(), fullScreenWidget->resizeEventSize);
    QCOMPARE(maximizedWidget->resizeEventCount, 1);
    QCOMPARE(maximizedWidget->size(), maximizedWidget->resizeEventSize);
    QCOMPARE(normalWidget->resizeEventCount, 1);
    QCOMPARE(normalWidget->size(), normalWidget->resizeEventSize);

    fullScreenWidget->reset();
    maximizedWidget->reset();
    normalWidget->reset();

    // Assumes that Qt application is AVKON application.
    CAknAppUi *appUi = static_cast<CAknAppUi*>(CEikonEnv::Static()->EikAppUi());

    // Determine 'opposite' orientation to the current orientation.

    CAknAppUi::TAppUiOrientation orientation = CAknAppUi::EAppUiOrientationLandscape;
    if (fullScreenWidget->size().width() > fullScreenWidget->size().height()) {
        orientation = CAknAppUi::EAppUiOrientationPortrait;
    }

    TRAPD(err, appUi->SetOrientationL(orientation));

    QCoreApplication::sendPostedEvents();
    QCoreApplication::sendPostedEvents();

    // setOrientationL is not guaranteed to change orientation
    // (if emulator configured to support just portrait or landscape, then
    //  setOrientationL call shouldn't do anything).
    // So let's ensure that we do not get resize event twice.

    QVERIFY(fullScreenWidget->resizeEventCount <= 1);
    if (fullScreenWidget->resizeEventCount) {
        QCOMPARE(fullScreenWidget->size(), fullScreenWidget->resizeEventSize);
    }
    QVERIFY(maximizedWidget->resizeEventCount <= 1);
    if (fullScreenWidget->resizeEventCount) {
        QCOMPARE(maximizedWidget->size(), maximizedWidget->resizeEventSize);
    }
    QCOMPARE(normalWidget->resizeEventCount, 0);

    QDesktopWidget desktop;
    QRect qtAvail = desktop.availableGeometry(normalWidget);
    TRect clientRect = static_cast<CEikAppUi*>(CCoeEnv::Static()-> AppUi())->ClientRect();
    QRect symbianAvail = qt_TRect2QRect(clientRect);
    QCOMPARE(qtAvail, symbianAvail);

    // Switch orientation back to original
    orientation = orientation == CAknAppUi::EAppUiOrientationPortrait
                                 ? CAknAppUi::EAppUiOrientationLandscape
                                 : CAknAppUi::EAppUiOrientationPortrait;


    fullScreenWidget->reset();
    maximizedWidget->reset();
    normalWidget->reset();

    TRAP(err, appUi->SetOrientationL(orientation));

    QCoreApplication::sendPostedEvents();
    QCoreApplication::sendPostedEvents();

    // setOrientationL is not guaranteed to change orientation
    // (if emulator configured to support just portrait or landscape, then
    //  setOrientationL call shouldn't do anything).
    // So let's ensure that we do not get resize event twice.

    QVERIFY(fullScreenWidget->resizeEventCount <= 1);
    if (fullScreenWidget->resizeEventCount) {
        QCOMPARE(fullScreenWidget->size(), fullScreenWidget->resizeEventSize);
    }
    QVERIFY(maximizedWidget->resizeEventCount <= 1);
    if (fullScreenWidget->resizeEventCount) {
        QCOMPARE(maximizedWidget->size(), maximizedWidget->resizeEventSize);
    }
    QCOMPARE(normalWidget->resizeEventCount, 0);

    qtAvail = desktop.availableGeometry(normalWidget);
    clientRect = static_cast<CEikAppUi*>(CCoeEnv::Static()-> AppUi())->ClientRect();
    symbianAvail = qt_TRect2QRect(clientRect);
    QCOMPARE(qtAvail, symbianAvail);

    TRAP(err, appUi->SetOrientationL(CAknAppUi::EAppUiOrientationUnspecified));

    delete normalWidget;
    delete fullScreenWidget;
    delete maximizedWidget;
}

QTEST_MAIN(tst_orientationchange)
#include "tst_orientationchange.moc"
#else
QTEST_NOOP_MAIN
#endif
