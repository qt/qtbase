/********************************************************************************
** Form generated from reading UI file 'calculator.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Calculator
{
public:
    QToolButton *backspaceButton;
    QToolButton *clearButton;
    QToolButton *clearAllButton;
    QToolButton *clearMemoryButton;
    QToolButton *readMemoryButton;
    QToolButton *setMemoryButton;
    QToolButton *addToMemoryButton;
    QToolButton *sevenButton;
    QToolButton *eightButton;
    QToolButton *nineButton;
    QToolButton *fourButton;
    QToolButton *fiveButton;
    QToolButton *sixButton;
    QToolButton *oneButton;
    QToolButton *twoButton;
    QToolButton *threeButton;
    QToolButton *zeroButton;
    QToolButton *pointButton;
    QToolButton *changeSignButton;
    QToolButton *plusButton;
    QToolButton *divisionButton;
    QToolButton *timesButton;
    QToolButton *minusButton;
    QToolButton *squareRootButton;
    QToolButton *powerButton;
    QToolButton *reciprocalButton;
    QToolButton *equalButton;
    QLineEdit *display;

    void setupUi(QWidget *Calculator)
    {
        if (Calculator->objectName().isEmpty())
            Calculator->setObjectName("Calculator");
        Calculator->resize(314, 301);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(Calculator->sizePolicy().hasHeightForWidth());
        Calculator->setSizePolicy(sizePolicy);
        Calculator->setMinimumSize(QSize(314, 301));
        Calculator->setMaximumSize(QSize(314, 301));
        backspaceButton = new QToolButton(Calculator);
        backspaceButton->setObjectName("backspaceButton");
        backspaceButton->setGeometry(QRect(10, 50, 91, 41));
        clearButton = new QToolButton(Calculator);
        clearButton->setObjectName("clearButton");
        clearButton->setGeometry(QRect(110, 50, 91, 41));
        clearAllButton = new QToolButton(Calculator);
        clearAllButton->setObjectName("clearAllButton");
        clearAllButton->setGeometry(QRect(210, 50, 91, 41));
        clearMemoryButton = new QToolButton(Calculator);
        clearMemoryButton->setObjectName("clearMemoryButton");
        clearMemoryButton->setGeometry(QRect(10, 100, 41, 41));
        readMemoryButton = new QToolButton(Calculator);
        readMemoryButton->setObjectName("readMemoryButton");
        readMemoryButton->setGeometry(QRect(10, 150, 41, 41));
        setMemoryButton = new QToolButton(Calculator);
        setMemoryButton->setObjectName("setMemoryButton");
        setMemoryButton->setGeometry(QRect(10, 200, 41, 41));
        addToMemoryButton = new QToolButton(Calculator);
        addToMemoryButton->setObjectName("addToMemoryButton");
        addToMemoryButton->setGeometry(QRect(10, 250, 41, 41));
        sevenButton = new QToolButton(Calculator);
        sevenButton->setObjectName("sevenButton");
        sevenButton->setGeometry(QRect(60, 100, 41, 41));
        eightButton = new QToolButton(Calculator);
        eightButton->setObjectName("eightButton");
        eightButton->setGeometry(QRect(110, 100, 41, 41));
        nineButton = new QToolButton(Calculator);
        nineButton->setObjectName("nineButton");
        nineButton->setGeometry(QRect(160, 100, 41, 41));
        fourButton = new QToolButton(Calculator);
        fourButton->setObjectName("fourButton");
        fourButton->setGeometry(QRect(60, 150, 41, 41));
        fiveButton = new QToolButton(Calculator);
        fiveButton->setObjectName("fiveButton");
        fiveButton->setGeometry(QRect(110, 150, 41, 41));
        sixButton = new QToolButton(Calculator);
        sixButton->setObjectName("sixButton");
        sixButton->setGeometry(QRect(160, 150, 41, 41));
        oneButton = new QToolButton(Calculator);
        oneButton->setObjectName("oneButton");
        oneButton->setGeometry(QRect(60, 200, 41, 41));
        twoButton = new QToolButton(Calculator);
        twoButton->setObjectName("twoButton");
        twoButton->setGeometry(QRect(110, 200, 41, 41));
        threeButton = new QToolButton(Calculator);
        threeButton->setObjectName("threeButton");
        threeButton->setGeometry(QRect(160, 200, 41, 41));
        zeroButton = new QToolButton(Calculator);
        zeroButton->setObjectName("zeroButton");
        zeroButton->setGeometry(QRect(60, 250, 41, 41));
        pointButton = new QToolButton(Calculator);
        pointButton->setObjectName("pointButton");
        pointButton->setGeometry(QRect(110, 250, 41, 41));
        changeSignButton = new QToolButton(Calculator);
        changeSignButton->setObjectName("changeSignButton");
        changeSignButton->setGeometry(QRect(160, 250, 41, 41));
        plusButton = new QToolButton(Calculator);
        plusButton->setObjectName("plusButton");
        plusButton->setGeometry(QRect(210, 250, 41, 41));
        divisionButton = new QToolButton(Calculator);
        divisionButton->setObjectName("divisionButton");
        divisionButton->setGeometry(QRect(210, 100, 41, 41));
        timesButton = new QToolButton(Calculator);
        timesButton->setObjectName("timesButton");
        timesButton->setGeometry(QRect(210, 150, 41, 41));
        minusButton = new QToolButton(Calculator);
        minusButton->setObjectName("minusButton");
        minusButton->setGeometry(QRect(210, 200, 41, 41));
        squareRootButton = new QToolButton(Calculator);
        squareRootButton->setObjectName("squareRootButton");
        squareRootButton->setGeometry(QRect(260, 100, 41, 41));
        powerButton = new QToolButton(Calculator);
        powerButton->setObjectName("powerButton");
        powerButton->setGeometry(QRect(260, 150, 41, 41));
        reciprocalButton = new QToolButton(Calculator);
        reciprocalButton->setObjectName("reciprocalButton");
        reciprocalButton->setGeometry(QRect(260, 200, 41, 41));
        equalButton = new QToolButton(Calculator);
        equalButton->setObjectName("equalButton");
        equalButton->setGeometry(QRect(260, 250, 41, 41));
        display = new QLineEdit(Calculator);
        display->setObjectName("display");
        display->setGeometry(QRect(10, 10, 291, 31));
        display->setMaxLength(15);
        display->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        display->setReadOnly(true);

        retranslateUi(Calculator);

        QMetaObject::connectSlotsByName(Calculator);
    } // setupUi

    void retranslateUi(QWidget *Calculator)
    {
        Calculator->setWindowTitle(QCoreApplication::translate("Calculator", "Calculator", nullptr));
        backspaceButton->setText(QCoreApplication::translate("Calculator", "Backspace", nullptr));
        clearButton->setText(QCoreApplication::translate("Calculator", "Clear", nullptr));
        clearAllButton->setText(QCoreApplication::translate("Calculator", "Clear All", nullptr));
        clearMemoryButton->setText(QCoreApplication::translate("Calculator", "MC", nullptr));
        readMemoryButton->setText(QCoreApplication::translate("Calculator", "MR", nullptr));
        setMemoryButton->setText(QCoreApplication::translate("Calculator", "MS", nullptr));
        addToMemoryButton->setText(QCoreApplication::translate("Calculator", "M+", nullptr));
        sevenButton->setText(QCoreApplication::translate("Calculator", "7", nullptr));
        eightButton->setText(QCoreApplication::translate("Calculator", "8", nullptr));
        nineButton->setText(QCoreApplication::translate("Calculator", "9", nullptr));
        fourButton->setText(QCoreApplication::translate("Calculator", "4", nullptr));
        fiveButton->setText(QCoreApplication::translate("Calculator", "5", nullptr));
        sixButton->setText(QCoreApplication::translate("Calculator", "6", nullptr));
        oneButton->setText(QCoreApplication::translate("Calculator", "1", nullptr));
        twoButton->setText(QCoreApplication::translate("Calculator", "2", nullptr));
        threeButton->setText(QCoreApplication::translate("Calculator", "3", nullptr));
        zeroButton->setText(QCoreApplication::translate("Calculator", "0", nullptr));
        pointButton->setText(QCoreApplication::translate("Calculator", ".", nullptr));
        changeSignButton->setText(QCoreApplication::translate("Calculator", "+-", nullptr));
        plusButton->setText(QCoreApplication::translate("Calculator", "+", nullptr));
        divisionButton->setText(QCoreApplication::translate("Calculator", "/", nullptr));
        timesButton->setText(QCoreApplication::translate("Calculator", "*", nullptr));
        minusButton->setText(QCoreApplication::translate("Calculator", "-", nullptr));
        squareRootButton->setText(QCoreApplication::translate("Calculator", "Sqrt", nullptr));
        powerButton->setText(QCoreApplication::translate("Calculator", "x^2", nullptr));
        reciprocalButton->setText(QCoreApplication::translate("Calculator", "1/x", nullptr));
        equalButton->setText(QCoreApplication::translate("Calculator", "=", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Calculator: public Ui_Calculator {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CALCULATOR_H
