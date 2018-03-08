/********************************************************************************
** Form generated from reading UI file 'qprintsettingsoutput.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QPRINTSETTINGSOUTPUT_H
#define QPRINTSETTINGSOUTPUT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QPrintSettingsOutput
{
public:
    QHBoxLayout *horizontalLayout_2;
    QTabWidget *tabs;
    QWidget *copiesTab;
    QHBoxLayout *horizontalLayout;
    QGroupBox *gbPrintRange;
    QVBoxLayout *_3;
    QRadioButton *printAll;
    QHBoxLayout *_4;
    QRadioButton *printRange;
    QSpinBox *from;
    QLabel *label_3;
    QSpinBox *to;
    QSpacerItem *spacerItem;
    QRadioButton *printSelection;
    QSpacerItem *verticalSpacer;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QLabel *label;
    QSpinBox *copies;
    QSpacerItem *horizontalSpacer;
    QCheckBox *collate;
    QLabel *outputIcon;
    QCheckBox *reverse;
    QSpacerItem *verticalSpacer_2;
    QWidget *optionsTab;
    QGridLayout *gridLayout_2;
    QGroupBox *colorMode;
    QGridLayout *gridLayout_4;
    QSpacerItem *verticalSpacer_6;
    QRadioButton *color;
    QLabel *colorIcon;
    QRadioButton *grayscale;
    QGroupBox *duplex;
    QVBoxLayout *verticalLayout;
    QRadioButton *noDuplex;
    QRadioButton *duplexLong;
    QRadioButton *duplexShort;
    QSpacerItem *verticalSpacer1;

    void setupUi(QWidget *QPrintSettingsOutput)
    {
        if (QPrintSettingsOutput->objectName().isEmpty())
            QPrintSettingsOutput->setObjectName(QStringLiteral("QPrintSettingsOutput"));
        QPrintSettingsOutput->resize(416, 166);
        horizontalLayout_2 = new QHBoxLayout(QPrintSettingsOutput);
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        tabs = new QTabWidget(QPrintSettingsOutput);
        tabs->setObjectName(QStringLiteral("tabs"));
        copiesTab = new QWidget();
        copiesTab->setObjectName(QStringLiteral("copiesTab"));
        copiesTab->setGeometry(QRect(0, 0, 412, 139));
        horizontalLayout = new QHBoxLayout(copiesTab);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        gbPrintRange = new QGroupBox(copiesTab);
        gbPrintRange->setObjectName(QStringLiteral("gbPrintRange"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(gbPrintRange->sizePolicy().hasHeightForWidth());
        gbPrintRange->setSizePolicy(sizePolicy);
        _3 = new QVBoxLayout(gbPrintRange);
        _3->setSpacing(4);
        _3->setContentsMargins(6, 6, 6, 6);
        _3->setObjectName(QStringLiteral("_3"));
        printAll = new QRadioButton(gbPrintRange);
        printAll->setObjectName(QStringLiteral("printAll"));
        printAll->setChecked(true);

        _3->addWidget(printAll);

        _4 = new QHBoxLayout();
#ifndef Q_OS_MAC
        _4->setSpacing(6);
#endif
        _4->setContentsMargins(0, 0, 0, 0);
        _4->setObjectName(QStringLiteral("_4"));
        printRange = new QRadioButton(gbPrintRange);
        printRange->setObjectName(QStringLiteral("printRange"));

        _4->addWidget(printRange);

        from = new QSpinBox(gbPrintRange);
        from->setObjectName(QStringLiteral("from"));
        from->setEnabled(false);
        from->setMinimum(1);
        from->setMaximum(999);

        _4->addWidget(from);

        label_3 = new QLabel(gbPrintRange);
        label_3->setObjectName(QStringLiteral("label_3"));

        _4->addWidget(label_3);

        to = new QSpinBox(gbPrintRange);
        to->setObjectName(QStringLiteral("to"));
        to->setEnabled(false);
        to->setMinimum(1);
        to->setMaximum(999);

        _4->addWidget(to);

        spacerItem = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _4->addItem(spacerItem);


        _3->addLayout(_4);

        printSelection = new QRadioButton(gbPrintRange);
        printSelection->setObjectName(QStringLiteral("printSelection"));

        _3->addWidget(printSelection);

        verticalSpacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        _3->addItem(verticalSpacer);


        horizontalLayout->addWidget(gbPrintRange);

        groupBox = new QGroupBox(copiesTab);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        copies = new QSpinBox(groupBox);
        copies->setObjectName(QStringLiteral("copies"));
        copies->setMinimum(1);
        copies->setMaximum(999);

        gridLayout->addWidget(copies, 0, 1, 1, 2);

        horizontalSpacer = new QSpacerItem(91, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 3, 1, 1);

        collate = new QCheckBox(groupBox);
        collate->setObjectName(QStringLiteral("collate"));

        gridLayout->addWidget(collate, 1, 0, 1, 2);

        outputIcon = new QLabel(groupBox);
        outputIcon->setObjectName(QStringLiteral("outputIcon"));
        QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Ignored);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(outputIcon->sizePolicy().hasHeightForWidth());
        outputIcon->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(outputIcon, 1, 2, 2, 2);

        reverse = new QCheckBox(groupBox);
        reverse->setObjectName(QStringLiteral("reverse"));

        gridLayout->addWidget(reverse, 2, 0, 1, 2);

        verticalSpacer_2 = new QSpacerItem(0, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer_2, 3, 0, 1, 4);


        horizontalLayout->addWidget(groupBox);

        tabs->addTab(copiesTab, QString());
        optionsTab = new QWidget();
        optionsTab->setObjectName(QStringLiteral("optionsTab"));
        optionsTab->setGeometry(QRect(0, 0, 412, 139));
        gridLayout_2 = new QGridLayout(optionsTab);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        colorMode = new QGroupBox(optionsTab);
        colorMode->setObjectName(QStringLiteral("colorMode"));
        gridLayout_4 = new QGridLayout(colorMode);
        gridLayout_4->setObjectName(QStringLiteral("gridLayout_4"));
        verticalSpacer_6 = new QSpacerItem(1, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_4->addItem(verticalSpacer_6, 2, 0, 1, 1);

        color = new QRadioButton(colorMode);
        color->setObjectName(QStringLiteral("color"));

        gridLayout_4->addWidget(color, 0, 0, 1, 1);

        colorIcon = new QLabel(colorMode);
        colorIcon->setObjectName(QStringLiteral("colorIcon"));

        gridLayout_4->addWidget(colorIcon, 0, 1, 3, 1);

        grayscale = new QRadioButton(colorMode);
        grayscale->setObjectName(QStringLiteral("grayscale"));

        gridLayout_4->addWidget(grayscale, 1, 0, 1, 1);


        gridLayout_2->addWidget(colorMode, 0, 1, 1, 1);

        duplex = new QGroupBox(optionsTab);
        duplex->setObjectName(QStringLiteral("duplex"));
        verticalLayout = new QVBoxLayout(duplex);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        noDuplex = new QRadioButton(duplex);
        noDuplex->setObjectName(QStringLiteral("noDuplex"));
        noDuplex->setChecked(true);

        verticalLayout->addWidget(noDuplex);

        duplexLong = new QRadioButton(duplex);
        duplexLong->setObjectName(QStringLiteral("duplexLong"));

        verticalLayout->addWidget(duplexLong);

        duplexShort = new QRadioButton(duplex);
        duplexShort->setObjectName(QStringLiteral("duplexShort"));

        verticalLayout->addWidget(duplexShort);

        verticalSpacer1 = new QSpacerItem(1, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer1);


        gridLayout_2->addWidget(duplex, 0, 0, 1, 1);

        tabs->addTab(optionsTab, QString());

        horizontalLayout_2->addWidget(tabs);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(copies);
#endif // QT_NO_SHORTCUT

        retranslateUi(QPrintSettingsOutput);
        QObject::connect(printRange, SIGNAL(toggled(bool)), from, SLOT(setEnabled(bool)));
        QObject::connect(printRange, SIGNAL(toggled(bool)), to, SLOT(setEnabled(bool)));

        tabs->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(QPrintSettingsOutput);
    } // setupUi

    void retranslateUi(QWidget *QPrintSettingsOutput)
    {
        QPrintSettingsOutput->setWindowTitle(QApplication::translate("QPrintSettingsOutput", "Form", nullptr));
        gbPrintRange->setTitle(QApplication::translate("QPrintSettingsOutput", "Print range", nullptr));
        printAll->setText(QApplication::translate("QPrintSettingsOutput", "Print all", nullptr));
        printRange->setText(QApplication::translate("QPrintSettingsOutput", "Pages from", nullptr));
        label_3->setText(QApplication::translate("QPrintSettingsOutput", "to", nullptr));
        printSelection->setText(QApplication::translate("QPrintSettingsOutput", "Selection", nullptr));
        groupBox->setTitle(QApplication::translate("QPrintSettingsOutput", "Output Settings", nullptr));
        label->setText(QApplication::translate("QPrintSettingsOutput", "Copies:", nullptr));
        collate->setText(QApplication::translate("QPrintSettingsOutput", "Collate", nullptr));
        reverse->setText(QApplication::translate("QPrintSettingsOutput", "Reverse", nullptr));
        tabs->setTabText(tabs->indexOf(copiesTab), QApplication::translate("QPrintSettingsOutput", "Copies", nullptr));
        colorMode->setTitle(QApplication::translate("QPrintSettingsOutput", "Color Mode", nullptr));
        color->setText(QApplication::translate("QPrintSettingsOutput", "Color", nullptr));
        grayscale->setText(QApplication::translate("QPrintSettingsOutput", "Grayscale", nullptr));
        duplex->setTitle(QApplication::translate("QPrintSettingsOutput", "Duplex Printing", nullptr));
        noDuplex->setText(QApplication::translate("QPrintSettingsOutput", "None", nullptr));
        duplexLong->setText(QApplication::translate("QPrintSettingsOutput", "Long side", nullptr));
        duplexShort->setText(QApplication::translate("QPrintSettingsOutput", "Short side", nullptr));
        tabs->setTabText(tabs->indexOf(optionsTab), QApplication::translate("QPrintSettingsOutput", "Options", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QPrintSettingsOutput: public Ui_QPrintSettingsOutput {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QPRINTSETTINGSOUTPUT_H
