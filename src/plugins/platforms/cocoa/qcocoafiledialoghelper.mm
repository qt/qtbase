// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qglobal.h>

#include <AppKit/AppKit.h>

#include "qcocoafiledialoghelper.h"
#include "qcocoahelpers.h"
#include "qcocoaeventdispatcher.h"

#include <QtCore/qbuffer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qdir.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qpointer.h>
#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformtheme.h>
#include <qpa/qplatformnativeinterface.h>

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;

static NSString *strippedText(QString s)
{
    s.remove("..."_L1);
    return QPlatformTheme::removeMnemonics(s).trimmed().toNSString();
}

// NSOpenPanel extends NSSavePanel with some extra APIs
static NSOpenPanel *openpanel_cast(NSSavePanel *panel)
{
    if ([panel isKindOfClass:NSOpenPanel.class])
        return static_cast<NSOpenPanel*>(panel);
    else
        return nil;
}

typedef QSharedPointer<QFileDialogOptions> SharedPointerFileDialogOptions;

@implementation QNSOpenSavePanelDelegate {
  @public
    NSSavePanel *m_panel;
    NSView *m_accessoryView;
    NSPopUpButton *m_popupButton;
    NSTextField *m_textField;
    QPointer<QCocoaFileDialogHelper> m_helper;
    NSString *m_currentDirectory;

    SharedPointerFileDialogOptions m_options;
    QString m_currentSelection;
    QStringList m_nameFilterDropDownList;
    QStringList m_selectedNameFilter;
}

- (instancetype)initWithAcceptMode:(const QString &)selectFile
                           options:(SharedPointerFileDialogOptions)options
                            helper:(QCocoaFileDialogHelper *)helper
{
    if ((self = [super init])) {
        m_options = options;

        if (m_options->acceptMode() == QFileDialogOptions::AcceptOpen)
            m_panel = [[NSOpenPanel openPanel] retain];
        else
            m_panel = [[NSSavePanel savePanel] retain];

        m_panel.canSelectHiddenExtension = YES;
        m_panel.level = NSModalPanelWindowLevel;

        m_helper = helper;

        m_nameFilterDropDownList = m_options->nameFilters();
        QString selectedVisualNameFilter = m_options->initiallySelectedNameFilter();
        m_selectedNameFilter = [self findStrippedFilterWithVisualFilterName:selectedVisualNameFilter];

        const QFileInfo sel(selectFile);
        if (sel.isDir() && !sel.isBundle()){
            m_currentDirectory = [sel.absoluteFilePath().toNSString() retain];
            m_currentSelection.clear();
        } else {
            m_currentDirectory = [sel.absolutePath().toNSString() retain];
            m_currentSelection = sel.absoluteFilePath();
        }

        [self createPopUpButton:selectedVisualNameFilter hideDetails:options->testOption(QFileDialogOptions::HideNameFilterDetails)];
        [self createTextField];
        [self createAccessory];

        m_panel.accessoryView = m_nameFilterDropDownList.size() > 1 ? m_accessoryView : nil;
        // -setAccessoryView: can result in -panel:directoryDidChange:
        // resetting our m_currentDirectory, set the delegate
        // here to make sure it gets the correct value.
        m_panel.delegate = self;

        if (auto *openPanel = openpanel_cast(m_panel))
            openPanel.accessoryViewDisclosed = YES;

        [self updateProperties];
    }
    return self;
}

- (void)dealloc
{
    [m_panel orderOut:m_panel];
    m_panel.accessoryView = nil;
    [m_popupButton release];
    [m_textField release];
    [m_accessoryView release];
    m_panel.delegate = nil;
    [m_panel release];
    [m_currentDirectory release];
    [super dealloc];
}

