// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "characterwidget.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStatusBar>
#include <QTextStream>

//! [0]
Q_DECLARE_METATYPE(QFontComboBox::FontFilter)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(tr("Quit"), this, &QWidget::close);
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("Show Font Info"), this, &MainWindow::showInfo);
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);

    QWidget *centralWidget = new QWidget;

    QLabel *filterLabel = new QLabel(tr("Filter:"));
    filterCombo = new QComboBox;
    filterCombo->addItem(tr("All"), QVariant::fromValue(QFontComboBox::AllFonts));
    filterCombo->addItem(tr("Scalable"), QVariant::fromValue(QFontComboBox::ScalableFonts));
    filterCombo->addItem(tr("Monospaced"), QVariant::fromValue(QFontComboBox::MonospacedFonts));
    filterCombo->addItem(tr("Proportional"), QVariant::fromValue(QFontComboBox::ProportionalFonts));
    filterCombo->setCurrentIndex(0);
    connect(filterCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::filterChanged);

    QLabel *fontLabel = new QLabel(tr("Font:"));
    fontCombo = new QFontComboBox;
    QLabel *sizeLabel = new QLabel(tr("Size:"));
    sizeCombo = new QComboBox;
    QLabel *styleLabel = new QLabel(tr("Style:"));
    styleCombo = new QComboBox;
    QLabel *fontMergingLabel = new QLabel(tr("Automatic Font Merging:"));
    fontMerging = new QCheckBox;
    fontMerging->setChecked(true);

    scrollArea = new QScrollArea;
    characterWidget = new CharacterWidget;
    scrollArea->setWidget(characterWidget);
//! [0]

//! [1]
    findStyles(fontCombo->currentFont());
//! [1]
    findSizes(fontCombo->currentFont());

//! [2]
    lineEdit = new QLineEdit;
    lineEdit->setClearButtonEnabled(true);
#ifndef QT_NO_CLIPBOARD
    QPushButton *clipboardButton = new QPushButton(tr("&To clipboard"));
//! [2]

#endif

//! [4]
    connect(fontCombo, &QFontComboBox::currentFontChanged,
            this, &MainWindow::findStyles);
    connect(fontCombo, &QFontComboBox::currentFontChanged,
            this, &MainWindow::findSizes);
    connect(fontCombo, &QFontComboBox::currentFontChanged,
            characterWidget, &CharacterWidget::updateFont);
    connect(sizeCombo, &QComboBox::currentTextChanged,
            characterWidget, &CharacterWidget::updateSize);
    connect(styleCombo, &QComboBox::currentTextChanged,
            characterWidget, &CharacterWidget::updateStyle);
//! [4] //! [5]
    connect(characterWidget, &CharacterWidget::characterSelected,
            this, &MainWindow::insertCharacter);

#ifndef QT_NO_CLIPBOARD
    connect(clipboardButton, &QAbstractButton::clicked, this, &MainWindow::updateClipboard);
#endif
//! [5]
    connect(fontMerging, &QAbstractButton::toggled, characterWidget, &CharacterWidget::updateFontMerging);

//! [6]
    QHBoxLayout *controlsLayout = new QHBoxLayout;
    controlsLayout->addWidget(filterLabel);
    controlsLayout->addWidget(filterCombo, 1);
    controlsLayout->addWidget(fontLabel);
    controlsLayout->addWidget(fontCombo, 1);
    controlsLayout->addWidget(sizeLabel);
    controlsLayout->addWidget(sizeCombo, 1);
    controlsLayout->addWidget(styleLabel);
    controlsLayout->addWidget(styleCombo, 1);
    controlsLayout->addWidget(fontMergingLabel);
    controlsLayout->addWidget(fontMerging, 1);
    controlsLayout->addStretch(1);

    QHBoxLayout *lineLayout = new QHBoxLayout;
    lineLayout->addWidget(lineEdit, 1);
    lineLayout->addSpacing(12);
#ifndef QT_NO_CLIPBOARD
    lineLayout->addWidget(clipboardButton);
#endif

    QVBoxLayout *centralLayout = new QVBoxLayout;
    centralLayout->addLayout(controlsLayout);
    centralLayout->addWidget(scrollArea, 1);
    centralLayout->addSpacing(4);
    centralLayout->addLayout(lineLayout);
    centralWidget->setLayout(centralLayout);

    setCentralWidget(centralWidget);
    setWindowTitle(tr("Character Map"));
}
//! [6]

