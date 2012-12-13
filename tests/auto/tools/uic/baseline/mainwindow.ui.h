/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
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
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(829, 813);
        actionAdd_Custom_Font = new QAction(MainWindow);
        actionAdd_Custom_Font->setObjectName(QStringLiteral("actionAdd_Custom_Font"));
        action_Exit = new QAction(MainWindow);
        action_Exit->setObjectName(QStringLiteral("action_Exit"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        vboxLayout = new QVBoxLayout(centralwidget);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QStringLiteral("vboxLayout"));
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        hboxLayout = new QHBoxLayout(groupBox);
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout->setObjectName(QStringLiteral("hboxLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));

        hboxLayout->addWidget(label);

        fontComboBox = new QFontComboBox(groupBox);
        fontComboBox->setObjectName(QStringLiteral("fontComboBox"));

        hboxLayout->addWidget(fontComboBox);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QStringLiteral("label_2"));

        hboxLayout->addWidget(label_2);

        pixelSize = new QSpinBox(groupBox);
        pixelSize->setObjectName(QStringLiteral("pixelSize"));
        pixelSize->setMinimum(1);

        hboxLayout->addWidget(pixelSize);

        label_7 = new QLabel(groupBox);
        label_7->setObjectName(QStringLiteral("label_7"));

        hboxLayout->addWidget(label_7);

        weightCombo = new QComboBox(groupBox);
        weightCombo->setObjectName(QStringLiteral("weightCombo"));

        hboxLayout->addWidget(weightCombo);

        italic = new QCheckBox(groupBox);
        italic->setObjectName(QStringLiteral("italic"));

        hboxLayout->addWidget(italic);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        vboxLayout1 = new QVBoxLayout(groupBox_2);
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout1->setObjectName(QStringLiteral("vboxLayout1"));
        chooseFromCodePoints = new QRadioButton(groupBox_2);
        chooseFromCodePoints->setObjectName(QStringLiteral("chooseFromCodePoints"));
        chooseFromCodePoints->setChecked(true);

        vboxLayout1->addWidget(chooseFromCodePoints);

        vboxLayout2 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        vboxLayout2->setObjectName(QStringLiteral("vboxLayout2"));
        characterRangeView = new QListWidget(groupBox_2);
        characterRangeView->setObjectName(QStringLiteral("characterRangeView"));

        vboxLayout2->addWidget(characterRangeView);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QStringLiteral("hboxLayout1"));
        selectAll = new QPushButton(groupBox_2);
        selectAll->setObjectName(QStringLiteral("selectAll"));

        hboxLayout1->addWidget(selectAll);

        deselectAll = new QPushButton(groupBox_2);
        deselectAll->setObjectName(QStringLiteral("deselectAll"));

        hboxLayout1->addWidget(deselectAll);

        invertSelection = new QPushButton(groupBox_2);
        invertSelection->setObjectName(QStringLiteral("invertSelection"));

        hboxLayout1->addWidget(invertSelection);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem1);


        vboxLayout2->addLayout(hboxLayout1);


        vboxLayout1->addLayout(vboxLayout2);

        chooseFromSampleFile = new QRadioButton(groupBox_2);
        chooseFromSampleFile->setObjectName(QStringLiteral("chooseFromSampleFile"));

        vboxLayout1->addWidget(chooseFromSampleFile);

        hboxLayout2 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout2->setSpacing(6);
#endif
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        hboxLayout2->setObjectName(QStringLiteral("hboxLayout2"));
        label_5 = new QLabel(groupBox_2);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setEnabled(false);

        hboxLayout2->addWidget(label_5);

        sampleFile = new QLineEdit(groupBox_2);
        sampleFile->setObjectName(QStringLiteral("sampleFile"));
        sampleFile->setEnabled(false);

        hboxLayout2->addWidget(sampleFile);

        browseSampleFile = new QPushButton(groupBox_2);
        browseSampleFile->setObjectName(QStringLiteral("browseSampleFile"));
        browseSampleFile->setEnabled(false);

        hboxLayout2->addWidget(browseSampleFile);

        charCount = new QLabel(groupBox_2);
        charCount->setObjectName(QStringLiteral("charCount"));
        charCount->setEnabled(false);

        hboxLayout2->addWidget(charCount);


        vboxLayout1->addLayout(hboxLayout2);


        vboxLayout->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(centralwidget);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        hboxLayout3 = new QHBoxLayout(groupBox_3);
#ifndef Q_OS_MAC
        hboxLayout3->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout3->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout3->setObjectName(QStringLiteral("hboxLayout3"));
        preview = new QLineEdit(groupBox_3);
        preview->setObjectName(QStringLiteral("preview"));

        hboxLayout3->addWidget(preview);


        vboxLayout->addWidget(groupBox_3);

        groupBox_4 = new QGroupBox(centralwidget);
        groupBox_4->setObjectName(QStringLiteral("groupBox_4"));
        hboxLayout4 = new QHBoxLayout(groupBox_4);
