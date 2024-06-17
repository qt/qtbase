// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractfileiconprovider.h"

#include <qguiapplication.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>
#include <qicon.h>
#if QT_CONFIG(mimetype)
#include <qmimedatabase.h>
#endif


#include <private/qabstractfileiconprovider_p.h>
#include <private/qfilesystementry_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QAbstractFileIconProviderPrivate::QAbstractFileIconProviderPrivate(QAbstractFileIconProvider *q)
    : q_ptr(q)
{}

QAbstractFileIconProviderPrivate::~QAbstractFileIconProviderPrivate() = default;

using IconTypeCache = QHash<QAbstractFileIconProvider::IconType, QIcon>;
Q_GLOBAL_STATIC(IconTypeCache, iconTypeCache)

void QAbstractFileIconProviderPrivate::clearIconTypeCache()
{
    iconTypeCache()->clear();
}

QIcon QAbstractFileIconProviderPrivate::getPlatformThemeIcon(QAbstractFileIconProvider::IconType type) const
{
    auto theme = QGuiApplicationPrivate::platformTheme();
    if (theme == nullptr)
        return {};

    if (theme->themeHint(QPlatformTheme::PreferFileIconFromTheme).toBool()) {
        const QIcon result = getIconThemeIcon(type);
        if (!result.isNull())
            return result;
    }

    auto &cache = *iconTypeCache();
    auto it = cache.find(type);
    if (it == cache.end()) {
        const auto sp = [&]() -> QPlatformTheme::StandardPixmap {
            switch (type) {
            case QAbstractFileIconProvider::Computer:
                return QPlatformTheme::ComputerIcon;
            case QAbstractFileIconProvider::Desktop:
                return QPlatformTheme::DesktopIcon;
            case QAbstractFileIconProvider::Trashcan:
                return QPlatformTheme::TrashIcon;
            case QAbstractFileIconProvider::Network:
                return QPlatformTheme::DriveNetIcon;
            case QAbstractFileIconProvider::Drive:
                return QPlatformTheme::DriveHDIcon;
            case QAbstractFileIconProvider::Folder:
                return QPlatformTheme::DirIcon;
            case QAbstractFileIconProvider::File:
                break;
            // no default on purpose; we want warnings when the type enum is extended
            }
            return QPlatformTheme::FileIcon;
        }();

        const auto sizesHint = theme->themeHint(QPlatformTheme::IconPixmapSizes);
        auto sizes = sizesHint.value<QList<QSize>>();
        if (sizes.isEmpty())
            sizes.append({64, 64});

        QIcon icon;
        for (const auto &size : sizes)
            icon.addPixmap(theme->standardPixmap(sp, size));
        it = cache.insert(type, icon);
    }
    return it.value();
}

QIcon QAbstractFileIconProviderPrivate::getIconThemeIcon(QAbstractFileIconProvider::IconType type) const
{
    switch (type) {
    case QAbstractFileIconProvider::Computer:
        return QIcon::fromTheme("computer"_L1);
    case QAbstractFileIconProvider::Desktop:
        return QIcon::fromTheme("user-desktop"_L1);
    case QAbstractFileIconProvider::Trashcan:
        return QIcon::fromTheme("user-trash"_L1);
    case QAbstractFileIconProvider::Network:
        return QIcon::fromTheme("network-workgroup"_L1);
    case QAbstractFileIconProvider::Drive:
        return QIcon::fromTheme("drive-harddisk"_L1);
    case QAbstractFileIconProvider::Folder:
        return QIcon::fromTheme("folder"_L1);
    case QAbstractFileIconProvider::File:
        return QIcon::fromTheme("text-x-generic"_L1);
        // no default on purpose; we want warnings when the type enum is extended
    }
    return QIcon::fromTheme("text-x-generic"_L1);
}

static inline QPlatformTheme::IconOptions toThemeIconOptions(QAbstractFileIconProvider::Options options)
{
    QPlatformTheme::IconOptions result;
    if (options.testFlag(QAbstractFileIconProvider::DontUseCustomDirectoryIcons))
        result |= QPlatformTheme::DontUseCustomDirectoryIcons;
    return result;
}

QIcon QAbstractFileIconProviderPrivate::getPlatformThemeIcon(const QFileInfo &info) const
{
    if (auto theme = QGuiApplicationPrivate::platformTheme()) {
        if (theme->themeHint(QPlatformTheme::PreferFileIconFromTheme).toBool()) {
            const QIcon result = getIconThemeIcon(info);
            if (!result.isNull())
                return result;
        }
        return theme->fileIcon(info, toThemeIconOptions(options));
    }
    return {};
}

