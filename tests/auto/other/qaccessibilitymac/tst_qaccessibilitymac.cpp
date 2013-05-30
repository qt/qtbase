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

    void lineEditTest();
    void hierarchyTest();
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
    QPushButton *b = new QPushButton(w);
    w->setLayout(new QVBoxLayout());
    w->layout()->addWidget(b);
    b->setText("I am a button");

    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();
    QVERIFY(testHierarchy());
}

QTEST_MAIN(tst_QAccessibilityMac)
#include "tst_qaccessibilitymac.moc"
