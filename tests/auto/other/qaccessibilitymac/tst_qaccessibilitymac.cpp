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

#include <QApplication>
#include <QtWidgets>
#include <QtTest/QtTest>
#include <QtCore/qcoreapplication.h>

#include "tst_qaccessibilitymac_helpers.h"

QT_USE_NAMESPACE


class AccessibleTestWindow : public QWidget
{
    Q_OBJECT
public:
    AccessibleTestWindow()
    {
        new QHBoxLayout(this);
    }

    void addWidget(QWidget* widget)
    {
        layout()->addWidget(widget);
        widget->show();
        QVERIFY(QTest::qWaitForWindowExposed(widget));
    }

    void clearChildren()
    {
        qDeleteAll(children());
        new QHBoxLayout(this);
    }
};

class tst_QAccessibilityMac : public QObject
{
Q_OBJECT
private slots:
    void init();
    void cleanup();

    void singleWidgetTest();
    void lineEditTest();
    void hierarchyTest();
    void notificationsTest();
    void checkBoxTest();

private:
    AccessibleTestWindow *m_window;
};


void tst_QAccessibilityMac::init()
{
    m_window = new AccessibleTestWindow();
    m_window->setWindowTitle("Test window");
    m_window->show();
    m_window->resize(400, 400);

    QVERIFY(QTest::qWaitForWindowExposed(m_window));
}

void tst_QAccessibilityMac::cleanup()
{
    delete m_window;
}

void tst_QAccessibilityMac::singleWidgetTest()
{
    delete m_window;
    m_window = 0;

    QVERIFY(singleWidget());
}

void tst_QAccessibilityMac::lineEditTest()
{
    QLineEdit *lineEdit = new QLineEdit(m_window);
    lineEdit->setText("a11y test QLineEdit");
    m_window->addWidget(lineEdit);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    QVERIFY(testLineEdit());
}

void tst_QAccessibilityMac::hierarchyTest()
{
    QWidget *w = new QWidget(m_window);
    m_window->addWidget(w);

    w->setLayout(new QVBoxLayout());
    QPushButton *b = new QPushButton(w);
    w->layout()->addWidget(b);
    b->setText("I am a button");

    QPushButton *b2 = new QPushButton(w);
    w->layout()->addWidget(b2);
    b2->setText("Button 2");

    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();
    QVERIFY(testHierarchy(w));
}

void tst_QAccessibilityMac::notificationsTest()
{
    QVERIFY(notifications(m_window));
}

void tst_QAccessibilityMac::checkBoxTest()
{
    QCheckBox *cb = new QCheckBox(m_window);
    cb->setText("Great option");
    m_window->addWidget(cb);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    QVERIFY(testCheckBox());
}

QTEST_MAIN(tst_QAccessibilityMac)
#include "tst_qaccessibilitymac.moc"
