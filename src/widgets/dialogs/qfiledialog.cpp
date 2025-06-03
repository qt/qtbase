// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_NO_URL_CAST_FROM_STRING

#include <qvariant.h>
#include <private/qwidgetitemdata_p.h>
#include "qfiledialog.h"

#include "qfiledialog_p.h"
#include <private/qapplication_p.h>
#include <private/qguiapplication_p.h>
#include <qfontmetrics.h>
#include <qaction.h>
#include <qactiongroup.h>
#include <qheaderview.h>
#if QT_CONFIG(shortcut)
#  include <qshortcut.h>
#endif
#include <qgridlayout.h>
#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#if QT_CONFIG(messagebox)
#include <qmessagebox.h>
#endif
#include <stdlib.h>
#if QT_CONFIG(settings)
#include <qsettings.h>
#endif
#include <qdebug.h>
#if QT_CONFIG(mimetype)
#include <qmimedatabase.h>
#endif
#if QT_CONFIG(regularexpression)
#include <qregularexpression.h>
#endif
#include <qapplication.h>
#include <qstylepainter.h>
#include "ui_qfiledialog.h"
#if defined(Q_OS_UNIX)
#include <pwd.h>
#include <unistd.h> // for pathconf() on OS X
#elif defined(Q_OS_WIN)
#  include <QtCore/qt_windows.h>
#endif
#if defined(Q_OS_WASM)
#include <private/qwasmlocalfileaccess_p.h>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_GLOBAL_STATIC(QUrl, lastVisitedDir)

/*!
  \class QFileDialog
  \brief The QFileDialog class provides a dialog that allows users to select files or directories.
  \ingroup standard-dialogs
  \inmodule QtWidgets

  The QFileDialog class enables a user to traverse the file system
  to select one or many files or a directory.

  \image qtquickdialogs-filedialog-gtk.png

  The easiest way to create a QFileDialog is to use the static functions,
  such as \l getOpenFileName().

  \snippet code/src_gui_dialogs_qfiledialog.cpp 0

  In the above example, a modal QFileDialog is created using a static
  function. The dialog initially displays the contents of the "/home/jana"
  directory, and displays files matching the patterns given in the
  string "Image Files (*.png *.jpg *.bmp)". The parent of the file dialog
  is set to \e this, and the window title is set to "Open Image".

  If you want to use multiple filters, separate each one with
  \e two semicolons. For example:

  \snippet code/src_gui_dialogs_qfiledialog.cpp 1

  You can create your own QFileDialog without using the static
  functions. By calling setFileMode(), you can specify what the user must
  select in the dialog:

  \snippet code/src_gui_dialogs_qfiledialog.cpp 2

  In the above example, the mode of the file dialog is set to
  AnyFile, meaning that the user can select any file, or even specify a
  file that doesn't exist. This mode is useful for creating a
  "Save As" file dialog. Use ExistingFile if the user must select an
  existing file, or \l Directory if only a directory can be selected.
  See the \l QFileDialog::FileMode enum for the complete list of modes.

  The fileMode property contains the mode of operation for the dialog;
  this indicates what types of objects the user is expected to select.
  Use setNameFilter() to set the dialog's file filter. For example:

  \snippet code/src_gui_dialogs_qfiledialog.cpp 3

  In the above example, the filter is set to \c{"Images (*.png *.xpm *.jpg)"}.
  This means that only files with the extension \c png, \c xpm,
  or \c jpg are shown in the QFileDialog. You can apply
  several filters by using setNameFilters(). Use selectNameFilter() to select
  one of the filters you've given as the file dialog's default filter.

  The file dialog has two view modes: \l{QFileDialog::}{List} and
  \l{QFileDialog::}{Detail}.
  \l{QFileDialog::}{List} presents the contents of the current directory
  as a list of file and directory names. \l{QFileDialog::}{Detail} also
  displays a list of file and directory names, but provides additional
  information alongside each name, such as the file size and modification
  date. Set the mode with setViewMode():

  \snippet code/src_gui_dialogs_qfiledialog.cpp 4

  The last important function you need to use when creating your
  own file dialog is selectedFiles().

  \snippet code/src_gui_dialogs_qfiledialog.cpp 5

  In the above example, a modal file dialog is created and shown. If
  the user clicked OK, the file they selected is put in \c fileName.

  The dialog's working directory can be set with setDirectory().
  Each file in the current directory can be selected using
  the selectFile() function.

  The \l{dialogs/standarddialogs}{Standard Dialogs} example shows
  how to use QFileDialog as well as other built-in Qt dialogs.

  By default, a platform-native file dialog is used if the platform has
  one. In that case, the widgets that would otherwise be used to construct the
  dialog are not instantiated, so related accessors such as layout() and
  itemDelegate() return null. Also, not all platforms show file dialogs
  with a title bar, so be aware that the caption text might not be visible to
  the user. You can set the \l DontUseNativeDialog option or set the
  \l{Qt::AA_DontUseNativeDialogs}{AA_DontUseNativeDialogs} application attribute
  to ensure that the widget-based implementation is used instead of the native dialog.

  \sa QDir, QFileInfo, QFile, QColorDialog, QFontDialog, {Standard Dialogs Example}
*/

/*!
    \enum QFileDialog::AcceptMode

    \value AcceptOpen
    \value AcceptSave
*/

/*!
    \enum QFileDialog::ViewMode

    This enum describes the view mode of the file dialog; that is, what
    information about each file is displayed.

    \value Detail Displays an icon, a name, and details for each item in
                  the directory.
    \value List   Displays only an icon and a name for each item in the
                  directory.

    \sa setViewMode()
*/

/*!
    \enum QFileDialog::FileMode

    This enum is used to indicate what the user may select in the file
    dialog; that is, what the dialog returns if the user clicks OK.

    \value AnyFile        The name of a file, whether it exists or not.
    \value ExistingFile   The name of a single existing file.
    \value Directory      The name of a directory. Both files and
                          directories are displayed. However, the native Windows
                          file dialog does not support displaying files in the
                          directory chooser.
    \value ExistingFiles  The names of zero or more existing files.

    \sa setFileMode()
*/

/*!
    \enum QFileDialog::Option

    Options that influence the behavior of the dialog.

    \value ShowDirsOnly Only show directories. By
    default, both files and directories are shown.\br
    This option is only effective in the \l Directory file mode.

    \value DontResolveSymlinks Don't resolve symlinks.
    By default, symlinks are resolved.

    \value DontConfirmOverwrite Don't ask for confirmation if an
    existing file is selected. By default, confirmation is requested.\br
    This option is only effective if \l acceptMode is \l {QFileDialog::}{AcceptSave}).
    It is furthermore not used on macOS for native file dialogs.

    \value DontUseNativeDialog Don't use a platform-native file dialog,
    but the widget-based one provided by Qt.\br
    By default, a native file dialog is shown unless you use a subclass
    of QFileDialog that contains the Q_OBJECT macro, the global
    \l{Qt::}{AA_DontUseNativeDialogs} application attribute is set, or the platform
    does not have a native dialog of the type that you require.\br
    For the option to be effective, you must set it before changing
    other properties of the dialog, or showing the dialog.

    \value ReadOnly Indicates that the model is read-only.

    \value HideNameFilterDetails Indicates if the file name filter details are
    hidden or not.

    \value DontUseCustomDirectoryIcons Always use the default directory icon.\br
    Some platforms allow the user to set a different icon, but custom icon lookup
    might cause significant performance issues over network or removable drives.\br
    Setting this will enable the
    \l{QAbstractFileIconProvider::}{DontUseCustomDirectoryIcons}
    option in \l{iconProvider()}.\br
    This enum value was added in Qt 5.2.

    \sa options, testOption
*/

/*!
  \enum QFileDialog::DialogLabel

  \value LookIn
  \value FileName
  \value FileType
  \value Accept
  \value Reject
*/

/*!
    \fn void QFileDialog::filesSelected(const QStringList &selected)

    When the selection changes for local operations and the dialog is
    accepted, this signal is emitted with the (possibly empty) list
    of \a selected files.

    \sa currentChanged(), QDialog::Accepted
*/

/*!
    \fn void QFileDialog::urlsSelected(const QList<QUrl> &urls)

    When the selection changes and the dialog is accepted, this signal is
    emitted with the (possibly empty) list of selected \a urls.

    \sa currentUrlChanged(), QDialog::Accepted
    \since 5.2
*/

/*!
    \fn void QFileDialog::fileSelected(const QString &file)

    When the selection changes for local operations and the dialog is
    accepted, this signal is emitted with the (possibly empty)
    selected \a file.

    \sa currentChanged(), QDialog::Accepted
*/

/*!
    \fn void QFileDialog::urlSelected(const QUrl &url)

    When the selection changes and the dialog is accepted, this signal is
    emitted with the (possibly empty) selected \a url.

    \sa currentUrlChanged(), QDialog::Accepted
    \since 5.2
*/

/*!
    \fn void QFileDialog::currentChanged(const QString &path)

    When the current file changes for local operations, this signal is
    emitted with the new file name as the \a path parameter.

    \sa filesSelected()
*/

/*!
    \fn void QFileDialog::currentUrlChanged(const QUrl &url)

    When the current file changes, this signal is emitted with the
    new file URL as the \a url parameter.

    \sa urlsSelected()
    \since 5.2
*/

/*!
  \fn void QFileDialog::directoryEntered(const QString &directory)

  This signal is emitted for local operations when the user enters
  a \a directory.
*/

/*!
  \fn void QFileDialog::directoryUrlEntered(const QUrl &directory)

  This signal is emitted when the user enters a \a directory.

  \since 5.2
*/

/*!
  \fn void QFileDialog::filterSelected(const QString &filter)

  This signal is emitted when the user selects a \a filter.
*/

QT_BEGIN_INCLUDE_NAMESPACE
#include <QMetaEnum>
#if QT_CONFIG(shortcut)
#  include <qshortcut.h>
#endif
QT_END_INCLUDE_NAMESPACE

/*!
    \fn QFileDialog::QFileDialog(QWidget *parent, Qt::WindowFlags flags)

    Constructs a file dialog with the given \a parent and widget \a flags.
*/
QFileDialog::QFileDialog(QWidget *parent, Qt::WindowFlags f)
    : QDialog(*new QFileDialogPrivate, parent, f)
{
    Q_D(QFileDialog);
    QFileDialogArgs args;
    d->init(args);
}

/*!
    Constructs a file dialog with the given \a parent and \a caption that
    initially displays the contents of the specified \a directory.
    The contents of the directory are filtered before being shown in the
    dialog, using a semicolon-separated list of filters specified by
    \a filter.
*/
QFileDialog::QFileDialog(QWidget *parent,
                     const QString &caption,
                     const QString &directory,
                     const QString &filter)
    : QDialog(*new QFileDialogPrivate, parent, { })
{
    Q_D(QFileDialog);
    QFileDialogArgs args(QUrl::fromLocalFile(directory));
    args.filter = filter;
    args.caption = caption;
    d->init(args);
}

/*!
    \internal
*/
QFileDialog::QFileDialog(const QFileDialogArgs &args)
    : QDialog(*new QFileDialogPrivate, args.parent, { })
{
    Q_D(QFileDialog);
    d->init(args);
    setFileMode(args.mode);
    setOptions(args.options);
    selectFile(args.selection);
}

/*!
    Destroys the file dialog.
*/
QFileDialog::~QFileDialog()
{
    Q_D(QFileDialog);
#if QT_CONFIG(settings)
    d->saveSettings();
#endif
    if (QPlatformFileDialogHelper *platformHelper = d->platformFileDialogHelper()) {
        // QIOSFileDialog emits directoryChanged while hiding, causing an assert
        // because of a partially destroyed QFileDialog.
        QObjectPrivate::disconnect(platformHelper, &QPlatformFileDialogHelper::directoryEntered,
                                   d, &QFileDialogPrivate::nativeEnterDirectory);
    }
}

/*!
    Sets the \a urls that are located in the sidebar.

    For instance:

    \snippet filedialogurls/filedialogurls.cpp 0

    Then the file dialog looks like this:

    \image filedialogurls.png

    \sa sidebarUrls()
*/
void QFileDialog::setSidebarUrls(const QList<QUrl> &urls)
{
    Q_D(QFileDialog);
    if (!d->nativeDialogInUse)
        d->qFileDialogUi->sidebar->setUrls(urls);
}

/*!
    Returns a list of urls that are currently in the sidebar
*/
QList<QUrl> QFileDialog::sidebarUrls() const
{
    Q_D(const QFileDialog);
    return (d->nativeDialogInUse ? QList<QUrl>() : d->qFileDialogUi->sidebar->urls());
}

static const qint32 QFileDialogMagic = 0xbe;

/*!
    Saves the state of the dialog's layout, history and current directory.

    Typically this is used in conjunction with QSettings to remember the size
    for a future session. A version number is stored as part of the data.
*/
QByteArray QFileDialog::saveState() const
{
    Q_D(const QFileDialog);
    int version = 4;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_0);

    stream << qint32(QFileDialogMagic);
    stream << qint32(version);
    if (d->usingWidgets()) {
        stream << d->qFileDialogUi->splitter->saveState();
        stream << d->qFileDialogUi->sidebar->urls();
    } else {
        stream << d->splitterState;
        stream << d->sidebarUrls;
    }
    stream << history();
    stream << *lastVisitedDir();
    if (d->usingWidgets())
        stream << d->qFileDialogUi->treeView->header()->saveState();
    else
        stream << d->headerData;
    stream << qint32(viewMode());
    return data;
}

/*!
    Restores the dialogs's layout, history and current directory to the \a state specified.

    Typically this is used in conjunction with QSettings to restore the size
    from a past session.

    Returns \c false if there are errors
*/
bool QFileDialog::restoreState(const QByteArray &state)
{
    Q_D(QFileDialog);
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_5_0);
    if (stream.atEnd())
        return false;
    QStringList history;
    QUrl currentDirectory;
    qint32 marker;
    qint32 v;
    qint32 viewMode;
    stream >> marker;
    stream >> v;
    // the code below only supports versions 3 and 4
    if (marker != QFileDialogMagic || (v != 3 && v != 4))
        return false;

    stream >> d->splitterState
           >> d->sidebarUrls
           >> history;
    if (v == 3) {
        QString currentDirectoryString;
        stream >> currentDirectoryString;
        currentDirectory = QUrl::fromLocalFile(currentDirectoryString);
    } else {
        stream >> currentDirectory;
    }
    stream >> d->headerData
           >> viewMode;

    setDirectoryUrl(lastVisitedDir()->isEmpty() ? currentDirectory : *lastVisitedDir());
    setViewMode(static_cast<QFileDialog::ViewMode>(viewMode));

    if (!d->usingWidgets())
        return true;

    return d->restoreWidgetState(history, -1);
}

/*!
    \reimp
*/
void QFileDialog::changeEvent(QEvent *e)
{
    Q_D(QFileDialog);
    if (e->type() == QEvent::LanguageChange) {
        d->retranslateWindowTitle();
        d->retranslateStrings();
    }
    QDialog::changeEvent(e);
}

QFileDialogPrivate::QFileDialogPrivate()
    :
#if QT_CONFIG(proxymodel)
        proxyModel(nullptr),
#endif
        model(nullptr),
        currentHistoryLocation(-1),
        renameAction(nullptr),
        deleteAction(nullptr),
        showHiddenAction(nullptr),
        useDefaultCaption(true),
        qFileDialogUi(nullptr),
        options(QFileDialogOptions::create())
{
}

QFileDialogPrivate::~QFileDialogPrivate()
{
}

void QFileDialogPrivate::initHelper(QPlatformDialogHelper *h)
{
    Q_Q(QFileDialog);
    auto *fileDialogHelper = static_cast<QPlatformFileDialogHelper *>(h);
    QObjectPrivate::connect(fileDialogHelper, &QPlatformFileDialogHelper::fileSelected,
                            this, &QFileDialogPrivate::emitUrlSelected);
    QObjectPrivate::connect(fileDialogHelper, &QPlatformFileDialogHelper::filesSelected,
                            this, &QFileDialogPrivate::emitUrlsSelected);
    QObjectPrivate::connect(fileDialogHelper, &QPlatformFileDialogHelper::currentChanged,
                            this, &QFileDialogPrivate::nativeCurrentChanged);
    QObjectPrivate::connect(fileDialogHelper, &QPlatformFileDialogHelper::directoryEntered,
                            this, &QFileDialogPrivate::nativeEnterDirectory);
    QObject::connect(fileDialogHelper, &QPlatformFileDialogHelper::filterSelected,
                     q, &QFileDialog::filterSelected);
    fileDialogHelper->setOptions(options);
}

void QFileDialogPrivate::helperPrepareShow(QPlatformDialogHelper *)
{
    Q_Q(QFileDialog);
    options->setWindowTitle(q->windowTitle());
    options->setHistory(q->history());
    if (usingWidgets())
        options->setSidebarUrls(qFileDialogUi->sidebar->urls());
    if (options->initiallySelectedNameFilter().isEmpty())
        options->setInitiallySelectedNameFilter(q->selectedNameFilter());
    if (options->initiallySelectedFiles().isEmpty())
        options->setInitiallySelectedFiles(userSelectedFiles());
}

void QFileDialogPrivate::helperDone(QDialog::DialogCode code, QPlatformDialogHelper *)
{
    if (code == QDialog::Accepted) {
        Q_Q(QFileDialog);
        q->setViewMode(static_cast<QFileDialog::ViewMode>(options->viewMode()));
        q->setSidebarUrls(options->sidebarUrls());
        q->setHistory(options->history());
    }
}

void QFileDialogPrivate::retranslateWindowTitle()
{
    Q_Q(QFileDialog);
    if (!useDefaultCaption || setWindowTitle != q->windowTitle())
        return;
    if (q->acceptMode() == QFileDialog::AcceptOpen) {
        const QFileDialog::FileMode fileMode = q->fileMode();
        if (fileMode == QFileDialog::Directory)
            q->setWindowTitle(QFileDialog::tr("Find Directory"));
        else
            q->setWindowTitle(QFileDialog::tr("Open"));
    } else
        q->setWindowTitle(QFileDialog::tr("Save As"));

    setWindowTitle = q->windowTitle();
}

void QFileDialogPrivate::setLastVisitedDirectory(const QUrl &dir)
{
    *lastVisitedDir() = dir;
}