- (bool)showPanel:(Qt::WindowModality) windowModality withParent:(QWindow *)parent
{
    const QFileInfo info(m_currentSelection);
    NSString *filepath = info.filePath().toNSString();
    NSURL *url = [NSURL fileURLWithPath:filepath isDirectory:info.isDir()];
    bool selectable = (m_options->acceptMode() == QFileDialogOptions::AcceptSave)
        || [self panel:m_panel shouldEnableURL:url];

    m_panel.directoryURL = [NSURL fileURLWithPath:m_currentDirectory];
    m_panel.nameFieldStringValue = selectable ? info.fileName().toNSString() : @"";

    [self updateProperties];

    auto completionHandler = ^(NSInteger result) { m_helper->panelClosed(result); };

    if (windowModality == Qt::WindowModal && parent) {
        NSView *view = reinterpret_cast<NSView*>(parent->winId());
        [m_panel beginSheetModalForWindow:view.window completionHandler:completionHandler];
    } else if (windowModality == Qt::ApplicationModal) {
        return true; // Defer until exec()
    } else {
        [m_panel beginWithCompletionHandler:completionHandler];
    }

    return true;
}

-(void)runApplicationModalPanel
{
    // Note: If NSApp is not running (which is the case if e.g a top-most
    // QEventLoop has been interrupted, and the second-most event loop has not
    // yet been reactivated (regardless if [NSApp run] is still on the stack)),
    // showing a native modal dialog will fail.
    if (!m_helper)
        return;

    QMacAutoReleasePool pool;

    // Call processEvents in case the event dispatcher has been interrupted, and needs to do
    // cleanup of modal sessions. Do this before showing the native dialog, otherwise it will
    // close down during the cleanup.
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    // Make sure we don't interrupt the runModal call below.
    QCocoaEventDispatcher::clearCurrentThreadCocoaEventDispatcherInterruptFlag();

    auto result = [m_panel runModal];
    m_helper->panelClosed(result);

    // Wake up the event dispatcher so it can check whether the
    // current event loop should continue spinning or not.
    QCoreApplication::eventDispatcher()->wakeUp();
}

- (void)closePanel
{
    m_currentSelection = QString::fromNSString(m_panel.URL.path).normalized(QString::NormalizationForm_C);

    if (m_panel.sheet)
        [NSApp endSheet:m_panel];
    else if (NSApp.modalWindow == m_panel)
        [NSApp stopModal];
    else
        [m_panel close];
}

- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url
{
    Q_UNUSED(sender);

    NSString *filename = url.path;
    if (!filename.length)
        return NO;

    const QFileInfo fileInfo(QString::fromNSString(filename));

    // Always accept directories regardless of their names.
    // This also includes symlinks and aliases to directories.
    if (fileInfo.isDir()) {
        // Unless it's a bundle, and we should treat bundles as files.
        // FIXME: We'd like to use QFileInfo::isBundle() here, but the
        // detection in QFileInfo goes deeper than NSWorkspace does
        // (likely a bug), and as a result causes TCC permission
        // dialogs to pop up when used.
        bool treatBundlesAsFiles = !m_panel.treatsFilePackagesAsDirectories;
        if (!(treatBundlesAsFiles && [NSWorkspace.sharedWorkspace isFilePackageAtPath:filename]))
            return YES;
    }

    const QString qtFileName = fileInfo.fileName();
    // No filter means accept everything
    bool nameMatches = m_selectedNameFilter.isEmpty();
    // Check if the current file name filter accepts the file:
    for (int i = 0; !nameMatches && i < m_selectedNameFilter.size(); ++i) {
        if (QDir::match(m_selectedNameFilter.at(i), qtFileName))
            nameMatches = true;
    }
    if (!nameMatches)
        return NO;

    QDir::Filters filter = m_options->filter();
    if ((!(filter & (QDir::Dirs | QDir::AllDirs)) && fileInfo.isDir())
        || (!(filter & QDir::Files) && (fileInfo.isFile() && !fileInfo.isSymLink()))
        || ((filter & QDir::NoSymLinks) && fileInfo.isSymLink()))
        return NO;

    bool filterPermissions = ((filter & QDir::PermissionMask)
                              && (filter & QDir::PermissionMask) != QDir::PermissionMask);
    if (filterPermissions) {
        if ((!(filter & QDir::Readable) && fileInfo.isReadable())
            || (!(filter & QDir::Writable) && fileInfo.isWritable())
            || (!(filter & QDir::Executable) && fileInfo.isExecutable()))
            return NO;
    }

    // We control the visibility of hidden files via the showsHiddenFiles
    // property on the panel, based on QDir::Hidden being set. But the user
    // can also toggle this via the Command+Shift+. keyboard shortcut,
    // in which case they have explicitly requested to show hidden files,
    // and we should enable them even if QDir::Hidden was not set. In
    // effect, we don't need to filter on QDir::Hidden here.

    return YES;
}

