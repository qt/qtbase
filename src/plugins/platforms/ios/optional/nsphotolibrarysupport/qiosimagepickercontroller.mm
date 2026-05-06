// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#import <UIKit/UIKit.h>

#include "qiosimagepickercontroller.h"

@implementation QIOSImagePickerController {
    QPlatformFileDialogHelper *m_fileDialog;
}

- (instancetype)initWithQIOSFileDialog:(QPlatformFileDialogHelper *)fileDialog
{
    self = [super init];
    if (self) {
        m_fileDialog = fileDialog;
        self.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
        self.delegate = self;
    }
    return self;
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
    Q_UNUSED(picker);
    NSURL *url = info[UIImagePickerControllerReferenceURL];
    QUrl fileUrl = QUrl::fromLocalFile(QString::fromNSString(url.description));
    emit m_fileDialog->filesSelected(QList<QUrl>() << fileUrl);
    emit m_fileDialog->accept();
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
    Q_UNUSED(picker);
    emit m_fileDialog->reject();
}

@end
