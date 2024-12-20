/********************************************************************************
** Form generated from reading UI file 'addlinkdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ADDLINKDIALOG_H
#define ADDLINKDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_AddLinkDialog
{
public:
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *titleInput;
    QLabel *label_2;
    QLineEdit *urlInput;
    QSpacerItem *verticalSpacer;
    QFrame *line;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AddLinkDialog)
    {
        if (AddLinkDialog->objectName().isEmpty())
            AddLinkDialog->setObjectName("AddLinkDialog");
        AddLinkDialog->setSizeGripEnabled(false);
        AddLinkDialog->setModal(true);
        verticalLayout = new QVBoxLayout(AddLinkDialog);
        verticalLayout->setObjectName("verticalLayout");
        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        label = new QLabel(AddLinkDialog);
        label->setObjectName("label");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, label);

        titleInput = new QLineEdit(AddLinkDialog);
        titleInput->setObjectName("titleInput");
        titleInput->setMinimumSize(QSize(337, 0));

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, titleInput);

        label_2 = new QLabel(AddLinkDialog);
        label_2->setObjectName("label_2");

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, label_2);

        urlInput = new QLineEdit(AddLinkDialog);
        urlInput->setObjectName("urlInput");

        formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, urlInput);


        verticalLayout->addLayout(formLayout);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        line = new QFrame(AddLinkDialog);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayout->addWidget(line);

        buttonBox = new QDialogButtonBox(AddLinkDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(AddLinkDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, AddLinkDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, AddLinkDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(AddLinkDialog);
    } // setupUi

    void retranslateUi(QDialog *AddLinkDialog)
    {
        AddLinkDialog->setWindowTitle(QCoreApplication::translate("AddLinkDialog", "Insert Link", nullptr));
        label->setText(QCoreApplication::translate("AddLinkDialog", "Title:", nullptr));
        label_2->setText(QCoreApplication::translate("AddLinkDialog", "URL:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AddLinkDialog: public Ui_AddLinkDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ADDLINKDIALOG_H
