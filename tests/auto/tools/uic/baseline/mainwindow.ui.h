/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionAdd_Custom_Font;
    QAction *action_Exit;
    QWidget *centralwidget;
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QHBoxLayout *hboxLayout;
    QLabel *label;
    QFontComboBox *fontComboBox;
    QLabel *label_2;
    QSpinBox *pixelSize;
    QLabel *label_7;
    QComboBox *weightCombo;
    QCheckBox *italic;
    QSpacerItem *spacerItem;
    QGroupBox *groupBox_2;
    QVBoxLayout *vboxLayout1;
    QRadioButton *chooseFromCodePoints;
    QVBoxLayout *vboxLayout2;
    QListWidget *characterRangeView;
    QHBoxLayout *hboxLayout1;
    QPushButton *selectAll;
    QPushButton *deselectAll;
    QPushButton *invertSelection;
    QSpacerItem *spacerItem1;
    QRadioButton *chooseFromSampleFile;
    QHBoxLayout *hboxLayout2;
    QLabel *label_5;
    QLineEdit *sampleFile;
    QPushButton *browseSampleFile;
    QLabel *charCount;
    QGroupBox *groupBox_3;
    QHBoxLayout *hboxLayout3;
    QLineEdit *preview;
    QGroupBox *groupBox_4;
    QHBoxLayout *hboxLayout4;
    QLabel *label_3;
    QLineEdit *path;
    QPushButton *browsePath;
    QLabel *label_4;
    QLineEdit *fileName;
    QHBoxLayout *hboxLayout5;
    QPushButton *generate;
    QSpacerItem *spacerItem2;
    QMenuBar *menubar;
    QMenu *menuFile;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(829, 813);
        actionAdd_Custom_Font = new QAction(MainWindow);
        actionAdd_Custom_Font->setObjectName("actionAdd_Custom_Font");
        action_Exit = new QAction(MainWindow);
        action_Exit->setObjectName("action_Exit");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        vboxLayout = new QVBoxLayout(centralwidget);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName("vboxLayout");
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        hboxLayout = new QHBoxLayout(groupBox);
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout->setObjectName("hboxLayout");
        label = new QLabel(groupBox);
        label->setObjectName("label");

        hboxLayout->addWidget(label);

        fontComboBox = new QFontComboBox(groupBox);
        fontComboBox->setObjectName("fontComboBox");

        hboxLayout->addWidget(fontComboBox);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");

        hboxLayout->addWidget(label_2);

        pixelSize = new QSpinBox(groupBox);
        pixelSize->setObjectName("pixelSize");
        pixelSize->setMinimum(1);

        hboxLayout->addWidget(pixelSize);

        label_7 = new QLabel(groupBox);
        label_7->setObjectName("label_7");

        hboxLayout->addWidget(label_7);

        weightCombo = new QComboBox(groupBox);
        weightCombo->setObjectName("weightCombo");

        hboxLayout->addWidget(weightCombo);

        italic = new QCheckBox(groupBox);
        italic->setObjectName("italic");

        hboxLayout->addWidget(italic);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName("groupBox_2");
        vboxLayout1 = new QVBoxLayout(groupBox_2);
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout1->setObjectName("vboxLayout1");
        chooseFromCodePoints = new QRadioButton(groupBox_2);
        chooseFromCodePoints->setObjectName("chooseFromCodePoints");
        chooseFromCodePoints->setChecked(true);

        vboxLayout1->addWidget(chooseFromCodePoints);

        vboxLayout2 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        vboxLayout2->setObjectName("vboxLayout2");
        characterRangeView = new QListWidget(groupBox_2);
        characterRangeView->setObjectName("characterRangeView");

        vboxLayout2->addWidget(characterRangeView);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName("hboxLayout1");
        selectAll = new QPushButton(groupBox_2);
        selectAll->setObjectName("selectAll");

        hboxLayout1->addWidget(selectAll);

        deselectAll = new QPushButton(groupBox_2);
        deselectAll->setObjectName("deselectAll");

        hboxLayout1->addWidget(deselectAll);

        invertSelection = new QPushButton(groupBox_2);
        invertSelection->setObjectName("invertSelection");

        hboxLayout1->addWidget(invertSelection);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout1->addItem(spacerItem1);


        vboxLayout2->addLayout(hboxLayout1);


        vboxLayout1->addLayout(vboxLayout2);

        chooseFromSampleFile = new QRadioButton(groupBox_2);
        chooseFromSampleFile->setObjectName("chooseFromSampleFile");

        vboxLayout1->addWidget(chooseFromSampleFile);

        hboxLayout2 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout2->setSpacing(6);
#endif
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        hboxLayout2->setObjectName("hboxLayout2");
        label_5 = new QLabel(groupBox_2);
        label_5->setObjectName("label_5");
        label_5->setEnabled(false);

        hboxLayout2->addWidget(label_5);

        sampleFile = new QLineEdit(groupBox_2);
        sampleFile->setObjectName("sampleFile");
        sampleFile->setEnabled(false);

        hboxLayout2->addWidget(sampleFile);

        browseSampleFile = new QPushButton(groupBox_2);
        browseSampleFile->setObjectName("browseSampleFile");
        browseSampleFile->setEnabled(false);

        hboxLayout2->addWidget(browseSampleFile);

        charCount = new QLabel(groupBox_2);
        charCount->setObjectName("charCount");
        charCount->setEnabled(false);

        hboxLayout2->addWidget(charCount);


        vboxLayout1->addLayout(hboxLayout2);


        vboxLayout->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(centralwidget);
        groupBox_3->setObjectName("groupBox_3");
        hboxLayout3 = new QHBoxLayout(groupBox_3);
