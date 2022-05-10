// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    {
//! [0]
    QWidget *window = new QWidget;
//! [0] //! [1]
    QPushButton *button1 = new QPushButton("One");
//! [1] //! [2]
    QPushButton *button2 = new QPushButton("Two");
    QPushButton *button3 = new QPushButton("Three");
    QPushButton *button4 = new QPushButton("Four");
    QPushButton *button5 = new QPushButton("Five");
//! [2]

//! [3]
    QHBoxLayout *layout = new QHBoxLayout(window);
//! [3] //! [4]
    layout->addWidget(button1);
    layout->addWidget(button2);
    layout->addWidget(button3);
    layout->addWidget(button4);
    layout->addWidget(button5);

//! [4]
    window->setWindowTitle("QHBoxLayout");
//! [5]
    window->show();
//! [5]
    }

    {
//! [6]
    QWidget *window = new QWidget;
//! [6] //! [7]
    QPushButton *button1 = new QPushButton("One");
//! [7] //! [8]
    QPushButton *button2 = new QPushButton("Two");
    QPushButton *button3 = new QPushButton("Three");
    QPushButton *button4 = new QPushButton("Four");
    QPushButton *button5 = new QPushButton("Five");
//! [8]

//! [9]
    QVBoxLayout *layout = new QVBoxLayout(window);
//! [9] //! [10]
    layout->addWidget(button1);
    layout->addWidget(button2);
    layout->addWidget(button3);
    layout->addWidget(button4);
    layout->addWidget(button5);

//! [10]
    window->setWindowTitle("QVBoxLayout");
//! [11]
    window->show();
//! [11]
    }

    {
//! [12]
    QWidget *window = new QWidget;
//! [12] //! [13]
    QPushButton *button1 = new QPushButton("One");
//! [13] //! [14]
    QPushButton *button2 = new QPushButton("Two");
    QPushButton *button3 = new QPushButton("Three");
    QPushButton *button4 = new QPushButton("Four");
    QPushButton *button5 = new QPushButton("Five");
//! [14]

//! [15]
    QGridLayout *layout = new QGridLayout(window);
//! [15] //! [16]
    layout->addWidget(button1, 0, 0);
    layout->addWidget(button2, 0, 1);
    layout->addWidget(button3, 1, 0, 1, 2);
    layout->addWidget(button4, 2, 0);
    layout->addWidget(button5, 2, 1);

//! [16]
    window->setWindowTitle("QGridLayout");
//! [17]
    window->show();
//! [17]
    }

    {
//! [18]
    QWidget *window = new QWidget;
//! [18]
//! [19]
    QPushButton *button1 = new QPushButton("One");
    QLineEdit *lineEdit1 = new QLineEdit();
//! [19]
//! [20]
    QPushButton *button2 = new QPushButton("Two");
    QLineEdit *lineEdit2 = new QLineEdit();
    QPushButton *button3 = new QPushButton("Three");
    QLineEdit *lineEdit3 = new QLineEdit();
//! [20]
//! [21]
    QFormLayout *layout = new QFormLayout(window);
//! [21]
//! [22]
    layout->addRow(button1, lineEdit1);
    layout->addRow(button2, lineEdit2);
    layout->addRow(button3, lineEdit3);

//! [22]
    window->setWindowTitle("QFormLayout");
//! [23]
    window->show();
//! [23]
    }

    {
//! [24]
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(formWidget);
    setLayout(layout);
//! [24]
    }
    return app.exec();
}
