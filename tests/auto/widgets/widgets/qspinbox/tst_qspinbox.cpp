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
#include <qdebug.h>
#include <qapplication.h>
#include <limits.h>

#include <qspinbox.h>
#include <qlocale.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <QSpinBox>
#include <QWidget>
#include <QString>
#include <QValidator>
#include <QLineEdit>
#include <QObject>
#include <QStringList>
#include <QList>
#include <QLocale>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QKeySequence>
#include <QStackedWidget>
#include <QDebug>

class SpinBox : public QSpinBox
{
public:
    SpinBox(QWidget *parent = 0)
        : QSpinBox(parent)
    {}
    QString textFromValue(int v) const
    {
        return QSpinBox::textFromValue(v);
    }
    QValidator::State validate(QString &text, int &pos) const
    {
        return QSpinBox::validate(text, pos);
    }
    int valueFromText(const QString &text) const
    {
        return QSpinBox::valueFromText(text);
    }
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event)
    {
        QSpinBox::wheelEvent(event);
    }
#endif

    QLineEdit *lineEdit() const { return QSpinBox::lineEdit(); }
};

class tst_QSpinBox : public QObject
{
    Q_OBJECT
public:
    tst_QSpinBox();
public slots:
    void init();
private slots:
    void getSetCheck();
    void setValue_data();
    void setValue();

    void setDisplayIntegerBase_data();
    void setDisplayIntegerBase();

    void setPrefixSuffix_data();
    void setPrefixSuffix();

    void setReadOnly();

    void setTracking_data();
    void setTracking();

    void locale_data();
    void locale();

    void setWrapping_data();
    void setWrapping();

    void setSpecialValueText_data();
    void setSpecialValueText();

    void setSingleStep_data();
    void setSingleStep();

    void setMinMax_data();
    void setMinMax();

    void editingFinished();

    void valueFromTextAndValidate_data();
    void valueFromTextAndValidate();

    void removeAll();
    void startWithDash();
    void undoRedo();

    void specialValue();
    void textFromValue();

    void sizeHint();

    void integerOverflow();

    void taskQTBUG_5008_textFromValueAndValidate();
    void lineEditReturnPressed();

    void positiveSign();

    void setGroupSeparatorShown_data();
    void setGroupSeparatorShown();

    void wheelEvents();

public slots:
    void valueChangedHelper(const QString &);
    void valueChangedHelper(int);
private:
    QStringList actualTexts;
    QList<int> actualValues;
};

typedef QList<int> IntList;

Q_DECLARE_METATYPE(QLocale::Language)
Q_DECLARE_METATYPE(QLocale::Country)

// Testing get/set functions
void tst_QSpinBox::getSetCheck()
{
    QSpinBox obj1;
    QCOMPARE(obj1.inputMethodQuery(Qt::ImHints), QVariant(int(Qt::ImhDigitsOnly)));
    // int QSpinBox::singleStep()
    // void QSpinBox::setSingleStep(int)
    obj1.setSingleStep(0);
    QCOMPARE(0, obj1.singleStep());
    obj1.setSingleStep(INT_MIN);
    QCOMPARE(0, obj1.singleStep()); // Can't have negative steps => keep old value
    obj1.setSingleStep(INT_MAX);
    QCOMPARE(INT_MAX, obj1.singleStep());

    // int QSpinBox::minimum()
    // void QSpinBox::setMinimum(int)
    obj1.setMinimum(0);
    QCOMPARE(0, obj1.minimum());
    obj1.setMinimum(INT_MIN);
    QCOMPARE(INT_MIN, obj1.minimum());
    obj1.setMinimum(INT_MAX);
    QCOMPARE(INT_MAX, obj1.minimum());

    // int QSpinBox::maximum()
    // void QSpinBox::setMaximum(int)
    obj1.setMaximum(0);
    QCOMPARE(0, obj1.maximum());
    obj1.setMaximum(INT_MIN);
    QCOMPARE(INT_MIN, obj1.maximum());
    obj1.setMaximum(INT_MAX);
    QCOMPARE(INT_MAX, obj1.maximum());

    // int QSpinBox::value()
    // void QSpinBox::setValue(int)
    obj1.setValue(0);
    QCOMPARE(0, obj1.value());
    obj1.setValue(INT_MIN);
    QCOMPARE(INT_MIN, obj1.value());
    obj1.setValue(INT_MAX);
    QCOMPARE(INT_MAX, obj1.value());

    QDoubleSpinBox obj2;
    QCOMPARE(obj2.inputMethodQuery(Qt::ImHints), QVariant(int(Qt::ImhFormattedNumbersOnly)));
    // double QDoubleSpinBox::singleStep()
    // void QDoubleSpinBox::setSingleStep(double)
    obj2.setSingleStep(0.0);
    QCOMPARE(0.0, obj2.singleStep());
    obj2.setSingleStep(1.0);
    QCOMPARE(1.0, obj2.singleStep());

    // double QDoubleSpinBox::minimum()
    // void QDoubleSpinBox::setMinimum(double)
    obj2.setMinimum(1.0);
    QCOMPARE(1.0, obj2.minimum());
    obj2.setMinimum(0.0);
    QCOMPARE(0.0, obj2.minimum());
    obj2.setMinimum(-1.0);
    QCOMPARE(-1.0, obj2.minimum());

    // double QDoubleSpinBox::maximum()
    // void QDoubleSpinBox::setMaximum(double)
    obj2.setMaximum(-1.0);
    QCOMPARE(-1.0, obj2.maximum());
    obj2.setMaximum(0.0);
    QCOMPARE(0.0, obj2.maximum());
    obj2.setMaximum(1.0);
    QCOMPARE(1.0, obj2.maximum());

    // int QDoubleSpinBox::decimals()
    // void QDoubleSpinBox::setDecimals(int)
    obj2.setDecimals(0);
    QCOMPARE(0, obj2.decimals());
    obj2.setDecimals(INT_MIN);
    QCOMPARE(0, obj2.decimals()); // Range<0, 13>

    //obj2.setDecimals(INT_MAX);
    //QCOMPARE(13, obj2.decimals()); // Range<0, 13>
    obj2.setDecimals(128);
    QCOMPARE(obj2.decimals(), 128); // Range<0, 128>

    // double QDoubleSpinBox::value()
    // void QDoubleSpinBox::setValue(double)
    obj2.setValue(-1.0);
    QCOMPARE(-1.0, obj2.value());
    obj2.setValue(0.0);
    QCOMPARE(0.0, obj2.value());
    obj2.setValue(1.0);
    QCOMPARE(1.0, obj2.value());

    // Make sure we update line edit geometry when updating
    // buttons - see task 235747
    QRect oldEditGeometry = obj1.childrenRect();
    obj1.setButtonSymbols(QAbstractSpinBox::NoButtons);
    QVERIFY(obj1.childrenRect() != oldEditGeometry);
}

