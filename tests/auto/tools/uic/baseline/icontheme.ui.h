/********************************************************************************
** Form generated from reading UI file 'icontheme.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ICONTHEME_H
#define ICONTHEME_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
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
            Form->setObjectName(QStringLiteral("Form"));
        Form->resize(122, 117);
        verticalLayout = new QVBoxLayout(Form);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        fileicon = new QPushButton(Form);
        fileicon->setObjectName(QStringLiteral("fileicon"));
        QIcon icon;
        icon.addFile(QStringLiteral("image1.png"), QSize(), QIcon::Normal, QIcon::Off);
        fileicon->setIcon(icon);

        verticalLayout->addWidget(fileicon);

        fileandthemeicon = new QPushButton(Form);
        fileandthemeicon->setObjectName(QStringLiteral("fileandthemeicon"));
        QIcon icon1;
        QString iconThemeName = QStringLiteral("edit-copy");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon1 = QIcon::fromTheme(iconThemeName);
        } else {
            icon1.addFile(QStringLiteral("image7.png"), QSize(), QIcon::Normal, QIcon::Off);
        }
        fileandthemeicon->setIcon(icon1);

        verticalLayout->addWidget(fileandthemeicon);

        themeicon = new QPushButton(Form);
        themeicon->setObjectName(QStringLiteral("themeicon"));
        QIcon icon2;
        iconThemeName = QStringLiteral("edit-copy");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon2 = QIcon::fromTheme(iconThemeName);
        } else {
            icon2.addFile(QStringLiteral(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        themeicon->setIcon(icon2);

        verticalLayout->addWidget(themeicon);


        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QApplication::translate("Form", "Form", Q_NULLPTR));
        fileicon->setText(QApplication::translate("Form", "fileicon", Q_NULLPTR));
        fileandthemeicon->setText(QApplication::translate("Form", "PushButton", Q_NULLPTR));
        themeicon->setText(QApplication::translate("Form", "PushButton", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ICONTHEME_H
