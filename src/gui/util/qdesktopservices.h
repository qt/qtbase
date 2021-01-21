/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QDESKTOPSERVICES_H
#define QDESKTOPSERVICES_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qstandardpaths.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_DESKTOPSERVICES

class QStringList;
class QUrl;
class QObject;

class Q_GUI_EXPORT QDesktopServices
{
public:
    static bool openUrl(const QUrl &url);
    static void setUrlHandler(const QString &scheme, QObject *receiver, const char *method);
    static void unsetUrlHandler(const QString &scheme);

#if QT_DEPRECATED_SINCE(5, 0)
    //Must match QStandardPaths::StandardLocation
    enum StandardLocation {
        DesktopLocation,
        DocumentsLocation,
        FontsLocation,
        ApplicationsLocation,
        MusicLocation,
        MoviesLocation,
        PicturesLocation,
        TempLocation,
        HomeLocation,
        DataLocation,
        CacheLocation
    };

    QT_DEPRECATED static QString storageLocation(StandardLocation type) {
        return storageLocationImpl(static_cast<QStandardPaths::StandardLocation>(type));
    }
    QT_DEPRECATED static QString displayName(StandardLocation type) {
        return QStandardPaths::displayName(static_cast<QStandardPaths::StandardLocation>(type));
    }
#endif
private:
    static QString storageLocationImpl(QStandardPaths::StandardLocation type);
};

#endif // QT_NO_DESKTOPSERVICES

QT_END_NAMESPACE

#endif // QDESKTOPSERVICES_H

