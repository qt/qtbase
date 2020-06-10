/********************************************************************************
** Form generated from reading UI file 'querywidget.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QUERYWIDGET_H
#define QUERYWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QueryWidget
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QVBoxLayout *vboxLayout;
    QGroupBox *inputGroupBox;
    QVBoxLayout *verticalLayout_4;
    QVBoxLayout *_2;
    QTextEdit *inputTextEdit;
    QGroupBox *queryGroupBox;
    QVBoxLayout *verticalLayout_5;
    QComboBox *defaultQueries;
    QTextEdit *queryTextEdit;
    QGroupBox *outputGroupBox;
    QVBoxLayout *verticalLayout_6;
    QVBoxLayout *_3;
    QTextEdit *outputTextEdit;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *QueryWidget)
    {
        if (QueryWidget->objectName().isEmpty())
            QueryWidget->setObjectName("QueryWidget");
        QueryWidget->resize(545, 531);
        centralwidget = new QWidget(QueryWidget);
        centralwidget->setObjectName("centralwidget");
        centralwidget->setGeometry(QRect(0, 29, 545, 480));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        vboxLayout = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        vboxLayout->setObjectName("vboxLayout");
        inputGroupBox = new QGroupBox(centralwidget);
        inputGroupBox->setObjectName("inputGroupBox");
        inputGroupBox->setMinimumSize(QSize(550, 120));
        verticalLayout_4 = new QVBoxLayout(inputGroupBox);
        verticalLayout_4->setObjectName("verticalLayout_4");
        _2 = new QVBoxLayout();
#ifndef Q_OS_MAC
        _2->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        _2->setContentsMargins(0, 0, 0, 0);
#endif
        _2->setObjectName("_2");
        inputTextEdit = new QTextEdit(inputGroupBox);
        inputTextEdit->setObjectName("inputTextEdit");

        _2->addWidget(inputTextEdit);


        verticalLayout_4->addLayout(_2);


        vboxLayout->addWidget(inputGroupBox);

        queryGroupBox = new QGroupBox(centralwidget);
        queryGroupBox->setObjectName("queryGroupBox");
        queryGroupBox->setMinimumSize(QSize(550, 120));
        verticalLayout_5 = new QVBoxLayout(queryGroupBox);
        verticalLayout_5->setObjectName("verticalLayout_5");
        defaultQueries = new QComboBox(queryGroupBox);
        defaultQueries->setObjectName("defaultQueries");

        verticalLayout_5->addWidget(defaultQueries);

        queryTextEdit = new QTextEdit(queryGroupBox);
        queryTextEdit->setObjectName("queryTextEdit");
        queryTextEdit->setMinimumSize(QSize(400, 60));
        queryTextEdit->setReadOnly(true);
        queryTextEdit->setAcceptRichText(false);

        verticalLayout_5->addWidget(queryTextEdit);


        vboxLayout->addWidget(queryGroupBox);

        outputGroupBox = new QGroupBox(centralwidget);
        outputGroupBox->setObjectName("outputGroupBox");
        outputGroupBox->setMinimumSize(QSize(550, 120));
        verticalLayout_6 = new QVBoxLayout(outputGroupBox);
        verticalLayout_6->setObjectName("verticalLayout_6");
        _3 = new QVBoxLayout();
#ifndef Q_OS_MAC
        _3->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        _3->setContentsMargins(0, 0, 0, 0);
#endif
        _3->setObjectName("_3");
        outputTextEdit = new QTextEdit(outputGroupBox);
        outputTextEdit->setObjectName("outputTextEdit");
        outputTextEdit->setMinimumSize(QSize(500, 80));
        outputTextEdit->setReadOnly(true);
        outputTextEdit->setAcceptRichText(false);

        _3->addWidget(outputTextEdit);


        verticalLayout_6->addLayout(_3);


        vboxLayout->addWidget(outputGroupBox);


        verticalLayout->addLayout(vboxLayout);

        QueryWidget->setCentralWidget(centralwidget);
        menubar = new QMenuBar(QueryWidget);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 545, 29));
        QueryWidget->setMenuBar(menubar);
        statusbar = new QStatusBar(QueryWidget);
        statusbar->setObjectName("statusbar");
        statusbar->setGeometry(QRect(0, 509, 545, 22));
        QueryWidget->setStatusBar(statusbar);

        retranslateUi(QueryWidget);

        QMetaObject::connectSlotsByName(QueryWidget);
    } // setupUi

    void retranslateUi(QMainWindow *QueryWidget)
    {
        QueryWidget->setWindowTitle(QCoreApplication::translate("QueryWidget", "Recipes XQuery Example", nullptr));
        inputGroupBox->setTitle(QCoreApplication::translate("QueryWidget", "Input Document", nullptr));
        queryGroupBox->setTitle(QCoreApplication::translate("QueryWidget", "Select your query:", nullptr));
        outputGroupBox->setTitle(QCoreApplication::translate("QueryWidget", "Output Document", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QueryWidget: public Ui_QueryWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QUERYWIDGET_H
