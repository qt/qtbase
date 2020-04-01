/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qpa/qplatformtheme.h>

#include "qcocoafiledialoghelper.h"

/*****************************************************************************
  QFileDialog debug facilities
 *****************************************************************************/
//#define DEBUG_FILEDIALOG_FILTERS

#include <qguiapplication.h>
#include <private/qguiapplication_p.h>
#include "qcocoahelpers.h"
#include "qcocoaeventdispatcher.h"
#include <qbuffer.h>
#include <qdebug.h>
#include <qstringlist.h>
#include <qvarlengtharray.h>
#include <stdlib.h>
#include <qabstracteventdispatcher.h>
#include <qsysinfo.h>
#include <qoperatingsystemversion.h>
#include <qglobal.h>
#include <qdir.h>
#include <qregularexpression.h>

#include <qpa/qplatformnativeinterface.h>

#import <AppKit/NSSavePanel.h>
#import <CoreFoundation/CFNumber.h>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QStringList)
QT_FORWARD_DECLARE_CLASS(QFileInfo)
QT_FORWARD_DECLARE_CLASS(QWindow)
QT_USE_NAMESPACE

typedef QSharedPointer<QFileDialogOptions> SharedPointerFileDialogOptions;

@implementation QNSOpenSavePanelDelegate {
    @public
    NSOpenPanel *mOpenPanel;
    NSSavePanel *mSavePanel;
    NSView *mAccessoryView;
    NSPopUpButton *mPopUpButton;
    NSTextField *mTextField;
    QCocoaFileDialogHelper *mHelper;
    NSString *mCurrentDir;

    int mReturnCode;

    SharedPointerFileDialogOptions mOptions;
    QString *mCurrentSelection;
    QStringList *mNameFilterDropDownList;
    QStringList *mSelectedNameFilter;
}

- (instancetype)initWithAcceptMode:(const QString &)selectFile
                           options:(SharedPointerFileDialogOptions)options
                            helper:(QCocoaFileDialogHelper *)helper
{
    self = [super init];
    mOptions = options;
    if (mOptions->acceptMode() == QFileDialogOptions::AcceptOpen){
        mOpenPanel = [NSOpenPanel openPanel];
        mSavePanel = mOpenPanel;
    } else {
        mSavePanel = [NSSavePanel savePanel];
        [mSavePanel setCanSelectHiddenExtension:YES];
        mOpenPanel = nil;
    }

    if ([mSavePanel respondsToSelector:@selector(setLevel:)])
        [mSavePanel setLevel:NSModalPanelWindowLevel];

    mReturnCode = -1;
    mHelper = helper;
    mNameFilterDropDownList = new QStringList(mOptions->nameFilters());
    QString selectedVisualNameFilter = mOptions->initiallySelectedNameFilter();
    mSelectedNameFilter = new QStringList([self findStrippedFilterWithVisualFilterName:selectedVisualNameFilter]);

    QFileInfo sel(selectFile);
    if (sel.isDir() && !sel.isBundle()){
        mCurrentDir = [sel.absoluteFilePath().toNSString() retain];
        mCurrentSelection = new QString;
    } else {
        mCurrentDir = [sel.absolutePath().toNSString() retain];
        mCurrentSelection = new QString(sel.absoluteFilePath());
    }

    [mSavePanel setTitle:options->windowTitle().toNSString()];
    [self createPopUpButton:selectedVisualNameFilter hideDetails:options->testOption(QFileDialogOptions::HideNameFilterDetails)];
    [self createTextField];
    [self createAccessory];
    [mSavePanel setAccessoryView:mNameFilterDropDownList->size() > 1 ? mAccessoryView : nil];
    // -setAccessoryView: can result in -panel:directoryDidChange:
    // resetting our mCurrentDir, set the delegate
    // here to make sure it gets the correct value.
    [mSavePanel setDelegate:self];
    mOpenPanel.accessoryViewDisclosed = YES;

    if (mOptions->isLabelExplicitlySet(QFileDialogOptions::Accept))
        [mSavePanel setPrompt:[self strip:options->labelText(QFileDialogOptions::Accept)]];
    if (mOptions->isLabelExplicitlySet(QFileDialogOptions::FileName))
        [mSavePanel setNameFieldLabel:[self strip:options->labelText(QFileDialogOptions::FileName)]];

    [self updateProperties];
    [mSavePanel retain];
    return self;
}

