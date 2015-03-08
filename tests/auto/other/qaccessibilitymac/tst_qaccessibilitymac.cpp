/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        QTest::qWaitForWindowExposed(widget);
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

    QTest::qWaitForWindowExposed(m_window);
}

void tst_QAccessibilityMac::cleanup()
{
    delete m_window;
}

void tst_QAccessibilityMac::singleWidgetTest()
{
    if (!macNativeAccessibilityEnabled())
        return;

    delete m_window;
    m_window = 0;

    QVERIFY(singleWidget());
}

void tst_QAccessibilityMac::lineEditTest()
{
    if (!macNativeAccessibilityEnabled())
        return;

    QLineEdit *lineEdit = new QLineEdit(m_window);
    lineEdit->setText("a11y test QLineEdit");
    m_window->addWidget(lineEdit);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    QVERIFY(testLineEdit());
}

void tst_QAccessibilityMac::hierarchyTest()
{
    if (!macNativeAccessibilityEnabled())
        return;

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
    if (!macNativeAccessibilityEnabled())
        return;

    QVERIFY(notifications(m_window));
}

void tst_QAccessibilityMac::checkBoxTest()
{
    if (!macNativeAccessibilityEnabled())
        return;

    QCheckBox *cb = new QCheckBox(m_window);
    cb->setText("Great option");
    m_window->addWidget(cb);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();

    QVERIFY(testCheckBox());
}

QTEST_MAIN(tst_QAccessibilityMac)
#include "tst_qaccessibilitymac.moc"
