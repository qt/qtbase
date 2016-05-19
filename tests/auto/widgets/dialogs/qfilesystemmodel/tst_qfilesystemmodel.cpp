/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#ifdef QT_BUILD_INTERNAL
#include <private/qfilesystemmodel_p.h>
#endif
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QTreeView>
#include <QHeaderView>
#include <QStandardPaths>
#include <QTime>
#include <QStyle>
#include <QtGlobal>
#include <QTemporaryDir>
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
# include <qt_windows.h> // for SetFileAttributes
#endif

#include <algorithm>

#define WAITTIME 1000

// Will try to wait for the condition while allowing event processing
// for a maximum of 5 seconds.
#define TRY_WAIT(expr) \
    do { \
        const int step = 50; \
        for (int __i = 0; __i < 5000 && !(expr); __i+=step) { \
            QTest::qWait(step); \
        } \
    } while(0)

class tst_QFileSystemModel : public QObject {
  Q_OBJECT

public:
    tst_QFileSystemModel();

public Q_SLOTS:
    void init();
    void cleanup();

private slots:
    void initTestCase();
    void indexPath();

    void rootPath();
    void readOnly();
    void iconProvider();

    void rowCount();

    void rowsInserted_data();
    void rowsInserted();

    void rowsRemoved_data();
    void rowsRemoved();

    void dataChanged_data();
    void dataChanged();

    void filters_data();
    void filters();

    void nameFilters();

    void setData_data();
    void setData();

    void sortPersistentIndex();
    void sort_data();
    void sort();

    void mkdir();
    void deleteFile();

    void caseSensitivity();

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    void Win32LongFileName();
#endif

    void drives_data();
    void drives();
    void dirsBeforeFiles();

    void roleNames_data();
    void roleNames();

    void permissions_data();
    void permissions();

    void doNotUnwatchOnFailedRmdir();
    void specialFiles();

    void fileInfo();

protected:
    bool createFiles(const QString &test_path, const QStringList &initial_files, int existingFileCount = 0, const QStringList &intial_dirs = QStringList());

private:
    QFileSystemModel *model;
    QString flatDirTestPath;
    QTemporaryDir m_tempDir;
};

tst_QFileSystemModel::tst_QFileSystemModel() : model(0)
{
}

void tst_QFileSystemModel::init()
{
    cleanup();
    QCOMPARE(model, (QFileSystemModel*)0);
    model = new QFileSystemModel;
}

