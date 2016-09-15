/********************************************************************************
** Form generated from reading UI file 'querywidget.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QUERYWIDGET_H
#define QUERYWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
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
            QueryWidget->setObjectName(QStringLiteral("QueryWidget"));
        QueryWidget->resize(545, 531);
        centralwidget = new QWidget(QueryWidget);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        centralwidget->setGeometry(QRect(0, 29, 545, 480));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        vboxLayout = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        inputGroupBox = new QGroupBox(centralwidget);
        inputGroupBox->setObjectName(QStringLiteral("inputGroupBox"));
        inputGroupBox->setMinimumSize(QSize(550, 120));
        verticalLayout_4 = new QVBoxLayout(inputGroupBox);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        _2 = new QVBoxLayout();
#ifndef Q_OS_MAC
        _2->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        _2->setContentsMargins(0, 0, 0, 0);
#endif
        _2->setObjectName(QStringLiteral("_2"));
        inputTextEdit = new QTextEdit(inputGroupBox);
        inputTextEdit->setObjectName(QStringLiteral("inputTextEdit"));

        _2->addWidget(inputTextEdit);


        verticalLayout_4->addLayout(_2);


        vboxLayout->addWidget(inputGroupBox);

        queryGroupBox = new QGroupBox(centralwidget);
        queryGroupBox->setObjectName(QStringLiteral("queryGroupBox"));
        queryGroupBox->setMinimumSize(QSize(550, 120));
        verticalLayout_5 = new QVBoxLayout(queryGroupBox);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        defaultQueries = new QComboBox(queryGroupBox);
        defaultQueries->setObjectName(QStringLiteral("defaultQueries"));

        verticalLayout_5->addWidget(defaultQueries);

        queryTextEdit = new QTextEdit(queryGroupBox);
        queryTextEdit->setObjectName(QStringLiteral("queryTextEdit"));
        queryTextEdit->setMinimumSize(QSize(400, 60));
        queryTextEdit->setReadOnly(true);
        queryTextEdit->setAcceptRichText(false);

        verticalLayout_5->addWidget(queryTextEdit);


        vboxLayout->addWidget(queryGroupBox);

        outputGroupBox = new QGroupBox(centralwidget);
        outputGroupBox->setObjectName(QStringLiteral("outputGroupBox"));
        outputGroupBox->setMinimumSize(QSize(550, 120));
        verticalLayout_6 = new QVBoxLayout(outputGroupBox);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        _3 = new QVBoxLayout();
#ifndef Q_OS_MAC
        _3->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        _3->setContentsMargins(0, 0, 0, 0);
#endif
        _3->setObjectName(QStringLiteral("_3"));
        outputTextEdit = new QTextEdit(outputGroupBox);
        outputTextEdit->setObjectName(QStringLiteral("outputTextEdit"));
        outputTextEdit->setMinimumSize(QSize(500, 80));
        outputTextEdit->setReadOnly(true);
        outputTextEdit->setAcceptRichText(false);

        _3->addWidget(outputTextEdit);


        verticalLayout_6->addLayout(_3);


        vboxLayout->addWidget(outputGroupBox);


        verticalLayout->addLayout(vboxLayout);

        QueryWidget->setCentralWidget(centralwidget);
        menubar = new QMenuBar(QueryWidget);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 545, 29));
        QueryWidget->setMenuBar(menubar);
        statusbar = new QStatusBar(QueryWidget);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        statusbar->setGeometry(QRect(0, 509, 545, 22));
        QueryWidget->setStatusBar(statusbar);

        retranslateUi(QueryWidget);

        QMetaObject::connectSlotsByName(QueryWidget);
    } // setupUi

    void retranslateUi(QMainWindow *QueryWidget)
    {
        QueryWidget->setWindowTitle(QApplication::translate("QueryWidget", "Recipes XQuery Example", Q_NULLPTR));
        inputGroupBox->setTitle(QApplication::translate("QueryWidget", "Input Document", Q_NULLPTR));
        queryGroupBox->setTitle(QApplication::translate("QueryWidget", "Select your query:", Q_NULLPTR));
        outputGroupBox->setTitle(QApplication::translate("QueryWidget", "Output Document", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class QueryWidget: public Ui_QueryWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QUERYWIDGET_H
