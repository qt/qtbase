/********************************************************************************
** Form generated from reading UI file 'qsqlconnectiondialog.ui'
**
** Created: Fri Sep 4 10:17:14 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QSQLCONNECTIONDIALOG_H
#define QSQLCONNECTIONDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_QSqlConnectionDialogUi
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *connGroupBox;
    QGridLayout *gridLayout;
    QComboBox *comboDriver;
    QLabel *textLabel4;
    QLabel *textLabel2;
    QLineEdit *editDatabase;
    QSpinBox *portSpinBox;
    QLabel *textLabel3;
    QLineEdit *editPassword;
    QLineEdit *editUsername;
    QLineEdit *editHostname;
    QLabel *textLabel5;
    QLabel *textLabel5_2;
    QLabel *textLabel4_2;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem;
    QCheckBox *dbCheckBox;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacerItem1;
    QPushButton *okButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *QSqlConnectionDialogUi)
    {
        if (QSqlConnectionDialogUi->objectName().isEmpty())
            QSqlConnectionDialogUi->setObjectName(QString::fromUtf8("QSqlConnectionDialogUi"));
        QSqlConnectionDialogUi->resize(315, 302);
        vboxLayout = new QVBoxLayout(QSqlConnectionDialogUi);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(8, 8, 8, 8);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        connGroupBox = new QGroupBox(QSqlConnectionDialogUi);
        connGroupBox->setObjectName(QString::fromUtf8("connGroupBox"));
        gridLayout = new QGridLayout(connGroupBox);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        comboDriver = new QComboBox(connGroupBox);
        comboDriver->setObjectName(QString::fromUtf8("comboDriver"));

        gridLayout->addWidget(comboDriver, 0, 1, 1, 1);

        textLabel4 = new QLabel(connGroupBox);
        textLabel4->setObjectName(QString::fromUtf8("textLabel4"));

        gridLayout->addWidget(textLabel4, 2, 0, 1, 1);

        textLabel2 = new QLabel(connGroupBox);
        textLabel2->setObjectName(QString::fromUtf8("textLabel2"));

        gridLayout->addWidget(textLabel2, 0, 0, 1, 1);

        editDatabase = new QLineEdit(connGroupBox);
        editDatabase->setObjectName(QString::fromUtf8("editDatabase"));

        gridLayout->addWidget(editDatabase, 1, 1, 1, 1);

        portSpinBox = new QSpinBox(connGroupBox);
        portSpinBox->setObjectName(QString::fromUtf8("portSpinBox"));
        portSpinBox->setMaximum(65535);
        portSpinBox->setMinimum(-1);
        portSpinBox->setValue(-1);

        gridLayout->addWidget(portSpinBox, 5, 1, 1, 1);

        textLabel3 = new QLabel(connGroupBox);
        textLabel3->setObjectName(QString::fromUtf8("textLabel3"));

        gridLayout->addWidget(textLabel3, 1, 0, 1, 1);

        editPassword = new QLineEdit(connGroupBox);
        editPassword->setObjectName(QString::fromUtf8("editPassword"));
        editPassword->setEchoMode(QLineEdit::Password);

        gridLayout->addWidget(editPassword, 3, 1, 1, 1);

        editUsername = new QLineEdit(connGroupBox);
        editUsername->setObjectName(QString::fromUtf8("editUsername"));

        gridLayout->addWidget(editUsername, 2, 1, 1, 1);

        editHostname = new QLineEdit(connGroupBox);
        editHostname->setObjectName(QString::fromUtf8("editHostname"));

        gridLayout->addWidget(editHostname, 4, 1, 1, 1);

        textLabel5 = new QLabel(connGroupBox);
        textLabel5->setObjectName(QString::fromUtf8("textLabel5"));

        gridLayout->addWidget(textLabel5, 4, 0, 1, 1);

        textLabel5_2 = new QLabel(connGroupBox);
        textLabel5_2->setObjectName(QString::fromUtf8("textLabel5_2"));

        gridLayout->addWidget(textLabel5_2, 5, 0, 1, 1);

        textLabel4_2 = new QLabel(connGroupBox);
        textLabel4_2->setObjectName(QString::fromUtf8("textLabel4_2"));

        gridLayout->addWidget(textLabel4_2, 3, 0, 1, 1);


        vboxLayout->addWidget(connGroupBox);

        hboxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        dbCheckBox = new QCheckBox(QSqlConnectionDialogUi);
        dbCheckBox->setObjectName(QString::fromUtf8("dbCheckBox"));

        hboxLayout->addWidget(dbCheckBox);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        spacerItem1 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem1);

        okButton = new QPushButton(QSqlConnectionDialogUi);
        okButton->setObjectName(QString::fromUtf8("okButton"));
        okButton->setDefault(true);

        hboxLayout1->addWidget(okButton);

        cancelButton = new QPushButton(QSqlConnectionDialogUi);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

        hboxLayout1->addWidget(cancelButton);


        vboxLayout->addLayout(hboxLayout1);

#ifndef QT_NO_SHORTCUT
        textLabel4->setBuddy(editUsername);
        textLabel2->setBuddy(comboDriver);
        textLabel3->setBuddy(editDatabase);
        textLabel5->setBuddy(editHostname);
        textLabel5_2->setBuddy(portSpinBox);
        textLabel4_2->setBuddy(editPassword);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(comboDriver, editDatabase);
        QWidget::setTabOrder(editDatabase, editUsername);
        QWidget::setTabOrder(editUsername, editPassword);
        QWidget::setTabOrder(editPassword, editHostname);
        QWidget::setTabOrder(editHostname, portSpinBox);
        QWidget::setTabOrder(portSpinBox, dbCheckBox);
        QWidget::setTabOrder(dbCheckBox, okButton);
        QWidget::setTabOrder(okButton, cancelButton);

        retranslateUi(QSqlConnectionDialogUi);

        QMetaObject::connectSlotsByName(QSqlConnectionDialogUi);
    } // setupUi

    void retranslateUi(QDialog *QSqlConnectionDialogUi)
    {
        QSqlConnectionDialogUi->setWindowTitle(QApplication::translate("QSqlConnectionDialogUi", "Connect...", 0, QApplication::UnicodeUTF8));
        connGroupBox->setTitle(QApplication::translate("QSqlConnectionDialogUi", "Connection settings", 0, QApplication::UnicodeUTF8));
        textLabel4->setText(QApplication::translate("QSqlConnectionDialogUi", "&Username:", 0, QApplication::UnicodeUTF8));
        textLabel2->setText(QApplication::translate("QSqlConnectionDialogUi", "D&river", 0, QApplication::UnicodeUTF8));
        portSpinBox->setSpecialValueText(QApplication::translate("QSqlConnectionDialogUi", "Default", 0, QApplication::UnicodeUTF8));
        textLabel3->setText(QApplication::translate("QSqlConnectionDialogUi", "Database Name:", 0, QApplication::UnicodeUTF8));
        textLabel5->setText(QApplication::translate("QSqlConnectionDialogUi", "&Hostname:", 0, QApplication::UnicodeUTF8));
        textLabel5_2->setText(QApplication::translate("QSqlConnectionDialogUi", "P&ort:", 0, QApplication::UnicodeUTF8));
        textLabel4_2->setText(QApplication::translate("QSqlConnectionDialogUi", "&Password:", 0, QApplication::UnicodeUTF8));
        dbCheckBox->setText(QApplication::translate("QSqlConnectionDialogUi", "Us&e predefined in-memory database", 0, QApplication::UnicodeUTF8));
        okButton->setText(QApplication::translate("QSqlConnectionDialogUi", "&OK", 0, QApplication::UnicodeUTF8));
        cancelButton->setText(QApplication::translate("QSqlConnectionDialogUi", "&Cancel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QSqlConnectionDialogUi: public Ui_QSqlConnectionDialogUi {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QSQLCONNECTIONDIALOG_H