void QFileDialogPrivate::updateLookInLabel()
{
    if (options->isLabelExplicitlySet(QFileDialogOptions::LookIn))
        setLabelTextControl(QFileDialog::LookIn, options->labelText(QFileDialogOptions::LookIn));
}

void QFileDialogPrivate::updateFileNameLabel()
{
    if (options->isLabelExplicitlySet(QFileDialogOptions::FileName)) {
        setLabelTextControl(QFileDialog::FileName, options->labelText(QFileDialogOptions::FileName));
    } else {
        switch (q_func()->fileMode()) {
        case QFileDialog::Directory:
            setLabelTextControl(QFileDialog::FileName, QFileDialog::tr("Directory:"));
            break;
        default:
            setLabelTextControl(QFileDialog::FileName, QFileDialog::tr("File &name:"));
            break;
        }
    }
}

void QFileDialogPrivate::updateFileTypeLabel()
{
    if (options->isLabelExplicitlySet(QFileDialogOptions::FileType))
        setLabelTextControl(QFileDialog::FileType, options->labelText(QFileDialogOptions::FileType));
}

void QFileDialogPrivate::updateOkButtonText(bool saveAsOnFolder)
{
    Q_Q(QFileDialog);
    // 'Save as' at a folder: Temporarily change to "Open".
    if (saveAsOnFolder) {
        setLabelTextControl(QFileDialog::Accept, QFileDialog::tr("&Open"));
    } else if (options->isLabelExplicitlySet(QFileDialogOptions::Accept)) {
        setLabelTextControl(QFileDialog::Accept, options->labelText(QFileDialogOptions::Accept));
        return;
    } else {
        switch (q->fileMode()) {
        case QFileDialog::Directory:
            setLabelTextControl(QFileDialog::Accept, QFileDialog::tr("&Choose"));
            break;
        default:
            setLabelTextControl(QFileDialog::Accept,
                                q->acceptMode() == QFileDialog::AcceptOpen ?
                                    QFileDialog::tr("&Open")  :
                                    QFileDialog::tr("&Save"));
            break;
        }
    }
}

void QFileDialogPrivate::updateCancelButtonText()
{
    if (options->isLabelExplicitlySet(QFileDialogOptions::Reject))
        setLabelTextControl(QFileDialog::Reject, options->labelText(QFileDialogOptions::Reject));
}

void QFileDialogPrivate::retranslateStrings()
{
    Q_Q(QFileDialog);
    /* WIDGETS */
    if (options->useDefaultNameFilters())
        q->setNameFilter(QFileDialogOptions::defaultNameFilterString());
    if (!usingWidgets())
        return;

    QList<QAction*> actions = qFileDialogUi->treeView->header()->actions();
    QAbstractItemModel *abstractModel = model;
#if QT_CONFIG(proxymodel)
    if (proxyModel)
        abstractModel = proxyModel;
#endif
    const int total = qMin(abstractModel->columnCount(QModelIndex()), int(actions.size() + 1));
    for (int i = 1; i < total; ++i) {
        actions.at(i - 1)->setText(QFileDialog::tr("Show ") + abstractModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
    }

    /* MENU ACTIONS */
    renameAction->setText(QFileDialog::tr("&Rename"));
    deleteAction->setText(QFileDialog::tr("&Delete"));
    showHiddenAction->setText(QFileDialog::tr("Show &hidden files"));
    newFolderAction->setText(QFileDialog::tr("&New Folder"));
    qFileDialogUi->retranslateUi(q);
    updateLookInLabel();
    updateFileNameLabel();
    updateFileTypeLabel();
    updateCancelButtonText();
}

void QFileDialogPrivate::emitFilesSelected(const QStringList &files)
{
    Q_Q(QFileDialog);
    emit q->filesSelected(files);
    if (files.size() == 1)
        emit q->fileSelected(files.first());
}

bool QFileDialogPrivate::canBeNativeDialog() const
{
    // Don't use Q_Q here! This function is called from ~QDialog,
    // so Q_Q calling q_func() invokes undefined behavior (invalid cast in q_func()).
    const QDialog * const q = static_cast<const QDialog*>(q_ptr);
    if (nativeDialogInUse)
        return true;
    if (QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs)
        || q->testAttribute(Qt::WA_DontShowOnScreen)
        || (options->options() & QFileDialog::DontUseNativeDialog)) {
        return false;
    }

    return strcmp(QFileDialog::staticMetaObject.className(), q->metaObject()->className()) == 0;
}

bool QFileDialogPrivate::usingWidgets() const
{
    return !nativeDialogInUse && qFileDialogUi;
}

/*!
    Sets the given \a option to be enabled if \a on is true; otherwise,
    clears the given \a option.

    Options (particularly the \l DontUseNativeDialog option) should be set
    before changing dialog properties or showing the dialog.

    Setting options while the dialog is visible is not guaranteed to have
    an immediate effect on the dialog (depending on the option and on the
    platform).

    Setting options after changing other properties may cause these
    values to have no effect.

    \sa options, testOption()
*/
void QFileDialog::setOption(Option option, bool on)
{
    const QFileDialog::Options previousOptions = options();
    if (!(previousOptions & option) != !on)
        setOptions(previousOptions ^ option);
}

/*!
    Returns \c true if the given \a option is enabled; otherwise, returns
    false.

    \sa options, setOption()
*/
bool QFileDialog::testOption(Option option) const
{
    Q_D(const QFileDialog);
    return d->options->testOption(static_cast<QFileDialogOptions::FileDialogOption>(option));
}

/*!
    \property QFileDialog::options
    \brief The various options that affect the look and feel of the dialog.

    By default, all options are disabled.

    Options (particularly the \l DontUseNativeDialog option) should be set
    before changing dialog properties or showing the dialog.

    Setting options while the dialog is visible is not guaranteed to have
    an immediate effect on the dialog (depending on the option and on the
    platform).

    Setting options after changing other properties may cause these
    values to have no effect.

    \sa setOption(), testOption()
*/
void QFileDialog::setOptions(Options options)
{
    Q_D(QFileDialog);

    Options changed = (options ^ QFileDialog::options());
    if (!changed)
        return;

    d->options->setOptions(QFileDialogOptions::FileDialogOptions(int(options)));

    if (options & DontUseNativeDialog) {
        d->nativeDialogInUse = false;
        d->createWidgets();
    }

    if (d->usingWidgets()) {
        if (changed & DontResolveSymlinks)
            d->model->setResolveSymlinks(!(options & DontResolveSymlinks));
        if (changed & ReadOnly) {
            bool ro = (options & ReadOnly);
            d->model->setReadOnly(ro);
            d->qFileDialogUi->newFolderButton->setEnabled(!ro);
            d->renameAction->setEnabled(!ro);
            d->deleteAction->setEnabled(!ro);
        }

        if (changed & DontUseCustomDirectoryIcons) {
            QFileIconProvider::Options providerOptions = iconProvider()->options();
            providerOptions.setFlag(QFileIconProvider::DontUseCustomDirectoryIcons,
                                    options & DontUseCustomDirectoryIcons);
            iconProvider()->setOptions(providerOptions);
        }
    }

    if (changed & HideNameFilterDetails)
        setNameFilters(d->options->nameFilters());

    if (changed & ShowDirsOnly)
        setFilter((options & ShowDirsOnly) ? filter() & ~QDir::Files : filter() | QDir::Files);
}

QFileDialog::Options QFileDialog::options() const
{
    Q_D(const QFileDialog);
    static_assert((int)QFileDialog::ShowDirsOnly == (int)QFileDialogOptions::ShowDirsOnly);
    static_assert((int)QFileDialog::DontResolveSymlinks == (int)QFileDialogOptions::DontResolveSymlinks);
    static_assert((int)QFileDialog::DontConfirmOverwrite == (int)QFileDialogOptions::DontConfirmOverwrite);
    static_assert((int)QFileDialog::DontUseNativeDialog == (int)QFileDialogOptions::DontUseNativeDialog);
    static_assert((int)QFileDialog::ReadOnly == (int)QFileDialogOptions::ReadOnly);
    static_assert((int)QFileDialog::HideNameFilterDetails == (int)QFileDialogOptions::HideNameFilterDetails);
    static_assert((int)QFileDialog::DontUseCustomDirectoryIcons == (int)QFileDialogOptions::DontUseCustomDirectoryIcons);
    return QFileDialog::Options(int(d->options->options()));
}

/*!
    This function shows the dialog, and connects the slot specified by \a receiver
    and \a member to the signal that informs about selection changes. If the fileMode is
    ExistingFiles, this is the filesSelected() signal, otherwise it is the fileSelected() signal.

    The signal is disconnected from the slot when the dialog is closed.
*/
void QFileDialog::open(QObject *receiver, const char *member)
{
    Q_D(QFileDialog);
    const char *signal = (fileMode() == ExistingFiles) ? SIGNAL(filesSelected(QStringList))
                                                       : SIGNAL(fileSelected(QString));
    connect(this, signal, receiver, member);
    d->signalToDisconnectOnClose = signal;
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;

    QDialog::open();
}


/*!
    \reimp
*/
void QFileDialog::setVisible(bool visible)
{
    // will call QFileDialogPrivate::setVisible override
    QDialog::setVisible(visible);
}

/*!
    \internal

    The logic has to live here so that the call to hide() in ~QDialog calls
    this function; it wouldn't call an override of QDialog::setVisible().
*/
void QFileDialogPrivate::setVisible(bool visible)
{
    // Don't use Q_Q here! This function is called from ~QDialog,
    // so Q_Q calling q_func() invokes undefined behavior (invalid cast in q_func()).
    const auto q = static_cast<QDialog *>(q_ptr);

    if (canBeNativeDialog()){
        if (setNativeDialogVisible(visible)){
            // Set WA_DontShowOnScreen so that QDialogPrivate::setVisible(visible) below
            // updates the state correctly, but skips showing the non-native version:
            q->setAttribute(Qt::WA_DontShowOnScreen);
#if QT_CONFIG(fscompleter)
            // So the completer doesn't try to complete and therefore show a popup
            if (!nativeDialogInUse)
                completer->setModel(nullptr);
#endif
        } else if (visible) {
            createWidgets();
            q->setAttribute(Qt::WA_DontShowOnScreen, false);
#if QT_CONFIG(fscompleter)
            if (!nativeDialogInUse) {
                if (proxyModel != nullptr)
                    completer->setModel(proxyModel);
                else
                    completer->setModel(model);
            }
#endif
        }
    }

    if (visible && usingWidgets())
        qFileDialogUi->fileNameEdit->setFocus();

    QDialogPrivate::setVisible(visible);
}

/*!
    \internal
    set the directory to url
*/
void QFileDialogPrivate::goToUrl(const QUrl &url)
{
    //The shortcut in the side bar may have a parent that is not fetched yet (e.g. an hidden file)
    //so we force the fetching
    QFileSystemModelPrivate::QFileSystemNode *node = model->d_func()->node(url.toLocalFile(), true);
    QModelIndex idx =  model->d_func()->index(node);
    enterDirectory(idx);
}

/*!
    \fn void QFileDialog::setDirectory(const QDir &directory)

    \overload
*/

/*!
    Sets the file dialog's current \a directory.

    \note On iOS, if you set \a directory to \l{QStandardPaths::standardLocations()}
        {QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).last()},
        a native image picker dialog is used for accessing the user's photo album.
        The filename returned can be loaded using QFile and related APIs.
        For this to be enabled, the Info.plist assigned to QMAKE_INFO_PLIST in the
        project file must contain the key \c NSPhotoLibraryUsageDescription. See
        Info.plist documentation from Apple for more information regarding this key.
        This feature was added in Qt 5.5.
*/
void QFileDialog::setDirectory(const QString &directory)
{
    Q_D(QFileDialog);
    QString newDirectory = directory;
    //we remove .. and . from the given path if exist
    if (!directory.isEmpty())
        newDirectory = QDir::cleanPath(directory);

    if (!directory.isEmpty() && newDirectory.isEmpty())
        return;

    QUrl newDirUrl = QUrl::fromLocalFile(newDirectory);
    QFileDialogPrivate::setLastVisitedDirectory(newDirUrl);

    d->options->setInitialDirectory(QUrl::fromLocalFile(directory));
    if (!d->usingWidgets()) {
        d->setDirectory_sys(newDirUrl);
        return;
    }
    if (d->rootPath() == newDirectory)
        return;
    QModelIndex root = d->model->setRootPath(newDirectory);
    if (!d->nativeDialogInUse) {
        d->qFileDialogUi->newFolderButton->setEnabled(d->model->flags(root) & Qt::ItemIsDropEnabled);
        if (root != d->rootIndex()) {
#if QT_CONFIG(fscompleter)
            if (directory.endsWith(u'/'))
                d->completer->setCompletionPrefix(newDirectory);
            else
                d->completer->setCompletionPrefix(newDirectory + u'/');
#endif
            d->setRootIndex(root);
        }
        d->qFileDialogUi->listView->selectionModel()->clear();
    }
}

/*!
    Returns the directory currently being displayed in the dialog.
*/
QDir QFileDialog::directory() const
{
    Q_D(const QFileDialog);
    if (d->nativeDialogInUse) {
        QString dir = d->directory_sys().toLocalFile();
        return QDir(dir.isEmpty() ? d->options->initialDirectory().toLocalFile() : dir);
    }
    return d->rootPath();
}

/*!
    Sets the file dialog's current \a directory url.

    \note The non-native QFileDialog supports only local files.

    \note On Windows, it is possible to pass URLs representing
    one of the \e {virtual folders}, such as "Computer" or "Network".
    This is done by passing a QUrl using the scheme \c clsid followed
    by the CLSID value with the curly braces removed. For example the URL
    \c clsid:374DE290-123F-4565-9164-39C4925E467B denotes the download
    location. For a complete list of possible values, see the MSDN documentation on
    \l{https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid}{KNOWNFOLDERID}.
    This feature was added in Qt 5.5.

    \sa QUuid
    \since 5.2
*/
void QFileDialog::setDirectoryUrl(const QUrl &directory)
{
    Q_D(QFileDialog);
    if (!directory.isValid())
        return;

    QFileDialogPrivate::setLastVisitedDirectory(directory);
    d->options->setInitialDirectory(directory);

    if (d->nativeDialogInUse)
        d->setDirectory_sys(directory);
    else if (directory.isLocalFile())
        setDirectory(directory.toLocalFile());
    else if (Q_UNLIKELY(d->usingWidgets()))
        qWarning("Non-native QFileDialog supports only local files");
}

/*!
    Returns the url of the directory currently being displayed in the dialog.

    \since 5.2
*/
QUrl QFileDialog::directoryUrl() const
{
    Q_D(const QFileDialog);
    if (d->nativeDialogInUse)
        return d->directory_sys();
    else
        return QUrl::fromLocalFile(directory().absolutePath());
}

// FIXME Qt 5.4: Use upcoming QVolumeInfo class to determine this information?
static inline bool isCaseSensitiveFileSystem(const QString &path)
{
    Q_UNUSED(path);
#if defined(Q_OS_WIN)
    // Return case insensitive unconditionally, even if someone has a case sensitive
    // file system mounted, wrongly capitalized drive letters will cause mismatches.
    return false;
#elif defined(Q_OS_MACOS)
    return pathconf(QFile::encodeName(path).constData(), _PC_CASE_SENSITIVE) == 1;
#else
    return true;
#endif
}

// Determine the file name to be set on the line edit from the path
// passed to selectFile() in mode QFileDialog::AcceptSave.
static inline QString fileFromPath(const QString &rootPath, QString path)
{
    if (!QFileInfo(path).isAbsolute())
        return path;
    if (path.startsWith(rootPath, isCaseSensitiveFileSystem(rootPath) ? Qt::CaseSensitive : Qt::CaseInsensitive))
        path.remove(0, rootPath.size());

    if (path.isEmpty())
        return path;

    if (path.at(0) == QDir::separator()
#ifdef Q_OS_WIN
            //On Windows both cases can happen
            || path.at(0) == u'/'
#endif
            ) {
            path.remove(0, 1);
    }
    return path;
}

/*!
    Selects the given \a filename in the file dialog.

    \sa selectedFiles()
*/
void QFileDialog::selectFile(const QString &filename)
{
    Q_D(QFileDialog);
    if (filename.isEmpty())
        return;

    if (!d->usingWidgets()) {
        QUrl url;
        if (QFileInfo(filename).isRelative()) {
            url = d->options->initialDirectory();
            QString path = url.path();
            if (!path.endsWith(u'/'))
                path += u'/';
            url.setPath(path + filename);
        } else {
            url = QUrl::fromLocalFile(filename);
        }
        d->selectFile_sys(url);
        d->options->setInitiallySelectedFiles(QList<QUrl>() << url);
        return;
    }

    if (!QDir::isRelativePath(filename)) {
        QFileInfo info(filename);
        QString filenamePath = info.absoluteDir().path();

        if (d->model->rootPath() != filenamePath)
            setDirectory(filenamePath);
    }

    QModelIndex index = d->model->index(filename);
    d->qFileDialogUi->listView->selectionModel()->clear();
    if (!isVisible() || !d->lineEdit()->hasFocus())
        d->lineEdit()->setText(index.isValid() ? index.data().toString() : fileFromPath(d->rootPath(), filename));
}

/*!
    Selects the given \a url in the file dialog.

    \note The non-native QFileDialog supports only local files.

    \sa selectedUrls()
    \since 5.2
*/
void QFileDialog::selectUrl(const QUrl &url)
{
    Q_D(QFileDialog);
    if (!url.isValid())
        return;

    if (d->nativeDialogInUse)
        d->selectFile_sys(url);
    else if (url.isLocalFile())
        selectFile(url.toLocalFile());
    else
        qWarning("Non-native QFileDialog supports only local files");
}

#ifdef Q_OS_UNIX
static QString homeDirFromPasswdEntry(const QString &path, const QByteArray &userName)
{
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD) && !defined(Q_OS_WASM)
    passwd pw;
    passwd *tmpPw;
    long bufSize = ::sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufSize == -1)
        bufSize = 1024;
    QVarLengthArray<char, 1024> buf(bufSize);
    int err = 0;
