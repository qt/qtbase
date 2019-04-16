/********************************************************************************
** Form generated from reading UI file 'default.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef DEFAULT_H
#define DEFAULT_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *exitAction;
    QAction *aboutQtAction;
    QAction *editStyleAction;
    QAction *aboutAction;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QLabel *nameLabel;
    QComboBox *nameCombo;
    QSpacerItem *spacerItem;
    QRadioButton *femaleRadioButton;
    QCheckBox *agreeCheckBox;
    QRadioButton *maleRadioButton;
    QLabel *genderLabel;
    QSpinBox *ageSpinBox;
    QDialogButtonBox *buttonBox;
    QLabel *ageLabel;
    QLabel *passwordLabel;
    QLineEdit *passwordEdit;
    QLabel *label;
    QLabel *countryLabel;
    QListWidget *professionList;
    QComboBox *countryCombo;
    QMenuBar *menubar;
    QMenu *menu_File;
    QMenu *menu_Help;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(388, 413);
        exitAction = new QAction(MainWindow);
        exitAction->setObjectName(QString::fromUtf8("exitAction"));
        aboutQtAction = new QAction(MainWindow);
        aboutQtAction->setObjectName(QString::fromUtf8("aboutQtAction"));
        editStyleAction = new QAction(MainWindow);
        editStyleAction->setObjectName(QString::fromUtf8("editStyleAction"));
        aboutAction = new QAction(MainWindow);
        aboutAction->setObjectName(QString::fromUtf8("aboutAction"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        nameLabel = new QLabel(centralwidget);
        nameLabel->setObjectName(QString::fromUtf8("nameLabel"));

        gridLayout->addWidget(nameLabel, 0, 0, 1, 1);

        nameCombo = new QComboBox(centralwidget);
        nameCombo->addItem(QString());
        nameCombo->addItem(QString());
        nameCombo->addItem(QString());
        nameCombo->addItem(QString());
        nameCombo->setObjectName(QString::fromUtf8("nameCombo"));
        nameCombo->setEditable(true);

        gridLayout->addWidget(nameCombo, 0, 1, 1, 3);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 1, 3, 1, 1);

        femaleRadioButton = new QRadioButton(centralwidget);
        femaleRadioButton->setObjectName(QString::fromUtf8("femaleRadioButton"));

        gridLayout->addWidget(femaleRadioButton, 1, 2, 1, 1);

        agreeCheckBox = new QCheckBox(centralwidget);
        agreeCheckBox->setObjectName(QString::fromUtf8("agreeCheckBox"));

        gridLayout->addWidget(agreeCheckBox, 6, 0, 1, 4);

        maleRadioButton = new QRadioButton(centralwidget);
        maleRadioButton->setObjectName(QString::fromUtf8("maleRadioButton"));

        gridLayout->addWidget(maleRadioButton, 1, 1, 1, 1);

        genderLabel = new QLabel(centralwidget);
        genderLabel->setObjectName(QString::fromUtf8("genderLabel"));

        gridLayout->addWidget(genderLabel, 1, 0, 1, 1);

        ageSpinBox = new QSpinBox(centralwidget);
        ageSpinBox->setObjectName(QString::fromUtf8("ageSpinBox"));
        ageSpinBox->setMinimum(12);
        ageSpinBox->setValue(22);

        gridLayout->addWidget(ageSpinBox, 2, 1, 1, 3);

        buttonBox = new QDialogButtonBox(centralwidget);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 7, 2, 1, 2);

        ageLabel = new QLabel(centralwidget);
        ageLabel->setObjectName(QString::fromUtf8("ageLabel"));

        gridLayout->addWidget(ageLabel, 2, 0, 1, 1);

        passwordLabel = new QLabel(centralwidget);
        passwordLabel->setObjectName(QString::fromUtf8("passwordLabel"));

        gridLayout->addWidget(passwordLabel, 3, 0, 1, 1);

        passwordEdit = new QLineEdit(centralwidget);
        passwordEdit->setObjectName(QString::fromUtf8("passwordEdit"));
        passwordEdit->setEchoMode(QLineEdit::Password);

        gridLayout->addWidget(passwordEdit, 3, 1, 1, 3);

        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 5, 0, 1, 1);

        countryLabel = new QLabel(centralwidget);
        countryLabel->setObjectName(QString::fromUtf8("countryLabel"));

        gridLayout->addWidget(countryLabel, 4, 0, 1, 1);

        professionList = new QListWidget(centralwidget);
        new QListWidgetItem(professionList);
        new QListWidgetItem(professionList);
        new QListWidgetItem(professionList);
        professionList->setObjectName(QString::fromUtf8("professionList"));

        gridLayout->addWidget(professionList, 5, 1, 1, 3);

        countryCombo = new QComboBox(centralwidget);
        countryCombo->addItem(QString());
        countryCombo->addItem(QString());
        countryCombo->addItem(QString());
        countryCombo->addItem(QString());
        countryCombo->addItem(QString());
        countryCombo->setObjectName(QString::fromUtf8("countryCombo"));

        gridLayout->addWidget(countryCombo, 4, 1, 1, 3);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 388, 21));
        menu_File = new QMenu(menubar);
        menu_File->setObjectName(QString::fromUtf8("menu_File"));
        menu_Help = new QMenu(menubar);
        menu_Help->setObjectName(QString::fromUtf8("menu_Help"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);
#if QT_CONFIG(shortcut)
        nameLabel->setBuddy(nameCombo);
        ageLabel->setBuddy(ageSpinBox);
        passwordLabel->setBuddy(passwordEdit);
        label->setBuddy(professionList);
        countryLabel->setBuddy(professionList);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(maleRadioButton, femaleRadioButton);
        QWidget::setTabOrder(femaleRadioButton, ageSpinBox);
        QWidget::setTabOrder(ageSpinBox, passwordEdit);
        QWidget::setTabOrder(passwordEdit, professionList);
        QWidget::setTabOrder(professionList, agreeCheckBox);

        menubar->addAction(menu_File->menuAction());
        menubar->addAction(menu_Help->menuAction());
        menu_File->addAction(editStyleAction);
        menu_File->addSeparator();
        menu_File->addAction(exitAction);
        menu_Help->addAction(aboutAction);
        menu_Help->addSeparator();
        menu_Help->addAction(aboutQtAction);

        retranslateUi(MainWindow);

        nameCombo->setCurrentIndex(-1);
        professionList->setCurrentRow(1);
        countryCombo->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        exitAction->setText(QCoreApplication::translate("MainWindow", "&Exit", nullptr));
        aboutQtAction->setText(QCoreApplication::translate("MainWindow", "About Qt", nullptr));
        editStyleAction->setText(QCoreApplication::translate("MainWindow", "Edit &Style", nullptr));
        aboutAction->setText(QCoreApplication::translate("MainWindow", "About", nullptr));
        nameLabel->setText(QCoreApplication::translate("MainWindow", "&Name:", nullptr));
        nameCombo->setItemText(0, QCoreApplication::translate("MainWindow", "Girish", nullptr));
        nameCombo->setItemText(1, QCoreApplication::translate("MainWindow", "Jasmin", nullptr));
        nameCombo->setItemText(2, QCoreApplication::translate("MainWindow", "Simon", nullptr));
        nameCombo->setItemText(3, QCoreApplication::translate("MainWindow", "Zack", nullptr));

#if QT_CONFIG(tooltip)
        nameCombo->setToolTip(QCoreApplication::translate("MainWindow", "Specify your name", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        femaleRadioButton->setToolTip(QCoreApplication::translate("MainWindow", "Check this if you are female", nullptr));
#endif // QT_CONFIG(tooltip)
        femaleRadioButton->setText(QCoreApplication::translate("MainWindow", "&Female", nullptr));
#if QT_CONFIG(tooltip)
        agreeCheckBox->setToolTip(QCoreApplication::translate("MainWindow", "Please read the license before checking this", nullptr));
#endif // QT_CONFIG(tooltip)
        agreeCheckBox->setText(QCoreApplication::translate("MainWindow", "I &accept the terms and conditions", nullptr));
#if QT_CONFIG(tooltip)
        maleRadioButton->setToolTip(QCoreApplication::translate("MainWindow", "Check this if you are male", nullptr));
#endif // QT_CONFIG(tooltip)
        maleRadioButton->setText(QCoreApplication::translate("MainWindow", "&Male", nullptr));
        genderLabel->setText(QCoreApplication::translate("MainWindow", "Gender:", nullptr));
#if QT_CONFIG(tooltip)
        ageSpinBox->setToolTip(QCoreApplication::translate("MainWindow", "Specify your age", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        ageSpinBox->setStatusTip(QCoreApplication::translate("MainWindow", "Specify your age here", nullptr));
#endif // QT_CONFIG(statustip)
        ageLabel->setText(QCoreApplication::translate("MainWindow", "&Age:", nullptr));
        passwordLabel->setText(QCoreApplication::translate("MainWindow", "&Password:", nullptr));
#if QT_CONFIG(tooltip)
        passwordEdit->setToolTip(QCoreApplication::translate("MainWindow", "Specify your password", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        passwordEdit->setStatusTip(QCoreApplication::translate("MainWindow", "Specify your password here", nullptr));
#endif // QT_CONFIG(statustip)
        passwordEdit->setText(QCoreApplication::translate("MainWindow", "Password", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Profession", nullptr));
        countryLabel->setText(QCoreApplication::translate("MainWindow", "&Country", nullptr));

        const bool __sortingEnabled = professionList->isSortingEnabled();
        professionList->setSortingEnabled(false);
        QListWidgetItem *___qlistwidgetitem = professionList->item(0);
        ___qlistwidgetitem->setText(QCoreApplication::translate("MainWindow", "Developer", nullptr));
        QListWidgetItem *___qlistwidgetitem1 = professionList->item(1);
        ___qlistwidgetitem1->setText(QCoreApplication::translate("MainWindow", "Student", nullptr));
        QListWidgetItem *___qlistwidgetitem2 = professionList->item(2);
        ___qlistwidgetitem2->setText(QCoreApplication::translate("MainWindow", "Fisherman", nullptr));
        professionList->setSortingEnabled(__sortingEnabled);

#if QT_CONFIG(tooltip)
        professionList->setToolTip(QCoreApplication::translate("MainWindow", "Select your profession", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        professionList->setStatusTip(QCoreApplication::translate("MainWindow", "Select your profession", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(whatsthis)
        professionList->setWhatsThis(QCoreApplication::translate("MainWindow", "Select your profession", nullptr));
#endif // QT_CONFIG(whatsthis)
        countryCombo->setItemText(0, QCoreApplication::translate("MainWindow", "Germany", nullptr));
        countryCombo->setItemText(1, QCoreApplication::translate("MainWindow", "India", nullptr));
        countryCombo->setItemText(2, QCoreApplication::translate("MainWindow", "Norway", nullptr));
        countryCombo->setItemText(3, QCoreApplication::translate("MainWindow", "United States Of America", nullptr));
        countryCombo->setItemText(4, QCoreApplication::translate("MainWindow", "United Kingdom", nullptr));

#if QT_CONFIG(tooltip)
        countryCombo->setToolTip(QCoreApplication::translate("MainWindow", "Specify your country", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        countryCombo->setStatusTip(QCoreApplication::translate("MainWindow", "Specify your country here", nullptr));
#endif // QT_CONFIG(statustip)
        menu_File->setTitle(QCoreApplication::translate("MainWindow", "&File", nullptr));
        menu_Help->setTitle(QCoreApplication::translate("MainWindow", "&Help", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // DEFAULT_H
