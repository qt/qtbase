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
#include <QString>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <qinputdialog.h>

class tst_QInputDialog : public QObject
{
    Q_OBJECT
    QWidget *parent;
    QDialog::DialogCode doneCode;
    void (*testFunc)(QInputDialog *);
    static void testFuncGetInt(QInputDialog *dialog);
    static void testFuncGetDouble(QInputDialog *dialog);
    static void testFuncGetText(QInputDialog *dialog);
    static void testFuncGetItem(QInputDialog *dialog);
    void timerEvent(QTimerEvent *event);
private slots:
    void getInt_data();
    void getInt();
    void getDouble_data();
    void getDouble();
    void task255502getDouble();
    void getText_data();
    void getText();
    void getItem_data();
    void getItem();
    void task256299_getTextReturnNullStringOnRejected();
    void inputMethodHintsOfChildWidget();
};

QString stripFraction(const QString &s)
{
    int period;
    if (s.contains('.'))
        period = s.indexOf('.');
    else if (s.contains(','))
        period = s.indexOf(',');
    else
        return s;
    int end;
    for (end = s.size() - 1; end > period && s[end] == '0'; --end) ;
    return s.left(end + (end == period ? 0 : 1));
}

QString normalizeNumericString(const QString &s)
{
    return stripFraction(s); // assumed to be sufficient
}

void _keyClick(QWidget *widget, char key)
{
    QTest::keyClick(widget, key);
}

void _keyClick(QWidget *widget, Qt::Key key)
{
    QTest::keyClick(widget, key);
}

template <typename SpinBoxType>
void testTypingValue(
    SpinBoxType* sbox, QPushButton *okButton, const QString &value)
{
    sbox->selectAll();
    for (int i = 0; i < value.size(); ++i) {
        const QChar valChar = value[i];
	_keyClick(static_cast<QWidget *>(sbox), valChar.toLatin1()); // ### always guaranteed to work?
        if (sbox->hasAcceptableInput())
            QVERIFY(okButton->isEnabled());
        else
            QVERIFY(!okButton->isEnabled());
    }
}

void testTypingValue(QLineEdit *ledit, QPushButton *okButton, const QString &value)
{
    ledit->selectAll();
    for (int i = 0; i < value.size(); ++i) {
        const QChar valChar = value[i];
	_keyClick(ledit, valChar.toLatin1()); // ### always guaranteed to work?
        QVERIFY(ledit->hasAcceptableInput());
        QVERIFY(okButton->isEnabled());
    }
}

template <typename SpinBoxType, typename ValueType>
void testInvalidateAndRestore(
    SpinBoxType* sbox, QPushButton *okButton, QLineEdit *ledit, ValueType * = 0)
{
    const ValueType lastValidValue = sbox->value();

    sbox->selectAll();
    _keyClick(ledit, Qt::Key_Delete);
    QVERIFY(!sbox->hasAcceptableInput());
    QVERIFY(!okButton->isEnabled());

    _keyClick(ledit, Qt::Key_Return); // should work with Qt::Key_Enter too
    QVERIFY(sbox->hasAcceptableInput());
    QVERIFY(okButton->isEnabled());
    QCOMPARE(sbox->value(), lastValidValue);
    QLocale loc;
    QCOMPARE(
        normalizeNumericString(ledit->text()),
        normalizeNumericString(loc.toString(sbox->value())));
}

template <typename SpinBoxType, typename ValueType>
void testGetNumeric(QInputDialog *dialog, SpinBoxType * = 0, ValueType * = 0)
{
    SpinBoxType *sbox = dialog->findChild<SpinBoxType *>();
    QVERIFY(sbox != 0);

    QLineEdit *ledit = static_cast<QObject *>(sbox)->findChild<QLineEdit *>();
    QVERIFY(ledit != 0);

    QDialogButtonBox *bbox = dialog->findChild<QDialogButtonBox *>();
    QVERIFY(bbox != 0);
    QPushButton *okButton = bbox->button(QDialogButtonBox::Ok);
    QVERIFY(okButton != 0);

    QVERIFY(sbox->value() >= sbox->minimum());
    QVERIFY(sbox->value() <= sbox->maximum());
    QVERIFY(sbox->hasAcceptableInput());
    QLocale loc;
    QCOMPARE(
        normalizeNumericString(ledit->selectedText()),
        normalizeNumericString(loc.toString(sbox->value())));
    QVERIFY(okButton->isEnabled());

    const ValueType origValue = sbox->value();

    testInvalidateAndRestore<SpinBoxType, ValueType>(sbox, okButton, ledit);
    testTypingValue<SpinBoxType>(sbox, okButton, QString("%1").arg(sbox->minimum()));
    testTypingValue<SpinBoxType>(sbox, okButton, QString("%1").arg(sbox->maximum()));
    testTypingValue<SpinBoxType>(sbox, okButton, QString("%1").arg(sbox->minimum() - 1));
    testTypingValue<SpinBoxType>(sbox, okButton, QString("%1").arg(sbox->maximum() + 1));
    testTypingValue<SpinBoxType>(sbox, okButton, "0");
    testTypingValue<SpinBoxType>(sbox, okButton, "0.0");
    testTypingValue<SpinBoxType>(sbox, okButton, "foobar");

    testTypingValue<SpinBoxType>(sbox, okButton, loc.toString(origValue));
}