void tst_QFileSystemModel::cleanup()
{
    delete model;
    model = 0;
    QString tmp = flatDirTestPath;
    QDir dir(tmp);
    if (dir.exists(tmp)) {
        QStringList list = dir.entryList(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
        for (int i = 0; i < list.count(); ++i) {
            QFileInfo fi(dir.path() + '/' + list.at(i));
            if (fi.exists() && fi.isFile()) {
                QFile p(fi.absoluteFilePath());
                p.setPermissions(QFile::ReadUser | QFile::ReadOwner | QFile::ExeOwner | QFile::ExeUser | QFile::WriteUser | QFile::WriteOwner | QFile::WriteOther);
                QFile dead(dir.path() + '/' + list.at(i));
                dead.remove();
            }
            if (fi.exists() && fi.isDir())
                QVERIFY(dir.rmdir(list.at(i)));
        }
        list = dir.entryList(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot);
        QCOMPARE(list.count(), 0);
    }
}

void tst_QFileSystemModel::initTestCase()
{
    QVERIFY2(m_tempDir.isValid(), qPrintable(m_tempDir.errorString()));
    flatDirTestPath = m_tempDir.path();
}

void tst_QFileSystemModel::indexPath()
{
#if !defined(Q_OS_WIN)
    int depth = QDir::currentPath().count('/');
    model->setRootPath(QDir::currentPath());
    QTest::qWait(WAITTIME);
    QString backPath;
    for (int i = 0; i <= depth * 2 + 1; ++i) {
        backPath += "../";
        QModelIndex idx = model->index(backPath);
        QVERIFY(i != depth - 1 ? idx.isValid() : !idx.isValid());
    }
    QTest::qWait(WAITTIME * 3);
    qApp->processEvents();
#endif
}

void tst_QFileSystemModel::rootPath()
{
    QCOMPARE(model->rootPath(), QString(QDir().path()));

    QSignalSpy rootChanged(model, SIGNAL(rootPathChanged(QString)));
    QModelIndex root = model->setRootPath(model->rootPath());
    root = model->setRootPath("this directory shouldn't exist");
    QCOMPARE(rootChanged.count(), 0);

    QString oldRootPath = model->rootPath();
    const QStringList documentPaths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    QVERIFY(!documentPaths.isEmpty());
    QString documentPath = documentPaths.front();
    // In particular on Linux, ~/Documents (the first
    // DocumentsLocation) may not exist, so choose ~ in that case:
    if (!QFile::exists(documentPath)) {
        documentPath = QDir::homePath();
        qWarning("%s: first documentPath \"%s\" does not exist. Using ~ (\"%s\") instead.",
                 Q_FUNC_INFO, qPrintable(documentPaths.front()), qPrintable(documentPath));
    }
    root = model->setRootPath(documentPath);

    QTRY_VERIFY(model->rowCount(root) >= 0);
    QCOMPARE(model->rootPath(), QString(documentPath));
    QCOMPARE(rootChanged.count(), oldRootPath == model->rootPath() ? 0 : 1);
    QCOMPARE(model->rootDirectory().absolutePath(), documentPath);

    model->setRootPath(QDir::rootPath());
    int oldCount = rootChanged.count();
    oldRootPath = model->rootPath();
    root = model->setRootPath(documentPath + QLatin1String("/."));
    QTRY_VERIFY(model->rowCount(root) >= 0);
    QCOMPARE(model->rootPath(), documentPath);
    QCOMPARE(rootChanged.count(), oldRootPath == model->rootPath() ? oldCount : oldCount + 1);
    QCOMPARE(model->rootDirectory().absolutePath(), documentPath);

    QDir newdir = documentPath;
    if (newdir.cdUp()) {
        oldCount = rootChanged.count();
        oldRootPath = model->rootPath();
        root = model->setRootPath(documentPath + QLatin1String("/.."));
        QTRY_VERIFY(model->rowCount(root) >= 0);
        QCOMPARE(model->rootPath(), newdir.path());
        QCOMPARE(rootChanged.count(), oldCount + 1);
        QCOMPARE(model->rootDirectory().absolutePath(), newdir.path());
    }
}

void tst_QFileSystemModel::readOnly()
{
    QCOMPARE(model->isReadOnly(), true);
    QTemporaryFile file(flatDirTestPath + QStringLiteral("/XXXXXX.dat"));
    QVERIFY2(file.open(), qPrintable(file.errorString()));
    QModelIndex root = model->setRootPath(flatDirTestPath);

    QTRY_VERIFY(model->rowCount(root) > 0);
    QVERIFY(!(model->flags(model->index(file.fileName())) & Qt::ItemIsEditable));
    model->setReadOnly(false);
    QCOMPARE(model->isReadOnly(), false);
    QVERIFY(model->flags(model->index(file.fileName())) & Qt::ItemIsEditable);
}

class CustomFileIconProvider : public QFileIconProvider
{
public:
    CustomFileIconProvider() : QFileIconProvider() {
        mb = qApp->style()->standardIcon(QStyle::SP_MessageBoxCritical);
        dvd = qApp->style()->standardIcon(QStyle::SP_DriveDVDIcon);
    }

    virtual QIcon icon(const QFileInfo &info) const
    {
        if (info.isDir())
            return mb;

        return QFileIconProvider::icon(info);
    }
    virtual QIcon icon(IconType type) const
    {
        if (type == QFileIconProvider::Folder)
            return dvd;

        return QFileIconProvider::icon(type);
    }
private:
    QIcon mb;
    QIcon dvd;
};

void tst_QFileSystemModel::iconProvider()
{
    QVERIFY(model->iconProvider());
    QFileIconProvider *p = new QFileIconProvider();
    model->setIconProvider(p);
    QCOMPARE(model->iconProvider(), p);
    model->setIconProvider(0);
    delete p;

    QFileSystemModel *myModel = new QFileSystemModel();
    const QStringList documentPaths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    QVERIFY(!documentPaths.isEmpty());
    const QString documentPath = documentPaths.front();
    myModel->setRootPath(documentPath);
    //Let's wait to populate the model
    QTest::qWait(250);
    //We change the provider, icons must me updated
    CustomFileIconProvider *custom = new CustomFileIconProvider();
    myModel->setIconProvider(custom);

    QPixmap mb = qApp->style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(50, 50);
    QCOMPARE(myModel->fileIcon(myModel->index(QDir::homePath())).pixmap(50, 50), mb);
    delete myModel;
    delete custom;
}

bool tst_QFileSystemModel::createFiles(const QString &test_path, const QStringList &initial_files, int existingFileCount, const QStringList &initial_dirs)
{
    //qDebug() << (model->rowCount(model->index(test_path))) << existingFileCount << initial_files;
    TRY_WAIT((model->rowCount(model->index(test_path)) == existingFileCount));
    for (int i = 0; i < initial_dirs.count(); ++i) {
        QDir dir(test_path);
        if (!dir.exists()) {
            qWarning() << "error" << test_path << "doesn't exists";
            return false;
        }
        if(!dir.mkdir(initial_dirs.at(i))) {
            qWarning() << "error" << "failed to make" << initial_dirs.at(i);
            return false;
        }
        //qDebug() << test_path + '/' + initial_dirs.at(i) << (QFile::exists(test_path + '/' + initial_dirs.at(i)));
    }
    for (int i = 0; i < initial_files.count(); ++i) {
        QFile file(test_path + '/' + initial_files.at(i));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            qDebug() << "failed to open file" << initial_files.at(i);
            return false;
        }
        if (!file.resize(1024 + file.size())) {
            qDebug() << "failed to resize file" << initial_files.at(i);
            return false;
        }
        if (!file.flush()) {
            qDebug() << "failed to flush file" << initial_files.at(i);
            return false;
        }
        file.close();
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
        if (initial_files.at(i)[0] == '.') {
            QString hiddenFile = QDir::toNativeSeparators(file.fileName());
            wchar_t nativeHiddenFile[MAX_PATH];
            memset(nativeHiddenFile, 0, sizeof(nativeHiddenFile));
            hiddenFile.toWCharArray(nativeHiddenFile);
#ifndef Q_OS_WINRT
            DWORD currentAttributes = ::GetFileAttributes(nativeHiddenFile);
#else // !Q_OS_WINRT
            WIN32_FILE_ATTRIBUTE_DATA attributeData;
            if (!::GetFileAttributesEx(nativeHiddenFile, GetFileExInfoStandard, &attributeData))
                attributeData.dwFileAttributes = 0xFFFFFFFF;
            DWORD currentAttributes = attributeData.dwFileAttributes;
#endif // Q_OS_WINRT
            if (currentAttributes == 0xFFFFFFFF) {
                qErrnoWarning("failed to get file attributes: %s", qPrintable(hiddenFile));
                return false;
            }
            if (!::SetFileAttributes(nativeHiddenFile, currentAttributes | FILE_ATTRIBUTE_HIDDEN)) {
                qErrnoWarning("failed to set file hidden: %s", qPrintable(hiddenFile));
                return false;
            }
        }
#endif
        //qDebug() << test_path + '/' + initial_files.at(i) << (QFile::exists(test_path + '/' + initial_files.at(i)));
    }
    return true;
}