#  if defined(Q_OS_SOLARIS) && (_POSIX_C_SOURCE - 0 < 199506L)
    tmpPw = getpwnam_r(userName.constData(), &pw, buf.data(), buf.size());
#  else
    err = getpwnam_r(userName.constData(), &pw, buf.data(), buf.size(), &tmpPw);
#  endif
    if (err || !tmpPw)
        return path;
    return QFile::decodeName(pw.pw_dir);
#else
    passwd *pw = getpwnam(userName.constData());
    if (!pw)
        return path;
    return QFile::decodeName(pw->pw_dir);
#endif // defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD) && !defined(Q_OS_WASM)
}

Q_AUTOTEST_EXPORT QString qt_tildeExpansion(const QString &path)
{
    if (!path.startsWith(u'~'))
        return path;

    if (path.size() == 1) // '~'
        return QDir::homePath();

    QStringView sv(path);
    const qsizetype sepIndex = sv.indexOf(QDir::separator());
    if (sepIndex == 1) // '~/' or '~/a/b/c'
        return QDir::homePath() + sv.sliced(1);

#if defined(Q_OS_VXWORKS) || defined(Q_OS_INTEGRITY)
    if (sepIndex == -1)
        return QDir::homePath();
    return QDir::homePath() + sv.sliced(sepIndex);
#else
    const qsizetype userNameLen = sepIndex != -1 ? sepIndex - strlen("~") // '~user/a/b'
                                                 : path.size() - strlen("~"); // '~user'
    const QByteArray userName = sv.sliced(1, userNameLen).toLocal8Bit();
    QString homePath = homeDirFromPasswdEntry(path, userName);
    if (sepIndex == -1)
        return homePath;
    return homePath + sv.sliced(sepIndex);
#endif // defined(Q_OS_VXWORKS) || defined(Q_OS_INTEGRITY)
}
#endif

/**
    Returns the text in the line edit which can be one or more file names
  */
QStringList QFileDialogPrivate::typedFiles() const
{
    Q_Q(const QFileDialog);
    QStringList files;
    QString editText = lineEdit()->text();
    if (!editText.contains(u'"')) {
#ifdef Q_OS_UNIX
        const QString prefix = q->directory().absolutePath() + QDir::separator();
        if (QFile::exists(prefix + editText))
            files << editText;
        else
            files << qt_tildeExpansion(editText);
#else
        files << editText;
        Q_UNUSED(q);
#endif
    } else {
        // " is used to separate files like so: "file1" "file2" "file3" ...
        // ### need escape character for filenames with quotes (")
        QStringList tokens = editText.split(u'\"');
        for (int i=0; i<tokens.size(); ++i) {
            if ((i % 2) == 0)
                continue; // Every even token is a separator
#ifdef Q_OS_UNIX
            const QString token = tokens.at(i);
            const QString prefix = q->directory().absolutePath() + QDir::separator();
            if (QFile::exists(prefix + token))
                files << token;
            else
                files << qt_tildeExpansion(token);
#else
            files << toInternal(tokens.at(i));
#endif
        }
    }
    return addDefaultSuffixToFiles(files);
}

// Return selected files without defaulting to the root of the file system model
// used for initializing QFileDialogOptions for native dialogs. The default is
// not suitable for native dialogs since it mostly equals directory().
QList<QUrl> QFileDialogPrivate::userSelectedFiles() const
{
    QList<QUrl> files;

    if (!usingWidgets())
        return addDefaultSuffixToUrls(selectedFiles_sys());

    const QModelIndexList selectedRows = qFileDialogUi->listView->selectionModel()->selectedRows();
    files.reserve(selectedRows.size());
    for (const QModelIndex &index : selectedRows)
        files.append(QUrl::fromLocalFile(index.data(QFileSystemModel::FilePathRole).toString()));

    if (files.isEmpty() && !lineEdit()->text().isEmpty()) {
        const QStringList typedFilesList = typedFiles();
        files.reserve(typedFilesList.size());
        for (const QString &path : typedFilesList)
            files.append(QUrl::fromLocalFile(path));
    }

    return files;
}

QStringList QFileDialogPrivate::addDefaultSuffixToFiles(const QStringList &filesToFix) const
{
    QStringList files;
    for (int i=0; i<filesToFix.size(); ++i) {
        QString name = toInternal(filesToFix.at(i));
        QFileInfo info(name);
        // if the filename has no suffix, add the default suffix
        const QString defaultSuffix = options->defaultSuffix();
        if (!defaultSuffix.isEmpty() && !info.isDir() && !info.fileName().contains(u'.'))
            name += u'.' + defaultSuffix;

        if (info.isAbsolute()) {
            files.append(name);
        } else {
            // at this point the path should only have Qt path separators.
            // This check is needed since we might be at the root directory
            // and on Windows it already ends with slash.
            QString path = rootPath();
            if (!path.endsWith(u'/'))
                path += u'/';
            path += name;
            files.append(path);
        }
    }
    return files;
}

QList<QUrl> QFileDialogPrivate::addDefaultSuffixToUrls(const QList<QUrl> &urlsToFix) const
{
    QList<QUrl> urls;
    urls.reserve(urlsToFix.size());
    // if the filename has no suffix, add the default suffix
    const QString defaultSuffix = options->defaultSuffix();
    for (QUrl url : urlsToFix) {
        if (!defaultSuffix.isEmpty()) {
            const QString urlPath = url.path();
            const auto idx = urlPath.lastIndexOf(u'/');
            if (idx != (urlPath.size() - 1) && !QStringView{urlPath}.mid(idx + 1).contains(u'.'))
                url.setPath(urlPath + u'.' + defaultSuffix);
        }
        urls.append(url);
    }
    return urls;
}


/*!
    Returns a list of strings containing the absolute paths of the
    selected files in the dialog. If no files are selected, or
    the mode is not ExistingFiles or ExistingFile, selectedFiles() contains the current path in the viewport.

    \sa selectedNameFilter(), selectFile()
*/
QStringList QFileDialog::selectedFiles() const
{
    Q_D(const QFileDialog);

    QStringList files;
    const QList<QUrl> userSelectedFiles = d->userSelectedFiles();
    files.reserve(userSelectedFiles.size());
    for (const QUrl &file : userSelectedFiles)
        files.append(file.toString(QUrl::PreferLocalFile));

    if (files.isEmpty() && d->usingWidgets()) {
        const FileMode fm = fileMode();
        if (fm != ExistingFile && fm != ExistingFiles)
            files.append(d->rootIndex().data(QFileSystemModel::FilePathRole).toString());
    }
    return files;
}

/*!
    Returns a list of urls containing the selected files in the dialog.
    If no files are selected, or the mode is not ExistingFiles or
    ExistingFile, selectedUrls() contains the current path in the viewport.

    \sa selectedNameFilter(), selectUrl()
    \since 5.2
*/
QList<QUrl> QFileDialog::selectedUrls() const
{
    Q_D(const QFileDialog);
    if (d->nativeDialogInUse) {
        return d->userSelectedFiles();
    } else {
        QList<QUrl> urls;
        const QStringList selectedFileList = selectedFiles();
        urls.reserve(selectedFileList.size());
        for (const QString &file : selectedFileList)
            urls.append(QUrl::fromLocalFile(file));
        return urls;
    }
}

/*
    Makes a list of filters from ;;-separated text.
    Used by the mac and windows implementations
*/
QStringList qt_make_filter_list(const QString &filter)
{
    if (filter.isEmpty())
        return QStringList();

    auto sep = ";;"_L1;
    if (!filter.contains(sep) && filter.contains(u'\n'))
        sep = "\n"_L1;

    return filter.split(sep);
}

/*!
    Sets the filter used in the file dialog to the given \a filter.

    If \a filter contains a pair of parentheses containing one or more
    filename-wildcard patterns, separated by spaces, then only the
    text contained in the parentheses is used as the filter. This means
    that these calls are all equivalent:

    \snippet code/src_gui_dialogs_qfiledialog.cpp 6

    \note With Android's native file dialog, the mime type matching the given
        name filter is used because only mime types are supported.

    \sa setMimeTypeFilters(), setNameFilters()
*/
void QFileDialog::setNameFilter(const QString &filter)
{
    setNameFilters(qt_make_filter_list(filter));
}


/*
    Strip the filters by removing the details, e.g. (*.*).
*/
QStringList qt_strip_filters(const QStringList &filters)
{
#if QT_CONFIG(regularexpression)
    QStringList strippedFilters;
    static const QRegularExpression r(QString::fromLatin1(QPlatformFileDialogHelper::filterRegExp));
    strippedFilters.reserve(filters.size());
    for (const QString &filter : filters) {
        QString filterName;
        auto match = r.match(filter);
        if (match.hasMatch())
            filterName = match.captured(1);
        strippedFilters.append(filterName.simplified());
    }
    return strippedFilters;
#else
    return filters;
#endif
}


/*!
    Sets the \a filters used in the file dialog.

    Note that the filter \b{*.*} is not portable, because the historical
    assumption that the file extension determines the file type is not
    consistent on every operating system. It is possible to have a file with no
    dot in its name (for example, \c Makefile). In a native Windows file
    dialog, \b{*.*} matches such files, while in other types of file dialogs
    it might not match. So, it's better to use \b{*} if you mean to select any file.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 7

    \l setMimeTypeFilters() has the advantage of providing all possible name
    filters for each file type. For example, JPEG images have three possible
    extensions; if your application can open such files, selecting the
    \c image/jpeg mime type as a filter allows you to open all of them.
*/
void QFileDialog::setNameFilters(const QStringList &filters)
{
    Q_D(QFileDialog);
    QStringList cleanedFilters;
    cleanedFilters.reserve(filters.size());
    for (const QString &filter : filters)
        cleanedFilters << filter.simplified();

    d->options->setNameFilters(cleanedFilters);

    if (!d->usingWidgets())
        return;

    d->qFileDialogUi->fileTypeCombo->clear();
    if (cleanedFilters.isEmpty())
        return;

    if (testOption(HideNameFilterDetails))
        d->qFileDialogUi->fileTypeCombo->addItems(qt_strip_filters(cleanedFilters));
    else
        d->qFileDialogUi->fileTypeCombo->addItems(cleanedFilters);

    d->useNameFilter(0);
}

/*!
    Returns the file type filters that are in operation on this file
    dialog.
*/
QStringList QFileDialog::nameFilters() const
{
    return d_func()->options->nameFilters();
}

/*!
    Sets the current file type \a filter. Multiple filters can be
    passed in \a filter by separating them with semicolons or spaces.

    \sa setNameFilter(), setNameFilters(), selectedNameFilter()
*/
void QFileDialog::selectNameFilter(const QString &filter)
{
    Q_D(QFileDialog);
    d->options->setInitiallySelectedNameFilter(filter);
    if (!d->usingWidgets()) {
        d->selectNameFilter_sys(filter);
        return;
    }
    int i = -1;
    if (testOption(HideNameFilterDetails)) {
        const QStringList filters = qt_strip_filters(qt_make_filter_list(filter));
        if (!filters.isEmpty())
            i = d->qFileDialogUi->fileTypeCombo->findText(filters.first());
    } else {
        i = d->qFileDialogUi->fileTypeCombo->findText(filter);
    }
    if (i >= 0) {
        d->qFileDialogUi->fileTypeCombo->setCurrentIndex(i);
        d->useNameFilter(d->qFileDialogUi->fileTypeCombo->currentIndex());
    }
}

/*!
    Returns the filter that the user selected in the file dialog.

    \sa selectedFiles()
*/
QString QFileDialog::selectedNameFilter() const
{
    Q_D(const QFileDialog);
    if (!d->usingWidgets())
        return d->selectedNameFilter_sys();

    if (testOption(HideNameFilterDetails)) {
        const auto idx = d->qFileDialogUi->fileTypeCombo->currentIndex();
        if (idx >= 0 && idx < d->options->nameFilters().size())
            return d->options->nameFilters().at(d->qFileDialogUi->fileTypeCombo->currentIndex());
    }
    return d->qFileDialogUi->fileTypeCombo->currentText();
}

/*!
    Returns the filter that is used when displaying files.

    \sa setFilter()
*/
QDir::Filters QFileDialog::filter() const
{
    Q_D(const QFileDialog);
    if (d->usingWidgets())
        return d->model->filter();
    return d->options->filter();
}

/*!
    Sets the filter used by the model to \a filters. The filter is used
    to specify the kind of files that should be shown.

    \sa filter()
*/

void QFileDialog::setFilter(QDir::Filters filters)
{
    Q_D(QFileDialog);
    d->options->setFilter(filters);
    if (!d->usingWidgets()) {
        d->setFilter_sys();
        return;
    }

    d->model->setFilter(filters);
    d->showHiddenAction->setChecked((filters & QDir::Hidden));
}

#if QT_CONFIG(mimetype)

static QString nameFilterForMime(const QString &mimeType)
{
    QMimeDatabase db;
    QMimeType mime(db.mimeTypeForName(mimeType));
    if (mime.isValid()) {
        if (mime.isDefault()) {
            return QFileDialog::tr("All files (*)");
        } else {
            const QString patterns = mime.globPatterns().join(u' ');
            return mime.comment() + " ("_L1 + patterns + u')';
        }
    }
    return QString();
}

/*!
    \since 5.2

    Sets the \a filters used in the file dialog, from a list of MIME types.

    Convenience method for setNameFilters().
    Uses QMimeType to create a name filter from the glob patterns and description
    defined in each MIME type.

    Use application/octet-stream for the "All files (*)" filter, since that
    is the base MIME type for all files.

    Calling setMimeTypeFilters overrides any previously set name filters,
    and changes the return value of nameFilters().

    \snippet code/src_gui_dialogs_qfiledialog.cpp 13
*/
void QFileDialog::setMimeTypeFilters(const QStringList &filters)
{
    Q_D(QFileDialog);
    QStringList nameFilters;
    for (const QString &mimeType : filters) {
        const QString text = nameFilterForMime(mimeType);
        if (!text.isEmpty())
            nameFilters.append(text);
    }
    setNameFilters(nameFilters);
    d->options->setMimeTypeFilters(filters);
}

/*!
    \since 5.2

    Returns the MIME type filters that are in operation on this file
    dialog.
*/
QStringList QFileDialog::mimeTypeFilters() const
{
    return d_func()->options->mimeTypeFilters();
}

/*!
    \since 5.2

    Sets the current MIME type \a filter.

*/
void QFileDialog::selectMimeTypeFilter(const QString &filter)
{
    Q_D(QFileDialog);
    d->options->setInitiallySelectedMimeTypeFilter(filter);

    const QString filterForMime = nameFilterForMime(filter);

    if (!d->usingWidgets()) {
        d->selectMimeTypeFilter_sys(filter);
        if (d->selectedMimeTypeFilter_sys().isEmpty() && !filterForMime.isEmpty()) {
            selectNameFilter(filterForMime);
        }
    } else if (!filterForMime.isEmpty()) {
        selectNameFilter(filterForMime);
    }
}

#endif // mimetype

/*!
 * \since 5.9
 * \return The mimetype of the file that the user selected in the file dialog.
 */
QString QFileDialog::selectedMimeTypeFilter() const
{
    Q_D(const QFileDialog);
    QString mimeTypeFilter;
    if (!d->usingWidgets())
        mimeTypeFilter = d->selectedMimeTypeFilter_sys();

#if QT_CONFIG(mimetype)
    if (mimeTypeFilter.isNull() && !d->options->mimeTypeFilters().isEmpty()) {
        const auto nameFilter = selectedNameFilter();
        const auto mimeTypes = d->options->mimeTypeFilters();
        for (const auto &mimeType: mimeTypes) {
            QString filter = nameFilterForMime(mimeType);
            if (testOption(HideNameFilterDetails))
                filter = qt_strip_filters({ filter }).constFirst();
            if (filter == nameFilter) {
                mimeTypeFilter = mimeType;
                break;
            }
        }
    }
#endif

    return mimeTypeFilter;
}

/*!
    \property QFileDialog::viewMode
    \brief The way files and directories are displayed in the dialog.

    By default, the \c Detail mode is used to display information about
    files and directories.

    \sa ViewMode
*/
void QFileDialog::setViewMode(QFileDialog::ViewMode mode)
{
    Q_D(QFileDialog);
    d->options->setViewMode(static_cast<QFileDialogOptions::ViewMode>(mode));
    if (!d->usingWidgets())
        return;
    if (mode == Detail)
        d->showDetailsView();
    else
        d->showListView();
}

QFileDialog::ViewMode QFileDialog::viewMode() const
{
    Q_D(const QFileDialog);
    if (!d->usingWidgets())
        return static_cast<QFileDialog::ViewMode>(d->options->viewMode());
    return (d->qFileDialogUi->stackedWidget->currentWidget() == d->qFileDialogUi->listView->parent() ? QFileDialog::List : QFileDialog::Detail);
}

/*!
    \property QFileDialog::fileMode
    \brief The file mode of the dialog.

    The file mode defines the number and type of items that the user is
    expected to select in the dialog.

    By default, this property is set to AnyFile.

    This function sets the labels for the FileName and
    \l{QFileDialog::}{Accept} \l{DialogLabel}s. It is possible to set
    custom text after the call to setFileMode().

    \sa FileMode
*/
void QFileDialog::setFileMode(QFileDialog::FileMode mode)
{
    Q_D(QFileDialog);
    d->options->setFileMode(static_cast<QFileDialogOptions::FileMode>(mode));
    if (!d->usingWidgets())
        return;

    d->retranslateWindowTitle();

    // set selection mode and behavior
    QAbstractItemView::SelectionMode selectionMode;
    if (mode == QFileDialog::ExistingFiles)
        selectionMode = QAbstractItemView::ExtendedSelection;
    else
        selectionMode = QAbstractItemView::SingleSelection;
    d->qFileDialogUi->listView->setSelectionMode(selectionMode);
    d->qFileDialogUi->treeView->setSelectionMode(selectionMode);
    // set filter
    d->model->setFilter(d->filterForMode(filter()));
    // setup file type for directory
    if (mode == Directory) {
        d->qFileDialogUi->fileTypeCombo->clear();
        d->qFileDialogUi->fileTypeCombo->addItem(tr("Directories"));
        d->qFileDialogUi->fileTypeCombo->setEnabled(false);
    }
    d->updateFileNameLabel();
    d->updateOkButtonText();
    d->qFileDialogUi->fileTypeCombo->setEnabled(!testOption(ShowDirsOnly));
    d->updateOkButton();
}

