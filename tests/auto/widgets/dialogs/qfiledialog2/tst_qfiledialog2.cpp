/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qfiledialog.h>
#include <qabstractitemdelegate.h>
#include <qdirmodel.h>
#include <qitemdelegate.h>
#include <qlistview.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qtreeview.h>
#include <qheaderview.h>
#include <qcompleter.h>
#include <qaction.h>
#include <qdialogbuttonbox.h>
#include <qsortfilterproxymodel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qmenu.h>
#include "../../../../../src/widgets/dialogs/qsidebar_p.h"
#include "../../../../../src/widgets/dialogs/qfilesystemmodel_p.h"
#include "../../../../../src/widgets/dialogs/qfiledialog_p.h"

#include <qpa/qplatformdialoghelper.h>

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
#include "../../../network-settings.h"
#endif

#if defined QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
Q_GUI_EXPORT bool qt_test_isFetchedRoot();
Q_GUI_EXPORT void qt_test_resetFetchedRoot();
QT_END_NAMESPACE
#endif

class QNonNativeFileDialog : public QFileDialog
{
    Q_OBJECT
public:
    QNonNativeFileDialog(QWidget *parent = 0, const QString &caption = QString(), const QString &directory = QString(), const QString &filter = QString())
        : QFileDialog(parent, caption, directory, filter)
    {
        setOption(QFileDialog::DontUseNativeDialog, true);
    }
};

static QByteArray msgDoesNotExist(const QString &name)
{
    return (QLatin1Char('"') + QDir::toNativeSeparators(name)
        + QLatin1String("\" does not exist.")).toLocal8Bit();
}

class tst_QFileDialog2 : public QObject
{
Q_OBJECT

public:
    tst_QFileDialog2();
    virtual ~tst_QFileDialog2();

public slots:
    void initTestCase();
    void init();
    void cleanup();

private slots:
#ifdef QT_BUILD_INTERNAL
    void deleteDirAndFiles();
    void listRoot();
    void task227304_proxyOnFileDialog();
    void task236402_dontWatchDeletedDir();
    void task251321_sideBarHiddenEntries();
    void task251341_sideBarRemoveEntries();
    void task257579_sideBarWithNonCleanUrls();
#endif
    void heapCorruption();
    void filter();
    void showNameFilterDetails();
    void unc();
    void emptyUncPath();

#if !defined(QT_NO_CONTEXTMENU) && !defined(QT_NO_MENU)
    void task143519_deleteAndRenameActionBehavior();
#endif
    void task178897_minimumSize();
    void task180459_lastDirectory_data();
    void task180459_lastDirectory();
#ifndef Q_OS_MAC
    void task227930_correctNavigationKeyboardBehavior();
#endif
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    void task226366_lowerCaseHardDriveWindows();
#endif
    void completionOnLevelAfterRoot();
    void task233037_selectingDirectory();
    void task235069_hideOnEscape_data();
    void task235069_hideOnEscape();
    void task203703_returnProperSeparator();
    void task228844_ensurePreviousSorting();
    void task239706_editableFilterCombo();
    void task218353_relativePaths();
    void task254490_selectFileMultipleTimes();
    void task259105_filtersCornerCases();

    void QTBUG4419_lineEditSelectAll();
    void QTBUG6558_showDirsOnly();
    void QTBUG4842_selectFilterWithHideNameFilterDetails();
    void dontShowCompleterOnRoot();
    void nameFilterParsing_data();
    void nameFilterParsing();

private:
    void cleanupSettingsFile();

    QTemporaryDir tempDir;
};

tst_QFileDialog2::tst_QFileDialog2()
    : tempDir(QDir::tempPath() + "/tst_qfiledialog2.XXXXXX")
{
#if defined(Q_OS_WINCE)
    qApp->setAutoMaximizeThreshold(-1);
#endif
}

tst_QFileDialog2::~tst_QFileDialog2()
{
}

void tst_QFileDialog2::cleanupSettingsFile()
{
    // clean up the sidebar between each test
    QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
    settings.beginGroup(QLatin1String("FileDialog"));
    settings.remove(QString());
    settings.endGroup();
    settings.beginGroup(QLatin1String("Qt")); // Compatibility settings
    settings.remove(QLatin1String("filedialog"));
    settings.endGroup();
}

void tst_QFileDialog2::initTestCase()
{
    QVERIFY2(tempDir.isValid(), qPrintable(tempDir.errorString()));
    QStandardPaths::setTestModeEnabled(true);
    cleanupSettingsFile();
}

void tst_QFileDialog2::init()
{
    QFileDialogPrivate::setLastVisitedDirectory(QUrl());
    // populate the sidebar with some default settings
    QNonNativeFileDialog fd;
#if defined(Q_OS_WINCE)
    QTest::qWait(1000);
#endif
}

void tst_QFileDialog2::cleanup()
{
    cleanupSettingsFile();
}

#ifdef QT_BUILD_INTERNAL
void tst_QFileDialog2::listRoot()
{
    QFileInfoGatherer fileInfoGatherer;
    fileInfoGatherer.start();
    QTest::qWait(1500);
    qt_test_resetFetchedRoot();
    QString dir(QDir::currentPath());
    QNonNativeFileDialog fd(0, QString(), dir);
    fd.show();
    QCOMPARE(qt_test_isFetchedRoot(),false);
    fd.setDirectory("");
#ifdef Q_OS_WINCE
    QTest::qWait(1500);
#else
    QTest::qWait(500);
#endif
    QCOMPARE(qt_test_isFetchedRoot(),true);
}
#endif

void tst_QFileDialog2::heapCorruption()
{
    QVector<QNonNativeFileDialog*> dialogs;
    for (int i=0; i < 10; i++) {
        QNonNativeFileDialog *f = new QNonNativeFileDialog(NULL);
        dialogs << f;
    }
    qDeleteAll(dialogs);
}

struct FriendlyQFileDialog : public QNonNativeFileDialog
{
    friend class tst_QFileDialog2;
    Q_DECLARE_PRIVATE(QFileDialog)
};


