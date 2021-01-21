/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
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

#ifndef QSHADERNODESLOADER_P_H
#define QSHADERNODESLOADER_P_H

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

#include <QtGui/private/qtguiglobal_p.h>

#include <QtGui/private/qshadergraph_p.h>

QT_BEGIN_NAMESPACE

class QIODevice;

class QShaderNodesLoader
{
public:
    enum Status : char {
        Null,
        Waiting,
        Ready,
        Error
    };

    Q_GUI_EXPORT QShaderNodesLoader() noexcept;

    Q_GUI_EXPORT Status status() const noexcept;
    Q_GUI_EXPORT QHash<QString, QShaderNode> nodes() const noexcept;

    Q_GUI_EXPORT QIODevice *device() const noexcept;
    Q_GUI_EXPORT void setDevice(QIODevice *device) noexcept;

    Q_GUI_EXPORT void load();
    Q_GUI_EXPORT void load(const QJsonObject &prototypesObject);

private:
    Status m_status;
    QIODevice *m_device;
    QHash<QString, QShaderNode> m_nodes;
};

Q_DECLARE_TYPEINFO(QShaderNodesLoader, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QShaderNodesLoader)
Q_DECLARE_METATYPE(QShaderNodesLoader::Status)

#endif // QSHADERNODESLOADER_P_H
