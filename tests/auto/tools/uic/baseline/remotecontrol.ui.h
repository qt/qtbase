/********************************************************************************
** Form generated from reading UI file 'remotecontrol.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_RemoteControlClass
{
public:
    QAction *actionQuit;
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *startUrlLineEdit;
    QPushButton *launchButton;
    QSpacerItem *spacerItem;
    QSpacerItem *spacerItem1;
    QGroupBox *actionGroupBox;
    QGridLayout *gridLayout1;
    QLabel *label_2;
    QHBoxLayout *hboxLayout;
    QLineEdit *indexLineEdit;
    QToolButton *indexButton;
    QLabel *label_4;
    QHBoxLayout *hboxLayout1;
    QLineEdit *identifierLineEdit;
    QToolButton *identifierButton;
    QLabel *label_3;
    QHBoxLayout *hboxLayout2;
    QLineEdit *urlLineEdit;
    QToolButton *urlButton;
    QPushButton *syncContentsButton;
    QSpacerItem *spacerItem2;
    QCheckBox *contentsCheckBox;
    QCheckBox *indexCheckBox;
    QCheckBox *bookmarksCheckBox;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *RemoteControlClass)
    {
        if (RemoteControlClass->objectName().isEmpty())
            RemoteControlClass->setObjectName(QString::fromUtf8("RemoteControlClass"));
        RemoteControlClass->resize(344, 364);
        actionQuit = new QAction(RemoteControlClass);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        centralWidget = new QWidget(RemoteControlClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(centralWidget);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        startUrlLineEdit = new QLineEdit(centralWidget);
        startUrlLineEdit->setObjectName(QString::fromUtf8("startUrlLineEdit"));

        gridLayout->addWidget(startUrlLineEdit, 0, 1, 1, 2);

        launchButton = new QPushButton(centralWidget);
        launchButton->setObjectName(QString::fromUtf8("launchButton"));

        gridLayout->addWidget(launchButton, 1, 1, 1, 1);

        spacerItem = new QSpacerItem(101, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 1, 2, 1, 1);

        spacerItem1 = new QSpacerItem(113, 16, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(spacerItem1, 2, 1, 1, 1);

        actionGroupBox = new QGroupBox(centralWidget);
        actionGroupBox->setObjectName(QString::fromUtf8("actionGroupBox"));
        actionGroupBox->setEnabled(false);
        gridLayout1 = new QGridLayout(actionGroupBox);
        gridLayout1->setSpacing(6);
        gridLayout1->setContentsMargins(11, 11, 11, 11);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        label_2 = new QLabel(actionGroupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout1->addWidget(label_2, 0, 0, 1, 1);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        indexLineEdit = new QLineEdit(actionGroupBox);
        indexLineEdit->setObjectName(QString::fromUtf8("indexLineEdit"));

        hboxLayout->addWidget(indexLineEdit);

        indexButton = new QToolButton(actionGroupBox);
        indexButton->setObjectName(QString::fromUtf8("indexButton"));
        const QIcon icon = QIcon(QString::fromUtf8(":/remotecontrol/enter.png"));
        indexButton->setIcon(icon);

        hboxLayout->addWidget(indexButton);


        gridLayout1->addLayout(hboxLayout, 0, 1, 1, 2);

        label_4 = new QLabel(actionGroupBox);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout1->addWidget(label_4, 1, 0, 1, 1);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        identifierLineEdit = new QLineEdit(actionGroupBox);
        identifierLineEdit->setObjectName(QString::fromUtf8("identifierLineEdit"));

        hboxLayout1->addWidget(identifierLineEdit);

        identifierButton = new QToolButton(actionGroupBox);
        identifierButton->setObjectName(QString::fromUtf8("identifierButton"));
        identifierButton->setIcon(icon);

        hboxLayout1->addWidget(identifierButton);


        gridLayout1->addLayout(hboxLayout1, 1, 1, 1, 2);

        label_3 = new QLabel(actionGroupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout1->addWidget(label_3, 2, 0, 1, 1);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(0);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        urlLineEdit = new QLineEdit(actionGroupBox);
        urlLineEdit->setObjectName(QString::fromUtf8("urlLineEdit"));

        hboxLayout2->addWidget(urlLineEdit);

        urlButton = new QToolButton(actionGroupBox);
        urlButton->setObjectName(QString::fromUtf8("urlButton"));
        urlButton->setIcon(icon);

        hboxLayout2->addWidget(urlButton);


        gridLayout1->addLayout(hboxLayout2, 2, 1, 1, 2);

        syncContentsButton = new QPushButton(actionGroupBox);
        syncContentsButton->setObjectName(QString::fromUtf8("syncContentsButton"));

        gridLayout1->addWidget(syncContentsButton, 3, 1, 1, 1);

        spacerItem2 = new QSpacerItem(81, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout1->addItem(spacerItem2, 3, 2, 1, 1);

        contentsCheckBox = new QCheckBox(actionGroupBox);
        contentsCheckBox->setObjectName(QString::fromUtf8("contentsCheckBox"));

        gridLayout1->addWidget(contentsCheckBox, 4, 0, 1, 3);

        indexCheckBox = new QCheckBox(actionGroupBox);
        indexCheckBox->setObjectName(QString::fromUtf8("indexCheckBox"));

        gridLayout1->addWidget(indexCheckBox, 5, 0, 1, 1);

        bookmarksCheckBox = new QCheckBox(actionGroupBox);
        bookmarksCheckBox->setObjectName(QString::fromUtf8("bookmarksCheckBox"));

        gridLayout1->addWidget(bookmarksCheckBox, 6, 0, 1, 3);


        gridLayout->addWidget(actionGroupBox, 3, 0, 1, 3);

        RemoteControlClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(RemoteControlClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 344, 21));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        RemoteControlClass->setMenuBar(menuBar);
        statusBar = new QStatusBar(RemoteControlClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        RemoteControlClass->setStatusBar(statusBar);

        menuBar->addAction(menuFile->menuAction());
        menuFile->addAction(actionQuit);

        retranslateUi(RemoteControlClass);

        QMetaObject::connectSlotsByName(RemoteControlClass);
    } // setupUi

    void retranslateUi(QMainWindow *RemoteControlClass)
    {
        RemoteControlClass->setWindowTitle(QCoreApplication::translate("RemoteControlClass", "RemoteControl", nullptr));
        actionQuit->setText(QCoreApplication::translate("RemoteControlClass", "Quit", nullptr));
        label->setText(QCoreApplication::translate("RemoteControlClass", "Start URL:", nullptr));
        launchButton->setText(QCoreApplication::translate("RemoteControlClass", "Launch Qt HelpViewer", nullptr));
        actionGroupBox->setTitle(QCoreApplication::translate("RemoteControlClass", "Actions", nullptr));
        label_2->setText(QCoreApplication::translate("RemoteControlClass", "Search in Index:", nullptr));
        indexButton->setText(QString());
        label_4->setText(QCoreApplication::translate("RemoteControlClass", "Identifier:", nullptr));
        identifierButton->setText(QString());
        label_3->setText(QCoreApplication::translate("RemoteControlClass", "Show URL:", nullptr));
        urlButton->setText(QString());
        syncContentsButton->setText(QCoreApplication::translate("RemoteControlClass", "Sync Contents", nullptr));
        contentsCheckBox->setText(QCoreApplication::translate("RemoteControlClass", "Show Contents", nullptr));
        indexCheckBox->setText(QCoreApplication::translate("RemoteControlClass", "Show Index", nullptr));
        bookmarksCheckBox->setText(QCoreApplication::translate("RemoteControlClass", "Show Bookmarks", nullptr));
        menuFile->setTitle(QCoreApplication::translate("RemoteControlClass", "File", nullptr));
    } // retranslateUi

};

namespace Ui {
    class RemoteControlClass: public Ui_RemoteControlClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // REMOTECONTROL_H
