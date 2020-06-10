/********************************************************************************
** Form generated from reading UI file 'dialog.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef DIALOG_H
#define DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QGridLayout *gridLayout;
    QPushButton *loadFromFileButton;
    QLabel *label;
    QPushButton *loadFromSharedMemoryButton;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName("Dialog");
        Dialog->resize(451, 322);
        gridLayout = new QGridLayout(Dialog);
        gridLayout->setObjectName("gridLayout");
        loadFromFileButton = new QPushButton(Dialog);
        loadFromFileButton->setObjectName("loadFromFileButton");

        gridLayout->addWidget(loadFromFileButton, 0, 0, 1, 1);

        label = new QLabel(Dialog);
        label->setObjectName("label");
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);

        gridLayout->addWidget(label, 1, 0, 1, 1);

        loadFromSharedMemoryButton = new QPushButton(Dialog);
        loadFromSharedMemoryButton->setObjectName("loadFromSharedMemoryButton");

        gridLayout->addWidget(loadFromSharedMemoryButton, 2, 0, 1, 1);


        retranslateUi(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QCoreApplication::translate("Dialog", "Dialog", nullptr));
        loadFromFileButton->setText(QCoreApplication::translate("Dialog", "Load Image From File...", nullptr));
        label->setText(QCoreApplication::translate("Dialog", "Launch two of these dialogs.  In the first, press the top button and load an image from a file.  In the second, press the bottom button and display the loaded image from shared memory.", nullptr));
        loadFromSharedMemoryButton->setText(QCoreApplication::translate("Dialog", "Display Image From Shared Memory", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // DIALOG_H
