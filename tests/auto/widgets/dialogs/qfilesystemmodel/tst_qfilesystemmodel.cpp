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


#include <emulationdetector.h>
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
#if defined(Q_OS_WIN)
# include <qt_windows.h> // for SetFileAttributes
#endif
#include <private/qfilesystemengine_p.h>

#include <algorithm>

#define WAITTIME 1000

// Will try to wait for the condition while allowing event processing
// for a maximum of 5 seconds.
#define TRY_WAIT(expr, timedOut) \
    do { \
        *timedOut = true; \
        const int step = 50; \
        for (int __i = 0; __i < 5000; __i += step) { \
            if (expr) { \
                *timedOut = false; \
                break; \
            } \
            QTest::qWait(step); \
        } \
    } while(0)

Q_DECLARE_METATYPE(QDir::Filters)
Q_DECLARE_METATYPE(QFileDevice::Permissions)

Q_LOGGING_CATEGORY(lcFileSystemModel, "qt.widgets.tests.qfilesystemmodel")

class tst_QFileSystemModel : public QObject {
  Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

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
    void deleteDirectory();

    void caseSensitivity();

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
    bool createFiles(QFileSystemModel *model, const QString &test_path,
                     const QStringList &initial_files, int existingFileCount = 0,
                     const QStringList &initial_dirs = QStringList());
    QModelIndex prepareTestModelRoot(QFileSystemModel *model, const QString &test_path,
                                     QSignalSpy **spy2 = nullptr, QSignalSpy **spy3 = nullptr);

private:
    QString flatDirTestPath;
    QTemporaryDir m_tempDir;
};

