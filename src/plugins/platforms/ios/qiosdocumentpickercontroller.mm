// Copyright (C) 2020 Harald Meyer.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#import <UIKit/UIKit.h>
#import <MobileCoreServices/MobileCoreServices.h>

#include "qiosdocumentpickercontroller.h"

#include <QtCore/qpointer.h>
#include <QtCore/private/qdarwinsecurityscopedfileengine_p.h>

@implementation QIOSDocumentPickerController {
    QPointer<QIOSFileDialog> m_fileDialog;
}

- (instancetype)initWithQIOSFileDialog:(QIOSFileDialog *)fileDialog
{
    NSMutableArray <UTType *> *docTypes = [[[NSMutableArray alloc] init] autorelease];

    const auto options = fileDialog->options();

    const QStringList nameFilters = options->nameFilters();
    if (!nameFilters.isEmpty() && (options->fileMode() != QFileDialogOptions::Directory
                               || options->fileMode() != QFileDialogOptions::DirectoryOnly))
    {
        QStringList results;
        for (const QString &filter : nameFilters)
            results.append(QPlatformFileDialogHelper::cleanFilterList(filter));

        docTypes = [self computeAllowedFileTypes:results];
    }

    if (!docTypes.count) {
        switch (options->fileMode()) {
        case QFileDialogOptions::AnyFile:
        case QFileDialogOptions::ExistingFile:
        case QFileDialogOptions::ExistingFiles:
            [docTypes addObject:UTTypeContent];
            [docTypes addObject:UTTypeItem];
            [docTypes addObject:UTTypeData];
            break;
        // Showing files is not supported in Directory mode in iOS
        case QFileDialogOptions::Directory:
        case QFileDialogOptions::DirectoryOnly:
            [docTypes addObject:UTTypeFolder];
            break;
        }
    }

    if (options->acceptMode() == QFileDialogOptions::AcceptSave) {
        auto selectedUrls = options->initiallySelectedFiles();
        auto suggestedFileName = !selectedUrls.isEmpty() ? selectedUrls.first().fileName() : "Untitled";

        // Create an empty dummy file, so that the export dialog will allow us
        // to choose the export destination, which we are then given access to
        // write to.
        NSURL *dummyExportFile = [NSFileManager.defaultManager.temporaryDirectory
            URLByAppendingPathComponent:suggestedFileName.toNSString()];
        [NSFileManager.defaultManager createFileAtPath:dummyExportFile.path contents:nil attributes:nil];

        if (!(self = [super initForExportingURLs:@[dummyExportFile]]))
            return nil;

        // Note, we don't set the directoryURL, as if the directory can't be
        // accessed, or written to, the file dialog is shown but is empty.
        // FIXME: See comment below for open dialogs as well
    } else {
        if (!(self = [super initForOpeningContentTypes:docTypes asCopy:NO]))
            return nil;

        if (options->fileMode() == QFileDialogOptions::ExistingFiles)
            self.allowsMultipleSelection = YES;

        // FIXME: This doesn't seem to have any effect
        self.directoryURL = options->initialDirectory().toNSURL();
    }

    m_fileDialog = fileDialog;
    self.modalPresentationStyle = UIModalPresentationFormSheet;
    self.delegate = self;
    self.presentationController.delegate = self;

    return self;
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray <NSURL *>*)urls
{
    Q_UNUSED(controller);

    if (!m_fileDialog)
        return;

    QList<QUrl> files;
    for (NSURL* url in urls)
        files.append(qt_apple_urlFromPossiblySecurityScopedURL(url));

    emit m_fileDialog->filesSelected(files);
    emit m_fileDialog->accept();
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
    if (!m_fileDialog)
        return;

    Q_UNUSED(controller);
    emit m_fileDialog->reject();
}

- (void)presentationControllerDidDismiss:(UIPresentationController *)presentationController
{
    if (!m_fileDialog)
        return;

    Q_UNUSED(presentationController);

    // "Called on the delegate when the user has taken action to dismiss the
    // presentation successfully, after all animations are finished.
    // This is not called if the presentation is dismissed programmatically."

    // So if document picker's view was dismissed, for example by swiping it away,
    // we got this method called. But not if the dialog was cancelled or a file
    // was selected.
    emit m_fileDialog->reject();
}

- (NSMutableArray<UTType*>*)computeAllowedFileTypes:(QStringList)filters
{
    QStringList fileTypes;
    for (const QString &filter : filters) {
        if (filter == (QLatin1String("*")))
            continue;

        if (filter.contains(u'?'))
            continue;

        if (filter.count(u'*') != 1)
            continue;

        auto extensions = filter.split('.', Qt::SkipEmptyParts);
        fileTypes += extensions.last();
    }

    NSMutableArray<UTType *> *result = [NSMutableArray<UTType *> arrayWithCapacity:fileTypes.size()];
    for (const QString &string : fileTypes)
        [result addObject:[UTType typeWithFilenameExtension:string.toNSString()]];

    return result;
}

@end
