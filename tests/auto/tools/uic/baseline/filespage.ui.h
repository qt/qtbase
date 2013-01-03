/********************************************************************************
** Form generated from reading UI file 'filespage.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef FILESPAGE_H
#define FILESPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FilesPage
{
public:
    QGridLayout *gridLayout;
    QLabel *fileLabel;
    QListWidget *fileListWidget;
    QPushButton *removeButton;
    QPushButton *removeAllButton;
    QSpacerItem *spacerItem;
    QSpacerItem *spacerItem1;

    void setupUi(QWidget *FilesPage)
    {
        if (FilesPage->objectName().isEmpty())
            FilesPage->setObjectName(QStringLiteral("FilesPage"));
        FilesPage->resize(417, 242);
        gridLayout = new QGridLayout(FilesPage);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        fileLabel = new QLabel(FilesPage);
        fileLabel->setObjectName(QStringLiteral("fileLabel"));
        fileLabel->setWordWrap(true);

        gridLayout->addWidget(fileLabel, 0, 0, 1, 2);

        fileListWidget = new QListWidget(FilesPage);
        fileListWidget->setObjectName(QStringLiteral("fileListWidget"));

        gridLayout->addWidget(fileListWidget, 1, 0, 3, 1);

        removeButton = new QPushButton(FilesPage);
        removeButton->setObjectName(QStringLiteral("removeButton"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(removeButton->sizePolicy().hasHeightForWidth());
        removeButton->setSizePolicy(sizePolicy);

        gridLayout->addWidget(removeButton, 1, 1, 1, 1);

        removeAllButton = new QPushButton(FilesPage);
        removeAllButton->setObjectName(QStringLiteral("removeAllButton"));

        gridLayout->addWidget(removeAllButton, 2, 1, 1, 1);

        spacerItem = new QSpacerItem(75, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem, 3, 1, 1, 1);

        spacerItem1 = new QSpacerItem(20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem1, 4, 0, 1, 1);


        retranslateUi(FilesPage);

        QMetaObject::connectSlotsByName(FilesPage);
    } // setupUi

    void retranslateUi(QWidget *FilesPage)
    {
        FilesPage->setWindowTitle(QApplication::translate("FilesPage", "Form", 0));
        fileLabel->setText(QApplication::translate("FilesPage", "Files:", 0));
        removeButton->setText(QApplication::translate("FilesPage", "Remove", 0));
        removeAllButton->setText(QApplication::translate("FilesPage", "Remove All", 0));
    } // retranslateUi

};

namespace Ui {
    class FilesPage: public Ui_FilesPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // FILESPAGE_H
