/********************************************************************************
** Form generated from reading UI file 'calculator.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
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
            Calculator->setObjectName(QStringLiteral("Calculator"));
        Calculator->resize(314, 301);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(Calculator->sizePolicy().hasHeightForWidth());
        Calculator->setSizePolicy(sizePolicy);
        Calculator->setMinimumSize(QSize(314, 301));
        Calculator->setMaximumSize(QSize(314, 301));
        backspaceButton = new QToolButton(Calculator);
        backspaceButton->setObjectName(QStringLiteral("backspaceButton"));
        backspaceButton->setGeometry(QRect(10, 50, 91, 41));
        clearButton = new QToolButton(Calculator);
        clearButton->setObjectName(QStringLiteral("clearButton"));
        clearButton->setGeometry(QRect(110, 50, 91, 41));
        clearAllButton = new QToolButton(Calculator);
        clearAllButton->setObjectName(QStringLiteral("clearAllButton"));
        clearAllButton->setGeometry(QRect(210, 50, 91, 41));
        clearMemoryButton = new QToolButton(Calculator);
        clearMemoryButton->setObjectName(QStringLiteral("clearMemoryButton"));
        clearMemoryButton->setGeometry(QRect(10, 100, 41, 41));
        readMemoryButton = new QToolButton(Calculator);
        readMemoryButton->setObjectName(QStringLiteral("readMemoryButton"));
        readMemoryButton->setGeometry(QRect(10, 150, 41, 41));
        setMemoryButton = new QToolButton(Calculator);
        setMemoryButton->setObjectName(QStringLiteral("setMemoryButton"));
        setMemoryButton->setGeometry(QRect(10, 200, 41, 41));
        addToMemoryButton = new QToolButton(Calculator);
        addToMemoryButton->setObjectName(QStringLiteral("addToMemoryButton"));
        addToMemoryButton->setGeometry(QRect(10, 250, 41, 41));
        sevenButton = new QToolButton(Calculator);
        sevenButton->setObjectName(QStringLiteral("sevenButton"));
        sevenButton->setGeometry(QRect(60, 100, 41, 41));
        eightButton = new QToolButton(Calculator);
        eightButton->setObjectName(QStringLiteral("eightButton"));
        eightButton->setGeometry(QRect(110, 100, 41, 41));
        nineButton = new QToolButton(Calculator);
        nineButton->setObjectName(QStringLiteral("nineButton"));
        nineButton->setGeometry(QRect(160, 100, 41, 41));
        fourButton = new QToolButton(Calculator);
        fourButton->setObjectName(QStringLiteral("fourButton"));
        fourButton->setGeometry(QRect(60, 150, 41, 41));
        fiveButton = new QToolButton(Calculator);
        fiveButton->setObjectName(QStringLiteral("fiveButton"));
        fiveButton->setGeometry(QRect(110, 150, 41, 41));
        sixButton = new QToolButton(Calculator);
        sixButton->setObjectName(QStringLiteral("sixButton"));
        sixButton->setGeometry(QRect(160, 150, 41, 41));
        oneButton = new QToolButton(Calculator);
        oneButton->setObjectName(QStringLiteral("oneButton"));
        oneButton->setGeometry(QRect(60, 200, 41, 41));
        twoButton = new QToolButton(Calculator);
        twoButton->setObjectName(QStringLiteral("twoButton"));
        twoButton->setGeometry(QRect(110, 200, 41, 41));
        threeButton = new QToolButton(Calculator);
        threeButton->setObjectName(QStringLiteral("threeButton"));
        threeButton->setGeometry(QRect(160, 200, 41, 41));
        zeroButton = new QToolButton(Calculator);
        zeroButton->setObjectName(QStringLiteral("zeroButton"));
        zeroButton->setGeometry(QRect(60, 250, 41, 41));
        pointButton = new QToolButton(Calculator);
        pointButton->setObjectName(QStringLiteral("pointButton"));
        pointButton->setGeometry(QRect(110, 250, 41, 41));
        changeSignButton = new QToolButton(Calculator);
        changeSignButton->setObjectName(QStringLiteral("changeSignButton"));
        changeSignButton->setGeometry(QRect(160, 250, 41, 41));
        plusButton = new QToolButton(Calculator);
        plusButton->setObjectName(QStringLiteral("plusButton"));
        plusButton->setGeometry(QRect(210, 250, 41, 41));
        divisionButton = new QToolButton(Calculator);
        divisionButton->setObjectName(QStringLiteral("divisionButton"));
        divisionButton->setGeometry(QRect(210, 100, 41, 41));
        timesButton = new QToolButton(Calculator);
        timesButton->setObjectName(QStringLiteral("timesButton"));
        timesButton->setGeometry(QRect(210, 150, 41, 41));
        minusButton = new QToolButton(Calculator);
        minusButton->setObjectName(QStringLiteral("minusButton"));
        minusButton->setGeometry(QRect(210, 200, 41, 41));
        squareRootButton = new QToolButton(Calculator);
        squareRootButton->setObjectName(QStringLiteral("squareRootButton"));
        squareRootButton->setGeometry(QRect(260, 100, 41, 41));
        powerButton = new QToolButton(Calculator);
        powerButton->setObjectName(QStringLiteral("powerButton"));
        powerButton->setGeometry(QRect(260, 150, 41, 41));
        reciprocalButton = new QToolButton(Calculator);
        reciprocalButton->setObjectName(QStringLiteral("reciprocalButton"));
        reciprocalButton->setGeometry(QRect(260, 200, 41, 41));
        equalButton = new QToolButton(Calculator);
        equalButton->setObjectName(QStringLiteral("equalButton"));
        equalButton->setGeometry(QRect(260, 250, 41, 41));
        display = new QLineEdit(Calculator);
        display->setObjectName(QStringLiteral("display"));
        display->setGeometry(QRect(10, 10, 291, 31));
        display->setMaxLength(15);
        display->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        display->setReadOnly(true);

        retranslateUi(Calculator);

        QMetaObject::connectSlotsByName(Calculator);
    } // setupUi

    void retranslateUi(QWidget *Calculator)
    {
        Calculator->setWindowTitle(QApplication::translate("Calculator", "Calculator", 0));
        backspaceButton->setText(QApplication::translate("Calculator", "Backspace", 0));
        clearButton->setText(QApplication::translate("Calculator", "Clear", 0));
        clearAllButton->setText(QApplication::translate("Calculator", "Clear All", 0));
        clearMemoryButton->setText(QApplication::translate("Calculator", "MC", 0));
        readMemoryButton->setText(QApplication::translate("Calculator", "MR", 0));
        setMemoryButton->setText(QApplication::translate("Calculator", "MS", 0));
        addToMemoryButton->setText(QApplication::translate("Calculator", "M+", 0));
        sevenButton->setText(QApplication::translate("Calculator", "7", 0));
        eightButton->setText(QApplication::translate("Calculator", "8", 0));
        nineButton->setText(QApplication::translate("Calculator", "9", 0));
        fourButton->setText(QApplication::translate("Calculator", "4", 0));
        fiveButton->setText(QApplication::translate("Calculator", "5", 0));
        sixButton->setText(QApplication::translate("Calculator", "6", 0));
        oneButton->setText(QApplication::translate("Calculator", "1", 0));
        twoButton->setText(QApplication::translate("Calculator", "2", 0));
        threeButton->setText(QApplication::translate("Calculator", "3", 0));
        zeroButton->setText(QApplication::translate("Calculator", "0", 0));
        pointButton->setText(QApplication::translate("Calculator", ".", 0));
        changeSignButton->setText(QApplication::translate("Calculator", "+-", 0));
        plusButton->setText(QApplication::translate("Calculator", "+", 0));
        divisionButton->setText(QApplication::translate("Calculator", "/", 0));
        timesButton->setText(QApplication::translate("Calculator", "*", 0));
        minusButton->setText(QApplication::translate("Calculator", "-", 0));
        squareRootButton->setText(QApplication::translate("Calculator", "Sqrt", 0));
        powerButton->setText(QApplication::translate("Calculator", "x^2", 0));
        reciprocalButton->setText(QApplication::translate("Calculator", "1/x", 0));
        equalButton->setText(QApplication::translate("Calculator", "=", 0));
    } // retranslateUi

};

namespace Ui {
    class Calculator: public Ui_Calculator {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CALCULATOR_H
