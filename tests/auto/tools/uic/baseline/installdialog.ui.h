/********************************************************************************
** Form generated from reading UI file 'installdialog.ui'
**
** Created: Fri Sep 4 10:17:13 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef INSTALLDIALOG_H
#define INSTALLDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>

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
        InstallDialog->setWindowTitle(QApplication::translate("InstallDialog", "Install Documentation", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("InstallDialog", "Available Documentation:", 0, QApplication::UnicodeUTF8));
        installButton->setText(QApplication::translate("InstallDialog", "Install", 0, QApplication::UnicodeUTF8));
        cancelButton->setText(QApplication::translate("InstallDialog", "Cancel", 0, QApplication::UnicodeUTF8));
        closeButton->setText(QApplication::translate("InstallDialog", "Close", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("InstallDialog", "Installation Path:", 0, QApplication::UnicodeUTF8));
        browseButton->setText(QApplication::translate("InstallDialog", "...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class InstallDialog: public Ui_InstallDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // INSTALLDIALOG_H
