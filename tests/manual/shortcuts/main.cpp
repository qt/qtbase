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
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QPushButton>

class ShortcutTester : public QWidget
{
public:
    ShortcutTester() {
        setupLayout();
        setFixedWidth(200);
    }
protected:
    void setupLayout()
    {
        QVBoxLayout *layout = new QVBoxLayout(this);

        QKeySequence sq1(Qt::AltModifier + Qt::ShiftModifier + Qt::Key_G);
        QPushButton *b1 = new QPushButton(sq1.toString());
        b1->setShortcut(sq1);

        QKeySequence sq2(Qt::AltModifier + Qt::Key_G);
        QPushButton *b2 = new QPushButton(sq2.toString());
        b2->setShortcut(sq2);

        QKeySequence sq3(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_R);
        QPushButton *b3 = new QPushButton(sq3.toString());
        b3->setShortcut(sq3);

        QKeySequence sq4(Qt::ControlModifier + Qt::Key_R);
        QPushButton *b4 = new QPushButton(sq4.toString());
        b4->setShortcut(sq4);

        QKeySequence sq5(Qt::ControlModifier + Qt::Key_Return);
        QPushButton *b5 = new QPushButton(sq5.toString());
        b5->setShortcut(sq5);

        QKeySequence sq6(Qt::ControlModifier + Qt::ShiftModifier + Qt::AltModifier + Qt::Key_R);
        QPushButton *b6 = new QPushButton(sq6.toString());
        b6->setShortcut(sq6);

        QKeySequence sq7(Qt::ShiftModifier + Qt::Key_5);
        QPushButton *b7 = new QPushButton(sq7.toString());
        b7->setShortcut(sq7);

        QKeySequence sq8(Qt::ControlModifier + Qt::Key_Q);
        QPushButton *b8 = new QPushButton(sq8.toString());
        b8->setShortcut(sq8);

        QKeySequence sq9(Qt::ControlModifier + Qt::Key_Plus);
        QPushButton *b9 = new QPushButton(sq9.toString());
        b9->setShortcut(sq9);

        QKeySequence sq10(Qt::ControlModifier + Qt::Key_Y);
        QPushButton *b10 = new QPushButton(sq10.toString());
        b10->setShortcut(sq10);

        QKeySequence sq11(Qt::ShiftModifier + Qt::Key_Comma);
        QPushButton *b11 = new QPushButton(sq11.toString());
        b11->setShortcut(sq11);

        QKeySequence sq12(Qt::ControlModifier + Qt::Key_Slash);
        QPushButton *b12 = new QPushButton(sq12.toString());
        b12->setShortcut(sq12);

        // LATIN SMALL LETTER O WITH STROKE
        QKeySequence sq13(QString(QChar(ushort(0xf8))));
        QPushButton *b13 = new QPushButton(sq13.toString());
        b13->setShortcut(sq13);

        // CYRILLIC SMALL LETTER ZHE
        QKeySequence sq14(QString(QChar(ushort(0x436))));
        QPushButton *b14 = new QPushButton(sq14.toString());
        b14->setShortcut(sq14);

        QLabel *testPurpose = new QLabel();
        testPurpose->setWordWrap(true);
        testPurpose->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        testPurpose->setText("This test come in handy to verify shortcuts under different"
                             " keyboard layouts - qwerty, dvorak, non-latin (russian, arabic), etc.");
        layout->addWidget(testPurpose);
        layout->addWidget(b1);
        layout->addWidget(b2);
        layout->addWidget(b3);
        layout->addWidget(b4);
        layout->addWidget(b5);
        layout->addWidget(b6);
        layout->addWidget(b7);
        layout->addWidget(b8);
        layout->addWidget(b9);
        layout->addWidget(b10);
        layout->addWidget(b11);
        layout->addWidget(b12);
        layout->addWidget(new QLabel("Norwegian layout"));
        layout->addWidget(b13);
        layout->addWidget(new QLabel("Russian layout"));
        layout->addWidget(b14);

        setLayout(layout);
    }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ShortcutTester tester;
    tester.show();

    return a.exec();
}
