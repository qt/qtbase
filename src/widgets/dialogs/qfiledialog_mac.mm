/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfiledialog.h"

#ifndef QT_NO_FILEDIALOG

/*****************************************************************************
  QFileDialog debug facilities
 *****************************************************************************/
//#define DEBUG_FILEDIALOG_FILTERS

#include <qapplication.h>
#include <private/qapplication_p.h>
#include <private/qfiledialog_p.h>
#include <private/qt_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <qregexp.h>
#include <qbuffer.h>
#include <qdebug.h>
#include <qstringlist.h>
#include <qaction.h>
#include <qtextcodec.h>
#include <qvarlengtharray.h>
#include <qdesktopwidget.h>
#include <stdlib.h>
#include <qabstracteventdispatcher.h>
#import <AppKit/NSSavePanel.h>
#include "ui_qfiledialog.h"

QT_BEGIN_NAMESPACE

extern QStringList qt_make_filter_list(const QString &filter); // qfiledialog.cpp
extern QStringList qt_clean_filter_list(const QString &filter); // qfiledialog.cpp
extern const char *qt_file_dialog_filter_reg_exp; // qfiledialog.cpp
extern bool qt_mac_is_macsheet(const QWidget *w); // qwidget_mac.mm

QT_END_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QFileDialogPrivate)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QStringList)
QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QFileInfo)
QT_USE_NAMESPACE

@class QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate);

@interface QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate)
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
    : NSObject<NSOpenSavePanelDelegate>
#else
    : NSObject
#endif
{
    @public
    NSOpenPanel *mOpenPanel;
    NSSavePanel *mSavePanel;
    NSView *mAccessoryView;
    NSPopUpButton *mPopUpButton;
    NSTextField *mTextField;
    QFileDialogPrivate *mPriv;
    NSString *mCurrentDir;
    bool mConfirmOverwrite;
    int mReturnCode;

    QT_PREPEND_NAMESPACE(QFileDialog::AcceptMode) mAcceptMode;
    QT_PREPEND_NAMESPACE(QDir::Filters) *mQDirFilter;
    QT_PREPEND_NAMESPACE(QFileDialog::FileMode) mFileMode;
    QT_PREPEND_NAMESPACE(QFileDialog::Options) *mFileOptions;

    QString *mLastFilterCheckPath;
    QString *mCurrentSelection;
    QStringList *mQDirFilterEntryList;
    QStringList *mNameFilterDropDownList;
    QStringList *mSelectedNameFilter;
}

- (NSString *)strip:(const QString &)label;
- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename;
- (void)filterChanged:(id)sender;
- (void)showModelessPanel;
- (BOOL)runApplicationModalPanel;
- (void)showWindowModalSheet:(QWidget *)docWidget;
- (void)updateProperties;
- (QStringList)acceptableExtensionsForSave;
- (QString)removeExtensions:(const QString &)filter;
- (void)createTextField;
- (void)createPopUpButton:(const QString &)selectedFilter hideDetails:(BOOL)hideDetails;
- (QStringList)findStrippedFilterWithVisualFilterName:(QString)name;
- (void)createAccessory;

@end

@implementation QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate)