tst_QSpinBox::tst_QSpinBox()
{
}

void tst_QSpinBox::init()
{
    QLocale::setDefault(QLocale(QLocale::C));
}

void tst_QSpinBox::setValue_data()
{
    QTest::addColumn<int>("set");
    QTest::addColumn<int>("expected");

    QTest::newRow("data0") << 0 << 0;
    QTest::newRow("data1") << 100 << 100;
    QTest::newRow("data2") << -100 << -100;
    QTest::newRow("data3") << INT_MIN << INT_MIN;
    QTest::newRow("data4") << INT_MAX << INT_MAX;
}

void tst_QSpinBox::setValue()
{
    QFETCH(int, set);
    QFETCH(int, expected);
    QSpinBox spin(0);
    spin.setRange(INT_MIN, INT_MAX);
    spin.setValue(set);
    QCOMPARE(spin.value(), expected);
}

void tst_QSpinBox::setDisplayIntegerBase_data()
{
    QTest::addColumn<int>("value");
    QTest::addColumn<int>("base");
    QTest::addColumn<QString>("string");

    QTest::newRow("base 10") << 42 << 10 << "42";
    QTest::newRow("base 2") << 42 << 2 << "101010";
    QTest::newRow("base 8") << 42 << 8 << "52";
    QTest::newRow("base 16") << 42 << 16 << "2a";
    QTest::newRow("base 0") << 42 << 0 << "42";
    QTest::newRow("base -4") << 42 << -4 << "42";
    QTest::newRow("base 40") << 42 << 40 << "42";

    QTest::newRow("negative base 10") << -42 << 10 << "-42";
    QTest::newRow("negative base 2") << -42 << 2 << "-101010";
    QTest::newRow("negative base 8") << -42 << 8 << "-52";
    QTest::newRow("negative base 16") << -42 << 16 << "-2a";
    QTest::newRow("negative base 0") << -42 << 0 << "-42";
    QTest::newRow("negative base -4") << -42 << -4 << "-42";
    QTest::newRow("negative base 40") << -42 << 40 << "-42";

    QTest::newRow("0 base 10") << 0 << 10 << "0";
    QTest::newRow("0 base 2") << 0 << 2 << "0";
    QTest::newRow("0 base 8") << 0 << 8 << "0";
    QTest::newRow("0 base 16") << 0 << 16 << "0";
    QTest::newRow("0 base 0") << 0 << 0 << "0";
    QTest::newRow("0 base -4") << 0 << -4 << "0";
    QTest::newRow("0 base 40") << 0 << 40 << "0";
}

void tst_QSpinBox::setDisplayIntegerBase()
{
    QFETCH(int, value);
    QFETCH(int, base);
    QFETCH(QString, string);

    SpinBox spin;
    spin.setRange(INT_MIN, INT_MAX);

    spin.setValue(value);
    QCOMPARE(spin.lineEdit()->text(), QString::number(value));

    spin.setDisplayIntegerBase(base);
    QCOMPARE(spin.lineEdit()->text(), string);

    spin.setValue(0);
    QCOMPARE(spin.value(), 0);
    QCOMPARE(spin.lineEdit()->text(), QString::number(0, base));

    spin.lineEdit()->clear();
    QTest::keyClicks(spin.lineEdit(), string);
    QCOMPARE(spin.value(), value);
}

