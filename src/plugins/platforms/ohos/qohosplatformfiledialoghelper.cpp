// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformdialoghelper.h"
#include "qohosplatformintegration.h"
#include "qohosplatformwindow.h"
#include <QtCore/qeventloop.h>
#include <QtCore/qfileinfo.h>
#include <qohosjsenv_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qwindow.h>
#include <algorithm>
#include <qohoswindowmanager.h>

QT_BEGIN_NAMESPACE

namespace {

QOhosWindowManager::DocumentSelectMode
mapQFileDialogOptionsToOhosDocumentSelectMode(const QFileDialogOptions &options)
{
    using FileMode = QFileDialogOptions::FileMode;
    using DocumentSelectMode = QOhosWindowManager::DocumentSelectMode;

    switch (options.fileMode()) {
    case FileMode::Directory:
    case FileMode::DirectoryOnly:
        return DocumentSelectMode::FOLDER;
    case FileMode::ExistingFile:
    case FileMode::ExistingFiles:
        return DocumentSelectMode::FILE;
    case FileMode::AnyFile:
        return DocumentSelectMode::MIXED;
    }
    return DocumentSelectMode::MIXED;
}

QOhosWindowManager::ResultMultiplicity
mapQFileDialogOptionsToOhosResultMultiplicity(const QFileDialogOptions &options)
{
    return options.fileMode() == QFileDialogOptions::FileMode::ExistingFile
        ? QOhosWindowManager::ResultMultiplicity::SINGLE
        : QOhosWindowManager::ResultMultiplicity::MULTIPLE;
}

QtOhos::InternalWindowId getQWindowInternalWindowIdOrFail(QWindow *qWindow)
{
    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(qWindow);
    auto windowId = platformWindow != nullptr
        ? platformWindow->internalWindowId()
        : QtOhos::InternalWindowId::invalidWindowId();

    if (!windowId.isValid())
        qOhosReportFatalErrorAndAbort("%s: Failed to retrieve window id", Q_FUNC_INFO);

    return windowId;
}

QtOhos::InternalWindowId tryGetFocusedWindowInternalWindowId()
{
    auto *focusedWindow = QGuiApplication::focusWindow();
    return focusedWindow != nullptr
        ? getQWindowInternalWindowIdOrFail(focusedWindow)
        : QtOhos::InternalWindowId::invalidWindowId();
}

class QOhosPlatformFileDialogHelperImpl : public QPlatformFileDialogHelper
{
public:
    QOhosPlatformFileDialogHelperImpl();
    ~QOhosPlatformFileDialogHelperImpl() = default;

    bool defaultNameFilterDisables() const override;
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    void selectFile(const QUrl &filename) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override;
    void selectNameFilter(const QString &filter) override;
    QString selectedNameFilter() const override;
    void hide() override;
    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;

private:
    void setDialogResult(bool accepted, QStringList files, QOhosOptional<int> optSelectedFilterIndex);
    static QStringList convertQtNameFiltersToOhosStandard(const QStringList &);

    QEventLoop m_eventLoop;
    bool m_shown;

    QUrl m_directory;
    QUrl m_selectFileName;
    QString m_selectedNameFilter;

    QList<QUrl> m_selectedFiles;
};

QOhosPlatformFileDialogHelperImpl::QOhosPlatformFileDialogHelperImpl()
    : QPlatformFileDialogHelper()
{
    m_shown = false;
}

bool QOhosPlatformFileDialogHelperImpl::defaultNameFilterDisables() const
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::defaultNameFilterDisables");
    return false;
}

void QOhosPlatformFileDialogHelperImpl::setDirectory(const QUrl &directory)
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::setDirectory");
    m_directory = directory;
}

QUrl QOhosPlatformFileDialogHelperImpl::directory() const
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::directory");
    return m_directory;
}

void QOhosPlatformFileDialogHelperImpl::selectFile(const QUrl &filename)
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::selectFile");
    m_selectFileName = filename;
}

void QOhosPlatformFileDialogHelperImpl::setFilter()
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::setFilter");
}

void QOhosPlatformFileDialogHelperImpl::selectNameFilter(const QString &filter)
{
    m_selectedNameFilter = filter;
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::selectNameFilter");
}

QString QOhosPlatformFileDialogHelperImpl::selectedNameFilter() const
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::selectedNameFilter");
    return m_selectedNameFilter;
}

QList<QUrl> QOhosPlatformFileDialogHelperImpl::selectedFiles() const
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::selectedFiles");
    return m_selectedFiles;
}

void QOhosPlatformFileDialogHelperImpl::hide()
{
    m_shown = false;
    // Programmatically closing the DocumentViewPicker is not supported by the
    // HarmonyOS platform API. The picker can only be dismissed by the user.
    // See: https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-file-picker#documentviewpicker
    m_eventLoop.exit();
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::hide()");
}