- (void)dealloc
{
    delete mNameFilterDropDownList;
    delete mSelectedNameFilter;
    delete mCurrentSelection;

    if ([mSavePanel respondsToSelector:@selector(orderOut:)])
        [mSavePanel orderOut:mSavePanel];
    [mSavePanel setAccessoryView:nil];
    [mPopUpButton release];
    [mTextField release];
    [mAccessoryView release];
    [mSavePanel setDelegate:nil];
    [mSavePanel release];
    [mCurrentDir release];
    [super dealloc];
}

static QString strippedText(QString s)
{
    s.remove(QLatin1String("..."));
    return QPlatformTheme::removeMnemonics(s).trimmed();
}

- (NSString *)strip:(const QString &)label
{
    return strippedText(label).toNSString();
}

- (void)closePanel
{
    *mCurrentSelection = QString::fromNSString([[mSavePanel URL] path]).normalized(QString::NormalizationForm_C);
    if ([mSavePanel respondsToSelector:@selector(close)])
        [mSavePanel close];
    if ([mSavePanel isSheet])
        [NSApp endSheet: mSavePanel];
}

- (void)showModelessPanel
{
    if (mOpenPanel){
        QFileInfo info(*mCurrentSelection);
        NSString *filepath = info.filePath().toNSString();
        NSURL *url = [NSURL fileURLWithPath:filepath isDirectory:info.isDir()];
        bool selectable = (mOptions->acceptMode() == QFileDialogOptions::AcceptSave)
            || [self panel:mOpenPanel shouldEnableURL:url];

        [self updateProperties];
        [mSavePanel setNameFieldStringValue:selectable ? info.fileName().toNSString() : @""];

        [mOpenPanel beginWithCompletionHandler:^(NSInteger result){
            mReturnCode = result;
            if (mHelper)
                mHelper->QNSOpenSavePanelDelegate_panelClosed(result == NSModalResponseOK);
        }];
    }
}

- (BOOL)runApplicationModalPanel
{
    QFileInfo info(*mCurrentSelection);
    NSString *filepath = info.filePath().toNSString();
    NSURL *url = [NSURL fileURLWithPath:filepath isDirectory:info.isDir()];
    bool selectable = (mOptions->acceptMode() == QFileDialogOptions::AcceptSave)
        || [self panel:mSavePanel shouldEnableURL:url];

    [mSavePanel setDirectoryURL: [NSURL fileURLWithPath:mCurrentDir]];
    [mSavePanel setNameFieldStringValue:selectable ? info.fileName().toNSString() : @""];

    // Call processEvents in case the event dispatcher has been interrupted, and needs to do
    // cleanup of modal sessions. Do this before showing the native dialog, otherwise it will
    // close down during the cleanup.
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    // Make sure we don't interrupt the runModal call below.
    QCocoaEventDispatcher::clearCurrentThreadCocoaEventDispatcherInterruptFlag();

    mReturnCode = [mSavePanel runModal];

    QAbstractEventDispatcher::instance()->interrupt();
    return (mReturnCode == NSModalResponseOK);
}

- (QPlatformDialogHelper::DialogCode)dialogResultCode
{
    return (mReturnCode == NSModalResponseOK) ? QPlatformDialogHelper::Accepted : QPlatformDialogHelper::Rejected;
}

