/********************************************************************************
** Form generated from reading UI file 'installdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef INSTALLDIALOG_H
#define INSTALLDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>

QT_BEGIN_NAMESPACE

class Ui_InstallDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QListWidget *listWidget;
    QPushButton *installButton;
    QPushButton *cancelButton;
    QPushButton *closeButton;
    QSpacerItem *spacerItem;
    QLabel *label_4;
    QLineEdit *pathLineEdit;
    QToolButton *browseButton;
    QFrame *line;
    QLabel *statusLabel;
    QProgressBar *progressBar;

    void setupUi(QDialog *InstallDialog)
    {
        if (InstallDialog->objectName().isEmpty())
            InstallDialog->setObjectName(QString::fromUtf8("InstallDialog"));
        InstallDialog->resize(436, 245);
        gridLayout = new QGridLayout(InstallDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(InstallDialog);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 4);

        listWidget = new QListWidget(InstallDialog);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));

        gridLayout->addWidget(listWidget, 1, 0, 4, 4);

        installButton = new QPushButton(InstallDialog);
        installButton->setObjectName(QString::fromUtf8("installButton"));

        gridLayout->addWidget(installButton, 1, 4, 1, 1);

        cancelButton = new QPushButton(InstallDialog);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

        gridLayout->addWidget(cancelButton, 2, 4, 1, 1);

        closeButton = new QPushButton(InstallDialog);
        closeButton->setObjectName(QString::fromUtf8("closeButton"));

        gridLayout->addWidget(closeButton, 3, 4, 1, 1);

        spacerItem = new QSpacerItem(20, 56, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem, 4, 4, 1, 1);

        label_4 = new QLabel(InstallDialog);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 5, 0, 1, 1);

        pathLineEdit = new QLineEdit(InstallDialog);
        pathLineEdit->setObjectName(QString::fromUtf8("pathLineEdit"));

        gridLayout->addWidget(pathLineEdit, 5, 1, 1, 2);

        browseButton = new QToolButton(InstallDialog);
        browseButton->setObjectName(QString::fromUtf8("browseButton"));

        gridLayout->addWidget(browseButton, 5, 3, 1, 1);

        line = new QFrame(InstallDialog);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(line, 6, 0, 1, 5);

        statusLabel = new QLabel(InstallDialog);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));

        gridLayout->addWidget(statusLabel, 7, 0, 1, 2);

        progressBar = new QProgressBar(InstallDialog);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setValue(0);
        progressBar->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(progressBar, 7, 2, 1, 3);


        retranslateUi(InstallDialog);
        QObject::connect(closeButton, SIGNAL(clicked()), InstallDialog, SLOT(accept()));

        QMetaObject::connectSlotsByName(InstallDialog);
    } // setupUi

    void retranslateUi(QDialog *InstallDialog)
    {
        InstallDialog->setWindowTitle(QCoreApplication::translate("InstallDialog", "Install Documentation", nullptr));
        label->setText(QCoreApplication::translate("InstallDialog", "Available Documentation:", nullptr));
        installButton->setText(QCoreApplication::translate("InstallDialog", "Install", nullptr));
        cancelButton->setText(QCoreApplication::translate("InstallDialog", "Cancel", nullptr));
        closeButton->setText(QCoreApplication::translate("InstallDialog", "Close", nullptr));
        label_4->setText(QCoreApplication::translate("InstallDialog", "Installation Path:", nullptr));
        browseButton->setText(QCoreApplication::translate("InstallDialog", "...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class InstallDialog: public Ui_InstallDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // INSTALLDIALOG_H
