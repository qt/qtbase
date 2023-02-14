// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>

using namespace Qt::StringLiterals;

class tst_ContentUris: public QObject
{
    Q_OBJECT
private slots:
    void dirFacilities();
    void readWriteFile();
    void readWriteNonExistingFile_data();
    void readWriteNonExistingFile();
    void createFileFromDirUrl_data();
    void createFileFromDirUrl();
    void fileOperations();
};

static QStringList listFiles(const QDir &dir, QDirIterator::IteratorFlag flag = {})
{
    QDirIterator it(dir, flag);
    QStringList dirs;
    while (it.hasNext())
        dirs << it.next();
    return dirs;
}

void showInstructionsDialog(const QString &message)
{
    QMessageBox::information(nullptr, "Instructions", message);
}

void tst_ContentUris::dirFacilities()
{
    showInstructionsDialog("Choose a folder with no content/files/subdirs");

    auto url = QFileDialog::getExistingDirectory();
    QVERIFY(url.startsWith("content"_L1));
    QDir dir(url);

    QVERIFY(dir.exists());
    QVERIFY(!dir.dirName().isEmpty());
    QVERIFY(listFiles(dir).isEmpty());

    QVERIFY(dir.mkdir("Sub"));
    const auto dirList = listFiles(dir);
    QVERIFY(dirList.size() == 1);
    const QDir subDir = dirList.first();

    QVERIFY(subDir.dirName() == "Sub"_L1);
    qWarning() << "subDir.absolutePath()" << subDir.absolutePath() << dirList.first();
    QVERIFY(subDir.absolutePath() == dirList.first());
    QVERIFY(subDir.path() == dirList.first());

    QVERIFY(listFiles(dir, QDirIterator::Subdirectories).size() == 1);
    QVERIFY(dir.mkdir("Sub")); // Create an existing dir
    QVERIFY(dir.rmdir("Sub"));

    QVERIFY(dir.mkpath("Sub/Sub2/Sub3"));
    QVERIFY(listFiles(dir).size() == 1);
    QVERIFY(listFiles(dir, QDirIterator::Subdirectories).size() == 3);
    QVERIFY(dir.mkpath("Sub/Sub2/Sub3"));  // Create an existing dir hierarchy
    QVERIFY(dir.rmdir("Sub"));
}

void tst_ContentUris::readWriteFile()
{
    const QByteArray content = "Written to file";
    const QString fileName = "new_file.txt";

    {
        showInstructionsDialog("Choose a name for new file to create");

        auto url = QFileDialog::getSaveFileName(nullptr, tr("Save File"), fileName);
        QFile file(url);
        QVERIFY(file.exists());
        QVERIFY(file.size() == 0);
        QVERIFY(file.fileName() == url);
        QVERIFY(QFileInfo(url).fileName() == fileName);

        QVERIFY(file.open(QFile::WriteOnly));
        QVERIFY(file.isOpen());
        QVERIFY(file.isWritable());
        QVERIFY(file.fileTime(QFileDevice::FileModificationTime) != QDateTime());
        QVERIFY(file.write(content) > 0);
        QVERIFY(file.size() == content.size());
        file.close();

        // NOTE: The native file cursor is not returning an updated time or it takes long
        // for it to get updated, for now just check that we actually received a valid QDateTime
        QVERIFY(file.fileTime(QFileDevice::FileModificationTime) != QDateTime());
    }

    {
        showInstructionsDialog("Choose the file that was created");

        auto url = QFileDialog::getOpenFileName(nullptr, tr("Open File"), fileName);
        QFile file(url);
        QVERIFY(file.exists());

        QVERIFY(file.open(QFile::ReadOnly));
        QVERIFY(file.isOpen());
        QVERIFY(file.isReadable());
        QVERIFY(file.readAll() == content);

        QVERIFY(file.remove());
    }
}

void tst_ContentUris::readWriteNonExistingFile_data()
{
    QTest::addColumn<QString>("path");

    const QString fileName = "non-existing-file.txt";
    const QString uriSchemeAuthority = "content://com.android.externalstorage.documents";
    const QString id = "primary%3APictures";
    const QString encSlash = QUrl::toPercentEncoding("/"_L1);

    const QString docSlash = uriSchemeAuthority + "/document/"_L1 + id + "/"_L1 + fileName;
    const QString docEncodedSlash = uriSchemeAuthority + "/document/"_L1 + id + encSlash + fileName;

    QTest::newRow("document_with_slash") << docSlash;
    QTest::newRow("document_with_encoded_slash") << docEncodedSlash;
}