- (void)setNameFilters:(const QStringList &)filters hideDetails:(BOOL)hideDetails
{
    [m_popupButton removeAllItems];
    m_nameFilterDropDownList = filters;
    if (filters.size() > 0){
        for (int i = 0; i < filters.size(); ++i) {
            const QString filter = hideDetails ? [self removeExtensions:filters.at(i)] : filters.at(i);
            [m_popupButton.menu addItemWithTitle:filter.toNSString() action:nil keyEquivalent:@""];
        }
        [m_popupButton selectItemAtIndex:0];
        m_panel.accessoryView = m_accessoryView;
    } else {
        m_panel.accessoryView = nil;
    }

    [self filterChanged:self];
}

- (void)filterChanged:(id)sender
{
    // This m_delegate function is called when the _name_ filter changes.
    Q_UNUSED(sender);
    if (!m_helper)
        return;
    const QString selection = m_nameFilterDropDownList.value([m_popupButton indexOfSelectedItem]);
    m_selectedNameFilter = [self findStrippedFilterWithVisualFilterName:selection];
    [m_panel validateVisibleColumns];
    [self updateProperties];

    const QStringList filters = m_options->nameFilters();
    const int menuIndex = m_popupButton.indexOfSelectedItem;
    emit m_helper->filterSelected(menuIndex >= 0 && menuIndex < filters.size() ? filters.at(menuIndex) : QString());
}

- (QList<QUrl>)selectedFiles
{
    if (auto *openPanel = openpanel_cast(m_panel)) {
        QList<QUrl> result;
        for (NSURL *url in openPanel.URLs) {
            QString path = QString::fromNSString(url.path).normalized(QString::NormalizationForm_C);
            result << QUrl::fromLocalFile(path);
        }
        return result;
    } else {
        QList<QUrl> result;
        QString filename = QString::fromNSString(m_panel.URL.path).normalized(QString::NormalizationForm_C);
        const QString defaultSuffix = m_options->defaultSuffix();
        const QFileInfo fileInfo(filename);

        // If neither the user or the NSSavePanel have provided a suffix, use
        // the default suffix (if it exists).
        if (fileInfo.suffix().isEmpty() && !defaultSuffix.isEmpty())
            filename.append('.').append(defaultSuffix);

        result << QUrl::fromLocalFile(filename);
        return result;
    }
}

- (void)updateProperties
{
    const QFileDialogOptions::FileMode fileMode = m_options->fileMode();
    bool chooseFilesOnly = fileMode == QFileDialogOptions::ExistingFile
        || fileMode == QFileDialogOptions::ExistingFiles;
    bool chooseDirsOnly = fileMode == QFileDialogOptions::Directory
        || fileMode == QFileDialogOptions::DirectoryOnly
        || m_options->testOption(QFileDialogOptions::ShowDirsOnly);

    m_panel.title = m_options->windowTitle().toNSString();
    m_panel.canCreateDirectories = !(m_options->testOption(QFileDialogOptions::ReadOnly));

    if (m_options->isLabelExplicitlySet(QFileDialogOptions::Accept))
        m_panel.prompt = strippedText(m_options->labelText(QFileDialogOptions::Accept));
    if (m_options->isLabelExplicitlySet(QFileDialogOptions::FileName))
        m_panel.nameFieldLabel = strippedText(m_options->labelText(QFileDialogOptions::FileName));

    if (auto *openPanel = openpanel_cast(m_panel)) {
        openPanel.canChooseFiles = !chooseDirsOnly;
        openPanel.canChooseDirectories = !chooseFilesOnly;
        openPanel.allowsMultipleSelection = (fileMode == QFileDialogOptions::ExistingFiles);
        openPanel.resolvesAliases = !(m_options->testOption(QFileDialogOptions::DontResolveSymlinks));
    }

    m_popupButton.hidden = chooseDirsOnly;    // TODO hide the whole sunken pane instead?

    m_panel.allowedFileTypes = [self computeAllowedFileTypes];

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
    // switching filters in an already opened dialog.
    if (m_panel.allowedFileTypes.count > 2)
        m_panel.extensionHidden = NO;

    m_panel.showsHiddenFiles = m_options->filter().testFlag(QDir::Hidden);

    if (m_panel.visible)
        [m_panel validateVisibleColumns];
}