QFileDialog::FileMode QFileDialog::fileMode() const
{
    Q_D(const QFileDialog);
    return static_cast<FileMode>(d->options->fileMode());
}

/*!
    \property QFileDialog::acceptMode
    \brief The accept mode of the dialog.

    The action mode defines whether the dialog is for opening or saving files.

    By default, this property is set to \l{AcceptOpen}.

    \sa AcceptMode
*/
void QFileDialog::setAcceptMode(QFileDialog::AcceptMode mode)
{
    Q_D(QFileDialog);
    d->options->setAcceptMode(static_cast<QFileDialogOptions::AcceptMode>(mode));
    // clear WA_DontShowOnScreen so that d->canBeNativeDialog() doesn't return false incorrectly
    setAttribute(Qt::WA_DontShowOnScreen, false);
    if (!d->usingWidgets())
        return;
    QDialogButtonBox::StandardButton button = (mode == AcceptOpen ? QDialogButtonBox::Open : QDialogButtonBox::Save);
    d->qFileDialogUi->buttonBox->setStandardButtons(button | QDialogButtonBox::Cancel);
    d->qFileDialogUi->buttonBox->button(button)->setEnabled(false);
    d->updateOkButton();
    if (mode == AcceptSave) {
        d->qFileDialogUi->lookInCombo->setEditable(false);
    }
    d->retranslateWindowTitle();
}

/*!
    \property QFileDialog::supportedSchemes
    \brief The URL schemes that the file dialog should allow navigating to.
    \since 5.6

    Setting this property allows to restrict the type of URLs the
    user can select. It is a way for the application to declare
    the protocols it supports to fetch the file content. An empty list
    means that no restriction is applied (the default).
    Support for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.
*/

void QFileDialog::setSupportedSchemes(const QStringList &schemes)
{
    Q_D(QFileDialog);
    d->options->setSupportedSchemes(schemes);
}

QStringList QFileDialog::supportedSchemes() const
{
    return d_func()->options->supportedSchemes();
}

/*
    Returns the file system model index that is the root index in the
    views
*/
QModelIndex QFileDialogPrivate::rootIndex() const {
    return mapToSource(qFileDialogUi->listView->rootIndex());
}

QAbstractItemView *QFileDialogPrivate::currentView() const {
    if (!qFileDialogUi->stackedWidget)
        return nullptr;
    if (qFileDialogUi->stackedWidget->currentWidget() == qFileDialogUi->listView->parent())
        return qFileDialogUi->listView;
    return qFileDialogUi->treeView;
}

QLineEdit *QFileDialogPrivate::lineEdit() const {
    return (QLineEdit*)qFileDialogUi->fileNameEdit;
}

long QFileDialogPrivate::maxNameLength(const QString &path)
{
#if defined(Q_OS_UNIX)
    return ::pathconf(QFile::encodeName(path).data(), _PC_NAME_MAX);
#elif defined(Q_OS_WIN)
    DWORD maxLength;
    const QString drive = path.left(3);
    if (::GetVolumeInformation(reinterpret_cast<const wchar_t *>(drive.utf16()), NULL, 0, NULL, &maxLength, NULL, NULL, 0) == false)
        return -1;
    return maxLength;
#else
    Q_UNUSED(path);
#endif
    return -1;
}