void tst_ContentUris::readWriteNonExistingFile()
{
    QFETCH(QString, path);

    QFile file(path);
    QVERIFY(!file.exists());
    QVERIFY(file.size() == 0);

    QVERIFY(!file.open(QFile::WriteOnly));
    QVERIFY(!file.isOpen());
    QVERIFY(!file.isWritable());
}

void tst_ContentUris::createFileFromDirUrl_data()
{
    QTest::addColumn<QString>("path");

    showInstructionsDialog("Choose a folder with no content/files/subdirs");

    const QString treeUrl = QFileDialog::getExistingDirectory();
    const QString fileName = "text.txt";
    const QString treeSlash = treeUrl + "/"_L1 + fileName;
    QTest::newRow("tree_with_slash") << treeSlash;

    // TODO: This is not handled at the moment
    // const QString encSlash = QUrl::toPercentEncoding("/"_L1);
    // const QString treeEncodedSlash = treeUrl + encSlash + fileName;
    // QTest::newRow("tree_with_encoded_slash") << treeEncodedSlash;
}

void tst_ContentUris::createFileFromDirUrl()
{
    QFETCH(QString, path);

    const QByteArray content = "Written to file";

    QFile file(path);
    QVERIFY(!file.exists());
    QVERIFY(file.size() == 0);

    QVERIFY(file.open(QFile::WriteOnly));
    QVERIFY(file.isOpen());
    QVERIFY(file.isWritable());
    QVERIFY(file.exists());
    QVERIFY(file.write(content));
    QVERIFY(file.size() == content.size());
    file.close();

    QVERIFY(file.open(QFile::ReadOnly));
    QVERIFY(file.isOpen());
    QVERIFY(file.isReadable());
    QVERIFY(file.readAll() == content);

    QVERIFY(file.remove());
}

void tst_ContentUris::fileOperations()
{
    showInstructionsDialog("Choose a name for new file to create");

    const QString fileName = "new_file.txt";
    auto url = QFileDialog::getSaveFileName(nullptr, tr("Save File"), fileName);
    QFile file(url);
    QVERIFY(file.exists());

    // Rename
    {
        const QString renamedFileName = "renamed_new_file.txt";
        QVERIFY(file.rename(renamedFileName));
        const auto renamedUrl = url.replace(fileName, renamedFileName);
        QVERIFY(file.fileName() == renamedUrl);

        // NOTE: The uri doesn't seem to stay usable after a rename and it needs to get
        // permission again via the SAF picker.
        showInstructionsDialog("Choose the file that was renamed");
        QFileDialog::getOpenFileName(nullptr, tr("Open File"));
        QVERIFY(file.exists());

        // rename now with full content uri
        const auto secondRenamedUrl = url.replace(renamedFileName, "second_nenamed_file.txt");
        QVERIFY(file.rename(secondRenamedUrl));
        QVERIFY(file.fileName() == secondRenamedUrl);

        // NOTE: The uri doesn't seem to stay usable after a rename and it needs to get
        // permission again via the SAF picker.
        showInstructionsDialog("Choose the file that was renamed");
        QFileDialog::getOpenFileName(nullptr, tr("Open File"));
        QVERIFY(file.exists());
    }

    // Remove
    QVERIFY(file.remove());
    QVERIFY(!file.exists());

    // Move
    {
        showInstructionsDialog("Choose source directory of file to move");
        const QString srcDir = QFileDialog::getExistingDirectory(nullptr, tr("Choose Directory"));

        const QString fileName = "file_to_move.txt"_L1;

        // Create a file
        QFile file(srcDir + u'/' + fileName);
        QVERIFY(file.open(QFile::WriteOnly));
        QVERIFY(file.exists());

        showInstructionsDialog("Choose target directory to where to move the file");
        const QString destDir = QFileDialog::getExistingDirectory(nullptr, tr("Choose Directory"));

        QVERIFY(file.rename(destDir  + u'/' + fileName));

        // NOTE: The uri doesn't seem to stay usable after a rename and it needs to get
        // permission again via the SAF picker.
        showInstructionsDialog("Choose the file that was moved");
        QFileDialog::getOpenFileName(nullptr, tr("Open File"));

        QVERIFY(file.remove());
    }
}

QTEST_MAIN(tst_ContentUris)
#include "tst_content_uris.moc"
