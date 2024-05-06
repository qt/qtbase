// Copyright (C) 2020 Harald Meyer.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>
#import <MobileCoreServices/MobileCoreServices.h>

#include "qiosdocumentpickercontroller.h"

@implementation QIOSDocumentPickerController {
    QIOSFileDialog *m_fileDialog;
}

- (instancetype)initWithQIOSFileDialog:(QIOSFileDialog *)fileDialog
{
    NSMutableArray <UTType *> *docTypes = [[[NSMutableArray alloc] init] autorelease];

    QStringList nameFilters = fileDialog->options()->nameFilters();
    if (!nameFilters.isEmpty() && (fileDialog->options()->fileMode() != QFileDialogOptions::Directory
                               || fileDialog->options()->fileMode() != QFileDialogOptions::DirectoryOnly))
    {
        QStringList results;
        for (const QString &filter : nameFilters)
            results.append(QPlatformFileDialogHelper::cleanFilterList(filter));

        docTypes = [self computeAllowedFileTypes:results];
    }

    if (!docTypes.count) {
        switch (fileDialog->options()->fileMode()) {
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

    if (self = [super initForOpeningContentTypes:docTypes]) {
        m_fileDialog = fileDialog;
        self.modalPresentationStyle = UIModalPresentationFormSheet;
        self.delegate = self;
        self.presentationController.delegate = self;

        if (m_fileDialog->options()->fileMode() == QFileDialogOptions::ExistingFiles)
            self.allowsMultipleSelection = YES;

        self.directoryURL = m_fileDialog->options()->initialDirectory().toNSURL();
    }
    return self;
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray <NSURL *>*)urls
{
    Q_UNUSED(controller);

    QList<QUrl> files;
    for (NSURL* url in urls)
        files.append(QUrl::fromNSURL(url));

    m_fileDialog->selectedFilesChanged(files);
    emit m_fileDialog->accept();
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
    Q_UNUSED(controller);
    emit m_fileDialog->reject();
}

- (void)presentationControllerDidDismiss:(UIPresentationController *)presentationController
{
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
