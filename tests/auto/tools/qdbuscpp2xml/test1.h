/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDBUSCPP2XML_TEST1_H
#define QDBUSCPP2XML_TEST1_H

#include <QObject>

class QDBusObjectPath;
class QDBusUnixFileDescriptor;
class QDBusSignature;

class Test1 : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtProject.qdbuscpp2xmlTests.Test1")
    Q_PROPERTY(int numProperty1 READ numProperty1 CONSTANT)
    Q_PROPERTY(int numProperty2 READ numProperty2 WRITE setNumProperty2)
    Q_ENUMS(Salaries)
public:
    // C++1y allows use of single quote as a digit separator, useful for large
    // numbers. http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3781.pdf
    // Ensure that qdbuscpp2xml does not get confused with this appearing.
    enum Salaries {
        Steve
#ifdef Q_MOC_RUN
        = 1'234'567
#endif
    };

    Test1(QObject *parent = 0) : QObject(parent) {}

    int numProperty1() { return 42; }

    int numProperty2() { return 42; }
    void setNumProperty2(int) {}

signals:
    void signalVoidType();
    int signalIntType();
    void signal_primitive_args(int a1, bool a2, short a3, ushort a4, uint a5, qlonglong a6, double a7, qlonglong a8 = 0);
    void signal_string_args(const QByteArray &ba, const QString &a2);
    void signal_Qt_args1(const QDate &a1, const QTime &a2, const QDateTime &a3,
        const QRect &a4, const QRectF &a5, const QSize &a6, const QSizeF &a7);
    void signal_Qt_args2(const QPoint &a1, const QPointF &a2, const QLine &a3, const QLineF &a4,
        const QVariantList &a5, const QVariantMap &a6, const QVariantHash &a7);

    void signal_QDBus_args(const QDBusObjectPath &a1, const QDBusSignature &a2, const QDBusUnixFileDescriptor &a3);

    void signal_container_args1(const QList<bool> &a1, const QList<short> &a2, const QList<ushort> &a3, const QList<int> &a4, const QList<uint> &a5);
    void signal_container_args2(const QList<qlonglong> &a1, const QList<qulonglong> &a2, const QList<double> &a3, const QList<QDBusObjectPath> &a4, const QList<QDBusSignature> &a5, const QList<QDBusUnixFileDescriptor> &a6);

    Q_SCRIPTABLE void signalVoidType_scriptable();
    Q_SCRIPTABLE int signalIntType_scriptable();
    Q_SCRIPTABLE void signal_primitive_args_scriptable(int a1, bool a2, short a3, ushort a4, uint a5, qlonglong a6, double a7, qlonglong a8 = 0);
    Q_SCRIPTABLE void signal_string_args_scriptable(const QByteArray &ba, const QString &a2);
    Q_SCRIPTABLE void signal_Qt_args1_scriptable(const QDate &a1, const QTime &a2, const QDateTime &a3,
        const QRect &a4, const QRectF &a5, const QSize &a6, const QSizeF &a7);
    Q_SCRIPTABLE void signal_Qt_args2_scriptable(const QPoint &a1, const QPointF &a2, const QLine &a3, const QLineF &a4,
        const QVariantList &a5, const QVariantMap &a6, const QVariantHash &a7);

    Q_SCRIPTABLE void signal_QDBus_args_scriptable(const QDBusObjectPath &a1, const QDBusSignature &a2, const QDBusUnixFileDescriptor &a3);

    Q_SCRIPTABLE void signal_container_args1_scriptable(const QList<bool> &a1, const QList<short> &a2, const QList<ushort> &a3, const QList<int> &a4, const QList<uint> &a5);
    Q_SCRIPTABLE void signal_container_args2_scriptable(const QList<qlonglong> &a1, const QList<qulonglong> &a2, const QList<double> &a3, const QList<QDBusObjectPath> &a4, const QList<QDBusSignature> &a5, const QList<QDBusUnixFileDescriptor> &a6);

public slots:
    void slotVoidType() {}
    int slotIntType() { return 42; }

    Q_SCRIPTABLE void slotVoidType_scriptable() {}
    Q_SCRIPTABLE int slotIntType_scriptable() { return 42; }

protected slots:
    void neverExported1() {}
    int neverExported2() { return 42; }

    Q_SCRIPTABLE void neverExported3() {}
    Q_SCRIPTABLE int neverExported4() { return 42; }

private slots:
    void neverExported5() {}
    int neverExported6() { return 42; }

    Q_SCRIPTABLE void neverExported7() {}
    Q_SCRIPTABLE int neverExported8() { return 42; }

public:
    Q_SCRIPTABLE void methodVoidType() {}
    Q_SCRIPTABLE int methodIntType() { return 42; }

protected:
    Q_SCRIPTABLE void neverExported9() {}
    Q_SCRIPTABLE int neverExported10() { return 42; }

private:
    Q_SCRIPTABLE void neverExported11() {}
    Q_SCRIPTABLE int neverExported12() { return 42; }
};

#endif