void tst_QFileSystemModel::cleanup()
{
    QDir dir(flatDirTestPath);
    if (dir.exists()) {
        const QDir::Filters filters = QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot;
        const QFileInfoList list = dir.entryInfoList(filters);
        for (const QFileInfo &fi : list) {
            if (fi.isDir()) {
                QVERIFY(dir.rmdir(fi.fileName()));
            } else {
                QFile dead(fi.absoluteFilePath());
                dead.setPermissions(QFile::ReadUser | QFile::ReadOwner | QFile::ExeOwner | QFile::ExeUser | QFile::WriteUser | QFile::WriteOwner | QFile::WriteOther);
                QVERIFY(dead.remove());
            }
        }
        QVERIFY(dir.entryInfoList(filters).isEmpty());
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
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    int depth = QDir::currentPath().count('/');
    model->setRootPath(QDir::currentPath());
    QString backPath;
    for (int i = 0; i <= depth * 2 + 1; ++i) {
        backPath += "../";
        QModelIndex idx = model->index(backPath);
        QVERIFY(i != depth - 1 ? idx.isValid() : !idx.isValid());
    }
#endif
}

void tst_QFileSystemModel::rootPath()
{
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QCOMPARE(model->rootPath(), QString(QDir().path()));

    QSignalSpy rootChanged(model.data(), &QFileSystemModel::rootPathChanged);
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
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QCOMPARE(model->isReadOnly(), true);
    QTemporaryFile file(flatDirTestPath + QStringLiteral("/XXXXXX.dat"));
    QVERIFY2(file.open(), qPrintable(file.errorString()));
    const QString fileName = file.fileName();
    file.close();

    const QFileInfo fileInfo(fileName);
    QTRY_VERIFY(QDir(flatDirTestPath).entryInfoList().contains(fileInfo));
    QModelIndex root = model->setRootPath(flatDirTestPath);

    QTRY_VERIFY(model->rowCount(root) > 0);
    QVERIFY(!(model->flags(model->index(fileName)) & Qt::ItemIsEditable));
    model->setReadOnly(false);
    QCOMPARE(model->isReadOnly(), false);
    QVERIFY(model->flags(model->index(fileName)) & Qt::ItemIsEditable);
}

class CustomFileIconProvider : public QFileIconProvider
{
public:
    CustomFileIconProvider() : QFileIconProvider()
    {
        auto style = QApplication::style();
        mb = style->standardIcon(QStyle::SP_MessageBoxCritical);
        dvd = style->standardIcon(QStyle::SP_DriveDVDIcon);
    }

    QIcon icon(const QFileInfo &info) const override
    {
        if (info.isDir())
            return mb;

        return QFileIconProvider::icon(info);
    }
    QIcon icon(IconType type) const override
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
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QVERIFY(model->iconProvider());
    QScopedPointer<QFileIconProvider> provider(new QFileIconProvider);
    model->setIconProvider(provider.data());
    QCOMPARE(model->iconProvider(), provider.data());
    model->setIconProvider(nullptr);
    provider.reset();

    QScopedPointer<QFileSystemModel> myModel(new QFileSystemModel);
    const QStringList documentPaths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    QVERIFY(!documentPaths.isEmpty());
    myModel->setRootPath(documentPaths.constFirst());
    //We change the provider, icons must be updated
    provider.reset(new CustomFileIconProvider);
    myModel->setIconProvider(provider.data());

    QPixmap mb = QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(50, 50);
    QCOMPARE(myModel->fileIcon(myModel->index(QDir::homePath())).pixmap(50, 50), mb);
}

bool tst_QFileSystemModel::createFiles(QFileSystemModel *model, const QString &test_path,
                                       const QStringList &initial_files, int existingFileCount,
                                       const QStringList &initial_dirs)
{
    qCDebug(lcFileSystemModel) << (model->rowCount(model->index(test_path))) << existingFileCount << initial_files;
    bool timedOut = false;
    TRY_WAIT((model->rowCount(model->index(test_path)) == existingFileCount), &timedOut);
    if (timedOut)
        return false;

    QDir dir(test_path);
    if (!dir.exists()) {
        qWarning() << "error" << test_path << "doesn't exist";
        return false;
    }
    for (const auto &initial_dir : initial_dirs) {
        if (!dir.mkdir(initial_dir)) {
            qWarning() << "error" << "failed to make" << initial_dir;
            return false;
        }
        qCDebug(lcFileSystemModel) << test_path + '/' + initial_dir << (QFile::exists(test_path + '/' + initial_dir));
    }
    for (const auto &initial_file : initial_files) {
        QFile file(test_path + '/' + initial_file);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            qDebug() << "failed to open file" << initial_file;
            return false;
        }
        if (!file.resize(1024 + file.size())) {
            qDebug() << "failed to resize file" << initial_file;
            return false;
        }
        if (!file.flush()) {
            qDebug() << "failed to flush file" << initial_file;
            return false;
        }
        file.close();
#if defined(Q_OS_WIN)
        if (initial_file[0] == '.') {
            const QString hiddenFile = QDir::toNativeSeparators(file.fileName());
            const auto nativeHiddenFile = reinterpret_cast<const wchar_t *>(hiddenFile.utf16());
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
        qCDebug(lcFileSystemModel) << test_path + '/' + initial_file << (QFile::exists(test_path + '/' + initial_file));
    }
    return true;
}

QModelIndex tst_QFileSystemModel::prepareTestModelRoot(QFileSystemModel *model, const QString &test_path,
                                                       QSignalSpy **spy2, QSignalSpy **spy3)
{
    if (model->rowCount(model->index(test_path)) != 0)
        return QModelIndex();

    if (spy2)
        *spy2 = new QSignalSpy(model, &QFileSystemModel::rowsInserted);
    if (spy3)
        *spy3 = new QSignalSpy(model, &QFileSystemModel::rowsAboutToBeInserted);

    QStringList files = { "b", "d", "f", "h", "j", ".a", ".c", ".e", ".g" };

    if (!createFiles(model, test_path, files))
        return QModelIndex();

    QModelIndex root = model->setRootPath(test_path);
    if (!root.isValid())
        return QModelIndex();

    bool timedOut = false;
    TRY_WAIT(model->rowCount(root) == 5, &timedOut);
    if (timedOut)
        return QModelIndex();

    return root;
}

void tst_QFileSystemModel::rowCount()
{
    QSignalSpy *spy2 = nullptr;
    QSignalSpy *spy3 = nullptr;
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QModelIndex root = prepareTestModelRoot(model.data(), flatDirTestPath, &spy2, &spy3);
    QVERIFY(root.isValid());

    QVERIFY(spy2 && spy2->count() > 0);
    QVERIFY(spy3 && spy3->count() > 0);
}

void tst_QFileSystemModel::rowsInserted_data()
{
    QTest::addColumn<int>("count");
    QTest::addColumn<Qt::SortOrder>("ascending");
    for (int i = 0; i < 4; ++i) {
        const QByteArray iB = QByteArray::number(i);
        QTest::newRow(("Qt::AscendingOrder " + iB).constData()) << i << Qt::AscendingOrder;
        QTest::newRow(("Qt::DescendingOrder " + iB).constData()) << i << Qt::DescendingOrder;
    }
}

static inline QString lastEntry(const QModelIndex &root)
{
    const QAbstractItemModel *model = root.model();
    return model->index(model->rowCount(root) - 1, 0, root).data().toString();
}

void tst_QFileSystemModel::rowsInserted()
{
    const QString tmp = flatDirTestPath;
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QModelIndex root = prepareTestModelRoot(model.data(), tmp);
    QVERIFY(root.isValid());

    QFETCH(Qt::SortOrder, ascending);
    QFETCH(int, count);
    model->sort(0, ascending);

    QSignalSpy spy0(model.data(), &QAbstractItemModel::rowsInserted);
    QSignalSpy spy1(model.data(), &QAbstractItemModel::rowsAboutToBeInserted);
    int oldCount = model->rowCount(root);
    QStringList files;
    for (int i = 0; i < count; ++i)
        files.append(QLatin1Char('c') + QString::number(i));
    QVERIFY(createFiles(model.data(), tmp, files, 5));
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

    QVERIFY(createFiles(model.data(), tmp, QStringList(".hidden_file"), 5 + count));

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
    const QString tmp = flatDirTestPath;
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QModelIndex root = prepareTestModelRoot(model.data(), tmp);
    QVERIFY(root.isValid());

    QFETCH(int, count);
    QFETCH(Qt::SortOrder, ascending);
    model->sort(0, ascending);

    QSignalSpy spy0(model.data(), &QAbstractItemModel::rowsRemoved);
    QSignalSpy spy1(model.data(), &QAbstractItemModel::rowsAboutToBeRemoved);
    int oldCount = model->rowCount(root);
    for (int i = count - 1; i >= 0; --i) {
        const QString fileName = model->index(i, 0, root).data().toString();
        qCDebug(lcFileSystemModel) << "removing" << fileName;
        QVERIFY(QFile::remove(tmp + QLatin1Char('/') + fileName));
    }
    for (int i = 0 ; i < 10; ++i) {
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
        qCDebug(lcFileSystemModel) << "still have:" << lst << QFile::exists(tmp + QLatin1String("/.a"));
        QDir tmpLister(tmp);
        qCDebug(lcFileSystemModel) << tmpLister.entryList();
    }
    QTRY_COMPARE(model->rowCount(root), oldCount - count);

    QVERIFY(QFile::exists(tmp + QLatin1String("/.a")));
    QVERIFY(QFile::remove(tmp + QLatin1String("/.a")));
    QVERIFY(QFile::remove(tmp + QLatin1String("/.c")));

    if (count != 0) {
        QVERIFY(spy0.count() >= 1);
        QVERIFY(spy1.count() >= 1);
    } else {
        QCOMPARE(spy0.count(), 0);
        QCOMPARE(spy1.count(), 0);
    }
}

void tst_QFileSystemModel::dataChanged_data()
{
    rowsInserted_data();
}

void tst_QFileSystemModel::dataChanged()
{
    QSKIP("This can't be tested right now since we don't watch files, only directories.");

    const QString tmp = flatDirTestPath;
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QModelIndex root = prepareTestModelRoot(model.data(), tmp);
    QVERIFY(root.isValid());

    QFETCH(int, count);
    QFETCH(Qt::SortOrder, ascending);
    model->sort(0, ascending);

    QSignalSpy spy(model.data(), &QAbstractItemModel::dataChanged);
    QStringList files;
    for (int i = 0; i < count; ++i)
        files.append(model->index(i, 0, root).data().toString());
    createFiles(model.data(), tmp, files);

    QTest::qWait(WAITTIME);

    if (count != 0) QVERIFY(spy.count() >= 1); else QCOMPARE(spy.count(), 0);
}

void tst_QFileSystemModel::filters_data()
{
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QStringList>("dirs");
    QTest::addColumn<QDir::Filters>("dirFilters");
    QTest::addColumn<QStringList>("nameFilters");
    QTest::addColumn<int>("rowCount");

    const QStringList abcList{QLatin1String("a"), QLatin1String("b"), QLatin1String("c")};
    const QStringList zList{QLatin1String("Z")};

    QTest::newRow("no dirs") << abcList << QStringList() << QDir::Filters(QDir::Dirs) << QStringList() << 2;
    QTest::newRow("no dirs - dot") << abcList << QStringList() << (QDir::Dirs | QDir::NoDot) << QStringList() << 1;
    QTest::newRow("no dirs - dotdot") << abcList << QStringList() << (QDir::Dirs | QDir::NoDotDot) << QStringList() << 1;
    QTest::newRow("no dirs - dotanddotdot") << abcList << QStringList() << (QDir::Dirs | QDir::NoDotAndDotDot) << QStringList() << 0;
    QTest::newRow("one dir - dot") << abcList << zList << (QDir::Dirs | QDir::NoDot) << QStringList() << 2;
    QTest::newRow("one dir - dotdot") << abcList << zList << (QDir::Dirs | QDir::NoDotDot) << QStringList() << 2;
    QTest::newRow("one dir - dotanddotdot") << abcList << zList << (QDir::Dirs | QDir::NoDotAndDotDot) << QStringList() << 1;
    QTest::newRow("one dir") << abcList << zList << QDir::Filters(QDir::Dirs) << QStringList() << 3;
    QTest::newRow("no dir + hidden") << abcList << QStringList() << (QDir::Dirs | QDir::Hidden) << QStringList() << 2;
    QTest::newRow("dir+hid+files") << abcList << QStringList() <<
                         (QDir::Dirs | QDir::Files | QDir::Hidden) << QStringList() << 5;
    QTest::newRow("dir+file+hid-dot .A") << abcList << QStringList{QLatin1String(".A")} <<
                         (QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot) << QStringList() << 4;
    QTest::newRow("dir+files+hid+dot A") << abcList << QStringList{QLatin1String("AFolder")} <<
                         (QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot) << QStringList{QLatin1String("A*")} << 2;
    QTest::newRow("dir+files+hid+dot+cas1") << abcList << zList <<
                         (QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive) << zList << 1;
    QTest::newRow("dir+files+hid+dot+cas2") << abcList << zList <<
                         (QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive) << QStringList{QLatin1String("a")} << 1;
    QTest::newRow("dir+files+hid+dot+cas+alldir") << abcList << zList <<
                         (QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::CaseSensitive | QDir::AllDirs) << zList << 1;

    QTest::newRow("case sensitive") << QStringList{QLatin1String("Antiguagdb"), QLatin1String("Antiguamtd"),
        QLatin1String("Antiguamtp"), QLatin1String("afghanistangdb"), QLatin1String("afghanistanmtd")}
        << QStringList() << QDir::Filters(QDir::Files) << QStringList() << 5;
}

void tst_QFileSystemModel::filters()
{
    QString tmp = flatDirTestPath;
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QVERIFY(createFiles(model.data(), tmp, QStringList()));
    QModelIndex root = model->setRootPath(tmp);
    QFETCH(QStringList, files);
    QFETCH(QStringList, dirs);
    QFETCH(QDir::Filters, dirFilters);
    QFETCH(QStringList, nameFilters);
    QFETCH(int, rowCount);

    if (nameFilters.count() > 0)
        model->setNameFilters(nameFilters);
    model->setNameFilterDisables(false);
    model->setFilter(dirFilters);

    QVERIFY(createFiles(model.data(), tmp, files, 0, dirs));
    QTRY_COMPARE(model->rowCount(root), rowCount);

    // Make sure that we do what QDir does
    QDir xFactor(tmp);
    QStringList dirEntries;

    if (nameFilters.count() > 0)
        dirEntries = xFactor.entryList(nameFilters, dirFilters);
    else
        dirEntries = xFactor.entryList(dirFilters);

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
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    model->setNameFilters(list);
    model->setNameFilterDisables(false);
    QCOMPARE(model->nameFilters(), list);

    QString tmp = flatDirTestPath;
    QVERIFY(createFiles(model.data(), tmp, list));
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

    const QStringList abcList{QLatin1String("a"), QLatin1String("b"), QLatin1String("c")};
    QTest::newRow("in current dir")
              << QString()
              << abcList
              << "a"
              << "d"
              << true;
    QTest::newRow("in subdir")
              << "s"
              << abcList
              << "a"
              << "d"
              << true;
}

void tst_QFileSystemModel::setData()
{
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QSignalSpy spy(model.data(), &QFileSystemModel::fileRenamed);
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
    QVERIFY(createFiles(model.data(), tmp, files));
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
    const QFileInfo fileInfo(file.fileName());
    file.close();
    QTRY_VERIFY(QDir(flatDirTestPath).entryInfoList().contains(fileInfo));
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
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

    QScopedPointer<MyFriendFileSystemModel> myModel(new MyFriendFileSystemModel);
    QTreeView tree;
    tree.setWindowTitle(QTest::currentTestFunction());

    if (fileDialogMode && EmulationDetector::isRunningArmOnX86())
        QSKIP("Crashes in QEMU. QTBUG-70572");

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
    tree.setSortingEnabled(true);
    tree.setModel(myModel.data());
    tree.show();
    tree.resize(800, 800);
    QVERIFY(QTest::qWaitForWindowExposed(&tree));
    tree.header()->setSortIndicator(1, Qt::DescendingOrder);
    tree.header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    QStringList dirsToOpen;
    do {
        dirsToOpen << dir.absolutePath();
    } while (dir.cdUp());

    for (int i = dirsToOpen.size() -1 ; i > 0 ; --i) {
        QString path = dirsToOpen[i];
        tree.expand(myModel->index(path, 0));
    }
    tree.expand(myModel->index(dirPath, 0));
    QModelIndex parent = myModel->index(dirPath, 0);
    QList<QString> expectedOrder;
    expectedOrder << tempFile2.fileName() << tempFile.fileName() << dirPath + QChar('/') + ".." << dirPath + QChar('/') + ".";

    if (fileDialogMode) {
        QTRY_COMPARE(myModel->rowCount(parent), expectedOrder.count());
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
}

void tst_QFileSystemModel::mkdir()
{
    QString tmp = flatDirTestPath;
    QString newFolderPath = QDir::toNativeSeparators(tmp + '/' + "NewFoldermkdirtest4");
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
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
    idx = model->index(newFolderPath);
    QVERIFY(idx.isValid());
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
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QModelIndex idx = model->index(newFilePath);
    QVERIFY(idx.isValid());
    QVERIFY(model->remove(idx));
    QVERIFY(!newFile.exists());
}

void tst_QFileSystemModel::deleteDirectory()
{
    // QTBUG-65683: Verify that directories can be removed recursively despite
    // file system watchers being active on them or their sub-directories (Windows).
    // Create a temporary directory, a nested directory and expand a treeview
    // to show them to ensure watcher creation. Then delete the directory.
    QTemporaryDir dirToBeDeleted(flatDirTestPath + QStringLiteral("/deleteDirectory-XXXXXX"));
    QVERIFY(dirToBeDeleted.isValid());
    const QString dirToBeDeletedPath = dirToBeDeleted.path();
    const QString nestedTestDir = QStringLiteral("test");
    QVERIFY(QDir(dirToBeDeletedPath).mkpath(nestedTestDir));
    const QString nestedTestDirPath = dirToBeDeletedPath + QLatin1Char('/') + nestedTestDir;
    QFile testFile(nestedTestDirPath + QStringLiteral("/test.txt"));
    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Text));
    testFile.write("Hello\n");
    testFile.close();

    QFileSystemModel model;
    const QModelIndex rootIndex = model.setRootPath(flatDirTestPath);
    QTreeView treeView;
    treeView.setWindowTitle(QTest::currentTestFunction());
    treeView.setModel(&model);
    treeView.setRootIndex(rootIndex);

    const QModelIndex dirToBeDeletedPathIndex = model.index(dirToBeDeletedPath);
    QVERIFY(dirToBeDeletedPathIndex.isValid());
    treeView.setExpanded(dirToBeDeletedPathIndex, true);
    const QModelIndex nestedTestDirIndex = model.index(nestedTestDirPath);
    QVERIFY(nestedTestDirIndex.isValid());
    treeView.setExpanded(nestedTestDirIndex, true);

    treeView.show();
    QVERIFY(QTest::qWaitForWindowExposed(&treeView));

    QVERIFY(model.remove(dirToBeDeletedPathIndex));
    dirToBeDeleted.setAutoRemove(false);
}