#ifndef Q_OS_MAC
        hboxLayout4->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout4->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout4->setObjectName(QStringLiteral("hboxLayout4"));
        label_3 = new QLabel(groupBox_4);
        label_3->setObjectName(QStringLiteral("label_3"));

        hboxLayout4->addWidget(label_3);

        path = new QLineEdit(groupBox_4);
        path->setObjectName(QStringLiteral("path"));

        hboxLayout4->addWidget(path);

        browsePath = new QPushButton(groupBox_4);
        browsePath->setObjectName(QStringLiteral("browsePath"));

        hboxLayout4->addWidget(browsePath);

        label_4 = new QLabel(groupBox_4);
        label_4->setObjectName(QStringLiteral("label_4"));

        hboxLayout4->addWidget(label_4);

        fileName = new QLineEdit(groupBox_4);
        fileName->setObjectName(QStringLiteral("fileName"));
        fileName->setEnabled(false);

        hboxLayout4->addWidget(fileName);


        vboxLayout->addWidget(groupBox_4);

        hboxLayout5 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout5->setSpacing(6);
#endif
        hboxLayout5->setContentsMargins(0, 0, 0, 0);
        hboxLayout5->setObjectName(QStringLiteral("hboxLayout5"));
        generate = new QPushButton(centralwidget);
        generate->setObjectName(QStringLiteral("generate"));

        hboxLayout5->addWidget(generate);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout5->addItem(spacerItem2);


        vboxLayout->addLayout(hboxLayout5);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 829, 29));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menuFile->addAction(actionAdd_Custom_Font);
        menuFile->addSeparator();
        menuFile->addAction(action_Exit);

        retranslateUi(MainWindow);
        QObject::connect(action_Exit, SIGNAL(triggered()), MainWindow, SLOT(close()));
        QObject::connect(chooseFromCodePoints, SIGNAL(toggled(bool)), characterRangeView, SLOT(setEnabled(bool)));
        QObject::connect(chooseFromCodePoints, SIGNAL(toggled(bool)), selectAll, SLOT(setEnabled(bool)));
        QObject::connect(chooseFromCodePoints, SIGNAL(toggled(bool)), deselectAll, SLOT(setEnabled(bool)));
        QObject::connect(chooseFromCodePoints, SIGNAL(toggled(bool)), invertSelection, SLOT(setEnabled(bool)));
        QObject::connect(chooseFromSampleFile, SIGNAL(toggled(bool)), sampleFile, SLOT(setEnabled(bool)));
        QObject::connect(chooseFromSampleFile, SIGNAL(toggled(bool)), browseSampleFile, SLOT(setEnabled(bool)));
        QObject::connect(chooseFromSampleFile, SIGNAL(toggled(bool)), charCount, SLOT(setEnabled(bool)));
        QObject::connect(chooseFromSampleFile, SIGNAL(toggled(bool)), label_5, SLOT(setEnabled(bool)));

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MakeQPF", 0));
        actionAdd_Custom_Font->setText(QApplication::translate("MainWindow", "&Add Custom Font...", 0));
        action_Exit->setText(QApplication::translate("MainWindow", "&Exit", 0));
        groupBox->setTitle(QApplication::translate("MainWindow", "Font Properties", 0));
        label->setText(QApplication::translate("MainWindow", "Family:", 0));
        label_2->setText(QApplication::translate("MainWindow", "Pixel Size:", 0));
        label_7->setText(QApplication::translate("MainWindow", "Weight:", 0));
        italic->setText(QApplication::translate("MainWindow", "Italic", 0));
        groupBox_2->setTitle(QApplication::translate("MainWindow", "Glyph Coverage", 0));
        chooseFromCodePoints->setText(QApplication::translate("MainWindow", "Choose from Unicode Codepoints:", 0));
        selectAll->setText(QApplication::translate("MainWindow", "Select &All", 0));
        deselectAll->setText(QApplication::translate("MainWindow", "&Deselect All", 0));
        invertSelection->setText(QApplication::translate("MainWindow", "&Invert Selection", 0));
        chooseFromSampleFile->setText(QApplication::translate("MainWindow", "Choose from Sample Text File (UTF-8 Encoded):", 0));
        label_5->setText(QApplication::translate("MainWindow", "Path:", 0));
        browseSampleFile->setText(QApplication::translate("MainWindow", "Browse...", 0));
        charCount->setText(QApplication::translate("MainWindow", "TextLabel", 0));
        groupBox_3->setTitle(QApplication::translate("MainWindow", "Preview", 0));
        groupBox_4->setTitle(QApplication::translate("MainWindow", "Output Options", 0));
        label_3->setText(QApplication::translate("MainWindow", "Path:", 0));
        browsePath->setText(QApplication::translate("MainWindow", "Browse...", 0));
        label_4->setText(QApplication::translate("MainWindow", "Filename:", 0));
        generate->setText(QApplication::translate("MainWindow", "Generate Pre-Rendered Font...", 0));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MAINWINDOW_H