- (void)panelSelectionDidChange:(id)sender
{
    Q_UNUSED(sender);

    if (!m_helper)
        return;

    if (m_panel.visible) {
        const QString selection = QString::fromNSString(m_panel.URL.path);
        if (selection != m_currentSelection) {
            m_currentSelection = selection;
            emit m_helper->currentChanged(QUrl::fromLocalFile(selection));
        }
    }
}

- (void)panel:(id)sender directoryDidChange:(NSString *)path
{
    Q_UNUSED(sender);

    if (!m_helper)
        return;

    if (!(path && path.length) || [path isEqualToString:m_currentDirectory])
        return;

    [m_currentDirectory release];
    m_currentDirectory = [path retain];

    // ### fixme: priv->setLastVisitedDirectory(newDir);
    emit m_helper->directoryEntered(QUrl::fromLocalFile(QString::fromNSString(m_currentDirectory)));
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
- (NSArray<NSString*>*)computeAllowedFileTypes
{
    if (m_options->acceptMode() != QFileDialogOptions::AcceptSave)
        return nil; // panel:shouldEnableURL: does the file filtering for NSOpenPanel

    QStringList fileTypes;
    for (const QString &filter : std::as_const(m_selectedNameFilter)) {
        if (!filter.startsWith("*."_L1))
            continue;

        if (filter.contains(u'?'))
            continue;

        if (filter.count(u'*') != 1)
            continue;

        auto extensions = filter.split('.', Qt::SkipEmptyParts);
        fileTypes += extensions.last();
    }

    return fileTypes.isEmpty() ? nil : qt_mac_QStringListToNSMutableArray(fileTypes);
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
    m_textField = [[NSTextField alloc] initWithFrame:textRect];
    m_textField.cell.font = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeRegular]];
    m_textField.alignment = NSTextAlignmentRight;
    m_textField.editable = false;
    m_textField.selectable = false;
    m_textField.bordered = false;
    m_textField.drawsBackground = false;
    if (m_options->isLabelExplicitlySet(QFileDialogOptions::FileType))
        m_textField.stringValue = strippedText(m_options->labelText(QFileDialogOptions::FileType));
}

- (void)createPopUpButton:(const QString &)selectedFilter hideDetails:(BOOL)hideDetails
{
    NSRect popUpRect = { { 100.0, 5.0 }, { 250.0, 25.0 } };
    m_popupButton = [[NSPopUpButton alloc] initWithFrame:popUpRect pullsDown:NO];
    m_popupButton.target = self;
    m_popupButton.action = @selector(filterChanged:);

    if (!m_nameFilterDropDownList.isEmpty()) {
        int filterToUse = -1;
        for (int i = 0; i < m_nameFilterDropDownList.size(); ++i) {
            const QString currentFilter = m_nameFilterDropDownList.at(i);
            if (selectedFilter == currentFilter ||
                (filterToUse == -1 && currentFilter.startsWith(selectedFilter)))
                filterToUse = i;
            QString filter = hideDetails ? [self removeExtensions:currentFilter] : currentFilter;
            [m_popupButton.menu addItemWithTitle:filter.toNSString() action:nil keyEquivalent:@""];
        }
        if (filterToUse != -1)
            [m_popupButton selectItemAtIndex:filterToUse];
    }
}

- (QStringList) findStrippedFilterWithVisualFilterName:(QString)name
{
    for (const QString &currentFilter : std::as_const(m_nameFilterDropDownList)) {
        if (currentFilter.startsWith(name))
            return QPlatformFileDialogHelper::cleanFilterList(currentFilter);
    }
    return QStringList();
}

- (void)createAccessory
{
    NSRect accessoryRect = { { 0.0, 0.0 }, { 450.0, 33.0 } };
    m_accessoryView = [[NSView alloc] initWithFrame:accessoryRect];
    [m_accessoryView addSubview:m_textField];
    [m_accessoryView addSubview:m_popupButton];
}