/*
    Sets the view root index to be the file system model index
*/
void QFileDialogPrivate::setRootIndex(const QModelIndex &index) const {
    Q_ASSERT(index.isValid() ? index.model() == model : true);
    QModelIndex idx = mapFromSource(index);
    qFileDialogUi->treeView->setRootIndex(idx);
    qFileDialogUi->listView->setRootIndex(idx);
}
/*
    Select a file system model index
    returns the index that was selected (or not depending upon sortfilterproxymodel)
*/
QModelIndex QFileDialogPrivate::select(const QModelIndex &index) const {
    Q_ASSERT(index.isValid() ? index.model() == model : true);

    QModelIndex idx = mapFromSource(index);
    if (idx.isValid() && !qFileDialogUi->listView->selectionModel()->isSelected(idx))
        qFileDialogUi->listView->selectionModel()->select(idx,
            QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return idx;
}

QFileDialog::AcceptMode QFileDialog::acceptMode() const
{
    Q_D(const QFileDialog);
    return static_cast<AcceptMode>(d->options->acceptMode());
}

/*!
    \property QFileDialog::defaultSuffix
    \brief Suffix added to the filename if no other suffix was specified.

    This property specifies a string that is added to the
    filename if it has no suffix yet. The suffix is typically
    used to indicate the file type (e.g. "txt" indicates a text
    file).

    If the first character is a dot ('.'), it is removed.
*/
void QFileDialog::setDefaultSuffix(const QString &suffix)
{
    Q_D(QFileDialog);
    d->options->setDefaultSuffix(suffix);
}

QString QFileDialog::defaultSuffix() const
{
    Q_D(const QFileDialog);
    return d->options->defaultSuffix();
}

/*!
    Sets the browsing history of the filedialog to contain the given
    \a paths.
*/
void QFileDialog::setHistory(const QStringList &paths)
{
    Q_D(QFileDialog);
    if (d->usingWidgets())
        d->qFileDialogUi->lookInCombo->setHistory(paths);
}

void QFileDialogComboBox::setHistory(const QStringList &paths)
{
    m_history = paths;
    // Only populate the first item, showPopup will populate the rest if needed
    QList<QUrl> list;
    const QModelIndex idx = d_ptr->model->index(d_ptr->rootPath());
    //On windows the popup display the "C:\", convert to nativeSeparators
    const QUrl url = idx.isValid()
                   ? QUrl::fromLocalFile(QDir::toNativeSeparators(idx.data(QFileSystemModel::FilePathRole).toString()))
                   : QUrl("file:"_L1);
    if (url.isValid())
        list.append(url);
    urlModel->setUrls(list);
}

/*!
    Returns the browsing history of the filedialog as a list of paths.
*/
QStringList QFileDialog::history() const
{
    Q_D(const QFileDialog);
    if (!d->usingWidgets())
        return QStringList();
    QStringList currentHistory = d->qFileDialogUi->lookInCombo->history();
    //On windows the popup display the "C:\", convert to nativeSeparators
    QString newHistory = QDir::toNativeSeparators(d->rootIndex().data(QFileSystemModel::FilePathRole).toString());
    if (!currentHistory.contains(newHistory))
        currentHistory << newHistory;
    return currentHistory;
}

/*!
    Sets the item delegate used to render items in the views in the
    file dialog to the given \a delegate.

    Any existing delegate will be removed, but not deleted. QFileDialog
    does not take ownership of \a delegate.

    \warning You should not share the same instance of a delegate between views.
    Doing so can cause incorrect or unintuitive editing behavior since each
    view connected to a given delegate may receive the \l{QAbstractItemDelegate::}{closeEditor()}
    signal, and attempt to access, modify or close an editor that has already been closed.

    Note that the model used is QFileSystemModel. It has custom item data roles, which is
    described by the \l{QFileSystemModel::}{Roles} enum. You can use a QFileIconProvider if
    you only want custom icons.

    \sa itemDelegate(), setIconProvider(), QFileSystemModel
*/
void QFileDialog::setItemDelegate(QAbstractItemDelegate *delegate)
{
    Q_D(QFileDialog);
    if (!d->usingWidgets())
        return;
    d->qFileDialogUi->listView->setItemDelegate(delegate);
    d->qFileDialogUi->treeView->setItemDelegate(delegate);
}

/*!
  Returns the item delegate used to render the items in the views in the filedialog.
*/
QAbstractItemDelegate *QFileDialog::itemDelegate() const
{
    Q_D(const QFileDialog);
    if (!d->usingWidgets())
        return nullptr;
    return d->qFileDialogUi->listView->itemDelegate();
}

/*!
    Sets the icon provider used by the filedialog to the specified \a provider.
*/
void QFileDialog::setIconProvider(QAbstractFileIconProvider *provider)
{
    Q_D(QFileDialog);
    if (!d->usingWidgets())
        return;
    d->model->setIconProvider(provider);
    //It forces the refresh of all entries in the side bar, then we can get new icons
    d->qFileDialogUi->sidebar->setUrls(d->qFileDialogUi->sidebar->urls());
}

/*!
    Returns the icon provider used by the filedialog.
*/
QAbstractFileIconProvider *QFileDialog::iconProvider() const
{
    Q_D(const QFileDialog);
    if (!d->model)
        return nullptr;
    return d->model->iconProvider();
}

void QFileDialogPrivate::setLabelTextControl(QFileDialog::DialogLabel label, const QString &text)
{
    if (!qFileDialogUi)
        return;
    switch (label) {
    case QFileDialog::LookIn:
        qFileDialogUi->lookInLabel->setText(text);
        break;
    case QFileDialog::FileName:
        qFileDialogUi->fileNameLabel->setText(text);
        break;
    case QFileDialog::FileType:
        qFileDialogUi->fileTypeLabel->setText(text);
        break;
    case QFileDialog::Accept:
        if (q_func()->acceptMode() == QFileDialog::AcceptOpen) {
            if (QPushButton *button = qFileDialogUi->buttonBox->button(QDialogButtonBox::Open))
                button->setText(text);
        } else {
            if (QPushButton *button = qFileDialogUi->buttonBox->button(QDialogButtonBox::Save))
                button->setText(text);
        }
        break;
    case QFileDialog::Reject:
        if (QPushButton *button = qFileDialogUi->buttonBox->button(QDialogButtonBox::Cancel))
            button->setText(text);
        break;
    }
}

/*!
    Sets the \a text shown in the filedialog in the specified \a label.
*/

void QFileDialog::setLabelText(DialogLabel label, const QString &text)
{
    Q_D(QFileDialog);
    d->options->setLabelText(static_cast<QFileDialogOptions::DialogLabel>(label), text);
    d->setLabelTextControl(label, text);
}

/*!
    Returns the text shown in the filedialog in the specified \a label.
*/
QString QFileDialog::labelText(DialogLabel label) const
{
    Q_D(const QFileDialog);
    if (!d->usingWidgets())
        return d->options->labelText(static_cast<QFileDialogOptions::DialogLabel>(label));
    QPushButton *button;
    switch (label) {
    case LookIn:
        return d->qFileDialogUi->lookInLabel->text();
    case FileName:
        return d->qFileDialogUi->fileNameLabel->text();
    case FileType:
        return d->qFileDialogUi->fileTypeLabel->text();
    case Accept:
        if (acceptMode() == AcceptOpen)
            button = d->qFileDialogUi->buttonBox->button(QDialogButtonBox::Open);
        else
            button = d->qFileDialogUi->buttonBox->button(QDialogButtonBox::Save);
        if (button)
            return button->text();
        break;
    case Reject:
        button = d->qFileDialogUi->buttonBox->button(QDialogButtonBox::Cancel);
        if (button)
            return button->text();
        break;
    }
    return QString();
}

/*!
    This is a convenience static function that returns an existing file
    selected by the user. If the user presses Cancel, it returns a null string.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 8

    The function creates a modal file dialog with the given \a parent widget.
    If \a parent is not \nullptr, the dialog is shown centered over the
    parent widget.

    The file dialog's working directory is set to \a dir. If \a dir
    includes a file name, the file is selected. Only files that match the
    given \a filter are shown. The selected filter is set to \a selectedFilter.
    The parameters \a dir, \a selectedFilter, and \a filter may be empty
    strings. If you want multiple filters, separate them with ';;', for
    example:

    \snippet code/src_gui_dialogs_qfiledialog.cpp 14

    The \a options argument holds various options about how to run the dialog.
    See the QFileDialog::Option enum for more information on the flags you can
    pass.

    The dialog's caption is set to \a caption. If \a caption is not specified,
    then a default caption will be used.

    On Windows, and \macos, this static function uses the
    native file dialog and not a QFileDialog. Note that the \macos native file
    dialog does not show a title bar.

    On Windows the dialog spins a blocking modal event loop that does not
    dispatch any QTimers, and if \a parent is not \nullptr then it positions
    the dialog just below the parent's title bar.

    On Unix/X11, the normal behavior of the file dialog is to resolve and
    follow symlinks. For example, if \c{/usr/tmp} is a symlink to \c{/var/tmp},
    the file dialog changes to \c{/var/tmp} after entering \c{/usr/tmp}. If
    \a options includes DontResolveSymlinks, the file dialog treats
    symlinks as regular directories.

    \sa getOpenFileNames(), getSaveFileName(), getExistingDirectory()
*/
QString QFileDialog::getOpenFileName(QWidget *parent,
                               const QString &caption,
                               const QString &dir,
                               const QString &filter,
                               QString *selectedFilter,
                               Options options)
{
    const QStringList schemes = QStringList(QStringLiteral("file"));
    const QUrl selectedUrl = getOpenFileUrl(parent, caption, QUrl::fromLocalFile(dir), filter,
                                            selectedFilter, options, schemes);
    if (selectedUrl.isLocalFile() || selectedUrl.isEmpty())
        return selectedUrl.toLocalFile();
    else
        return selectedUrl.toString();
}

/*!
    This is a convenience static function that returns an existing file
    selected by the user. If the user presses Cancel, it returns an
    empty url.

    The function is used similarly to QFileDialog::getOpenFileName(). In
    particular \a parent, \a caption, \a dir, \a filter, \a selectedFilter
    and \a options are used in exactly the same way.

    The main difference with QFileDialog::getOpenFileName() comes from
    the ability offered to the user to select a remote file. That's why
    the return type and the type of \a dir is QUrl.

    The \a supportedSchemes argument allows to restrict the type of URLs the
    user is able to select. It is a way for the application to declare
    the protocols it will support to fetch the file content. An empty list
    means that no restriction is applied (the default).
    Support for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.

    When possible, this static function uses the native file dialog and
    not a QFileDialog. On platforms that don't support selecting remote
    files, Qt will allow to select only local files.

    \sa getOpenFileName(), getOpenFileUrls(), getSaveFileUrl(), getExistingDirectoryUrl()
    \since 5.2
*/
QUrl QFileDialog::getOpenFileUrl(QWidget *parent,
                                 const QString &caption,
                                 const QUrl &dir,
                                 const QString &filter,
                                 QString *selectedFilter,
                                 Options options,
                                 const QStringList &supportedSchemes)
{
    QFileDialogArgs args(dir);
    args.parent = parent;
    args.caption = caption;
    args.filter = filter;
    args.mode = ExistingFile;
    args.options = options;

    QAutoPointer<QFileDialog> dialog(new QFileDialog(args));
    dialog->setSupportedSchemes(supportedSchemes);
    if (selectedFilter && !selectedFilter->isEmpty())
        dialog->selectNameFilter(*selectedFilter);
    const int execResult = dialog->exec();
    if (bool(dialog) && execResult == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog->selectedNameFilter();
        return dialog->selectedUrls().value(0);
    }
    return QUrl();
}

/*!
    This is a convenience static function that returns one or more existing
    files selected by the user.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 9

    This function creates a modal file dialog with the given \a parent widget.
    If \a parent is not \nullptr, the dialog is shown centered over the
    parent widget.

    The file dialog's working directory is set to \a dir. If \a dir
    includes a file name, the file is selected. The filter is set to
    \a filter so that only those files which match the filter are shown. The
    filter selected is set to \a selectedFilter. The parameters \a dir,
    \a selectedFilter and \a filter can be empty strings. If you need multiple
    filters, separate them with ';;', for instance:

    \snippet code/src_gui_dialogs_qfiledialog.cpp 14

    The dialog's caption is set to \a caption. If \a caption is not specified,
    then a default caption is used.

    On Windows and \macos, this static function uses the
    native file dialog and not a QFileDialog. Note that the \macos native file
    dialog does not show a title bar.

    On Windows the dialog spins a blocking modal event loop that does not
    dispatch any QTimers, and if \a parent is not \nullptr then it positions
    the dialog just below the parent's title bar.

    On Unix/X11, the normal behavior of the file dialog is to resolve and
    follow symlinks. For example, if \c{/usr/tmp} is a symlink to \c{/var/tmp},
    the file dialog will change to \c{/var/tmp} after entering \c{/usr/tmp}.
    The \a options argument holds various options about how to run the dialog,
    see the QFileDialog::Option enum for more information on the flags you can
    pass.

    \sa getOpenFileName(), getSaveFileName(), getExistingDirectory()
*/
QStringList QFileDialog::getOpenFileNames(QWidget *parent,
                                          const QString &caption,
                                          const QString &dir,
                                          const QString &filter,
                                          QString *selectedFilter,
                                          Options options)
{
    const QStringList schemes = QStringList(QStringLiteral("file"));
    const QList<QUrl> selectedUrls = getOpenFileUrls(parent, caption, QUrl::fromLocalFile(dir),
                                                     filter, selectedFilter, options, schemes);
    QStringList fileNames;
    fileNames.reserve(selectedUrls.size());
    for (const QUrl &url : selectedUrls)
        fileNames.append(url.toString(QUrl::PreferLocalFile));
    return fileNames;
}

/*!
    This is a convenience static function that returns one or more existing
    files selected by the user. If the user presses Cancel, it returns an
    empty list.

    The function is used similarly to QFileDialog::getOpenFileNames(). In
    particular \a parent, \a caption, \a dir, \a filter, \a selectedFilter
    and \a options are used in exactly the same way.

    The main difference with QFileDialog::getOpenFileNames() comes from
    the ability offered to the user to select remote files. That's why
    the return type and the type of \a dir are respectively QList<QUrl>
    and QUrl.

    The \a supportedSchemes argument allows to restrict the type of URLs the
    user can select. It is a way for the application to declare
    the protocols it supports to fetch the file content. An empty list
    means that no restriction is applied (the default).
    Support for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.

    When possible, this static function uses the native file dialog and
    not a QFileDialog. On platforms that don't support selecting remote
    files, Qt will allow to select only local files.

    \sa getOpenFileNames(), getOpenFileUrl(), getSaveFileUrl(), getExistingDirectoryUrl()
    \since 5.2
*/
QList<QUrl> QFileDialog::getOpenFileUrls(QWidget *parent,
                                         const QString &caption,
                                         const QUrl &dir,
                                         const QString &filter,
                                         QString *selectedFilter,
                                         Options options,
                                         const QStringList &supportedSchemes)
{
    QFileDialogArgs args(dir);
    args.parent = parent;
    args.caption = caption;
    args.filter = filter;
    args.mode = ExistingFiles;
    args.options = options;

    QAutoPointer<QFileDialog> dialog(new QFileDialog(args));
    dialog->setSupportedSchemes(supportedSchemes);
    if (selectedFilter && !selectedFilter->isEmpty())
        dialog->selectNameFilter(*selectedFilter);
    const int execResult = dialog->exec();
    if (bool(dialog) && execResult == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog->selectedNameFilter();
        return dialog->selectedUrls();
    }
    return QList<QUrl>();
}

/*!
    This is a convenience static function that returns the content of a file
    selected by the user.

    Use this function to access local files on Qt for WebAssembly, if the web sandbox
    restricts file access. Its implementation enables displaying a native file dialog in
    the browser, where the user selects a file based on the \a nameFilter parameter.

    \a parent is ignored on Qt for WebAssembly. Pass \a parent on other platforms, to make
    the popup a child of another widget. If the platform doesn't support native file
    dialogs, the function falls back to QFileDialog.

    The function is asynchronous and returns immediately. The \a fileOpenCompleted
    callback will be called when a file has been selected and its contents have been
    read into memory.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 15
    \since 5.13
*/
void QFileDialog::getOpenFileContent(const QString &nameFilter, const std::function<void(const QString &, const QByteArray &)> &fileOpenCompleted, QWidget *parent)
{
#ifdef Q_OS_WASM
    Q_UNUSED(parent);
    auto openFileImpl = std::make_shared<std::function<void(void)>>();
    QString fileName;
    QByteArray fileContent;
    *openFileImpl = [=]() mutable {
        auto fileDialogClosed = [&](bool fileSelected) {
            if (!fileSelected) {
                fileOpenCompleted(fileName, fileContent);
                openFileImpl.reset();
            }
        };
        auto acceptFile = [&](uint64_t size, const std::string name) -> char * {
            const uint64_t twoGB = 1ULL << 31; // QByteArray limit
            if (size > twoGB)
                return nullptr;

            fileName = QString::fromStdString(name);
            fileContent.resize(size);
            return fileContent.data();
        };
        auto fileContentReady = [&]() mutable {
            fileOpenCompleted(fileName, fileContent);
            openFileImpl.reset();
        };

        QWasmLocalFileAccess::openFile(nameFilter.toStdString(), fileDialogClosed, acceptFile, fileContentReady);
    };

    (*openFileImpl)();
#else
    QFileDialog *dialog = new QFileDialog(parent);
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setNameFilter(nameFilter);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    auto fileSelected = [=](const QString &fileName) {
        QByteArray fileContent;
        if (!fileName.isNull()) {
            QFile selectedFile(fileName);
            if (selectedFile.open(QIODevice::ReadOnly))
                fileContent = selectedFile.readAll();
        }
        fileOpenCompleted(fileName, fileContent);
    };

    connect(dialog, &QFileDialog::fileSelected, dialog, fileSelected);
    dialog->open();
#endif
}

/*!
    This is a convenience static function that saves \a fileContent to a file, using
    a file name and location chosen by the user. \a fileNameHint can be provided to
    suggest a file name to the user.

    Use this function to save content to local files on Qt for WebAssembly, if the web sandbox
    restricts file access. Its implementation enables displaying a native file dialog in the
    browser, where the user specifies an output file based on the \a fileNameHint argument.

    \a parent is ignored on Qt for WebAssembly. Pass \a parent on other platforms, to make
    the popup a child of another widget. If the platform doesn't support native file
    dialogs, the function falls back to QFileDialog.

    The function is asynchronous and returns immediately.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 16
    \since 5.14
*/
void QFileDialog::saveFileContent(const QByteArray &fileContent, const QString &fileNameHint, QWidget *parent)
{
#ifdef Q_OS_WASM
    Q_UNUSED(parent);
    QWasmLocalFileAccess::saveFile(fileContent, fileNameHint.toStdString());
#else
    QFileDialog *dialog = new QFileDialog(parent);
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->setFileMode(QFileDialog::AnyFile);
    dialog->selectFile(fileNameHint);

    auto fileSelected = [=](const QString &fileName) {
        if (!fileName.isNull()) {
            QFile selectedFile(fileName);
            if (selectedFile.open(QIODevice::WriteOnly))
                selectedFile.write(fileContent);
        }
    };

    connect(dialog, &QFileDialog::fileSelected, dialog, fileSelected);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
#endif
}

/*!
    This is a convenience static function that returns a file name selected
    by the user. The file does not have to exist.

    It creates a modal file dialog with the given \a parent widget. If
    \a parent is not \nullptr, the dialog will be shown centered over the
    parent widget.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 11

    The file dialog's working directory is set to \a dir. If \a dir
    includes a file name, the file is selected. Only files that match the
    \a filter are shown. The filter selected is set to \a selectedFilter. The
    parameters \a dir, \a selectedFilter, and \a filter may be empty strings.
    Multiple filters are separated with ';;'. For instance:

    \snippet code/src_gui_dialogs_qfiledialog.cpp 14

    The \a options argument holds various options about how to run the dialog,
    see the QFileDialog::Option enum for more information on the flags you can
    pass.

    The default filter can be chosen by setting \a selectedFilter to the
    desired value.

    The dialog's caption is set to \a caption. If \a caption is not specified,
    a default caption is used.

    On Windows, and \macos, this static function uses the
    native file dialog and not a QFileDialog.

    On Windows the dialog spins a blocking modal event loop that does not
    dispatch any QTimers, and if \a parent is not \nullptr then it
    positions the dialog just below the parent's title bar. On \macos, with its
    native file dialog, the filter argument is ignored.

    On Unix/X11, the normal behavior of the file dialog is to resolve and
    follow symlinks. For example, if \c{/usr/tmp} is a symlink to \c{/var/tmp},
    the file dialog changes to \c{/var/tmp} after entering \c{/usr/tmp}. If
    \a options includes DontResolveSymlinks, the file dialog treats symlinks
    as regular directories.

    \sa getOpenFileName(), getOpenFileNames(), getExistingDirectory()
*/
QString QFileDialog::getSaveFileName(QWidget *parent,
                                     const QString &caption,
                                     const QString &dir,
                                     const QString &filter,
                                     QString *selectedFilter,
                                     Options options)
{
    const QStringList schemes = QStringList(QStringLiteral("file"));
    const QUrl selectedUrl = getSaveFileUrl(parent, caption, QUrl::fromLocalFile(dir), filter,
                                            selectedFilter, options, schemes);
    if (selectedUrl.isLocalFile() || selectedUrl.isEmpty())
        return selectedUrl.toLocalFile();
    else
        return selectedUrl.toString();
}

/*!
    This is a convenience static function that returns a file selected by
    the user. The file does not have to exist. If the user presses Cancel,
    it returns an empty url.

    The function is used similarly to QFileDialog::getSaveFileName(). In
    particular \a parent, \a caption, \a dir, \a filter, \a selectedFilter
    and \a options are used in exactly the same way.

    The main difference with QFileDialog::getSaveFileName() comes from
    the ability offered to the user to select a remote file. That's why
    the return type and the type of \a dir is QUrl.

    The \a supportedSchemes argument allows to restrict the type of URLs the
    user can select. It is a way for the application to declare
    the protocols it supports to save the file content. An empty list
    means that no restriction is applied (the default).
    Support for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.

    When possible, this static function uses the native file dialog and
    not a QFileDialog. On platforms that don't support selecting remote
    files, Qt will allow to select only local files.

    \sa getSaveFileName(), getOpenFileUrl(), getOpenFileUrls(), getExistingDirectoryUrl()
    \since 5.2
*/
QUrl QFileDialog::getSaveFileUrl(QWidget *parent,
                                 const QString &caption,
                                 const QUrl &dir,
                                 const QString &filter,
                                 QString *selectedFilter,
                                 Options options,
                                 const QStringList &supportedSchemes)
{
    QFileDialogArgs args(dir);
    args.parent = parent;
    args.caption = caption;
    args.filter = filter;
    args.mode = AnyFile;
    args.options = options;

    QAutoPointer<QFileDialog> dialog(new QFileDialog(args));
    dialog->setSupportedSchemes(supportedSchemes);
    dialog->setAcceptMode(AcceptSave);
    if (selectedFilter && !selectedFilter->isEmpty())
        dialog->selectNameFilter(*selectedFilter);
    const int execResult = dialog->exec();
    if (bool(dialog) && execResult == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog->selectedNameFilter();
        return dialog->selectedUrls().value(0);
    }
    return QUrl();
}

/*!
    This is a convenience static function that returns an existing
    directory selected by the user.

    \snippet code/src_gui_dialogs_qfiledialog.cpp 12

    This function creates a modal file dialog with the given \a parent widget.
    If \a parent is not \nullptr, the dialog is shown centered over the
    parent widget.

    The dialog's working directory is set to \a dir, and the caption is set to
    \a caption. Either of these can be an empty string in which case the
    current directory and a default caption are used respectively.

    The \a options argument holds various options about how to run the dialog.
    See the QFileDialog::Option enum for more information on the flags you can
    pass. To ensure a native file dialog, \l{QFileDialog::}{ShowDirsOnly} must
    be set.

    On Windows and \macos, this static function uses the
    native file dialog and not a QFileDialog. However, the native Windows file
    dialog does not support displaying files in the directory chooser. You need
    to pass the \l{QFileDialog::}{DontUseNativeDialog} option, or set the global
    \l{Qt::}{AA_DontUseNativeDialogs} application attribute to display files using a
    QFileDialog.

    Note that the \macos native file dialog does not show a title bar.

    On Unix/X11, the normal behavior of the file dialog is to resolve and
    follow symlinks. For example, if \c{/usr/tmp} is a symlink to \c{/var/tmp},
    the file dialog changes to \c{/var/tmp} after entering \c{/usr/tmp}. If
    \a options includes DontResolveSymlinks, the file dialog treats
    symlinks as regular directories.

    On Windows, the dialog spins a blocking modal event loop that does not
    dispatch any QTimers, and if \a parent is not \nullptr then it positions
    the dialog just below the parent's title bar.

    \sa getOpenFileName(), getOpenFileNames(), getSaveFileName()
*/
QString QFileDialog::getExistingDirectory(QWidget *parent,
                                          const QString &caption,
                                          const QString &dir,
                                          Options options)
{
    const QStringList schemes = QStringList(QStringLiteral("file"));
    const QUrl selectedUrl =
            getExistingDirectoryUrl(parent, caption, QUrl::fromLocalFile(dir), options, schemes);
    if (selectedUrl.isLocalFile() || selectedUrl.isEmpty())
        return selectedUrl.toLocalFile();
    else
        return selectedUrl.toString();
}

/*!
    This is a convenience static function that returns an existing
    directory selected by the user. If the user presses Cancel, it
    returns an empty url.

    The function is used similarly to QFileDialog::getExistingDirectory().
    In particular \a parent, \a caption, \a dir and \a options are used
    in exactly the same way.

    The main difference with QFileDialog::getExistingDirectory() comes from
    the ability offered to the user to select a remote directory. That's why
    the return type and the type of \a dir is QUrl.

    The \a supportedSchemes argument allows to restrict the type of URLs the
    user is able to select. It is a way for the application to declare
    the protocols it supports to fetch the file content. An empty list
    means that no restriction is applied (the default).
    Support for local files ("file" scheme) is implicit and always enabled;
    it is not necessary to include it in the restriction.

    When possible, this static function uses the native file dialog and
    not a QFileDialog. On platforms that don't support selecting remote
    files, Qt allows to select only local files.

    \sa getExistingDirectory(), getOpenFileUrl(), getOpenFileUrls(), getSaveFileUrl()
    \since 5.2
*/
QUrl QFileDialog::getExistingDirectoryUrl(QWidget *parent,
                                          const QString &caption,
                                          const QUrl &dir,
                                          Options options,
                                          const QStringList &supportedSchemes)
{
    QFileDialogArgs args(dir);
    args.parent = parent;
    args.caption = caption;
    args.mode = Directory;
    args.options = options;

    QAutoPointer<QFileDialog> dialog(new QFileDialog(args));
    dialog->setSupportedSchemes(supportedSchemes);
    const int execResult = dialog->exec();
    if (bool(dialog) && execResult == QDialog::Accepted)
        return dialog->selectedUrls().value(0);
    return QUrl();
}

inline static QUrl _qt_get_directory(const QUrl &url, const QFileInfo &local)
{
    if (url.isLocalFile()) {
        QFileInfo info = local;
        if (!local.isAbsolute())
            info = QFileInfo(QDir::current(), url.toLocalFile());
        const QFileInfo pathInfo(info.absolutePath());
        if (!pathInfo.exists() || !pathInfo.isDir())
            return QUrl();
        if (info.exists() && info.isDir())
            return QUrl::fromLocalFile(QDir::cleanPath(info.absoluteFilePath()));
        return QUrl::fromLocalFile(pathInfo.absoluteFilePath());
    } else {
        return url;
    }
}

inline static void _qt_init_lastVisited() {
#if QT_CONFIG(settings)
    if (lastVisitedDir()->isEmpty()) {
        QSettings settings(QSettings::UserScope, u"QtProject"_s);
        const QString &lastVisisted = settings.value("FileDialog/lastVisited", QString()).toString();
        *lastVisitedDir() = QUrl::fromLocalFile(lastVisisted);
    }
#endif
}

/*
    Initialize working directory and selection from \a url.
*/
QFileDialogArgs::QFileDialogArgs(const QUrl &url)
{
    // default case, re-use QFileInfo to avoid stat'ing
    const QFileInfo local(url.toLocalFile());
    // Get the initial directory URL
    if (!url.isEmpty())
        directory = _qt_get_directory(url, local);
    if (directory.isEmpty()) {
        _qt_init_lastVisited();
        const QUrl lastVisited = *lastVisitedDir();
        if (lastVisited != url)
            directory = _qt_get_directory(lastVisited, QFileInfo());
    }
    if (directory.isEmpty())
        directory = QUrl::fromLocalFile(QDir::currentPath());

    /*
    The initial directory can contain both the initial directory
    and initial selection, e.g. /home/user/foo.txt
    */
    if (selection.isEmpty() && !url.isEmpty()) {
        if (url.isLocalFile()) {
            if (!local.isDir())
                selection = local.fileName();
        } else {
            // With remote URLs we can only assume.
            selection = url.fileName();
        }
    }
}

/*!
 \reimp
*/
void QFileDialog::done(int result)
{
    Q_D(QFileDialog);

    QDialog::done(result);

    if (d->receiverToDisconnectOnClose) {
        disconnect(this, d->signalToDisconnectOnClose,
                   d->receiverToDisconnectOnClose, d->memberToDisconnectOnClose);
        d->receiverToDisconnectOnClose = nullptr;
    }
    d->memberToDisconnectOnClose.clear();
    d->signalToDisconnectOnClose.clear();
}

bool QFileDialogPrivate::itemAlreadyExists(const QString &fileName)
{
#if QT_CONFIG(messagebox)
    Q_Q(QFileDialog);
    const QString msg = QFileDialog::tr("%1 already exists.\nDo you want to replace it?").arg(fileName);
    using B = QMessageBox;
    const auto res = B::warning(q, q->windowTitle(), msg, B::Yes | B::No, B::No);
    return res == B::Yes;
#endif
    return false;
}

void QFileDialogPrivate::itemNotFound(const QString &fileName, QFileDialog::FileMode mode)
{
#if QT_CONFIG(messagebox)
    Q_Q(QFileDialog);
    const QString message = mode == QFileDialog::Directory
            ? QFileDialog::tr("%1\nDirectory not found.\n"
                              "Please verify the correct directory name was given.")
            : QFileDialog::tr("%1\nFile not found.\nPlease verify the "
                              "correct file name was given.");

    QMessageBox::warning(q, q->windowTitle(), message.arg(fileName));
#endif // QT_CONFIG(messagebox)
}

/*!
 \reimp
*/
void QFileDialog::accept()
{
    Q_D(QFileDialog);
    if (!d->usingWidgets()) {
        const QList<QUrl> urls = selectedUrls();
        if (urls.isEmpty())
            return;
        d->emitUrlsSelected(urls);
        if (urls.size() == 1)
            d->emitUrlSelected(urls.first());
        QDialog::accept();
        return;
    }

    const QStringList files = selectedFiles();
    if (files.isEmpty())
        return;
    QString lineEditText = d->lineEdit()->text();
    // "hidden feature" type .. and then enter, and it will move up a dir
    // special case for ".."
    if (lineEditText == ".."_L1) {
        d->navigateToParent();
        const QSignalBlocker blocker(d->qFileDialogUi->fileNameEdit);
        d->lineEdit()->selectAll();
        return;
    }

    const auto mode = fileMode();
    switch (mode) {
    case Directory: {
        QString fn = files.first();
        QFileInfo info(fn);
        if (!info.exists())
            info = QFileInfo(d->getEnvironmentVariable(fn));
        if (!info.exists()) {
            d->itemNotFound(info.fileName(), mode);
            return;
        }
        if (info.isDir()) {
            d->emitFilesSelected(files);
            QDialog::accept();
        }
        return;
    }

    case AnyFile: {
        QString fn = files.first();
        QFileInfo info(fn);
        if (info.isDir()) {
            setDirectory(info.absoluteFilePath());
            return;
        }

        if (!info.exists()) {
            const long maxNameLength = d->maxNameLength(info.path());
            if (maxNameLength >= 0 && info.fileName().size() > maxNameLength)
                return;
        }

        // check if we have to ask for permission to overwrite the file
        if (!info.exists() || testOption(DontConfirmOverwrite) || acceptMode() == AcceptOpen) {
            d->emitFilesSelected(QStringList(fn));
            QDialog::accept();
        } else {
            if (d->itemAlreadyExists(info.fileName())) {
                d->emitFilesSelected(QStringList(fn));
                QDialog::accept();
            }
        }
        return;
    }

    case ExistingFile:
    case ExistingFiles:
        for (const auto &file : files) {
            QFileInfo info(file);
            if (!info.exists())
                info = QFileInfo(d->getEnvironmentVariable(file));
            if (!info.exists()) {
                d->itemNotFound(info.fileName(), mode);
                return;
            }
            if (info.isDir()) {
                setDirectory(info.absoluteFilePath());
                d->lineEdit()->clear();
                return;
            }
        }
        d->emitFilesSelected(files);
        QDialog::accept();
        return;
    }
}

#if QT_CONFIG(settings)
void QFileDialogPrivate::saveSettings()
{
    Q_Q(QFileDialog);
    QSettings settings(QSettings::UserScope, u"QtProject"_s);
    settings.beginGroup("FileDialog");

    if (usingWidgets()) {
        settings.setValue("sidebarWidth", qFileDialogUi->splitter->sizes().constFirst());
        settings.setValue("shortcuts", QUrl::toStringList(qFileDialogUi->sidebar->urls()));
        settings.setValue("treeViewHeader", qFileDialogUi->treeView->header()->saveState());
    }
    QStringList historyUrls;
    const QStringList history = q->history();
    historyUrls.reserve(history.size());
    for (const QString &path : history)
        historyUrls << QUrl::fromLocalFile(path).toString();
    settings.setValue("history", historyUrls);
    settings.setValue("lastVisited", lastVisitedDir()->toString());
    const QMetaEnum &viewModeMeta = q->metaObject()->enumerator(q->metaObject()->indexOfEnumerator("ViewMode"));
    settings.setValue("viewMode", QLatin1StringView(viewModeMeta.key(q->viewMode())));
    settings.setValue("qtVersion", QT_VERSION_STR ""_L1);
}

bool QFileDialogPrivate::restoreFromSettings()
{
    Q_Q(QFileDialog);
    QSettings settings(QSettings::UserScope, u"QtProject"_s);
    if (!settings.childGroups().contains("FileDialog"_L1))
        return false;
    settings.beginGroup("FileDialog");

    q->setDirectoryUrl(lastVisitedDir()->isEmpty() ? settings.value("lastVisited").toUrl() : *lastVisitedDir());

    QByteArray viewModeStr = settings.value("viewMode").toString().toLatin1();
    const QMetaEnum &viewModeMeta = q->metaObject()->enumerator(q->metaObject()->indexOfEnumerator("ViewMode"));
    bool ok = false;
    int viewMode = viewModeMeta.keyToValue(viewModeStr.constData(), &ok);
    if (!ok)
        viewMode = QFileDialog::List;
    q->setViewMode(static_cast<QFileDialog::ViewMode>(viewMode));

    sidebarUrls = QUrl::fromStringList(settings.value("shortcuts").toStringList());
    headerData = settings.value("treeViewHeader").toByteArray();

    if (!usingWidgets())
        return true;

    QStringList history;
    const auto urlStrings = settings.value("history").toStringList();
    for (const QString &urlStr : urlStrings) {
        QUrl url(urlStr);
        if (url.isLocalFile())
            history << url.toLocalFile();
    }

    return restoreWidgetState(history, settings.value("sidebarWidth", -1).toInt());
}
#endif // settings

bool QFileDialogPrivate::restoreWidgetState(QStringList &history, int splitterPosition)
{
    Q_Q(QFileDialog);
    if (splitterPosition >= 0) {
        QList<int> splitterSizes;
        splitterSizes.append(splitterPosition);
        splitterSizes.append(qFileDialogUi->splitter->widget(1)->sizeHint().width());
        qFileDialogUi->splitter->setSizes(splitterSizes);
    } else {
        if (!qFileDialogUi->splitter->restoreState(splitterState))
            return false;
        QList<int> list = qFileDialogUi->splitter->sizes();
        if (list.size() >= 2 && (list.at(0) == 0 || list.at(1) == 0)) {
            for (int i = 0; i < list.size(); ++i)
                list[i] = qFileDialogUi->splitter->widget(i)->sizeHint().width();
            qFileDialogUi->splitter->setSizes(list);
        }
    }

    qFileDialogUi->sidebar->setUrls(sidebarUrls);

    static const int MaxHistorySize = 5;
    if (history.size() > MaxHistorySize)
        history.erase(history.begin(), history.end() - MaxHistorySize);
    q->setHistory(history);

    QHeaderView *headerView = qFileDialogUi->treeView->header();
    if (!headerView->restoreState(headerData))
        return false;

    QList<QAction*> actions = headerView->actions();
    QAbstractItemModel *abstractModel = model;
#if QT_CONFIG(proxymodel)
    if (proxyModel)
        abstractModel = proxyModel;
#endif
    const int total = qMin(abstractModel->columnCount(QModelIndex()), int(actions.size() + 1));
    for (int i = 1; i < total; ++i)
        actions.at(i - 1)->setChecked(!headerView->isSectionHidden(i));

    return true;
}

/*!
    \internal

    Create widgets, layout and set default values
*/
void QFileDialogPrivate::init(const QFileDialogArgs &args)
{
    Q_Q(QFileDialog);
    if (!args.caption.isEmpty()) {
        useDefaultCaption = false;
        setWindowTitle = args.caption;
        q->setWindowTitle(args.caption);
    }

    q->setAcceptMode(QFileDialog::AcceptOpen);
    nativeDialogInUse = platformFileDialogHelper() != nullptr;
    if (!nativeDialogInUse)
        createWidgets();
    q->setFileMode(QFileDialog::AnyFile);
    if (!args.filter.isEmpty())
        q->setNameFilter(args.filter);
    q->setDirectoryUrl(args.directory);
    if (args.directory.isLocalFile())
        q->selectFile(args.selection);
    else
        q->selectUrl(args.directory);

#if QT_CONFIG(settings)
    // Try to restore from the FileDialog settings group; if it fails, fall back
    // to the pre-5.5 QByteArray serialized settings.
    if (!restoreFromSettings()) {
        const QSettings settings(QSettings::UserScope, u"QtProject"_s);
        q->restoreState(settings.value("Qt/filedialog").toByteArray());
    }
#endif

#if defined(Q_EMBEDDED_SMALLSCREEN)
    qFileDialogUi->lookInLabel->setVisible(false);
    qFileDialogUi->fileNameLabel->setVisible(false);
    qFileDialogUi->fileTypeLabel->setVisible(false);
    qFileDialogUi->sidebar->hide();
#endif

    const QSize sizeHint = q->sizeHint();
    if (sizeHint.isValid())
       q->resize(sizeHint);
}

/*!
    \internal

    Create the widgets, set properties and connections
*/
void QFileDialogPrivate::createWidgets()
{
    if (qFileDialogUi)
        return;
    Q_Q(QFileDialog);

    // This function is sometimes called late (e.g as a fallback from setVisible). In that case we
    // need to ensure that the following UI code (setupUI in particular) doesn't reset any explicitly
    // set window state or geometry.
    QSize preSize = q->testAttribute(Qt::WA_Resized) ? q->size() : QSize();
    Qt::WindowStates preState = q->windowState();

    model = new QFileSystemModel(q);
    model->setIconProvider(&defaultIconProvider);
    model->setFilter(options->filter());
    model->setObjectName("qt_filesystem_model"_L1);
    if (QPlatformFileDialogHelper *helper = platformFileDialogHelper())
        model->setNameFilterDisables(helper->defaultNameFilterDisables());
    else
        model->setNameFilterDisables(false);
    model->d_func()->disableRecursiveSort = true;
    QObjectPrivate::connect(model, &QFileSystemModel::fileRenamed,
                            this, &QFileDialogPrivate::fileRenamed);
    QObjectPrivate::connect(model, &QFileSystemModel::rootPathChanged,
                            this, &QFileDialogPrivate::pathChanged);
    QObjectPrivate::connect(model, &QFileSystemModel::rowsInserted,
                            this, &QFileDialogPrivate::rowsInserted);
    model->setReadOnly(false);

    qFileDialogUi.reset(new Ui_QFileDialog());
    qFileDialogUi->setupUi(q);

    QList<QUrl> initialBookmarks;
    initialBookmarks << QUrl("file:"_L1)
                     << QUrl::fromLocalFile(QDir::homePath());
    qFileDialogUi->sidebar->setModelAndUrls(model, initialBookmarks);
    QObjectPrivate::connect(qFileDialogUi->sidebar, &QSidebar::goToUrl,
                            this, &QFileDialogPrivate::goToUrl);

    QObject::connect(qFileDialogUi->buttonBox, &QDialogButtonBox::accepted,
                     q, &QFileDialog::accept);
    QObject::connect(qFileDialogUi->buttonBox, &QDialogButtonBox::rejected,
                     q, &QFileDialog::reject);

    qFileDialogUi->lookInCombo->setFileDialogPrivate(this);
    QObjectPrivate::connect(qFileDialogUi->lookInCombo, &QComboBox::textActivated,
                            this, &QFileDialogPrivate::goToDirectory);

    qFileDialogUi->lookInCombo->setInsertPolicy(QComboBox::NoInsert);
    qFileDialogUi->lookInCombo->setDuplicatesEnabled(false);

    // filename
    qFileDialogUi->fileNameEdit->setFileDialogPrivate(this);
#ifndef QT_NO_SHORTCUT
    qFileDialogUi->fileNameLabel->setBuddy(qFileDialogUi->fileNameEdit);
#endif
#if QT_CONFIG(fscompleter)
    completer = new QFSCompleter(model, q);
    qFileDialogUi->fileNameEdit->setCompleter(completer);
#endif // QT_CONFIG(fscompleter)

    qFileDialogUi->fileNameEdit->setInputMethodHints(Qt::ImhNoPredictiveText);

    QObjectPrivate::connect(qFileDialogUi->fileNameEdit, &QLineEdit::textChanged,
                            this, &QFileDialogPrivate::autoCompleteFileName);
    QObjectPrivate::connect(qFileDialogUi->fileNameEdit, &QLineEdit::textChanged,
                            this, &QFileDialogPrivate::updateOkButton);
    QObject::connect(qFileDialogUi->fileNameEdit, &QLineEdit::returnPressed,
                     q, &QFileDialog::accept);

    // filetype
    qFileDialogUi->fileTypeCombo->setDuplicatesEnabled(false);
    qFileDialogUi->fileTypeCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    qFileDialogUi->fileTypeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QObjectPrivate::connect(qFileDialogUi->fileTypeCombo, &QComboBox::activated,
                            this, &QFileDialogPrivate::useNameFilter);
    QObject::connect(qFileDialogUi->fileTypeCombo, &QComboBox::textActivated,
                     q, &QFileDialog::filterSelected);

    qFileDialogUi->listView->setFileDialogPrivate(this);
    qFileDialogUi->listView->setModel(model);
    QObjectPrivate::connect(qFileDialogUi->listView, &QAbstractItemView::activated,
                            this, &QFileDialogPrivate::enterDirectory);
    QObjectPrivate::connect(qFileDialogUi->listView, &QAbstractItemView::customContextMenuRequested,
                            this, &QFileDialogPrivate::showContextMenu);
#ifndef QT_NO_SHORTCUT
    QShortcut *shortcut = new QShortcut(QKeySequence::Delete, qFileDialogUi->listView);
    QObjectPrivate::connect(shortcut, &QShortcut::activated,
                            this, &QFileDialogPrivate::deleteCurrent);
#endif

    qFileDialogUi->treeView->setFileDialogPrivate(this);
    qFileDialogUi->treeView->setModel(model);
    QHeaderView *treeHeader = qFileDialogUi->treeView->header();
    QFontMetrics fm(q->font());
    treeHeader->resizeSection(0, fm.horizontalAdvance("wwwwwwwwwwwwwwwwwwwwwwwwww"_L1));
    treeHeader->resizeSection(1, fm.horizontalAdvance("128.88 GB"_L1));
    treeHeader->resizeSection(2, fm.horizontalAdvance("mp3Folder"_L1));
    treeHeader->resizeSection(3, fm.horizontalAdvance("10/29/81 02:02PM"_L1));
    treeHeader->setContextMenuPolicy(Qt::ActionsContextMenu);

    QActionGroup *showActionGroup = new QActionGroup(q);
    showActionGroup->setExclusive(false);
    QObjectPrivate::connect(showActionGroup, &QActionGroup::triggered,
                            this, &QFileDialogPrivate::showHeader);

    QAbstractItemModel *abstractModel = model;
#if QT_CONFIG(proxymodel)
    if (proxyModel)
        abstractModel = proxyModel;
#endif
    for (int i = 1; i < abstractModel->columnCount(QModelIndex()); ++i) {
        QAction *showHeader = new QAction(showActionGroup);
        showHeader->setCheckable(true);
        showHeader->setChecked(true);
        treeHeader->addAction(showHeader);
    }

    QScopedPointer<QItemSelectionModel> selModel(qFileDialogUi->treeView->selectionModel());
    qFileDialogUi->treeView->setSelectionModel(qFileDialogUi->listView->selectionModel());

    QObjectPrivate::connect(qFileDialogUi->treeView, &QAbstractItemView::activated,
                            this, &QFileDialogPrivate::enterDirectory);
    QObjectPrivate::connect(qFileDialogUi->treeView, &QAbstractItemView::customContextMenuRequested,
                            this, &QFileDialogPrivate::showContextMenu);
#ifndef QT_NO_SHORTCUT
    shortcut = new QShortcut(QKeySequence::Delete, qFileDialogUi->treeView);
    QObjectPrivate::connect(shortcut, &QShortcut::activated,
                            this, &QFileDialogPrivate::deleteCurrent);
#endif

    // Selections
    QItemSelectionModel *selections = qFileDialogUi->listView->selectionModel();
    QObjectPrivate::connect(selections, &QItemSelectionModel::selectionChanged,
                            this, &QFileDialogPrivate::selectionChanged);
    QObjectPrivate::connect(selections, &QItemSelectionModel::currentChanged,
                            this, &QFileDialogPrivate::currentChanged);
    qFileDialogUi->splitter->setStretchFactor(qFileDialogUi->splitter->indexOf(qFileDialogUi->splitter->widget(1)), QSizePolicy::Expanding);

    createToolButtons();
    createMenuActions();

#if QT_CONFIG(settings)
    // Try to restore from the FileDialog settings group; if it fails, fall back
    // to the pre-5.5 QByteArray serialized settings.
    if (!restoreFromSettings()) {
        const QSettings settings(QSettings::UserScope, u"QtProject"_s);
        q->restoreState(settings.value("Qt/filedialog").toByteArray());
    }
#endif

    // Initial widget states from options
    q->setFileMode(static_cast<QFileDialog::FileMode>(options->fileMode()));
    q->setAcceptMode(static_cast<QFileDialog::AcceptMode>(options->acceptMode()));
    q->setViewMode(static_cast<QFileDialog::ViewMode>(options->viewMode()));
    q->setOptions(static_cast<QFileDialog::Options>(static_cast<int>(options->options())));
    if (!options->sidebarUrls().isEmpty())
        q->setSidebarUrls(options->sidebarUrls());
    q->setDirectoryUrl(options->initialDirectory());
#if QT_CONFIG(mimetype)
    if (!options->mimeTypeFilters().isEmpty())
        q->setMimeTypeFilters(options->mimeTypeFilters());
    else
#endif
    if (!options->nameFilters().isEmpty())
        q->setNameFilters(options->nameFilters());
    q->selectNameFilter(options->initiallySelectedNameFilter());
    q->setDefaultSuffix(options->defaultSuffix());
    q->setHistory(options->history());
    const auto initiallySelectedFiles = options->initiallySelectedFiles();
    if (initiallySelectedFiles.size() == 1)
        q->selectFile(initiallySelectedFiles.first().fileName());
    for (const QUrl &url : initiallySelectedFiles)
        q->selectUrl(url);
    lineEdit()->selectAll();
    updateOkButton();
    retranslateStrings();
    q->resize(preSize.isValid() ? preSize : q->sizeHint());
    q->setWindowState(preState);
}

void QFileDialogPrivate::showHeader(QAction *action)
{
    Q_Q(QFileDialog);
    QActionGroup *actionGroup = qobject_cast<QActionGroup*>(q->sender());
    qFileDialogUi->treeView->header()->setSectionHidden(int(actionGroup->actions().indexOf(action) + 1),
                                                        !action->isChecked());
}

#if QT_CONFIG(proxymodel)
/*!
    Sets the model for the views to the given \a proxyModel.  This is useful if you
    want to modify the underlying model; for example, to add columns, filter
    data or add drives.

    Any existing proxy model is removed, but not deleted.  The file dialog
    takes ownership of the \a proxyModel.

    \sa proxyModel()
*/
void QFileDialog::setProxyModel(QAbstractProxyModel *proxyModel)
{
    Q_D(QFileDialog);
    if (!d->usingWidgets())
        return;
    if ((!proxyModel && !d->proxyModel)
        || (proxyModel == d->proxyModel))
        return;

    QModelIndex idx = d->rootIndex();
    if (d->proxyModel)
        QObjectPrivate::disconnect(d->proxyModel, &QAbstractProxyModel::rowsInserted,
                                   d, &QFileDialogPrivate::rowsInserted);
    else
        QObjectPrivate::disconnect(d->model, &QAbstractItemModel::rowsInserted,
                                   d, &QFileDialogPrivate::rowsInserted);

    if (proxyModel != nullptr) {
        proxyModel->setParent(this);
        d->proxyModel = proxyModel;
        proxyModel->setSourceModel(d->model);
        d->qFileDialogUi->listView->setModel(d->proxyModel);
        d->qFileDialogUi->treeView->setModel(d->proxyModel);
#if QT_CONFIG(fscompleter)
        d->completer->setModel(d->proxyModel);
        d->completer->proxyModel = d->proxyModel;
#endif
        QObjectPrivate::connect(d->proxyModel, &QAbstractItemModel::rowsInserted,
                                d, &QFileDialogPrivate::rowsInserted);
    } else {
        d->proxyModel = nullptr;
        d->qFileDialogUi->listView->setModel(d->model);
        d->qFileDialogUi->treeView->setModel(d->model);
#if QT_CONFIG(fscompleter)
        d->completer->setModel(d->model);
        d->completer->sourceModel = d->model;
        d->completer->proxyModel = nullptr;
#endif
        QObjectPrivate::connect(d->model, &QAbstractItemModel::rowsInserted,
                                d, &QFileDialogPrivate::rowsInserted);
    }
    QScopedPointer<QItemSelectionModel> selModel(d->qFileDialogUi->treeView->selectionModel());
    d->qFileDialogUi->treeView->setSelectionModel(d->qFileDialogUi->listView->selectionModel());

    d->setRootIndex(idx);

    // reconnect selection
    QItemSelectionModel *selections = d->qFileDialogUi->listView->selectionModel();
    QObjectPrivate::connect(selections, &QItemSelectionModel::selectionChanged,
                            d, &QFileDialogPrivate::selectionChanged);
    QObjectPrivate::connect(selections, &QItemSelectionModel::currentChanged,
                            d, &QFileDialogPrivate::currentChanged);
}

/*!
    Returns the proxy model used by the file dialog.  By default no proxy is set.

    \sa setProxyModel()
*/
QAbstractProxyModel *QFileDialog::proxyModel() const
{
    Q_D(const QFileDialog);
    return d->proxyModel;
}
#endif // QT_CONFIG(proxymodel)

/*!
    \internal

    Create tool buttons, set properties and connections
*/
void QFileDialogPrivate::createToolButtons()
{
    Q_Q(QFileDialog);
    qFileDialogUi->backButton->setIcon(q->style()->standardIcon(QStyle::SP_ArrowBack, nullptr, q));
    qFileDialogUi->backButton->setAutoRaise(true);
    qFileDialogUi->backButton->setEnabled(false);
    QObjectPrivate::connect(qFileDialogUi->backButton, &QPushButton::clicked,
                            this, &QFileDialogPrivate::navigateBackward);

    qFileDialogUi->forwardButton->setIcon(q->style()->standardIcon(QStyle::SP_ArrowForward, nullptr, q));
    qFileDialogUi->forwardButton->setAutoRaise(true);
    qFileDialogUi->forwardButton->setEnabled(false);
    QObjectPrivate::connect(qFileDialogUi->forwardButton, &QPushButton::clicked,
                            this, &QFileDialogPrivate::navigateForward);

    qFileDialogUi->toParentButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogToParent, nullptr, q));
    qFileDialogUi->toParentButton->setAutoRaise(true);
    qFileDialogUi->toParentButton->setEnabled(false);
    QObjectPrivate::connect(qFileDialogUi->toParentButton, &QPushButton::clicked,
                            this, &QFileDialogPrivate::navigateToParent);

    qFileDialogUi->listModeButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogListView, nullptr, q));
    qFileDialogUi->listModeButton->setAutoRaise(true);
    qFileDialogUi->listModeButton->setDown(true);
    QObjectPrivate::connect(qFileDialogUi->listModeButton, &QPushButton::clicked,
                            this, &QFileDialogPrivate::showListView);

    qFileDialogUi->detailModeButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogDetailedView, nullptr, q));
    qFileDialogUi->detailModeButton->setAutoRaise(true);
    QObjectPrivate::connect(qFileDialogUi->detailModeButton, &QPushButton::clicked,
                            this, &QFileDialogPrivate::showDetailsView);

    QSize toolSize(qFileDialogUi->fileNameEdit->sizeHint().height(), qFileDialogUi->fileNameEdit->sizeHint().height());
    qFileDialogUi->backButton->setFixedSize(toolSize);
    qFileDialogUi->listModeButton->setFixedSize(toolSize);
    qFileDialogUi->detailModeButton->setFixedSize(toolSize);
    qFileDialogUi->forwardButton->setFixedSize(toolSize);
    qFileDialogUi->toParentButton->setFixedSize(toolSize);

    qFileDialogUi->newFolderButton->setIcon(q->style()->standardIcon(QStyle::SP_FileDialogNewFolder, nullptr, q));
    qFileDialogUi->newFolderButton->setFixedSize(toolSize);
    qFileDialogUi->newFolderButton->setAutoRaise(true);
    qFileDialogUi->newFolderButton->setEnabled(false);
    QObjectPrivate::connect(qFileDialogUi->newFolderButton, &QPushButton::clicked,
                            this, &QFileDialogPrivate::createDirectory);
}

