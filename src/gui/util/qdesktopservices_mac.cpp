/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_NO_DESKTOPSERVICES

#include <qprocess.h>
#include <qstringlist.h>
#include <qdir.h>
#include <qurl.h>
#include <private/qcore_mac_p.h>
#include <qcoreapplication.h>

#include <ApplicationServices/ApplicationServices.h>

QT_BEGIN_NAMESPACE

/*
    Translates a QDesktopServices::StandardLocation into the mac equivalent.
*/
OSType translateLocation(QDesktopServices::StandardLocation type)
{
    switch (type) {
    case QDesktopServices::DesktopLocation:
        return kDesktopFolderType; break;

    case QDesktopServices::DocumentsLocation:
        return kDocumentsFolderType; break;

    case QDesktopServices::FontsLocation:
        // There are at least two different font directories on the mac: /Library/Fonts and ~/Library/Fonts.
        // To select a specific one we have to specify a different first parameter when calling FSFindFolder.
        return kFontsFolderType; break;

    case QDesktopServices::ApplicationsLocation:
        return kApplicationsFolderType; break;

    case QDesktopServices::MusicLocation:
        return kMusicDocumentsFolderType; break;

    case QDesktopServices::MoviesLocation:
        return kMovieDocumentsFolderType; break;

    case QDesktopServices::PicturesLocation:
        return kPictureDocumentsFolderType; break;

    case QDesktopServices::TempLocation:
        return kTemporaryFolderType; break;

    case QDesktopServices::DataLocation:
        return kApplicationSupportFolderType; break;

    case QDesktopServices::CacheLocation:
        return kCachedDataFolderType; break;

    default:
        return kDesktopFolderType; break;
    }
}

static bool lsOpen(const QUrl &url)
{
    if (!url.isValid() || url.scheme().isEmpty())
        return false;

    QCFType<CFURLRef> cfUrl = CFURLCreateWithString(0, QCFString(QString::fromLatin1(url.toEncoded())), 0);
    if (cfUrl == 0)
        return false;

    const OSStatus err = LSOpenCFURLRef(cfUrl, 0);
    return (err == noErr);
}

static bool launchWebBrowser(const QUrl &url)
{
    return lsOpen(url);
}

static bool openDocument(const QUrl &file)
{
    if (!file.isValid())
        return false;

   // LSOpen does not work in this case, use QProcess open instead.
   return QProcess::startDetached(QLatin1String("open"), QStringList() << file.toLocalFile());
}

QT_END_NAMESPACE

#endif // QT_NO_DESKTOPSERVICES