- (void)showWindowModalSheet:(QWindow *)parent
{
    QFileInfo info(*mCurrentSelection);
    NSString *filepath = info.filePath().toNSString();
    NSURL *url = [NSURL fileURLWithPath:filepath isDirectory:info.isDir()];
    bool selectable = (mOptions->acceptMode() == QFileDialogOptions::AcceptSave)
        || [self panel:mSavePanel shouldEnableURL:url];

    [self updateProperties];
    [mSavePanel setDirectoryURL: [NSURL fileURLWithPath:mCurrentDir]];

    [mSavePanel setNameFieldStringValue:selectable ? info.fileName().toNSString() : @""];
    NSWindow *nsparent = static_cast<NSWindow *>(qGuiApp->platformNativeInterface()->nativeResourceForWindow("nswindow", parent));

    [mSavePanel beginSheetModalForWindow:nsparent completionHandler:^(NSInteger result){
        mReturnCode = result;
        if (mHelper)
            mHelper->QNSOpenSavePanelDelegate_panelClosed(result == NSModalResponseOK);
    }];
}

- (BOOL)isHiddenFileAtURL:(NSURL *)url
{
    BOOL hidden = NO;
    if (url) {
        CFBooleanRef isHiddenProperty;
        if (CFURLCopyResourcePropertyForKey((__bridge CFURLRef)url, kCFURLIsHiddenKey, &isHiddenProperty, nullptr)) {
            hidden = CFBooleanGetValue(isHiddenProperty);
            CFRelease(isHiddenProperty);
        }
    }
    return hidden;
}

- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url
{
    Q_UNUSED(sender);

    NSString *filename = [url path];
    if ([filename length] == 0)
        return NO;

    // Always accept directories regardless of their names (unless it is a bundle):
    NSFileManager *fm = [NSFileManager defaultManager];
    NSDictionary *fileAttrs = [fm attributesOfItemAtPath:filename error:nil];
    if (!fileAttrs)
        return NO; // Error accessing the file means 'no'.
    NSString *fileType = [fileAttrs fileType];
    bool isDir = [fileType isEqualToString:NSFileTypeDirectory];
    if (isDir) {
        if ([mSavePanel treatsFilePackagesAsDirectories] == NO) {
            if ([[NSWorkspace sharedWorkspace] isFilePackageAtPath:filename] == NO)
                return YES;
        }
    }

    QString qtFileName = QFileInfo(QString::fromNSString(filename)).fileName();
    // No filter means accept everything
    bool nameMatches = mSelectedNameFilter->isEmpty();
    // Check if the current file name filter accepts the file:
    for (int i = 0; !nameMatches && i < mSelectedNameFilter->size(); ++i) {
        if (QDir::match(mSelectedNameFilter->at(i), qtFileName))
            nameMatches = true;
    }
    if (!nameMatches)
        return NO;

    QDir::Filters filter = mOptions->filter();
    if ((!(filter & (QDir::Dirs | QDir::AllDirs)) && isDir)
        || (!(filter & QDir::Files) && [fileType isEqualToString:NSFileTypeRegular])
        || ((filter & QDir::NoSymLinks) && [fileType isEqualToString:NSFileTypeSymbolicLink]))
        return NO;

    bool filterPermissions = ((filter & QDir::PermissionMask)
                              && (filter & QDir::PermissionMask) != QDir::PermissionMask);
    if (filterPermissions) {
        if ((!(filter & QDir::Readable) && [fm isReadableFileAtPath:filename])
            || (!(filter & QDir::Writable) && [fm isWritableFileAtPath:filename])
            || (!(filter & QDir::Executable) && [fm isExecutableFileAtPath:filename]))
            return NO;
    }
    if (!(filter & QDir::Hidden)
        && (qtFileName.startsWith(QLatin1Char('.')) || [self isHiddenFileAtURL:url]))
            return NO;

    return YES;
}

- (NSString *)panel:(id)sender userEnteredFilename:(NSString *)filename confirmed:(BOOL)okFlag
{
    Q_UNUSED(sender);
    if (!okFlag)
        return filename;
    if (!mOptions->testOption(QFileDialogOptions::DontConfirmOverwrite))
        return filename;

    // User has clicked save, and no overwrite confirmation should occur.
    // To get the latter, we need to change the name we return (hence the prefix):
    return [@"___qt_very_unlikely_prefix_" stringByAppendingString:filename];
}

