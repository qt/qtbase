/********************************************************************************
** Form generated from reading UI file 'bookwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef BOOKWINDOW_H
#define BOOKWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_BookWindow
{
public:
    QWidget *centralWidget;
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QVBoxLayout *vboxLayout1;
    QTableView *bookTable;
    QGroupBox *groupBox_2;
    QFormLayout *formLayout;
    QLabel *label_5;
    QLineEdit *titleEdit;
    QLabel *label_2_2_2_2;
    QComboBox *authorEdit;
    QLabel *label_3;
    QComboBox *genreEdit;
    QLabel *label_4;
    QSpinBox *yearEdit;
    QLabel *label;
    QSpinBox *ratingEdit;

    void setupUi(QMainWindow *BookWindow)
    {
        if (BookWindow->objectName().isEmpty())
            BookWindow->setObjectName(QStringLiteral("BookWindow"));
        BookWindow->resize(601, 420);
        centralWidget = new QWidget(BookWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        vboxLayout = new QVBoxLayout(centralWidget);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        groupBox = new QGroupBox(centralWidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        vboxLayout1 = new QVBoxLayout(groupBox);
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        bookTable = new QTableView(groupBox);
        bookTable->setObjectName(QStringLiteral("bookTable"));
        bookTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        vboxLayout1->addWidget(bookTable);

        groupBox_2 = new QGroupBox(groupBox);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        formLayout = new QFormLayout(groupBox_2);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        label_5 = new QLabel(groupBox_2);
        label_5->setObjectName(QStringLiteral("label_5"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label_5);

        titleEdit = new QLineEdit(groupBox_2);
        titleEdit->setObjectName(QStringLiteral("titleEdit"));
        titleEdit->setEnabled(true);

        formLayout->setWidget(0, QFormLayout::FieldRole, titleEdit);

        label_2_2_2_2 = new QLabel(groupBox_2);
        label_2_2_2_2->setObjectName(QStringLiteral("label_2_2_2_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2_2_2_2);

        authorEdit = new QComboBox(groupBox_2);
        authorEdit->setObjectName(QStringLiteral("authorEdit"));
        authorEdit->setEnabled(true);

        formLayout->setWidget(1, QFormLayout::FieldRole, authorEdit);

        label_3 = new QLabel(groupBox_2);
        label_3->setObjectName(QStringLiteral("label_3"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        genreEdit = new QComboBox(groupBox_2);
        genreEdit->setObjectName(QStringLiteral("genreEdit"));
        genreEdit->setEnabled(true);

        formLayout->setWidget(2, QFormLayout::FieldRole, genreEdit);

        label_4 = new QLabel(groupBox_2);
        label_4->setObjectName(QStringLiteral("label_4"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_4);

        yearEdit = new QSpinBox(groupBox_2);
        yearEdit->setObjectName(QStringLiteral("yearEdit"));
        yearEdit->setEnabled(true);
        yearEdit->setMaximum(2100);
        yearEdit->setMinimum(-1000);

        formLayout->setWidget(3, QFormLayout::FieldRole, yearEdit);

        label = new QLabel(groupBox_2);
        label->setObjectName(QStringLiteral("label"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label);

        ratingEdit = new QSpinBox(groupBox_2);
        ratingEdit->setObjectName(QStringLiteral("ratingEdit"));
        ratingEdit->setMaximum(5);

        formLayout->setWidget(4, QFormLayout::FieldRole, ratingEdit);


        vboxLayout1->addWidget(groupBox_2);


        vboxLayout->addWidget(groupBox);

        BookWindow->setCentralWidget(centralWidget);
        QWidget::setTabOrder(bookTable, titleEdit);
        QWidget::setTabOrder(titleEdit, authorEdit);
        QWidget::setTabOrder(authorEdit, genreEdit);
        QWidget::setTabOrder(genreEdit, yearEdit);

        retranslateUi(BookWindow);

        QMetaObject::connectSlotsByName(BookWindow);
    } // setupUi

    void retranslateUi(QMainWindow *BookWindow)
    {
        BookWindow->setWindowTitle(QApplication::translate("BookWindow", "Books", 0));
        groupBox->setTitle(QApplication::translate("BookWindow", "Books", 0));
        groupBox_2->setTitle(QApplication::translate("BookWindow", "Details", 0));
        label_5->setText(QApplication::translate("BookWindow", "<b>Title:</b>", 0));
        label_2_2_2_2->setText(QApplication::translate("BookWindow", "<b>Author: </b>", 0));
        label_3->setText(QApplication::translate("BookWindow", "<b>Genre:</b>", 0));
        label_4->setText(QApplication::translate("BookWindow", "<b>Year:</b>", 0));
        yearEdit->setPrefix(QString());
        label->setText(QApplication::translate("BookWindow", "<b>Rating:</b>", 0));
    } // retranslateUi

};

namespace Ui {
    class BookWindow: public Ui_BookWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // BOOKWINDOW_H
