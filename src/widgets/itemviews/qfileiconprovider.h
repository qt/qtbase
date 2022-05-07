/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QFILEICONPROVIDER_H
#define QFILEICONPROVIDER_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qscopedpointer.h>
#include <QtGui/qicon.h>
#include <QtGui/qabstractfileiconprovider.h>

QT_BEGIN_NAMESPACE


class QFileIconProviderPrivate;

class Q_WIDGETS_EXPORT QFileIconProvider : public QAbstractFileIconProvider
{
public:
    QFileIconProvider();
    ~QFileIconProvider();

    QIcon icon(IconType type) const override;
    QIcon icon(const QFileInfo &info) const override;

private:
    Q_DECLARE_PRIVATE(QFileIconProvider)
    Q_DISABLE_COPY(QFileIconProvider)
};

QT_END_NAMESPACE

#endif // QFILEICONPROVIDER_H
