/********************************************************************************
** Form generated from reading UI file 'signalslotdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SIGNALSLOTDIALOG_H
#define SIGNALSLOTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListView>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_SignalSlotDialogClass
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *slotGroupBox;
    QVBoxLayout *vboxLayout1;
    QListView *slotListView;
    QHBoxLayout *hboxLayout;
    QToolButton *addSlotButton;
    QToolButton *removeSlotButton;
    QSpacerItem *spacerItem;
    QGroupBox *signalGroupBox;
    QVBoxLayout *vboxLayout2;
    QListView *signalListView;
    QHBoxLayout *hboxLayout1;
    QToolButton *addSignalButton;
    QToolButton *removeSignalButton;
    QSpacerItem *spacerItem1;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *SignalSlotDialogClass)
    {
        if (SignalSlotDialogClass->objectName().isEmpty())
            SignalSlotDialogClass->setObjectName(QString::fromUtf8("SignalSlotDialogClass"));
        SignalSlotDialogClass->resize(617, 535);
        vboxLayout = new QVBoxLayout(SignalSlotDialogClass);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        slotGroupBox = new QGroupBox(SignalSlotDialogClass);
        slotGroupBox->setObjectName(QString::fromUtf8("slotGroupBox"));
        vboxLayout1 = new QVBoxLayout(slotGroupBox);
        vboxLayout1->setSpacing(6);
        vboxLayout1->setContentsMargins(11, 11, 11, 11);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        slotListView = new QListView(slotGroupBox);
        slotListView->setObjectName(QString::fromUtf8("slotListView"));

        vboxLayout1->addWidget(slotListView);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        addSlotButton = new QToolButton(slotGroupBox);
        addSlotButton->setObjectName(QString::fromUtf8("addSlotButton"));

        hboxLayout->addWidget(addSlotButton);

        removeSlotButton = new QToolButton(slotGroupBox);
        removeSlotButton->setObjectName(QString::fromUtf8("removeSlotButton"));

        hboxLayout->addWidget(removeSlotButton);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout1->addLayout(hboxLayout);


        vboxLayout->addWidget(slotGroupBox);

        signalGroupBox = new QGroupBox(SignalSlotDialogClass);
        signalGroupBox->setObjectName(QString::fromUtf8("signalGroupBox"));
        vboxLayout2 = new QVBoxLayout(signalGroupBox);
        vboxLayout2->setSpacing(6);
        vboxLayout2->setContentsMargins(11, 11, 11, 11);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        signalListView = new QListView(signalGroupBox);
        signalListView->setObjectName(QString::fromUtf8("signalListView"));

        vboxLayout2->addWidget(signalListView);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        addSignalButton = new QToolButton(signalGroupBox);
        addSignalButton->setObjectName(QString::fromUtf8("addSignalButton"));

        hboxLayout1->addWidget(addSignalButton);

        removeSignalButton = new QToolButton(signalGroupBox);
        removeSignalButton->setObjectName(QString::fromUtf8("removeSignalButton"));

        hboxLayout1->addWidget(removeSignalButton);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem1);


        vboxLayout2->addLayout(hboxLayout1);


        vboxLayout->addWidget(signalGroupBox);

        buttonBox = new QDialogButtonBox(SignalSlotDialogClass);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(SignalSlotDialogClass);

        QMetaObject::connectSlotsByName(SignalSlotDialogClass);
    } // setupUi

    void retranslateUi(QDialog *SignalSlotDialogClass)
    {
        SignalSlotDialogClass->setWindowTitle(QCoreApplication::translate("SignalSlotDialogClass", "Signals and slots", nullptr));
        slotGroupBox->setTitle(QCoreApplication::translate("SignalSlotDialogClass", "Slots", nullptr));
#if QT_CONFIG(tooltip)
        addSlotButton->setToolTip(QCoreApplication::translate("SignalSlotDialogClass", "Add", nullptr));
#endif // QT_CONFIG(tooltip)
        addSlotButton->setText(QCoreApplication::translate("SignalSlotDialogClass", "...", nullptr));
#if QT_CONFIG(tooltip)
        removeSlotButton->setToolTip(QCoreApplication::translate("SignalSlotDialogClass", "Delete", nullptr));
#endif // QT_CONFIG(tooltip)
        removeSlotButton->setText(QCoreApplication::translate("SignalSlotDialogClass", "...", nullptr));
        signalGroupBox->setTitle(QCoreApplication::translate("SignalSlotDialogClass", "Signals", nullptr));
#if QT_CONFIG(tooltip)
        addSignalButton->setToolTip(QCoreApplication::translate("SignalSlotDialogClass", "Add", nullptr));
#endif // QT_CONFIG(tooltip)
        addSignalButton->setText(QCoreApplication::translate("SignalSlotDialogClass", "...", nullptr));
#if QT_CONFIG(tooltip)
        removeSignalButton->setToolTip(QCoreApplication::translate("SignalSlotDialogClass", "Delete", nullptr));
#endif // QT_CONFIG(tooltip)
        removeSignalButton->setText(QCoreApplication::translate("SignalSlotDialogClass", "...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SignalSlotDialogClass: public Ui_SignalSlotDialogClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SIGNALSLOTDIALOG_H