void tst_QSpinBox::setPrefixSuffix_data()
{
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<int>("value");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QString>("expectedCleanText");
    QTest::addColumn<bool>("show");

    QTest::newRow("data0") << QString() << QString() << 10 << "10" << "10" << false;
    QTest::newRow("data1") << QString() << "cm" << 10 << "10cm" << "10" << false;
    QTest::newRow("data2") << "cm: " << QString() << 10 << "cm: 10" << "10" << false;
    QTest::newRow("data3") << "length: " << "cm" << 10 << "length: 10cm" << "10" << false;

    QTest::newRow("data4") << QString() << QString() << 10 << "10" << "10" << true;
    QTest::newRow("data5") << QString() << "cm" << 10 << "10cm" << "10" << true;
    QTest::newRow("data6") << "cm: " << QString() << 10 << "cm: 10" << "10" << true;
    QTest::newRow("data7") << "length: " << "cm" << 10 << "length: 10cm" << "10" << true;
}

void tst_QSpinBox::setPrefixSuffix()
{
    QFETCH(QString, prefix);
    QFETCH(QString, suffix);
    QFETCH(int, value);
    QFETCH(QString, expectedText);
    QFETCH(QString, expectedCleanText);
    QFETCH(bool, show);

    QSpinBox spin(0);
    spin.setPrefix(prefix);
    spin.setSuffix(suffix);
    spin.setValue(value);
    if (show)
        spin.show();

    QCOMPARE(spin.prefix(), prefix);
    QCOMPARE(spin.suffix(), suffix);
    QCOMPARE(spin.text(), expectedText);
    QCOMPARE(spin.cleanText(), expectedCleanText);
}

void tst_QSpinBox::valueChangedHelper(const QString &text)
{
    actualTexts << text;
}

void tst_QSpinBox::valueChangedHelper(int value)
{
    actualValues << value;
}

class MySpinBox: public QSpinBox
{
public:
    MySpinBox(QWidget *parent = 0) : QSpinBox(parent) {}

    void changeEvent(QEvent *ev) {
        eventsReceived.append(ev->type());
    }
    QList<QEvent::Type> eventsReceived;
};

void tst_QSpinBox::setReadOnly()
{
    MySpinBox spin(0);
    spin.show();
    QTest::keyClick(&spin, Qt::Key_Up);
    QCOMPARE(spin.value(), 1);
    spin.setReadOnly(true);
    QCOMPARE(spin.eventsReceived, QList<QEvent::Type>() << QEvent::ReadOnlyChange);
    QTest::keyClick(&spin, Qt::Key_Up);
    QCOMPARE(spin.value(), 1);
    spin.stepBy(1);
    QCOMPARE(spin.value(), 2);
    spin.setReadOnly(false);
    QCOMPARE(spin.eventsReceived, QList<QEvent::Type>() << QEvent::ReadOnlyChange << QEvent::ReadOnlyChange);
    QTest::keyClick(&spin, Qt::Key_Up);
    QCOMPARE(spin.value(), 3);
}
void tst_QSpinBox::setTracking_data()
{
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<QStringList>("texts");
    QTest::addColumn<bool>("tracking");

    QTestEventList keys;
    QStringList texts1;
    QStringList texts2;

#ifdef Q_OS_MAC
    keys.addKeyClick(Qt::Key_Right, Qt::ControlModifier);
#else
    keys.addKeyClick(Qt::Key_End);
#endif
    keys.addKeyClick('7');
    keys.addKeyClick('9');
    keys.addKeyClick(Qt::Key_Enter);
    keys.addKeyClick(Qt::Key_Enter);
    keys.addKeyClick(Qt::Key_Enter);
    texts1 << "07" << "079" << "79" << "79" << "79";
    texts2 << "79";
    QTest::newRow("data1") << keys << texts1 << true;
    QTest::newRow("data2") << keys << texts2 << false;
}

void tst_QSpinBox::setTracking()
{
    actualTexts.clear();
    QFETCH(QTestEventList, keys);
    QFETCH(QStringList, texts);
    QFETCH(bool, tracking);

    QSpinBox spin(0);
    spin.setKeyboardTracking(tracking);
    spin.show();
    connect(&spin, SIGNAL(valueChanged(QString)), this, SLOT(valueChangedHelper(QString)));

    keys.simulate(&spin);
    QCOMPARE(actualTexts, texts);
}

void tst_QSpinBox::setWrapping_data()
{
    QTest::addColumn<bool>("wrapping");
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<int>("startValue");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<IntList>("expected");

    QTestEventList keys;
    IntList values;
    keys.addKeyClick(Qt::Key_Up);
    values << 10;
    keys.addKeyClick(Qt::Key_Up);
    QTest::newRow("data0") << false << 0 << 10 << 9 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_Up);
    values << 10;
    keys.addKeyClick(Qt::Key_Up);
    values << 0;
    QTest::newRow("data1") << true << 0 << 10 << 9 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_Delete); // doesn't emit because lineedit is empty so intermediate
    keys.addKeyClick('1');
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Down);
    values << 1 << 0;
    QTest::newRow("data2") << false << 0 << 10 << 9 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_Delete);
    keys.addKeyClick('1');
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Down);
    values << 1 << 0 << 10;
    QTest::newRow("data3") << true << 0 << 10 << 9 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_PageDown);
    keys.addKeyClick(Qt::Key_Down);
    values << 0;
    QTest::newRow("data4") << false << 0 << 10 << 6 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_PageDown);
    keys.addKeyClick(Qt::Key_Down);
    values << 0 << 10;
    QTest::newRow("data5") << true << 0 << 10 << 6 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_PageUp);
    keys.addKeyClick(Qt::Key_PageDown);
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_PageDown);
    keys.addKeyClick(Qt::Key_PageDown);
    values << 10 << 0 << 10 << 0 << 10 << 0;
    QTest::newRow("data6") << true << 0 << 10 << 6 << keys << values;

}


