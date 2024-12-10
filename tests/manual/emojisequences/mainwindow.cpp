// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->toolButton, &QToolButton::clicked, this, &MainWindow::loadCustomFont);

    populateEmojiTest();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::populateEmojiTest()
{
    QFile file(":/emoji-test.txt");

    ui->tableWidget->clear();
    ui->tableWidget->setColumnCount(8);

    QList<QPair<QString, QString> > strings;
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QString l = file.readLine();

            QStringList toolTip;
            QString testString;
            QStringList tokens = l.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (int i = 0; i < tokens.size(); ++i) {
                if (tokens.at(i) == QLatin1Char(';'))
                    break;

                bool ok;
                char32_t ucs4 = tokens.at(i).toUInt(&ok, 16);
                if (!ok)
                    break;

                testString += QString::fromUcs4(&ucs4, 1);
                toolTip << QString::number(ucs4, 16);
            }

            if (!toolTip.isEmpty()) {
                strings.append(qMakePair(testString, toolTip.join(',')));
            }
        }
    }

    ui->tableWidget->setRowCount(strings.count() / 8);
    for (int i = 0; i < strings.count(); ++i) {
        int row = i / 8;
        int column = i % 8;
        QString testString = strings.at(i).first;
        QString toolTip = strings.at(i).second;

        QTableWidgetItem *it = new QTableWidgetItem(testString);
        ui->tableWidget->setItem(row, column, it);
        it->setText(testString);
        it->setToolTip(toolTip);
    }
}

void MainWindow::loadCustomFont()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    ui->tableWidget->clear();
    QFontDatabase::removeAllApplicationFonts();

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open font file"), QString(), tr("Fonts (*.ttf *.otf);All files (*)"));
    if (!fileName.isEmpty()) {
        int id = QFontDatabase::addApplicationFont(fileName);
        if (id >= 0) {
            QStringList families = QFontDatabase::applicationFontFamilies(id);
            QString family = families.size() > 0 ? families.first() : QString();
            if (!family.isEmpty()) {
                QFontDatabase::setApplicationEmojiFontFamilies(QStringList() << family);
                populateEmojiTest();
            }
        }
    }
#endif
}

