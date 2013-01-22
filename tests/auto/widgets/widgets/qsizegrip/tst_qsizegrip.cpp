/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include <QSizeGrip>
#include <QEvent>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>

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
    void initTestCase();
    void cleanupTestCase();

private slots:
    void hideAndShowOnWindowStateChange_data();
    void hideAndShowOnWindowStateChange();
    void orientation();

private:
    QLineEdit *dummyWidget;
};

class TestWidget : public QWidget
{
public:
    TestWidget(QWidget *parent = 0, Qt::WindowFlags flags = 0) : QWidget(parent, flags) {}
    QSize sizeHint() const { return QSize(300, 200); }
    void changeEvent(QEvent *event)
    {
        QWidget::changeEvent(event);
        if (isWindow() && event->type() == QEvent::WindowStateChange)
            QVERIFY(QTest::qWaitForWindowExposed(this));
    }
};

void tst_QSizeGrip::initTestCase()
{
    dummyWidget = new QLineEdit;
    dummyWidget->show();
}

void tst_QSizeGrip::cleanupTestCase()
{
    delete dummyWidget;
    dummyWidget = 0;
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

    QWidget *parentWidget = windowType == Qt::Window ?  0 : new QWidget;
    TestWidget *widget = new TestWidget(parentWidget, Qt::WindowFlags(windowType));
    QSizeGrip *sizeGrip = new QSizeGrip(widget);

    // Normal.
    if (parentWidget)
        parentWidget->show();
    else
        widget->show();
    QVERIFY(sizeGrip->isVisible());

    widget->showFullScreen();
    QVERIFY(!sizeGrip->isVisible());

    widget->showNormal();
    QVERIFY(sizeGrip->isVisible());

    widget->showMaximized();
#ifndef Q_OS_MAC
    QVERIFY(!sizeGrip->isVisible());
#else
    QEXPECT_FAIL("", "QTBUG-23681", Abort);
    QVERIFY(sizeGrip->isVisible());
#endif

    widget->showNormal();
    QVERIFY(sizeGrip->isVisible());

    sizeGrip->hide();
    QVERIFY(!sizeGrip->isVisible());

    widget->showFullScreen();
    widget->showNormal();
    QVERIFY(!sizeGrip->isVisible());
    widget->showMaximized();
    widget->showNormal();
    QVERIFY(!sizeGrip->isVisible());

    delete widget;
    delete parentWidget;
}

void tst_QSizeGrip::orientation()
{
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

QTEST_MAIN(tst_QSizeGrip)
#include "tst_qsizegrip.moc"

