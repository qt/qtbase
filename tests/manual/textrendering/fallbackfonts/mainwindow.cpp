// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui>
#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyTerritory);
    int selectedIndex = 0;
    for (QLocale locale : allLocales) {
        if (QLocale::system() == locale)
            selectedIndex = ui->cbLocale->count();
        ui->cbLocale->addItem(locale.name() + " | " + locale.nativeLanguageName() + " | " + locale.nativeTerritoryName(),
                              QVariant::fromValue(locale));
    }
    ui->cbLocale->setCurrentIndex(selectedIndex);

    connect(ui->fcbDefaultFont, &QFontComboBox::currentFontChanged, this, &MainWindow::updateList);
    connect(ui->cbLocale, &QComboBox::currentIndexChanged, this, &MainWindow::updateList);
    connect(ui->leSample, &QLineEdit::textChanged, this, &MainWindow::updateList);

    updateList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateList()
{
    delete ui->scrollArea->widget();

    QLocale::setDefault(ui->cbLocale->currentData().value<QLocale>());

    QWidget *w = new QWidget;
    ui->scrollArea->setWidget(w);

    QVBoxLayout *l = new QVBoxLayout(w);

    for (int i = QFontDatabase::Any; i < QFontDatabase::WritingSystemsCount; ++i) {
        QString s = i == QFontDatabase::Any
                        ? ui->leSample->text()
                        : QFontDatabase::writingSystemSample(QFontDatabase::WritingSystem(i));
        if (!s.isEmpty()) {
            QTextLayout lout;
            lout.setFont(ui->fcbDefaultFont->currentFont());

            lout.setText(s);
            lout.beginLayout(); lout.createLine(); lout.endLayout();

            QSet<QString> families;
            QList<QGlyphRun> grs = lout.glyphRuns();
            for (const QGlyphRun &gr : grs)
                families.insert(gr.rawFont().familyName());

            QLabel *label = new QLabel;
            label->setFont(ui->fcbDefaultFont->currentFont());
            label->setText(QFontDatabase::writingSystemName(QFontDatabase::WritingSystem(i))
                           + QStringLiteral(": ")
                           + s
                           + QStringLiteral("; ")
                           + families.values().join(','));
            l->addWidget(label);
        }
    }

}