void tst_QFileSystemModel::rowCount()
{
    QString tmp = flatDirTestPath;
    QVERIFY(createFiles(tmp, QStringList()));

    QSignalSpy spy2(model, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy spy3(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));

#if !defined(Q_OS_WINCE)
    QStringList files = QStringList() <<  "b" << "d" << "f" << "h" << "j" << ".a" << ".c" << ".e" << ".g";
    QString l = "b,d,f,h,j,.a,.c,.e,.g";
#else
    // Cannot hide them on CE
    QStringList files = QStringList() <<  "b" << "d" << "f" << "h" << "j";
    QString l = "b,d,f,h,j";
#endif
    QVERIFY(createFiles(tmp, files));

    QModelIndex root = model->setRootPath(tmp);
    QTRY_COMPARE(model->rowCount(root), 5);
    QVERIFY(spy2.count() > 0);
    QVERIFY(spy3.count() > 0);
}

void tst_QFileSystemModel::rowsInserted_data()
{
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("ascending");
    for (int i = 0; i < 4; ++i) {
        const QByteArray iB = QByteArray::number(i);
        QTest::newRow(("Qt::AscendingOrder " + iB).constData()) << i << (int)Qt::AscendingOrder;
        QTest::newRow(("Qt::DescendingOrder " + iB).constData()) << i << (int)Qt::DescendingOrder;
    }
}

static inline QString lastEntry(const QModelIndex &root)
{
    const QAbstractItemModel *model = root.model();
    return model->index(model->rowCount(root) - 1, 0, root).data().toString();
}

