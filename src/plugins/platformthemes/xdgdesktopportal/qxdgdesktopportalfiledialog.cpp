// Copyright (C) 2017-2018 Red Hat, Inc
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxdgdesktopportalfiledialog_p.h"

#include <private/qdesktopunixservices_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusMetaType>

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMetaType>
#include <QMimeType>
#include <QMimeDatabase>
#include <QRandomGenerator>
#include <QWindow>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QDBusArgument &operator <<(QDBusArgument &arg, const QXdgDesktopPortalFileDialog::FilterCondition &filterCondition)
{
    arg.beginStructure();
    arg << filterCondition.type << filterCondition.pattern;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator >>(const QDBusArgument &arg, QXdgDesktopPortalFileDialog::FilterCondition &filterCondition)
{
    uint type;
    QString filterPattern;
    arg.beginStructure();
    arg >> type >> filterPattern;
    filterCondition.type = (QXdgDesktopPortalFileDialog::ConditionType)type;
    filterCondition.pattern = filterPattern;
    arg.endStructure();

    return arg;
}

QDBusArgument &operator <<(QDBusArgument &arg, const QXdgDesktopPortalFileDialog::Filter filter)
{
    arg.beginStructure();
    arg << filter.name << filter.filterConditions;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator >>(const QDBusArgument &arg, QXdgDesktopPortalFileDialog::Filter &filter)
{
    QString name;
    QXdgDesktopPortalFileDialog::FilterConditionList filterConditions;
    arg.beginStructure();
    arg >> name >> filterConditions;
    filter.name = name;
    filter.filterConditions = filterConditions;
    arg.endStructure();

    return arg;
}

class QXdgDesktopPortalFileDialogPrivate
{
public:
    QXdgDesktopPortalFileDialogPrivate(QPlatformFileDialogHelper *nativeFileDialog, uint fileChooserPortalVersion)
        : nativeFileDialog(nativeFileDialog)
        , fileChooserPortalVersion(fileChooserPortalVersion)
    { }

    QEventLoop loop;
    QString acceptLabel;
    QUrl directory;
    QString title;
    QStringList nameFilters;
    QStringList mimeTypesFilters;
    // maps user-visible name for portal to full name filter
    QMap<QString, QString> userVisibleToNameFilter;
    QString selectedMimeTypeFilter;
    QString selectedNameFilter;
    QStringList selectedFiles;
    std::unique_ptr<QPlatformFileDialogHelper> nativeFileDialog;
    uint fileChooserPortalVersion = 0;
    bool failedToOpen = false;
    bool directoryMode = false;
    bool multipleFiles = false;
    bool saveFile = false;
};

QXdgDesktopPortalFileDialog::QXdgDesktopPortalFileDialog(QPlatformFileDialogHelper *nativeFileDialog, uint fileChooserPortalVersion)
    : QPlatformFileDialogHelper()
    , d_ptr(new QXdgDesktopPortalFileDialogPrivate(nativeFileDialog, fileChooserPortalVersion))
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog) {
        connect(d->nativeFileDialog.get(), SIGNAL(accept()), this, SIGNAL(accept()));
        connect(d->nativeFileDialog.get(), SIGNAL(reject()), this, SIGNAL(reject()));
    }

    d->loop.connect(this, SIGNAL(accept()), SLOT(quit()));
    d->loop.connect(this, SIGNAL(reject()), SLOT(quit()));
}

QXdgDesktopPortalFileDialog::~QXdgDesktopPortalFileDialog()
{
}

void QXdgDesktopPortalFileDialog::initializeDialog()
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog)
        d->nativeFileDialog->setOptions(options());

    if (options()->fileMode() == QFileDialogOptions::ExistingFiles)
        d->multipleFiles = true;

    if (options()->fileMode() == QFileDialogOptions::Directory || options()->fileMode() == QFileDialogOptions::DirectoryOnly)
        d->directoryMode = true;

    if (options()->isLabelExplicitlySet(QFileDialogOptions::Accept))
        d->acceptLabel = options()->labelText(QFileDialogOptions::Accept);

    if (!options()->windowTitle().isEmpty())
        d->title = options()->windowTitle();

    if (options()->acceptMode() == QFileDialogOptions::AcceptSave)
        d->saveFile = true;

    if (!options()->nameFilters().isEmpty())
        d->nameFilters = options()->nameFilters();

    if (!options()->mimeTypeFilters().isEmpty())
        d->mimeTypesFilters = options()->mimeTypeFilters();

    if (!options()->initiallySelectedMimeTypeFilter().isEmpty())
        d->selectedMimeTypeFilter = options()->initiallySelectedMimeTypeFilter();

    if (!options()->initiallySelectedNameFilter().isEmpty())
        d->selectedNameFilter = options()->initiallySelectedNameFilter();

    setDirectory(options()->initialDirectory());
}

