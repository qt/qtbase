/********************************************************************************
** Form generated from reading UI file 'icontheme.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ICONTHEME_H
#define ICONTHEME_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QVBoxLayout *verticalLayout;
    QPushButton *fileicon;
    QPushButton *fileandthemeicon;
    QPushButton *themeicon;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName(QString::fromUtf8("Form"));
        Form->resize(122, 117);
        verticalLayout = new QVBoxLayout(Form);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        fileicon = new QPushButton(Form);
        fileicon->setObjectName(QString::fromUtf8("fileicon"));
        QIcon icon;
        icon.addFile(QString::fromUtf8("image1.png"), QSize(), QIcon::Normal, QIcon::Off);
        fileicon->setIcon(icon);

        verticalLayout->addWidget(fileicon);

        fileandthemeicon = new QPushButton(Form);
        fileandthemeicon->setObjectName(QString::fromUtf8("fileandthemeicon"));
        QIcon icon1;
        QString iconThemeName = QString::fromUtf8("edit-copy");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon1 = QIcon::fromTheme(iconThemeName);
        } else {
            icon1.addFile(QString::fromUtf8("image7.png"), QSize(), QIcon::Normal, QIcon::Off);
        }
        fileandthemeicon->setIcon(icon1);

        verticalLayout->addWidget(fileandthemeicon);

        themeicon = new QPushButton(Form);
        themeicon->setObjectName(QString::fromUtf8("themeicon"));
        QIcon icon2;
        iconThemeName = QString::fromUtf8("edit-copy");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon2 = QIcon::fromTheme(iconThemeName);
        } else {
            icon2.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        themeicon->setIcon(icon2);

        verticalLayout->addWidget(themeicon);


        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QCoreApplication::translate("Form", "Form", nullptr));
        fileicon->setText(QCoreApplication::translate("Form", "fileicon", nullptr));
        fileandthemeicon->setText(QCoreApplication::translate("Form", "PushButton", nullptr));
        themeicon->setText(QCoreApplication::translate("Form", "PushButton", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ICONTHEME_H
