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

#ifndef QFILEICONPROVIDER_P_H
#define QFILEICONPROVIDER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qfileiconprovider.h"

#include <QtCore/qstring.h>
#include <QtGui/qicon.h>
#include <QtWidgets/qstyle.h>

QT_BEGIN_NAMESPACE

class QFileInfo;

class QFileIconProviderPrivate
{
    Q_DECLARE_PUBLIC(QFileIconProvider)

public:
    QFileIconProviderPrivate(QFileIconProvider *q);
    QIcon getIcon(QStyle::StandardPixmap name) const;
    QIcon getIcon(const QFileInfo &fi) const;

    QFileIconProvider *q_ptr;
    const QString homePath;
    QFileIconProvider::Options options;

private:
    mutable QIcon file;
    mutable QIcon fileLink;
    mutable QIcon directory;
    mutable QIcon directoryLink;
    mutable QIcon harddisk;
    mutable QIcon floppy;
    mutable QIcon cdrom;
    mutable QIcon ram;
    mutable QIcon network;
    mutable QIcon computer;
    mutable QIcon desktop;
    mutable QIcon trashcan;
    mutable QIcon generic;
    mutable QIcon home;
};

QT_END_NAMESPACE

#endif // QFILEICONPROVIDER_P_H