void tst_QFileSystemModel::rowsInserted()
{
#if defined(Q_OS_WINCE)
    QSKIP("Watching directories does not work on CE(see #137910)");
#endif
    QString tmp = flatDirTestPath;
    rowCount();
    QModelIndex root = model->index(model->rootPath());

    QFETCH(int, ascending);
    QFETCH(int, count);
    model->sort(0, (Qt::SortOrder)ascending);

    QSignalSpy spy0(model, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy spy1(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    int oldCount = model->rowCount(root);
    QStringList files;
    for (int i = 0; i < count; ++i)
        files.append(QLatin1Char('c') + QString::number(i));
    QVERIFY(createFiles(tmp, files, 5));
    TRY_WAIT(model->rowCount(root) == oldCount + count);
    QTRY_COMPARE(model->rowCount(root), oldCount + count);
    int totalRowsInserted = 0;
    for (int i = 0; i < spy0.count(); ++i) {
        int start = spy0[i].value(1).toInt();
        int end = spy0[i].value(2).toInt();
        totalRowsInserted += end - start + 1;
    }
    QCOMPARE(totalRowsInserted, count);
    const QString expected = ascending == Qt::AscendingOrder ? QStringLiteral("j") : QStringLiteral("b");
    QTRY_COMPARE(lastEntry(root), expected);

    if (spy0.count() > 0) {
        if (count == 0)
            QCOMPARE(spy0.count(), 0);
        else
            QVERIFY(spy0.count() >= 1);
    }
    if (count == 0) QCOMPARE(spy1.count(), 0); else QVERIFY(spy1.count() >= 1);

    QVERIFY(createFiles(tmp, QStringList(".hidden_file"), 5 + count));

    if (count != 0)
        QTRY_VERIFY(spy0.count() >= 1);
    else
        QTRY_COMPARE(spy0.count(), 0);
    if (count != 0)
        QTRY_VERIFY(spy1.count() >= 1);
    else
        QTRY_COMPARE(spy1.count(), 0);
}

void tst_QFileSystemModel::rowsRemoved_data()
{
    rowsInserted_data();
}

void tst_QFileSystemModel::rowsRemoved()
{
#if defined(Q_OS_WINCE)
    QSKIP("Watching directories does not work on CE(see #137910)");
#endif
    QString tmp = flatDirTestPath;
    rowCount();
    QModelIndex root = model->index(model->rootPath());

    QFETCH(int, count);
    QFETCH(int, ascending);
    model->sort(0, (Qt::SortOrder)ascending);
    QTest::qWait(WAITTIME);

    QSignalSpy spy0(model, SIGNAL(rowsRemoved(QModelIndex,int,int)));
    QSignalSpy spy1(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    int oldCount = model->rowCount(root);
    for (int i = count - 1; i >= 0; --i) {
        //qDebug() << "removing" <<  model->index(i, 0, root).data().toString();
        QVERIFY(QFile::remove(tmp + '/' + model->index(i, 0, root).data().toString()));
    }
    for (int i = 0 ; i < 10; ++i) {
        QTest::qWait(WAITTIME);
        qApp->processEvents();
        if (count != 0) {
            if (i == 10 || spy0.count() != 0) {
                QVERIFY(spy0.count() >= 1);
                QVERIFY(spy1.count() >= 1);
            }
        } else {
            if (i == 10 || spy0.count() == 0) {
                QCOMPARE(spy0.count(), 0);
                QCOMPARE(spy1.count(), 0);
            }
        }
        QStringList lst;
        for (int i = 0; i < model->rowCount(root); ++i)
            lst.append(model->index(i, 0, root).data().toString());
        if (model->rowCount(root) == oldCount - count)
            break;
        qDebug() << "still have:" << lst << QFile::exists(tmp + '/' + QString(".a"));
        QDir tmpLister(tmp);
        qDebug() << tmpLister.entryList();
    }
    QTRY_COMPARE(model->rowCount(root), oldCount - count);

    QVERIFY(QFile::exists(tmp + '/' + QString(".a")));
    QVERIFY(QFile::remove(tmp + '/' + QString(".a")));
    QVERIFY(QFile::remove(tmp + '/' + QString(".c")));
    QTest::qWait(WAITTIME);

    if (count != 0) QVERIFY(spy0.count() >= 1); else QCOMPARE(spy0.count(), 0);
    if (count != 0) QVERIFY(spy1.count() >= 1); else QCOMPARE(spy1.count(), 0);
}

void tst_QFileSystemModel::dataChanged_data()
{
    rowsInserted_data();
}

void tst_QFileSystemModel::dataChanged()
{
    // This can't be tested right now sense we don't watch files, only directories
    return;

    /*
    QString tmp = flatDirTestPath;
    rowCount();
    QModelIndex root = model->index(model->rootPath());

    QFETCH(int, count);
    QFETCH(int, assending);
    model->sort(0, (Qt::SortOrder)assending);

    QSignalSpy spy(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)));
    QStringList files;
    for (int i = 0; i < count; ++i)
        files.append(model->index(i, 0, root).data().toString());
    createFiles(tmp, files);

    QTest::qWait(WAITTIME);

    if (count != 0) QVERIFY(spy.count() >= 1); else QCOMPARE(spy.count(), 0);
    */
}

void tst_QFileSystemModel::filters_data()
{
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QStringList>("dirs");
    QTest::addColumn<int>("dirFilters");
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<int>("rowCount");
#if !defined(Q_OS_WINCE)
    QTest::newRow("no dirs") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs) << QStringList() << 2;
    QTest::newRow("no dirs - dot") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs | QDir::NoDot) << QStringList() << 1;
    QTest::newRow("no dirs - dotdot") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs | QDir::NoDotDot) << QStringList() << 1;
    QTest::newRow("no dirs - dotanddotdot") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs | QDir::NoDotAndDotDot) << QStringList() << 0;
    QTest::newRow("one dir - dot") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs | QDir::NoDot) << QStringList() << 2;
    QTest::newRow("one dir - dotdot") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs | QDir::NoDotDot) << QStringList() << 2;
    QTest::newRow("one dir - dotanddotdot") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs | QDir::NoDotAndDotDot) << QStringList() << 1;
    QTest::newRow("one dir") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs) << QStringList() << 3;
    QTest::newRow("no dir + hidden") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs | QDir::Hidden) << QStringList() << 2;
    QTest::newRow("dir+hid+files") << (QStringList() << "a" << "b" << "c") << QStringList() <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden) << QStringList() << 5;
    QTest::newRow("dir+file+hid-dot .A") << (QStringList() << "a" << "b" << "c") << (QStringList() << ".A") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot) << QStringList() << 4;
    QTest::newRow("dir+files+hid+dot A") << (QStringList() << "a" << "b" << "c") << (QStringList() << "AFolder") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot) << (QStringList() << "A*") << 2;
    QTest::newRow("dir+files+hid+dot+cas1") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive) << (QStringList() << "Z") << 1;
    QTest::newRow("dir+files+hid+dot+cas2") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive) << (QStringList() << "a") << 1;
    QTest::newRow("dir+files+hid+dot+cas+alldir") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive | QDir::AllDirs) << (QStringList() << "Z") << 1;
#else
    QTest::qWait(3000); // We need to calm down a bit...
    QTest::newRow("no dirs") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs) << QStringList() << 0;
    QTest::newRow("no dirs - dot") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs | QDir::NoDot) << QStringList() << 1;
    QTest::newRow("no dirs - dotdot") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs | QDir::NoDotDot) << QStringList() << 1;
    QTest::newRow("no dirs - dotanddotdot") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs | QDir::NoDotAndDotDot) << QStringList() << 0;
    QTest::newRow("one dir - dot") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs | QDir::NoDot) << QStringList() << 2;
    QTest::newRow("one dir - dotdot") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs | QDir::NoDotDot) << QStringList() << 2;
    QTest::newRow("one dir - dotanddotdot") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs | QDir::NoDotAndDotDot) << QStringList() << 1;
    QTest::newRow("one dir") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") << (int)(QDir::Dirs) << QStringList() << 1;
    QTest::newRow("no dir + hidden") << (QStringList() << "a" << "b" << "c") << QStringList() << (int)(QDir::Dirs | QDir::Hidden) << QStringList() << 0;
    QTest::newRow("dir+hid+files") << (QStringList() << "a" << "b" << "c") << QStringList() <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden) << QStringList() << 3;
    QTest::newRow("dir+file+hid-dot .A") << (QStringList() << "a" << "b" << "c") << (QStringList() << ".A") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot) << QStringList() << 4;
    QTest::newRow("dir+files+hid+dot A") << (QStringList() << "a" << "b" << "c") << (QStringList() << "AFolder") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot) << (QStringList() << "A*") << 2;
    QTest::newRow("dir+files+hid+dot+cas1") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive) << (QStringList() << "Z") << 1;
    QTest::newRow("dir+files+hid+dot+cas2") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive) << (QStringList() << "a") << 1;
    QTest::newRow("dir+files+hid+dot+cas+alldir") << (QStringList() << "a" << "b" << "c") << (QStringList() << "Z") <<
                         (int)(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive | QDir::AllDirs) << (QStringList() << "Z") << 1;
