// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qbaselinetest.h>
#include <qwidgetbaselinetest.h>
#include <QtWidgets>
#include <QByteArray>

class tst_Stylesheet : public QWidgetBaselineTest
{
    Q_OBJECT

public:
    tst_Stylesheet();

    void loadTestFiles();

    void doInit() override;

private slots:
    void tst_QToolButton_data();
    void tst_QToolButton();

    void tst_QScrollArea_data();
    void tst_QScrollArea();

    void tst_QTreeView_data();
    void tst_QTreeView();

private:
    QDir styleSheetDir;
};

tst_Stylesheet::tst_Stylesheet()
{
    QString baseDir = QFINDTESTDATA("qss/default.qss");
    styleSheetDir = QDir(QFileInfo(baseDir).path());
}

void tst_Stylesheet::doInit()
{
    QFETCH(QString, styleSheet);
    testWindow()->setStyleSheet(styleSheet);
}

void tst_Stylesheet::loadTestFiles()
{
    QTest::addColumn<QString>("styleSheet");

    QStringList qssFiles;
    // first add generic test files
    for (const auto &qssFile : styleSheetDir.entryList({QStringLiteral("*.qss")}, QDir::Files | QDir::Readable))
        qssFiles << styleSheetDir.absoluteFilePath(qssFile);

    // then test-function specific files
    const QString testFunction = QString(QTest::currentTestFunction()).remove("tst_").toLower();
    if (styleSheetDir.cd(testFunction)) {
        for (const auto &qssFile : styleSheetDir.entryList({QStringLiteral("*.qss")}, QDir::Files | QDir::Readable))
            qssFiles << styleSheetDir.absoluteFilePath(qssFile);
        styleSheetDir.cdUp();
    }

    for (const auto &qssFile : qssFiles) {
        QFileInfo fileInfo(qssFile);
        QFile file(qssFile);
        file.open(QFile::ReadOnly);
        QString styleSheet = QString::fromUtf8(file.readAll());
        QBaselineTest::newRow(fileInfo.baseName().toUtf8()) << styleSheet;
    }
}

void tst_Stylesheet::tst_QToolButton_data()
{
    loadTestFiles();
}

void tst_Stylesheet::tst_QToolButton()
{
    const QIcon trashIcon = QApplication::style()->standardIcon(QStyle::SP_TrashIcon);

    QVBoxLayout *vbox = new QVBoxLayout;

    QHBoxLayout *normalButtons = new QHBoxLayout;
    for (const auto &buttonStyle : {Qt::ToolButtonIconOnly, Qt::ToolButtonTextOnly,
                                    Qt::ToolButtonTextUnderIcon, Qt::ToolButtonTextBesideIcon}) {
        QToolButton *normal = new QToolButton;
        normal->setToolButtonStyle(buttonStyle);
        normal->setText("Norm");
        normal->setIcon(trashIcon);
        normalButtons->addWidget(normal);
    }
    vbox->addLayout(normalButtons);

    QHBoxLayout *arrowButtons = new QHBoxLayout;
    for (const auto &arrowType : {Qt::LeftArrow, Qt::RightArrow, Qt::UpArrow, Qt::DownArrow}) {
        QToolButton *arrow = new QToolButton;
        arrow->setText("Arrs");
        arrow->setArrowType(arrowType);
        arrowButtons->addWidget(arrow);
    }
    vbox->addLayout(arrowButtons);

    QHBoxLayout *arrowWithTextButtons = new QHBoxLayout;
    for (const auto &buttonStyle : {Qt::ToolButtonTextOnly,
                                    Qt::ToolButtonTextUnderIcon, Qt::ToolButtonTextBesideIcon}) {
        QToolButton *arrow = new QToolButton;
        arrow->setText("ArrTxt");
        arrow->setArrowType(Qt::UpArrow);
        arrow->setToolButtonStyle(buttonStyle);
        arrowWithTextButtons->addWidget(arrow);
    }
    vbox->addLayout(arrowWithTextButtons);

    QHBoxLayout *menuButtons = new QHBoxLayout;
    for (const auto &popupMode : {QToolButton::InstantPopup, QToolButton::MenuButtonPopup,
                                  QToolButton::DelayedPopup}) {
        QToolButton *menuButton = new QToolButton;
        menuButton->setText("PppMd");
        menuButton->setIcon(trashIcon);
        QMenu *menuButtonMenu = new QMenu;
        menuButtonMenu->addAction(QIcon(":/icons/align-left.png"), "Left");
        menuButtonMenu->addAction(QIcon(":/icons/align-right.png"), "Right");
        menuButtonMenu->addAction(QIcon(":/icons/align-center.png"), "Center");
        menuButton->setMenu(menuButtonMenu);
        menuButton->setPopupMode(popupMode);
        menuButtons->addWidget(menuButton);
    }
    vbox->addLayout(menuButtons);
    testWindow()->setLayout(vbox);

    makeVisible();
    QBASELINE_TEST(takeSnapshot());
}

void tst_Stylesheet::tst_QScrollArea_data()
{
    loadTestFiles();
}

void tst_Stylesheet::tst_QScrollArea()
{
    QHBoxLayout *layout = new QHBoxLayout;
    QTableWidget *table = new QTableWidget(20, 20);
    layout->addWidget(table);
    testWindow()->setLayout(layout);

    makeVisible();
    QBASELINE_TEST(takeSnapshot());
}

void tst_Stylesheet::tst_QTreeView_data()
{
    loadTestFiles();
}

void tst_Stylesheet::tst_QTreeView()
{
    QHBoxLayout *layout = new QHBoxLayout;
    QTreeWidget *tw = new QTreeWidget();
    tw->header()->hide();
    layout->addWidget(tw);

    enum {
        Unchecked           = 0,
        Checked             = 1,
        Children            = 2,
        Disabled            = 3,
        CheckedDisabled     = 4,
        ChildrenDisabled    = 5,
        NConfigs
    };

    for (int i = 0; i < NConfigs; ++i) {
        QTreeWidgetItem *topLevelItem = new QTreeWidgetItem(tw, QStringList{QString("top %1").arg(i)});
        switch (i) {
        case Unchecked:
        case Disabled:
            topLevelItem->setCheckState(0, Qt::Unchecked);
            break;
        case Checked:
        case CheckedDisabled:
            topLevelItem->setCheckState(0, Qt::Checked);
            break;
        case Children:
        case ChildrenDisabled:
            topLevelItem->setCheckState(0, Qt::PartiallyChecked);
            topLevelItem->setExpanded(true);
            for (int j = 0; j < 2; ++j) {
                QTreeWidgetItem *childItem = new QTreeWidgetItem(topLevelItem, QStringList{QString("child %1").arg(j)});
                childItem->setCheckState(0, j % 2 ? Qt::Unchecked : Qt::Checked);
            }
            break;
        }
        topLevelItem->setDisabled(i >= Disabled);
    }
    testWindow()->setLayout(layout);
    tw->setRootIsDecorated(true);
    makeVisible();

    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "rootDecorated");
    tw->setRootIsDecorated(false);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "rootNotDecorated");

    tw->topLevelItem(Children)->child(0)->setSelected(true);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "itemSelected");
}

#define main _realmain
QTEST_MAIN(tst_Stylesheet)
#undef main

int main(int argc, char *argv[])
{
    // Avoid rendering variations caused by QHash randomization
    QHashSeed::setDeterministicGlobalSeed();

    QBaselineTest::handleCmdLineArgs(&argc, &argv);
    return _realmain(argc, argv);
}

#include "tst_baseline_stylesheet.moc"
