// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmfiledialoghelper.h"

#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtGui/private/qwasmlocalfileaccess_p.h>
#include <QtCore/private/qwasmlocalfileengine_p.h>

QT_BEGIN_NAMESPACE

QWasmFileDialogHelper::QWasmFileDialogHelper()
    : m_eventLoop(nullptr)
{

}

QWasmFileDialogHelper::~QWasmFileDialogHelper()
{

}

bool QWasmFileDialogHelper::defaultNameFilterDisables() const
{
    return false;
}

void QWasmFileDialogHelper::setDirectory(const QUrl &directory)
{
    Q_UNUSED(directory)
}

QUrl QWasmFileDialogHelper::directory() const
{
    return QUrl();
}

void QWasmFileDialogHelper::selectFile(const QUrl &file)
{
    m_selectedFiles.clear();
    m_selectedFiles.append(file);
}

QList<QUrl> QWasmFileDialogHelper::selectedFiles() const
{
    return m_selectedFiles;
}

void QWasmFileDialogHelper::setFilter()
{

}

void QWasmFileDialogHelper::selectNameFilter(const QString &filter)
{
    Q_UNUSED(filter);
    // TODO
}

QString QWasmFileDialogHelper::selectedNameFilter() const
{
    return QString();
}

void QWasmFileDialogHelper::exec()
{
    QEventLoop eventLoop;
    m_eventLoop = &eventLoop;
    eventLoop.exec();
    m_eventLoop = nullptr;
}

bool QWasmFileDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    Q_UNUSED(flags)
    Q_UNUSED(modality)
    Q_UNUSED(parent)
    showFileDialog();
    return true;
}

void QWasmFileDialogHelper::hide()
{

}

void QWasmFileDialogHelper::showFileDialog()
{
    if (options()->acceptMode() == QFileDialogOptions::AcceptOpen) {
        // Use name filters from options
        QString nameFilter = options()->nameFilters().join(";;");
        if (nameFilter.isEmpty())
            nameFilter = "*";

        QWasmLocalFileAccess::showOpenFileDialog(nameFilter.toStdString(), [this](bool accepted, std::vector<qstdweb::File> files) {
            onOpenDialogClosed(accepted, files);
        });
    } else if (options()->acceptMode() == QFileDialogOptions::AcceptSave) {
        QString suggestion = m_selectedFiles.isEmpty() ? QString() : QUrl(m_selectedFiles.first()).fileName();
        m_selectedFiles.clear();

        QWasmLocalFileAccess::showSaveFileDialog(suggestion.toStdString(), [this](bool accepted, qstdweb::FileSystemFileHandle file){
            onSaveDialogClosed(accepted, file);
        });
    }
}

void QWasmFileDialogHelper::onOpenDialogClosed(bool accepted, std::vector<qstdweb::File> files)
{
    m_selectedFiles.clear();

    if (!accepted) {
        emit reject();
        return;
    }

    // Track opened files
    for (const auto &file : files) {
        QString wasmFileName = QWasmFileEngineHandler::addFile(file);
        QUrl fileUrl(wasmFileName);
        m_selectedFiles.append(fileUrl);
    }

    // Emit signals
    if (m_selectedFiles.size() > 0) {
        emit fileSelected(m_selectedFiles.first());
        emit filesSelected(m_selectedFiles);
    }
    emit accept();

    // exit exec() if in exec()
    if (m_eventLoop)
        m_eventLoop->quit();
}

void QWasmFileDialogHelper::onSaveDialogClosed(bool accepted, qstdweb::FileSystemFileHandle file)
{
    if (!accepted) {
        emit reject();
        return;
    }

    // Track save file
    QString wasmFileName = QWasmFileEngineHandler::addFile(file);
    QUrl fileUrl(wasmFileName);
    m_selectedFiles.append(fileUrl);

    // Emit signals
    emit fileSelected(m_selectedFiles.first());
    emit accept();

    if (m_eventLoop)
        m_eventLoop->quit();
}

QT_END_NAMESPACE