#endif

    QTest::newRow("case sensitive") << (QStringList() << "Antiguagdb" << "Antiguamtd"
        << "Antiguamtp" << "afghanistangdb" << "afghanistanmtd")
        << QStringList() << (int)(QDir::Files) << QStringList() << 5;
}

void tst_QFileSystemModel::filters()
{
    QString tmp = flatDirTestPath;
    QVERIFY(createFiles(tmp, QStringList()));
    QModelIndex root = model->setRootPath(tmp);
    QFETCH(QStringList, files);
    QFETCH(QStringList, dirs);
    QFETCH(int, dirFilters);
    QFETCH(QStringList, nameFilters);
    QFETCH(int, rowCount);

    if (nameFilters.count() > 0)
        model->setNameFilters(nameFilters);
    model->setNameFilterDisables(false);
    model->setFilter((QDir::Filters)dirFilters);

    QVERIFY(createFiles(tmp, files, 0, dirs));
    QTRY_COMPARE(model->rowCount(root), rowCount);

    // Make sure that we do what QDir does
    QDir xFactor(tmp);
    QDir::Filters  filters = (QDir::Filters)dirFilters;
    QStringList dirEntries;

    if (nameFilters.count() > 0)
        dirEntries = xFactor.entryList(nameFilters, filters);
    else
        dirEntries = xFactor.entryList(filters);

    QCOMPARE(dirEntries.count(), rowCount);

    QStringList modelEntries;

    for (int i = 0; i < rowCount; ++i)
        modelEntries.append(model->data(model->index(i, 0, root), QFileSystemModel::FileNameRole).toString());

    std::sort(dirEntries.begin(), dirEntries.end());
    std::sort(modelEntries.begin(), modelEntries.end());
    QCOMPARE(dirEntries, modelEntries);

#ifdef Q_OS_LINUX
    if (files.count() >= 3 && rowCount >= 3 && rowCount != 5) {
        QString fileName1 = (tmp + '/' + files.at(0));
        QString fileName2 = (tmp + '/' + files.at(1));
        QString fileName3 = (tmp + '/' + files.at(2));
        QFile::Permissions originalPermissions = QFile::permissions(fileName1);
        QVERIFY(QFile::setPermissions(fileName1, QFile::WriteOwner));
        QVERIFY(QFile::setPermissions(fileName2, QFile::ReadOwner));
        QVERIFY(QFile::setPermissions(fileName3, QFile::ExeOwner));

        model->setFilter((QDir::Files | QDir::Readable));
        QTRY_COMPARE(model->rowCount(root), 1);

        model->setFilter((QDir::Files | QDir::Writable));
        QTRY_COMPARE(model->rowCount(root), 1);

        model->setFilter((QDir::Files | QDir::Executable));
        QTRY_COMPARE(model->rowCount(root), 1);

        // reset permissions
        QVERIFY(QFile::setPermissions(fileName1, originalPermissions));
        QVERIFY(QFile::setPermissions(fileName2, originalPermissions));
        QVERIFY(QFile::setPermissions(fileName3, originalPermissions));
    }
#endif
}

void tst_QFileSystemModel::nameFilters()
{
    QStringList list;
    list << "a" << "b" << "c";
    model->setNameFilters(list);
    model->setNameFilterDisables(false);
    QCOMPARE(model->nameFilters(), list);

    QString tmp = flatDirTestPath;
    QVERIFY(createFiles(tmp, list));
    QModelIndex root = model->setRootPath(tmp);
    QTRY_COMPARE(model->rowCount(root), 3);

    QStringList filters;
    filters << "a" << "b";
    model->setNameFilters(filters);
    QTRY_COMPARE(model->rowCount(root), 2);
}
void tst_QFileSystemModel::setData_data()
{
    QTest::addColumn<QString>("subdirName");
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QString>("oldFileName");
    QTest::addColumn<QString>("newFileName");
    QTest::addColumn<bool>("success");
    /*QTest::newRow("outside current dir") << (QStringList() << "a" << "b" << "c")
              << flatDirTestPath + '/' + "a"
              << QDir::temp().absolutePath() + '/' + "a"
              << false;
    */
    QTest::newRow("in current dir")
              << QString()
              << (QStringList() << "a" << "b" << "c")
              << "a"
              << "d"
              << true;
    QTest::newRow("in subdir")
              << "s"
              << (QStringList() << "a" << "b" << "c")
              << "a"
              << "d"
              << true;
}

