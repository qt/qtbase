/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiosfiledialog.h"

#import <UIKit/UIKit.h>

#include <QtCore/qstandardpaths.h>
#include <QtGui/qwindow.h>

@interface QIOSImagePickerController : UIImagePickerController <UIImagePickerControllerDelegate, UINavigationControllerDelegate> {
    QIOSFileDialog *m_fileDialog;
}
@end

@implementation QIOSImagePickerController

- (id)initWithQIOSFileDialog:(QIOSFileDialog *)fileDialog
{
    self = [super init];
    if (self) {
        m_fileDialog = fileDialog;
        [self setSourceType:UIImagePickerControllerSourceTypePhotoLibrary];
        [self setDelegate:self];
    }
    return self;
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
{
    Q_UNUSED(picker);
    NSURL *url = [info objectForKey:UIImagePickerControllerReferenceURL];
    m_fileDialog->selectedFilesChanged(QList<QUrl>() << QUrl::fromNSURL(url));
    emit m_fileDialog->accept();
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
    Q_UNUSED(picker)
    emit m_fileDialog->reject();
}

@end

// --------------------------------------------------------------------------

QIOSFileDialog::QIOSFileDialog()
    : m_viewController(0)
{
}

QIOSFileDialog::~QIOSFileDialog()
{
    [m_viewController release];
}

void QIOSFileDialog::exec()
{
    m_eventLoop.exec(QEventLoop::DialogExec);
}

bool QIOSFileDialog::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags);
    Q_UNUSED(windowModality);

    if (options()->initialDirectory().scheme() == QLatin1String("assets-library")) {
        m_viewController = [[QIOSImagePickerController alloc] initWithQIOSFileDialog:this];
        UIWindow *window = parent ? reinterpret_cast<UIView *>(parent->winId()).window
            : [UIApplication sharedApplication].keyWindow;
        [window.rootViewController presentViewController:m_viewController animated:YES completion:nil];
        return true;
    }

    return false;
}

void QIOSFileDialog::hide()
{
    // QFileDialog will remember the last directory set, and open subsequent dialogs in the same
    // directory for convenience. This works for normal file dialogs, but not when using native
    // pickers. Those can only be used for picking specific types, without support for normal file
    // system navigation. To avoid showing a native picker by accident, we change directory back
    // before we return. More could have been done to preserve the "last directory" logic here, but
    // navigating the file system on iOS is not recommended in the first place, so we keep it simple.
    emit directoryEntered(QUrl::fromLocalFile(QDir::currentPath()));

    [m_viewController dismissViewControllerAnimated:YES completion:nil];
    m_eventLoop.exit();
}

QList<QUrl> QIOSFileDialog::selectedFiles() const
{
    return m_selection;
}

void QIOSFileDialog::selectedFilesChanged(QList<QUrl> selection)
{
    m_selection = selection;
    emit filesSelected(m_selection);
    if (m_selection.count() == 1)
        emit fileSelected(m_selection[0]);
}
