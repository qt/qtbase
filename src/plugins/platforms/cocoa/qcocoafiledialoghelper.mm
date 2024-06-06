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
#include <QtCore/qdir.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qpointer.h>
#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformtheme.h>
#include <qpa/qplatformnativeinterface.h>

#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

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

        m_panel.extensionHidden = [&]{
            for (const auto &nameFilter : m_nameFilterDropDownList) {
                 const auto extensions = QPlatformFileDialogHelper::cleanFilterList(nameFilter);
                 for (const auto &extension : extensions) {
                    // Explicitly show extensions if we detect a filter
                    // of "all files", as clicking a single file with
                    // extensions hidden will then populate the name
                    // field with only the file name, without any
                    // extension.
                    if (extension == "*"_L1 || extension == "*.*"_L1)
                        return false;

                    // Explicitly show extensions if we detect a filter
                    // that has a multi-part extension. This prevents
                    // confusing situations where the user clicks e.g.
                    // 'foo.tar.gz' and 'foo.tar' is populated in the
                    // file name box, but when then clicking save macOS
                    // will warn that the file needs to end in .gz,
                    // due to thinking the user tried to save the file
                    // as a 'tar' file instead. Unfortunately this
                    // property can only be set before the panel is
                    // shown, so we can't toggle it on and off based
                    // on the active filter.
                    if (extension.count('.') > 1)
                        return false;
                 }
            }
            return true;
        }();

        const QFileInfo sel(selectFile);
        if (sel.isDir() && !sel.isBundle()){
            m_panel.directoryURL = [NSURL fileURLWithPath:sel.absoluteFilePath().toNSString()];
            m_currentSelection.clear();
        } else {
            m_panel.directoryURL = [NSURL fileURLWithPath:sel.absolutePath().toNSString()];
            m_currentSelection = sel.absoluteFilePath();
        }

        [self createPopUpButton:selectedVisualNameFilter hideDetails:options->testOption(QFileDialogOptions::HideNameFilterDetails)];
        [self createTextField];
        [self createAccessory];

        m_panel.accessoryView = m_nameFilterDropDownList.size() > 1 ? m_accessoryView : nil;
        // -setAccessoryView: can result in -panel:directoryDidChange:
        // resetting our current directory. Set the delegate
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
    [super dealloc];
}