- (id)initWithAcceptMode:(QT_PREPEND_NAMESPACE(QFileDialog::AcceptMode))acceptMode
    title:(const QString &)title
    hideNameFilterDetails:(bool)hideNameFilterDetails
    qDirFilter:(QT_PREPEND_NAMESPACE(QDir::Filters))qDirFilter
    fileOptions:(QT_PREPEND_NAMESPACE(QFileDialog::Options))fileOptions
    fileMode:(QT_PREPEND_NAMESPACE(QFileDialog::FileMode))fileMode
    selectFile:(const QString &)selectFile
    confirmOverwrite:(bool)confirm
    priv:(QFileDialogPrivate *)priv
{
    self = [super init];

    mAcceptMode = acceptMode;
    if (mAcceptMode == QT_PREPEND_NAMESPACE(QFileDialog::AcceptOpen)){
        mOpenPanel = [NSOpenPanel openPanel];
        mSavePanel = mOpenPanel;
    } else {
        mSavePanel = [NSSavePanel savePanel];
        mOpenPanel = 0;
    }

    [mSavePanel setLevel:NSModalPanelWindowLevel];
    [mSavePanel setDelegate:self];
    mQDirFilter = new QT_PREPEND_NAMESPACE(QDir::Filters)(qDirFilter);
    mFileOptions = new QT_PREPEND_NAMESPACE(QFileDialog::Options)(fileOptions);
    mFileMode = fileMode;
    mConfirmOverwrite = confirm;
    mReturnCode = -1;
    mPriv = priv;
    mLastFilterCheckPath = new QString;
    mQDirFilterEntryList = new QStringList;
    mNameFilterDropDownList = new QStringList(priv->nameFilters);
    QString selectedVisualNameFilter = priv->qFileDialogUi->fileTypeCombo->currentText();
    mSelectedNameFilter = new QStringList([self findStrippedFilterWithVisualFilterName:selectedVisualNameFilter]);

    QFileInfo sel(selectFile);
    if (sel.isDir()){
        mCurrentDir = [qt_mac_QStringToNSString(sel.absoluteFilePath()) retain];
        mCurrentSelection = new QString;
    } else {
        mCurrentDir = [qt_mac_QStringToNSString(sel.absolutePath()) retain];
        mCurrentSelection = new QString(sel.absoluteFilePath());
    }

    [mSavePanel setTitle:qt_mac_QStringToNSString(title)];
    [self createPopUpButton:selectedVisualNameFilter hideDetails:hideNameFilterDetails];
    [self createTextField];
    [self createAccessory];
    [mSavePanel setAccessoryView:mNameFilterDropDownList->size() > 1 ? mAccessoryView : nil];

    if (mPriv){
        [mSavePanel setPrompt:[self strip:mPriv->acceptLabel]];
        if (mPriv->fileNameLabelExplicitlySat)
            [mSavePanel setNameFieldLabel:[self strip:mPriv->qFileDialogUi->fileNameLabel->text()]];
    }

    [self updateProperties];
    [mSavePanel retain];
    return self;
}