#ifdef QT_BUILD_INTERNAL
void tst_QFileDialog2::deleteDirAndFiles()
{
    QString tempPath = tempDir.path() + "/QFileDialogTestDir4FullDelete";
    QDir dir;
    QVERIFY(dir.mkpath(tempPath + "/foo"));
    QVERIFY(dir.mkpath(tempPath + "/foo/B"));
    QVERIFY(dir.mkpath(tempPath + "/foo/B"));
    QVERIFY(dir.mkpath(tempPath + "/foo/c"));
    QVERIFY(dir.mkpath(tempPath + "/bar"));
    QFile(tempPath + "/foo/a");
    QTemporaryFile *t;
    t = new QTemporaryFile(tempPath + "/foo/aXXXXXX");
    t->setAutoRemove(false);
    QVERIFY2(t->open(), qPrintable(t->errorString()));
    t->close();
    delete t;

    t = new QTemporaryFile(tempPath + "/foo/B/yXXXXXX");
    t->setAutoRemove(false);
    QVERIFY2(t->open(), qPrintable(t->errorString()));
    t->close();
    delete t;
    FriendlyQFileDialog fd;
    fd.setOption(QFileDialog::DontUseNativeDialog);
    fd.d_func()->removeDirectory(tempPath);
    QFileInfo info(tempPath);
    QTest::qWait(2000);
    QVERIFY(!info.exists());
}
#endif

void tst_QFileDialog2::filter()
{
    QNonNativeFileDialog fd;
    QAction *hiddenAction = fd.findChild<QAction*>("qt_show_hidden_action");
    QVERIFY(hiddenAction);
    QVERIFY(hiddenAction->isEnabled());
    QVERIFY(!hiddenAction->isChecked());
    QDir::Filters filter = fd.filter();
    filter |= QDir::Hidden;
    fd.setFilter(filter);
    QVERIFY(hiddenAction->isChecked());
}

void tst_QFileDialog2::showNameFilterDetails()
{
    QNonNativeFileDialog fd;
    QComboBox *filters = fd.findChild<QComboBox*>("fileTypeCombo");
    QVERIFY(filters);
    QVERIFY(fd.isNameFilterDetailsVisible());


    QStringList filterChoices;
    filterChoices << "Image files (*.png *.xpm *.jpg)"
                  << "Text files (*.txt)"
                  << "Any files (*.*)";
    fd.setNameFilters(filterChoices);

    fd.setNameFilterDetailsVisible(false);
    QCOMPARE(filters->itemText(0), QString("Image files"));
    QCOMPARE(filters->itemText(1), QString("Text files"));
    QCOMPARE(filters->itemText(2), QString("Any files"));

    fd.setNameFilterDetailsVisible(true);
    QCOMPARE(filters->itemText(0), filterChoices.at(0));
    QCOMPARE(filters->itemText(1), filterChoices.at(1));
    QCOMPARE(filters->itemText(2), filterChoices.at(2));
}

void tst_QFileDialog2::unc()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    // Only test UNC on Windows./
    QString dir("\\\\"  + QtNetworkSettings::winServerName() + "\\testsharewritable");
#else
    QString dir(QDir::currentPath());
#endif
    QVERIFY2(QFile::exists(dir), msgDoesNotExist(dir).constData());
    QNonNativeFileDialog fd(0, QString(), dir);
    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
    QCOMPARE(model->index(fd.directory().absolutePath()), model->index(dir));
}

void tst_QFileDialog2::emptyUncPath()
{
    QNonNativeFileDialog fd;
    fd.show();
    QLineEdit *lineEdit = fd.findChild<QLineEdit*>("fileNameEdit");
    QVERIFY(lineEdit);
    // press 'keys' for the input
    for (int i = 0; i < 3 ; ++i)
        QTest::keyPress(lineEdit, Qt::Key_Backslash);
    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
}

#if !defined(QT_NO_CONTEXTMENU) && !defined(QT_NO_MENU)
struct MenuCloser : public QObject {
    QWidget *w;
    explicit MenuCloser(QWidget *w) : w(w) {}

    void close()
    {
        QMenu *menu = w->findChild<QMenu*>();
        if (!menu) {
            qDebug("%s: cannot find file dialog child of type QMenu", Q_FUNC_INFO);
            w->close();
        }
        QTest::keyClick(menu, Qt::Key_Escape);
    }
};
static bool openContextMenu(QFileDialog &fd)
{
    QListView *list = fd.findChild<QListView*>("listView");
    if (!list) {
        qDebug("%s: didn't find file dialog child \"listView\"", Q_FUNC_INFO);
        return false;
    }
    QTimer timer;
    timer.setInterval(300);
    timer.setSingleShot(true);
    MenuCloser closer(&fd);
    QObject::connect(&timer, &QTimer::timeout, &closer, &MenuCloser::close);
    timer.start();
    QContextMenuEvent cme(QContextMenuEvent::Mouse, QPoint(10, 10));
    qApp->sendEvent(list->viewport(), &cme); // blocks until menu is closed again.
    return true;
}

void tst_QFileDialog2::task143519_deleteAndRenameActionBehavior()
{
    // test based on task233037_selectingDirectory

    struct TestContext {
        TestContext()
            : current(QDir::current()) {}
        ~TestContext() {
            file.remove();
            current.rmdir(test.dirName());
        }
        QDir current;
        QDir test;
        QFile file;
    } ctx;

    // setup testbed
    QVERIFY(ctx.current.mkdir("task143519_deleteAndRenameActionBehavior_test")); // ensure at least one item
    ctx.test = ctx.current;
    QVERIFY(ctx.test.cd("task143519_deleteAndRenameActionBehavior_test"));
    ctx.file.setFileName(ctx.test.absoluteFilePath("hello"));
    QVERIFY(ctx.file.open(QIODevice::WriteOnly));
    QVERIFY(ctx.file.permissions() & QFile::WriteUser);
    ctx.file.close();

    QNonNativeFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    fd.setDirectory(ctx.test.absolutePath());
    fd.show();

    QTest::qWaitForWindowActive(&fd);

    // grab some internals:
    QAction *rm = fd.findChild<QAction*>("qt_delete_action");
    QVERIFY(rm);
    QAction *mv = fd.findChild<QAction*>("qt_rename_action");
    QVERIFY(mv);

    // these are the real test cases:

    // defaults
    QVERIFY(openContextMenu(fd));
    QCOMPARE(fd.selectedFiles().size(), 1);
    QCOMPARE(rm->isEnabled(), !fd.isReadOnly());
    QCOMPARE(mv->isEnabled(), !fd.isReadOnly());

    // change to non-defaults:
    fd.setReadOnly(!fd.isReadOnly());

    QVERIFY(openContextMenu(fd));
    QCOMPARE(fd.selectedFiles().size(), 1);
    QCOMPARE(rm->isEnabled(), !fd.isReadOnly());
    QCOMPARE(mv->isEnabled(), !fd.isReadOnly());

    // and changed back to defaults:
    fd.setReadOnly(!fd.isReadOnly());

    QVERIFY(openContextMenu(fd));
    QCOMPARE(fd.selectedFiles().size(), 1);
    QCOMPARE(rm->isEnabled(), !fd.isReadOnly());
    QCOMPARE(mv->isEnabled(), !fd.isReadOnly());
}
#endif // !QT_NO_CONTEXTMENU && !QT_NO_MENU