void tst_QSpinBox::setWrapping()
{
    QFETCH(bool, wrapping);
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(int, startValue);
    QFETCH(QTestEventList, keys);
    QFETCH(IntList, expected);

    QSpinBox spin(0);
    QVERIFY(!spin.wrapping());
    spin.setMinimum(minimum);
    spin.setMaximum(maximum);
    spin.setValue(startValue);
    spin.setWrapping(wrapping);
    spin.show();
    actualValues.clear();
    connect(&spin, SIGNAL(valueChanged(int)), this, SLOT(valueChangedHelper(int)));

    keys.simulate(&spin);

    QCOMPARE(actualValues.size(), expected.size());
    for (int i=0; i<qMin(actualValues.size(), expected.size()); ++i) {
        QCOMPARE(actualValues.at(i), expected.at(i));
    }
}

void tst_QSpinBox::setSpecialValueText_data()
{
    QTest::addColumn<QString>("specialValueText");
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<int>("value");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("show");

    QTest::newRow("data0") << QString() << 0 << 10 << 1 << "1" << false;
    QTest::newRow("data1") << QString() << 0 << 10 << 1 << "1" << true;
    QTest::newRow("data2") << "foo" << 0 << 10 << 0 << "foo" << false;
    QTest::newRow("data3") << "foo" << 0 << 10 << 0 << "foo" << true;
}

void tst_QSpinBox::setSpecialValueText()
{
    QFETCH(QString, specialValueText);
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(int, value);
    QFETCH(QString, expected);
    QFETCH(bool, show);

    QSpinBox spin(0);
    spin.setSpecialValueText(specialValueText);
    QCOMPARE(spin.specialValueText(), specialValueText);
    spin.setMinimum(minimum);
    spin.setMaximum(maximum);
    spin.setValue(value);
    if (show)
        spin.show();

    QCOMPARE(spin.text(), expected);
}

void tst_QSpinBox::setSingleStep_data()
{
    QTest::addColumn<int>("singleStep");
    QTest::addColumn<int>("startValue");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<IntList>("expected");
    QTest::addColumn<bool>("show");

    QTestEventList keys;
    IntList values;
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Up);
    values << 11 << 10 << 11;
    QTest::newRow("data0") << 1 << 10 << keys << values << false;
    QTest::newRow("data1") << 1 << 10 << keys << values << true;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Up);
    values << 12 << 10 << 12;
    QTest::newRow("data2") << 2 << 10 << keys << values << false;
    QTest::newRow("data3") << 2 << 10 << keys << values << true;
}

void tst_QSpinBox::setSingleStep()
{
    QFETCH(int, singleStep);
    QFETCH(int, startValue);
    QFETCH(QTestEventList, keys);
    QFETCH(IntList, expected);
    QFETCH(bool, show);

    QSpinBox spin(0);
    actualValues.clear();
    spin.setSingleStep(singleStep);
    QCOMPARE(spin.singleStep(), singleStep);
    spin.setValue(startValue);
    if (show)
        spin.show();
    connect(&spin, SIGNAL(valueChanged(int)), this, SLOT(valueChangedHelper(int)));

    QCOMPARE(actualValues.size(), 0);
    keys.simulate(&spin);
    QCOMPARE(actualValues.size(), expected.size());
    for (int i=0; i<qMin(actualValues.size(), expected.size()); ++i) {
        QCOMPARE(actualValues.at(i), expected.at(i));
    }
}

void tst_QSpinBox::setMinMax_data()
{
    QTest::addColumn<int>("startValue");
    QTest::addColumn<int>("mini");
    QTest::addColumn<int>("maxi");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<int>("expected");
    QTest::addColumn<bool>("show");

    QTestEventList keys;
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Up);
    QTest::newRow("data0") << 1 << INT_MIN << 2 << keys << 2 << false;
    QTest::newRow("data1") << 1 << INT_MIN << 2 << keys << 2 << true;

    keys.clear();
    QTest::newRow("data2") << 2 << INT_MAX - 2 << INT_MAX << keys << INT_MAX - 2 << false;
    QTest::newRow("data3") << 2 << INT_MAX - 2 << INT_MAX << keys << INT_MAX - 2 << true;
}

void tst_QSpinBox::setMinMax()
{
    QFETCH(int, startValue);
    QFETCH(int, mini);
    QFETCH(int, maxi);
    QFETCH(QTestEventList, keys);
    QFETCH(int, expected);
    QFETCH(bool, show);

    QSpinBox spin(0);
    spin.setValue(startValue);
    spin.setMinimum(mini);
    spin.setMaximum(maxi);
    QCOMPARE(spin.minimum(), mini);
    QCOMPARE(spin.maximum(), maxi);
    if (show)
        spin.show();
    keys.simulate(&spin);
    QCOMPARE(spin.value(), expected);
}