void testGetText(QInputDialog *dialog)
{
    QLineEdit *ledit = dialog->findChild<QLineEdit *>();
    QVERIFY(ledit);

    QDialogButtonBox *bbox = dialog->findChild<QDialogButtonBox *>();
    QVERIFY(bbox);
    QPushButton *okButton = bbox->button(QDialogButtonBox::Ok);
    QVERIFY(okButton);

    QVERIFY(ledit->hasAcceptableInput());
    QCOMPARE(ledit->selectedText(), ledit->text());
    QVERIFY(okButton->isEnabled());
    const QString origValue = ledit->text();

    testTypingValue(ledit, okButton, origValue);
}

void testGetItem(QInputDialog *dialog)
{
    QComboBox *cbox = dialog->findChild<QComboBox *>();
    QVERIFY(cbox);

    QDialogButtonBox *bbox = dialog->findChild<QDialogButtonBox *>();
    QVERIFY(bbox);
    QPushButton *okButton = bbox->button(QDialogButtonBox::Ok);
    QVERIFY(okButton);

    QVERIFY(okButton->isEnabled());
    const int origIndex = cbox->currentIndex();
    cbox->setCurrentIndex(origIndex - 1);
    cbox->setCurrentIndex(origIndex);
    QVERIFY(okButton->isEnabled());
}

void tst_QInputDialog::testFuncGetInt(QInputDialog *dialog)
{
    testGetNumeric<QSpinBox, int>(dialog);
}

void tst_QInputDialog::testFuncGetDouble(QInputDialog *dialog)
{
    testGetNumeric<QDoubleSpinBox, double>(dialog);
}

void tst_QInputDialog::testFuncGetText(QInputDialog *dialog)
{
    ::testGetText(dialog);
}

void tst_QInputDialog::testFuncGetItem(QInputDialog *dialog)
{
    ::testGetItem(dialog);
}

void tst_QInputDialog::timerEvent(QTimerEvent *event)
{
    killTimer(event->timerId());
    QInputDialog *dialog = parent->findChild<QInputDialog *>();
    QVERIFY(dialog);
    if (testFunc)
        testFunc(dialog);
    dialog->done(doneCode); // cause static function call to return
}

void tst_QInputDialog::getInt_data()
{
    QTest::addColumn<int>("min");
    QTest::addColumn<int>("max");
    QTest::newRow("getInt() - -") << -20 << -10;
    QTest::newRow("getInt() - 0") << -20 <<   0;
    QTest::newRow("getInt() - +") << -20 <<  20;
    QTest::newRow("getInt() 0 +") <<   0 <<  20;
    QTest::newRow("getInt() + +") <<  10 <<  20;
}

void tst_QInputDialog::getInt()
{
    QFETCH(int, min);
    QFETCH(int, max);
    QVERIFY(min < max);
    parent = new QWidget;
    doneCode = QDialog::Accepted;
    testFunc = &tst_QInputDialog::testFuncGetInt;
    startTimer(0);
    bool ok = false;
    const int value = min + (max - min) / 2;
    const int result = QInputDialog::getInt(parent, "", "", value, min, max, 1, &ok);
    QVERIFY(ok);
    QCOMPARE(result, value);
    delete parent;
}

void tst_QInputDialog::getDouble_data()
{
    QTest::addColumn<double>("min");
    QTest::addColumn<double>("max");
    QTest::addColumn<int>("decimals");
    QTest::newRow("getDouble() - - d0") << -20.0  << -10.0  << 0;
    QTest::newRow("getDouble() - 0 d0") << -20.0  <<   0.0  << 0;
    QTest::newRow("getDouble() - + d0") << -20.0  <<  20.0  << 0;
    QTest::newRow("getDouble() 0 + d0") <<   0.0  <<  20.0  << 0;
    QTest::newRow("getDouble() + + d0") <<  10.0  <<  20.0  << 0;
    QTest::newRow("getDouble() - - d1") << -20.5  << -10.5  << 1;
    QTest::newRow("getDouble() - 0 d1") << -20.5  <<   0.0  << 1;
    QTest::newRow("getDouble() - + d1") << -20.5  <<  20.5  << 1;
    QTest::newRow("getDouble() 0 + d1") <<   0.0  <<  20.5  << 1;
    QTest::newRow("getDouble() + + d1") <<  10.5  <<  20.5  << 1;
    QTest::newRow("getDouble() - - d2") << -20.05 << -10.05 << 2;
    QTest::newRow("getDouble() - 0 d2") << -20.05 <<   0.0  << 2;
    QTest::newRow("getDouble() - + d2") << -20.05 <<  20.05 << 2;
    QTest::newRow("getDouble() 0 + d2") <<   0.0  <<  20.05 << 2;
    QTest::newRow("getDouble() + + d2") <<  10.05 <<  20.05 << 2;
}