void tst_QFileDialog2::task178897_minimumSize()
{
    QNonNativeFileDialog fd;
    QSize oldMs = fd.layout()->minimumSize();
    QStringList history = fd.history();
    history << QDir::toNativeSeparators("/verylongdirectory/"
            "aaaaaaaaaabbbbbbbbcccccccccccddddddddddddddeeeeeeeeeeeeffffffffffgggtggggggggghhhhhhhhiiiiiijjjk");
    fd.setHistory(history);
    fd.show();

    QSize ms = fd.layout()->minimumSize();
    QVERIFY(ms.width() <= oldMs.width());
}

void tst_QFileDialog2::task180459_lastDirectory_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("directory");
    QTest::addColumn<bool>("isEnabled");
    QTest::addColumn<QString>("result");

    QTest::newRow("path+file") << QDir::homePath() + QDir::separator() + "foo"
            << QDir::homePath()  << true
            << QDir::homePath() + QDir::separator() + "foo"  ;
    QTest::newRow("no path") << ""
            << tempDir.path() << false << QString();
    QTest::newRow("file") << "foo"
            << QDir::currentPath() << true
            << QDir::currentPath() + QDir::separator() + "foo"  ;
    QTest::newRow("path") << QDir::homePath()
            << QDir::homePath() << false << QString();
    QTest::newRow("path not existing") << "/usr/bin/foo/bar/foo/foo.txt"
            << tempDir.path() << true
            << tempDir.path() + QDir::separator() + "foo.txt";

}

void tst_QFileDialog2::task180459_lastDirectory()
{
    if (!QGuiApplication::platformName().compare(QLatin1String("cocoa"), Qt::CaseInsensitive))
        QSKIP("Insignificant on OSX"); //QTBUG-39183
    //first visit the temp directory and close the dialog
    QNonNativeFileDialog *dlg = new QNonNativeFileDialog(0, "", tempDir.path());
    QFileSystemModel *model = dlg->findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
    QCOMPARE(model->index(tempDir.path()), model->index(dlg->directory().absolutePath()));
    delete dlg;

    QFETCH(QString, path);
    QFETCH(QString, directory);
    QFETCH(bool, isEnabled);
    QFETCH(QString, result);

    dlg = new QNonNativeFileDialog(0, "", path);
    model = dlg->findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    QCOMPARE(model->index(dlg->directory().absolutePath()), model->index(directory));

    QDialogButtonBox *buttonBox = dlg->findChild<QDialogButtonBox*>("buttonBox");
    QPushButton *button = buttonBox->button(QDialogButtonBox::Save);
    QVERIFY(button);
    QCOMPARE(button->isEnabled(), isEnabled);
    if (isEnabled)
        QCOMPARE(model->index(result), model->index(dlg->selectedFiles().first()));

    delete dlg;
}



class FilterDirModel : public QSortFilterProxyModel
{

public:
      FilterDirModel(QString root, QObject* parent=0):QSortFilterProxyModel(parent), m_root(root)
      {}
      ~FilterDirModel()
      {};

protected:
      bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
      {
            QModelIndex parentIndex;
            parentIndex = source_parent;

            QString path;
            path = parentIndex.child(source_row,0).data(Qt::DisplayRole).toString();

            do {
              path = parentIndex.data(Qt::DisplayRole).toString() + "/" + path;
              parentIndex = parentIndex.parent();
            } while(parentIndex.isValid());

            QFileInfo info(path);
            if (info.isDir() && (QDir(path) != m_root))
                return false;
            return true;
      }


private:
      QDir m_root;


};

class sortProxy : public QSortFilterProxyModel
{
public:
        sortProxy(QObject *parent) : QSortFilterProxyModel(parent)
        {
        }
protected:
        virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const
        {
            QFileSystemModel * const model = qobject_cast<QFileSystemModel *>(sourceModel());
            const QFileInfo leftInfo(model->fileInfo(left));
            const QFileInfo rightInfo(model->fileInfo(right));

            if (leftInfo.isDir() == rightInfo.isDir())
                return(leftInfo.filePath().compare(rightInfo.filePath(),Qt::CaseInsensitive) < 0);
            else if (leftInfo.isDir())
                return(false);
            else
                return(true);
        }
};

class CrashDialog : public QNonNativeFileDialog
{
        Q_OBJECT

public:
        CrashDialog(QWidget *parent, const QString &caption, const
QString &dir, const QString &filter)
                   : QNonNativeFileDialog(parent, caption, dir, filter)
        {
                sortProxy *proxyModel = new sortProxy(this);
                setProxyModel(proxyModel);
        }
};

#ifdef QT_BUILD_INTERNAL
void tst_QFileDialog2::task227304_proxyOnFileDialog()
{
    QNonNativeFileDialog fd(0, "", QDir::currentPath(), 0);
    fd.setProxyModel(new FilterDirModel(QDir::currentPath()));
    fd.show();
    QLineEdit *edit = fd.findChild<QLineEdit*>("fileNameEdit");
    QTest::qWait(200);
    QTest::keyClick(edit, Qt::Key_T);
    QTest::keyClick(edit, Qt::Key_S);
    QTest::qWait(200);
    QTest::keyClick(edit->completer()->popup(), Qt::Key_Down);

    CrashDialog *dialog = new CrashDialog(0, QString("crash dialog test"), QDir::homePath(), QString("*") );
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->show();

    QListView *list = dialog->findChild<QListView*>("listView");
    QTest::qWait(200);
    QTest::keyClick(list, Qt::Key_Down);
    QTest::keyClick(list, Qt::Key_Return);
    QTest::qWait(200);

    dialog->close();
    fd.close();

    QNonNativeFileDialog fd2(0, "I should not crash with a proxy", tempDir.path(), 0);
    QSortFilterProxyModel *pm = new QSortFilterProxyModel;
    fd2.setProxyModel(pm);
    fd2.show();
    QSidebar *sidebar = fd2.findChild<QSidebar*>("sidebar");
    sidebar->setFocus();
    sidebar->selectUrl(QUrl::fromLocalFile(QDir::homePath()));
    QTest::mouseClick(sidebar->viewport(), Qt::LeftButton, 0, sidebar->visualRect(sidebar->model()->index(1, 0)).center());
    QTest::qWait(250);
    //We shouldn't crash
}
#endif