void tst_QSpinBox::valueFromTextAndValidate_data()
{
    const int Intermediate = QValidator::Intermediate;
    const int Invalid = QValidator::Invalid;
    const int Acceptable = QValidator::Acceptable;

    QTest::addColumn<QString>("txt");
    QTest::addColumn<int>("state");
    QTest::addColumn<int>("mini");
    QTest::addColumn<int>("maxi");
    QTest::addColumn<QString>("expectedText"); // if empty we don't check

    QTest::newRow("data0") << QString("2") << Intermediate << 3 << 5 << QString();
    QTest::newRow("data1") << QString() << Intermediate << 0 << 100 << QString();
    QTest::newRow("data2") << QString("asd") << Invalid << 0 << 100 << QString();
    QTest::newRow("data3") << QString("2") << Acceptable << 0 << 100 << QString();
    QTest::newRow("data4") << QString() << Intermediate << 0 << 1 << QString();
    QTest::newRow("data5") << QString() << Invalid << 0 << 0 << QString();
    QTest::newRow("data6") << QString("5") << Intermediate << 2004 << 2005 << QString();
    QTest::newRow("data7") << QString("50") << Intermediate << 2004 << 2005 << QString();
    QTest::newRow("data8") << QString("205") << Intermediate << 2004 << 2005 << QString();
    QTest::newRow("data9") << QString("2005") << Acceptable << 2004 << 2005 << QString();
    QTest::newRow("data10") << QString("3") << Intermediate << 2004 << 2005 << QString();
    QTest::newRow("data11") << QString("-") << Intermediate << -20 << -10 << QString();
    QTest::newRow("data12") << QString("-1") << Intermediate << -20 << -10 << QString();
    QTest::newRow("data13") << QString("-5") << Intermediate << -20 << -10 << QString();
    QTest::newRow("data14") << QString("-5") << Intermediate << -20 << -16 << QString();
    QTest::newRow("data15") << QString("-2") << Intermediate << -20 << -16 << QString();
    QTest::newRow("data16") << QString("2") << Invalid << -20 << -16 << QString();
    QTest::newRow("data17") << QString() << Intermediate << -20 << -16 << QString();
    QTest::newRow("data18") << QString("  22") << Acceptable << 0 << 1000 << QString("22");
    QTest::newRow("data19") << QString("22  ") << Acceptable << 0 << 1000 << QString("22");
    QTest::newRow("data20") << QString("  22  ") << Acceptable << 0 << 1000 << QString("22");
    QTest::newRow("data21") << QString("2 2") << Invalid << 0 << 1000 << QString();
}

static QString stateName(int state)
{
    switch (state) {
    case QValidator::Acceptable: return QString("Acceptable");
    case QValidator::Intermediate: return QString("Intermediate");
    case QValidator::Invalid: return QString("Invalid");
    default: break;
    }
    qWarning("%s %d: this should never happen", __FILE__, __LINE__);
    return QString();
}

void tst_QSpinBox::valueFromTextAndValidate()
{
    QFETCH(QString, txt);
    QFETCH(int, state);
    QFETCH(int, mini);
    QFETCH(int, maxi);
    QFETCH(QString, expectedText);

    SpinBox sb(0);
    sb.show();
    sb.setRange(mini, maxi);
    int unused = 0;
    QCOMPARE(stateName(sb.validate(txt, unused)), stateName(state));
    if (!expectedText.isEmpty())
        QCOMPARE(txt, expectedText);
}

void tst_QSpinBox::locale_data()
{
    QTest::addColumn<QLocale>("loc");
    QTest::addColumn<int>("value");
    QTest::addColumn<QString>("textFromVal");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("valFromText");

    QTest::newRow("data0") << QLocale(QLocale::Norwegian, QLocale::Norway) << 1234 << QString("1234") << QString("2345") << 2345;
    QTest::newRow("data1") << QLocale(QLocale::German, QLocale::Germany) << 1234 << QString("1234") << QString("2345") << 2345;
}

void tst_QSpinBox::locale()
{
    QFETCH(QLocale, loc);
    QFETCH(int, value);
    QFETCH(QString, textFromVal);
    QFETCH(QString, text);
    QFETCH(int, valFromText);

    QLocale old;

    QLocale::setDefault(loc);
    SpinBox box;
    box.setMaximum(100000);
    box.setValue(value);
    QCOMPARE(box.cleanText(), textFromVal);

    box.lineEdit()->setText(text);
    QCOMPARE(box.cleanText(), text);
    box.interpretText();

    QCOMPARE(box.value(), valFromText);
}


