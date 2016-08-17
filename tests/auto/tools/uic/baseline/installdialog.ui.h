/********************************************************************************
** Form generated from reading UI file 'installdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef INSTALLDIALOG_H
#define INSTALLDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
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
            InstallDialog->setObjectName(QStringLiteral("InstallDialog"));
        InstallDialog->resize(436, 245);
        gridLayout = new QGridLayout(InstallDialog);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label = new QLabel(InstallDialog);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 0, 0, 1, 4);

        listWidget = new QListWidget(InstallDialog);
        listWidget->setObjectName(QStringLiteral("listWidget"));

        gridLayout->addWidget(listWidget, 1, 0, 4, 4);

        installButton = new QPushButton(InstallDialog);
        installButton->setObjectName(QStringLiteral("installButton"));

        gridLayout->addWidget(installButton, 1, 4, 1, 1);

        cancelButton = new QPushButton(InstallDialog);
        cancelButton->setObjectName(QStringLiteral("cancelButton"));

        gridLayout->addWidget(cancelButton, 2, 4, 1, 1);

        closeButton = new QPushButton(InstallDialog);
        closeButton->setObjectName(QStringLiteral("closeButton"));

        gridLayout->addWidget(closeButton, 3, 4, 1, 1);

        spacerItem = new QSpacerItem(20, 56, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem, 4, 4, 1, 1);

        label_4 = new QLabel(InstallDialog);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 5, 0, 1, 1);

        pathLineEdit = new QLineEdit(InstallDialog);
        pathLineEdit->setObjectName(QStringLiteral("pathLineEdit"));

        gridLayout->addWidget(pathLineEdit, 5, 1, 1, 2);

        browseButton = new QToolButton(InstallDialog);
        browseButton->setObjectName(QStringLiteral("browseButton"));

        gridLayout->addWidget(browseButton, 5, 3, 1, 1);

        line = new QFrame(InstallDialog);
        line->setObjectName(QStringLiteral("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(line, 6, 0, 1, 5);

        statusLabel = new QLabel(InstallDialog);
        statusLabel->setObjectName(QStringLiteral("statusLabel"));

        gridLayout->addWidget(statusLabel, 7, 0, 1, 2);

        progressBar = new QProgressBar(InstallDialog);
        progressBar->setObjectName(QStringLiteral("progressBar"));
        progressBar->setValue(0);
        progressBar->setOrientation(Qt::Horizontal);

        gridLayout->addWidget(progressBar, 7, 2, 1, 3);


        retranslateUi(InstallDialog);
        QObject::connect(closeButton, SIGNAL(clicked()), InstallDialog, SLOT(accept()));

        QMetaObject::connectSlotsByName(InstallDialog);
    } // setupUi

    void retranslateUi(QDialog *InstallDialog)
    {
        InstallDialog->setWindowTitle(QApplication::translate("InstallDialog", "Install Documentation", Q_NULLPTR));
        label->setText(QApplication::translate("InstallDialog", "Available Documentation:", Q_NULLPTR));
        installButton->setText(QApplication::translate("InstallDialog", "Install", Q_NULLPTR));
        cancelButton->setText(QApplication::translate("InstallDialog", "Cancel", Q_NULLPTR));
        closeButton->setText(QApplication::translate("InstallDialog", "Close", Q_NULLPTR));
        label_4->setText(QApplication::translate("InstallDialog", "Installation Path:", Q_NULLPTR));
        browseButton->setText(QApplication::translate("InstallDialog", "...", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class InstallDialog: public Ui_InstallDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // INSTALLDIALOG_H
