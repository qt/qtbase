/********************************************************************************
** Form generated from reading UI file 'default.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef DEFAULT_H
#define DEFAULT_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
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
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(388, 413);
        exitAction = new QAction(MainWindow);
        exitAction->setObjectName(QStringLiteral("exitAction"));
        aboutQtAction = new QAction(MainWindow);
        aboutQtAction->setObjectName(QStringLiteral("aboutQtAction"));
        editStyleAction = new QAction(MainWindow);
        editStyleAction->setObjectName(QStringLiteral("editStyleAction"));
        aboutAction = new QAction(MainWindow);
        aboutAction->setObjectName(QStringLiteral("aboutAction"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        nameLabel = new QLabel(centralwidget);
        nameLabel->setObjectName(QStringLiteral("nameLabel"));

        gridLayout->addWidget(nameLabel, 0, 0, 1, 1);

        nameCombo = new QComboBox(centralwidget);
        nameCombo->setObjectName(QStringLiteral("nameCombo"));
        nameCombo->setEditable(true);

        gridLayout->addWidget(nameCombo, 0, 1, 1, 3);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 1, 3, 1, 1);

        femaleRadioButton = new QRadioButton(centralwidget);
        femaleRadioButton->setObjectName(QStringLiteral("femaleRadioButton"));

        gridLayout->addWidget(femaleRadioButton, 1, 2, 1, 1);

        agreeCheckBox = new QCheckBox(centralwidget);
        agreeCheckBox->setObjectName(QStringLiteral("agreeCheckBox"));

        gridLayout->addWidget(agreeCheckBox, 6, 0, 1, 4);

        maleRadioButton = new QRadioButton(centralwidget);
        maleRadioButton->setObjectName(QStringLiteral("maleRadioButton"));

        gridLayout->addWidget(maleRadioButton, 1, 1, 1, 1);

        genderLabel = new QLabel(centralwidget);
        genderLabel->setObjectName(QStringLiteral("genderLabel"));

        gridLayout->addWidget(genderLabel, 1, 0, 1, 1);

        ageSpinBox = new QSpinBox(centralwidget);
        ageSpinBox->setObjectName(QStringLiteral("ageSpinBox"));
        ageSpinBox->setMinimum(12);
        ageSpinBox->setValue(22);

        gridLayout->addWidget(ageSpinBox, 2, 1, 1, 3);

        buttonBox = new QDialogButtonBox(centralwidget);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 7, 2, 1, 2);

        ageLabel = new QLabel(centralwidget);
        ageLabel->setObjectName(QStringLiteral("ageLabel"));

        gridLayout->addWidget(ageLabel, 2, 0, 1, 1);

        passwordLabel = new QLabel(centralwidget);
        passwordLabel->setObjectName(QStringLiteral("passwordLabel"));

        gridLayout->addWidget(passwordLabel, 3, 0, 1, 1);

        passwordEdit = new QLineEdit(centralwidget);
        passwordEdit->setObjectName(QStringLiteral("passwordEdit"));
        passwordEdit->setEchoMode(QLineEdit::Password);

        gridLayout->addWidget(passwordEdit, 3, 1, 1, 3);

        label = new QLabel(centralwidget);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 5, 0, 1, 1);

        countryLabel = new QLabel(centralwidget);
        countryLabel->setObjectName(QStringLiteral("countryLabel"));

        gridLayout->addWidget(countryLabel, 4, 0, 1, 1);

        professionList = new QListWidget(centralwidget);
        new QListWidgetItem(professionList);
        new QListWidgetItem(professionList);
        new QListWidgetItem(professionList);
        professionList->setObjectName(QStringLiteral("professionList"));

        gridLayout->addWidget(professionList, 5, 1, 1, 3);

        countryCombo = new QComboBox(centralwidget);
        countryCombo->setObjectName(QStringLiteral("countryCombo"));

        gridLayout->addWidget(countryCombo, 4, 1, 1, 3);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 388, 21));
        menu_File = new QMenu(menubar);
        menu_File->setObjectName(QStringLiteral("menu_File"));
        menu_Help = new QMenu(menubar);
        menu_Help->setObjectName(QStringLiteral("menu_Help"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);
#ifndef QT_NO_SHORTCUT
        nameLabel->setBuddy(nameCombo);
        ageLabel->setBuddy(ageSpinBox);
        passwordLabel->setBuddy(passwordEdit);
        label->setBuddy(professionList);
        countryLabel->setBuddy(professionList);
#endif // QT_NO_SHORTCUT
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
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0));
        exitAction->setText(QApplication::translate("MainWindow", "&Exit", 0));
        aboutQtAction->setText(QApplication::translate("MainWindow", "About Qt", 0));
        editStyleAction->setText(QApplication::translate("MainWindow", "Edit &Style", 0));
        aboutAction->setText(QApplication::translate("MainWindow", "About", 0));
        nameLabel->setText(QApplication::translate("MainWindow", "&Name:", 0));
        nameCombo->clear();
        nameCombo->insertItems(0, QStringList()
         << QApplication::translate("MainWindow", "Girish", 0)
         << QApplication::translate("MainWindow", "Jasmin", 0)
         << QApplication::translate("MainWindow", "Simon", 0)
         << QApplication::translate("MainWindow", "Zack", 0)
        );
#ifndef QT_NO_TOOLTIP
        nameCombo->setToolTip(QApplication::translate("MainWindow", "Specify your name", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        femaleRadioButton->setToolTip(QApplication::translate("MainWindow", "Check this if you are female", 0));
#endif // QT_NO_TOOLTIP
        femaleRadioButton->setText(QApplication::translate("MainWindow", "&Female", 0));
#ifndef QT_NO_TOOLTIP
        agreeCheckBox->setToolTip(QApplication::translate("MainWindow", "Please read the license before checking this", 0));
#endif // QT_NO_TOOLTIP
        agreeCheckBox->setText(QApplication::translate("MainWindow", "I &accept the terms and conditions", 0));
#ifndef QT_NO_TOOLTIP
        maleRadioButton->setToolTip(QApplication::translate("MainWindow", "Check this if you are male", 0));
#endif // QT_NO_TOOLTIP
        maleRadioButton->setText(QApplication::translate("MainWindow", "&Male", 0));
        genderLabel->setText(QApplication::translate("MainWindow", "Gender:", 0));
#ifndef QT_NO_TOOLTIP
        ageSpinBox->setToolTip(QApplication::translate("MainWindow", "Specify your age", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        ageSpinBox->setStatusTip(QApplication::translate("MainWindow", "Specify your age here", 0));
#endif // QT_NO_STATUSTIP
        ageLabel->setText(QApplication::translate("MainWindow", "&Age:", 0));
        passwordLabel->setText(QApplication::translate("MainWindow", "&Password:", 0));
#ifndef QT_NO_TOOLTIP
        passwordEdit->setToolTip(QApplication::translate("MainWindow", "Specify your password", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        passwordEdit->setStatusTip(QApplication::translate("MainWindow", "Specify your password here", 0));
#endif // QT_NO_STATUSTIP
        passwordEdit->setText(QApplication::translate("MainWindow", "Password", 0));
        label->setText(QApplication::translate("MainWindow", "Profession", 0));
        countryLabel->setText(QApplication::translate("MainWindow", "&Country", 0));

        const bool __sortingEnabled = professionList->isSortingEnabled();
        professionList->setSortingEnabled(false);
        QListWidgetItem *___qlistwidgetitem = professionList->item(0);
        ___qlistwidgetitem->setText(QApplication::translate("MainWindow", "Developer", 0));
        QListWidgetItem *___qlistwidgetitem1 = professionList->item(1);
        ___qlistwidgetitem1->setText(QApplication::translate("MainWindow", "Student", 0));
        QListWidgetItem *___qlistwidgetitem2 = professionList->item(2);
        ___qlistwidgetitem2->setText(QApplication::translate("MainWindow", "Fisherman", 0));
        professionList->setSortingEnabled(__sortingEnabled);

#ifndef QT_NO_TOOLTIP
        professionList->setToolTip(QApplication::translate("MainWindow", "Select your profession", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        professionList->setStatusTip(QApplication::translate("MainWindow", "Select your profession", 0));
#endif // QT_NO_STATUSTIP
#ifndef QT_NO_WHATSTHIS
        professionList->setWhatsThis(QApplication::translate("MainWindow", "Select your profession", 0));
#endif // QT_NO_WHATSTHIS
        countryCombo->clear();
        countryCombo->insertItems(0, QStringList()
         << QApplication::translate("MainWindow", "Germany", 0)
         << QApplication::translate("MainWindow", "India", 0)
         << QApplication::translate("MainWindow", "Norway", 0)
         << QApplication::translate("MainWindow", "United States Of America", 0)
         << QApplication::translate("MainWindow", "United Kingdom", 0)
        );
#ifndef QT_NO_TOOLTIP
        countryCombo->setToolTip(QApplication::translate("MainWindow", "Specify your country", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        countryCombo->setStatusTip(QApplication::translate("MainWindow", "Specify your country here", 0));
#endif // QT_NO_STATUSTIP
        menu_File->setTitle(QApplication::translate("MainWindow", "&File", 0));
        menu_Help->setTitle(QApplication::translate("MainWindow", "&Help", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // DEFAULT_H
