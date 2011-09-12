/********************************************************************************
** Form generated from reading UI file 'addlinkdialog.ui'
**
** Created: Fri Sep 4 10:17:12 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ADDLINKDIALOG_H
#define ADDLINKDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFormLayout>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>

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
            AddLinkDialog->setObjectName(QString::fromUtf8("AddLinkDialog"));
        AddLinkDialog->setSizeGripEnabled(false);
        AddLinkDialog->setModal(true);
        verticalLayout = new QVBoxLayout(AddLinkDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label = new QLabel(AddLinkDialog);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        titleInput = new QLineEdit(AddLinkDialog);
        titleInput->setObjectName(QString::fromUtf8("titleInput"));
        titleInput->setMinimumSize(QSize(337, 0));

        formLayout->setWidget(0, QFormLayout::FieldRole, titleInput);

        label_2 = new QLabel(AddLinkDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        urlInput = new QLineEdit(AddLinkDialog);
        urlInput->setObjectName(QString::fromUtf8("urlInput"));

        formLayout->setWidget(1, QFormLayout::FieldRole, urlInput);


        verticalLayout->addLayout(formLayout);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        line = new QFrame(AddLinkDialog);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(line);

        buttonBox = new QDialogButtonBox(AddLinkDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
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
        AddLinkDialog->setWindowTitle(QApplication::translate("AddLinkDialog", "Insert Link", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("AddLinkDialog", "Title:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("AddLinkDialog", "URL:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class AddLinkDialog: public Ui_AddLinkDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ADDLINKDIALOG_H