void tst_QSpinBox::editingFinished()
{
    QWidget testFocusWidget;
    testFocusWidget.setObjectName(QLatin1String("tst_qspinbox"));
    testFocusWidget.setWindowTitle(objectName());
    testFocusWidget.resize(200, 100);

    QVBoxLayout *layout = new QVBoxLayout(&testFocusWidget);
    QSpinBox *box = new QSpinBox(&testFocusWidget);
    layout->addWidget(box);
    QSpinBox *box2 = new QSpinBox(&testFocusWidget);
    layout->addWidget(box2);

    testFocusWidget.show();
    QApplication::setActiveWindow(&testFocusWidget);
    QVERIFY(QTest::qWaitForWindowActive(&testFocusWidget));
    box->activateWindow();
    box->setFocus();

    QTRY_COMPARE(qApp->focusWidget(), (QWidget *)box);

    QSignalSpy editingFinishedSpy1(box, SIGNAL(editingFinished()));
    QSignalSpy editingFinishedSpy2(box2, SIGNAL(editingFinished()));

    box->setFocus();
    QTest::keyClick(box, Qt::Key_Up);
    QTest::keyClick(box, Qt::Key_Up);

    QCOMPARE(editingFinishedSpy1.count(), 0);
    QCOMPARE(editingFinishedSpy2.count(), 0);

    QTest::keyClick(box2, Qt::Key_Up);
    QTest::keyClick(box2, Qt::Key_Up);
    box2->setFocus();
    QCOMPARE(editingFinishedSpy1.count(), 1);
    box->setFocus();
    QCOMPARE(editingFinishedSpy1.count(), 1);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    QTest::keyClick(box, Qt::Key_Up);
    QCOMPARE(editingFinishedSpy1.count(), 1);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    QTest::keyClick(box, Qt::Key_Enter);
    QCOMPARE(editingFinishedSpy1.count(), 2);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    QTest::keyClick(box, Qt::Key_Return);
    QCOMPARE(editingFinishedSpy1.count(), 3);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    box2->setFocus();
    QCOMPARE(editingFinishedSpy1.count(), 4);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    QTest::keyClick(box2, Qt::Key_Enter);
    QCOMPARE(editingFinishedSpy1.count(), 4);
    QCOMPARE(editingFinishedSpy2.count(), 2);
    QTest::keyClick(box2, Qt::Key_Return);
    QCOMPARE(editingFinishedSpy1.count(), 4);
    QCOMPARE(editingFinishedSpy2.count(), 3);

    testFocusWidget.hide();
    QCOMPARE(editingFinishedSpy1.count(), 4);
    QCOMPARE(editingFinishedSpy2.count(), 4);
    QTest::qWait(100);

    //task203285
    editingFinishedSpy1.clear();
    testFocusWidget.show();
    QTest::qWait(100);
    box->setKeyboardTracking(false);
    qApp->setActiveWindow(&testFocusWidget);
    testFocusWidget.activateWindow();
    box->setFocus();
    QTRY_VERIFY(box->hasFocus());
    box->setValue(0);
    QTest::keyClick(box, '2');
    QCOMPARE(box->text(), QLatin1String("20"));
    box2->setFocus();
    QTRY_VERIFY(qApp->focusWidget() != box);
    QCOMPARE(box->text(), QLatin1String("20"));
    QCOMPARE(editingFinishedSpy1.count(), 1);
}

void tst_QSpinBox::removeAll()
{
    SpinBox spin(0);
    spin.setPrefix("foo");
    spin.setSuffix("bar");
    spin.setValue(2);
    spin.show();
#ifdef Q_OS_MAC
    QTest::keyClick(&spin, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(&spin, Qt::Key_Home);
#endif

#ifdef Q_OS_MAC
    QTest::keyClick(&spin, Qt::Key_Right, Qt::ControlModifier|Qt::ShiftModifier);
#else
    QTest::keyClick(&spin, Qt::Key_End, Qt::ShiftModifier);
#endif

    QCOMPARE(spin.lineEdit()->selectedText(), QString("foo2bar"));
    QTest::keyClick(&spin, Qt::Key_1);
    QCOMPARE(spin.text(), QString("foo1bar"));
}

void tst_QSpinBox::startWithDash()
{
    SpinBox spin(0);
    spin.show();
#ifdef Q_OS_MAC
    QTest::keyClick(&spin, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(&spin, Qt::Key_Home);
#endif
    QCOMPARE(spin.text(), QString("0"));
    QTest::keyClick(&spin, Qt::Key_Minus);
    QCOMPARE(spin.text(), QString("0"));
}

void tst_QSpinBox::undoRedo()
{
    //test undo/redo feature (in conjunction with the "undoRedoEnabled" property)
    SpinBox spin(0);
    spin.show();

    //the undo/redo is disabled by default

    QCOMPARE(spin.value(), 0); //this is the default value
    QVERIFY(!spin.lineEdit()->isUndoAvailable());
    QVERIFY(!spin.lineEdit()->isRedoAvailable());

    spin.lineEdit()->selectAll(); //ensures everything is selected and will be cleared by typing a key
    QTest::keyClick(&spin, Qt::Key_1); //we put 1 into the spinbox
    QCOMPARE(spin.value(), 1);
    QVERIFY(spin.lineEdit()->isUndoAvailable());

    //testing CTRL+Z (undo)
    int val = QKeySequence(QKeySequence::Undo)[0];
    Qt::KeyboardModifiers mods = (Qt::KeyboardModifiers)(val & Qt::KeyboardModifierMask);
    QTest::keyClick(&spin, val & ~mods, mods);

    QCOMPARE(spin.value(), 0);
    QVERIFY(!spin.lineEdit()->isUndoAvailable());
    QVERIFY(spin.lineEdit()->isRedoAvailable());

    //testing CTRL+Y (redo)
    val = QKeySequence(QKeySequence::Redo)[0];
    mods = (Qt::KeyboardModifiers)(val & Qt::KeyboardModifierMask);
    QTest::keyClick(&spin, val & ~mods, mods);
    QCOMPARE(spin.value(), 1);
    QVERIFY(!spin.lineEdit()->isRedoAvailable());
    QVERIFY(spin.lineEdit()->isUndoAvailable());

    spin.setValue(55);
    QVERIFY(!spin.lineEdit()->isUndoAvailable());
    QVERIFY(!spin.lineEdit()->isRedoAvailable());

    QTest::keyClick(&spin, Qt::Key_Return);
    QTest::keyClick(&spin, '1');
    QVERIFY(spin.lineEdit()->isUndoAvailable());
    QVERIFY(!spin.lineEdit()->isRedoAvailable());
    spin.lineEdit()->undo();
    QCOMPARE(spin.value(), 55);
    QVERIFY(!spin.lineEdit()->isUndoAvailable());
    QVERIFY(spin.lineEdit()->isRedoAvailable());
    spin.lineEdit()->redo();
    QCOMPARE(spin.value(), 1);
    QVERIFY(spin.lineEdit()->isUndoAvailable());
    QVERIFY(!spin.lineEdit()->isRedoAvailable());
}

void tst_QSpinBox::specialValue()
{
    QString specialText="foo";

    QWidget topWidget;
    QVBoxLayout layout(&topWidget);
    SpinBox spin(&topWidget);
    layout.addWidget(&spin);
    SpinBox box2(&topWidget);
    layout.addWidget(&box2);

    spin.setSpecialValueText(specialText);
    spin.setMinimum(0);
    spin.setMaximum(100);
    spin.setValue(50);
    topWidget.show();
    //make sure we have the focus (even if editingFinished fails)
    qApp->setActiveWindow(&topWidget);
    topWidget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&topWidget));
    spin.setFocus();

    QTest::keyClick(&spin, Qt::Key_Return);
    QTest::keyClick(&spin, '0');
    QCOMPARE(spin.text(), QString("0"));
    QTest::keyClick(&spin, Qt::Key_Return);
    QCOMPARE(spin.text(), specialText);

    spin.setValue(50);
    QTest::keyClick(&spin, Qt::Key_Return);
    QTest::keyClick(&spin, '0');
    QCOMPARE(spin.text(), QString("0"));
    QTest::keyClick(spin.lineEdit(), Qt::Key_Tab);
    QCOMPARE(spin.text(), specialText);

    spin.setValue(50);
    spin.setFocus();
    QTest::keyClick(&spin, Qt::Key_Return);
    QTest::keyClick(&spin, '0');
    QCOMPARE(spin.text(), QString("0"));
    box2.setFocus();
    QCOMPARE(spin.text(), specialText);
}

