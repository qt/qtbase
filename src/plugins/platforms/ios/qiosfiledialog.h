/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
};

QT_END_NAMESPACE

#endif // QIOSFILEDIALOG_H