/*!
    \internal

    Create actions which will be used in the right click.
*/
void QFileDialogPrivate::createMenuActions()
{
    Q_Q(QFileDialog);

    QAction *goHomeAction =  new QAction(q);
#ifndef QT_NO_SHORTCUT
    goHomeAction->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_H);
#endif
    QObjectPrivate::connect(goHomeAction, &QAction::triggered,
                            this, &QFileDialogPrivate::goHome);
    q->addAction(goHomeAction);

    // ### TODO add Desktop & Computer actions

    QAction *goToParent =  new QAction(q);
    goToParent->setObjectName("qt_goto_parent_action"_L1);
#ifndef QT_NO_SHORTCUT
    goToParent->setShortcut(Qt::CTRL | Qt::Key_Up);
#endif
    QObjectPrivate::connect(goToParent, &QAction::triggered,
                            this, &QFileDialogPrivate::navigateToParent);
    q->addAction(goToParent);

    renameAction = new QAction(q);
    renameAction->setEnabled(false);
    renameAction->setObjectName("qt_rename_action"_L1);
    QObjectPrivate::connect(renameAction, &QAction::triggered,
                            this, &QFileDialogPrivate::renameCurrent);

    deleteAction = new QAction(q);
    deleteAction->setEnabled(false);
    deleteAction->setObjectName("qt_delete_action"_L1);
    QObjectPrivate::connect(deleteAction, &QAction::triggered,
                            this, &QFileDialogPrivate::deleteCurrent);

    showHiddenAction = new QAction(q);
    showHiddenAction->setObjectName("qt_show_hidden_action"_L1);
    showHiddenAction->setCheckable(true);
    QObjectPrivate::connect(showHiddenAction, &QAction::triggered,
                            this, &QFileDialogPrivate::showHidden);

    newFolderAction = new QAction(q);
    newFolderAction->setObjectName("qt_new_folder_action"_L1);
    QObjectPrivate::connect(newFolderAction, &QAction::triggered,
                            this, &QFileDialogPrivate::createDirectory);
}

