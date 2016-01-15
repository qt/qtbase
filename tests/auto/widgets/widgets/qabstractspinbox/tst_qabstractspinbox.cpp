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

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qabstractspinbox.h>
#include <qlineedit.h>
#include <qspinbox.h>

class tst_QAbstractSpinBox : public QObject
{
Q_OBJECT

public:
    tst_QAbstractSpinBox();
    virtual ~tst_QAbstractSpinBox();

private slots:
    void getSetCheck();

    // task-specific tests below me:
    void task183108_clear();
    void task228728_cssselector();
};

tst_QAbstractSpinBox::tst_QAbstractSpinBox()
{
}

tst_QAbstractSpinBox::~tst_QAbstractSpinBox()
{
}

class MyAbstractSpinBox : public QAbstractSpinBox
{
public:
    MyAbstractSpinBox() : QAbstractSpinBox() {}
    QLineEdit *lineEdit() { return QAbstractSpinBox::lineEdit(); }
    void setLineEdit(QLineEdit *le) { QAbstractSpinBox::setLineEdit(le); }
};

// Testing get/set functions
void tst_QAbstractSpinBox::getSetCheck()
{
    MyAbstractSpinBox obj1;
    // ButtonSymbols QAbstractSpinBox::buttonSymbols()
    // void QAbstractSpinBox::setButtonSymbols(ButtonSymbols)
    obj1.setButtonSymbols(QAbstractSpinBox::ButtonSymbols(QAbstractSpinBox::UpDownArrows));
    QCOMPARE(QAbstractSpinBox::ButtonSymbols(QAbstractSpinBox::UpDownArrows), obj1.buttonSymbols());
    obj1.setButtonSymbols(QAbstractSpinBox::ButtonSymbols(QAbstractSpinBox::PlusMinus));
    QCOMPARE(QAbstractSpinBox::ButtonSymbols(QAbstractSpinBox::PlusMinus), obj1.buttonSymbols());

    // bool QAbstractSpinBox::wrapping()
    // void QAbstractSpinBox::setWrapping(bool)
    obj1.setWrapping(false);
    QCOMPARE(false, obj1.wrapping());
    obj1.setWrapping(true);
    QCOMPARE(true, obj1.wrapping());

    // QLineEdit * QAbstractSpinBox::lineEdit()
    // void QAbstractSpinBox::setLineEdit(QLineEdit *)
    QLineEdit *var3 = new QLineEdit(0);
    obj1.setLineEdit(var3);
    QCOMPARE(var3, obj1.lineEdit());
#ifndef QT_DEBUG
    obj1.setLineEdit((QLineEdit *)0); // Will assert in debug, so only test in release
    QCOMPARE(var3, obj1.lineEdit()); // Setting 0 should keep the current editor
#endif
    // delete var3; // No delete, since QAbstractSpinBox takes ownership
}

void tst_QAbstractSpinBox::task183108_clear()
{
    QAbstractSpinBox *sbox;

    sbox = new QSpinBox;
    sbox->clear();
    sbox->show();
    qApp->processEvents();
    QVERIFY(sbox->text().isEmpty());

    delete sbox;
    sbox = new QSpinBox;
    sbox->clear();
    sbox->show();
    sbox->hide();
    qApp->processEvents();
    QCOMPARE(sbox->text(), QString());

    delete sbox;
    sbox = new QSpinBox;
    sbox->show();
    sbox->clear();
    qApp->processEvents();
    QCOMPARE(sbox->text(), QString());

    delete sbox;
    sbox = new QSpinBox;
    sbox->show();
    sbox->clear();
    sbox->hide();
    qApp->processEvents();
    QCOMPARE(sbox->text(), QString());

    delete sbox;
}

void tst_QAbstractSpinBox::task228728_cssselector()
{
    //QAbstractSpinBox does some call to stylehint into his constructor.
    //so while the stylesheet want to access property, it should not crash
    qApp->setStyleSheet("[alignment=\"1\"], [text=\"foo\"] { color:black; }" );
    QSpinBox box;
}


QTEST_MAIN(tst_QAbstractSpinBox)
#include "tst_qabstractspinbox.moc"
