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


#include <QRadioButton>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>


class tst_QRadioButton : public QObject
{
Q_OBJECT
public:
    tst_QRadioButton(){};
    virtual ~tst_QRadioButton(){};

private slots:
    void task190739_focus();
    void minimumSizeHint();

private:
};

void tst_QRadioButton::task190739_focus()
{
    QWidget widget;
    QPushButton button1(&widget);
    button1.setText("button1");
    QLineEdit edit(&widget);
    edit.setFocus();

    QRadioButton radio1(&widget);
    radio1.setText("radio1");
    radio1.setFocusPolicy(Qt::TabFocus);
    radio1.setShortcut(QKeySequence("Ctrl+O"));

    QVBoxLayout layout(&widget);
    layout.addWidget(&button1);
    layout.addWidget(&edit);
    layout.addWidget(&radio1);

    widget.show();
    widget.activateWindow();
    QApplication::setActiveWindow(&widget);
    QTest::qWait(100);

    QVERIFY(edit.hasFocus());
    QVERIFY(!radio1.isChecked());

    QTest::keyClick(&edit, Qt::Key_O, Qt::ControlModifier, 20);
    QTest::qWait(200);
    QVERIFY(radio1.isChecked());
    QVERIFY(edit.hasFocus());
    QVERIFY(!radio1.hasFocus());
}


void tst_QRadioButton::minimumSizeHint()
{
    QRadioButton button(tr("QRadioButtons sizeHint is the same as it's minimumSizeHint"));
    QCOMPARE(button.sizeHint(), button.minimumSizeHint());
}


QTEST_MAIN(tst_QRadioButton)
#include "tst_qradiobutton.moc"
