// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QSharedPointer>

using namespace Qt::StringLiterals;

class ContentUris: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void validateDirPath();
    void createFile();
    void removeFile();
    void renameFile();
    void moveFile();
    void fileInfoPath();
    void dirOperations();
    void listingFiles();

    void specialFileNames_data();
    void specialFileNames();

    void accessNonExistentFile();

    void filePickerSave();
    void filePickerOpen();

    void validateStaticFileUris_data();
    void validateStaticFileUris();
    void validateStaticDirUris_data();
    void validateStaticDirUris();

private:
    QString m_url;
};

const QByteArray content = "Written to file";
const QByteArray fileName = "text.txt";

void ContentUris::initTestCase()
{
    QMessageBox::information(nullptr, "Select a Directory",
                             "Create an new directory and grant permission for it from "
                             "the Android File Picker will be shown after this dialog...");
    m_url = QFileDialog::getExistingDirectory();
}

void ContentUris::validateDirPath()
{
    QDir rootDir(m_url);
    QVERIFY(rootDir.isReadable());

    QFileInfo info(m_url);
    QVERIFY(info.isDir());
    QVERIFY(!info.isFile());
    QVERIFY(info.isReadable());
}

void ContentUris::createFile()
{
    QFile file(m_url + "/"_L1 + fileName);
    QVERIFY(!file.exists());
    QCOMPARE(file.size(), 0);

    QVERIFY(file.open(QFile::WriteOnly));
    QVERIFY(file.isOpen());
    QVERIFY(file.isWritable());
    QCOMPARE_NE(file.fileTime(QFileDevice::FileModificationTime), QDateTime());
    QVERIFY(file.exists());
    QVERIFY(file.write(content));
    QCOMPARE(file.size(), content.size());
    file.close();

    // NOTE: The native file cursor is not returning an updated time or it takes long
    // for it to get updated, for now just check that we actually received a valid QDateTime
    QCOMPARE_NE(file.fileTime(QFileDevice::FileModificationTime), QDateTime());

    QVERIFY(file.open(QFile::ReadOnly));
    QVERIFY(file.isOpen());
    QVERIFY(file.isReadable());
    QCOMPARE(file.readAll(), content);
    file.close();

    QVERIFY(file.remove());
    QVERIFY(!file.exists());
}

void ContentUris::removeFile()
{
    QFile file(m_url + "/initial-file.txt");
    QVERIFY(file.open(QFile::WriteOnly));
    QVERIFY(file.exists());
    file.close();
    QVERIFY(file.remove());
    QVERIFY(!file.exists());
}

void ContentUris::renameFile()
{
    QFile file(m_url + u'/' + fileName);
    QVERIFY(file.open(QFile::WriteOnly));
    QVERIFY(file.exists());
    file.close();

    // rename with only file name
    QString newName = "renamed-"_L1 + fileName;
    QVERIFY(file.rename(newName));
    QCOMPARE(file.fileName(), QFile(m_url + u'/' + newName).fileName());
    QVERIFY(file.exists());

    // rename now with full content uri
    QFile doubleRenamedFile(m_url + "/double-" + newName);
    QVERIFY(file.rename(doubleRenamedFile.fileName()));
    QCOMPARE(file.fileName(), doubleRenamedFile.fileName());
    QVERIFY(file.exists());

    QVERIFY(file.remove());
    QVERIFY(!file.exists());
}

void ContentUris::moveFile()
{
    // Create a file
    QFile file(m_url + u'/' + fileName);
    QVERIFY(file.open(QFile::WriteOnly));
    file.close();
    QVERIFY(file.exists());

    const auto moveToDir = "movedir"_L1;
    QVERIFY(QDir(m_url).mkdir(moveToDir));
    QString destDir = m_url + u'/' + moveToDir;
    QFile destFile = QFile(destDir +  u'/' + fileName);
    QVERIFY(file.rename(destFile.fileName()));
    QCOMPARE(file.fileName(), destFile.fileName());
    QFileInfo info(file.fileName());
    QCOMPARE(info.fileName(), fileName);

    // move under new name
    QFile destFile2 = QFile(m_url + "/renamed-"_L1 + fileName);
    QVERIFY(file.rename(destFile2.fileName()));
    QCOMPARE(file.fileName(), destFile2.fileName());
    QCOMPARE(info.fileName(), fileName);
    info = QFileInfo(file.fileName());
    QCOMPARE(info.fileName(), "renamed-"_L1 + fileName);

    QVERIFY(file.remove());
    QVERIFY(!file.exists());

    QVERIFY(QDir(m_url).rmdir(moveToDir));
    QVERIFY(!QDir(destDir).exists());
}

