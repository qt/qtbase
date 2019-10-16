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
#include <QtWidgets/QDesktopWidget>
#include <QtGui/QWindow>
#include <QDebug>

// the complete class is deprecated
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
class tst_QDesktopWidget : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

#if QT_DEPRECATED_SINCE(5, 11)
    void numScreens();
    void primaryScreen();
    void screenNumberForQPoint();
#endif
    void screenNumberForQWidget();
    void availableGeometry();
    void screenGeometry();
    void topLevels();
};

void tst_QDesktopWidget::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

#if QT_DEPRECATED_SINCE(5, 11)
void tst_QDesktopWidget::numScreens()
{
    QDesktopWidget desktop;
    QVERIFY(desktop.numScreens() > 0);
}

void tst_QDesktopWidget::primaryScreen()
{
    QDesktopWidget desktop;
    QVERIFY(desktop.primaryScreen() >= 0);
    QVERIFY(desktop.primaryScreen() < desktop.numScreens());
}
#endif

void tst_QDesktopWidget::availableGeometry()
{
    QDesktopWidget desktop;
    QTest::ignoreMessage(QtWarningMsg, "QDesktopWidget::availableGeometry(): Attempt "
                                       "to get the available geometry of a null widget");
    QRect r = desktop.availableGeometry(nullptr);
    QVERIFY(r.isNull());

#if QT_DEPRECATED_SINCE(5, 11)
    QRect total;
    QRect available;

    for (int i = 0; i < desktop.screenCount(); ++i) {
        total = desktop.screenGeometry(i);
        available = desktop.availableGeometry(i);
        QVERIFY(total.contains(available));
    }

    total = desktop.screenGeometry();
    available = desktop.availableGeometry();
    QVERIFY(total.contains(available));
    QCOMPARE(desktop.availableGeometry(desktop.primaryScreen()), available);
    QCOMPARE(desktop.screenGeometry(desktop.primaryScreen()), total);
#endif
}

void tst_QDesktopWidget::screenNumberForQWidget()
{
    QDesktopWidget desktop;

    QCOMPARE(desktop.screenNumber(nullptr), 0);

    QWidget widget;
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QVERIFY(widget.isVisible());

    int widgetScreen = desktop.screenNumber(&widget);
    QVERIFY(widgetScreen > -1);
    QVERIFY(widgetScreen < QGuiApplication::screens().size());
}

#if QT_DEPRECATED_SINCE(5, 11)
void tst_QDesktopWidget::screenNumberForQPoint()
{
    // make sure QDesktopWidget::screenNumber(QPoint) returns the correct screen
    QDesktopWidget *desktopWidget = QApplication::desktop();
    QRect allScreens;
    for (int i = 0; i < desktopWidget->numScreens(); ++i) {
        QRect screenGeometry = desktopWidget->screenGeometry(i);
        QCOMPARE(desktopWidget->screenNumber(screenGeometry.center()), i);
        allScreens |= screenGeometry;
    }

    // make sure QDesktopWidget::screenNumber(QPoint) returns a valid screen for points that aren't on any screen
    int screen;
    screen = desktopWidget->screenNumber(allScreens.topLeft() - QPoint(1, 1));

    QVERIFY(screen >= 0 && screen < desktopWidget->numScreens());
    screen = desktopWidget->screenNumber(allScreens.topRight() + QPoint(1, 1));
    QVERIFY(screen >= 0 && screen < desktopWidget->numScreens());
    screen = desktopWidget->screenNumber(allScreens.bottomLeft() - QPoint(1, 1));
    QVERIFY(screen >= 0 && screen < desktopWidget->numScreens());
    screen = desktopWidget->screenNumber(allScreens.bottomRight() + QPoint(1, 1));
    QVERIFY(screen >= 0 && screen < desktopWidget->numScreens());
}
#endif

void tst_QDesktopWidget::screenGeometry()
{
    QDesktopWidget *desktopWidget = QApplication::desktop();
    QTest::ignoreMessage(QtWarningMsg, "QDesktopWidget::screenGeometry(): Attempt "
                                       "to get the screen geometry of a null widget");
    QRect r = desktopWidget->screenGeometry(nullptr);
    QVERIFY(r.isNull());
    QWidget widget;
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    r = desktopWidget->screenGeometry(&widget);

#if QT_DEPRECATED_SINCE(5, 11)
    QRect total;
    QRect available;
    for (int i = 0; i < desktopWidget->screenCount(); ++i) {
        total = desktopWidget->screenGeometry(i);
        available = desktopWidget->availableGeometry(i);
    }
#endif
}

void tst_QDesktopWidget::topLevels()
{
    // Desktop widgets/windows should not be listed as top-levels.
    int topLevelDesktopWidgets = 0;
    int topLevelDesktopWindows = 0;
    foreach (const QWidget *w, QApplication::topLevelWidgets())
        if (w->windowType() == Qt::Desktop)
            topLevelDesktopWidgets++;
    foreach (const QWindow *w, QGuiApplication::topLevelWindows())
        if (w->type() == Qt::Desktop)
            topLevelDesktopWindows++;
    QCOMPARE(topLevelDesktopWidgets, 0);
    QCOMPARE(topLevelDesktopWindows, 0);
}
QT_WARNING_POP

QTEST_MAIN(tst_QDesktopWidget)
#include "tst_qdesktopwidget.moc"
