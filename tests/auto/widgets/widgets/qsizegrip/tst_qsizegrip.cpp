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
#include <QSizeGrip>
#include <QEvent>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QStatusBar>
#include <QMdiArea>
#include <QMdiSubWindow>

static inline Qt::Corner sizeGripCorner(QWidget *parent, QSizeGrip *sizeGrip)
{
    if (!parent || !sizeGrip)
        return Qt::TopLeftCorner;

    const QPoint sizeGripPos = sizeGrip->mapTo(parent, QPoint(0, 0));
    bool isAtBottom = sizeGripPos.y() >= parent->height() / 2;
    bool isAtLeft = sizeGripPos.x() <= parent->width() / 2;
    if (isAtLeft)
        return isAtBottom ? Qt::BottomLeftCorner : Qt::TopLeftCorner;
    else
        return isAtBottom ? Qt::BottomRightCorner : Qt::TopRightCorner;

}

Q_DECLARE_METATYPE(Qt::WindowType);

class tst_QSizeGrip : public QObject
{
    Q_OBJECT
public slots:
    void cleanup();

private slots:
    void hideAndShowOnWindowStateChange_data();
    void hideAndShowOnWindowStateChange();
    void orientation();
    void dontCrashOnTLWChange();
};

class TestWidget : public QWidget
{
public:
    using QWidget::QWidget;

    QSize sizeHint() const override { return QSize(300, 200); }
    void changeEvent(QEvent *event) override
    {
        QWidget::changeEvent(event);
        if (isWindow() && event->type() == QEvent::WindowStateChange)
            QVERIFY(QTest::qWaitForWindowExposed(this));
    }
};

void tst_QSizeGrip::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QSizeGrip::hideAndShowOnWindowStateChange_data()
{
    QTest::addColumn<Qt::WindowType>("windowType");
    QTest::newRow("Qt::Window") << Qt::Window;
    QTest::newRow("Qt::SubWindow") << Qt::SubWindow;
}

void tst_QSizeGrip::hideAndShowOnWindowStateChange()
{
    QFETCH(Qt::WindowType, windowType);
#ifdef Q_OS_WINRT
    QSKIP("Broken on WinRT - QTBUG-68297");
#endif

    QScopedPointer<QWidget> parentWidget;
    if (windowType != Qt::Window)
        parentWidget.reset(new QWidget);
    QScopedPointer<TestWidget> widget(new TestWidget(parentWidget.data(), Qt::WindowFlags(windowType)));
    QSizeGrip *sizeGrip = new QSizeGrip(widget.data());

    // Normal.
    if (parentWidget)
        parentWidget->show();
    else
        widget->show();
    QTRY_VERIFY(sizeGrip->isVisible());

    widget->showFullScreen();
    QTRY_VERIFY(!sizeGrip->isVisible());

    widget->showNormal();
    QTRY_VERIFY(sizeGrip->isVisible());

    widget->showMaximized();
#ifndef Q_OS_MAC
    QTRY_VERIFY(!sizeGrip->isVisible());
#else
    QEXPECT_FAIL("", "QTBUG-23681", Abort);
    QVERIFY(sizeGrip->isVisible());
#endif

    widget->showNormal();
    QTRY_VERIFY(sizeGrip->isVisible());

    sizeGrip->hide();
    QTRY_VERIFY(!sizeGrip->isVisible());

    widget->showFullScreen();
    widget->showNormal();
    QTRY_VERIFY(!sizeGrip->isVisible());
    widget->showMaximized();
    widget->showNormal();
    QTRY_VERIFY(!sizeGrip->isVisible());
}

void tst_QSizeGrip::orientation()
{
#ifdef Q_OS_WINRT
    QSKIP("Broken on WinRT - QTBUG-68297");
#endif

    TestWidget widget;
    widget.setLayout(new QVBoxLayout);
    QSizeGrip *sizeGrip = new QSizeGrip(&widget);
    sizeGrip->setFixedSize(sizeGrip->sizeHint());
    widget.layout()->addWidget(sizeGrip);
    widget.layout()->setAlignment(sizeGrip, Qt::AlignBottom | Qt::AlignRight);

    widget.show();
    QCOMPARE(sizeGripCorner(&widget, sizeGrip), Qt::BottomRightCorner);

    widget.setLayoutDirection(Qt::RightToLeft);
    qApp->processEvents();
    QCOMPARE(sizeGripCorner(&widget, sizeGrip), Qt::BottomLeftCorner);

    widget.unsetLayoutDirection();
    qApp->processEvents();
    QCOMPARE(sizeGripCorner(&widget, sizeGrip), Qt::BottomRightCorner);

    widget.layout()->setAlignment(sizeGrip, Qt::AlignTop | Qt::AlignRight);
    qApp->processEvents();
    QCOMPARE(sizeGripCorner(&widget, sizeGrip), Qt::TopRightCorner);

    widget.setLayoutDirection(Qt::RightToLeft);
    qApp->processEvents();
    QCOMPARE(sizeGripCorner(&widget, sizeGrip), Qt::TopLeftCorner);

    widget.unsetLayoutDirection();
    qApp->processEvents();
    QCOMPARE(sizeGripCorner(&widget, sizeGrip), Qt::TopRightCorner);
}

void tst_QSizeGrip::dontCrashOnTLWChange()
{
    // QTBUG-22867
    QMdiArea mdiArea;
    mdiArea.show();

    QScopedPointer<QMainWindow> mw(new QMainWindow);
    QMdiSubWindow *mdi = mdiArea.addSubWindow(mw.data());
    mw->statusBar()->setSizeGripEnabled(true);
    mdiArea.removeSubWindow(mw.data());
    delete mdi;
    mw->show();

    // the above setup causes a change of TLW for the size grip,
    // and it must not crash.
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Broken on WinRT - QTBUG-68297", Abort);
#endif
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));
    QVERIFY(QTest::qWaitForWindowExposed(mw.data()));
}

QTEST_MAIN(tst_QSizeGrip)
#include "tst_qsizegrip.moc"