void tst_QFileSystemModel::setData()
{
    QSignalSpy spy(model, SIGNAL(fileRenamed(QString,QString,QString)));
    QFETCH(QString, subdirName);
    QFETCH(QStringList, files);
    QFETCH(QString, oldFileName);
    QFETCH(QString, newFileName);
    QFETCH(bool, success);

    QString tmp = flatDirTestPath;
    if (!subdirName.isEmpty()) {
        QDir dir(tmp);
        QVERIFY(dir.mkdir(subdirName));
        tmp.append('/' + subdirName);
    }
    QVERIFY(createFiles(tmp, files));
    QModelIndex tmpIdx = model->setRootPath(flatDirTestPath);
    if (!subdirName.isEmpty()) {
        tmpIdx = model->index(tmp);
        model->fetchMore(tmpIdx);
    }
    QTRY_COMPARE(model->rowCount(tmpIdx), files.count());

    QModelIndex idx = model->index(tmp + '/' + oldFileName);
    QCOMPARE(idx.isValid(), true);
    QCOMPARE(model->setData(idx, newFileName), false);

    model->setReadOnly(false);
    QCOMPARE(model->setData(idx, newFileName), success);
    model->setReadOnly(true);
    if (success) {
        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(model->data(idx, QFileSystemModel::FileNameRole).toString(), newFileName);
        QCOMPARE(model->fileInfo(idx).filePath(), tmp + '/' + newFileName);
        QCOMPARE(model->index(arguments.at(0).toString()), model->index(tmp));
        QCOMPARE(arguments.at(1).toString(), oldFileName);
        QCOMPARE(arguments.at(2).toString(), newFileName);
        QCOMPARE(QFile::rename(tmp + '/' + newFileName, tmp + '/' + oldFileName), true);
    }
    QTRY_COMPARE(model->rowCount(tmpIdx), files.count());
    // cleanup
    if (!subdirName.isEmpty())
        QVERIFY(QDir(tmp).removeRecursively());
}

void tst_QFileSystemModel::sortPersistentIndex()
{
    QTemporaryFile file(flatDirTestPath + QStringLiteral("/XXXXXX.dat"));
    QVERIFY2(file.open(), qPrintable(file.errorString()));
    QModelIndex root = model->setRootPath(flatDirTestPath);
    QTRY_VERIFY(model->rowCount(root) > 0);

    QPersistentModelIndex idx = model->index(0, 1, root);
    model->sort(0, Qt::AscendingOrder);
    model->sort(0, Qt::DescendingOrder);
    QVERIFY(idx.column() != 0);
}

class MyFriendFileSystemModel : public QFileSystemModel
{
    friend class tst_QFileSystemModel;
    Q_DECLARE_PRIVATE(QFileSystemModel)
};

void tst_QFileSystemModel::sort_data()
{
    QTest::addColumn<bool>("fileDialogMode");
    QTest::newRow("standard usage") << false;
    QTest::newRow("QFileDialog usage") << true;
}

void tst_QFileSystemModel::sort()
{
    QFETCH(bool, fileDialogMode);

    MyFriendFileSystemModel *myModel = new MyFriendFileSystemModel();
    QTreeView *tree = new QTreeView();

#ifdef QT_BUILD_INTERNAL
    if (fileDialogMode)
        myModel->d_func()->disableRecursiveSort = true;
#endif

    QDir dir(flatDirTestPath);
    const QString dirPath = dir.absolutePath();

    //Create a file that will be at the end when sorting by name (For Mac, the default)
    //but if we sort by size descending it will be the first
    QFile tempFile(dirPath + "/plop2.txt");
    tempFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&tempFile);
    out << "The magic number is: " << 49 << "\n";
    tempFile.close();

    QFile tempFile2(dirPath + "/plop.txt");
    tempFile2.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out2(&tempFile2);
    out2 << "The magic number is : " << 49 << " but i write some stuff in the file \n";
    tempFile2.close();

    myModel->setRootPath("");
    myModel->setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
    tree->setSortingEnabled(true);
    tree->setModel(myModel);
    tree->show();
    tree->resize(800, 800);
    QTest::qWait(500);
    tree->header()->setSortIndicator(1,Qt::DescendingOrder);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    QStringList dirsToOpen;
    do
    {
        dirsToOpen<<dir.absolutePath();
    } while (dir.cdUp());

    for (int i = dirsToOpen.size() -1 ; i > 0 ; --i) {
        QString path = dirsToOpen[i];
        QTest::qWait(500);
        tree->expand(myModel->index(path, 0));
    }
    tree->expand(myModel->index(dirPath, 0));
    QTest::qWait(500);
    QModelIndex parent = myModel->index(dirPath, 0);
    QList<QString> expectedOrder;
    expectedOrder << tempFile2.fileName() << tempFile.fileName() << dirPath + QChar('/') + ".." << dirPath + QChar('/') + ".";

    if (fileDialogMode) {
        // File dialog Mode means sub trees are not sorted, only the current root.
        // There's no way we can check that the sub tree is "not sorted"; just check if it
        // has the same contents of the expected list
        QList<QString> actualRows;
        for(int i = 0; i < myModel->rowCount(parent); ++i)
        {
            actualRows << dirPath + QChar('/') + myModel->index(i, 1, parent).data(QFileSystemModel::FileNameRole).toString();
        }

        std::sort(expectedOrder.begin(), expectedOrder.end());
        std::sort(actualRows.begin(), actualRows.end());

        QCOMPARE(actualRows, expectedOrder);
    } else {
        for(int i = 0; i < myModel->rowCount(parent); ++i)
        {
            QTRY_COMPARE(dirPath + QChar('/') + myModel->index(i, 1, parent).data(QFileSystemModel::FileNameRole).toString(), expectedOrder.at(i));
        }
    }

    delete tree;
    delete myModel;
}

