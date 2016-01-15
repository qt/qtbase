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
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qscreen.h>

class tst_QToolTip : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void task183679_data();
    void task183679();
    void whatsThis();
    void setPalette();
};

void tst_QToolTip::init()
{
    QVERIFY(!QToolTip::isVisible());
}

void tst_QToolTip::cleanup()
{
    QTRY_VERIFY(QApplication::topLevelWidgets().isEmpty());
}

class Widget_task183679 : public QWidget
{
    Q_OBJECT
public:
    Widget_task183679(QWidget *parent = 0) : QWidget(parent) {}

    void showDelayedToolTip(int msecs)
    {
        QTimer::singleShot(msecs, this, SLOT(showToolTip()));
    }

private slots:
    void showToolTip()
    {
        QToolTip::showText(mapToGlobal(QPoint(0, 0)), "tool tip text", this);
    }
};

Q_DECLARE_METATYPE(Qt::Key)

void tst_QToolTip::task183679_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<bool>("visible");

    QTest::newRow("non-modifier") << Qt::Key_A << true;
    QTest::newRow("Shift") << Qt::Key_Shift << true;
    QTest::newRow("Control") << Qt::Key_Control << true;
    QTest::newRow("Alt") << Qt::Key_Alt << true;
    QTest::newRow("Meta") << Qt::Key_Meta << true;
}

void tst_QToolTip::task183679()
{
    QFETCH(Qt::Key, key);
    QFETCH(bool, visible);

#ifdef Q_OS_MAC
    QSKIP("This test fails in the CI system, QTBUG-30040");
#endif

    Widget_task183679 widget;
    widget.move(QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(50, 50));
    // Ensure cursor is not over tooltip, which causes it to hide
#ifndef QT_NO_CURSOR
    QCursor::setPos(widget.geometry().topRight() + QPoint(-50, 50));
#endif
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction())
                          + QLatin1Char(' ') + QLatin1String(QTest::currentDataTag()));
    widget.show();
    QApplication::setActiveWindow(&widget);
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    widget.showDelayedToolTip(100);
    QTRY_VERIFY(QToolTip::isVisible());

    QTest::keyPress(&widget, key);

    // Important: the following delay must be larger than the duration of the timer potentially
    // initiated by the key press (currently 300 msecs), but smaller than the minimum
    // auto-close timeout (currently 10000 msecs)
    QTest::qWait(1500);

    QCOMPARE(QToolTip::isVisible(), visible);
    if (visible)
        QToolTip::hideText();
}

static QWidget *findWhatsThat()
{
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        if (widget->inherits("QWhatsThat"))
            return widget;
    }
    return Q_NULLPTR;
}

void tst_QToolTip::whatsThis()
{
    qApp->setStyleSheet( "QWidget { font-size: 72px; }" );
    QWhatsThis::showText(QPoint(0, 0), "This is text");

    QWidget *whatsthis = Q_NULLPTR;
    QTRY_VERIFY( (whatsthis = findWhatsThat()) );
    QVERIFY(whatsthis->isVisible());
    const int whatsThisHeight = whatsthis->height();
    qApp->setStyleSheet(QString());
    QWhatsThis::hideText();
    QVERIFY2(whatsThisHeight > 100, QByteArray::number(whatsThisHeight)); // Test QTBUG-2416
}


void tst_QToolTip::setPalette()
{
    //the previous test may still have a tooltip pending for deletion
    QVERIFY(!QToolTip::isVisible());

    QToolTip::showText(QPoint(), "tool tip text", 0);

    QTRY_VERIFY(QToolTip::isVisible());

    QWidget *toolTip = 0;
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        if (widget->windowType() == Qt::ToolTip
            && widget->objectName() == QLatin1String("qtooltip_label"))
        {
            toolTip = widget;
            break;
        }
    }

    QVERIFY(toolTip);
    QTRY_VERIFY(toolTip->isVisible());

    const QPalette oldPalette = toolTip->palette();
    QPalette newPalette = oldPalette;
    newPalette.setColor(QPalette::ToolTipBase, Qt::red);
    newPalette.setColor(QPalette::ToolTipText, Qt::blue);
    QToolTip::setPalette(newPalette);
    QCOMPARE(toolTip->palette(), newPalette);
    QToolTip::hideText();
}

QTEST_MAIN(tst_QToolTip)
#include "tst_qtooltip.moc"
