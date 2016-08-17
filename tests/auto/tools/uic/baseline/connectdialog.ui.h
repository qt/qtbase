/********************************************************************************
** Form generated from reading UI file 'connectdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ConnectDialog
{
public:
    QGridLayout *gridLayout;
    QGroupBox *signalGroupBox;
    QVBoxLayout *vboxLayout;
    QListWidget *signalList;
    QHBoxLayout *hboxLayout;
    QToolButton *editSignalsButton;
    QSpacerItem *spacerItem;
    QGroupBox *slotGroupBox;
    QVBoxLayout *vboxLayout1;
    QListWidget *slotList;
    QHBoxLayout *hboxLayout1;
    QToolButton *editSlotsButton;
    QSpacerItem *spacerItem1;
    QCheckBox *showAllCheckBox;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *ConnectDialog)
    {
        if (ConnectDialog->objectName().isEmpty())
            ConnectDialog->setObjectName(QStringLiteral("ConnectDialog"));
        ConnectDialog->resize(585, 361);
        gridLayout = new QGridLayout(ConnectDialog);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        signalGroupBox = new QGroupBox(ConnectDialog);
        signalGroupBox->setObjectName(QStringLiteral("signalGroupBox"));
        vboxLayout = new QVBoxLayout(signalGroupBox);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        signalList = new QListWidget(signalGroupBox);
        signalList->setObjectName(QStringLiteral("signalList"));
        signalList->setTextElideMode(Qt::ElideMiddle);

        vboxLayout->addWidget(signalList);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        editSignalsButton = new QToolButton(signalGroupBox);
        editSignalsButton->setObjectName(QStringLiteral("editSignalsButton"));

        hboxLayout->addWidget(editSignalsButton);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout->addLayout(hboxLayout);


        gridLayout->addWidget(signalGroupBox, 0, 0, 1, 2);

        slotGroupBox = new QGroupBox(ConnectDialog);
        slotGroupBox->setObjectName(QStringLiteral("slotGroupBox"));
        vboxLayout1 = new QVBoxLayout(slotGroupBox);
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        slotList = new QListWidget(slotGroupBox);
        slotList->setObjectName(QStringLiteral("slotList"));
        slotList->setTextElideMode(Qt::ElideMiddle);

        vboxLayout1->addWidget(slotList);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        editSlotsButton = new QToolButton(slotGroupBox);
        editSlotsButton->setObjectName(QStringLiteral("editSlotsButton"));

        hboxLayout1->addWidget(editSlotsButton);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem1);


        vboxLayout1->addLayout(hboxLayout1);


        gridLayout->addWidget(slotGroupBox, 0, 2, 1, 1);

        showAllCheckBox = new QCheckBox(ConnectDialog);
        showAllCheckBox->setObjectName(QStringLiteral("showAllCheckBox"));

        gridLayout->addWidget(showAllCheckBox, 1, 0, 1, 1);

        buttonBox = new QDialogButtonBox(ConnectDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 2, 1, 1, 2);


        retranslateUi(ConnectDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), ConnectDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), ConnectDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(ConnectDialog);
    } // setupUi

    void retranslateUi(QDialog *ConnectDialog)
    {
        ConnectDialog->setWindowTitle(QApplication::translate("ConnectDialog", "Configure Connection", Q_NULLPTR));
        signalGroupBox->setTitle(QApplication::translate("ConnectDialog", "GroupBox", Q_NULLPTR));
        editSignalsButton->setText(QApplication::translate("ConnectDialog", "Edit...", Q_NULLPTR));
        slotGroupBox->setTitle(QApplication::translate("ConnectDialog", "GroupBox", Q_NULLPTR));
        editSlotsButton->setText(QApplication::translate("ConnectDialog", "Edit...", Q_NULLPTR));
        showAllCheckBox->setText(QApplication::translate("ConnectDialog", "Show signals and slots inherited from QWidget", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class ConnectDialog: public Ui_ConnectDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CONNECTDIALOG_H