void QFileDialogPrivate::goHome()
{
    Q_Q(QFileDialog);
    q->setDirectory(QDir::homePath());
}


void QFileDialogPrivate::saveHistorySelection()
{
    if (qFileDialogUi.isNull() || currentHistoryLocation < 0 || currentHistoryLocation >= currentHistory.size())
        return;
    auto &item = currentHistory[currentHistoryLocation];
    item.selection.clear();
    const auto selectedIndexes = qFileDialogUi->listView->selectionModel()->selectedRows();
    for (const auto &index : selectedIndexes)
        item.selection.append(QPersistentModelIndex(index));
}

/*!
    \internal

    Update history with new path, buttons, and combo
*/
void QFileDialogPrivate::pathChanged(const QString &newPath)
{
    Q_Q(QFileDialog);
    qFileDialogUi->toParentButton->setEnabled(QFileInfo::exists(model->rootPath()));
    qFileDialogUi->sidebar->selectUrl(QUrl::fromLocalFile(newPath));
    q->setHistory(qFileDialogUi->lookInCombo->history());

    const QString newNativePath = QDir::toNativeSeparators(newPath);

    // equal paths indicate this was invoked by _q_navigateBack/Forward()
    if (currentHistoryLocation < 0 || currentHistory.value(currentHistoryLocation).path != newNativePath) {
        if (currentHistoryLocation >= 0)
            saveHistorySelection();
        while (currentHistoryLocation >= 0 && currentHistoryLocation + 1 < currentHistory.size()) {
            currentHistory.removeLast();
        }
        currentHistory.append({newNativePath, PersistentModelIndexList()});
        ++currentHistoryLocation;
    }
    qFileDialogUi->forwardButton->setEnabled(currentHistory.size() - currentHistoryLocation > 1);
    qFileDialogUi->backButton->setEnabled(currentHistoryLocation > 0);
}

void QFileDialogPrivate::navigate(HistoryItem &historyItem)
{
    Q_Q(QFileDialog);
    q->setDirectory(historyItem.path);
    // Restore selection unless something has changed in the file system
    if (qFileDialogUi.isNull() || historyItem.selection.isEmpty())
        return;
    if (std::any_of(historyItem.selection.cbegin(), historyItem.selection.cend(),
                    [](const QPersistentModelIndex &i) { return !i.isValid(); })) {
        historyItem.selection.clear();
        return;
    }

    QAbstractItemView *view = q->viewMode() == QFileDialog::List
        ? static_cast<QAbstractItemView *>(qFileDialogUi->listView)
        : static_cast<QAbstractItemView *>(qFileDialogUi->treeView);
    auto selectionModel = view->selectionModel();
    const QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Select
        | QItemSelectionModel::Rows;
    selectionModel->select(historyItem.selection.constFirst(),
                           flags | QItemSelectionModel::Clear | QItemSelectionModel::Current);
    auto it = historyItem.selection.cbegin() + 1;
    const auto end = historyItem.selection.cend();
    for (; it != end; ++it)
        selectionModel->select(*it, flags);

    view->scrollTo(historyItem.selection.constFirst());
}

/*!
    \internal

    Navigates to the last directory viewed in the dialog.
*/
void QFileDialogPrivate::navigateBackward()
{
    if (!currentHistory.isEmpty() && currentHistoryLocation > 0) {
        saveHistorySelection();
        navigate(currentHistory[--currentHistoryLocation]);
    }
}

/*!
    \internal

    Navigates to the last directory viewed in the dialog.
*/
void QFileDialogPrivate::navigateForward()
{
    if (!currentHistory.isEmpty() && currentHistoryLocation < currentHistory.size() - 1) {
        saveHistorySelection();
        navigate(currentHistory[++currentHistoryLocation]);
    }
}

/*!
    \internal

    Navigates to the parent directory of the currently displayed directory
    in the dialog.
*/
void QFileDialogPrivate::navigateToParent()
{
    Q_Q(QFileDialog);
    QDir dir(model->rootDirectory());
    QString newDirectory;
    if (dir.isRoot()) {
        newDirectory = model->myComputer().toString();
    } else {
        dir.cdUp();
        newDirectory = dir.absolutePath();
    }
    q->setDirectory(newDirectory);
    emit q->directoryEntered(newDirectory);
}

/*!
    \internal

    Creates a new directory, first asking the user for a suitable name.
*/
void QFileDialogPrivate::createDirectory()
{
    Q_Q(QFileDialog);
    qFileDialogUi->listView->clearSelection();

    QString newFolderString = QFileDialog::tr("New Folder");
    QString folderName = newFolderString;
    QString prefix = q->directory().absolutePath() + QDir::separator();
    if (QFile::exists(prefix + folderName)) {
        qlonglong suffix = 2;
        while (QFile::exists(prefix + folderName)) {
            folderName = newFolderString + QString::number(suffix++);
        }
    }

    QModelIndex parent = rootIndex();
    QModelIndex index = model->mkdir(parent, folderName);
    if (!index.isValid())
        return;

    index = select(index);
    if (index.isValid()) {
        qFileDialogUi->treeView->setCurrentIndex(index);
        currentView()->edit(index);
    }
}

void QFileDialogPrivate::showListView()
{
    qFileDialogUi->listModeButton->setDown(true);
    qFileDialogUi->detailModeButton->setDown(false);
    qFileDialogUi->treeView->hide();
    qFileDialogUi->listView->show();
    qFileDialogUi->stackedWidget->setCurrentWidget(qFileDialogUi->listView->parentWidget());
    qFileDialogUi->listView->doItemsLayout();
}

void QFileDialogPrivate::showDetailsView()
{
    qFileDialogUi->listModeButton->setDown(false);
    qFileDialogUi->detailModeButton->setDown(true);
    qFileDialogUi->listView->hide();
    qFileDialogUi->treeView->show();
    qFileDialogUi->stackedWidget->setCurrentWidget(qFileDialogUi->treeView->parentWidget());
    qFileDialogUi->treeView->doItemsLayout();
}

/*!
    \internal

    Show the context menu for the file/dir under position
*/
void QFileDialogPrivate::showContextMenu(const QPoint &position)
{
#if !QT_CONFIG(menu)
    Q_UNUSED(position);
#else
    Q_Q(QFileDialog);
    QAbstractItemView *view = nullptr;
    if (q->viewMode() == QFileDialog::Detail)
        view = qFileDialogUi->treeView;
    else
        view = qFileDialogUi->listView;
    QModelIndex index = view->indexAt(position);
    index = mapToSource(index.sibling(index.row(), 0));

    QMenu *menu = new QMenu(view);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (index.isValid()) {
        // file context menu
        const bool ro = model && model->isReadOnly();
        QFile::Permissions p(index.parent().data(QFileSystemModel::FilePermissions).toInt());
        renameAction->setEnabled(!ro && p & QFile::WriteUser);
        menu->addAction(renameAction);
        deleteAction->setEnabled(!ro && p & QFile::WriteUser);
        menu->addAction(deleteAction);
        menu->addSeparator();
    }
    menu->addAction(showHiddenAction);
    if (qFileDialogUi->newFolderButton->isVisible()) {
        newFolderAction->setEnabled(qFileDialogUi->newFolderButton->isEnabled());
        menu->addAction(newFolderAction);
    }
    menu->popup(view->viewport()->mapToGlobal(position));

#endif // QT_CONFIG(menu)
}

/*!
    \internal
*/
void QFileDialogPrivate::renameCurrent()
{
    Q_Q(QFileDialog);
    QModelIndex index = qFileDialogUi->listView->currentIndex();
    index = index.sibling(index.row(), 0);
    if (q->viewMode() == QFileDialog::List)
        qFileDialogUi->listView->edit(index);
    else
        qFileDialogUi->treeView->edit(index);
}

bool QFileDialogPrivate::removeDirectory(const QString &path)
{
    QModelIndex modelIndex = model->index(path);
    return model->remove(modelIndex);
}

