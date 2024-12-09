// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    ui->cbWeight->addItem(QStringLiteral("Thin"), QFont::Thin);
    ui->cbWeight->addItem(QStringLiteral("ExtraLight"), QFont::ExtraLight);
    ui->cbWeight->addItem(QStringLiteral("Light"), QFont::Light);
    ui->cbWeight->addItem(QStringLiteral("Normal"), QFont::Normal);
    ui->cbWeight->addItem(QStringLiteral("Medium"), QFont::Medium);
    ui->cbWeight->addItem(QStringLiteral("DemiBold"), QFont::DemiBold);
    ui->cbWeight->addItem(QStringLiteral("Bold"), QFont::Bold);
    ui->cbWeight->addItem(QStringLiteral("ExtraBold"), QFont::ExtraBold);
    ui->cbWeight->addItem(QStringLiteral("Black"), QFont::Black);
    ui->cbWeight->setCurrentIndex(3);

    updateFont();

    connect(ui->sbPixelSize, &QSpinBox::valueChanged, this, &MainWindow::updateFont);
    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this, &MainWindow::updateFont);
    connect(ui->rbDefault, &QRadioButton::toggled, this, &MainWindow::updateFont);
    connect(ui->rbGdi, &QRadioButton::toggled, this, &MainWindow::updateFont);
    connect(ui->rbFreetype, &QRadioButton::toggled, this, &MainWindow::updateFont);
    connect(ui->leText, &QLineEdit::textChanged, this, &MainWindow::updateFont);
    connect(ui->cbWeight, &QComboBox::currentIndexChanged, this, &MainWindow::updateFont);
    connect(ui->cbItalic, &QCheckBox::toggled, this, &MainWindow::updateFont);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateImage()
{
    if (m_process == nullptr)
        return;

    QImage img(QStringLiteral("output.png"));
    if (!img.isNull())
        ui->lImage->setPixmap(QPixmap::fromImage(img));
}

void MainWindow::updateFont()
{
    if (m_process == nullptr) {
        m_process = new QProcess;
        connect(m_process, &QProcess::finished, this, &MainWindow::updateImage);
    }

    if (m_process->isOpen())
        m_process->close();

    QString fontEngineName = QStringLiteral("directwrite");
    if (ui->rbGdi->isChecked())
        fontEngineName = QStringLiteral("gdi");
    else if (ui->rbFreetype->isChecked())
        fontEngineName = QStringLiteral("freetype");

    QProcessEnvironment env;
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("windows:fontengine=%1").arg(fontEngineName));
    env.insert(QStringLiteral("windir"), qgetenv("windir"));
    m_process->setProcessEnvironment(env);

    QStringList args;
    args.append(ui->fontComboBox->currentFont().family());
    args.append(QString::number(ui->sbPixelSize->value()));
    args.append(ui->leText->text().isEmpty()
                    ? QStringLiteral("The quick brown fox jumps over the lazy dog")
                    : ui->leText->text());
    args.append(QString::number(ui->cbWeight->currentData().toInt()));
    args.append(QString::number(int(ui->cbItalic->isChecked())));

    m_process->start(qApp->arguments().first(), args);
    m_process->waitForFinished();
}
