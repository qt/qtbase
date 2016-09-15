/********************************************************************************
** Form generated from reading UI file 'addlinkdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ADDLINKDIALOG_H
#define ADDLINKDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHeaderView>
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
            AddLinkDialog->setObjectName(QStringLiteral("AddLinkDialog"));
        AddLinkDialog->setSizeGripEnabled(false);
        AddLinkDialog->setModal(true);
        verticalLayout = new QVBoxLayout(AddLinkDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        label = new QLabel(AddLinkDialog);
        label->setObjectName(QStringLiteral("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        titleInput = new QLineEdit(AddLinkDialog);
        titleInput->setObjectName(QStringLiteral("titleInput"));
        titleInput->setMinimumSize(QSize(337, 0));

        formLayout->setWidget(0, QFormLayout::FieldRole, titleInput);

        label_2 = new QLabel(AddLinkDialog);
        label_2->setObjectName(QStringLiteral("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        urlInput = new QLineEdit(AddLinkDialog);
        urlInput->setObjectName(QStringLiteral("urlInput"));

        formLayout->setWidget(1, QFormLayout::FieldRole, urlInput);


        verticalLayout->addLayout(formLayout);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        line = new QFrame(AddLinkDialog);
        line->setObjectName(QStringLiteral("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(line);

        buttonBox = new QDialogButtonBox(AddLinkDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(AddLinkDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), AddLinkDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), AddLinkDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(AddLinkDialog);
    } // setupUi

    void retranslateUi(QDialog *AddLinkDialog)
    {
        AddLinkDialog->setWindowTitle(QApplication::translate("AddLinkDialog", "Insert Link", Q_NULLPTR));
        label->setText(QApplication::translate("AddLinkDialog", "Title:", Q_NULLPTR));
        label_2->setText(QApplication::translate("AddLinkDialog", "URL:", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class AddLinkDialog: public Ui_AddLinkDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ADDLINKDIALOG_H