#ifndef Q_OS_MAC
// The following test implies the folder created will appear first in
// the list. On Mac files sorting depends on the locale and the order
// displayed cannot be known for sure.
void tst_QFileDialog2::task227930_correctNavigationKeyboardBehavior()
{
    QDir current = QDir::currentPath();
    current.mkdir("test");
    current.cd("test");
    QFile file("test/out.txt");
    QFile file2("test/out2.txt");
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(file2.open(QIODevice::WriteOnly | QIODevice::Text));
    current.cdUp();
    current.mkdir("test2");
    QNonNativeFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    fd.setDirectory(current.absolutePath());
    fd.show();
    QListView *list = fd.findChild<QListView*>("listView");
    QTest::qWait(200);
    QTest::keyClick(list, Qt::Key_Down);
    QTest::keyClick(list, Qt::Key_Return);
    QTest::qWait(200);
    QTest::mouseClick(list->viewport(), Qt::LeftButton,0);
    QTest::keyClick(list, Qt::Key_Down);
    QTest::keyClick(list, Qt::Key_Backspace);
    QTest::qWait(200);
    QTest::keyClick(list, Qt::Key_Down);
    QTest::keyClick(list, Qt::Key_Down);
    QTest::keyClick(list, Qt::Key_Return);
    QTest::qWait(200);
    QCOMPARE(fd.isVisible(), true);
    QTest::qWait(200);
    file.close();
    file2.close();
    file.remove();
    file2.remove();
    current.rmdir("test");
    current.rmdir("test2");
}
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
void tst_QFileDialog2::task226366_lowerCaseHardDriveWindows()
{
    QNonNativeFileDialog fd;
    fd.setDirectory(QDir::root().path());
    fd.show();
    QLineEdit *edit = fd.findChild<QLineEdit*>("fileNameEdit");
    QToolButton *buttonParent = fd.findChild<QToolButton*>("toParentButton");
    QTest::qWait(200);
    QTest::mouseClick(buttonParent, Qt::LeftButton,0,QPoint(0,0));
    QTest::qWait(2000);
    QTest::keyClick(edit, Qt::Key_C);
    QTest::qWait(200);
    QTest::keyClick(edit->completer()->popup(), Qt::Key_Down);
    QTest::qWait(200);
    QCOMPARE(edit->text(), QString("C:/"));
    QTest::qWait(2000);
    //i clear my previous selection in the completer
    QTest::keyClick(edit->completer()->popup(), Qt::Key_Down);
    edit->clear();
    QTest::keyClick(edit, (char)(Qt::Key_C | Qt::SHIFT));
    QTest::qWait(200);
    QTest::keyClick(edit->completer()->popup(), Qt::Key_Down);
    QCOMPARE(edit->text(), QString("C:/"));
}
#endif

void tst_QFileDialog2::completionOnLevelAfterRoot()
{
    QNonNativeFileDialog fd;
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    fd.setDirectory("C:/");
    QDir current = fd.directory();
    QStringList entryList = current.entryList(QStringList(), QDir::Dirs);
    // Find a suitable test dir under c:-root:
    // - At least 6 characters long
    // - Ascii, letters only
    // - No another dir with same start
    QString testDir;
    foreach (const QString &entry, entryList) {
        if (entry.size() > 5 && QString(entry.toLatin1()).compare(entry) == 0) {
            bool invalid = false;
            for (int i = 0; i < 5; i++) {
                if (!entry.at(i).isLetter()) {
                    invalid = true;
                    break;
                }
            }
            if (!invalid) {
                foreach (const QString &check, entryList) {
                    if (check.startsWith(entry.left(5)) && check != entry) {
                        invalid = true;
                        break;
                    }
                }
            }
            if (!invalid) {
                testDir = entry;
                break;
            }
        }
    }
    if (testDir.isEmpty())
        QSKIP("This test requires to have an unique directory of at least six ascii characters under c:/");
#else
    fd.setFilter(QDir::Hidden | QDir::AllDirs | QDir::Files | QDir::System);
    fd.setDirectory("/");
    QDir etc("/etc");
    if (!etc.exists())
        QSKIP("This test requires to have an etc directory under /");
#endif
    fd.show();
    QLineEdit *edit = fd.findChild<QLineEdit*>("fileNameEdit");
    QTest::qWait(2000);
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    //I love testlib :D
    for (int i = 0; i < 5; i++)
        QTest::keyClick(edit, testDir.at(i).toLower().toLatin1() - 'a' + Qt::Key_A);
#else
    QTest::keyClick(edit, Qt::Key_E);
    QTest::keyClick(edit, Qt::Key_T);
#endif
    QTest::qWait(200);
    QTest::keyClick(edit->completer()->popup(), Qt::Key_Down);
    QTest::qWait(200);
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    QCOMPARE(edit->text(), testDir);
#else
    QTRY_COMPARE(edit->text(), QString("etc"));
#endif
}

void tst_QFileDialog2::task233037_selectingDirectory()
{
    QDir current = QDir::currentPath();
    current.mkdir("test");
    QNonNativeFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    fd.setDirectory(current.absolutePath());
    fd.setAcceptMode( QFileDialog::AcceptSave);
    fd.show();
    QListView *list = fd.findChild<QListView*>("listView");
    QTest::qWait(3000); // Wait for sort to settle (I need a signal).
#ifdef QT_KEYPAD_NAVIGATION
    list->setEditFocus(true);
#endif
    QTest::keyClick(list, Qt::Key_Down);
    QTest::qWait(100);
    QDialogButtonBox *buttonBox = fd.findChild<QDialogButtonBox*>("buttonBox");
    QPushButton *button = buttonBox->button(QDialogButtonBox::Save);
    QVERIFY(button);
    QCOMPARE(button->isEnabled(), true);
    current.rmdir("test");
}

