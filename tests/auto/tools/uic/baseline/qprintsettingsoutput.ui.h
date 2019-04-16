/********************************************************************************
** Form generated from reading UI file 'qprintsettingsoutput.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
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
            QPrintSettingsOutput->setObjectName(QString::fromUtf8("QPrintSettingsOutput"));
        QPrintSettingsOutput->resize(416, 166);
        horizontalLayout_2 = new QHBoxLayout(QPrintSettingsOutput);
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        tabs = new QTabWidget(QPrintSettingsOutput);
        tabs->setObjectName(QString::fromUtf8("tabs"));
        copiesTab = new QWidget();
        copiesTab->setObjectName(QString::fromUtf8("copiesTab"));
        copiesTab->setGeometry(QRect(0, 0, 412, 139));
        horizontalLayout = new QHBoxLayout(copiesTab);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        gbPrintRange = new QGroupBox(copiesTab);
        gbPrintRange->setObjectName(QString::fromUtf8("gbPrintRange"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(gbPrintRange->sizePolicy().hasHeightForWidth());
        gbPrintRange->setSizePolicy(sizePolicy);
        _3 = new QVBoxLayout(gbPrintRange);
        _3->setSpacing(4);
        _3->setContentsMargins(6, 6, 6, 6);
        _3->setObjectName(QString::fromUtf8("_3"));
        printAll = new QRadioButton(gbPrintRange);
        printAll->setObjectName(QString::fromUtf8("printAll"));
        printAll->setChecked(true);

        _3->addWidget(printAll);

        _4 = new QHBoxLayout();
#ifndef Q_OS_MAC
        _4->setSpacing(6);
#endif
        _4->setContentsMargins(0, 0, 0, 0);
        _4->setObjectName(QString::fromUtf8("_4"));
        printRange = new QRadioButton(gbPrintRange);
        printRange->setObjectName(QString::fromUtf8("printRange"));

        _4->addWidget(printRange);

        from = new QSpinBox(gbPrintRange);
        from->setObjectName(QString::fromUtf8("from"));
        from->setEnabled(false);
        from->setMinimum(1);
        from->setMaximum(999);

        _4->addWidget(from);

        label_3 = new QLabel(gbPrintRange);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        _4->addWidget(label_3);

        to = new QSpinBox(gbPrintRange);
        to->setObjectName(QString::fromUtf8("to"));
        to->setEnabled(false);
        to->setMinimum(1);
        to->setMaximum(999);

        _4->addWidget(to);

        spacerItem = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _4->addItem(spacerItem);


        _3->addLayout(_4);

        printSelection = new QRadioButton(gbPrintRange);
        printSelection->setObjectName(QString::fromUtf8("printSelection"));

        _3->addWidget(printSelection);

        verticalSpacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        _3->addItem(verticalSpacer);


        horizontalLayout->addWidget(gbPrintRange);

        groupBox = new QGroupBox(copiesTab);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        copies = new QSpinBox(groupBox);
        copies->setObjectName(QString::fromUtf8("copies"));
        copies->setMinimum(1);
        copies->setMaximum(999);

        gridLayout->addWidget(copies, 0, 1, 1, 2);

        horizontalSpacer = new QSpacerItem(91, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 3, 1, 1);

        collate = new QCheckBox(groupBox);
        collate->setObjectName(QString::fromUtf8("collate"));

        gridLayout->addWidget(collate, 1, 0, 1, 2);

        outputIcon = new QLabel(groupBox);
        outputIcon->setObjectName(QString::fromUtf8("outputIcon"));
        QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Ignored);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(outputIcon->sizePolicy().hasHeightForWidth());
        outputIcon->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(outputIcon, 1, 2, 2, 2);

        reverse = new QCheckBox(groupBox);
        reverse->setObjectName(QString::fromUtf8("reverse"));

        gridLayout->addWidget(reverse, 2, 0, 1, 2);

        verticalSpacer_2 = new QSpacerItem(0, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer_2, 3, 0, 1, 4);


        horizontalLayout->addWidget(groupBox);

        tabs->addTab(copiesTab, QString());
        optionsTab = new QWidget();
        optionsTab->setObjectName(QString::fromUtf8("optionsTab"));
        optionsTab->setGeometry(QRect(0, 0, 412, 139));
        gridLayout_2 = new QGridLayout(optionsTab);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        colorMode = new QGroupBox(optionsTab);
        colorMode->setObjectName(QString::fromUtf8("colorMode"));
        gridLayout_4 = new QGridLayout(colorMode);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        verticalSpacer_6 = new QSpacerItem(1, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_4->addItem(verticalSpacer_6, 2, 0, 1, 1);

        color = new QRadioButton(colorMode);
        color->setObjectName(QString::fromUtf8("color"));

        gridLayout_4->addWidget(color, 0, 0, 1, 1);

        colorIcon = new QLabel(colorMode);
        colorIcon->setObjectName(QString::fromUtf8("colorIcon"));

        gridLayout_4->addWidget(colorIcon, 0, 1, 3, 1);

        grayscale = new QRadioButton(colorMode);
        grayscale->setObjectName(QString::fromUtf8("grayscale"));

        gridLayout_4->addWidget(grayscale, 1, 0, 1, 1);


        gridLayout_2->addWidget(colorMode, 0, 1, 1, 1);

        duplex = new QGroupBox(optionsTab);
        duplex->setObjectName(QString::fromUtf8("duplex"));
        verticalLayout = new QVBoxLayout(duplex);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        noDuplex = new QRadioButton(duplex);
        noDuplex->setObjectName(QString::fromUtf8("noDuplex"));
        noDuplex->setChecked(true);

        verticalLayout->addWidget(noDuplex);

        duplexLong = new QRadioButton(duplex);
        duplexLong->setObjectName(QString::fromUtf8("duplexLong"));

        verticalLayout->addWidget(duplexLong);

        duplexShort = new QRadioButton(duplex);
        duplexShort->setObjectName(QString::fromUtf8("duplexShort"));

        verticalLayout->addWidget(duplexShort);

        verticalSpacer1 = new QSpacerItem(1, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer1);


        gridLayout_2->addWidget(duplex, 0, 0, 1, 1);

        tabs->addTab(optionsTab, QString());

        horizontalLayout_2->addWidget(tabs);

#if QT_CONFIG(shortcut)
        label->setBuddy(copies);
#endif // QT_CONFIG(shortcut)

        retranslateUi(QPrintSettingsOutput);
        QObject::connect(printRange, SIGNAL(toggled(bool)), from, SLOT(setEnabled(bool)));
        QObject::connect(printRange, SIGNAL(toggled(bool)), to, SLOT(setEnabled(bool)));

        tabs->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(QPrintSettingsOutput);
    } // setupUi

    void retranslateUi(QWidget *QPrintSettingsOutput)
    {
        QPrintSettingsOutput->setWindowTitle(QCoreApplication::translate("QPrintSettingsOutput", "Form", nullptr));
        gbPrintRange->setTitle(QCoreApplication::translate("QPrintSettingsOutput", "Print range", nullptr));
        printAll->setText(QCoreApplication::translate("QPrintSettingsOutput", "Print all", nullptr));
        printRange->setText(QCoreApplication::translate("QPrintSettingsOutput", "Pages from", nullptr));
        label_3->setText(QCoreApplication::translate("QPrintSettingsOutput", "to", nullptr));
        printSelection->setText(QCoreApplication::translate("QPrintSettingsOutput", "Selection", nullptr));
        groupBox->setTitle(QCoreApplication::translate("QPrintSettingsOutput", "Output Settings", nullptr));
        label->setText(QCoreApplication::translate("QPrintSettingsOutput", "Copies:", nullptr));
        collate->setText(QCoreApplication::translate("QPrintSettingsOutput", "Collate", nullptr));
        reverse->setText(QCoreApplication::translate("QPrintSettingsOutput", "Reverse", nullptr));
        tabs->setTabText(tabs->indexOf(copiesTab), QCoreApplication::translate("QPrintSettingsOutput", "Copies", nullptr));
        colorMode->setTitle(QCoreApplication::translate("QPrintSettingsOutput", "Color Mode", nullptr));
        color->setText(QCoreApplication::translate("QPrintSettingsOutput", "Color", nullptr));
        grayscale->setText(QCoreApplication::translate("QPrintSettingsOutput", "Grayscale", nullptr));
        duplex->setTitle(QCoreApplication::translate("QPrintSettingsOutput", "Duplex Printing", nullptr));
        noDuplex->setText(QCoreApplication::translate("QPrintSettingsOutput", "None", nullptr));
        duplexLong->setText(QCoreApplication::translate("QPrintSettingsOutput", "Long side", nullptr));
        duplexShort->setText(QCoreApplication::translate("QPrintSettingsOutput", "Short side", nullptr));
        tabs->setTabText(tabs->indexOf(optionsTab), QCoreApplication::translate("QPrintSettingsOutput", "Options", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QPrintSettingsOutput: public Ui_QPrintSettingsOutput {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QPRINTSETTINGSOUTPUT_H
