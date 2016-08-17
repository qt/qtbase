/********************************************************************************
** Form generated from reading UI file 'qpagesetupwidget.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QPAGESETUPWIDGET_H
#define QPAGESETUPWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QPageSetupWidget
{
public:
    QGridLayout *gridLayout_3;
    QHBoxLayout *horizontalLayout_4;
    QComboBox *unit;
    QSpacerItem *horizontalSpacer_3;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *pageSizeLabel;
    QComboBox *paperSize;
    QLabel *widthLabel;
    QHBoxLayout *horizontalLayout_3;
    QDoubleSpinBox *paperWidth;
    QLabel *heightLabel;
    QDoubleSpinBox *paperHeight;
    QLabel *paperSourceLabel;
    QComboBox *paperSource;
    QSpacerItem *horizontalSpacer_4;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout;
    QRadioButton *portrait;
    QRadioButton *landscape;
    QRadioButton *reverseLandscape;
    QRadioButton *reversePortrait;
    QSpacerItem *verticalSpacer_5;
    QWidget *preview;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout_2;
    QGridLayout *gridLayout;
    QDoubleSpinBox *topMargin;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_7;
    QDoubleSpinBox *leftMargin;
    QSpacerItem *horizontalSpacer;
    QDoubleSpinBox *rightMargin;
    QSpacerItem *horizontalSpacer_8;
    QDoubleSpinBox *bottomMargin;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *horizontalSpacer_5;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *QPageSetupWidget)
    {
        if (QPageSetupWidget->objectName().isEmpty())
            QPageSetupWidget->setObjectName(QStringLiteral("QPageSetupWidget"));
        QPageSetupWidget->resize(416, 488);
        gridLayout_3 = new QGridLayout(QPageSetupWidget);
        gridLayout_3->setContentsMargins(0, 0, 0, 0);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        unit = new QComboBox(QPageSetupWidget);
        unit->setObjectName(QStringLiteral("unit"));

        horizontalLayout_4->addWidget(unit);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_3);


        gridLayout_3->addLayout(horizontalLayout_4, 0, 0, 1, 2);

        groupBox_2 = new QGroupBox(QPageSetupWidget);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        pageSizeLabel = new QLabel(groupBox_2);
        pageSizeLabel->setObjectName(QStringLiteral("pageSizeLabel"));

        gridLayout_2->addWidget(pageSizeLabel, 0, 0, 1, 1);

        paperSize = new QComboBox(groupBox_2);
        paperSize->setObjectName(QStringLiteral("paperSize"));

        gridLayout_2->addWidget(paperSize, 0, 1, 1, 1);

        widthLabel = new QLabel(groupBox_2);
        widthLabel->setObjectName(QStringLiteral("widthLabel"));

        gridLayout_2->addWidget(widthLabel, 1, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        paperWidth = new QDoubleSpinBox(groupBox_2);
        paperWidth->setObjectName(QStringLiteral("paperWidth"));
        paperWidth->setMaximum(9999.99);

        horizontalLayout_3->addWidget(paperWidth);

        heightLabel = new QLabel(groupBox_2);
        heightLabel->setObjectName(QStringLiteral("heightLabel"));

        horizontalLayout_3->addWidget(heightLabel);

        paperHeight = new QDoubleSpinBox(groupBox_2);
        paperHeight->setObjectName(QStringLiteral("paperHeight"));
        paperHeight->setMaximum(9999.99);

        horizontalLayout_3->addWidget(paperHeight);


        gridLayout_2->addLayout(horizontalLayout_3, 1, 1, 1, 1);

        paperSourceLabel = new QLabel(groupBox_2);
        paperSourceLabel->setObjectName(QStringLiteral("paperSourceLabel"));

        gridLayout_2->addWidget(paperSourceLabel, 2, 0, 1, 1);

        paperSource = new QComboBox(groupBox_2);
        paperSource->setObjectName(QStringLiteral("paperSource"));

        gridLayout_2->addWidget(paperSource, 2, 1, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_4, 1, 2, 1, 1);


        gridLayout_3->addWidget(groupBox_2, 1, 0, 1, 2);

        groupBox_3 = new QGroupBox(QPageSetupWidget);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        verticalLayout = new QVBoxLayout(groupBox_3);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        portrait = new QRadioButton(groupBox_3);
        portrait->setObjectName(QStringLiteral("portrait"));
        portrait->setChecked(true);

        verticalLayout->addWidget(portrait);

        landscape = new QRadioButton(groupBox_3);
        landscape->setObjectName(QStringLiteral("landscape"));

        verticalLayout->addWidget(landscape);

        reverseLandscape = new QRadioButton(groupBox_3);
        reverseLandscape->setObjectName(QStringLiteral("reverseLandscape"));

        verticalLayout->addWidget(reverseLandscape);

        reversePortrait = new QRadioButton(groupBox_3);
        reversePortrait->setObjectName(QStringLiteral("reversePortrait"));

        verticalLayout->addWidget(reversePortrait);

        verticalSpacer_5 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_5);


        gridLayout_3->addWidget(groupBox_3, 2, 0, 1, 1);

        preview = new QWidget(QPageSetupWidget);
        preview->setObjectName(QStringLiteral("preview"));

        gridLayout_3->addWidget(preview, 2, 1, 2, 1);

        groupBox = new QGroupBox(QPageSetupWidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        horizontalLayout_2 = new QHBoxLayout(groupBox);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        topMargin = new QDoubleSpinBox(groupBox);
        topMargin->setObjectName(QStringLiteral("topMargin"));
        topMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        topMargin->setMaximum(999.99);

        gridLayout->addWidget(topMargin, 0, 1, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer_7 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_7);

        leftMargin = new QDoubleSpinBox(groupBox);
        leftMargin->setObjectName(QStringLiteral("leftMargin"));
        leftMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        leftMargin->setMaximum(999.99);

        horizontalLayout->addWidget(leftMargin);

        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        rightMargin = new QDoubleSpinBox(groupBox);
        rightMargin->setObjectName(QStringLiteral("rightMargin"));
        rightMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        rightMargin->setMaximum(999.99);

        horizontalLayout->addWidget(rightMargin);

        horizontalSpacer_8 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_8);


        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 3);

        bottomMargin = new QDoubleSpinBox(groupBox);
        bottomMargin->setObjectName(QStringLiteral("bottomMargin"));
        bottomMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        bottomMargin->setMaximum(999.99);

        gridLayout->addWidget(bottomMargin, 2, 1, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(0, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 2, 1, 1);

        horizontalSpacer_5 = new QSpacerItem(0, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_5, 0, 0, 1, 1);


        horizontalLayout_2->addLayout(gridLayout);


        gridLayout_3->addWidget(groupBox, 3, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(verticalSpacer, 4, 0, 1, 1);

#ifndef QT_NO_SHORTCUT
        pageSizeLabel->setBuddy(paperSize);
        widthLabel->setBuddy(paperWidth);
        heightLabel->setBuddy(paperHeight);
        paperSourceLabel->setBuddy(paperSource);
#endif // QT_NO_SHORTCUT

        retranslateUi(QPageSetupWidget);

        QMetaObject::connectSlotsByName(QPageSetupWidget);
    } // setupUi

    void retranslateUi(QWidget *QPageSetupWidget)
    {
        QPageSetupWidget->setWindowTitle(QApplication::translate("QPageSetupWidget", "Form", Q_NULLPTR));
        groupBox_2->setTitle(QApplication::translate("QPageSetupWidget", "Paper", Q_NULLPTR));
        pageSizeLabel->setText(QApplication::translate("QPageSetupWidget", "Page size:", Q_NULLPTR));
        widthLabel->setText(QApplication::translate("QPageSetupWidget", "Width:", Q_NULLPTR));
        heightLabel->setText(QApplication::translate("QPageSetupWidget", "Height:", Q_NULLPTR));
        paperSourceLabel->setText(QApplication::translate("QPageSetupWidget", "Paper source:", Q_NULLPTR));
        groupBox_3->setTitle(QApplication::translate("QPageSetupWidget", "Orientation", Q_NULLPTR));
        portrait->setText(QApplication::translate("QPageSetupWidget", "Portrait", Q_NULLPTR));
        landscape->setText(QApplication::translate("QPageSetupWidget", "Landscape", Q_NULLPTR));
        reverseLandscape->setText(QApplication::translate("QPageSetupWidget", "Reverse landscape", Q_NULLPTR));
        reversePortrait->setText(QApplication::translate("QPageSetupWidget", "Reverse portrait", Q_NULLPTR));
        groupBox->setTitle(QApplication::translate("QPageSetupWidget", "Margins", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        topMargin->setToolTip(QApplication::translate("QPageSetupWidget", "top margin", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_ACCESSIBILITY
        topMargin->setAccessibleName(QApplication::translate("QPageSetupWidget", "top margin", Q_NULLPTR));
#endif // QT_NO_ACCESSIBILITY
#ifndef QT_NO_TOOLTIP
        leftMargin->setToolTip(QApplication::translate("QPageSetupWidget", "left margin", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_ACCESSIBILITY
        leftMargin->setAccessibleName(QApplication::translate("QPageSetupWidget", "left margin", Q_NULLPTR));
#endif // QT_NO_ACCESSIBILITY
#ifndef QT_NO_TOOLTIP
        rightMargin->setToolTip(QApplication::translate("QPageSetupWidget", "right margin", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_ACCESSIBILITY
        rightMargin->setAccessibleName(QApplication::translate("QPageSetupWidget", "right margin", Q_NULLPTR));
#endif // QT_NO_ACCESSIBILITY
#ifndef QT_NO_TOOLTIP
        bottomMargin->setToolTip(QApplication::translate("QPageSetupWidget", "bottom margin", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_ACCESSIBILITY
        bottomMargin->setAccessibleName(QApplication::translate("QPageSetupWidget", "bottom margin", Q_NULLPTR));
#endif // QT_NO_ACCESSIBILITY
    } // retranslateUi

};

namespace Ui {
    class QPageSetupWidget: public Ui_QPageSetupWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QPAGESETUPWIDGET_H
