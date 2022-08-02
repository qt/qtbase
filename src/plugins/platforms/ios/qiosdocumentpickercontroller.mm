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
    NSMutableArray <NSString *> *docTypes = [[[NSMutableArray alloc] init] autorelease];
    UIDocumentPickerMode importMode;
    switch (fileDialog->options()->fileMode()) {
    case QFileDialogOptions::AnyFile:
    case QFileDialogOptions::ExistingFile:
    case QFileDialogOptions::ExistingFiles:
        [docTypes addObject:(__bridge NSString *)kUTTypeContent];
        [docTypes addObject:(__bridge NSString *)kUTTypeItem];
        [docTypes addObject:(__bridge NSString *)kUTTypeData];
        importMode = UIDocumentPickerModeImport;
        break;
    case QFileDialogOptions::Directory:
    case QFileDialogOptions::DirectoryOnly:
        // Directory picking is not supported because it requires
        // special handling not possible with the current QFilePicker
        // implementation.

        Q_UNREACHABLE();
    }

    if (self = [super initWithDocumentTypes:docTypes inMode:importMode]) {
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

@end