void tst_QFileSystemModel::mkdir()
{
    QString tmp = flatDirTestPath;
    QString newFolderPath = QDir::toNativeSeparators(tmp + '/' + "NewFoldermkdirtest4");
    QModelIndex tmpDir = model->index(tmp);
    QVERIFY(tmpDir.isValid());
    QDir bestatic(newFolderPath);
    if (bestatic.exists()) {
        if (!bestatic.rmdir(newFolderPath))
            qWarning() << "unable to remove" << newFolderPath;
        QTest::qWait(WAITTIME);
    }
    model->mkdir(tmpDir, "NewFoldermkdirtest3");
    model->mkdir(tmpDir, "NewFoldermkdirtest5");
    QModelIndex idx = model->mkdir(tmpDir, "NewFoldermkdirtest4");
    QVERIFY(idx.isValid());
    int oldRow = idx.row();
    QTest::qWait(WAITTIME);
    idx = model->index(newFolderPath);
    QVERIFY(model->remove(idx));
    QVERIFY(!bestatic.exists());
    QVERIFY(0 != idx.row());
    QCOMPARE(oldRow, idx.row());
}

void tst_QFileSystemModel::deleteFile()
{
    QString newFilePath = QDir::temp().filePath("NewFileDeleteTest");
    QFile newFile(newFilePath);
    if (newFile.exists()) {
        if (!newFile.remove())
            qWarning() << "unable to remove" << newFilePath;
        QTest::qWait(WAITTIME);
    }
    if (!newFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "unable to create" << newFilePath;
    }
    newFile.close();
    QModelIndex idx = model->index(newFilePath);
    QVERIFY(idx.isValid());
    QVERIFY(model->remove(idx));
    QVERIFY(!newFile.exists());
}

void tst_QFileSystemModel::caseSensitivity()
{
    QString tmp = flatDirTestPath;
    QStringList files;
    files << "a" << "c" << "C";
    QVERIFY(createFiles(tmp, files));
    QModelIndex root = model->index(tmp);
    QCOMPARE(model->rowCount(root), 0);
    for (int i = 0; i < files.count(); ++i) {
        QVERIFY(model->index(tmp + '/' + files.at(i)).isValid());
    }
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
void tst_QFileSystemModel::Win32LongFileName()
{
    QString tmp = flatDirTestPath;
    QStringList files;
    files << "aaaaaaaaaa" << "bbbbbbbbbb" << "cccccccccc";
    QVERIFY(createFiles(tmp, files));
    QModelIndex root = model->setRootPath(tmp);
    QTRY_VERIFY(model->index(tmp + QLatin1String("/aaaaaa~1")).isValid());
    QTRY_VERIFY(model->index(tmp + QLatin1String("/bbbbbb~1")).isValid());
    QTRY_VERIFY(model->index(tmp + QLatin1String("/cccccc~1")).isValid());
}
#endif

void tst_QFileSystemModel::drives_data()
{
    QTest::addColumn<QString>("path");
    QTest::newRow("current") << QDir::currentPath();
    QTest::newRow("slash") << "/";
    QTest::newRow("My Computer") << "My Computer";
}

void tst_QFileSystemModel::drives()
{
    QFETCH(QString, path);
    QFileSystemModel model;
    model.setRootPath(path);
    model.fetchMore(QModelIndex());
    QFileInfoList drives = QDir::drives();
    int driveCount = 0;
    foreach(const QFileInfo& driveRoot, drives)
        if (driveRoot.exists())
            driveCount++;
    QTest::qWait(5000);
    QTRY_COMPARE(model.rowCount(), driveCount);
}

void tst_QFileSystemModel::dirsBeforeFiles()
{
    QDir dir(flatDirTestPath);

    for (int i = 0; i < 3; ++i) {
        QLatin1Char c('a' + i);
        dir.mkdir(c + QLatin1String("-dir"));
        QFile file(flatDirTestPath + QLatin1Char('/') + c + QLatin1String("-file"));
        file.open(QIODevice::ReadWrite);
        file.close();
    }

    QModelIndex root = model->setRootPath(flatDirTestPath);
    QTest::qWait(1000); // allow model to be notified by the file system watcher

    // ensure that no file occurs before a directory
    for (int i = 0; i < model->rowCount(root); ++i) {
#ifndef Q_OS_MAC
        QVERIFY(i == 0 ||
                !(model->fileInfo(model->index(i - 1, 0, root)).isFile()
                  && model->fileInfo(model->index(i, 0, root)).isDir()));
#else
        QVERIFY(i == 0 ||
                model->fileInfo(model->index(i - 1, 0, root)).fileName() <
                model->fileInfo(model->index(i, 0, root)).fileName());
#endif
    }
}

void tst_QFileSystemModel::roleNames_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<QByteArray>("roleName");
    QTest::newRow("decoration") << int(Qt::DecorationRole) << QByteArray("decoration");
    QTest::newRow("display") << int(Qt::DisplayRole) << QByteArray("display");
    QTest::newRow("fileIcon") << int(QFileSystemModel::FileIconRole) << QByteArray("fileIcon");
    QTest::newRow("filePath") << int(QFileSystemModel::FilePathRole) << QByteArray("filePath");
    QTest::newRow("fileName") << int(QFileSystemModel::FileNameRole) << QByteArray("fileName");
    QTest::newRow("filePermissions") << int(QFileSystemModel::FilePermissions) << QByteArray("filePermissions");
}

void tst_QFileSystemModel::roleNames()
{
    QFileSystemModel model;
    QHash<int, QByteArray> roles = model.roleNames();

    QFETCH(int, role);
    QVERIFY(roles.contains(role));

    QFETCH(QByteArray, roleName);
    QList<QByteArray> values = roles.values(role);
    QVERIFY(values.contains(roleName));
}

