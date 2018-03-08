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
#include <QDebug>

class ShortcutTester : public QWidget
{
public:
    ShortcutTester() {
        setupLayout();
    }
protected:
    void setupLayout()
    {
        QVBoxLayout *layout = new QVBoxLayout(this);
        setLayout(layout);

        QLabel *testPurpose = new QLabel();
        testPurpose->setWordWrap(true);
        testPurpose->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        testPurpose->setText("This test come in handy to verify shortcuts under different "
                             "keyboard layouts - qwerty, dvorak, non-latin (russian, arabic), etc.");
        layout->addWidget(testPurpose);

        QKeySequence altShiftG(Qt::AltModifier + Qt::ShiftModifier + Qt::Key_G);
        QPushButton *_altShiftG = new QPushButton(altShiftG.toString());
        _altShiftG->setShortcut(altShiftG);
        layout->addWidget(_altShiftG);

        QKeySequence altG(Qt::AltModifier + Qt::Key_G);
        QPushButton *_altG = new QPushButton(altG.toString());
        _altG->setShortcut(altG);
        layout->addWidget(_altG);

        QKeySequence ctrlShiftR(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_R);
        QPushButton *_ctrlShiftR = new QPushButton(ctrlShiftR.toString());
        _ctrlShiftR->setShortcut(ctrlShiftR);
        layout->addWidget(_ctrlShiftR);

        QKeySequence ctrlR(Qt::ControlModifier + Qt::Key_R);
        QPushButton *_ctrlR = new QPushButton(ctrlR.toString());
        _ctrlR->setShortcut(ctrlR);
        layout->addWidget(_ctrlR);

        QKeySequence ctrlReturn(Qt::ControlModifier + Qt::Key_Return);
        QPushButton *_ctrlReturn = new QPushButton(ctrlReturn.toString());
        _ctrlReturn->setShortcut(ctrlReturn);
        layout->addWidget(_ctrlReturn);

        QKeySequence ctrlEnter(Qt::ControlModifier + Qt::Key_Enter);
        QPushButton *_ctrlEnter = new QPushButton(ctrlEnter.toString());
        _ctrlEnter->setShortcut(ctrlEnter);
        layout->addWidget(_ctrlEnter);

        QKeySequence ctrlShiftAltR(Qt::ControlModifier + Qt::ShiftModifier + Qt::AltModifier + Qt::Key_R);
        QPushButton *_ctrlShiftAltR = new QPushButton(ctrlShiftAltR.toString());
        _ctrlShiftAltR->setShortcut(ctrlShiftAltR);
        layout->addWidget(_ctrlShiftAltR);

        QKeySequence shift5(Qt::ShiftModifier + Qt::Key_5);
        QPushButton *_shift5 = new QPushButton(shift5.toString());
        _shift5->setShortcut(shift5);
        layout->addWidget(_shift5);

        QKeySequence shiftPercent(Qt::ShiftModifier + Qt::Key_Percent);
        QPushButton *_shiftPercent = new QPushButton(shiftPercent.toString());
        _shiftPercent->setShortcut(shiftPercent);
        layout->addWidget(_shiftPercent);

        QKeySequence percent(Qt::Key_Percent);
        QPushButton *_percent = new QPushButton(percent.toString());
        _percent->setShortcut(percent);
        layout->addWidget(_percent);

        QKeySequence key5(Qt::Key_5);
        QPushButton *_key5 = new QPushButton(key5.toString());
        _key5->setShortcut(key5);
        layout->addWidget(_key5);

        QKeySequence keyQ(Qt::Key_Q);
        QPushButton *_keyQ = new QPushButton(keyQ.toString());
        _keyQ->setShortcut(keyQ);
        layout->addWidget(_keyQ);

        QKeySequence ctrlPercent(Qt::ControlModifier + Qt::Key_Percent);
        QPushButton *_ctrlPercent = new QPushButton(ctrlPercent.toString());
        _ctrlPercent->setShortcut(ctrlPercent);
        layout->addWidget(_ctrlPercent);

        QKeySequence ctrlShift5(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_5);
        QPushButton *_ctrlShift5 = new QPushButton(ctrlShift5.toString());
        _ctrlShift5->setShortcut(ctrlShift5);
        layout->addWidget(_ctrlShift5);

        QKeySequence ctrl5(Qt::ControlModifier + Qt::Key_5);
        QPushButton *_ctrl5 = new QPushButton(ctrl5.toString());
        _ctrl5->setShortcut(ctrl5);
        layout->addWidget(_ctrl5);

        QKeySequence alt5(Qt::AltModifier + Qt::Key_5);
        QPushButton *_alt5 = new QPushButton(alt5.toString());
        _alt5->setShortcut(alt5);
        layout->addWidget(_alt5);

        QKeySequence ctrlPlus(Qt::ControlModifier + Qt::Key_Plus);
        QPushButton *_ctrlPlus = new QPushButton(ctrlPlus.toString());
        _ctrlPlus->setShortcut(ctrlPlus);
        layout->addWidget(_ctrlPlus);

        QKeySequence ctrlShiftPlus(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_Plus);
        QPushButton *_ctrlShiftPlus = new QPushButton(ctrlShiftPlus.toString());
        _ctrlShiftPlus->setShortcut(ctrlShiftPlus);
        layout->addWidget(_ctrlShiftPlus);

        QKeySequence ctrlY(Qt::ControlModifier + Qt::Key_Y);
        QPushButton *_ctrlY = new QPushButton(ctrlY.toString());
        _ctrlY->setShortcut(ctrlY);
        layout->addWidget(_ctrlY);

        QKeySequence shiftComma(Qt::ShiftModifier + Qt::Key_Comma);
        QPushButton *_shiftComma = new QPushButton(shiftComma.toString());
        _shiftComma->setShortcut(shiftComma);
        layout->addWidget(_shiftComma);

        QKeySequence ctrlComma(Qt::ControlModifier + Qt::Key_Comma);
        QPushButton *_ctrlComma = new QPushButton(ctrlComma.toString());
        _ctrlComma->setShortcut(ctrlComma);
        layout->addWidget(_ctrlComma);

        QKeySequence ctrlSlash(Qt::ControlModifier + Qt::Key_Slash);
        QPushButton *_ctrlSlash = new QPushButton(ctrlSlash.toString());
        _ctrlSlash->setShortcut(ctrlSlash);
        layout->addWidget(_ctrlSlash);

        QKeySequence ctrlBackslash(Qt::ControlModifier + Qt::Key_Backslash);
        QPushButton *_ctrlBackslash = new QPushButton(ctrlBackslash.toString());
        _ctrlBackslash->setShortcut(ctrlBackslash);
        layout->addWidget(_ctrlBackslash);

        QKeySequence metaShiftA(Qt::MetaModifier + Qt::ShiftModifier + Qt::Key_A);
        QPushButton *_metaShiftA = new QPushButton(metaShiftA.toString());
        _metaShiftA->setShortcut(metaShiftA);
        layout->addWidget(_metaShiftA);

        QKeySequence metaShift5(Qt::MetaModifier + Qt::ShiftModifier + Qt::Key_5);
        QPushButton *_metaShift5 = new QPushButton(metaShift5.toString());
        _metaShift5->setShortcut(metaShift5);
        layout->addWidget(_metaShift5);

        QKeySequence ctrlBracketRigth(Qt::ControlModifier + Qt::Key_BracketRight);
        QPushButton *_ctrlBracketRigth = new QPushButton(ctrlBracketRigth.toString());
        _ctrlBracketRigth->setShortcut(ctrlBracketRigth);
        layout->addWidget(_ctrlBracketRigth);

        QKeySequence shiftF3(Qt::ShiftModifier + Qt::Key_F3);
        QPushButton *_shiftF3 = new QPushButton(shiftF3.toString());
        _shiftF3->setShortcut(shiftF3);
        layout->addWidget(_shiftF3);

        QKeySequence ctrlF3(Qt::ControlModifier + Qt::Key_F3);
        QPushButton *_ctrlF3 = new QPushButton(ctrlF3.toString());
        _ctrlF3->setShortcut(ctrlF3);
        layout->addWidget(_ctrlF3);

        QKeySequence euro(0x20AC); // EURO SIGN e.g. US (with euro on 5) on 3rd keyboard level
        QPushButton *_euro = new QPushButton(euro.toString());
        _euro->setShortcut(euro);
        layout->addWidget(_euro);

        QKeySequence ctrlEuro(Qt::ControlModifier + 0x20AC);
        QPushButton *_ctrlEuro = new QPushButton(ctrlEuro.toString());
        _ctrlEuro->setShortcut(ctrlEuro);
        layout->addWidget(_ctrlEuro);

        // with german (neo 2) layout on linux under ISO_Level3_Shift + ISO_Level5_Shift + I
        QKeySequence greekPsi(QString(QStringLiteral("\u03A8")));
        QPushButton *_greekPsi = new QPushButton(greekPsi.toString());
        _greekPsi->setShortcut(greekPsi);
        layout->addWidget(_greekPsi);

        layout->addWidget(new QLabel("Norwegian layout"));
        // LATIN SMALL LETTER O WITH STROKE
        QKeySequence norwegianO(QString(QStringLiteral("\u00F8")));
        QPushButton *_norwegianO = new QPushButton(norwegianO.toString());
        _norwegianO->setShortcut(norwegianO);
        layout->addWidget(_norwegianO);

        layout->addWidget(new QLabel("Russian layout"));
        // CYRILLIC SMALL LETTER ZHE
        QKeySequence zhe(QString(QStringLiteral("\u0436")));
        QPushButton *_zhe = new QPushButton(zhe.toString());
        _zhe->setShortcut(zhe);
        layout->addWidget(_zhe);

        // for sequence definitons see qplatformtheme.cpp
        layout->addWidget(new QLabel("QKeySequence::StandardKey(s)"));
        QPushButton *_open = new QPushButton("Open");
        _open->setShortcut(QKeySequence::Open); // Qt::CTRL | Qt::Key_O
        layout->addWidget(_open);
    }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << qVersion();

    ShortcutTester tester;
    tester.show();

    return a.exec();
}
