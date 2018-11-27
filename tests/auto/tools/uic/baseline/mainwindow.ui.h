/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
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
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(829, 813);
        actionAdd_Custom_Font = new QAction(MainWindow);
        actionAdd_Custom_Font->setObjectName(QString::fromUtf8("actionAdd_Custom_Font"));
        action_Exit = new QAction(MainWindow);
        action_Exit->setObjectName(QString::fromUtf8("action_Exit"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        vboxLayout = new QVBoxLayout(centralwidget);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        hboxLayout = new QHBoxLayout(groupBox);
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));

        hboxLayout->addWidget(label);

        fontComboBox = new QFontComboBox(groupBox);
        fontComboBox->setObjectName(QString::fromUtf8("fontComboBox"));

        hboxLayout->addWidget(fontComboBox);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        hboxLayout->addWidget(label_2);

        pixelSize = new QSpinBox(groupBox);
        pixelSize->setObjectName(QString::fromUtf8("pixelSize"));
        pixelSize->setMinimum(1);

        hboxLayout->addWidget(pixelSize);

        label_7 = new QLabel(groupBox);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        hboxLayout->addWidget(label_7);

        weightCombo = new QComboBox(groupBox);
        weightCombo->setObjectName(QString::fromUtf8("weightCombo"));

        hboxLayout->addWidget(weightCombo);

        italic = new QCheckBox(groupBox);
        italic->setObjectName(QString::fromUtf8("italic"));

        hboxLayout->addWidget(italic);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        vboxLayout1 = new QVBoxLayout(groupBox_2);
#ifndef Q_OS_MAC
        vboxLayout1->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        chooseFromCodePoints = new QRadioButton(groupBox_2);
        chooseFromCodePoints->setObjectName(QString::fromUtf8("chooseFromCodePoints"));
        chooseFromCodePoints->setChecked(true);

        vboxLayout1->addWidget(chooseFromCodePoints);

        vboxLayout2 = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout2->setSpacing(6);
#endif
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        characterRangeView = new QListWidget(groupBox_2);
        characterRangeView->setObjectName(QString::fromUtf8("characterRangeView"));

        vboxLayout2->addWidget(characterRangeView);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        selectAll = new QPushButton(groupBox_2);
        selectAll->setObjectName(QString::fromUtf8("selectAll"));

        hboxLayout1->addWidget(selectAll);

        deselectAll = new QPushButton(groupBox_2);
        deselectAll->setObjectName(QString::fromUtf8("deselectAll"));

        hboxLayout1->addWidget(deselectAll);

        invertSelection = new QPushButton(groupBox_2);
        invertSelection->setObjectName(QString::fromUtf8("invertSelection"));

        hboxLayout1->addWidget(invertSelection);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem1);


        vboxLayout2->addLayout(hboxLayout1);


        vboxLayout1->addLayout(vboxLayout2);

        chooseFromSampleFile = new QRadioButton(groupBox_2);
        chooseFromSampleFile->setObjectName(QString::fromUtf8("chooseFromSampleFile"));

        vboxLayout1->addWidget(chooseFromSampleFile);

        hboxLayout2 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout2->setSpacing(6);
#endif
        hboxLayout2->setContentsMargins(0, 0, 0, 0);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        label_5 = new QLabel(groupBox_2);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setEnabled(false);

        hboxLayout2->addWidget(label_5);

        sampleFile = new QLineEdit(groupBox_2);
        sampleFile->setObjectName(QString::fromUtf8("sampleFile"));
        sampleFile->setEnabled(false);

        hboxLayout2->addWidget(sampleFile);

        browseSampleFile = new QPushButton(groupBox_2);
        browseSampleFile->setObjectName(QString::fromUtf8("browseSampleFile"));
        browseSampleFile->setEnabled(false);

        hboxLayout2->addWidget(browseSampleFile);

        charCount = new QLabel(groupBox_2);
        charCount->setObjectName(QString::fromUtf8("charCount"));
        charCount->setEnabled(false);

        hboxLayout2->addWidget(charCount);


        vboxLayout1->addLayout(hboxLayout2);


        vboxLayout->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(centralwidget);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        hboxLayout3 = new QHBoxLayout(groupBox_3);
#ifndef Q_OS_MAC
        hboxLayout3->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout3->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout3->setObjectName(QString::fromUtf8("hboxLayout3"));
        preview = new QLineEdit(groupBox_3);
        preview->setObjectName(QString::fromUtf8("preview"));

        hboxLayout3->addWidget(preview);


        vboxLayout->addWidget(groupBox_3);

        groupBox_4 = new QGroupBox(centralwidget);
        groupBox_4->setObjectName(QString::fromUtf8("groupBox_4"));
        hboxLayout4 = new QHBoxLayout(groupBox_4);
#ifndef Q_OS_MAC
        hboxLayout4->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout4->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout4->setObjectName(QString::fromUtf8("hboxLayout4"));
        label_3 = new QLabel(groupBox_4);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        hboxLayout4->addWidget(label_3);

        path = new QLineEdit(groupBox_4);
        path->setObjectName(QString::fromUtf8("path"));

        hboxLayout4->addWidget(path);

        browsePath = new QPushButton(groupBox_4);
        browsePath->setObjectName(QString::fromUtf8("browsePath"));

        hboxLayout4->addWidget(browsePath);

        label_4 = new QLabel(groupBox_4);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        hboxLayout4->addWidget(label_4);

        fileName = new QLineEdit(groupBox_4);
        fileName->setObjectName(QString::fromUtf8("fileName"));
        fileName->setEnabled(false);

        hboxLayout4->addWidget(fileName);


        vboxLayout->addWidget(groupBox_4);

        hboxLayout5 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout5->setSpacing(6);
#endif
        hboxLayout5->setContentsMargins(0, 0, 0, 0);
        hboxLayout5->setObjectName(QString::fromUtf8("hboxLayout5"));
        generate = new QPushButton(centralwidget);
        generate->setObjectName(QString::fromUtf8("generate"));

        hboxLayout5->addWidget(generate);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout5->addItem(spacerItem2);


        vboxLayout->addLayout(hboxLayout5);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 829, 29));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
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
