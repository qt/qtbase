// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE
class Button;

//! [0]
class Calculator : public QWidget
{
    Q_OBJECT

public:
    Calculator(QWidget *parent = nullptr);

private slots:
    void digitClicked();
    void unaryOperatorClicked();
    void additiveOperatorClicked();
    void multiplicativeOperatorClicked();
    void equalClicked();
    void pointClicked();
    void changeSignClicked();
    void backspaceClicked();
    void clear();
    void clearAll();
    void clearMemory();
    void readMemory();
    void setMemory();
    void addToMemory();
//! [0]

//! [1]
private:
//! [1] //! [2]
    template<typename PointerToMemberFunction>
    Button *createButton(const QString &text, const PointerToMemberFunction &member);
    void abortOperation();
    bool calculate(double rightOperand, const QString &pendingOperator);
//! [2]

//! [3]
    double sumInMemory;
//! [3] //! [4]
    double sumSoFar;
//! [4] //! [5]
    double factorSoFar;
//! [5] //! [6]
    QString pendingAdditiveOperator;
//! [6] //! [7]
    QString pendingMultiplicativeOperator;
//! [7] //! [8]
    bool waitingForOperand;
//! [8]

//! [9]
    QLineEdit *display;
//! [9] //! [10]

    enum { NumDigitButtons = 10 };
    Button *digitButtons[NumDigitButtons];
};
//! [10]

#endif
