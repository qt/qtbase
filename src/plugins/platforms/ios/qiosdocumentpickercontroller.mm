/****************************************************************************
**
** Copyright (C) 2020 Harald Meyer.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