/*!
    \internal

    Deletes the currently selected item in the dialog.
*/
void QFileDialogPrivate::deleteCurrent()
{
    if (model->isReadOnly())
        return;

    const QModelIndexList list = qFileDialogUi->listView->selectionModel()->selectedRows();
    for (auto it = list.crbegin(), end = list.crend(); it != end; ++it) {
        QPersistentModelIndex index = *it;
        if (index == qFileDialogUi->listView->rootIndex())
            continue;

        index = mapToSource(index.sibling(index.row(), 0));
        if (!index.isValid())
            continue;

        QString fileName = index.data(QFileSystemModel::FileNameRole).toString();
        QString filePath = index.data(QFileSystemModel::FilePathRole).toString();

        QFile::Permissions p(index.parent().data(QFileSystemModel::FilePermissions).toInt());
#if QT_CONFIG(messagebox)
        Q_Q(QFileDialog);
        if (!(p & QFile::WriteUser) && (QMessageBox::warning(q_func(), QFileDialog::tr("Delete"),
                                    QFileDialog::tr("'%1' is write protected.\nDo you want to delete it anyway?")
                                    .arg(fileName),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No))
            return;
        else if (QMessageBox::warning(q_func(), QFileDialog::tr("Delete"),
                                      QFileDialog::tr("Are you sure you want to delete '%1'?")
                                      .arg(fileName),
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
            return;

        // the event loop has run, we have to validate if the index is valid because the model might have removed it.
        if (!index.isValid())
            return;

#else
        if (!(p & QFile::WriteUser))
            return;
#endif // QT_CONFIG(messagebox)

        if (model->isDir(index) && !model->fileInfo(index).isSymLink()) {
            if (!removeDirectory(filePath)) {
#if QT_CONFIG(messagebox)
            QMessageBox::warning(q, q->windowTitle(),
                                QFileDialog::tr("Could not delete directory."));
#endif
            }
        } else {
            model->remove(index);
        }
    }
}

void QFileDialogPrivate::autoCompleteFileName(const QString &text)
{
    if (text.startsWith("//"_L1) || text.startsWith(u'\\')) {
        qFileDialogUi->listView->selectionModel()->clearSelection();
        return;
    }

    const QStringList multipleFiles = typedFiles();
    if (multipleFiles.size() > 0) {
        QModelIndexList oldFiles = qFileDialogUi->listView->selectionModel()->selectedRows();
        QList<QModelIndex> newFiles;
        for (const auto &file : multipleFiles) {
            QModelIndex idx = model->index(file);
            if (oldFiles.removeAll(idx) == 0)
                newFiles.append(idx);
        }
        for (const auto &newFile : std::as_const(newFiles))
            select(newFile);
        if (lineEdit()->hasFocus()) {
            auto *sm = qFileDialogUi->listView->selectionModel();
            for (const auto &oldFile : std::as_const(oldFiles))
                sm->select(oldFile, QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
        }
    }
}

/*!
    \internal
*/
void QFileDialogPrivate::updateOkButton()
{
    Q_Q(QFileDialog);
    QPushButton *button =  qFileDialogUi->buttonBox->button((q->acceptMode() == QFileDialog::AcceptOpen)
                    ? QDialogButtonBox::Open : QDialogButtonBox::Save);
    if (!button)
        return;
    const QFileDialog::FileMode fileMode = q->fileMode();

    bool enableButton = true;
    bool isOpenDirectory = false;

    const QStringList files = q->selectedFiles();
    QString lineEditText = lineEdit()->text();

    if (lineEditText.startsWith("//"_L1) || lineEditText.startsWith(u'\\')) {
        button->setEnabled(true);
        updateOkButtonText();
        return;
    }

    if (files.isEmpty()) {
        enableButton = false;
    } else if (lineEditText == ".."_L1) {
        isOpenDirectory = true;
    } else {
        switch (fileMode) {
        case QFileDialog::Directory: {
            QString fn = files.first();
            QModelIndex idx = model->index(fn);
            if (!idx.isValid())
                idx = model->index(getEnvironmentVariable(fn));
            if (!idx.isValid() || !model->isDir(idx))
                enableButton = false;
            break;
        }
        case QFileDialog::AnyFile: {
            QString fn = files.first();
            QFileInfo info(fn);
            QModelIndex idx = model->index(fn);
            QString fileDir;
            QString fileName;
            if (info.isDir()) {
                fileDir = info.canonicalFilePath();
            } else {
                fileDir = fn.mid(0, fn.lastIndexOf(u'/'));
                fileName = fn.mid(fileDir.size() + 1);
            }
            if (lineEditText.contains(".."_L1)) {
                fileDir = info.canonicalFilePath();
                fileName = info.fileName();
            }

            if (fileDir == q->directory().canonicalPath() && fileName.isEmpty()) {
                enableButton = false;
                break;
            }
            if (idx.isValid() && model->isDir(idx)) {
                isOpenDirectory = true;
                enableButton = true;
                break;
            }
            if (!idx.isValid()) {
                const long maxLength = maxNameLength(fileDir);
                enableButton = maxLength < 0 || fileName.size() <= maxLength;
            }
            break;
        }
        case QFileDialog::ExistingFile:
        case QFileDialog::ExistingFiles:
            for (const auto &file : files) {
                QModelIndex idx = model->index(file);
                if (!idx.isValid())
                    idx = model->index(getEnvironmentVariable(file));
                if (!idx.isValid()) {
                    enableButton = false;
                    break;
                }
                if (idx.isValid() && model->isDir(idx)) {
                    isOpenDirectory = true;
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    button->setEnabled(enableButton);
    updateOkButtonText(isOpenDirectory);
}

/*!
    \internal
*/
void QFileDialogPrivate::currentChanged(const QModelIndex &index)
{
    updateOkButton();
    emit q_func()->currentChanged(index.data(QFileSystemModel::FilePathRole).toString());
}

/*!
    \internal

    This is called when the user double clicks on a file with the corresponding
    model item \a index.
*/
void QFileDialogPrivate::enterDirectory(const QModelIndex &index)
{
    Q_Q(QFileDialog);
    // My Computer or a directory
    QModelIndex sourceIndex = index.model() == proxyModel ? mapToSource(index) : index;
    QString path = sourceIndex.data(QFileSystemModel::FilePathRole).toString();
    if (path.isEmpty() || model->isDir(sourceIndex)) {
        const QFileDialog::FileMode fileMode = q->fileMode();
        q->setDirectory(path);
        emit q->directoryEntered(path);
        if (fileMode == QFileDialog::Directory) {
            // ### find out why you have to do both of these.
            lineEdit()->setText(QString());
            lineEdit()->clear();
        }
    } else {
        // Do not accept when shift-clicking to multi-select a file in environments with single-click-activation (KDE)
        if ((!q->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, qFileDialogUi->treeView)
             || q->fileMode() != QFileDialog::ExistingFiles || !(QGuiApplication::keyboardModifiers() & Qt::CTRL))
            && index.model()->flags(index) & Qt::ItemIsEnabled) {
            q->accept();
        }
    }
}

/*!
    \internal

    Changes the file dialog's current directory to the one specified
    by \a path.
*/
void QFileDialogPrivate::goToDirectory(const QString &path)
{
    enum { UrlRole = Qt::UserRole + 1 };

 #if QT_CONFIG(messagebox)
    Q_Q(QFileDialog);
#endif
    QModelIndex index = qFileDialogUi->lookInCombo->model()->index(qFileDialogUi->lookInCombo->currentIndex(),
                                                    qFileDialogUi->lookInCombo->modelColumn(),
                                                    qFileDialogUi->lookInCombo->rootModelIndex());
    QString path2 = path;
    if (!index.isValid())
        index = mapFromSource(model->index(getEnvironmentVariable(path)));
    else {
        path2 = index.data(UrlRole).toUrl().toLocalFile();
        index = mapFromSource(model->index(path2));
    }
    QDir dir(path2);
    if (!dir.exists())
        dir.setPath(getEnvironmentVariable(path2));

    if (dir.exists() || path2.isEmpty() || path2 == model->myComputer().toString()) {
        enterDirectory(index);
#if QT_CONFIG(messagebox)
    } else {
        QString message = QFileDialog::tr("%1\nDirectory not found.\nPlease verify the "
                                          "correct directory name was given.");
        QMessageBox::warning(q, q->windowTitle(), message.arg(path2));
#endif // QT_CONFIG(messagebox)
    }
}

/*!
    \internal

    Sets the current name filter to be nameFilter and
    update the qFileDialogUi->fileNameEdit when in AcceptSave mode with the new extension.
*/
void QFileDialogPrivate::useNameFilter(int index)
{
    QStringList nameFilters = options->nameFilters();
    if (index == nameFilters.size()) {
        QAbstractItemModel *comboModel = qFileDialogUi->fileTypeCombo->model();
        nameFilters.append(comboModel->index(comboModel->rowCount() - 1, 0).data().toString());
        options->setNameFilters(nameFilters);
    }

    QString nameFilter = nameFilters.at(index);
    QStringList newNameFilters = QPlatformFileDialogHelper::cleanFilterList(nameFilter);
    if (q_func()->acceptMode() == QFileDialog::AcceptSave) {
        QString newNameFilterExtension;
        if (newNameFilters.size() > 0)
            newNameFilterExtension = QFileInfo(newNameFilters.at(0)).suffix();

        QString fileName = lineEdit()->text();
        const QString fileNameExtension = QFileInfo(fileName).suffix();
        if (!fileNameExtension.isEmpty() && !newNameFilterExtension.isEmpty()) {
            const qsizetype fileNameExtensionLength = fileNameExtension.size();
            fileName.replace(fileName.size() - fileNameExtensionLength,
                             fileNameExtensionLength, newNameFilterExtension);
            qFileDialogUi->listView->clearSelection();
            lineEdit()->setText(fileName);
        }
    }

    model->setNameFilters(newNameFilters);
}

/*!
    \internal

    This is called when the model index corresponding to the current file is changed
    from \a index to \a current.
*/
void QFileDialogPrivate::selectionChanged()
{
    const QFileDialog::FileMode fileMode = q_func()->fileMode();
    const QModelIndexList indexes = qFileDialogUi->listView->selectionModel()->selectedRows();
    bool stripDirs = fileMode != QFileDialog::Directory;

    QStringList allFiles;
    for (const auto &index : indexes) {
        if (stripDirs && model->isDir(mapToSource(index)))
            continue;
        allFiles.append(index.data().toString());
    }
    if (allFiles.size() > 1)
        for (qsizetype i = 0; i < allFiles.size(); ++i) {
            allFiles.replace(i, QString(u'"' + allFiles.at(i) + u'"'));
    }

    QString finalFiles = allFiles.join(u' ');
    if (!finalFiles.isEmpty() && !lineEdit()->hasFocus() && lineEdit()->isVisible())
        lineEdit()->setText(finalFiles);
    else
        updateOkButton();
}

/*!
    \internal

    Includes hidden files and directories in the items displayed in the dialog.
*/
void QFileDialogPrivate::showHidden()
{
    Q_Q(QFileDialog);
    QDir::Filters dirFilters = q->filter();
    dirFilters.setFlag(QDir::Hidden, showHiddenAction->isChecked());
    q->setFilter(dirFilters);
}

/*!
    \internal

    When parent is root and rows have been inserted when none was there before
    then select the first one.
*/
void QFileDialogPrivate::rowsInserted(const QModelIndex &parent)
{
    if (!qFileDialogUi->treeView
        || parent != qFileDialogUi->treeView->rootIndex()
        || !qFileDialogUi->treeView->selectionModel()
        || qFileDialogUi->treeView->selectionModel()->hasSelection()
        || qFileDialogUi->treeView->model()->rowCount(parent) == 0)
        return;
}

void QFileDialogPrivate::fileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    const QFileDialog::FileMode fileMode = q_func()->fileMode();
    if (fileMode == QFileDialog::Directory) {
        if (path == rootPath() && lineEdit()->text() == oldName)
            lineEdit()->setText(newName);
    }
}

void QFileDialogPrivate::emitUrlSelected(const QUrl &file)
{
    Q_Q(QFileDialog);
    emit q->urlSelected(file);
    if (file.isLocalFile())
        emit q->fileSelected(file.toLocalFile());
}

void QFileDialogPrivate::emitUrlsSelected(const QList<QUrl> &files)
{
    Q_Q(QFileDialog);
    emit q->urlsSelected(files);
    QStringList localFiles;
    for (const QUrl &file : files)
        if (file.isLocalFile())
            localFiles.append(file.toLocalFile());
    if (!localFiles.isEmpty())
        emit q->filesSelected(localFiles);
}

void QFileDialogPrivate::nativeCurrentChanged(const QUrl &file)
{
    Q_Q(QFileDialog);
    emit q->currentUrlChanged(file);
    if (file.isLocalFile())
        emit q->currentChanged(file.toLocalFile());
}

void QFileDialogPrivate::nativeEnterDirectory(const QUrl &directory)
{
    Q_Q(QFileDialog);
    emit q->directoryUrlEntered(directory);
    if (!directory.isEmpty()) { // Windows native dialogs occasionally emit signals with empty strings.
        *lastVisitedDir() = directory;
        if (directory.isLocalFile())
            emit q->directoryEntered(directory.toLocalFile());
    }
}

/*!
    \internal

    For the list and tree view watch keys to goto parent and back in the history

    returns \c true if handled
*/
bool QFileDialogPrivate::itemViewKeyboardEvent(QKeyEvent *event) {

#if QT_CONFIG(shortcut)
    Q_Q(QFileDialog);
    if (event->matches(QKeySequence::Cancel)) {
        q->reject();
        return true;
    }
#endif
    switch (event->key()) {
    case Qt::Key_Backspace:
        navigateToParent();
        return true;
    case Qt::Key_Back:
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplicationPrivate::keypadNavigationEnabled())
            return false;
#endif
    case Qt::Key_Left:
        if (event->key() == Qt::Key_Back || event->modifiers() == Qt::AltModifier) {
            navigateBackward();
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

QString QFileDialogPrivate::getEnvironmentVariable(const QString &string)
{
#ifdef Q_OS_UNIX
    if (string.size() > 1 && string.startsWith(u'$')) {
        return qEnvironmentVariable(QStringView{string}.mid(1).toLatin1().constData());
    }
#else
    if (string.size() > 2 && string.startsWith(u'%') && string.endsWith(u'%')) {
        return qEnvironmentVariable(QStringView{string}.mid(1, string.size() - 2).toLatin1().constData());
    }
#endif
    return string;
}

void QFileDialogComboBox::setFileDialogPrivate(QFileDialogPrivate *d_pointer) {
    d_ptr = d_pointer;
    urlModel = new QUrlModel(this);
    urlModel->showFullPath = true;
    urlModel->setFileSystemModel(d_ptr->model);
    setModel(urlModel);
}

void QFileDialogComboBox::showPopup()
{
    if (model()->rowCount() > 1)
        QComboBox::showPopup();

    urlModel->setUrls(QList<QUrl>());
    QList<QUrl> list;
    QModelIndex idx = d_ptr->model->index(d_ptr->rootPath());
    while (idx.isValid()) {
        QUrl url = QUrl::fromLocalFile(idx.data(QFileSystemModel::FilePathRole).toString());
        if (url.isValid())
            list.append(url);
        idx = idx.parent();
    }
    // add "my computer"
    list.append(QUrl("file:"_L1));
    urlModel->addUrls(list, 0);
    idx = model()->index(model()->rowCount() - 1, 0);

    // append history
    QList<QUrl> urls;
    for (int i = 0; i < m_history.size(); ++i) {
        QUrl path = QUrl::fromLocalFile(m_history.at(i));
        if (!urls.contains(path))
            urls.prepend(path);
    }
    if (urls.size() > 0) {
        model()->insertRow(model()->rowCount());
        idx = model()->index(model()->rowCount()-1, 0);
        // ### TODO maybe add a horizontal line before this
        model()->setData(idx, QFileDialog::tr("Recent Places"));
        QStandardItemModel *m = qobject_cast<QStandardItemModel*>(model());
        if (m) {
            Qt::ItemFlags flags = m->flags(idx);
            flags &= ~Qt::ItemIsEnabled;
            m->item(idx.row(), idx.column())->setFlags(flags);
        }
        urlModel->addUrls(urls, -1, false);
    }
    setCurrentIndex(0);

    QComboBox::showPopup();
}

// Exact same as QComboBox::paintEvent(), except we elide the text.
void QFileDialogComboBox::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    // draw the combobox frame, focusrect and selected etc.
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QRect editRect = style()->subControlRect(QStyle::CC_ComboBox, &opt,
                                                QStyle::SC_ComboBoxEditField, this);
    int size = editRect.width() - opt.iconSize.width() - 4;
    opt.currentText = opt.fontMetrics.elidedText(opt.currentText, Qt::ElideMiddle, size);
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

void QFileDialogListView::setFileDialogPrivate(QFileDialogPrivate *d_pointer)
{
    d_ptr = d_pointer;
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setWrapping(true);
    setResizeMode(QListView::Adjust);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
#if QT_CONFIG(draganddrop)
    setDragDropMode(QAbstractItemView::InternalMove);
#endif
}

QSize QFileDialogListView::sizeHint() const
{
    int height = qMax(10, sizeHintForRow(0));
    return QSize(QListView::sizeHint().width() * 2, height * 30);
}

void QFileDialogListView::keyPressEvent(QKeyEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
        QListView::keyPressEvent(e);
        return;
    }
#endif // QT_KEYPAD_NAVIGATION

    if (!d_ptr->itemViewKeyboardEvent(e))
        QListView::keyPressEvent(e);
    e->accept();
}

void QFileDialogTreeView::setFileDialogPrivate(QFileDialogPrivate *d_pointer)
{
    d_ptr = d_pointer;
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setSortingEnabled(true);
    header()->setSortIndicator(0, Qt::AscendingOrder);
    header()->setStretchLastSection(false);
    setTextElideMode(Qt::ElideMiddle);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
#if QT_CONFIG(draganddrop)
    setDragDropMode(QAbstractItemView::InternalMove);
#endif
}

void QFileDialogTreeView::keyPressEvent(QKeyEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
        QTreeView::keyPressEvent(e);
        return;
    }
#endif // QT_KEYPAD_NAVIGATION

    if (!d_ptr->itemViewKeyboardEvent(e))
        QTreeView::keyPressEvent(e);
    e->accept();
}

QSize QFileDialogTreeView::sizeHint() const
{
    int height = qMax(10, sizeHintForRow(0));
    QSize sizeHint = header()->sizeHint();
    return QSize(sizeHint.width() * 4, height * 30);
}

/*!
    // FIXME: this is a hack to avoid propagating key press events
    // to the dialog and from there to the "Ok" button
*/
void QFileDialogLineEdit::keyPressEvent(QKeyEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
        QLineEdit::keyPressEvent(e);
        return;
    }
#endif // QT_KEYPAD_NAVIGATION

#if QT_CONFIG(shortcut)
    int key = e->key();
#endif
    QLineEdit::keyPressEvent(e);
#if QT_CONFIG(shortcut)
    if (!e->matches(QKeySequence::Cancel) && key != Qt::Key_Back)
#endif
        e->accept();
}

#if QT_CONFIG(fscompleter)

QString QFSCompleter::pathFromIndex(const QModelIndex &index) const
{
    const QFileSystemModel *dirModel;
    if (proxyModel)
        dirModel = qobject_cast<const QFileSystemModel *>(proxyModel->sourceModel());
    else
        dirModel = sourceModel;
    QString currentLocation = dirModel->rootPath();
    QString path = index.data(QFileSystemModel::FilePathRole).toString();
    if (!currentLocation.isEmpty() && path.startsWith(currentLocation)) {
#if defined(Q_OS_UNIX)
        if (currentLocation == QDir::separator())
            return path.remove(0, currentLocation.size());
#endif
        if (currentLocation.endsWith(u'/'))
            return path.remove(0, currentLocation.size());
        else
            return path.remove(0, currentLocation.size()+1);
    }
    return index.data(QFileSystemModel::FilePathRole).toString();
}

QStringList QFSCompleter::splitPath(const QString &path) const
{
    if (path.isEmpty())
        return QStringList(completionPrefix());

    QString pathCopy = QDir::toNativeSeparators(path);
    QChar sep = QDir::separator();
#if defined(Q_OS_WIN)
    if (pathCopy == "\\"_L1 || pathCopy == "\\\\"_L1)
        return QStringList(pathCopy);
    QString doubleSlash("\\\\"_L1);
    if (pathCopy.startsWith(doubleSlash))
        pathCopy = pathCopy.mid(2);
    else
        doubleSlash.clear();
#elif defined(Q_OS_UNIX)
    {
        QString tildeExpanded = qt_tildeExpansion(pathCopy);
        if (tildeExpanded != pathCopy) {
            QFileSystemModel *dirModel;
            if (proxyModel)
                dirModel = qobject_cast<QFileSystemModel *>(proxyModel->sourceModel());
            else
                dirModel = sourceModel;
            dirModel->fetchMore(dirModel->index(tildeExpanded));
        }
        pathCopy = std::move(tildeExpanded);
    }
#endif

#if defined(Q_OS_WIN)
    QStringList parts = pathCopy.split(sep, Qt::SkipEmptyParts);
    if (!doubleSlash.isEmpty() && !parts.isEmpty())
        parts[0].prepend(doubleSlash);
    if (pathCopy.endsWith(sep))
        parts.append(QString());
#else
    QStringList parts = pathCopy.split(sep);
    if (pathCopy[0] == sep) // read the "/" at the beginning as the split removed it
        parts[0] = sep;
#endif

#if defined(Q_OS_WIN)
    bool startsFromRoot = !parts.isEmpty() && parts[0].endsWith(u':');
#else
    bool startsFromRoot = pathCopy[0] == sep;
#endif
    if (parts.size() == 1 || (parts.size() > 1 && !startsFromRoot)) {
        const QFileSystemModel *dirModel;
        if (proxyModel)
            dirModel = qobject_cast<const QFileSystemModel *>(proxyModel->sourceModel());
        else
            dirModel = sourceModel;
        QString currentLocation = QDir::toNativeSeparators(dirModel->rootPath());
#if defined(Q_OS_WIN)
        if (currentLocation.endsWith(u':'))
            currentLocation.append(sep);
#endif
        if (currentLocation.contains(sep) && path != currentLocation) {
            QStringList currentLocationList = splitPath(currentLocation);
            while (!currentLocationList.isEmpty() && parts.size() > 0 && parts.at(0) == ".."_L1) {
                parts.removeFirst();
                currentLocationList.removeLast();
            }
            if (!currentLocationList.isEmpty() && currentLocationList.constLast().isEmpty())
                currentLocationList.removeLast();
            return currentLocationList + parts;
        }
    }
    return parts;
}

#endif // QT_CONFIG(completer)


QT_END_NAMESPACE

#include "moc_qfiledialog.cpp"