- (void)setNameFilters:(const QStringList &)filters hideDetails:(BOOL)hideDetails
{
    [mPopUpButton removeAllItems];
    *mNameFilterDropDownList = filters;
    if (filters.size() > 0){
        for (int i=0; i<filters.size(); ++i) {
            QString filter = hideDetails ? [self removeExtensions:filters.at(i)] : filters.at(i);
            [mPopUpButton addItemWithTitle:filter.toNSString()];
        }
        [mPopUpButton selectItemAtIndex:0];
        [mSavePanel setAccessoryView:mAccessoryView];
    } else
        [mSavePanel setAccessoryView:nil];

    [self filterChanged:self];
}

- (void)filterChanged:(id)sender
{
    // This mDelegate function is called when the _name_ filter changes.
    Q_UNUSED(sender);
    QString selection = mNameFilterDropDownList->value([mPopUpButton indexOfSelectedItem]);
    *mSelectedNameFilter = [self findStrippedFilterWithVisualFilterName:selection];
    if ([mSavePanel respondsToSelector:@selector(validateVisibleColumns:)])
        [mSavePanel validateVisibleColumns];
    [self updateProperties];
    if (mHelper)
        mHelper->QNSOpenSavePanelDelegate_filterSelected([mPopUpButton indexOfSelectedItem]);
}

- (QString)currentNameFilter
{
    return mNameFilterDropDownList->value([mPopUpButton indexOfSelectedItem]);
}

- (QList<QUrl>)selectedFiles
{
    if (mOpenPanel) {
        QList<QUrl> result;
        NSArray<NSURL *> *array = [mOpenPanel URLs];
        for (NSURL *url in array) {
            QString path = QString::fromNSString(url.path).normalized(QString::NormalizationForm_C);
            result << QUrl::fromLocalFile(path);
        }
        return result;
    } else {
        QList<QUrl> result;
        QString filename = QString::fromNSString([[mSavePanel URL] path]).normalized(QString::NormalizationForm_C);
        const QString defaultSuffix = mOptions->defaultSuffix();
        const QFileInfo fileInfo(filename);
        // If neither the user or the NSSavePanel have provided a suffix, use
        // the default suffix (if it exists).
        if (fileInfo.suffix().isEmpty() && !defaultSuffix.isEmpty()) {
                filename.append('.').append(defaultSuffix);
        }
        result << QUrl::fromLocalFile(filename.remove(QLatin1String("___qt_very_unlikely_prefix_")));
        return result;
    }
}

- (void)updateProperties
{
    // Call this functions if mFileMode, mFileOptions,
    // mNameFilterDropDownList or mQDirFilter changes.
    // The savepanel does not contain the necessary functions for this.
    const QFileDialogOptions::FileMode fileMode = mOptions->fileMode();
    bool chooseFilesOnly = fileMode == QFileDialogOptions::ExistingFile
        || fileMode == QFileDialogOptions::ExistingFiles;
    bool chooseDirsOnly = fileMode == QFileDialogOptions::Directory
        || fileMode == QFileDialogOptions::DirectoryOnly
        || mOptions->testOption(QFileDialogOptions::ShowDirsOnly);

    [mOpenPanel setCanChooseFiles:!chooseDirsOnly];
    [mOpenPanel setCanChooseDirectories:!chooseFilesOnly];
    [mSavePanel setCanCreateDirectories:!(mOptions->testOption(QFileDialogOptions::ReadOnly))];
    [mOpenPanel setAllowsMultipleSelection:(fileMode == QFileDialogOptions::ExistingFiles)];
    [mOpenPanel setResolvesAliases:!(mOptions->testOption(QFileDialogOptions::DontResolveSymlinks))];
    [mOpenPanel setTitle:mOptions->windowTitle().toNSString()];
    [mSavePanel setTitle:mOptions->windowTitle().toNSString()];
    [mPopUpButton setHidden:chooseDirsOnly];    // TODO hide the whole sunken pane instead?

    if (mOptions->acceptMode() == QFileDialogOptions::AcceptSave) {
        [self recomputeAcceptableExtensionsForSave];
    } else {
        [mOpenPanel setAllowedFileTypes:nil]; // delegate panel:shouldEnableURL: does the file filtering for NSOpenPanel
    }

    if ([mSavePanel respondsToSelector:@selector(isVisible)] && [mSavePanel isVisible]) {
        if ([mSavePanel respondsToSelector:@selector(validateVisibleColumns)])
            [mSavePanel validateVisibleColumns];
    }
}