void QXdgDesktopPortalFileDialog::openPortal(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_D(QXdgDesktopPortalFileDialog);

    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop"_L1,
                                                          "/org/freedesktop/portal/desktop"_L1,
                                                          "org.freedesktop.portal.FileChooser"_L1,
                                                          d->saveFile ? "SaveFile"_L1 : "OpenFile"_L1);
    QVariantMap options;
    if (!d->acceptLabel.isEmpty())
        options.insert("accept_label"_L1, d->acceptLabel);

    options.insert("modal"_L1, windowModality != Qt::NonModal);
    options.insert("multiple"_L1, d->multipleFiles);
    options.insert("directory"_L1, d->directoryMode);

    if (!d->directory.isEmpty())
        options.insert("current_folder"_L1, QFile::encodeName(d->directory.toLocalFile()).append('\0'));

    if (d->saveFile && !d->selectedFiles.isEmpty()) {
        // current_file for the file to be pre-selected, current_name for the file name to be
        // pre-filled current_file accepts absolute path and requires the file to exist while
        // current_name accepts just file name
        QFileInfo selectedFileInfo(d->selectedFiles.constFirst());
        if (selectedFileInfo.exists())
            options.insert("current_file"_L1,
                           QFile::encodeName(d->selectedFiles.constFirst()).append('\0'));
        options.insert("current_name"_L1, selectedFileInfo.fileName());
    }

    // Insert filters
    qDBusRegisterMetaType<FilterCondition>();
    qDBusRegisterMetaType<FilterConditionList>();
    qDBusRegisterMetaType<Filter>();
    qDBusRegisterMetaType<FilterList>();

    FilterList filterList;
    auto selectedFilterIndex = filterList.size() - 1;

    d->userVisibleToNameFilter.clear();

    if (!d->mimeTypesFilters.isEmpty()) {
        for (const QString &mimeTypefilter : d->mimeTypesFilters) {
            QMimeDatabase mimeDatabase;
            QMimeType mimeType = mimeDatabase.mimeTypeForName(mimeTypefilter);

            // Creates e.g. (1, "image/png")
            FilterCondition filterCondition;
            filterCondition.type = MimeType;
            filterCondition.pattern = mimeTypefilter;

            // Creates e.g. [((1, "image/png"))]
            FilterConditionList filterConditions;
            filterConditions << filterCondition;

            // Creates e.g. [("Images", [((1, "image/png"))])]
            Filter filter;
            filter.name = mimeType.comment();
            filter.filterConditions = filterConditions;

            if (filter.name.isEmpty())
                filter.name = mimeTypefilter;

            filterList << filter;

            if (!d->selectedMimeTypeFilter.isEmpty() && d->selectedMimeTypeFilter == mimeTypefilter)
                selectedFilterIndex = filterList.size() - 1;
        }
    } else if (!d->nameFilters.isEmpty()) {
        for (const QString &nameFilter : d->nameFilters) {
            // Do parsing:
            // Supported format is ("Images (*.png *.jpg)")
            QRegularExpression regexp(QPlatformFileDialogHelper::filterRegExp);
            QRegularExpressionMatch match = regexp.match(nameFilter);
            if (match.hasMatch()) {
                QString userVisibleName = match.captured(1);
                QStringList filterStrings = match.captured(2).split(u' ', Qt::SkipEmptyParts);

                if (filterStrings.isEmpty()) {
                    qWarning() << "Filter " << userVisibleName << " is empty and will be ignored.";
                    continue;
                }

                FilterConditionList filterConditions;
                for (const QString &filterString : filterStrings) {
                    FilterCondition filterCondition;
                    filterCondition.type = GlobalPattern;
                    filterCondition.pattern = filterString;
                    filterConditions << filterCondition;
                }

                Filter filter;
                filter.name = userVisibleName;
                filter.filterConditions = filterConditions;

                filterList << filter;

                d->userVisibleToNameFilter.insert(userVisibleName, nameFilter);

                if (!d->selectedNameFilter.isEmpty() && d->selectedNameFilter == nameFilter)
                    selectedFilterIndex = filterList.size() - 1;
            }
        }
    }

    if (!filterList.isEmpty())
        options.insert("filters"_L1, QVariant::fromValue(filterList));

    if (selectedFilterIndex != -1)
        options.insert("current_filter"_L1, QVariant::fromValue(filterList[selectedFilterIndex]));

    options.insert("handle_token"_L1, QStringLiteral("qt%1").arg(QRandomGenerator::global()->generate()));

    // TODO choices a(ssa(ss)s)
    // List of serialized combo boxes to add to the file chooser.

    auto unixServices = dynamic_cast<QDesktopUnixServices *>(
            QGuiApplicationPrivate::platformIntegration()->services());
    if (parent && unixServices)
        message << unixServices->portalWindowIdentifier(parent);
    else
        message << QString();

    message << d->title << options;

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, d, windowFlags, windowModality, parent] (QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        // Any error means the dialog is not shown and we need to fallback
        d->failedToOpen = reply.isError();
        if (d->failedToOpen) {
            if (d->nativeFileDialog) {
                d->nativeFileDialog->show(windowFlags, windowModality, parent);
                if (d->loop.isRunning())
                    d->nativeFileDialog->exec();
            } else {
                Q_EMIT reject();
            }
        } else {
            QDBusConnection::sessionBus().connect(nullptr,
                                                  reply.value().path(),
                                                  "org.freedesktop.portal.Request"_L1,
                                                  "Response"_L1,
                                                  this,
                                                  SLOT(gotResponse(uint,QVariantMap)));
        }
        watcher->deleteLater();
    });
}