static QString flipCase(QString s)
{
    for (int i = 0, size = s.size(); i < size; ++i) {
        const QChar c = s.at(i);
        if (c.isUpper())
            s[i] = c.toLower();
        else if (c.isLower())
            s[i] = c.toUpper();
    }
    return s;
}

void tst_QFileSystemModel::caseSensitivity()
{
    QString tmp = flatDirTestPath;
    QStringList files;
    files << "a" << "c" << "C";
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QVERIFY(createFiles(model.data(), tmp, files));
    QModelIndex root = model->index(tmp);
    QStringList paths;
    QModelIndexList indexes;
    QCOMPARE(model->rowCount(root), 0);
    for (int i = 0; i < files.count(); ++i) {
        const QString path = tmp + '/' + files.at(i);
        const QModelIndex index = model->index(path);
        QVERIFY(index.isValid());
        paths.append(path);
        indexes.append(index);
    }

    if (!QFileSystemEngine::isCaseSensitive()) {
        // QTBUG-31103, QTBUG-64147: Verify that files can be accessed by paths with fLipPeD case.
        for (int i = 0; i < paths.count(); ++i) {
            const QModelIndex flippedCaseIndex = model->index(flipCase(paths.at(i)));
            QCOMPARE(indexes.at(i), flippedCaseIndex);
        }
    }
}

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
    QTRY_COMPARE(model.rowCount(), driveCount);
}