- (void)panelSelectionDidChange:(id)sender
{
    Q_UNUSED(sender);
    if (mHelper && [mSavePanel isVisible]) {
        QString selection = QString::fromNSString([[mSavePanel URL] path]);
        if (selection != mCurrentSelection) {
            *mCurrentSelection = selection;
            mHelper->QNSOpenSavePanelDelegate_selectionChanged(selection);
        }
    }
}

- (void)panel:(id)sender directoryDidChange:(NSString *)path
{
    Q_UNUSED(sender);
    if (!mHelper)
        return;
    if (!(path && path.length) || [path isEqualToString:mCurrentDir])
        return;

    [mCurrentDir release];
    mCurrentDir = [path retain];
    mHelper->QNSOpenSavePanelDelegate_directoryEntered(QString::fromNSString(mCurrentDir));
}

/*
    Computes a list of extensions (e.g. "png", "jpg", "gif")
    for the current name filter, and updates the save panel.

    If a filter do not conform to the format *.xyz or * or *.*,
    all files types are allowed.

    Extensions with more than one part (e.g. "tar.gz") are
    reduced to their final part, as NSSavePanel does not deal
    well with multi-part extensions.
*/
- (void)recomputeAcceptableExtensionsForSave
{
    QStringList fileTypes;
    for (const QString &filter : *mSelectedNameFilter) {
        if (!filter.startsWith(QLatin1String("*.")))
            continue;

        if (filter.contains(QLatin1Char('?')))
            continue;

        if (filter.count(QLatin1Char('*')) != 1)
            continue;

        auto extensions = filter.split('.', Qt::SkipEmptyParts);
        fileTypes += extensions.last();

        // Explicitly show extensions if we detect a filter
        // that has a multi-part extension. This prevents
        // confusing situations where the user clicks e.g.
        // 'foo.tar.gz' and 'foo.tar' is populated in the
        // file name box, but when then clicking save macOS
        // will warn that the file needs to end in .gz,
        // due to thinking the user tried to save the file
        // as a 'tar' file instead. Unfortunately this
        // property can only be set before the panel is
        // shown, so it will not have any effect when
        // swithcing filters in an already opened dialog.
        if (extensions.size() > 2)
            mSavePanel.extensionHidden = NO;
    }

    mSavePanel.allowedFileTypes = fileTypes.isEmpty() ? nil
        : qt_mac_QStringListToNSMutableArray(fileTypes);
}

- (QString)removeExtensions:(const QString &)filter
{
    QRegularExpression regExp(QString::fromLatin1(QPlatformFileDialogHelper::filterRegExp));
    QRegularExpressionMatch match = regExp.match(filter);
    if (match.hasMatch())
        return match.captured(1).trimmed();
    return filter;
}

- (void)createTextField
{
    NSRect textRect = { { 0.0, 3.0 }, { 100.0, 25.0 } };
    mTextField = [[NSTextField alloc] initWithFrame:textRect];
    [[mTextField cell] setFont:[NSFont systemFontOfSize:
            [NSFont systemFontSizeForControlSize:NSControlSizeRegular]]];
    [mTextField setAlignment:NSTextAlignmentRight];
    [mTextField setEditable:false];
    [mTextField setSelectable:false];
    [mTextField setBordered:false];
    [mTextField setDrawsBackground:false];
    if (mOptions->isLabelExplicitlySet(QFileDialogOptions::FileType))
        [mTextField setStringValue:[self strip:mOptions->labelText(QFileDialogOptions::FileType)]];
}

