// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMFILEDIALOGHELPER_H
#define QWASMFILEDIALOGHELPER_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QEventLoop>
#include <QtGui/qpa/qplatformdialoghelper.h>
#include <QtGui/private/qwasmlocalfileaccess_p.h>

QT_BEGIN_NAMESPACE

class QWasmFileDialogHelper : public QPlatformFileDialogHelper
{
    Q_OBJECT
public:
    QWasmFileDialogHelper();
    ~QWasmFileDialogHelper();
public:
    virtual void exec() override;
    virtual bool show(Qt::WindowFlags windowFlags,
                          Qt::WindowModality windowModality,
                          QWindow *parent) override;
    virtual void hide() override;
    virtual bool defaultNameFilterDisables() const override;
    virtual void setDirectory(const QUrl &directory) override;
    virtual QUrl directory() const override;
    virtual void selectFile(const QUrl &filename) override;
    virtual QList<QUrl> selectedFiles() const override;
    virtual void setFilter() override;
    virtual void selectNameFilter(const QString &filter) override;
    virtual QString selectedNameFilter() const override;
    static QStringList cleanFilterList(const QString &filter);
signals:
    void fileDone(const QUrl &);
private:
    void showFileDialog();
    void onOpenDialogClosed(bool accepted, std::vector<qstdweb::File> files);
    void onSaveDialogClosed(bool accepted, qstdweb::FileSystemFileHandle file);

    QList<QUrl> m_selectedFiles;
    QEventLoop *m_eventLoop;
};

QT_END_NAMESPACE

#endif // QWASMFILEDIALOGHELPER_H