void tst_QSpinBox::textFromValue()
{
    SpinBox spinBox;
    QCOMPARE(spinBox.textFromValue(INT_MIN), QString::number(INT_MIN));
}

class sizeHint_SpinBox : public QSpinBox
{
public:
    QSize sizeHint() const
    {
        ++sizeHintRequests;
        return QSpinBox::sizeHint();
    }
    mutable int sizeHintRequests;
};

void tst_QSpinBox::sizeHint()
{
    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(widget);
    sizeHint_SpinBox *spinBox = new sizeHint_SpinBox;
    layout->addWidget(spinBox);
    widget->show();
    QVERIFY(QTest::qWaitForWindowExposed(widget));

    // Prefix
    spinBox->sizeHintRequests = 0;
    spinBox->setPrefix(QLatin1String("abcdefghij"));
    qApp->processEvents();
    QTRY_VERIFY(spinBox->sizeHintRequests > 0);

    // Suffix
    spinBox->sizeHintRequests = 0;
    spinBox->setSuffix(QLatin1String("abcdefghij"));
    qApp->processEvents();
    QTRY_VERIFY(spinBox->sizeHintRequests > 0);

    // Range
    spinBox->sizeHintRequests = 0;
    spinBox->setRange(0, 1234567890);
    spinBox->setValue(spinBox->maximum());
    qApp->processEvents();
    QTRY_VERIFY(spinBox->sizeHintRequests > 0);

    delete widget;
}

void tst_QSpinBox::taskQTBUG_5008_textFromValueAndValidate()
{
    class DecoratedSpinBox : public QSpinBox
    {
    public:
        DecoratedSpinBox()
        {
            setLocale(QLocale::French);
            setMaximum(100000000);
            setValue(1000000);
        }

        QLineEdit *lineEdit() const
        {
            return QSpinBox::lineEdit();
        }

        //we use the French delimiters here
        QString textFromValue (int value) const
        {
            return locale().toString(value);
        }

    } spinbox;
    spinbox.show();
    spinbox.activateWindow();
    spinbox.setFocus();
    QApplication::setActiveWindow(&spinbox);
    QVERIFY(QTest::qWaitForWindowActive(&spinbox));
    QVERIFY(spinbox.hasFocus());
    QTRY_COMPARE(static_cast<QWidget *>(&spinbox), QApplication::activeWindow());
    QCOMPARE(spinbox.text(), spinbox.locale().toString(spinbox.value()));
    spinbox.lineEdit()->setCursorPosition(2); //just after the first thousand separator
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_0); // let's insert a 0
    QCOMPARE(spinbox.value(), 10000000); //it's been multiplied by 10
    spinbox.clearFocus(); //make sure the value is correctly formatted
    QCOMPARE(spinbox.text(), spinbox.locale().toString(spinbox.value()));
}