//! [7]
void MainWindow::findStyles(const QFont &font)
{
    QString currentItem = styleCombo->currentText();
    styleCombo->clear();
//! [7]

//! [8]
    const QStringList styles = QFontDatabase::styles(font.family());
    for (const QString &style : styles)
        styleCombo->addItem(style);

    int styleIndex = styleCombo->findText(currentItem);

    if (styleIndex == -1)
        styleCombo->setCurrentIndex(0);
    else
        styleCombo->setCurrentIndex(styleIndex);
}
//! [8]

void MainWindow::filterChanged(int f)
{
    const QFontComboBox::FontFilter filter =
        qvariant_cast<QFontComboBox::FontFilter>(filterCombo->itemData(f));
    fontCombo->setFontFilters(filter);
    statusBar()->showMessage(tr("%n font(s) found", nullptr, fontCombo->count()));
}

void MainWindow::findSizes(const QFont &font)
{
    QString currentSize = sizeCombo->currentText();

    {
        const QSignalBlocker blocker(sizeCombo);
        // sizeCombo signals are now blocked until end of scope
        sizeCombo->clear();

        if (QFontDatabase::isSmoothlyScalable(font.family(), QFontDatabase::styleString(font))) {
            const QList<int> sizes = QFontDatabase::standardSizes();
            for (const int size : sizes) {
                sizeCombo->addItem(QVariant(size).toString());
                sizeCombo->setEditable(true);
            }

        } else {
            const QList<int> sizes = QFontDatabase::smoothSizes(font.family(), QFontDatabase::styleString(font));
            for (const int size : sizes ) {
                sizeCombo->addItem(QVariant(size).toString());
                sizeCombo->setEditable(false);
            }
        }
    }

    int sizeIndex = sizeCombo->findText(currentSize);

    if(sizeIndex == -1)
        sizeCombo->setCurrentIndex(qMax(0, sizeCombo->count() / 3));
    else
        sizeCombo->setCurrentIndex(sizeIndex);
}

//! [9]
void MainWindow::insertCharacter(const QString &character)
{
    lineEdit->insert(character);
}
//! [9]

//! [10]
#ifndef QT_NO_CLIPBOARD
void MainWindow::updateClipboard()
{
//! [11]
    QGuiApplication::clipboard()->setText(lineEdit->text(), QClipboard::Clipboard);
//! [11]
    QGuiApplication::clipboard()->setText(lineEdit->text(), QClipboard::Selection);
}
#endif

class FontInfoDialog : public QDialog
{
public:
    explicit FontInfoDialog(QWidget *parent = nullptr);

private:
    QString text() const;
};

FontInfoDialog::FontInfoDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QPlainTextEdit *textEdit = new QPlainTextEdit(text(), this);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    mainLayout->addWidget(textEdit);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

QString FontInfoDialog::text() const
{
    QString text;
    QTextStream str(&text);
    const QFont defaultFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QFont titleFont = QFontDatabase::systemFont(QFontDatabase::TitleFont);
    const QFont smallestReadableFont = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);

    str << "Qt " << QT_VERSION_STR << " on " << QGuiApplication::platformName()
        << ", " << logicalDpiX() << "DPI";
    if (!qFuzzyCompare(devicePixelRatio(), qreal(1)))
        str  << ", device pixel ratio: " << devicePixelRatio();
    str << "\n\nDefault font : " << defaultFont.family() << ", " << defaultFont.pointSizeF() << "pt\n"
        << "Fixed font   : " << fixedFont.family() << ", " << fixedFont.pointSizeF() << "pt\n"
        << "Title font   : " << titleFont.family() << ", " << titleFont.pointSizeF() << "pt\n"
        << "Smallest font: " << smallestReadableFont.family() << ", " << smallestReadableFont.pointSizeF() << "pt\n";

    return text;
}

void MainWindow::showInfo()
{
    const QRect screenGeometry = screen()->geometry();
    FontInfoDialog *dialog = new FontInfoDialog(this);
    dialog->setWindowTitle(tr("Fonts"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(screenGeometry.width() / 4, screenGeometry.height() / 4);
    dialog->show();
}

//! [10]
