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

#ifndef QFSCOMPLETOR_P_H
#define QFSCOMPLETOR_P_H

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
#include "qcompleter.h"
#include <QtWidgets/qfilesystemmodel.h>

QT_REQUIRE_CONFIG(fscompleter);

QT_BEGIN_NAMESPACE

/*!
    QCompleter that can deal with QFileSystemModel
  */
class Q_WIDGETS_EXPORT QFSCompleter :  public QCompleter {
public:
    explicit QFSCompleter(QFileSystemModel *model, QObject *parent = nullptr)
        : QCompleter(model, parent), proxyModel(nullptr), sourceModel(model)
    {
#if defined(Q_OS_WIN)
        setCaseSensitivity(Qt::CaseInsensitive);
#endif
    }
    QString pathFromIndex(const QModelIndex &index) const override;
    QStringList splitPath(const QString& path) const override;

    QAbstractProxyModel *proxyModel;
    QFileSystemModel *sourceModel;
};

QT_END_NAMESPACE

#endif // QFSCOMPLETOR_P_H