- (void)dealloc
{
    delete mQDirFilter;
    delete mFileOptions;
    delete mLastFilterCheckPath;
    delete mQDirFilterEntryList;
    delete mNameFilterDropDownList;
    delete mSelectedNameFilter;
    delete mCurrentSelection;

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

- (NSString *)strip:(const QString &)label
{
    QAction a(label, 0);
    return qt_mac_QStringToNSString(a.iconText());
}

- (void)closePanel
{
    *mCurrentSelection = QT_PREPEND_NAMESPACE(qt_mac_NSStringToQString)([mSavePanel filename]);
    [mSavePanel close];
}

- (void)showModelessPanel
{
    if (mOpenPanel){
        QFileInfo info(*mCurrentSelection);
        NSString *filename = QT_PREPEND_NAMESPACE(qt_mac_QStringToNSString)(info.fileName());
        NSString *filepath = QT_PREPEND_NAMESPACE(qt_mac_QStringToNSString)(info.filePath());
        bool selectable = (mAcceptMode == QFileDialog::AcceptSave)
            || [self panel:nil shouldShowFilename:filepath];
        [mOpenPanel 
            beginForDirectory:mCurrentDir
            file:selectable ? filename : nil
            types:nil
            modelessDelegate:self
            didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
            contextInfo:nil];
    }
}

- (BOOL)runApplicationModalPanel
{
    QFileInfo info(*mCurrentSelection);
    NSString *filename = QT_PREPEND_NAMESPACE(qt_mac_QStringToNSString)(info.fileName());
    NSString *filepath = QT_PREPEND_NAMESPACE(qt_mac_QStringToNSString)(info.filePath());
    bool selectable = (mAcceptMode == QFileDialog::AcceptSave)
        || [self panel:nil shouldShowFilename:filepath];
    mReturnCode = [mSavePanel 
        runModalForDirectory:mCurrentDir
        file:selectable ? filename : @"untitled"];

    QAbstractEventDispatcher::instance()->interrupt();
    return (mReturnCode == NSOKButton);
}

- (QT_PREPEND_NAMESPACE(QDialog::DialogCode))dialogResultCode
{
    return (mReturnCode == NSOKButton) ? QT_PREPEND_NAMESPACE(QDialog::Accepted) : QT_PREPEND_NAMESPACE(QDialog::Rejected);
}

- (void)showWindowModalSheet:(QWidget *)docWidget
{
    Q_UNUSED(docWidget);
    QFileInfo info(*mCurrentSelection);
    NSString *filename = QT_PREPEND_NAMESPACE(qt_mac_QStringToNSString)(info.fileName());
    NSString *filepath = QT_PREPEND_NAMESPACE(qt_mac_QStringToNSString)(info.filePath());
    bool selectable = (mAcceptMode == QFileDialog::AcceptSave)
        || [self panel:nil shouldShowFilename:filepath];
    [mSavePanel 
        beginSheetForDirectory:mCurrentDir
        file:selectable ? filename : nil
        modalForWindow:QT_PREPEND_NAMESPACE(qt_mac_window_for)(docWidget)
        modalDelegate:self
        didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
        contextInfo:nil];
}

- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename
{
    Q_UNUSED(sender);

    if ([filename length] == 0)
        return NO;

    // Always accept directories regardless of their names (unless it is a bundle):
    BOOL isDir;
    if ([[NSFileManager defaultManager] fileExistsAtPath:filename isDirectory:&isDir] && isDir) {
        if ([mSavePanel treatsFilePackagesAsDirectories] == NO) {
            if ([[NSWorkspace sharedWorkspace] isFilePackageAtPath:filename] == NO)
                return YES;
        }
    }

    QString qtFileName = QT_PREPEND_NAMESPACE(qt_mac_NSStringToQString)(filename);
    QFileInfo info(qtFileName.normalized(QT_PREPEND_NAMESPACE(QString::NormalizationForm_C)));
    QString path = info.absolutePath();
    if (path != *mLastFilterCheckPath){
        *mLastFilterCheckPath = path;
        *mQDirFilterEntryList = info.dir().entryList(*mQDirFilter);
    }
    // Check if the QDir filter accepts the file:
    if (!mQDirFilterEntryList->contains(info.fileName()))
        return NO;

    // No filter means accept everything
    if (mSelectedNameFilter->isEmpty())
        return YES;
    // Check if the current file name filter accepts the file:
    for (int i=0; i<mSelectedNameFilter->size(); ++i) {
        if (QDir::match(mSelectedNameFilter->at(i), qtFileName))
            return YES;
    }
    return NO;
}

- (NSString *)panel:(id)sender userEnteredFilename:(NSString *)filename confirmed:(BOOL)okFlag
{
    Q_UNUSED(sender);
    if (!okFlag)
        return filename;
    if (mConfirmOverwrite)
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
            [mPopUpButton addItemWithTitle:QT_PREPEND_NAMESPACE(qt_mac_QStringToNSString)(filter)];
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
    [mSavePanel validateVisibleColumns];
    [self updateProperties];
    if (mPriv)
        mPriv->QNSOpenSavePanelDelegate_filterSelected([mPopUpButton indexOfSelectedItem]);
}

- (QString)currentNameFilter
{
    return mNameFilterDropDownList->value([mPopUpButton indexOfSelectedItem]);
}

- (QStringList)selectedFiles
{
    if (mOpenPanel)
        return QT_PREPEND_NAMESPACE(qt_mac_NSArrayToQStringList)([mOpenPanel filenames]);
    else{
        QStringList result;
        QString filename = QT_PREPEND_NAMESPACE(qt_mac_NSStringToQString)([mSavePanel filename]);
        result << filename.remove(QLatin1String("___qt_very_unlikely_prefix_"));
        return result;
    }
}

- (void)updateProperties
{
    // Call this functions if mFileMode, mFileOptions,
    // mNameFilterDropDownList or mQDirFilter changes.
    // The savepanel does not contain the neccessary functions for this.
    bool chooseFilesOnly = mFileMode == QT_PREPEND_NAMESPACE(QFileDialog::ExistingFile)
        || mFileMode == QT_PREPEND_NAMESPACE(QFileDialog::ExistingFiles);
    bool chooseDirsOnly = mFileMode == QT_PREPEND_NAMESPACE(QFileDialog::Directory)
        || mFileMode == QT_PREPEND_NAMESPACE(QFileDialog::DirectoryOnly)
        || *mFileOptions & QT_PREPEND_NAMESPACE(QFileDialog::ShowDirsOnly);

    [mOpenPanel setCanChooseFiles:!chooseDirsOnly];
    [mOpenPanel setCanChooseDirectories:!chooseFilesOnly];
    [mSavePanel setCanCreateDirectories:!(*mFileOptions & QT_PREPEND_NAMESPACE(QFileDialog::ReadOnly))];
    [mOpenPanel setAllowsMultipleSelection:(mFileMode == QT_PREPEND_NAMESPACE(QFileDialog::ExistingFiles))];
    [mOpenPanel setResolvesAliases:!(*mFileOptions & QT_PREPEND_NAMESPACE(QFileDialog::DontResolveSymlinks))];

    QStringList ext = [self acceptableExtensionsForSave];
    if (mPriv && !ext.isEmpty() && !mPriv->defaultSuffix.isEmpty())
        ext.prepend(mPriv->defaultSuffix);
    [mSavePanel setAllowedFileTypes:ext.isEmpty() ? nil : QT_PREPEND_NAMESPACE(qt_mac_QStringListToNSMutableArray(ext))];

    if ([mSavePanel isVisible])
        [mOpenPanel validateVisibleColumns];
}

- (void)panelSelectionDidChange:(id)sender
{
    Q_UNUSED(sender);
    if (mPriv) {
        QString selection = QT_PREPEND_NAMESPACE(qt_mac_NSStringToQString([mSavePanel filename]));
        if (selection != mCurrentSelection) {
            *mCurrentSelection = selection;
            mPriv->QNSOpenSavePanelDelegate_selectionChanged(selection);
        }
    }
}

- (void)openPanelDidEnd:(NSOpenPanel *)panel returnCode:(int)returnCode  contextInfo:(void *)contextInfo
{
    Q_UNUSED(panel);
    Q_UNUSED(contextInfo);
    mReturnCode = returnCode;
    if (mPriv)
        mPriv->QNSOpenSavePanelDelegate_panelClosed(returnCode == NSOKButton);
}

- (void)panel:(id)sender directoryDidChange:(NSString *)path
{
    Q_UNUSED(sender);
    if (!mPriv)
        return;
    if ([path isEqualToString:mCurrentDir])
        return;

    [mCurrentDir release];
    mCurrentDir = [path retain];
    mPriv->QNSOpenSavePanelDelegate_directoryEntered(QT_PREPEND_NAMESPACE(qt_mac_NSStringToQString(mCurrentDir)));
}

/*
    Returns a list of extensions (e.g. "png", "jpg", "gif")
    for the current name filter. If a filter do not conform
    to the format *.xyz or * or *.*, an empty list
    is returned meaning accept everything.
*/
- (QStringList)acceptableExtensionsForSave
{
    QStringList result;
    for (int i=0; i<mSelectedNameFilter->count(); ++i) {
        const QString &filter = mSelectedNameFilter->at(i);
        if (filter.startsWith(QLatin1String("*."))
                && !filter.contains(QLatin1Char('?'))
                && filter.count(QLatin1Char('*')) == 1) {
            result += filter.mid(2);
        } else {
            return QStringList(); // Accept everything
        }
    }
    return result;
}

- (QString)removeExtensions:(const QString &)filter
{
    QRegExp regExp(QT_PREPEND_NAMESPACE(QString::fromLatin1)(QT_PREPEND_NAMESPACE(qt_file_dialog_filter_reg_exp)));
    if (regExp.indexIn(filter) != -1)
        return regExp.cap(1).trimmed();
    return filter;
}

- (void)createTextField
{
    NSRect textRect = { { 0.0, 3.0 }, { 100.0, 25.0 } };
    mTextField = [[NSTextField alloc] initWithFrame:textRect];
    [[mTextField cell] setFont:[NSFont systemFontOfSize:
            [NSFont systemFontSizeForControlSize:NSRegularControlSize]]];
    [mTextField setAlignment:NSRightTextAlignment];
    [mTextField setEditable:false];
    [mTextField setSelectable:false];
    [mTextField setBordered:false];
    [mTextField setDrawsBackground:false];
    if (mPriv){
        [mTextField setStringValue:[self strip:mPriv->qFileDialogUi->fileTypeLabel->text()]];
    } else
        [mTextField setStringValue:QT_PREPEND_NAMESPACE(qt_mac_QStringToNSString)(QT_PREPEND_NAMESPACE(QFileDialog::tr)("Files of type:"))];
}

- (void)createPopUpButton:(const QString &)selectedFilter hideDetails:(BOOL)hideDetails
{
    NSRect popUpRect = { { 100.0, 5.0 }, { 250.0, 25.0 } };
    mPopUpButton = [[NSPopUpButton alloc] initWithFrame:popUpRect pullsDown:NO];
    [mPopUpButton setTarget:self];
    [mPopUpButton setAction:@selector(filterChanged:)];

    QStringList *filters = mNameFilterDropDownList;
    if (filters->size() > 0){
        for (int i=0; i<mNameFilterDropDownList->size(); ++i) {
            QString filter = hideDetails ? [self removeExtensions:filters->at(i)] : filters->at(i);
            [mPopUpButton addItemWithTitle:QT_PREPEND_NAMESPACE(qt_mac_QStringToNSString)(filter)];
            if (filters->at(i).startsWith(selectedFilter))
                [mPopUpButton selectItemAtIndex:i];
        }
    }
}

- (QStringList) findStrippedFilterWithVisualFilterName:(QString)name
{
    for (int i=0; i<mNameFilterDropDownList->size(); ++i) {
        if (mNameFilterDropDownList->at(i).startsWith(name))
            return qt_clean_filter_list(mNameFilterDropDownList->at(i));
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

void QFileDialogPrivate::QNSOpenSavePanelDelegate_selectionChanged(const QString &newPath)
{
    emit q_func()->currentChanged(newPath);
}

void QFileDialogPrivate::QNSOpenSavePanelDelegate_panelClosed(bool accepted)
{
    if (accepted)
        q_func()->accept();
    else
        q_func()->reject();
}

void QFileDialogPrivate::QNSOpenSavePanelDelegate_directoryEntered(const QString &newDir)
{
    setLastVisitedDirectory(newDir);
    emit q_func()->directoryEntered(newDir);
}

void QFileDialogPrivate::QNSOpenSavePanelDelegate_filterSelected(int menuIndex)
{
    emit q_func()->filterSelected(nameFilters.at(menuIndex));
}

extern OSErr qt_mac_create_fsref(const QString &, FSRef *); // qglobal.cpp
extern void qt_mac_to_pascal_string(QString s, Str255 str, TextEncoding encoding=0, int len=-1); // qglobal.cpp

void QFileDialogPrivate::setDirectory_sys(const QString &directory)
{
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
    [delegate->mSavePanel setDirectory:qt_mac_QStringToNSString(directory)];
}

QString QFileDialogPrivate::directory_sys() const
{
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
    return qt_mac_NSStringToQString([delegate->mSavePanel directory]);
}

void QFileDialogPrivate::selectFile_sys(const QString &filename)
{
    QString filePath = filename;
    if (QDir::isRelativePath(filePath))
        filePath = QFileInfo(directory_sys(), filePath).filePath();

    // There seems to no way to select a file once the dialog is running.
    // So do the next best thing, set the file's directory:
    setDirectory_sys(QFileInfo(filePath).absolutePath());
}

QStringList QFileDialogPrivate::selectedFiles_sys() const
{
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
    return [delegate selectedFiles];
}

void QFileDialogPrivate::setNameFilters_sys(const QStringList &filters)
{
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
    bool hideDetails = q_func()->testOption(QFileDialog::HideNameFilterDetails);
    [delegate setNameFilters:filters hideDetails:hideDetails];
}

void QFileDialogPrivate::setFilter_sys()
{
    Q_Q(QFileDialog);
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
    *(delegate->mQDirFilter) = model->filter();
    delegate->mFileMode = fileMode;
    [delegate->mSavePanel setTitle:qt_mac_QStringToNSString(q->windowTitle())];
    [delegate->mSavePanel setPrompt:[delegate strip:acceptLabel]];
    if (fileNameLabelExplicitlySat)
        [delegate->mSavePanel setNameFieldLabel:[delegate strip:qFileDialogUi->fileNameLabel->text()]];

    [delegate updateProperties];
}

void QFileDialogPrivate::selectNameFilter_sys(const QString &filter)
{
    int index = nameFilters.indexOf(filter);
    if (index != -1) {
        QMacCocoaAutoReleasePool pool;
        QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
        [delegate->mPopUpButton selectItemAtIndex:index];
        [delegate filterChanged:nil];
    }
}

QString QFileDialogPrivate::selectedNameFilter_sys() const
{
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
    int index = [delegate->mPopUpButton indexOfSelectedItem];
    return index != -1 ? nameFilters.at(index) : QString();
}

void QFileDialogPrivate::deleteNativeDialog_sys()
{
    QMacCocoaAutoReleasePool pool;
    [reinterpret_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate) release];
    mDelegate = 0;
    nativeDialogInUse = false;
}

bool QFileDialogPrivate::setVisible_sys(bool visible)
{
    Q_Q(QFileDialog);
    if (!visible == q->isHidden())
        return false;

    if (q->windowFlags() & Qt::WindowStaysOnTopHint) {
        // The native file dialog tries all it can to stay
        // on the NSModalPanel level. And it might also show
        // its own "create directory" dialog that we cannot control.
        // So we need to use the non-native version in this case...
        return false;
    }

    return visible ? showCocoaFilePanel() : hideCocoaFilePanel();
}

void QFileDialogPrivate::createNSOpenSavePanelDelegate()
{
    Q_Q(QFileDialog);
    if (mDelegate)
        return;

    bool selectDir = q->selectedFiles().isEmpty();
    QString selection(selectDir ? q->directory().absolutePath() : q->selectedFiles().value(0));
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = [[QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) alloc]
        initWithAcceptMode:acceptMode
        title:q->windowTitle()
        hideNameFilterDetails:q->testOption(QFileDialog::HideNameFilterDetails)
        qDirFilter:model->filter()
        fileOptions:opts
        fileMode:fileMode
        selectFile:selection
        confirmOverwrite:!q->testOption(QFileDialog::DontConfirmOverwrite)
        priv:this];

    mDelegate = delegate;
}

bool QFileDialogPrivate::showCocoaFilePanel()
{
    Q_Q(QFileDialog);
    QMacCocoaAutoReleasePool pool;
    createNSOpenSavePanelDelegate();
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
    if (qt_mac_is_macsheet(q))
        [delegate showWindowModalSheet:q->parentWidget()];
    else
        [delegate showModelessPanel];
    return true;
}

bool QFileDialogPrivate::hideCocoaFilePanel()
{
    if (!mDelegate){
        // Nothing to do. We return false to leave the question
        // open regarding whether or not to go native:
        return false;
    } else {
        QMacCocoaAutoReleasePool pool;
        QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
        [delegate closePanel];
        // Even when we hide it, we are still using a
        // native dialog, so return true:
        return true;
    }
}


void QFileDialogPrivate::platformNativeDialogModalHelp()
{
    // Do a queued meta-call to open the native modal dialog so it opens after the new
    // event loop has started to execute (in QDialog::exec). Using a timer rather than
    // a queued meta call is intentional to ensure that the call is only delivered when
    // [NSApp run] runs (timers are handeled special in cocoa). If NSApp is not
    // running (which is the case if e.g a top-most QEventLoop has been
    // interrupted, and the second-most event loop has not yet been reactivated (regardless
    // if [NSApp run] is still on the stack)), showing a native modal dialog will fail.
    if (nativeDialogInUse){
        Q_Q(QFileDialog);
        QTimer::singleShot(1, q, SLOT(_q_macRunNativeAppModalPanel()));
    }
}

void QFileDialogPrivate::_q_macRunNativeAppModalPanel()
{
    QBoolBlocker nativeDialogOnTop(QApplicationPrivate::native_modal_dialog_active);
    Q_Q(QFileDialog);
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
    [delegate runApplicationModalPanel];
    dialogResultCode_sys() == QDialog::Accepted ? q->accept() : q->reject();
}

QDialog::DialogCode QFileDialogPrivate::dialogResultCode_sys()
{
    QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *delegate = static_cast<QT_MANGLE_NAMESPACE(QNSOpenSavePanelDelegate) *>(mDelegate);
    return [delegate dialogResultCode];
}


QT_END_NAMESPACE

#endif // QT_NO_FILEDIALOG

