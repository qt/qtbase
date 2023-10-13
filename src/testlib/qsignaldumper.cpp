// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qsignaldumper_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>

#include <QtTest/private/qtestlog_p.h>

#include <QtCore/private/qmetaobject_p.h>

QT_BEGIN_NAMESPACE

namespace QTest
{

inline static void qPrintMessage(const QByteArray &ba)
{
    QTestLog::info(ba.constData(), nullptr, 0);
}

Q_GLOBAL_STATIC(QList<QByteArray>, ignoreClasses)
Q_CONSTINIT static QBasicMutex ignoreClassesMutex;
Q_CONSTINIT thread_local int iLevel = 0;
Q_CONSTINIT thread_local int ignoreLevel = 0;
enum { IndentSpacesCount = 4 };

static bool classIsIgnored(const char *className)
{
    if (Q_LIKELY(!ignoreClasses.exists()))
        return false;
    QMutexLocker locker(&ignoreClassesMutex);
    if (ignoreClasses()->isEmpty())
        return false;
    return ignoreClasses()->contains(QByteArrayView(className));
}

static void qSignalDumperCallback(QObject *caller, int signal_index, void **argv)
{
    Q_ASSERT(caller);
    Q_ASSERT(argv);
    Q_UNUSED(argv);
    const QMetaObject *mo = caller->metaObject();
    Q_ASSERT(mo);
    QMetaMethod member = QMetaObjectPrivate::signal(mo, signal_index);
    Q_ASSERT(member.isValid());

    if (classIsIgnored(mo->className())) {
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
    str += QByteArray::number(quintptr(caller), 16).rightJustified(8, '0');

    str += ") ";
    str += member.name();
    str += " (";

    QList<QByteArray> args = member.parameterTypes();
    for (int i = 0; i < args.size(); ++i) {
        const QByteArray &arg = args.at(i);
        int typeId = QMetaType::fromName(args.at(i).constData()).id();
        if (arg.endsWith('*') || arg.endsWith('&')) {
            str += '(';
            str += arg;
            str += ')';
            if (arg.endsWith('&'))
                str += '@';

            quintptr addr = quintptr(*reinterpret_cast<void **>(argv[i + 1]));
            str.append(QByteArray::number(addr, 16).rightJustified(8, '0'));
        } else if (typeId != QMetaType::UnknownType) {
            Q_ASSERT(typeId != QMetaType::Void); // void parameter => metaobject is corrupt
            str.append(arg)
                .append('(')
                .append(QVariant(QMetaType(typeId), argv[i + 1]).toString().toLocal8Bit())
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
    Q_ASSERT(caller);
    Q_ASSERT(argv);
    Q_UNUSED(argv);
    const QMetaObject *mo = caller->metaObject();
    Q_ASSERT(mo);
    QMetaMethod member = mo->method(method_index);
    if (!member.isValid())
        return;

    if (QTest::ignoreLevel || classIsIgnored(mo->className()))
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
    str += QByteArray::number(quintptr(caller), 16).rightJustified(8, '0');

    str += ") ";
    str += member.methodSignature();
    qPrintMessage(str);
}

static void qSignalDumperCallbackEndSignal(QObject *caller, int /*signal_index*/)
{
    Q_ASSERT(caller); Q_ASSERT(caller->metaObject());
    if (classIsIgnored(caller->metaObject()->className())) {
        --QTest::ignoreLevel;
        Q_ASSERT(QTest::ignoreLevel >= 0);
        return;
    }
    --QTest::iLevel;
    Q_ASSERT(QTest::iLevel >= 0);
}

}

void QSignalDumper::setEnabled(bool enabled)
{
    s_isEnabled = enabled;
}

void QSignalDumper::startDump()
{
    if (!s_isEnabled)
        return;

    static QSignalSpyCallbackSet set = { QTest::qSignalDumperCallback,
        QTest::qSignalDumperCallbackSlot, QTest::qSignalDumperCallbackEndSignal, nullptr };
    qt_register_signal_spy_callbacks(&set);
}

void QSignalDumper::endDump()
{
    qt_register_signal_spy_callbacks(nullptr);
}

void QSignalDumper::ignoreClass(const QByteArray &klass)
{
    QMutexLocker locker(&QTest::ignoreClassesMutex);
    if (QTest::ignoreClasses())
        QTest::ignoreClasses()->append(klass);
}

void QSignalDumper::clearIgnoredClasses()
{
    QMutexLocker locker(&QTest::ignoreClassesMutex);
    if (QTest::ignoreClasses.exists())
        QTest::ignoreClasses()->clear();
}

bool QSignalDumper::s_isEnabled = false;

QT_END_NAMESPACE