- (bool)showPanel:(Qt::WindowModality) windowModality withParent:(QWindow *)parent
{
    const QFileInfo info(m_currentSelection);
    NSString *filepath = info.filePath().toNSString();
    NSURL *url = [NSURL fileURLWithPath:filepath isDirectory:info.isDir()];
    bool selectable = (m_options->acceptMode() == QFileDialogOptions::AcceptSave)
        || [self panel:m_panel shouldEnableURL:url];

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

    if (![self fileInfoMatchesCurrentNameFilter:fileInfo])
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

- (BOOL)panel:(id)sender validateURL:(NSURL *)url error:(NSError * _Nullable *)outError
{
    Q_ASSERT(sender == m_panel);

    if (!m_panel.allowedFileTypes && !m_selectedNameFilter.isEmpty()) {
        // The save panel hasn't done filtering on our behalf,
        // either because we couldn't represent the filter via
        // allowedFileTypes, or we opted out due to a multi part
        // extension, so do the filtering/validation ourselves.
        QFileInfo fileInfo(QString::fromNSString(url.path).normalized(QString::NormalizationForm_C));

        if ([self fileInfoMatchesCurrentNameFilter:fileInfo])
            return YES;

        if (fileInfo.suffix().isEmpty()) {
            // The filter requires a file name with an extension.
            // We're going to add a default file name in selectedFiles,
            // to match the native behavior. Check now that we can
            // overwrite the file, if is already exists.
            fileInfo = [self applyDefaultSuffixFromCurrentNameFilter:fileInfo];

            if (!fileInfo.exists() || m_options->testOption(QFileDialogOptions::DontConfirmOverwrite))
                return YES;

            QMacAutoReleasePool pool;
            auto *alert = [[NSAlert new] autorelease];
            alert.alertStyle = NSAlertStyleCritical;

            alert.messageText = [NSString stringWithFormat:qt_mac_AppKitString(@"SavePanel",
                @"\\U201c%@\\U201d already exists. Do you want to replace it?"),
                    fileInfo.fileName().toNSString()];
            alert.informativeText = [NSString stringWithFormat:qt_mac_AppKitString(@"SavePanel",
                @"A file or folder with the same name already exists in the folder %@. "
                "Replacing it will overwrite its current contents."),
                        fileInfo.absoluteDir().dirName().toNSString()];

            auto *replaceButton = [alert addButtonWithTitle:qt_mac_AppKitString(@"SavePanel", @"Replace")];
            replaceButton.hasDestructiveAction = YES;
            replaceButton.tag = 1337;
            [alert addButtonWithTitle:qt_mac_AppKitString(@"Common", @"Cancel")];

            [alert beginSheetModalForWindow:m_panel
                completionHandler:^(NSModalResponse returnCode) {
                    [NSApp stopModalWithCode:returnCode];
                }];
            return [NSApp runModalForWindow:alert.window] == replaceButton.tag;
        } else {
            QFileInfo firstFilter(m_selectedNameFilter.first());
            auto *domain = qGuiApp->organizationDomain().toNSString();
            *outError = [NSError errorWithDomain:domain code:0 userInfo:@{
                NSLocalizedDescriptionKey:[NSString stringWithFormat:qt_mac_AppKitString(@"SavePanel",
                    @"You cannot save this document with extension \\U201c.%1$@\\U201d at the end "
                    "of the name. The required extension is \\U201c.%2$@\\U201d."),
                fileInfo.completeSuffix().toNSString(), firstFilter.completeSuffix().toNSString()]
            }];
            return NO;
        }
    }

    return YES;
}

- (QFileInfo)applyDefaultSuffixFromCurrentNameFilter:(const QFileInfo &)fileInfo
{
    QFileInfo filterInfo(m_selectedNameFilter.first());
    return QFileInfo(fileInfo.absolutePath(),
        fileInfo.baseName() + '.' + filterInfo.completeSuffix());
}

- (bool)fileInfoMatchesCurrentNameFilter:(const QFileInfo &)fileInfo
{
    // No filter means accept everything
    if (m_selectedNameFilter.isEmpty())
        return true;

    // Check if the current file name filter accepts the file
    for (const auto &filter : m_selectedNameFilter) {
        if (QDir::match(filter, fileInfo.fileName()))
            return true;
    }

    return false;
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
        QString filename = QString::fromNSString(m_panel.URL.path).normalized(QString::NormalizationForm_C);
        QFileInfo fileInfo(filename);

        if (fileInfo.suffix().isEmpty() && ![self fileInfoMatchesCurrentNameFilter:fileInfo]) {
            // We end up in this situation if we accept a file name without extension
            // in panel:validateURL:error. If so, we match the behavior of the native
            // save dialog and add the first of the accepted extension from the filter.
            fileInfo = [self applyDefaultSuffixFromCurrentNameFilter:fileInfo];
        }

        // If neither the user or the NSSavePanel have provided a suffix, use
        // the default suffix (if it exists).
        const QString defaultSuffix = m_options->defaultSuffix();
        if (fileInfo.suffix().isEmpty() && !defaultSuffix.isEmpty()) {
            fileInfo.setFile(fileInfo.absolutePath(),
                fileInfo.baseName() + '.' + defaultSuffix);
        }

        return { QUrl::fromLocalFile(fileInfo.filePath()) };
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

    // Setting allowedFileTypes to nil is not enough to reset any
    // automatically added extension based on a previous filter.
    // This is problematic because extensions can in some cases
    // be hidden from the user, resulting in confusion when the
    // resulting file name doesn't match the current empty filter.
    // We work around this by temporarily resetting the allowed
    // content type to one without an extension, which forces
    // the save panel to update and remove the extension.
    const bool nameFieldHasExtension = m_panel.nameFieldStringValue.pathExtension.length > 0;
    if (!m_panel.allowedFileTypes && !nameFieldHasExtension && !openpanel_cast(m_panel)) {
        if (!UTTypeDirectory.preferredFilenameExtension) {
            m_panel.allowedContentTypes = @[ UTTypeDirectory ];
            m_panel.allowedFileTypes = nil;
        } else {
            qWarning() << "UTTypeDirectory unexpectedly reported an extension";
        }
    }

    m_panel.showsHiddenFiles = m_options->filter().testFlag(QDir::Hidden);

    if (m_panel.visible)
        [m_panel validateVisibleColumns];
}

- (void)panelSelectionDidChange:(id)sender
{
    Q_UNUSED(sender);

    if (!m_helper)
        return;

    // Save panels only allow you to select directories, which
    // means currentChanged will only be emitted when selecting
    // a directory, and if so, with the latest chosen file name,
    // which is confusing and inconsistent. We choose to bail
    // out entirely for save panels, to give consistent behavior.
    if (!openpanel_cast(m_panel))
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

    m_helper->panelDirectoryDidChange(path);
}

/*
    Computes a list of extensions (e.g. "png", "jpg", "gif")
    for the current name filter, and updates the save panel.

    If a filter do not conform to the format *.xyz or * or *.*,
    or contains an extensions with more than one part (e.g. "tar.gz")
    we treat that as allowing all file types, and do our own
    validation in panel:validateURL:error.
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
        if (extensions.count() > 2)
            return nil;

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
    m_directory = directory;

    if (m_delegate)
        m_delegate->m_panel.directoryURL = [NSURL fileURLWithPath:directory.toLocalFile().toNSString()];
}

QUrl QCocoaFileDialogHelper::directory() const
{
    return m_directory;
}

void QCocoaFileDialogHelper::panelDirectoryDidChange(NSString *path)
{
    if (!path || [path isEqual:NSNull.null] || !path.length)
        return;

    const auto oldDirectory = m_directory;
    m_directory = QUrl::fromLocalFile(
        QString::fromNSString(path).normalized(QString::NormalizationForm_C));

    if (m_directory != oldDirectory) {
        // FIXME: Plumb old directory back to QFileDialog's lastVisitedDir?
        emit directoryEntered(m_directory);
    }
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
