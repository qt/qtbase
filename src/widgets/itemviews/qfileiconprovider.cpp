/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qfileiconprovider.h"
#include "qfileiconprovider_p.h"

#include <qapplication.h>
#include <qdir.h>
#include <qpixmapcache.h>
#include <private/qfunctions_p.h>
#include <private/qguiapplication_p.h>
#include <private/qicon_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>
#include <qpa/qplatformtheme.h>

#if defined(Q_OS_WIN)
#  include <qt_windows.h>
#  ifndef Q_OS_WINRT
#    include <commctrl.h>
#    include <objbase.h>
#  endif
#endif

QT_BEGIN_NAMESPACE

static bool isCacheable(const QFileInfo &fi);

class QFileIconEngine : public QPixmapIconEngine
{
public:
    QFileIconEngine(const QFileInfo &info, QFileIconProvider::Options opts)
        : QPixmapIconEngine(), m_fileInfo(info), m_fipOpts(opts)
    { }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE
    {
        Q_UNUSED(mode);
        Q_UNUSED(state);
        QPixmap pixmap;

        if (!size.isValid())
            return pixmap;

        const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
        if (!theme)
            return pixmap;

        const QString &keyBase = QLatin1String("qt_.") + m_fileInfo.suffix().toUpper();

        bool cacheable = isCacheable(m_fileInfo);
        if (cacheable) {
            QPixmapCache::find(keyBase + QString::number(size.width()), pixmap);
            if (!pixmap.isNull())
                return pixmap;
        }

        QPlatformTheme::IconOptions iconOptions;
        if (m_fipOpts & QFileIconProvider::DontUseCustomDirectoryIcons)
            iconOptions |= QPlatformTheme::DontUseCustomDirectoryIcons;

        pixmap = theme->fileIconPixmap(m_fileInfo, size, iconOptions);
        if (!pixmap.isNull()) {
            if (cacheable)
                QPixmapCache::insert(keyBase + QString::number(size.width()), pixmap);
        }

        return pixmap;
    }

    QList<QSize> availableSizes(QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off) const Q_DECL_OVERRIDE
    {
        Q_UNUSED(mode);
        Q_UNUSED(state);
        static QList<QSize> sizes;
        static QPlatformTheme *theme = 0;
        if (!theme) {
            theme = QGuiApplicationPrivate::platformTheme();
            if (!theme)
                return sizes;

            QList<int> themeSizes = theme->themeHint(QPlatformTheme::IconPixmapSizes).value<QList<int> >();
            if (themeSizes.isEmpty())
                return sizes;

            sizes.reserve(themeSizes.count());
            foreach (int size, themeSizes)
                sizes << QSize(size, size);
        }
        return sizes;
    }

    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) Q_DECL_OVERRIDE
    {
        const QList<QSize> &sizes = availableSizes(mode, state);
        const int numberSizes = sizes.length();
        if (numberSizes == 0)
            return QSize();

        // Find the smallest available size whose area is still larger than the input
        // size. Otherwise, use the largest area available size. (We don't assume the
        // platform theme sizes are sorted, hence the extra logic.)
        const int sizeArea = size.width() * size.height();
        QSize actualSize = sizes.first();
        int actualArea = actualSize.width() * actualSize.height();
        for (int i = 1; i < numberSizes; ++i) {
            const QSize &s = sizes.at(i);
            const int a = s.width() * s.height();
            if ((sizeArea <= a && a < actualArea) || (actualArea < sizeArea && actualArea < a)) {
                actualSize = s;
                actualArea = a;
            }
        }

        if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
            actualSize.scale(size, Qt::KeepAspectRatio);

        return actualSize;
    }

private:
    QFileInfo m_fileInfo;
    QFileIconProvider::Options m_fipOpts;
};


/*!
  \class QFileIconProvider

  \inmodule QtWidgets

  \brief The QFileIconProvider class provides file icons for the QDirModel and the QFileSystemModel classes.
*/

/*!
  \enum QFileIconProvider::IconType
  \value Computer
  \value Desktop
  \value Trashcan
  \value Network
  \value Drive
  \value Folder
  \value File
*/