void ContentUris::specialFileNames_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::newRow("spaces") << u"a file with spaces.txt"_s;
    QTest::newRow("special_chars") << u"file-with_special$chars!.log"_s;
    QTest::newRow("arabic") << u"ملف.txt"_s;
    QTest::newRow("japanese") << u"ファイル.txt"_s;
    QTest::newRow("chinese") << u"文件.txt"_s;
}

void ContentUris::specialFileNames()
{
    QFETCH(QString, fileName);

    QFile file(m_url + u'/' + fileName);
    QVERIFY(!file.exists());
    QCOMPARE(file.size(), 0);

    QVERIFY(file.open(QFile::WriteOnly));
    QVERIFY(file.isOpen());
    QVERIFY(file.isWritable());
    QVERIFY(file.write(content));
    QCOMPARE(file.size(), content.size());
    QCOMPARE(QFileInfo(file.fileName()).fileName(), fileName);
    file.close();

    QVERIFY(file.open(QFile::ReadOnly));
    QVERIFY(file.isOpen());
    QVERIFY(file.isReadable());
    QCOMPARE(file.readAll(), content);
    file.close();

    QVERIFY(file.rename(u'_' + fileName));
    QCOMPARE(QFileInfo(file.fileName()).fileName(), u'_' + fileName);

    QVERIFY(file.remove());
    QVERIFY(!file.exists());

    QDir rootDir(m_url);
    QVERIFY(rootDir.mkdir(fileName));
    QDir dir(m_url + u'/' + fileName);
    QVERIFY(rootDir.exists(fileName));
    QCOMPARE(dir.dirName(), fileName);

    QVERIFY(rootDir.rmdir(fileName));
    QVERIFY(!rootDir.exists(fileName));
}

void ContentUris::validateStaticFileUris_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("expectedFileName");
    QTest::addColumn<QString>("expectedFilePath");

    // External Storage Provider
    QTest::newRow("file_in_tree_implicit")
        << "content://com.android.externalstorage.documents/tree/primary%3AMyDir/a_file.txt"
        << "a_file.txt"
        << "content://com.android.externalstorage.documents/tree/primary%3AMyDir/document/primary%3AMyDir%2Fa_file.txt";

    QTest::newRow("document_with_encoded_path")
        << "content://com.android.externalstorage.documents/document/primary%3AMyDir%2Ffile.txt"
        << "file.txt"
        << "content://com.android.externalstorage.documents/document/primary%3AMyDir%2Ffile.txt";

    QTest::newRow("mixed_tree_and_document")
        << "content://com.android.externalstorage.documents/tree/primary%3AMyDir/document/primary%3Afile.txt"
        << "file.txt"
        << "content://com.android.externalstorage.documents/tree/primary%3AMyDir/document/primary%3Afile.txt";

    QTest::newRow("mixed_tree_and_document_long")
        << "content://com.android.externalstorage.documents/tree/primary%3AMyDir/document/primary%3AMyDir%2Ffile.txt"
        << "file.txt"
        << "content://com.android.externalstorage.documents/tree/primary%3AMyDir/document/primary%3AMyDir%2Ffile.txt";

    // Downloads Provider
    QTest::newRow("downloads_provider_doc")
        << "content://com.android.providers.downloads.documents/document/1234"
        << "1234"
        << "content://com.android.providers.downloads.documents/document/1234";

    // MediaStore Provider
    QTest::newRow("mediastore_provider_doc")
        << "content://media/external/images/media/5678"
        << "5678"
        << "content://media/external/images/media/5678";

    // Hypothetical Cloud Provider
    QTest::newRow("cloud_provider_doc")
        << "content://com.example.cloudstorage.documents/document/some_file_id"
        << "some_file_id"
        << "content://com.example.cloudstorage.documents/document/some_file_id";

    QTest::newRow("cloud_provider_nested_doc")
        << "content://com.example.mycloudstorage.documents/tree/user_id%3Amy_docs/document/user_id%3Amy_docs%2Freport.docx"
        << "report.docx"
        << "content://com.example.mycloudstorage.documents/tree/user_id%3Amy_docs/document/user_id%3Amy_docs%2Freport.docx";

    // Contacts Provider
    QTest::newRow("contacts_people_item")
        << "content://com.contacts/people/123"
        << "123"
        << "content://com.contacts/people/123";
}

