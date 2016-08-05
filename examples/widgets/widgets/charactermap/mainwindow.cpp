/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include "characterwidget.h"
#include "mainwindow.h"

//! [0]

Q_DECLARE_METATYPE(QFontComboBox::FontFilter)

MainWindow::MainWindow()
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
    typedef void (QComboBox::*QComboBoxIntSignal)(int);
    connect(filterCombo, static_cast<QComboBoxIntSignal>(&QComboBox::currentIndexChanged),
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
    typedef void (QComboBox::*QComboBoxStringSignal)(const QString &);
    connect(sizeCombo, static_cast<QComboBoxStringSignal>(&QComboBox::currentIndexChanged),
            characterWidget, &CharacterWidget::updateSize);
    connect(styleCombo, static_cast<QComboBoxStringSignal>(&QComboBox::currentIndexChanged),
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
    QFontDatabase fontDatabase;
    QString currentItem = styleCombo->currentText();
    styleCombo->clear();
//! [7]

//! [8]
    QString style;
    foreach (style, fontDatabase.styles(font.family()))
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
        filterCombo->itemData(f).value<QFontComboBox::FontFilter>();
    fontCombo->setFontFilters(filter);
    statusBar()->showMessage(tr("%n font(s) found", 0, fontCombo->count()));
}

void MainWindow::findSizes(const QFont &font)
{
    QFontDatabase fontDatabase;
    QString currentSize = sizeCombo->currentText();

    {
        const QSignalBlocker blocker(sizeCombo);
        // sizeCombo signals are now blocked until end of scope
        sizeCombo->clear();

        int size;
        if (fontDatabase.isSmoothlyScalable(font.family(), fontDatabase.styleString(font))) {
            foreach (size, QFontDatabase::standardSizes()) {
                sizeCombo->addItem(QVariant(size).toString());
                sizeCombo->setEditable(true);
            }

        } else {
            foreach (size, fontDatabase.smoothSizes(font.family(), fontDatabase.styleString(font))) {
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
    explicit FontInfoDialog(QWidget *parent = Q_NULLPTR);

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
    if (!qFuzzyCompare(devicePixelRatioF(), qreal(1)))
        str  << ", device pixel ratio: " << devicePixelRatioF();
    str << "\n\nDefault font : " << defaultFont.family() << ", " << defaultFont.pointSizeF() << "pt\n"
        << "Fixed font   : " << fixedFont.family() << ", " << fixedFont.pointSizeF() << "pt\n"
        << "Title font   : " << titleFont.family() << ", " << titleFont.pointSizeF() << "pt\n"
        << "Smallest font: " << smallestReadableFont.family() << ", " << smallestReadableFont.pointSizeF() << "pt\n";

    return text;
}

void MainWindow::showInfo()
{
    const QRect screenGeometry = QApplication::desktop()->screenGeometry(this);
    FontInfoDialog *dialog = new FontInfoDialog(this);
    dialog->setWindowTitle(tr("Fonts"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(screenGeometry.width() / 4, screenGeometry.height() / 4);
    dialog->show();
}

//! [10]