/*!
    \enum QFileIconProvider::Option
    \since 5.2

    \value DontUseCustomDirectoryIcons Always use the default directory icon.
    Some platforms allow the user to set a different icon. Custom icon lookup
    cause a big performance impact over network or removable drives.
*/

QFileIconProviderPrivate::QFileIconProviderPrivate(QFileIconProvider *q) :
    q_ptr(q), homePath(QDir::home().absolutePath())
{
}

QIcon QFileIconProviderPrivate::getIcon(QStyle::StandardPixmap name) const
{
    switch (name) {
    case QStyle::SP_FileIcon:
        if (file.isNull())
            file = QApplication::style()->standardIcon(name);
        return file;
    case QStyle::SP_FileLinkIcon:
        if (fileLink.isNull())
            fileLink = QApplication::style()->standardIcon(name);
        return fileLink;
    case QStyle::SP_DirIcon:
        if (directory.isNull())
            directory = QApplication::style()->standardIcon(name);
        return directory;
    case QStyle::SP_DirLinkIcon:
        if (directoryLink.isNull())
            directoryLink = QApplication::style()->standardIcon(name);
        return directoryLink;
    case QStyle::SP_DriveHDIcon:
        if (harddisk.isNull())
            harddisk = QApplication::style()->standardIcon(name);
        return harddisk;
    case QStyle::SP_DriveFDIcon:
        if (floppy.isNull())
            floppy = QApplication::style()->standardIcon(name);
        return floppy;
    case QStyle::SP_DriveCDIcon:
        if (cdrom.isNull())
            cdrom = QApplication::style()->standardIcon(name);
        return cdrom;
    case QStyle::SP_DriveNetIcon:
        if (network.isNull())
            network = QApplication::style()->standardIcon(name);
        return network;
    case QStyle::SP_ComputerIcon:
        if (computer.isNull())
            computer = QApplication::style()->standardIcon(name);
        return computer;
    case QStyle::SP_DesktopIcon:
        if (desktop.isNull())
            desktop = QApplication::style()->standardIcon(name);
        return desktop;
    case QStyle::SP_TrashIcon:
        if (trashcan.isNull())
            trashcan = QApplication::style()->standardIcon(name);
        return trashcan;
    case QStyle::SP_DirHomeIcon:
        if (home.isNull())
            home = QApplication::style()->standardIcon(name);
        return home;
    default:
        return QIcon();
    }
    return QIcon();
}

/*!
  Constructs a file icon provider.
*/

QFileIconProvider::QFileIconProvider()
    : d_ptr(new QFileIconProviderPrivate(this))
{
}

/*!
  Destroys the file icon provider.

*/

QFileIconProvider::~QFileIconProvider()
{
}

/*!
    \since 5.2
    Sets \a options that affect the icon provider.
    \sa options()
*/

void QFileIconProvider::setOptions(QFileIconProvider::Options options)
{
    Q_D(QFileIconProvider);
    d->options = options;
}

/*!
    \since 5.2
    Returns all the options that affect the icon provider.
    By default, all options are disabled.
    \sa setOptions()
*/

QFileIconProvider::Options QFileIconProvider::options() const
{
    Q_D(const QFileIconProvider);
    return d->options;
}

/*!
  Returns an icon set for the given \a type.
*/

QIcon QFileIconProvider::icon(IconType type) const
{
    Q_D(const QFileIconProvider);
    switch (type) {
    case Computer:
        return d->getIcon(QStyle::SP_ComputerIcon);
    case Desktop:
        return d->getIcon(QStyle::SP_DesktopIcon);
    case Trashcan:
        return d->getIcon(QStyle::SP_TrashIcon);
    case Network:
        return d->getIcon(QStyle::SP_DriveNetIcon);
    case Drive:
        return d->getIcon(QStyle::SP_DriveHDIcon);
    case Folder:
        return d->getIcon(QStyle::SP_DirIcon);
    case File:
        return d->getIcon(QStyle::SP_FileIcon);
    default:
        break;
    };
    return QIcon();
}