#ifndef Q_OS_MAC
        hboxLayout3->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout3->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout3->setObjectName("hboxLayout3");
        preview = new QLineEdit(groupBox_3);
        preview->setObjectName("preview");

        hboxLayout3->addWidget(preview);


        vboxLayout->addWidget(groupBox_3);

        groupBox_4 = new QGroupBox(centralwidget);
        groupBox_4->setObjectName("groupBox_4");
        hboxLayout4 = new QHBoxLayout(groupBox_4);
#ifndef Q_OS_MAC
        hboxLayout4->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout4->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout4->setObjectName("hboxLayout4");
        label_3 = new QLabel(groupBox_4);
        label_3->setObjectName("label_3");

        hboxLayout4->addWidget(label_3);

        path = new QLineEdit(groupBox_4);
        path->setObjectName("path");

        hboxLayout4->addWidget(path);

        browsePath = new QPushButton(groupBox_4);
        browsePath->setObjectName("browsePath");

        hboxLayout4->addWidget(browsePath);

        label_4 = new QLabel(groupBox_4);
        label_4->setObjectName("label_4");

        hboxLayout4->addWidget(label_4);

        fileName = new QLineEdit(groupBox_4);
        fileName->setObjectName("fileName");
        fileName->setEnabled(false);

        hboxLayout4->addWidget(fileName);


        vboxLayout->addWidget(groupBox_4);

        hboxLayout5 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout5->setSpacing(6);
#endif
        hboxLayout5->setContentsMargins(0, 0, 0, 0);
        hboxLayout5->setObjectName("hboxLayout5");
        generate = new QPushButton(centralwidget);
        generate->setObjectName("generate");

        hboxLayout5->addWidget(generate);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout5->addItem(spacerItem2);


        vboxLayout->addLayout(hboxLayout5);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 829, 29));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName("menuFile");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menuFile->addAction(actionAdd_Custom_Font);
        menuFile->addSeparator();
        menuFile->addAction(action_Exit);

        retranslateUi(MainWindow);
        QObject::connect(action_Exit, &QAction::triggered, MainWindow, qOverload<>(&QMainWindow::close));
        QObject::connect(chooseFromCodePoints, &QRadioButton::toggled, characterRangeView, &QListWidget::setEnabled);
        QObject::connect(chooseFromCodePoints, &QRadioButton::toggled, selectAll, &QPushButton::setEnabled);
        QObject::connect(chooseFromCodePoints, &QRadioButton::toggled, deselectAll, &QPushButton::setEnabled);
        QObject::connect(chooseFromCodePoints, &QRadioButton::toggled, invertSelection, &QPushButton::setEnabled);
        QObject::connect(chooseFromSampleFile, &QRadioButton::toggled, sampleFile, &QLineEdit::setEnabled);
        QObject::connect(chooseFromSampleFile, &QRadioButton::toggled, browseSampleFile, &QPushButton::setEnabled);
        QObject::connect(chooseFromSampleFile, &QRadioButton::toggled, charCount, &QLabel::setEnabled);
        QObject::connect(chooseFromSampleFile, &QRadioButton::toggled, label_5, &QLabel::setEnabled);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MakeQPF", nullptr));
        actionAdd_Custom_Font->setText(QCoreApplication::translate("MainWindow", "&Add Custom Font...", nullptr));
        action_Exit->setText(QCoreApplication::translate("MainWindow", "&Exit", nullptr));
        groupBox->setTitle(QCoreApplication::translate("MainWindow", "Font Properties", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Family:", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Pixel Size:", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "Weight:", nullptr));
        italic->setText(QCoreApplication::translate("MainWindow", "Italic", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("MainWindow", "Glyph Coverage", nullptr));
        chooseFromCodePoints->setText(QCoreApplication::translate("MainWindow", "Choose from Unicode Codepoints:", nullptr));
        selectAll->setText(QCoreApplication::translate("MainWindow", "Select &All", nullptr));
        deselectAll->setText(QCoreApplication::translate("MainWindow", "&Deselect All", nullptr));
        invertSelection->setText(QCoreApplication::translate("MainWindow", "&Invert Selection", nullptr));
        chooseFromSampleFile->setText(QCoreApplication::translate("MainWindow", "Choose from Sample Text File (UTF-8 Encoded):", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "Path:", nullptr));
        browseSampleFile->setText(QCoreApplication::translate("MainWindow", "Browse...", nullptr));
        charCount->setText(QCoreApplication::translate("MainWindow", "TextLabel", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("MainWindow", "Preview", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("MainWindow", "Output Options", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Path:", nullptr));
        browsePath->setText(QCoreApplication::translate("MainWindow", "Browse...", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Filename:", nullptr));
        generate->setText(QCoreApplication::translate("MainWindow", "Generate Pre-Rendered Font...", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MAINWINDOW_H