void tst_QFileDialog2::task235069_hideOnEscape_data()
{
    QTest::addColumn<QString>("childName");
    QTest::addColumn<QFileDialog::ViewMode>("viewMode");
    QTest::newRow("listView") << QStringLiteral("listView") << QFileDialog::List;
    QTest::newRow("fileNameEdit") << QStringLiteral("fileNameEdit") << QFileDialog::List;
    QTest::newRow("treeView") << QStringLiteral("treeView") << QFileDialog::Detail;
}

void tst_QFileDialog2::task235069_hideOnEscape()
{
    QFETCH(QString, childName);
    QFETCH(QFileDialog::ViewMode, viewMode);
    QDir current = QDir::currentPath();

    QNonNativeFileDialog fd;
    QSignalSpy spyFinished(&fd, &QDialog::finished);
    QVERIFY(spyFinished.isValid());
    QSignalSpy spyRejected(&fd, &QDialog::rejected);
    QVERIFY(spyRejected.isValid());
    fd.setViewMode(viewMode);
    fd.setDirectory(current.absolutePath());
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.show();
    QWidget *child = fd.findChild<QWidget *>(childName);
    QVERIFY(child);
    child->setFocus();
    QTest::qWait(200);
    QTest::keyClick(child, Qt::Key_Escape);
    QCOMPARE(fd.isVisible(), false);
    QCOMPARE(spyFinished.count(), 1); // QTBUG-7690
    QCOMPARE(spyRejected.count(), 1); // reject(), don't hide()
}

#ifdef QT_BUILD_INTERNAL
void tst_QFileDialog2::task236402_dontWatchDeletedDir()
{
    //THIS TEST SHOULD NOT DISPLAY WARNINGS
    QDir current = QDir::currentPath();
    //make sure it is the first on the list
    current.mkdir("aaaaaaaaaa");
    FriendlyQFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    fd.setDirectory(current.absolutePath());
    fd.setAcceptMode( QFileDialog::AcceptSave);
    fd.show();
    QListView *list = fd.findChild<QListView*>("listView");
    list->setFocus();
    QTest::qWait(200);
    QTest::keyClick(list, Qt::Key_Return);
    QTest::qWait(200);
    QTest::keyClick(list, Qt::Key_Backspace);
    QTest::keyClick(list, Qt::Key_Down);
    QTest::qWait(200);
    fd.d_func()->removeDirectory(current.absolutePath() + "/aaaaaaaaaa/");
    QTest::qWait(1000);
}
#endif

void tst_QFileDialog2::task203703_returnProperSeparator()
{
    QDir current = QDir::currentPath();
    current.mkdir("aaaaaaaaaaaaaaaaaa");
    QNonNativeFileDialog fd;
    fd.setDirectory(current.absolutePath());
    fd.setViewMode(QFileDialog::List);
    fd.setFileMode(QFileDialog::Directory);
    fd.show();
    QTest::qWait(500);
    QListView *list = fd.findChild<QListView*>("listView");
    list->setFocus();
    QTest::qWait(200);
    QTest::keyClick(list, Qt::Key_Return);
    QTest::qWait(1000);
    QDialogButtonBox *buttonBox = fd.findChild<QDialogButtonBox*>("buttonBox");
    QPushButton *button = buttonBox->button(QDialogButtonBox::Cancel);
    QTest::keyClick(button, Qt::Key_Return);
    QTest::qWait(500);
    QString result = fd.selectedFiles().first();
    QVERIFY(result.at(result.count() - 1) != '/');
    QVERIFY(!result.contains('\\'));
    current.rmdir("aaaaaaaaaaaaaaaaaa");
}

void tst_QFileDialog2::task228844_ensurePreviousSorting()
{
    QDir current = QDir::currentPath();
    current.mkdir("aaaaaaaaaaaaaaaaaa");
    current.cd("aaaaaaaaaaaaaaaaaa");
    current.mkdir("a");
    current.mkdir("b");
    current.mkdir("c");
    current.mkdir("d");
    current.mkdir("e");
    current.mkdir("f");
    current.mkdir("g");
    QTemporaryFile *tempFile = new QTemporaryFile(current.absolutePath() + "/rXXXXXX");
    QVERIFY2(tempFile->open(), qPrintable(tempFile->errorString()));
    current.cdUp();

    QNonNativeFileDialog fd;
    fd.setDirectory(current.absolutePath());
    fd.setViewMode(QFileDialog::Detail);
    fd.show();
#if defined(Q_OS_WINCE)
    QTest::qWait(1500);
#else
    QTest::qWait(500);
#endif
    QTreeView *tree = fd.findChild<QTreeView*>("treeView");
    tree->header()->setSortIndicator(3,Qt::DescendingOrder);
    QTest::qWait(200);
    QDialogButtonBox *buttonBox = fd.findChild<QDialogButtonBox*>("buttonBox");
    QPushButton *button = buttonBox->button(QDialogButtonBox::Open);
    QTest::mouseClick(button, Qt::LeftButton);
#if defined(Q_OS_WINCE)
    QTest::qWait(1500);
#else
    QTest::qWait(500);
#endif
    QNonNativeFileDialog fd2;
    fd2.setFileMode(QFileDialog::Directory);
    fd2.restoreState(fd.saveState());
    current.cd("aaaaaaaaaaaaaaaaaa");
    fd2.setDirectory(current.absolutePath());
    fd2.show();
#if defined(Q_OS_WINCE)
    QTest::qWait(1500);
#else
    QTest::qWait(500);
#endif
    QTreeView *tree2 = fd2.findChild<QTreeView*>("treeView");
    tree2->setFocus();

    QCOMPARE(tree2->rootIndex().data(QFileSystemModel::FilePathRole).toString(),current.absolutePath());

    QDialogButtonBox *buttonBox2 = fd2.findChild<QDialogButtonBox*>("buttonBox");
    QPushButton *button2 = buttonBox2->button(QDialogButtonBox::Open);
    fd2.selectFile("g");
    QTest::mouseClick(button2, Qt::LeftButton);
#if defined(Q_OS_WINCE)
    QTest::qWait(1500);
#else
    QTest::qWait(500);
#endif
    QCOMPARE(fd2.selectedFiles().first(), current.absolutePath() + QChar('/') + QLatin1String("g"));

    QNonNativeFileDialog fd3(0, "This is a third file dialog", tempFile->fileName());
    fd3.restoreState(fd.saveState());
    fd3.setFileMode(QFileDialog::Directory);
    fd3.show();
#if defined(Q_OS_WINCE)
    QTest::qWait(1500);
#else
    QTest::qWait(500);
#endif
    QTreeView *tree3 = fd3.findChild<QTreeView*>("treeView");
    tree3->setFocus();

    QCOMPARE(tree3->rootIndex().data(QFileSystemModel::FilePathRole).toString(), current.absolutePath());

    QDialogButtonBox *buttonBox3 = fd3.findChild<QDialogButtonBox*>("buttonBox");
    QPushButton *button3 = buttonBox3->button(QDialogButtonBox::Open);
    QTest::mouseClick(button3, Qt::LeftButton);
#if defined(Q_OS_WINCE)
    QTest::qWait(1500);
#else
    QTest::qWait(500);
#endif
    QCOMPARE(fd3.selectedFiles().first(), tempFile->fileName());

    current.cd("aaaaaaaaaaaaaaaaaa");
    current.rmdir("a");
    current.rmdir("b");
    current.rmdir("c");
    current.rmdir("d");
    current.rmdir("e");
    current.rmdir("f");
    current.rmdir("g");
    tempFile->close();
    delete tempFile;
    current.cdUp();
    current.rmdir("aaaaaaaaaaaaaaaaaa");
}


