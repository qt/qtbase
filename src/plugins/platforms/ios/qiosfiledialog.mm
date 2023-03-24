// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>

#import <Photos/Photos.h>

#include <QtCore/qstandardpaths.h>
#include <QtGui/qwindow.h>
#include <QDebug>

#include <QtCore/private/qcore_mac_p.h>

#include "qiosfiledialog.h"
#include "qiosintegration.h"
#include "qiosoptionalplugininterface.h"
#include "qiosdocumentpickercontroller.h"

using namespace Qt::StringLiterals;

QIOSFileDialog::QIOSFileDialog()
    : m_viewController(nullptr)
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

    const bool acceptOpen = options()->acceptMode() == QFileDialogOptions::AcceptOpen;
    const auto initialDir = options()->initialDirectory();
    const QString directory = initialDir.toLocalFile();
    // We manually add assets-library:// to the list of paths,
    // when converted to QUrl, it becames a scheme.
    const QString scheme = initialDir.scheme();

    if (acceptOpen) {
        if (directory.startsWith("assets-library:"_L1) || scheme == "assets-library"_L1)
            return showImagePickerDialog(parent);
        else
            return showNativeDocumentPickerDialog(parent);
    }

    return false;
}

void QIOSFileDialog::showImagePickerDialog_helper(QWindow *parent)
{
    UIWindow *window = parent ? reinterpret_cast<UIView *>(parent->winId()).window
                              : qt_apple_sharedApplication().keyWindow;
    [window.rootViewController presentViewController:m_viewController animated:YES completion:nil];
}

bool QIOSFileDialog::showImagePickerDialog(QWindow *parent)
{
    if (!m_viewController) {
        QFactoryLoader *plugins = QIOSIntegration::instance()->optionalPlugins();
        qsizetype size = QList<QPluginParsedMetaData>(plugins->metaData()).size();
        for (qsizetype i = 0; i < size; ++i) {
            QIosOptionalPluginInterface *plugin = qobject_cast<QIosOptionalPluginInterface *>(plugins->instance(i));
            m_viewController = [plugin->createImagePickerController(this) retain];
            if (m_viewController)
                break;
        }
    }

    if (!m_viewController) {
        qWarning() << "QIOSFileDialog: Could not resolve Qt plugin that gives access to photos on iOS";
        return false;
    }

    // "Old style" authorization (deprecated, but we have to work with AssetsLibrary anyway).
    //
    // From the documentation:
    // "The authorizationStatus and requestAuthorization: methods aren’t compatible with the
    //  limited library and return PHAuthorizationStatusAuthorized when the user authorizes your
    //  app for limited access only."
    //
    // This is good enough for us.

    const auto authStatus = [PHPhotoLibrary authorizationStatus];
    if (authStatus == PHAuthorizationStatusAuthorized) {
        showImagePickerDialog_helper(parent);
    } else if (authStatus == PHAuthorizationStatusNotDetermined) {
        QPointer<QWindow> winGuard(parent);
        QPointer<QIOSFileDialog> thisGuard(this);
        [PHPhotoLibrary requestAuthorization:^(PHAuthorizationStatus status) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (status == PHAuthorizationStatusAuthorized) {
                    if (thisGuard && winGuard)
                        thisGuard->showImagePickerDialog_helper(winGuard);

                } else if (thisGuard) {
                    emit thisGuard->reject();
                }
            });
        }];
    } else {
        // Treat 'Limited' (we don't know how to deal with anyway) and 'Denied' as errors.
        // FIXME: logging category?
        qWarning() << "QIOSFileDialog: insufficient permission, cannot pick images";
        return false;
    }

    return true;
}

bool QIOSFileDialog::showNativeDocumentPickerDialog(QWindow *parent)
{
#ifndef Q_OS_TVOS
    m_viewController = [[QIOSDocumentPickerController alloc] initWithQIOSFileDialog:this];

    UIWindow *window = parent ? reinterpret_cast<UIView *>(parent->winId()).window
        : qt_apple_sharedApplication().keyWindow;
    [window.rootViewController presentViewController:m_viewController animated:YES completion:nil];

    return true;
#else
    return false;
#endif
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
    [m_viewController release];
    m_viewController = nullptr;
    m_eventLoop.exit();
}

QList<QUrl> QIOSFileDialog::selectedFiles() const
{
    return m_selection;
}

void QIOSFileDialog::selectedFilesChanged(const QList<QUrl> &selection)
{
    m_selection = selection;
    emit filesSelected(m_selection);
    if (m_selection.count() == 1)
        emit fileSelected(m_selection[0]);
}
