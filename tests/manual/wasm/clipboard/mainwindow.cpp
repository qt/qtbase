/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QClipboard>
#include <QMimeData>
#include <QImageReader>
#include <QBuffer>
#include <QRandomGenerator>
#include <QPainter>
#include <QKeyEvent>

#ifdef Q_OS_WASM
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>

using namespace emscripten;
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->imageLabel->installEventFilter(this);

    ui->imageLabel->setBackgroundRole(QPalette::Base);
    ui->imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->imageLabel->setScaledContents(true);

    clipboard = QGuiApplication::clipboard();
    connect(
        clipboard, &QClipboard::dataChanged,
        [=]() {
        ui->textEdit_2->insertHtml("<b>Clipboard data changed:</b><br>");
        const QMimeData *mimeData = clipboard->mimeData();
        QByteArray ba;

        for (auto mimetype : mimeData->formats()) {
            qDebug() << Q_FUNC_INFO << mimetype;
            ba = mimeData->data(mimetype);
        }
            QString sizeStr;

        if (mimeData->hasImage()) {
            qsizetype imageSize = qvariant_cast<QImage>(mimeData->imageData()).sizeInBytes();
            sizeStr.setNum(imageSize);
            ui->textEdit_2->insertHtml("has Image data: " + sizeStr + "<br>");
        }

        if (mimeData->hasHtml()) {
            int size = mimeData->html().length();
            sizeStr.setNum(size);
            ui->textEdit_2->insertHtml("has html data: " + sizeStr + "<br>");
        }
        if (mimeData->hasText()) {
            int size = mimeData->text().length();
            sizeStr.setNum(size);
            ui->textEdit_2->insertHtml("has text data: " + sizeStr + "<br>");
        }

        ui->textEdit_2->insertHtml(mimeData->formats().join(" | ")+ "<br>");

        ui->textEdit_2->ensureCursorVisible();

        const QString message = tr("Clipboard changed, %1 ")
            .arg(mimeData->formats().join(' '));

        statusBar()->showMessage(message + sizeStr);
    }
    );
#ifdef Q_OS_WASM
    val clipboard = val::global("navigator")["clipboard"];
    bool hasClipboardApi = (!clipboard.isUndefined() && !clipboard["readText"].isUndefined());
    QString messageApi;
    if (hasClipboardApi)
       messageApi = QStringLiteral("Using Clipboard API");
    else
        messageApi = QStringLiteral("Using Clipboard events");
     ui->label->setText(messageApi);
#else
    ui->label->setText("desktop clipboard");
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_setTextButton_clicked()
{
    QGuiApplication::clipboard()->setText(ui->textEdit->textCursor().selectedText());
}

static QImage clipboardImage()
{
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData()) {
        if (mimeData->hasImage()) {
            const QImage image = qvariant_cast<QImage>(mimeData->imageData());
            if (!image.isNull())
                return image;
        }
    }
    return QImage();
}

static QByteArray clipboardBinary()
{
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData()) {

        if (mimeData->formats().contains("application/octet-stream")) {
            const QByteArray ba = qvariant_cast<QByteArray>(mimeData->data("application/octet-stream"));
            qDebug() << Q_FUNC_INFO << ba;
            if (!ba.isNull())
                return ba;
        }
    }
    return QByteArray();
}

void MainWindow::on_pasteImageButton_clicked()
{
    const QImage newImage = clipboardImage();
    if (newImage.isNull()) {
        qDebug() << "No image in clipboard";
        const QString message = tr("No image in clipboard")
            .arg(newImage.width()).arg(newImage.height()).arg(newImage.depth());
        statusBar()->showMessage(message);
    } else {
        setImage(newImage);
        setWindowFilePath(QString());
        const QString message = tr("Obtained image from clipboard, %1x%2, Depth: %3")
            .arg(newImage.width()).arg(newImage.height()).arg(newImage.depth());
        statusBar()->showMessage(message);
    }
}

void MainWindow::setImage(const QImage &newImage)
{
    image = newImage;
    ui->imageLabel->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::on_pasteTextButton_clicked()
{
    ui->textEdit->insertPlainText(QGuiApplication::clipboard()->text());
}

void MainWindow::on_copyBinaryButton_clicked()
{
    QByteArray ba;
    ba.resize(10);
    ba[0] = 0x3c;
    ba[1] = 0xb8;
    ba[2] = 0x64;
    ba[3] = 0x18;
    ba[4] = 0xca;
    ba[5] = 0xca;
    ba[6] = 0x18;
    ba[7] = 0x64;
    ba[8] = 0xb8;
    ba[9] = 0x3c;

    QMimeData *mimeData = new QMimeData();
    mimeData->setData("application/octet-stream", ba);
    QGuiApplication::clipboard()->setMimeData(mimeData);

    const QString message = tr("Copied binary to clipboard: " + ba + " 10 bytes");
    statusBar()->showMessage(message);
}

void MainWindow::on_pasteBinaryButton_clicked()
{
    const QByteArray ba = clipboardBinary();
    if (ba.isNull()) {
        qDebug() << "No binary in clipboard";
        const QString message = tr("No binary in clipboard");
        statusBar()->showMessage(message);
    } else {
        setWindowFilePath(QString());
        const QString message = tr("Obtained binary from clipboard: " + ba);
        statusBar()->showMessage(message);
    }
}

void MainWindow::on_comboBox_textActivated(const QString &arg1)
{
    QImage image(QSize(150,100), QImage::Format_RGB32);
    QPainter painter(&image);
    painter.fillRect(QRectF(0,0,150,100),generateRandomColor());
    painter.fillRect(QRectF(20,30,130,40),generateRandomColor());
    painter.setPen(QPen(generateRandomColor()));
    painter.drawText(QRect(25,30,130,40),"Qt WebAssembly");

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, arg1.toLocal8Bit());

    qDebug() << ba.mid(0,10) << ba.length();
    qDebug() << Q_FUNC_INFO << image.sizeInBytes();

    QGuiApplication::clipboard()->setImage(image);
}

QColor MainWindow::generateRandomColor()
{
    return QColor::fromRgb(QRandomGenerator::global()->generate());
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_V && ke->modifiers().testFlag(Qt::ControlModifier)) {
            if (obj == ui->imageLabel) {
                setImage(clipboardImage());
                return true;
            }
        }
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
}

void MainWindow::on_pasteHtmlButton_clicked()
{
    ui->textEdit->insertHtml(QGuiApplication::clipboard()->mimeData()->html());
}

void MainWindow::on_clearButton_clicked()
{
    ui->textEdit_2->clear();
}