void ContentUris::validateStaticFileUris()
{
    QFETCH(QString, uri);
    QFETCH(QString, expectedFileName);
    QFETCH(QString, expectedFilePath);

    QFile file(uri);
    QCOMPARE(file.fileName(), expectedFilePath);

    QFileInfo fileInfo(uri);
    QCOMPARE(fileInfo.filePath(), uri);
    QCOMPARE(fileInfo.absoluteFilePath(), expectedFilePath);
    QCOMPARE(fileInfo.fileName(), expectedFileName);
}


void ContentUris::validateStaticDirUris_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("expectedDirName");

    // External Storage Provider
    QTest::newRow("tree_root")
        << "content://com.android.externalstorage.documents/tree/primary%3AMyDir"
        << "MyDir";

    QTest::newRow("children_uri")
        << "content://com.android.externalstorage.documents/tree/primary%3AMyProject/document/primary%3AMyProject/children"
        << "MyProject";

    QTest::newRow("base_provider_uri")
        << "content://com.android.externalstorage.documents/"
        << "";

    QTest::newRow("tree_with_device_id")
        << "content://com.android.externalstorage.documents/tree/ABCD-EFGH%3ADownloads"
        << "Downloads";

    // Hypothetical Cloud Provider
    QTest::newRow("cloud_provider_tree")
        << "content://com.example.cloudstorage.documents/tree/user_id%3Amy_docs"
        << "my_docs";

    // Contacts Provider (not file-system, but should be handled gracefully)
    QTest::newRow("contacts_people_base")
        << "content://com.contacts/people"
        << "people";

    QTest::newRow("contacts_data_base")
        << "content://com.contacts/data"
        << "data";
}

void ContentUris::validateStaticDirUris()
{
    QFETCH(QString, uri);
    QFETCH(QString, expectedDirName);

    QDir dir(uri);
    QCOMPARE(dir.dirName(), expectedDirName);
    // NOTE: since QDir APIs don't necessarily return a path with scheme
    // we can't really rely on them returning us a valid content uri.
}

void ContentUris::fileInfoPath()
{
    QFileInfo info(m_url + u'/' + fileName);
    QCOMPARE(info.fileName(), fileName);

    QFile file(info.absoluteFilePath());
    QCOMPARE(file.fileName(), info.absoluteFilePath());
    QVERIFY(!file.exists());
    QCOMPARE(file.size(), 0);
    QVERIFY(file.open(QFile::WriteOnly));
    QVERIFY(file.isOpen());
    QVERIFY(file.isWritable());
    QVERIFY(file.exists());
    QVERIFY(file.write(content));
    QCOMPARE(file.size(), content.size());
    QCOMPARE(info.size(), content.size());
    file.close();

    QVERIFY(file.remove());
    QVERIFY(!file.exists());
}

void ContentUris::accessNonExistentFile()
{
    const QString uri =
            "content://com.android.externalstorage.documents/document/primary%3APictures/non-existing-file.txt";

    QFile file(uri);
    QVERIFY(!file.exists());
    QCOMPARE(file.size(), 0);

    QVERIFY(!file.open(QFile::WriteOnly));
    QVERIFY(!file.isOpen());
    QVERIFY(!file.isWritable());
    file.close();

    QVERIFY(!file.open(QFile::ReadOnly));
    QVERIFY(!file.isOpen());
    QVERIFY(!file.isReadable());
    file.close();
}

void ContentUris::dirOperations()
{
    QVERIFY(m_url.startsWith("content"_L1));
    QDir dir(m_url);

    QVERIFY(dir.exists());
    QVERIFY(!dir.dirName().isEmpty());

    QVERIFY(dir.mkdir("Sub"));
    QVERIFY(dir.exists());

    QFileInfo info(m_url);
    QVERIFY(info.isDir());
    QVERIFY(info.exists());

    const QString subDirPath = m_url + "/Sub";
    QDir subDir(subDirPath);
    QCOMPARE(subDir.dirName(), "Sub");

    QFileInfo subDirInfo(subDirPath);
    QVERIFY(subDirInfo.isDir());
    QVERIFY(subDirInfo.exists());
    QCOMPARE(subDirInfo.completeBaseName(), "Sub");

    QVERIFY(dir.mkdir("Sub")); // Create an existing dir
    QVERIFY(dir.rmdir("Sub"));

    QVERIFY(dir.mkpath("Sub/Sub2/Sub3"));
    QVERIFY(QDir(m_url + "/Sub/Sub2").exists());
    QVERIFY(QDir(m_url + "/Sub/Sub2/Sub3").exists());

    QVERIFY(dir.mkpath("Sub/Sub2/Sub3"));  // Create an existing dir hierarchy
    QVERIFY(dir.rmdir("Sub"));
}