void tst_QFileDialog2::task239706_editableFilterCombo()
{
    QNonNativeFileDialog d;
    d.setNameFilter("*.cpp *.h");

    d.show();
    QTest::qWait(500);

    QList<QComboBox *> comboList = d.findChildren<QComboBox *>();
    QComboBox *filterCombo = 0;
    foreach (QComboBox *combo, comboList) {
        if (combo->objectName() == QString("fileTypeCombo")) {
            filterCombo = combo;
            break;
        }
    }
    QVERIFY(filterCombo);
    filterCombo->setEditable(true);
    QTest::mouseClick(filterCombo, Qt::LeftButton);
    QTest::keyPress(filterCombo, Qt::Key_X);
    QTest::keyPress(filterCombo, Qt::Key_Enter); // should not trigger assertion failure
}

void tst_QFileDialog2::task218353_relativePaths()
{
    QDir appDir = QDir::current();
    QVERIFY(appDir.cdUp() != false);
    QNonNativeFileDialog d(0, "TestDialog", "..");
    QCOMPARE(d.directory().absolutePath(), appDir.absolutePath());

    d.setDirectory(appDir.absolutePath() + QLatin1String("/non-existing-directory/../another-non-existing-dir/../"));
    QCOMPARE(d.directory().absolutePath(), appDir.absolutePath());

    QDir::current().mkdir("test");
    appDir = QDir::current();
    d.setDirectory(appDir.absolutePath() + QLatin1String("/test/../test/../"));
    QCOMPARE(d.directory().absolutePath(), appDir.absolutePath());
    appDir.rmdir("test");
}

#ifdef QT_BUILD_INTERNAL
void tst_QFileDialog2::task251321_sideBarHiddenEntries()
{
    QNonNativeFileDialog fd;

    QDir current = QDir::currentPath();
    current.mkdir(".hidden");
    QDir hiddenDir = QDir(".hidden");
    hiddenDir.mkdir("subdir");
    QDir hiddenSubDir = QDir(".hidden/subdir");
    hiddenSubDir.mkdir("happy");
    hiddenSubDir.mkdir("happy2");

    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(hiddenSubDir.absolutePath());
    fd.setSidebarUrls(urls);
    fd.show();
    QTest::qWait(250);

    QSidebar *sidebar = fd.findChild<QSidebar*>("sidebar");
    sidebar->setFocus();
    sidebar->selectUrl(QUrl::fromLocalFile(hiddenSubDir.absolutePath()));
    QTest::mouseClick(sidebar->viewport(), Qt::LeftButton, 0, sidebar->visualRect(sidebar->model()->index(0, 0)).center());
    // give the background processes more time on windows mobile
#ifdef Q_OS_WINCE
    QTest::qWait(1000);
#else
    QTest::qWait(250);
#endif

    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QCOMPARE(model->rowCount(model->index(hiddenSubDir.absolutePath())), 2);

    hiddenSubDir.rmdir("happy2");
    hiddenSubDir.rmdir("happy");
    hiddenDir.rmdir("subdir");
    current.rmdir(".hidden");
}
#endif

#if defined QT_BUILD_INTERNAL
class MyQSideBar : public QSidebar
{
public :
    MyQSideBar(QWidget *parent = 0) : QSidebar(parent)
    {}

    void removeSelection() {
        QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
        QList<QPersistentModelIndex> indexes;
        for (int i = 0; i < idxs.count(); i++)
            indexes.append(idxs.at(i));

        for (int i = 0; i < indexes.count(); ++i)
            if (!indexes.at(i).data(Qt::UserRole + 1).toUrl().path().isEmpty())
                model()->removeRow(indexes.at(i).row());
    }
};
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QFileDialog2::task251341_sideBarRemoveEntries()
{
    QNonNativeFileDialog fd;

    QDir current = QDir::currentPath();
    current.mkdir("testDir");
    QDir testSubDir = QDir("testDir");

    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(testSubDir.absolutePath());
    urls << QUrl::fromLocalFile("NotFound");
    fd.setSidebarUrls(urls);
    fd.show();
    QTest::qWait(250);

    QSidebar *sidebar = fd.findChild<QSidebar*>("sidebar");
    sidebar->setFocus();
    //We enter in the first bookmark
    sidebar->selectUrl(QUrl::fromLocalFile(testSubDir.absolutePath()));
    QTest::mouseClick(sidebar->viewport(), Qt::LeftButton, 0, sidebar->visualRect(sidebar->model()->index(0, 0)).center());
    QTest::qWait(250);

    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    //There is no file
    QCOMPARE(model->rowCount(model->index(testSubDir.absolutePath())), 0);
    //Icon is not enabled QUrlModel::EnabledRole
    QVariant value = sidebar->model()->index(0, 0).data(Qt::UserRole + 2);
    QCOMPARE(qvariant_cast<bool>(value), true);

    sidebar->setFocus();
    //We enter in the second bookmark which is invalid
    sidebar->selectUrl(QUrl::fromLocalFile("NotFound"));
    QTest::mouseClick(sidebar->viewport(), Qt::LeftButton, 0, sidebar->visualRect(sidebar->model()->index(1, 0)).center());
    QTest::qWait(250);

    //We fallback to root because the entry in the bookmark is invalid
    QCOMPARE(model->rowCount(model->index("NotFound")), model->rowCount(model->index(model->rootPath())));
    //Icon is not enabled QUrlModel::EnabledRole
    value = sidebar->model()->index(1, 0).data(Qt::UserRole + 2);
    QCOMPARE(qvariant_cast<bool>(value), false);

    MyQSideBar mySideBar;
    mySideBar.setModelAndUrls(model, urls);
    mySideBar.show();
    mySideBar.selectUrl(QUrl::fromLocalFile(testSubDir.absolutePath()));
    QTest::qWait(1000);
    mySideBar.removeSelection();

    //We remove the first entry
    QList<QUrl> expected;
    expected << QUrl::fromLocalFile("NotFound");
    QCOMPARE(mySideBar.urls(), expected);

    mySideBar.selectUrl(QUrl::fromLocalFile("NotFound"));
    mySideBar.removeSelection();

    //We remove the second entry
    expected.clear();
    QCOMPARE(mySideBar.urls(), expected);

    current.rmdir("testDir");
}
#endif