void tst_QInputDialog::getDouble()
{
    QFETCH(double, min);
    QFETCH(double, max);
    QFETCH(int, decimals);
    QVERIFY(min < max && decimals >= 0 && decimals <= 13);
    parent = new QWidget;
    doneCode = QDialog::Accepted;
    testFunc = &tst_QInputDialog::testFuncGetDouble;
    startTimer(0);
    bool ok = false;
    // avoid decimals due to inconsistent roundoff behavior in QInputDialog::getDouble()
    // (at one decimal, 10.25 is rounded off to 10.2, while at two decimals, 10.025 is
    // rounded off to 10.03)
    const double value = static_cast<int>(min + (max - min) / 2);
    const double result =
        QInputDialog::getDouble(parent, "", "", value, min, max, decimals, &ok);
    QVERIFY(ok);
    QCOMPARE(result, value);
    delete parent;
}

void tst_QInputDialog::task255502getDouble()
{
    parent = new QWidget;
    doneCode = QDialog::Accepted;
    testFunc = &tst_QInputDialog::testFuncGetDouble;
    startTimer(0);
    bool ok = false;
    const double value = 0.001;
    const double result =
        QInputDialog::getDouble(parent, "", "", value, -1, 1, 4, &ok);
    QVERIFY(ok);
    QCOMPARE(result, value);
    delete parent;
}

void tst_QInputDialog::getText_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("getText() 1") << "";
    QTest::newRow("getText() 2") << "foobar";
    QTest::newRow("getText() 3") << "  foobar";
    QTest::newRow("getText() 4") << "foobar  ";
    QTest::newRow("getText() 5") << "aAzZ`1234567890-=~!@#$%^&*()_+[]{}\\|;:'\",.<>/?";
}

void tst_QInputDialog::getText()
{
    QFETCH(QString, text);
    parent = new QWidget;
    doneCode = QDialog::Accepted;
    testFunc = &tst_QInputDialog::testFuncGetText;
    startTimer(0);
    bool ok = false;
    const QString result = QInputDialog::getText(parent, "", "", QLineEdit::Normal, text, &ok);
    QVERIFY(ok);
    QCOMPARE(result, text);
    delete parent;
}

void tst_QInputDialog::task256299_getTextReturnNullStringOnRejected()
{
    parent = new QWidget;
    doneCode = QDialog::Rejected;
    testFunc = 0;
    startTimer(0);
    bool ok = true;
    const QString result = QInputDialog::getText(parent, "", "", QLineEdit::Normal, "foobar", &ok);
    QVERIFY(!ok);
    QVERIFY(result.isNull());
    delete parent;
}

void tst_QInputDialog::getItem_data()
{
    QTest::addColumn<QStringList>("items");
    QTest::addColumn<bool>("editable");
    QTest::newRow("getItem() 1 true") << (QStringList() << "") << true;
    QTest::newRow("getItem() 2 true") <<
        (QStringList() << "spring" << "summer" << "fall" << "winter") << true;
    QTest::newRow("getItem() 1 false") << (QStringList() << "") << false;
    QTest::newRow("getItem() 2 false") <<
        (QStringList() << "spring" << "summer" << "fall" << "winter") << false;
}

void tst_QInputDialog::getItem()
{
    QFETCH(QStringList, items);
    QFETCH(bool, editable);
    parent = new QWidget;
    doneCode = QDialog::Accepted;
    testFunc = &tst_QInputDialog::testFuncGetItem;
    startTimer(0);
    bool ok = false;
    const int index = items.size() / 2;
    const QString result = QInputDialog::getItem(parent, "", "", items, index, editable, &ok);
    QVERIFY(ok);
    QCOMPARE(result, items[index]);
    delete parent;
}

void tst_QInputDialog::inputMethodHintsOfChildWidget()
{
    QInputDialog dialog;
    dialog.setInputMode(QInputDialog::TextInput);
    QList<QObject *> children = dialog.children();
    QLineEdit *editWidget = 0;
    for (int c = 0; c < children.size(); c++) {
        editWidget = qobject_cast<QLineEdit *>(children.at(c));
        if (editWidget)
            break;
    }
    QVERIFY(editWidget);
    QCOMPARE(editWidget->inputMethodHints(), dialog.inputMethodHints());
    QCOMPARE(editWidget->inputMethodHints(), Qt::ImhNone);
    dialog.setInputMethodHints(Qt::ImhDigitsOnly);
    QCOMPARE(editWidget->inputMethodHints(), dialog.inputMethodHints());
    QCOMPARE(editWidget->inputMethodHints(), Qt::ImhDigitsOnly);
}

QTEST_MAIN(tst_QInputDialog)
#include "tst_qinputdialog.moc"