- (void)createPopUpButton:(const QString &)selectedFilter hideDetails:(BOOL)hideDetails
{
    NSRect popUpRect = { { 100.0, 5.0 }, { 250.0, 25.0 } };
    mPopUpButton = [[NSPopUpButton alloc] initWithFrame:popUpRect pullsDown:NO];
    [mPopUpButton setTarget:self];
    [mPopUpButton setAction:@selector(filterChanged:)];

    if (mNameFilterDropDownList->size() > 0) {
        int filterToUse = -1;
        for (int i=0; i<mNameFilterDropDownList->size(); ++i) {
            QString currentFilter = mNameFilterDropDownList->at(i);
            if (selectedFilter == currentFilter ||
                (filterToUse == -1 && currentFilter.startsWith(selectedFilter)))
                filterToUse = i;
            QString filter = hideDetails ? [self removeExtensions:currentFilter] : currentFilter;
            [mPopUpButton addItemWithTitle:filter.toNSString()];
        }
        if (filterToUse != -1)
            [mPopUpButton selectItemAtIndex:filterToUse];
    }
}

- (QStringList) findStrippedFilterWithVisualFilterName:(QString)name
{
    for (int i=0; i<mNameFilterDropDownList->size(); ++i) {
        if (mNameFilterDropDownList->at(i).startsWith(name))
            return QPlatformFileDialogHelper::cleanFilterList(mNameFilterDropDownList->at(i));
    }
    return QStringList();
}

- (void)createAccessory
{
    NSRect accessoryRect = { { 0.0, 0.0 }, { 450.0, 33.0 } };
    mAccessoryView = [[NSView alloc] initWithFrame:accessoryRect];
    [mAccessoryView addSubview:mTextField];
    [mAccessoryView addSubview:mPopUpButton];
}

@end

QT_BEGIN_NAMESPACE

QCocoaFileDialogHelper::QCocoaFileDialogHelper()
    : mDelegate(nil)
{
}

QCocoaFileDialogHelper::~QCocoaFileDialogHelper()
{
    if (!mDelegate)
        return;
    QMacAutoReleasePool pool;
    [mDelegate release];
    mDelegate = nil;
}

void QCocoaFileDialogHelper::QNSOpenSavePanelDelegate_selectionChanged(const QString &newPath)
{
    emit currentChanged(QUrl::fromLocalFile(newPath));
}

void QCocoaFileDialogHelper::QNSOpenSavePanelDelegate_panelClosed(bool accepted)
{
    if (accepted) {
        emit accept();
    } else {
        emit reject();
    }
}

void QCocoaFileDialogHelper::QNSOpenSavePanelDelegate_directoryEntered(const QString &newDir)
{
    // ### fixme: priv->setLastVisitedDirectory(newDir);
    emit directoryEntered(QUrl::fromLocalFile(newDir));
}

void QCocoaFileDialogHelper::QNSOpenSavePanelDelegate_filterSelected(int menuIndex)
{
    const QStringList filters = options()->nameFilters();
    emit filterSelected(menuIndex >= 0 && menuIndex < filters.size() ? filters.at(menuIndex) : QString());
}

void QCocoaFileDialogHelper::setDirectory(const QUrl &directory)
{
    if (mDelegate)
        [mDelegate->mSavePanel setDirectoryURL:[NSURL fileURLWithPath:directory.toLocalFile().toNSString()]];
    else
        mDir = directory;
}

QUrl QCocoaFileDialogHelper::directory() const
{
    if (mDelegate) {
        QString path = QString::fromNSString([[mDelegate->mSavePanel directoryURL] path]).normalized(QString::NormalizationForm_C);
        return QUrl::fromLocalFile(path);
    }
    return mDir;
}

void QCocoaFileDialogHelper::selectFile(const QUrl &filename)
{
    QString filePath = filename.toLocalFile();
    if (QDir::isRelativePath(filePath))
        filePath = QFileInfo(directory().toLocalFile(), filePath).filePath();

    // There seems to no way to select a file once the dialog is running.
    // So do the next best thing, set the file's directory:
    setDirectory(QFileInfo(filePath).absolutePath());
}

QList<QUrl> QCocoaFileDialogHelper::selectedFiles() const
{
    if (mDelegate)
        return [mDelegate selectedFiles];
    return QList<QUrl>();
}

