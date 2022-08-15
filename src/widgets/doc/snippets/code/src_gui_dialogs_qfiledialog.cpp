// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
fileName = QFileDialog::getOpenFileName(this,
    tr("Open Image"), "/home/jana", tr("Image Files (*.png *.jpg *.bmp)"));
//! [0]


//! [1]
"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)"
//! [1]


//! [2]
QFileDialog dialog(this);
dialog.setFileMode(QFileDialog::AnyFile);
//! [2]


//! [3]
dialog.setNameFilter(tr("Images (*.png *.xpm *.jpg)"));
//! [3]


//! [4]
dialog.setViewMode(QFileDialog::Detail);
//! [4]


//! [5]
QStringList fileNames;
if (dialog.exec())
    fileNames = dialog.selectedFiles();
//! [5]


//! [6]
dialog.setNameFilter("All C++ files (*.cpp *.cc *.C *.cxx *.c++)");
dialog.setNameFilter("*.cpp *.cc *.C *.cxx *.c++");
//! [6]


//! [7]
const QStringList filters({"Image files (*.png *.xpm *.jpg)",
                           "Text files (*.txt)",
                           "Any files (*)"
                          });
QFileDialog dialog(this);
dialog.setNameFilters(filters);
dialog.exec();
//! [7]


//! [8]
QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                "/home",
                                                tr("Images (*.png *.xpm *.jpg)"));
//! [8]


//! [9]
QStringList files = QFileDialog::getOpenFileNames(
                        this,
                        "Select one or more files to open",
                        "/home",
                        "Images (*.png *.xpm *.jpg)");
//! [9]


//! [11]
QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                           "/home/jana/untitled.png",
                           tr("Images (*.png *.xpm *.jpg)"));
//! [11]


//! [12]
QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                "/home",
                                                QFileDialog::ShowDirsOnly
                                                | QFileDialog::DontResolveSymlinks);
//! [12]

//! [13]
QStringList mimeTypeFilters({"image/jpeg", // will show "JPEG image (*.jpeg *.jpg *.jpe)
                             "image/png",  // will show "PNG image (*.png)"
                             "application/octet-stream" // will show "All files (*)"
                            });

QFileDialog dialog(this);
dialog.setMimeTypeFilters(mimeTypeFilters);
dialog.exec();
//! [13]

//! [14]
"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)"
//! [14]

//! [15]
auto fileContentReady = [](const QString &fileName, const QByteArray &fileContent) {
    if (fileName.isEmpty()) {
        // No file was selected
    } else {
        // Use fileName and fileContent
    }
};
QFileDialog::getOpenFileContent("Images (*.png *.xpm *.jpg)",  fileContentReady);
//! [15]

//! [16]
QByteArray imageData; // obtained from e.g. QImage::save()
QFileDialog::saveFileContent(imageData, "myimage.png"); // with filename hint
// OR
QFileDialog::saveFileContent(imageData); // no filename hint
//! [16]
