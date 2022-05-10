// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
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