void QOhosPlatformFileDialogHelperImpl::setDialogResult(
    bool accepted, QStringList files, QOhosOptional<int> optSelectedFilterIndex)
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::dialogResult()");

    hide();
    m_selectedFiles.clear();
    if (accepted) {
        if (optSelectedFilterIndex.has_value()) {
            auto nameFilters = options()->nameFilters();
            auto selectedFilterIndex = optSelectedFilterIndex.value();
            if (selectedFilterIndex >= 0 && selectedFilterIndex < nameFilters.count())
                selectNameFilter(nameFilters.at(selectedFilterIndex));
        }
        std::copy(files.constBegin(), files.constEnd(), std::back_inserter(m_selectedFiles));
        emit accept();
    } else {
        emit reject();
    }

}

void QOhosPlatformFileDialogHelperImpl::exec()
{
    auto _dbg = make_QCScopedDebug("QOhosPlatformFileDialogHelperImpl::exec()");

    if (!m_shown)
        show(Qt::Dialog, Qt::ApplicationModal, 0);

    // Run event loop only if the call was successful
    if (m_shown)
        m_eventLoop.exec();
}

QStringList QOhosPlatformFileDialogHelperImpl::convertQtNameFiltersToOhosStandard(const QStringList &qtNameFilters)
{
    const auto qtAnyFilesFilter = QString::fromUtf8("*");
    const auto ohosAnyFilesFilter = QString::fromUtf8(".*");
    QStringList ohosNameFilters;
    for (const auto &qtFilter : qtNameFilters) {
        QStringList qtCleanFilters = QPlatformFileDialogHelper::cleanFilterList(qtFilter);
        QStringList ohosExtensionList;
        for (auto &qtCleanFilter : qtCleanFilters) {
            QString extensionOnly = qtCleanFilter.replace(QLatin1String("*."), QLatin1String("."));
            ohosExtensionList.append(extensionOnly);
        }
        QString ohosExtensionsSeparatedByComma = ohosExtensionList.join(QString::fromUtf8(","));
        if (ohosExtensionsSeparatedByComma == qtAnyFilesFilter)
            ohosExtensionsSeparatedByComma = ohosAnyFilesFilter;
        auto ohosNameFilter = qtFilter + QLatin1String("|") + ohosExtensionsSeparatedByComma;
        ohosNameFilters.append(ohosNameFilter);
    }
    return ohosNameFilters;
}

bool QOhosPlatformFileDialogHelperImpl::show(
    Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags)
    Q_UNUSED(windowModality)

    const auto contextWinId = parent != nullptr
        ? getQWindowInternalWindowIdOrFail(parent)
        : tryGetFocusedWindowInternalWindowId();

    // TODO: don't assume that "this" is always alive
    QSharedPointer<QFileDialogOptions> opt = options();
    auto ohosNameFilters = convertQtNameFiltersToOhosStandard(opt->nameFilters());
    if (opt->acceptMode() == QFileDialogOptions::AcceptOpen)
        QOhosWindowManager::showFileDialogOpen(
            contextWinId,
            ohosNameFilters,
            !m_selectFileName.isEmpty()
                ? m_selectFileName.toLocalFile()
                : !m_directory.isEmpty()
                    ? m_directory.toLocalFile()
                    : QString(),
            mapQFileDialogOptionsToOhosDocumentSelectMode(*opt),
            mapQFileDialogOptionsToOhosResultMultiplicity(*opt),
            [this](QOhosOptional<QOhosWindowManager::OpenResult> optOpenResult) {
                auto filesPaths = optOpenResult.has_value()
                    ? optOpenResult.value().selectedUrls
                    : QStringList();
                setDialogResult(!filesPaths.isEmpty(), filesPaths, {});
            });
    else
        QOhosWindowManager::showFileDialogSave(
            contextWinId,
            !m_selectFileName.isEmpty()
                ? QStringList(QFileInfo(m_selectFileName.toLocalFile()).fileName())
                : QStringList(),
            !m_directory.isEmpty()
                ? m_directory.toLocalFile()
                : !m_selectFileName.isEmpty()
                    ? QFileInfo(m_selectFileName.toLocalFile()).absoluteDir().path()
                    : QString(),
            ohosNameFilters,
            [this](QOhosOptional<QOhosWindowManager::SaveResult> optSaveResult) {
                auto filesPaths = optSaveResult.has_value()
                    ? optSaveResult.value().savedUrls
                    : QStringList();
                auto optSelectedFilterIndex = qTransform(
                    optSaveResult,
                    [](const auto &saveResult) {
                        return saveResult.selectedFileSuffixChoiceIndex;
                    });
                setDialogResult(!filesPaths.isEmpty(), filesPaths, optSelectedFilterIndex);
            });
    m_shown = true;

    return true;
}

}

QPlatformFileDialogHelper *makeQOhosPlatformFileDialogHelper()
{
    return new QOhosPlatformFileDialogHelperImpl();
}

QT_END_NAMESPACE