QIcon QAbstractFileIconProviderPrivate::getIconThemeIcon(const QFileInfo &info) const
{
    if (info.isRoot())
        return getIconThemeIcon(QAbstractFileIconProvider::Drive);
    if (info.isDir())
        return getIconThemeIcon(QAbstractFileIconProvider::Folder);
#if QT_CONFIG(mimetype)
    return QIcon::fromTheme(mimeDatabase.mimeTypeForFile(info).iconName());
#else
    return QIcon::fromTheme("text-x-generic"_L1);
#endif
}

/*!
  \class QAbstractFileIconProvider

  \inmodule QtGui
  \since 6.0

  \brief The QAbstractFileIconProvider class provides file icons for the QFileSystemModel class.
*/

/*!
  \enum QAbstractFileIconProvider::IconType

  \value Computer   The icon used for the computing device as a whole
  \value Desktop    The icon for the special "Desktop" directory of the user
  \value Trashcan   The icon for the user's "Trash" place in the desktop's file manager
  \value Network    The icon for the “Network Servers” place in the desktop's file manager,
                    and workgroups within the network
  \value Drive      The icon used for disk drives
  \value Folder     The standard folder icon used to represent directories on local filesystems
  \value File       The icon used for generic text file types
*/

/*!
    \enum QAbstractFileIconProvider::Option

    \value DontUseCustomDirectoryIcons Always use the default directory icon.
    Some platforms allow the user to set a different icon. Custom icon lookup
    cause a big performance impact over network or removable drives.
*/

/*!
  Constructs a file icon provider.
*/
QAbstractFileIconProvider::QAbstractFileIconProvider()
    : d_ptr(new QAbstractFileIconProviderPrivate(this))
{
}

/*!
  \internal
*/
QAbstractFileIconProvider::QAbstractFileIconProvider(QAbstractFileIconProviderPrivate &dd)
    : d_ptr(&dd)
{}

/*!
  Destroys the file icon provider.
*/

QAbstractFileIconProvider::~QAbstractFileIconProvider() = default;


/*!
    Sets \a options that affect the icon provider.
    \sa options()
*/

void QAbstractFileIconProvider::setOptions(QAbstractFileIconProvider::Options options)
{
    Q_D(QAbstractFileIconProvider);
    d->options = options;
}

/*!
    Returns all the options that affect the icon provider.
    By default, all options are disabled.
    \sa setOptions()
*/

QAbstractFileIconProvider::Options QAbstractFileIconProvider::options() const
{
    Q_D(const QAbstractFileIconProvider);
    return d->options;
}

/*!
  Returns an icon set for the given \a type, using the current
  icon theme.

  \sa QIcon::fromTheme
*/

QIcon QAbstractFileIconProvider::icon(IconType type) const
{
    Q_D(const QAbstractFileIconProvider);
    return d->getPlatformThemeIcon(type);
}

/*!
  Returns an icon for the file described by \a info, using the
  current icon theme.

  \sa QIcon::fromTheme
*/

QIcon QAbstractFileIconProvider::icon(const QFileInfo &info) const
{
    Q_D(const QAbstractFileIconProvider);
    return d->getPlatformThemeIcon(info);
}


QString QAbstractFileIconProviderPrivate::getFileType(const QFileInfo &info)
{
    if (QFileSystemEntry::isRootPath(info.absoluteFilePath()))
        return QGuiApplication::translate("QAbstractFileIconProvider", "Drive");
    if (info.isFile()) {
#if QT_CONFIG(mimetype)
        const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(info);
        return mimeType.comment().isEmpty() ? mimeType.name() : mimeType.comment();
#else
        return QGuiApplication::translate("QAbstractFileIconProvider", "File");
#endif
    }

    if (info.isDir())
#ifdef Q_OS_WIN
        return QGuiApplication::translate("QAbstractFileIconProvider", "File Folder", "Match Windows Explorer");
#else
        return QGuiApplication::translate("QAbstractFileIconProvider", "Folder", "All other platforms");
#endif
    // Windows   - "File Folder"
    // macOS     - "Folder"
    // Konqueror - "Folder"
    // Nautilus  - "folder"

    if (info.isSymLink())
#ifdef Q_OS_MACOS
        return QGuiApplication::translate("QAbstractFileIconProvider", "Alias", "macOS Finder");
#else
        return QGuiApplication::translate("QAbstractFileIconProvider", "Shortcut", "All other platforms");
#endif
    // macOS     - "Alias"
    // Windows   - "Shortcut"
    // Konqueror - "Folder" or "TXT File" i.e. what it is pointing to
    // Nautilus  - "link to folder" or "link to object file", same as Konqueror

    return QGuiApplication::translate("QAbstractFileIconProvider", "Unknown");
}

/*!
  Returns the type of the file described by \a info.
*/

QString QAbstractFileIconProvider::type(const QFileInfo &info) const
{
    return QAbstractFileIconProviderPrivate::getFileType(info);
}

QT_END_NAMESPACE