void tst_QFileSystemModel::dirsBeforeFiles()
{
    auto diagnosticMsg = [](int row, const QFileInfo &left, const QFileInfo &right) -> QByteArray {
        QString message;
        QDebug(&message).noquote() << "Unexpected sort order at #" << row << ':' << left << right;
        return message.toLocal8Bit();
    };
    QTemporaryDir testDir(flatDirTestPath);
    QVERIFY2(testDir.isValid(), qPrintable(testDir.errorString()));
    QDir dir(testDir.path());

    const int itemCount = 3;
    for (int i = 0; i < itemCount; ++i) {
        QLatin1Char c('a' + char(i));
        QVERIFY(dir.mkdir(c + QLatin1String("-dir")));
        QFile file(dir.filePath(c + QLatin1String("-file")));
        QVERIFY(file.open(QIODevice::ReadWrite));
        file.close();
    }

    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QModelIndex root = model->setRootPath(dir.absolutePath());
    // Wait for model to be notified by the file system watcher
    QTRY_COMPARE(model->rowCount(root), 2 * itemCount);
    // sort explicitly - dirs before files (except on macOS), and then by name
    model->sort(0);
    // Ensure that no file occurs before any directory (see QFileSystemModelSorter):
    for (int i = 1, count = model->rowCount(root); i < count; ++i) {
        const QFileInfo previous = model->fileInfo(model->index(i - 1, 0, root));
        const QFileInfo current = model->fileInfo(model->index(i, 0, root));
#ifndef Q_OS_MAC
        QVERIFY2(!(previous.isFile() && current.isDir()), diagnosticMsg(i, previous, current).constData());
#else
        QVERIFY2(previous.fileName() < current.fileName(), diagnosticMsg(i, previous, current).constData());
#endif
    }
}

