/****************************************************************************
**
** Copyright (C) 2017-2018 Red Hat, Inc
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

#include "qxdgdesktopportalfiledialog_p.h"

#include <QtCore/qeventloop.h>

#include <QtDBus/QtDBus>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

#include <QFile>
#include <QMetaType>
#include <QMimeType>
#include <QMimeDatabase>
#include <QRandomGenerator>
#include <QWindow>

QT_BEGIN_NAMESPACE

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
    QXdgDesktopPortalFileDialogPrivate(QPlatformFileDialogHelper *nativeFileDialog)
        : nativeFileDialog(nativeFileDialog)
    { }

    WId winId = 0;
    bool modal = false;
    bool multipleFiles = false;
    bool saveFile = false;
    QString acceptLabel;
    QString directory;
    QString title;
    QStringList nameFilters;
    QStringList mimeTypesFilters;
    QStringList selectedFiles;
    QPlatformFileDialogHelper *nativeFileDialog = nullptr;
};

QXdgDesktopPortalFileDialog::QXdgDesktopPortalFileDialog(QPlatformFileDialogHelper *nativeFileDialog)
    : QPlatformFileDialogHelper()
    , d_ptr(new QXdgDesktopPortalFileDialogPrivate(nativeFileDialog))
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog) {
        connect(d->nativeFileDialog, SIGNAL(accept()), this, SIGNAL(accept()));
        connect(d->nativeFileDialog, SIGNAL(reject()), this, SIGNAL(reject()));
    }
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

    setDirectory(options()->initialDirectory());
}

void QXdgDesktopPortalFileDialog::openPortal()
{
    Q_D(const QXdgDesktopPortalFileDialog);

    QDBusMessage message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.portal.Desktop"),
                                                          QLatin1String("/org/freedesktop/portal/desktop"),
                                                          QLatin1String("org.freedesktop.portal.FileChooser"),
                                                          d->saveFile ? QLatin1String("SaveFile") : QLatin1String("OpenFile"));
    QString parentWindowId = QLatin1String("x11:") + QString::number(d->winId);

    QVariantMap options;
    if (!d->acceptLabel.isEmpty())
        options.insert(QLatin1String("accept_label"), d->acceptLabel);

    options.insert(QLatin1String("modal"), d->modal);
    options.insert(QLatin1String("multiple"), d->multipleFiles);

    if (d->saveFile) {
        if (!d->directory.isEmpty())
            options.insert(QLatin1String("current_folder"), QFile::encodeName(d->directory).append('\0'));

        if (!d->selectedFiles.isEmpty())
            options.insert(QLatin1String("current_file"), QFile::encodeName(d->selectedFiles.first()).append('\0'));
    }

    // Insert filters
    qDBusRegisterMetaType<FilterCondition>();
    qDBusRegisterMetaType<FilterConditionList>();
    qDBusRegisterMetaType<Filter>();
    qDBusRegisterMetaType<FilterList>();

    FilterList filterList;

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

            filterList << filter;
        }
    } else if (!d->nameFilters.isEmpty()) {
        for (const QString &filter : d->nameFilters) {
            // Do parsing:
            // Supported format is ("Images (*.png *.jpg)")
            QRegularExpression regexp(QPlatformFileDialogHelper::filterRegExp);
            QRegularExpressionMatch match = regexp.match(filter);
            if (match.hasMatch()) {
                QString userVisibleName = match.captured(1);
                QStringList filterStrings = match.captured(2).split(QLatin1Char(' '), QString::SkipEmptyParts);

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
            }
        }
    }

    if (!filterList.isEmpty())
        options.insert(QLatin1String("filters"), QVariant::fromValue(filterList));

    options.insert(QLatin1String("handle_token"), QStringLiteral("qt%1").arg(QRandomGenerator::global()->generate()));

    // TODO choices a(ssa(ss)s)
    // List of serialized combo boxes to add to the file chooser.

    message << parentWindowId << d->title << options;

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this] (QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            Q_EMIT reject();
        } else {
            QDBusConnection::sessionBus().connect(nullptr,
                                                  reply.value().path(),
                                                  QLatin1String("org.freedesktop.portal.Request"),
                                                  QLatin1String("Response"),
                                                  this,
                                                  SLOT(gotResponse(uint,QVariantMap)));
        }
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

    d->directory = directory.path();
}

QUrl QXdgDesktopPortalFileDialog::directory() const
{
    Q_D(const QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog && (options()->fileMode() == QFileDialogOptions::Directory || options()->fileMode() == QFileDialogOptions::DirectoryOnly))
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

    if (d->nativeFileDialog && (options()->fileMode() == QFileDialogOptions::Directory || options()->fileMode() == QFileDialogOptions::DirectoryOnly))
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
    // TODO
    return QString();
}

void QXdgDesktopPortalFileDialog::exec()
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (d->nativeFileDialog && (options()->fileMode() == QFileDialogOptions::Directory || options()->fileMode() == QFileDialogOptions::DirectoryOnly)) {
        d->nativeFileDialog->exec();
        return;
    }

    // HACK we have to avoid returning until we emit that the dialog was accepted or rejected
    QEventLoop loop;
    loop.connect(this, SIGNAL(accept()), SLOT(quit()));
    loop.connect(this, SIGNAL(reject()), SLOT(quit()));
    loop.exec();
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

    d->modal = windowModality != Qt::NonModal;
    d->winId = parent ? parent->winId() : 0;

    if (d->nativeFileDialog && (options()->fileMode() == QFileDialogOptions::Directory || options()->fileMode() == QFileDialogOptions::DirectoryOnly))
        return d->nativeFileDialog->show(windowFlags, windowModality, parent);

    openPortal();

    return true;
}

void QXdgDesktopPortalFileDialog::gotResponse(uint response, const QVariantMap &results)
{
    Q_D(QXdgDesktopPortalFileDialog);

    if (!response) {
        if (results.contains(QLatin1String("uris")))
            d->selectedFiles = results.value(QLatin1String("uris")).toStringList();

        Q_EMIT accept();
    } else {
        Q_EMIT reject();
    }
}

QT_END_NAMESPACE