static bool isCacheable(const QFileInfo &fi)
{
    if (!fi.isFile())
        return false;

#ifdef Q_OS_WIN
    // On windows it's faster to just look at the file extensions. QTBUG-13182
    const QString fileExtension = fi.suffix();
    // Will return false for .exe, .lnk and .ico extensions
    return fileExtension.compare(QLatin1String("exe"), Qt::CaseInsensitive) &&
           fileExtension.compare(QLatin1String("lnk"), Qt::CaseInsensitive) &&
           fileExtension.compare(QLatin1String("ico"), Qt::CaseInsensitive);
#else
    return !fi.isExecutable() && !fi.isSymLink();
#endif
}

QIcon QFileIconProviderPrivate::getIcon(const QFileInfo &fi) const
{
    const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
    if (!theme)
        return QIcon();

    QList<int> sizes = theme->themeHint(QPlatformTheme::IconPixmapSizes).value<QList<int> >();
    if (sizes.isEmpty())
        return QIcon();

    return QIcon(new QFileIconEngine(fi, options));
}

/*!
  Returns an icon for the file described by \a info.
*/

QIcon QFileIconProvider::icon(const QFileInfo &info) const
{
    Q_D(const QFileIconProvider);

    QIcon retIcon = d->getIcon(info);
    if (!retIcon.isNull())
        return retIcon;

    if (info.isRoot())
#if defined (Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    {
        UINT type = GetDriveType((wchar_t *)info.absoluteFilePath().utf16());

        switch (type) {
        case DRIVE_REMOVABLE:
            return d->getIcon(QStyle::SP_DriveFDIcon);
        case DRIVE_FIXED:
            return d->getIcon(QStyle::SP_DriveHDIcon);
        case DRIVE_REMOTE:
            return d->getIcon(QStyle::SP_DriveNetIcon);
        case DRIVE_CDROM:
            return d->getIcon(QStyle::SP_DriveCDIcon);
        case DRIVE_RAMDISK:
        case DRIVE_UNKNOWN:
        case DRIVE_NO_ROOT_DIR:
        default:
            return d->getIcon(QStyle::SP_DriveHDIcon);
        }
    }
#else
    return d->getIcon(QStyle::SP_DriveHDIcon);
#endif

    if (info.isFile()) {
        if (info.isSymLink())
            return d->getIcon(QStyle::SP_FileLinkIcon);
        else
            return d->getIcon(QStyle::SP_FileIcon);
    }
  if (info.isDir()) {
    if (info.isSymLink()) {
      return d->getIcon(QStyle::SP_DirLinkIcon);
    } else {
      if (info.absoluteFilePath() == d->homePath) {
        return d->getIcon(QStyle::SP_DirHomeIcon);
      } else {
        return d->getIcon(QStyle::SP_DirIcon);
      }
    }
  }
  return QIcon();
}

/*!
  Returns the type of the file described by \a info.
*/

QString QFileIconProvider::type(const QFileInfo &info) const
{
    if (info.isRoot())
        return QApplication::translate("QFileDialog", "Drive");
    if (info.isFile()) {
        if (!info.suffix().isEmpty()) {
            //: %1 is a file name suffix, for example txt
            return QApplication::translate("QFileDialog", "%1 File").arg(info.suffix());
        }
        return QApplication::translate("QFileDialog", "File");
    }

    if (info.isDir())
#ifdef Q_OS_WIN
        return QApplication::translate("QFileDialog", "File Folder", "Match Windows Explorer");
#else
        return QApplication::translate("QFileDialog", "Folder", "All other platforms");
#endif
    // Windows   - "File Folder"
    // OS X      - "Folder"
    // Konqueror - "Folder"
    // Nautilus  - "folder"

    if (info.isSymLink())
#ifdef Q_OS_MAC
        return QApplication::translate("QFileDialog", "Alias", "OS X Finder");
#else
        return QApplication::translate("QFileDialog", "Shortcut", "All other platforms");
#endif
    // OS X      - "Alias"
    // Windows   - "Shortcut"
    // Konqueror - "Folder" or "TXT File" i.e. what it is pointing to
    // Nautilus  - "link to folder" or "link to object file", same as Konqueror

    return QApplication::translate("QFileDialog", "Unknown");
}

QT_END_NAMESPACE
