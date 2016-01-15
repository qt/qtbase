/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef INTERFACE_H
#define INTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtDBus/QDBusArgument>

struct RegisteredType
{
    inline RegisteredType(const QString &str = QString()) : s(str) {}
    inline bool operator==(const RegisteredType &other) const { return s == other.s; }
    QString s;
};
Q_DECLARE_METATYPE(RegisteredType)

inline QDBusArgument &operator<<(QDBusArgument &s, const RegisteredType &data)
{
    s.beginStructure();
    s << data.s;
    s.endStructure();
    return s;
}

inline const QDBusArgument &operator>>(const QDBusArgument &s, RegisteredType &data)
{
    s.beginStructure();
    s >> data.s;
    s.endStructure();
    return s;
}

struct UnregisteredType
{
    QString s;
};
Q_DECLARE_METATYPE(UnregisteredType)

class Interface: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.QtDBus.Pinger")
    Q_PROPERTY(QString stringProp READ stringProp WRITE setStringProp SCRIPTABLE true)
    Q_PROPERTY(QDBusVariant variantProp READ variantProp WRITE setVariantProp SCRIPTABLE true)
    Q_PROPERTY(RegisteredType complexProp READ complexProp WRITE setComplexProp SCRIPTABLE true)

    friend class tst_QDBusAbstractInterface;
    friend class PingerServer;
    QString m_stringProp;
    QDBusVariant m_variantProp;
    RegisteredType m_complexProp;

public:
    Interface();

    QString stringProp() const { return m_stringProp; }
    void setStringProp(const QString &s) { m_stringProp = s; }
    QDBusVariant variantProp() const { return m_variantProp; }
    void setVariantProp(const QDBusVariant &v) { m_variantProp = v; }
    RegisteredType complexProp() const { return m_complexProp; }
    void setComplexProp(const RegisteredType &r) { m_complexProp = r; }

public slots:
    Q_SCRIPTABLE void voidMethod() {}
    Q_SCRIPTABLE int sleepMethod(int);
    Q_SCRIPTABLE QString stringMethod() { return "Hello, world"; }
    Q_SCRIPTABLE RegisteredType complexMethod(const QVariantHash &vars) { return RegisteredType(vars.value("arg1").toString()); }
    Q_SCRIPTABLE QString multiOutMethod(int &value) { value = 42; return "Hello, world"; }

signals:
    Q_SCRIPTABLE void voidSignal();
    Q_SCRIPTABLE void stringSignal(const QString &);
    Q_SCRIPTABLE void complexSignal(RegisteredType);
};

#endif // INTERFACE_H