void tst_QFileSystemModel::roleNames_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<QByteArray>("roleName");
    QTest::newRow("decoration") << int(Qt::DecorationRole) << QByteArray("fileIcon");
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
    QCOMPARE(roles.values(role).count(), 1);
    QCOMPARE(roles.value(role), roleName);
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
    QTest::addColumn<QFileDevice::Permissions>("permissions");
    QTest::addColumn<bool>("readOnly");

    static const int permissions[] = {
        QFile::WriteOwner,
        QFile::ReadOwner,
        QFile::WriteOwner|QFile::ReadOwner,
    };
    for (int permission : permissions) {
        QTest::newRow(permissionRowName(false, permission).constData()) << QFileDevice::Permissions(permission) << false;
        QTest::newRow(permissionRowName(true, permission).constData()) << QFileDevice::Permissions(permission) << true;
    }
}

void tst_QFileSystemModel::permissions() // checks QTBUG-20503
{
    QFETCH(QFileDevice::Permissions, permissions);
    QFETCH(bool, readOnly);

    const QString tmp = flatDirTestPath;
    const QString file = tmp + QLatin1String("/f");
    QScopedPointer<QFileSystemModel> model(new QFileSystemModel);
    QVERIFY(createFiles(model.data(), tmp, QStringList{QLatin1String("f")}));

    QVERIFY(QFile::setPermissions(file,  permissions));

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
#ifndef Q_OS_UNIX
     QSKIP("Not implemented");
#endif

    QFileSystemModel model;

    model.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);

    // Can't simply verify if the model returns a valid model index for a special file
    // as it will always return a valid index for existing files,
    // even if the file is not visible with the given filter.

    const QModelIndex rootIndex = model.setRootPath(QStringLiteral("/dev/"));
    const QString testFileName = QStringLiteral("null");

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