void QCocoaFileDialogHelper::setFilter()
{
    if (!mDelegate)
        return;
    const SharedPointerFileDialogOptions &opts = options();
    [mDelegate->mSavePanel setTitle:opts->windowTitle().toNSString()];
    if (opts->isLabelExplicitlySet(QFileDialogOptions::Accept))
        [mDelegate->mSavePanel setPrompt:[mDelegate strip:opts->labelText(QFileDialogOptions::Accept)]];
    if (opts->isLabelExplicitlySet(QFileDialogOptions::FileName))
        [mDelegate->mSavePanel setNameFieldLabel:[mDelegate strip:opts->labelText(QFileDialogOptions::FileName)]];

    [mDelegate updateProperties];
}

void QCocoaFileDialogHelper::selectNameFilter(const QString &filter)
{
    if (!options())
        return;
    const int index = options()->nameFilters().indexOf(filter);
    if (index != -1) {
        if (!mDelegate) {
            options()->setInitiallySelectedNameFilter(filter);
            return;
        }
        [mDelegate->mPopUpButton selectItemAtIndex:index];
        [mDelegate filterChanged:nil];
    }
}

QString QCocoaFileDialogHelper::selectedNameFilter() const
{
    if (!mDelegate)
        return options()->initiallySelectedNameFilter();
    int index = [mDelegate->mPopUpButton indexOfSelectedItem];
    if (index >= options()->nameFilters().count())
        return QString();
    return index != -1 ? options()->nameFilters().at(index) : QString();
}

void QCocoaFileDialogHelper::hide()
{
    hideCocoaFilePanel();
}

bool QCocoaFileDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
//    Q_Q(QFileDialog);
    if (windowFlags & Qt::WindowStaysOnTopHint) {
        // The native file dialog tries all it can to stay
        // on the NSModalPanel level. And it might also show
        // its own "create directory" dialog that we cannot control.
        // So we need to use the non-native version in this case...
        return false;
    }

    return showCocoaFilePanel(windowModality, parent);
}

void QCocoaFileDialogHelper::createNSOpenSavePanelDelegate()
{
    QMacAutoReleasePool pool;

    const SharedPointerFileDialogOptions &opts = options();
    const QList<QUrl> selectedFiles = opts->initiallySelectedFiles();
    const QUrl directory = mDir.isEmpty() ? opts->initialDirectory() : mDir;
    const bool selectDir = selectedFiles.isEmpty();
    QString selection(selectDir ? directory.toLocalFile() : selectedFiles.front().toLocalFile());
    QNSOpenSavePanelDelegate *delegate = [[QNSOpenSavePanelDelegate alloc]
        initWithAcceptMode:
            selection
            options:opts
            helper:this];

    [static_cast<QNSOpenSavePanelDelegate *>(mDelegate) release];
    mDelegate = delegate;
}

bool QCocoaFileDialogHelper::showCocoaFilePanel(Qt::WindowModality windowModality, QWindow *parent)
{
    createNSOpenSavePanelDelegate();
    if (!mDelegate)
        return false;
    if (windowModality == Qt::NonModal)
        [mDelegate showModelessPanel];
    else if (windowModality == Qt::WindowModal && parent)
        [mDelegate showWindowModalSheet:parent];
    // no need to show a Qt::ApplicationModal dialog here, since it will be done in _q_platformRunNativeAppModalPanel()
    return true;
}

bool QCocoaFileDialogHelper::hideCocoaFilePanel()
{
    if (!mDelegate){
        // Nothing to do. We return false to leave the question
        // open regarding whether or not to go native:
        return false;
    } else {
        [mDelegate closePanel];
        // Even when we hide it, we are still using a
        // native dialog, so return true:
        return true;
    }
}

void QCocoaFileDialogHelper::exec()
{
    // Note: If NSApp is not running (which is the case if e.g a top-most
    // QEventLoop has been interrupted, and the second-most event loop has not
    // yet been reactivated (regardless if [NSApp run] is still on the stack)),
    // showing a native modal dialog will fail.
    QMacAutoReleasePool pool;
    if ([mDelegate runApplicationModalPanel])
        emit accept();
    else
        emit reject();

}

bool QCocoaFileDialogHelper::defaultNameFilterDisables() const
{
    return true;
}

QT_END_NAMESPACE