void tst_QFileDialog2::task254490_selectFileMultipleTimes()
{
    QString tempPath = tempDir.path();
    QTemporaryFile *t;
    t = new QTemporaryFile;
    QVERIFY2(t->open(), qPrintable(t->errorString()));
    t->open();
    QNonNativeFileDialog fd(0, "TestFileDialog");

    fd.setDirectory(tempPath);
    fd.setViewMode(QFileDialog::List);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setFileMode(QFileDialog::AnyFile);

    //This should select the file in the QFileDialog
    fd.selectFile(t->fileName());

    //This should clear the selection and write it into the filename line edit
    fd.selectFile("new_file.txt");

    fd.show();
    QTest::qWait(250);

    QLineEdit *lineEdit = fd.findChild<QLineEdit*>("fileNameEdit");
    QVERIFY(lineEdit);
    QCOMPARE(lineEdit->text(),QLatin1String("new_file.txt"));
    QListView *list = fd.findChild<QListView*>("listView");
    QVERIFY(list);
    QCOMPARE(list->selectionModel()->selectedRows(0).count(), 0);

    t->deleteLater();
}

#ifdef QT_BUILD_INTERNAL
void tst_QFileDialog2::task257579_sideBarWithNonCleanUrls()
{
    QDir dir(tempDir.path());
    QLatin1String dirname("autotest_task257579");
    dir.rmdir(dirname); //makes sure it doesn't exist any more
    QVERIFY(dir.mkdir(dirname));
    QString url = QString::fromLatin1("%1/%2/..").arg(dir.absolutePath()).arg(dirname);
    QNonNativeFileDialog fd;
    fd.setSidebarUrls(QList<QUrl>() << QUrl::fromLocalFile(url));
    QSidebar *sidebar = fd.findChild<QSidebar*>("sidebar");
    QCOMPARE(sidebar->urls().count(), 1);
    QVERIFY(sidebar->urls().first().toLocalFile() != url);
    QCOMPARE(sidebar->urls().first().toLocalFile(), QDir::cleanPath(url));

#ifdef Q_OS_WIN
    QCOMPARE(sidebar->model()->index(0,0).data().toString().toLower(), dir.dirName().toLower());
#else
    QCOMPARE(sidebar->model()->index(0,0).data().toString(), dir.dirName());
#endif

    //all tests are finished, we can remove the temporary dir
    QVERIFY(dir.rmdir(dirname));
}
#endif

void tst_QFileDialog2::task259105_filtersCornerCases()
{
    QNonNativeFileDialog fd(0, "TestFileDialog");
    fd.setNameFilter(QLatin1String("All Files! (*);;Text Files (*.txt)"));
    fd.setOption(QFileDialog::HideNameFilterDetails, true);
    fd.show();
    QTest::qWait(250);

    //Extensions are hidden
    QComboBox *filters = fd.findChild<QComboBox*>("fileTypeCombo");
    QVERIFY(filters);
    QCOMPARE(filters->currentText(), QLatin1String("All Files!"));
    filters->setCurrentIndex(1);
    QCOMPARE(filters->currentText(), QLatin1String("Text Files"));

    //We should have the full names
    fd.setOption(QFileDialog::HideNameFilterDetails, false);
    QTest::qWait(250);
    filters->setCurrentIndex(0);
    QCOMPARE(filters->currentText(), QLatin1String("All Files! (*)"));
    filters->setCurrentIndex(1);
    QCOMPARE(filters->currentText(), QLatin1String("Text Files (*.txt)"));

    //Corner case undocumented of the task
    fd.setNameFilter(QLatin1String("\352 (I like cheese) All Files! (*);;Text Files (*.txt)"));
    QCOMPARE(filters->currentText(), QLatin1String("\352 (I like cheese) All Files! (*)"));
    filters->setCurrentIndex(1);
    QCOMPARE(filters->currentText(), QLatin1String("Text Files (*.txt)"));

    fd.setOption(QFileDialog::HideNameFilterDetails, true);
    filters->setCurrentIndex(0);
    QTest::qWait(500);
    QCOMPARE(filters->currentText(), QLatin1String("\352 (I like cheese) All Files!"));
    filters->setCurrentIndex(1);
    QCOMPARE(filters->currentText(), QLatin1String("Text Files"));

    fd.setOption(QFileDialog::HideNameFilterDetails, true);
    filters->setCurrentIndex(0);
    QTest::qWait(500);
    QCOMPARE(filters->currentText(), QLatin1String("\352 (I like cheese) All Files!"));
    filters->setCurrentIndex(1);
    QCOMPARE(filters->currentText(), QLatin1String("Text Files"));
}

