/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include <QtWidgets/QtWidgets>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QByteArray content;

    QWidget loadFileUi;
    QVBoxLayout *layout = new QVBoxLayout();
    QPushButton *loadFile = new QPushButton("Load File");
    QLabel *fileInfo = new QLabel("Opened file:");
    fileInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QLabel *fileHash = new QLabel("Sha256:");
    fileHash->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QPushButton *saveFile = new QPushButton("Save File");
    saveFile->setEnabled(false);

    auto onFileReady = [=, &content](const QString &fileName, const QByteArray &fileContents) {
        content = fileContents;
        fileInfo->setText(QString("Opened file: %1 size: %2").arg(fileName).arg(fileContents.size()));
        saveFile->setEnabled(true);

        auto computeDisplayFileHash = [=](){
          QByteArray hash = QCryptographicHash::hash(fileContents, QCryptographicHash::Sha256);
          fileHash->setText(QString("Sha256: %1").arg(QString(hash.toHex())));
        };

        QTimer::singleShot(100, computeDisplayFileHash); // update UI before computing hash
    };
    auto onLoadClicked = [=](){
        QFileDialog::getOpenFileContent("*.*", onFileReady);
    };
    QObject::connect(loadFile, &QPushButton::clicked, onLoadClicked);

    auto onSaveClicked = [=, &content]() {
        QFileDialog::saveFileContent(content, "qtsavefiletest.dat");
    };
    QObject::connect(saveFile, &QPushButton::clicked, onSaveClicked);

    layout->addWidget(loadFile);
    layout->addWidget(fileInfo);
    layout->addWidget(fileHash);
    layout->addWidget(saveFile);
    layout->addStretch();

    loadFileUi.setLayout(layout);
    loadFileUi.show();

    return app.exec();
}