static inline QByteArray permissionRowName(bool readOnly, int permission)
{
    QByteArray result = readOnly ? QByteArrayLiteral("ro") : QByteArrayLiteral("rw");
    result += QByteArrayLiteral("-0");
    result += QByteArray::number(permission, 16);
    return result;
}

void tst_QFileSystemModel::permissions_data()
{
    QTest::addColumn<int>("permissions");
    QTest::addColumn<bool>("readOnly");

    static const int permissions[] = {
        QFile::WriteOwner,
        QFile::ReadOwner,
        QFile::WriteOwner|QFile::ReadOwner,
    };
    for (size_t i = 0; i < sizeof permissions / sizeof *permissions; ++i) {
        QTest::newRow(permissionRowName(false, permissions[i]).constData()) << permissions[i] << false;
        QTest::newRow(permissionRowName(true, permissions[i]).constData()) << permissions[i] << true;
    }
}

void tst_QFileSystemModel::permissions() // checks QTBUG-20503
{
    QFETCH(int, permissions);
    QFETCH(bool, readOnly);

    const QString tmp = flatDirTestPath;
    const QString file = tmp + QLatin1String("/f");
    QVERIFY(createFiles(tmp, QStringList() << "f"));

    QVERIFY(QFile::setPermissions(file,  QFile::Permissions(permissions)));

    const QModelIndex root = model->setRootPath(tmp);

    model->setReadOnly(readOnly);

    QCOMPARE(model->isReadOnly(), readOnly);

    QTRY_COMPARE(model->rowCount(root), 1);

    const QFile::Permissions modelPermissions = model->permissions(model->index(0, 0, root));
    const QFile::Permissions modelFileInfoPermissions = model->fileInfo(model->index(0, 0, root)).permissions();
    const QFile::Permissions fileInfoPermissions = QFileInfo(file).permissions();

    QCOMPARE(modelPermissions, modelFileInfoPermissions);
    QCOMPARE(modelFileInfoPermissions, fileInfoPermissions);
    QCOMPARE(fileInfoPermissions, modelPermissions);
}

void tst_QFileSystemModel::doNotUnwatchOnFailedRmdir()
{
    const QString tmp = flatDirTestPath;

    QFileSystemModel model;

    const QTemporaryDir tempDir(tmp + '/' + QStringLiteral("doNotUnwatchOnFailedRmdir-XXXXXX"));
    QVERIFY(tempDir.isValid());

    const QModelIndex rootIndex = model.setRootPath(tempDir.path());

    // create a file in the directory so to prevent it from deletion
    {
        QFile file(tempDir.path() + '/' + QStringLiteral("file1"));
        QVERIFY(file.open(QIODevice::WriteOnly));
    }

    QCOMPARE(model.rmdir(rootIndex), false);

    // create another file
    {
        QFile file(tempDir.path() + '/' + QStringLiteral("file2"));
        QVERIFY(file.open(QIODevice::WriteOnly));
    }

    // the model must now detect this second file
    QTRY_COMPARE(model.rowCount(rootIndex), 2);
}

static QSet<QString> fileListUnderIndex(const QFileSystemModel *model, const QModelIndex &parent)
{
    QSet<QString> fileNames;
    const int rowCount = model->rowCount(parent);
    for (int i = 0; i < rowCount; ++i)
        fileNames.insert(model->index(i, 0, parent).data(QFileSystemModel::FileNameRole).toString());
    return fileNames;
}

void tst_QFileSystemModel::specialFiles()
{
    QFileSystemModel model;

    model.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);

    // Can't simply verify if the model returns a valid model index for a special file
    // as it will always return a valid index for existing files,
    // even if the file is not visible with the given filter.

#if defined(Q_OS_UNIX)
    const QModelIndex rootIndex = model.setRootPath(QStringLiteral("/dev/"));
    const QString testFileName = QStringLiteral("null");
#elif defined(Q_OS_WIN)
    const QModelIndex rootIndex = model.setRootPath(flatDirTestPath);

    const QString testFileName = QStringLiteral("linkSource.lnk");

    QFile file(flatDirTestPath + QLatin1String("/linkTarget.txt"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    QVERIFY(file.link(flatDirTestPath + '/' + testFileName));
#else
    QSKIP("Not implemented");
    QModelIndex rootIndex;
    QString testFileName;
#endif

    QTRY_VERIFY(fileListUnderIndex(&model, rootIndex).contains(testFileName));

    model.setFilter(QDir::AllEntries | QDir::Hidden);

    QTRY_VERIFY(!fileListUnderIndex(&model, rootIndex).contains(testFileName));
}

void tst_QFileSystemModel::fileInfo()
{
    QFileSystemModel model;
    QModelIndex idx;

    QVERIFY(model.fileInfo(idx).filePath().isEmpty());

    const QString dirPath = flatDirTestPath;
    QDir dir(dirPath);
    const QString subdir = QStringLiteral("subdir");
    QVERIFY(dir.mkdir(subdir));
    const QString subdirPath = dir.absoluteFilePath(subdir);

    idx = model.setRootPath(subdirPath);
    QCOMPARE(model.fileInfo(idx), QFileInfo(subdirPath));
    idx = model.setRootPath(dirPath);
    QCOMPARE(model.fileInfo(idx), QFileInfo(dirPath));
}

QTEST_MAIN(tst_QFileSystemModel)
#include "tst_qfilesystemmodel.moc"

