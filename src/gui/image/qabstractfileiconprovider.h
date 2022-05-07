/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QABSTRACTFILEICONPROVIDER_H
#define QABSTRACTFILEICONPROVIDER_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qscopedpointer.h>
#include <QtGui/qicon.h>

QT_BEGIN_NAMESPACE

class QAbstractFileIconProviderPrivate;

class Q_GUI_EXPORT QAbstractFileIconProvider
{
public:
    enum IconType { Computer, Desktop, Trashcan, Network, Drive, Folder, File };
    enum Option {
        DontUseCustomDirectoryIcons = 0x00000001
    };
    Q_DECLARE_FLAGS(Options, Option)

    QAbstractFileIconProvider();
    virtual ~QAbstractFileIconProvider();

    virtual QIcon icon(IconType) const;
    virtual QIcon icon(const QFileInfo &) const;
    virtual QString type(const QFileInfo &) const;

    virtual void setOptions(Options);
    virtual Options options() const;

protected:
    QAbstractFileIconProvider(QAbstractFileIconProviderPrivate &dd);
    QScopedPointer<QAbstractFileIconProviderPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(QAbstractFileIconProvider)
    Q_DISABLE_COPY(QAbstractFileIconProvider)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstractFileIconProvider::Options)

QT_END_NAMESPACE

#endif // QABSTRACTFILEICONPROVIDER_H
