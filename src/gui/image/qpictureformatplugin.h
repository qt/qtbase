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

#ifndef QPICTUREFORMATPLUGIN_H
#define QPICTUREFORMATPLUGIN_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE


#if !defined(QT_NO_PICTURE)

class QPicture;
class QImage;
class QString;
class QStringList;

#define QPictureFormatInterface_iid "org.qt-project.Qt.QPictureFormatInterface"

class Q_GUI_EXPORT QPictureFormatPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QPictureFormatPlugin(QObject *parent = nullptr);
    ~QPictureFormatPlugin();

    virtual bool loadPicture(const QString &format, const QString &filename, QPicture *pic);
    virtual bool savePicture(const QString &format, const QString &filename, const QPicture &pic);
    virtual bool installIOHandler(const QString &format) = 0;

};

#endif // QT_NO_PICTURE

QT_END_NAMESPACE

#endif // QPICTUREFORMATPLUGIN_H