void ContentUris::listingFiles()
{
    auto listDirDocuments = [](const QDir &dir, QDirIterator::IteratorFlag flag = {}) {
        QDirIterator it(dir, flag);
        QStringList docs;
        while (it.hasNext())
            docs << it.next();
        return docs;
    };

    QDir dir(m_url);

    QVERIFY(dir.exists());
    QVERIFY(!dir.dirName().isEmpty());
    QVERIFY(listDirDocuments(dir).isEmpty());

    QVERIFY(dir.mkdir("Sub"));
    const auto dirList = listDirDocuments(dir);
    QCOMPARE(dirList.size(), 1);
    const QDir subDir = dirList.first();

    QCOMPARE(subDir.dirName(), "Sub"_L1);
    QCOMPARE(subDir.absolutePath(), dirList.first());
    QCOMPARE(subDir.path(), dirList.first());

    QCOMPARE(listDirDocuments(dir, QDirIterator::Subdirectories).size(), 1);
    QVERIFY(dir.rmdir("Sub"));
    QCOMPARE(listDirDocuments(dir, QDirIterator::Subdirectories).size(), 0);

    QVERIFY(dir.mkpath("Sub/Sub2/Sub3"));
    QCOMPARE(listDirDocuments(dir).size(), 1);
    QCOMPARE(listDirDocuments(dir, QDirIterator::Subdirectories).size(), 3);

    QList<QSharedPointer<QFile>> files;
    auto createFiles = [&files](const QString &path, int size) {
        for (int i = 0; i < size; ++i) {
            auto file = QSharedPointer<QFile>::create(path + "/file" + QString::number(i));
            QVERIFY(file->open(QFile::WriteOnly));
            QVERIFY(file->write(content));
            files.append(file);
        }
    };

    createFiles(m_url, 2);
    QCOMPARE(listDirDocuments(dir).size(), 3);
    QCOMPARE(listDirDocuments(dir, QDirIterator::Subdirectories).size(), 5);

    createFiles(m_url + "/Sub", 2);
    QCOMPARE(listDirDocuments(dir).size(), 3);
    QCOMPARE(listDirDocuments(dir, QDirIterator::Subdirectories).size(), 7);

    for (const QSharedPointer<QFile> &file : files)
        QVERIFY(file->remove());

    QVERIFY(dir.rmdir("Sub"));
}

void ContentUris::filePickerSave()
{
    QMessageBox::information(nullptr, "Saving a File", "Agree to create a new file...");
    auto url = QFileDialog::getSaveFileName(nullptr, tr("Save File"), fileName);

    QFile file(url);
    QVERIFY(file.exists());
    QCOMPARE(file.size(), 0);
    QCOMPARE(file.fileName(), url);
    QCOMPARE(QFileInfo(url).fileName(), fileName);

    QVERIFY(file.open(QFile::WriteOnly));
    QVERIFY(file.isOpen());
    QVERIFY(file.isWritable());
    QCOMPARE_NE(file.fileTime(QFileDevice::FileModificationTime), QDateTime());
    QCOMPARE_GT(file.write(content), 0);
    QCOMPARE(file.size(), content.size());
    file.close();

}

void ContentUris::filePickerOpen()
{
    QMessageBox::information(nullptr, "Reading a File", "Select the file that you just saved...");
    auto url = QFileDialog::getOpenFileName(nullptr, tr("Open File"));

    QFile file(url);
    QVERIFY(file.exists());

    QVERIFY(file.open(QFile::ReadOnly));
    QVERIFY(file.isOpen());
    QVERIFY(file.isReadable());
    QCOMPARE(file.readAll(), content);
    file.close();

    QVERIFY(file.remove());
    QVERIFY(!file.exists());
}

QTEST_MAIN(ContentUris)
#include "contenturis.moc"
