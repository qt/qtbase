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

#ifndef QSHADERNODE_P_H
#define QSHADERNODE_P_H

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

#include <QtGui/private/qshaderformat_p.h>
#include <QtGui/private/qshadernodeport_p.h>

#include <QtCore/quuid.h>

QT_BEGIN_NAMESPACE

class QShaderNode
{
public:
    enum Type : char {
        Invalid,
        Input,
        Output,
        Function
    };

    class Rule
    {
    public:
        Q_GUI_EXPORT Rule(const QByteArray &substitution = QByteArray(), const QByteArrayList &headerSnippets = QByteArrayList()) noexcept;

        QByteArray substitution;
        QByteArrayList headerSnippets;
    };

    Q_GUI_EXPORT Type type() const noexcept;

    Q_GUI_EXPORT QUuid uuid() const noexcept;
    Q_GUI_EXPORT void setUuid(const QUuid &uuid) noexcept;

    Q_GUI_EXPORT QStringList layers() const noexcept;
    Q_GUI_EXPORT void setLayers(const QStringList &layers) noexcept;

    Q_GUI_EXPORT QVector<QShaderNodePort> ports() const noexcept;
    Q_GUI_EXPORT void addPort(const QShaderNodePort &port);
    Q_GUI_EXPORT void removePort(const QShaderNodePort &port);

    Q_GUI_EXPORT QStringList parameterNames() const;
    Q_GUI_EXPORT QVariant parameter(const QString &name) const;
    Q_GUI_EXPORT void setParameter(const QString &name, const QVariant &value);
    Q_GUI_EXPORT void clearParameter(const QString &name);

    Q_GUI_EXPORT void addRule(const QShaderFormat &format, const Rule &rule);
    Q_GUI_EXPORT void removeRule(const QShaderFormat &format);

    Q_GUI_EXPORT QVector<QShaderFormat> availableFormats() const;
    Q_GUI_EXPORT Rule rule(const QShaderFormat &format) const;

private:
    QUuid m_uuid;
    QStringList m_layers;
    QVector<QShaderNodePort> m_ports;
    QHash<QString, QVariant> m_parameters;
    QVector<QPair<QShaderFormat, QShaderNode::Rule>> m_rules;
};

Q_GUI_EXPORT bool operator==(const QShaderNode::Rule &lhs, const QShaderNode::Rule &rhs) noexcept;

inline bool operator!=(const QShaderNode::Rule &lhs, const QShaderNode::Rule &rhs) noexcept
{
    return !(lhs == rhs);
}

Q_DECLARE_TYPEINFO(QShaderNode, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QShaderNode::Rule, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QShaderNode)
Q_DECLARE_METATYPE(QShaderNode::Rule)

#endif // QSHADERNODE_P_H
