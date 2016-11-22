/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QWINRTFILEDIALOGHELPER_H
#define QWINRTFILEDIALOGHELPER_H

#include <qpa/qplatformdialoghelper.h>
#include <QtCore/qt_windows.h>

struct IInspectable;
namespace ABI {
    namespace Windows {
        namespace Storage {
            class StorageFile;
            class StorageFolder;
            struct IStorageFile;
            struct IStorageFolder;
        }
        namespace Foundation {
            enum class AsyncStatus;
            template <typename T> struct IAsyncOperation;
            namespace Collections {
                template <typename T> struct IVectorView;
            }
        }
    }
}

QT_BEGIN_NAMESPACE

class QWinRTFileDialogHelperPrivate;
class QWinRTFileDialogHelper : public QPlatformFileDialogHelper
{
    Q_OBJECT
public:
    explicit QWinRTFileDialogHelper();
    ~QWinRTFileDialogHelper();

    void exec() override;
    bool show(Qt::WindowFlags, Qt::WindowModality, QWindow *) override;
    void hide() override;

    bool defaultNameFilterDisables() const override { return false; }
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    void selectFile(const QUrl &saveFileName) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override { }
    void selectNameFilter(const QString &selectedNameFilter) override;
    QString selectedNameFilter() const override;

    HRESULT onSingleFilePicked(ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::Storage::StorageFile *> *,
                               ABI::Windows::Foundation::AsyncStatus);
    HRESULT onMultipleFilesPicked(ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Storage::StorageFile *> *> *,
                                  ABI::Windows::Foundation::AsyncStatus);
    HRESULT onSingleFolderPicked(ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::Storage::StorageFolder *> *,
                                 ABI::Windows::Foundation::AsyncStatus);

private:
    HRESULT onFilesPicked(ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Storage::StorageFile *> *files);
    HRESULT onFolderPicked(ABI::Windows::Storage::IStorageFolder *folder);
    HRESULT onFilePicked(ABI::Windows::Storage::IStorageFile *file);
    void appendFile(IInspectable *);

    QScopedPointer<QWinRTFileDialogHelperPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTFileDialogHelper)
};

QT_END_NAMESPACE

#endif // QWINRTFILEDIALOGHELPER_H
