/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/private/qsignaldumper_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>

#include <QtTest/private/qtestlog_p.h>

QT_BEGIN_NAMESPACE

namespace QTest
{

inline static void qPrintMessage(const QByteArray &ba)
{
    QTestLog::info(ba.constData(), 0, 0);
}

Q_GLOBAL_STATIC(QList<QByteArray>, ignoreClasses)
static int iLevel = 0;
static int ignoreLevel = 0;
enum { IndentSpacesCount = 4 };

static QByteArray memberName(const QMetaMethod &member)
{
    QByteArray ba = member.methodSignature();
    return ba.left(ba.indexOf('('));
}

static void qSignalDumperCallback(QObject *caller, int method_index, void **argv)
{
    Q_ASSERT(caller); Q_ASSERT(argv); Q_UNUSED(argv);
    const QMetaObject *mo = caller->metaObject();
    Q_ASSERT(mo);
    QMetaMethod member = mo->method(method_index);
    Q_ASSERT(member.isValid());

    if (QTest::ignoreClasses() && QTest::ignoreClasses()->contains(mo->className())) {
        ++QTest::ignoreLevel;
        return;
    }

    QByteArray str;
    str.fill(' ', QTest::iLevel++ * QTest::IndentSpacesCount);
    str += "Signal: ";
    str += mo->className();
    str += '(';

    QString objname = caller->objectName();
    str += objname.toLocal8Bit();
    if (!objname.isEmpty())
        str += ' ';
    str += QByteArray::number(quintptr(caller), 16);

    str += ") ";
    str += QTest::memberName(member);
    str += " (";

    QList<QByteArray> args = member.parameterTypes();
    for (int i = 0; i < args.count(); ++i) {
        const QByteArray &arg = args.at(i);
        int typeId = QMetaType::type(args.at(i).constData());
        if (arg.endsWith('*') || arg.endsWith('&')) {
            str += '(';
            str += arg;
            str += ')';
            if (arg.endsWith('&'))
                str += '@';

            quintptr addr = quintptr(*reinterpret_cast<void **>(argv[i + 1]));
            str.append(QByteArray::number(addr, 16));
        } else if (typeId != QMetaType::UnknownType) {
            Q_ASSERT(typeId != QMetaType::Void); // void parameter => metaobject is corrupt
            str.append(arg)
                .append('(')
                .append(QVariant(typeId, argv[i + 1]).toString().toLocal8Bit())
                .append(')');
        }
        str.append(", ");
    }
    if (str.endsWith(", "))
        str.chop(2);
    str.append(')');
    qPrintMessage(str);
}

static void qSignalDumperCallbackSlot(QObject *caller, int method_index, void **argv)
{
    Q_ASSERT(caller); Q_ASSERT(argv); Q_UNUSED(argv);
    const QMetaObject *mo = caller->metaObject();
    Q_ASSERT(mo);
    QMetaMethod member = mo->method(method_index);
    if (!member.isValid())
        return;

    if (QTest::ignoreLevel ||
            (QTest::ignoreClasses() && QTest::ignoreClasses()->contains(mo->className())))
        return;

    QByteArray str;
    str.fill(' ', QTest::iLevel * QTest::IndentSpacesCount);
    str += "Slot: ";
    str += mo->className();
    str += '(';

    QString objname = caller->objectName();
    str += objname.toLocal8Bit();
    if (!objname.isEmpty())
        str += ' ';
    str += QByteArray::number(quintptr(caller), 16);

    str += ") ";
    str += member.methodSignature();
    qPrintMessage(str);
}

static void qSignalDumperCallbackEndSignal(QObject *caller, int /*method_index*/)
{
    Q_ASSERT(caller); Q_ASSERT(caller->metaObject());
    if (QTest::ignoreClasses()
            && QTest::ignoreClasses()->contains(caller->metaObject()->className())) {
        --QTest::ignoreLevel;
        Q_ASSERT(QTest::ignoreLevel >= 0);
        return;
    }
    --QTest::iLevel;
    Q_ASSERT(QTest::iLevel >= 0);
}

}

// this struct is copied from qobject_p.h to prevent us
// from including private Qt headers.
struct QSignalSpyCallbackSet
{
    typedef void (*BeginCallback)(QObject *caller, int method_index, void **argv);
    typedef void (*EndCallback)(QObject *caller, int method_index);
    BeginCallback signal_begin_callback,
                  slot_begin_callback;
    EndCallback signal_end_callback,
                slot_end_callback;
};
extern void Q_CORE_EXPORT qt_register_signal_spy_callbacks(const QSignalSpyCallbackSet &);

void QSignalDumper::startDump()
{
    static QSignalSpyCallbackSet set = { QTest::qSignalDumperCallback,
        QTest::qSignalDumperCallbackSlot, QTest::qSignalDumperCallbackEndSignal, 0 };
    qt_register_signal_spy_callbacks(set);
}

void QSignalDumper::endDump()
{
    static QSignalSpyCallbackSet nset = { 0, 0, 0 ,0 };
    qt_register_signal_spy_callbacks(nset);
}

void QSignalDumper::ignoreClass(const QByteArray &klass)
{
    if (QTest::ignoreClasses())
        QTest::ignoreClasses()->append(klass);
}

void QSignalDumper::clearIgnoredClasses()
{
    if (QTest::ignoreClasses())
        QTest::ignoreClasses()->clear();
}

QT_END_NAMESPACE