bool QXdgDesktopPortalFileDialog::defaultNameFilterDisables() const
{
    return false;
}

void QXdgDesktopPortalFileDialog::setDirectory(const QUrl &directory)
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog) {
        d->nativeFileDialog->setOptions(options());
        d->nativeFileDialog->setDirectory(directory);
    }

    d->directory = directory;
}

QUrl QXdgDesktopPortalFileDialog::directory() const
{
    Q_D(const QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog && useNativeFileDialog())
        return d->nativeFileDialog->directory();

    return d->directory;
}

void QXdgDesktopPortalFileDialog::selectFile(const QUrl &filename)
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog) {
        d->nativeFileDialog->setOptions(options());
        d->nativeFileDialog->selectFile(filename);
    }

    d->selectedFiles << filename.path();
}

QList<QUrl> QXdgDesktopPortalFileDialog::selectedFiles() const
{
    Q_D(const QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog && useNativeFileDialog())
        return d->nativeFileDialog->selectedFiles();

    QList<QUrl> files;
    for (const QString &file : d->selectedFiles) {
        files << QUrl(file);
    }
    return files;
}

void QXdgDesktopPortalFileDialog::setFilter()
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog) {
        d->nativeFileDialog->setOptions(options());
        d->nativeFileDialog->setFilter();
    }
}

void QXdgDesktopPortalFileDialog::selectMimeTypeFilter(const QString &filter)
{
    Q_D(QXdgDesktopPortalFileDialog);
    if (d->nativeFileDialog) {
        d->nativeFileDialog->setOptions(options());
        d->nativeFileDialog->selectMimeTypeFilter(filter);
    }
}

QString QXdgDesktopPortalFileDialog::selectedMimeTypeFilter() const
{
    Q_D(const QXdgDesktopPortalFileDialog);
    return d->selectedMimeTypeFilter;
}

void QXdgDesktopPortalFileDialog::selectNameFilter(const QString &filter)
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog) {
        d->nativeFileDialog->setOptions(options());
        d->nativeFileDialog->selectNameFilter(filter);
    }
}

QString QXdgDesktopPortalFileDialog::selectedNameFilter() const
{
    Q_D(const QXdgDesktopPortalFileDialog);
    return d->selectedNameFilter;
}

void QXdgDesktopPortalFileDialog::exec()
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog && useNativeFileDialog()) {
        d->nativeFileDialog->exec();
        return;
    }

    // HACK we have to avoid returning until we emit that the dialog was accepted or rejected
    d->loop.exec();
}

void QXdgDesktopPortalFileDialog::hide()
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog)
        d->nativeFileDialog->hide();
}

bool QXdgDesktopPortalFileDialog::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_D(QXdgDesktopPortalFileDialog);

    initializeDialog();

    if (d->nativeFileDialog && useNativeFileDialog(OpenFallback))
        return d->nativeFileDialog->show(windowFlags, windowModality, parent);

    openPortal(windowFlags, windowModality, parent);

    return true;
}

void QXdgDesktopPortalFileDialog::gotResponse(uint response, const QVariantMap &results)
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (!response) {
        if (results.contains("uris"_L1))
            d->selectedFiles = results.value("uris"_L1).toStringList();

        if (results.contains("current_filter"_L1)) {
            const Filter selectedFilter = qdbus_cast<Filter>(results.value(QStringLiteral("current_filter")));
            if (!selectedFilter.filterConditions.empty() && selectedFilter.filterConditions[0].type == MimeType) {
                // s.a. QXdgDesktopPortalFileDialog::openPortal which basically does the inverse
                d->selectedMimeTypeFilter = selectedFilter.filterConditions[0].pattern;
                d->selectedNameFilter.clear();
            } else {
                d->selectedNameFilter = d->userVisibleToNameFilter.value(selectedFilter.name);
                d->selectedMimeTypeFilter.clear();
            }
        }
        Q_EMIT accept();
    } else {
        Q_EMIT reject();
    }
}

bool QXdgDesktopPortalFileDialog::useNativeFileDialog(QXdgDesktopPortalFileDialog::FallbackType fallbackType) const
{
    Q_D(const QXdgDesktopPortalFileDialog);

    if (d->failedToOpen && fallbackType != OpenFallback)
        return true;

    if (d->fileChooserPortalVersion < 3) {
        if (options()->fileMode() == QFileDialogOptions::Directory)
            return true;
        else if (options()->fileMode() == QFileDialogOptions::DirectoryOnly)
            return true;
    }

    return false;
}

QT_END_NAMESPACE

#include "moc_qxdgdesktopportalfiledialog_p.cpp"
