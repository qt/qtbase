// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setup();
    updateSampleText();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateSampleText()
{
    QFont font = ui->fontComboBox->currentFont();
    font.setPixelSize(54);

    for (int i = 0; i < ui->lwFeatures->count(); ++i) {
        QListWidgetItem *it = ui->lwFeatures->item(i);
        if (it->checkState() != Qt::PartiallyChecked) {
            if (const auto maybeTag = QFont::Tag::fromString(it->text().toLatin1()))
                font.setFeature(*maybeTag, !!it->checkState());
        }
    }

    ui->lSampleDisplay->setFont(font);
    ui->lSampleDisplay->setText(ui->leSampleText->text());
}

void MainWindow::enableAll()
{
    for (int i = 0; i < ui->lwFeatures->count(); ++i) {
        QListWidgetItem *it = ui->lwFeatures->item(i);
        it->setCheckState(Qt::Checked);
    }
}

void MainWindow::disableAll()
{
    for (int i = 0; i < ui->lwFeatures->count(); ++i) {
        QListWidgetItem *it = ui->lwFeatures->item(i);
        it->setCheckState(Qt::Unchecked);
    }
}

void MainWindow::reset()
{
    for (int i = 0; i < ui->lwFeatures->count(); ++i) {
        QListWidgetItem *it = ui->lwFeatures->item(i);
        it->setCheckState(Qt::PartiallyChecked);
    }
}

void MainWindow::setup()
{
    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this, &MainWindow::updateSampleText);
    connect(ui->leSampleText, &QLineEdit::textChanged, this, &MainWindow::updateSampleText);
    connect(ui->lwFeatures, &QListWidget::itemChanged, this, &MainWindow::updateSampleText);
    connect(ui->pbEnableAll, &QPushButton::clicked, this, &MainWindow::enableAll);
    connect(ui->pbDisableAll, &QPushButton::clicked, this, &MainWindow::disableAll);
    connect(ui->pbReset, &QPushButton::clicked, this, &MainWindow::reset);

    QList<QByteArray> featureList =
    {
        "aalt",
        "abvf",
        "abvm",
        "abvs",
        "afrc",
        "akhn",
        "blwf",
        "blwm",
        "blws",
        "calt",
        "case",
        "ccmp",
        "cfar",
        "chws",
        "cjct",
        "clig",
        "cpct",
        "cpsp",
        "cswh",
        "curs",
        "cv01",
        "c2pc",
        "c2sc",
        "dist",
        "dlig",
        "dnom",
        "dtls",
        "expt",
        "falt",
        "fin2",
        "fin3",
        "fina",
        "flac",
        "frac",
        "fwid",
        "half",
        "haln",
        "halt",
        "hist",
        "hkna",
        "hlig",
        "hngl",
        "hojo",
        "hwid",
        "init",
        "isol",
        "ital",
        "jalt",
        "jp78",
        "jp83",
        "jp90",
        "jp04",
        "kern",
        "lfbd",
        "liga",
        "ljmo",
        "lnum",
        "locl",
        "ltra",
        "ltrm",
        "mark",
        "med2",
        "medi",
        "mgrk",
        "mkmk",
        "mset",
        "nalt",
        "nlck",
        "nukt",
        "numr",
        "onum",
        "opbd",
        "ordn",
        "ornm",
        "palt",
        "pcap",
        "pkna",
        "pnum",
        "pref",
        "pres",
        "pstf",
        "psts",
        "pwid",
        "qwid",
        "rand",
        "rclt",
        "rkrf",
        "rlig",
        "rphf",
        "rtbd",
        "rtla",
        "rtlm",
        "ruby",
        "rvrn",
        "salt",
        "sinf",
        "size",
        "smcp",
        "smpl",
        "ss01",
        "ss02",
        "ss03",
        "ss04",
        "ss05",
        "ss06",
        "ss07",
        "ss08",
        "ss09",
        "ss10",
        "ss11",
        "ss12",
        "ss13",
        "ss14",
        "ss15",
        "ss16",
        "ss17",
        "ss18",
        "ss19",
        "ss20",
        "ssty",
        "stch",
        "subs",
        "sups",
        "swsh",
        "titl",
        "tjmo",
        "tnam",
        "tnum",
        "trad",
        "twid",
        "unic",
        "valt",
        "vatu",
        "vchw",
        "vert",
        "vhal",
        "vjmo",
        "vkna",
        "vkrn",
        "vpal",
        "vrt2",
        "vrtr",
        "zero"
    };

    for (auto it = featureList.constBegin(); it != featureList.constEnd(); ++it) {
        QListWidgetItem *item = new QListWidgetItem(*it);
        item->setFlags(Qt::ItemIsUserTristate | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::PartiallyChecked);
        ui->lwFeatures->addItem(item);
    }
}
