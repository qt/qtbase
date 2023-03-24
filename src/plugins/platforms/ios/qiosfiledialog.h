// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSFILEDIALOG_H
#define QIOSFILEDIALOG_H

#include <QtCore/qeventloop.h>
#include <qpa/qplatformdialoghelper.h>

Q_FORWARD_DECLARE_OBJC_CLASS(UIViewController);

QT_BEGIN_NAMESPACE

class QIOSFileDialog : public QPlatformFileDialogHelper
{
public:
    QIOSFileDialog();
    ~QIOSFileDialog();

    void exec() override;
    bool defaultNameFilterDisables() const override { return false; }
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;
    void setDirectory(const QUrl &) override {}
    QUrl directory() const override { return QUrl(); }
    void selectFile(const QUrl &) override {}
    QList<QUrl> selectedFiles() const override;
    void setFilter() override {}
    void selectNameFilter(const QString &) override {}
    QString selectedNameFilter() const override { return QString(); }

    void selectedFilesChanged(const QList<QUrl> &selection);

private:
    QUrl m_directory;
    QList<QUrl> m_selection;
    QEventLoop m_eventLoop;
    UIViewController *m_viewController;

    bool showImagePickerDialog(QWindow *parent);
    bool showNativeDocumentPickerDialog(QWindow *parent);
    void showImagePickerDialog_helper(QWindow *parent);
};

QT_END_NAMESPACE

#endif // QIOSFILEDIALOG_H

