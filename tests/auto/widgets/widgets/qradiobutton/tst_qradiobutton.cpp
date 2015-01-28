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
