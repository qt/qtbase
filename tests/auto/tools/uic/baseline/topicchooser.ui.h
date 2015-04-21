/********************************************************************************
** Form generated from reading UI file 'topicchooser.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TOPICCHOOSER_H
#define TOPICCHOOSER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TopicChooser
{
public:
    QVBoxLayout *vboxLayout;
    QLabel *label;
    QListWidget *listWidget;
    QWidget *Layout16;
    QHBoxLayout *hboxLayout;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonDisplay;
    QPushButton *buttonCancel;

    void setupUi(QDialog *TopicChooser)
    {
        if (TopicChooser->objectName().isEmpty())
            TopicChooser->setObjectName(QStringLiteral("TopicChooser"));
        TopicChooser->resize(391, 223);
        TopicChooser->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(TopicChooser);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        vboxLayout->setObjectName(QStringLiteral("unnamed"));
        label = new QLabel(TopicChooser);
        label->setObjectName(QStringLiteral("label"));

        vboxLayout->addWidget(label);

        listWidget = new QListWidget(TopicChooser);
        listWidget->setObjectName(QStringLiteral("listWidget"));

        vboxLayout->addWidget(listWidget);

        Layout16 = new QWidget(TopicChooser);
        Layout16->setObjectName(QStringLiteral("Layout16"));
        hboxLayout = new QHBoxLayout(Layout16);
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        hboxLayout->setObjectName(QStringLiteral("unnamed"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(Horizontal_Spacing2);

        buttonDisplay = new QPushButton(Layout16);
        buttonDisplay->setObjectName(QStringLiteral("buttonDisplay"));
        buttonDisplay->setAutoDefault(true);

        hboxLayout->addWidget(buttonDisplay);

        buttonCancel = new QPushButton(Layout16);
        buttonCancel->setObjectName(QStringLiteral("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout->addWidget(buttonCancel);


        vboxLayout->addWidget(Layout16);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(listWidget);
#endif // QT_NO_SHORTCUT

        retranslateUi(TopicChooser);

        buttonDisplay->setDefault(true);


        QMetaObject::connectSlotsByName(TopicChooser);
    } // setupUi

    void retranslateUi(QDialog *TopicChooser)
    {
        TopicChooser->setWindowTitle(QApplication::translate("TopicChooser", "Choose Topic", 0));
        label->setText(QApplication::translate("TopicChooser", "&Topics", 0));
        buttonDisplay->setText(QApplication::translate("TopicChooser", "&Display", 0));
        buttonCancel->setText(QApplication::translate("TopicChooser", "&Close", 0));
    } // retranslateUi

};

namespace Ui {
    class TopicChooser: public Ui_TopicChooser {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TOPICCHOOSER_H
