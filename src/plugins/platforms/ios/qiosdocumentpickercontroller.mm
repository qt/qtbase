/****************************************************************************
**
** Copyright (C) 2020 Harald Meyer.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

        if (m_fileDialog->options()->fileMode() == QFileDialogOptions::ExistingFiles)
            self.allowsMultipleSelection = YES;

        if (@available(ios 13.0, *))
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
    Q_UNUSED(controller)
    emit m_fileDialog->reject();
}

@end