void tst_QSpinBox::integerOverflow()
{
    QSpinBox sb;
    sb.setRange(INT_MIN, INT_MAX);

    sb.setValue(INT_MAX - 1);
    sb.stepUp();
    QCOMPARE(sb.value(), INT_MAX);
    sb.stepUp();
    QCOMPARE(sb.value(), INT_MAX);

    sb.setValue(INT_MIN + 1);
    sb.stepDown();
    QCOMPARE(sb.value(), INT_MIN);
    sb.stepDown();
    QCOMPARE(sb.value(), INT_MIN);

    sb.setValue(0);
    QCOMPARE(sb.value(), 0);
    sb.setSingleStep(INT_MAX);
    sb.stepUp();
    QCOMPARE(sb.value(), INT_MAX);
    sb.stepDown();
    QCOMPARE(sb.value(), 0);
    sb.stepDown();
    QCOMPARE(sb.value(), INT_MIN + 1);
    sb.stepDown();
    QCOMPARE(sb.value(), INT_MIN);
}

void tst_QSpinBox::lineEditReturnPressed()
{
    SpinBox spinBox;
    QSignalSpy spyCurrentChanged(spinBox.lineEdit(), SIGNAL(returnPressed()));
    spinBox.show();
    QTest::keyClick(&spinBox, Qt::Key_Return);
    QCOMPARE(spyCurrentChanged.count(), 1);
}

void tst_QSpinBox::positiveSign()
{
    QSpinBox spinBox;
    spinBox.setRange(-20, 20);
    spinBox.setValue(-20);
    spinBox.show();
    QVERIFY(QTest::qWaitForWindowActive(&spinBox));

    QTest::keyClick(&spinBox, Qt::Key_End, Qt::ShiftModifier);
    QTest::keyClick(&spinBox, Qt::Key_Plus, Qt::ShiftModifier);
    QTest::keyClick(&spinBox, Qt::Key_2);
    QTest::keyClick(&spinBox, Qt::Key_0);
    QCOMPARE(spinBox.text(), QLatin1String("+20"));
}

void tst_QSpinBox::setGroupSeparatorShown_data()
{
    QTest::addColumn<QLocale::Language>("lang");
    QTest::addColumn<QLocale::Country>("country");

    QTest::newRow("data0") << QLocale::English << QLocale::UnitedStates;
    QTest::newRow("data1") << QLocale::Swedish << QLocale::Sweden;
    QTest::newRow("data2") << QLocale::German << QLocale::Germany;
    QTest::newRow("data3") << QLocale::Georgian << QLocale::Georgia;
    QTest::newRow("data3") << QLocale::Macedonian << QLocale::Macedonia;
}

void tst_QSpinBox::setGroupSeparatorShown()
{
    QFETCH(QLocale::Language, lang);
    QFETCH(QLocale::Country, country);

    QLocale loc(lang, country);
    QLocale::setDefault(loc);
    SpinBox spinBox;
    spinBox.setMaximum(99999);
    spinBox.setValue(13000);
    spinBox.setGroupSeparatorShown(true);
    QCOMPARE(spinBox.lineEdit()->text(), spinBox.locale().toString(13000));
    QCOMPARE(spinBox.isGroupSeparatorShown(), true);
    QCOMPARE(spinBox.textFromValue(23421),spinBox.locale().toString(23421));

    spinBox.setGroupSeparatorShown(false);
    QCOMPARE(spinBox.lineEdit()->text(), QStringLiteral("13000"));
    QCOMPARE(spinBox.isGroupSeparatorShown(), false);

    spinBox.setMaximum(72000);
    spinBox.lineEdit()->setText(spinBox.locale().toString(32000));
    QCOMPARE(spinBox.value()+1000, 33000);

    spinBox.lineEdit()->setText(QStringLiteral("32000"));
    QCOMPARE(spinBox.value()+1000, 33000);

    spinBox.lineEdit()->setText(QStringLiteral("32,000"));
    QCOMPARE(spinBox.value()+1000, 33000);
}

void tst_QSpinBox::wheelEvents()
{
#ifndef QT_NO_WHEELEVENT
    SpinBox spinBox;
    spinBox.setRange(-20, 20);
    spinBox.setValue(0);

    QWheelEvent wheelUp(QPointF(), QPointF(), QPoint(), QPoint(0, 120), 120, Qt::Vertical, Qt::NoButton, Qt::NoModifier);
    spinBox.wheelEvent(&wheelUp);
    QCOMPARE(spinBox.value(), 1);

    QWheelEvent wheelDown(QPointF(), QPointF(), QPoint(), QPoint(0, -120), -120, Qt::Vertical, Qt::NoButton, Qt::NoModifier);
    spinBox.wheelEvent(&wheelDown);
    spinBox.wheelEvent(&wheelDown);
    QCOMPARE(spinBox.value(), -1);

    QWheelEvent wheelHalfUp(QPointF(), QPointF(), QPoint(), QPoint(0, 60), 60,  Qt::Vertical, Qt::NoButton, Qt::NoModifier);
    spinBox.wheelEvent(&wheelHalfUp);
    QCOMPARE(spinBox.value(), -1);
    spinBox.wheelEvent(&wheelHalfUp);
    QCOMPARE(spinBox.value(), 0);
#endif
}

QTEST_MAIN(tst_QSpinBox)
#include "tst_qspinbox.moc"