void tst_QFileDialog2::QTBUG4419_lineEditSelectAll()
{
    QString tempPath = tempDir.path();
    QTemporaryFile temporaryFile(tempPath + "/tst_qfiledialog2_lineEditSelectAll.XXXXXX");
    QVERIFY2(temporaryFile.open(), qPrintable(temporaryFile.errorString()));
    QNonNativeFileDialog fd(0, "TestFileDialog", temporaryFile.fileName());

    fd.setDirectory(tempPath);
    fd.setViewMode(QFileDialog::List);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setFileMode(QFileDialog::AnyFile);

    fd.show();
    QApplication::setActiveWindow(&fd);
    QVERIFY(QTest::qWaitForWindowActive(&fd));
    QCOMPARE(fd.isVisible(), true);
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&fd));

    QLineEdit *lineEdit = fd.findChild<QLineEdit*>("fileNameEdit");
    QVERIFY(lineEdit);

    QTRY_COMPARE(tempPath + QChar('/') + lineEdit->text(), temporaryFile.fileName());
    QCOMPARE(tempPath + QChar('/') + lineEdit->selectedText(), temporaryFile.fileName());
}

void tst_QFileDialog2::QTBUG6558_showDirsOnly()
{
    const QString tempPath = tempDir.path();
    QDir dirTemp(tempPath);
    const QString tempName = QLatin1String("showDirsOnly.") + QString::number(qrand());
    dirTemp.mkdir(tempName);
    dirTemp.cd(tempName);
    QTRY_VERIFY(dirTemp.exists());

    const QString dirPath = dirTemp.absolutePath();
    QDir dir(dirPath);

    //We create two dirs
    dir.mkdir("a");
    dir.mkdir("b");

    //Create a file
    QFile tempFile(dirPath + "/plop.txt");
    tempFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&tempFile);
    out << "The magic number is: " << 49 << "\n";
    tempFile.close();

    QNonNativeFileDialog fd(0, "TestFileDialog");

    fd.setDirectory(dir.absolutePath());
    fd.setViewMode(QFileDialog::List);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setOption(QFileDialog::ShowDirsOnly, true);
    fd.show();

    QApplication::setActiveWindow(&fd);
    QVERIFY(QTest::qWaitForWindowActive(&fd));
    QCOMPARE(fd.isVisible(), true);
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&fd));

    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QTRY_COMPARE(model->rowCount(model->index(dir.absolutePath())), 2);

    fd.setOption(QFileDialog::ShowDirsOnly, false);
    QTRY_COMPARE(model->rowCount(model->index(dir.absolutePath())), 3);

    fd.setOption(QFileDialog::ShowDirsOnly, true);
    QTRY_COMPARE(model->rowCount(model->index(dir.absolutePath())), 2);

    fd.setFileMode(QFileDialog::DirectoryOnly);
    QTRY_COMPARE(model->rowCount(model->index(dir.absolutePath())), 2);
    QTRY_COMPARE(bool(fd.options() & QFileDialog::ShowDirsOnly), true);

    fd.setFileMode(QFileDialog::AnyFile);
    QTRY_COMPARE(model->rowCount(model->index(dir.absolutePath())), 3);
    QTRY_COMPARE(bool(fd.options() & QFileDialog::ShowDirsOnly), false);

    fd.setDirectory(QDir::homePath());

    //We remove the dirs
    dir.rmdir("a");
    dir.rmdir("b");

    //we delete the file
    tempFile.remove();

    dirTemp.cdUp();
    dirTemp.rmdir(tempName);
}

void tst_QFileDialog2::QTBUG4842_selectFilterWithHideNameFilterDetails()
{
    QStringList filtersStr;
    filtersStr << "Images (*.png *.xpm *.jpg)" << "Text files (*.txt)" << "XML files (*.xml)";
    QString chosenFilterString("Text files (*.txt)");

    QNonNativeFileDialog fd(0, "TestFileDialog");
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.setOption(QFileDialog::HideNameFilterDetails, true);
    fd.setNameFilters(filtersStr);
    fd.selectNameFilter(chosenFilterString);
    fd.show();

    QApplication::setActiveWindow(&fd);
    QVERIFY(QTest::qWaitForWindowActive(&fd));
    QCOMPARE(fd.isVisible(), true);
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&fd));

    QComboBox *filters = fd.findChild<QComboBox*>("fileTypeCombo");
    //We compare the current combobox text with the stripped version
    QCOMPARE(filters->currentText(), QString("Text files"));

    QNonNativeFileDialog fd2(0, "TestFileDialog");
    fd2.setAcceptMode(QFileDialog::AcceptSave);
    fd2.setOption(QFileDialog::HideNameFilterDetails, false);
    fd2.setNameFilters(filtersStr);
    fd2.selectNameFilter(chosenFilterString);
    fd2.show();

    QApplication::setActiveWindow(&fd2);
    QVERIFY(QTest::qWaitForWindowActive(&fd2));
    QCOMPARE(fd2.isVisible(), true);
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&fd2));

    QComboBox *filters2 = fd2.findChild<QComboBox*>("fileTypeCombo");
    //We compare the current combobox text with the non stripped version
    QCOMPARE(filters2->currentText(), chosenFilterString);

}

void tst_QFileDialog2::dontShowCompleterOnRoot()
{
    QNonNativeFileDialog fd(0, "TestFileDialog");
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.show();

    QApplication::setActiveWindow(&fd);
    QVERIFY(QTest::qWaitForWindowActive(&fd));
    QCOMPARE(fd.isVisible(), true);
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&fd));

    fd.setDirectory("");
    QLineEdit *lineEdit = fd.findChild<QLineEdit*>("fileNameEdit");
    QTRY_VERIFY(lineEdit->text().isEmpty());

    //The gatherer thread will then return the result
    QApplication::processEvents();

    QTRY_VERIFY(lineEdit->completer()->popup()->isHidden());
}

void tst_QFileDialog2::nameFilterParsing_data()
{
    QTest::addColumn<QString>("filterString");
    QTest::addColumn<QStringList>("filters");

    // QTBUG-47923: Do not trip over "*,v".
    QTest::newRow("text") << "plain text document (*.txt *.asc *,v *.doc)"
        << (QStringList() << "*.txt" << "*.asc" << "*,v" << "*.doc");
    QTest::newRow("html") << "HTML document (*.html *.htm)"
       << (QStringList() << "*.html" <<  "*.htm");
}

void tst_QFileDialog2::nameFilterParsing()
{
    QFETCH(QString, filterString);
    QFETCH(QStringList, filters);
    QCOMPARE(QPlatformFileDialogHelper::cleanFilterList(filterString), filters);
}

QTEST_MAIN(tst_QFileDialog2)
#include "tst_qfiledialog2.moc"
