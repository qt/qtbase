/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QFILEICONPROVIDER_H
#define QFILEICONPROVIDER_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qscopedpointer.h>
#include <QtGui/qicon.h>

QT_BEGIN_NAMESPACE


class QFileIconProviderPrivate;

class Q_WIDGETS_EXPORT QFileIconProvider
{
public:
    QFileIconProvider();
    virtual ~QFileIconProvider();
    enum IconType { Computer, Desktop, Trashcan, Network, Drive, Folder, File };

    enum Option {
        DontUseCustomDirectoryIcons = 0x00000001
    };
    Q_DECLARE_FLAGS(Options, Option)

    virtual QIcon icon(IconType type) const;
    virtual QIcon icon(const QFileInfo &info) const;
    virtual QString type(const QFileInfo &info) const;

    void setOptions(Options options);
    Options options() const;

private:
    Q_DECLARE_PRIVATE(QFileIconProvider)
    QScopedPointer<QFileIconProviderPrivate> d_ptr;
    Q_DISABLE_COPY(QFileIconProvider)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFileIconProvider::Options)

QT_END_NAMESPACE

#endif // QFILEICONPROVIDER_H