@end

QT_BEGIN_NAMESPACE

QCocoaFileDialogHelper::QCocoaFileDialogHelper()
{
}

QCocoaFileDialogHelper::~QCocoaFileDialogHelper()
{
    if (!m_delegate)
        return;

    QMacAutoReleasePool pool;
    [m_delegate release];
    m_delegate = nil;
}

void QCocoaFileDialogHelper::panelClosed(NSInteger result)
{
    if (result == NSModalResponseOK)
        emit accept();
    else
        emit reject();
}

void QCocoaFileDialogHelper::setDirectory(const QUrl &directory)
{
    if (m_delegate)
        m_delegate->m_panel.directoryURL = [NSURL fileURLWithPath:directory.toLocalFile().toNSString()];
    else
        m_directory = directory;
}

QUrl QCocoaFileDialogHelper::directory() const
{
    if (m_delegate) {
        QString path = QString::fromNSString(m_delegate->m_panel.directoryURL.path).normalized(QString::NormalizationForm_C);
        return QUrl::fromLocalFile(path);
    }
    return m_directory;
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
    if (m_delegate)
        return [m_delegate selectedFiles];
    return QList<QUrl>();
}

void QCocoaFileDialogHelper::setFilter()
{
    if (!m_delegate)
        return;

    [m_delegate updateProperties];
}

void QCocoaFileDialogHelper::selectNameFilter(const QString &filter)
{
    if (!options())
        return;
    const int index = options()->nameFilters().indexOf(filter);
    if (index != -1) {
        if (!m_delegate) {
            options()->setInitiallySelectedNameFilter(filter);
            return;
        }
        [m_delegate->m_popupButton selectItemAtIndex:index];
        [m_delegate filterChanged:nil];
    }
}

QString QCocoaFileDialogHelper::selectedNameFilter() const
{
    if (!m_delegate)
        return options()->initiallySelectedNameFilter();
    int index = [m_delegate->m_popupButton indexOfSelectedItem];
    if (index >= options()->nameFilters().count())
        return QString();
    return index != -1 ? options()->nameFilters().at(index) : QString();
}

void QCocoaFileDialogHelper::hide()
{
    if (!m_delegate)
        return;

    [m_delegate closePanel];

    if (m_eventLoop)
        m_eventLoop->exit();
}

bool QCocoaFileDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    if (windowFlags & Qt::WindowStaysOnTopHint) {
        // The native file dialog tries all it can to stay
        // on the NSModalPanel level. And it might also show
        // its own "create directory" dialog that we cannot control.
        // So we need to use the non-native version in this case...
        return false;
    }

    createNSOpenSavePanelDelegate();

    return [m_delegate showPanel:windowModality withParent:parent];
}

void QCocoaFileDialogHelper::createNSOpenSavePanelDelegate()
{
    QMacAutoReleasePool pool;

    const SharedPointerFileDialogOptions &opts = options();
    const QList<QUrl> selectedFiles = opts->initiallySelectedFiles();
    const QUrl directory = m_directory.isEmpty() ? opts->initialDirectory() : m_directory;
    const bool selectDir = selectedFiles.isEmpty();
    QString selection(selectDir ? directory.toLocalFile() : selectedFiles.front().toLocalFile());
    QNSOpenSavePanelDelegate *delegate = [[QNSOpenSavePanelDelegate alloc]
        initWithAcceptMode:
            selection
            options:opts
            helper:this];

    [static_cast<QNSOpenSavePanelDelegate *>(m_delegate) release];
    m_delegate = delegate;
}

void QCocoaFileDialogHelper::exec()
{
    Q_ASSERT(m_delegate);

    if (m_delegate->m_panel.visible) {
        // WindowModal or NonModal, so already shown above
        QEventLoop eventLoop;
        m_eventLoop = &eventLoop;
        eventLoop.exec(QEventLoop::DialogExec);
        m_eventLoop = nullptr;
    } else {
        // ApplicationModal, so show and block using native APIs
        [m_delegate runApplicationModalPanel];
    }
}

bool QCocoaFileDialogHelper::defaultNameFilterDisables() const
{
    return true;
}

QT_END_NAMESPACE
