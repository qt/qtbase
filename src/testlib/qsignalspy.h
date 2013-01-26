/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSIGNALSPY_H
#define QSIGNALSPY_H

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qvector.h>
#include <QtTest/qtesteventloop.h>

QT_BEGIN_NAMESPACE


class QVariant;

class QSignalSpy: public QObject, public QList<QList<QVariant> >
{
public:
    explicit QSignalSpy(const QObject *obj, const char *aSignal)
        : m_waiting(false)
    {
#ifdef Q_CC_BOR
        const int memberOffset = QObject::staticMetaObject.methodCount();
#else
        static const int memberOffset = QObject::staticMetaObject.methodCount();
#endif
        if (!obj) {
            qWarning("QSignalSpy: Cannot spy on a null object");
            return;
        }

        if (!aSignal) {
            qWarning("QSignalSpy: Null signal name is not valid");
            return;
        }

        if (((aSignal[0] - '0') & 0x03) != QSIGNAL_CODE) {
            qWarning("QSignalSpy: Not a valid signal, use the SIGNAL macro");
            return;
        }

        const QByteArray ba = QMetaObject::normalizedSignature(aSignal + 1);
        const QMetaObject * const mo = obj->metaObject();
        const int sigIndex = mo->indexOfMethod(ba.constData());
        if (sigIndex < 0) {
            qWarning("QSignalSpy: No such signal: '%s'", ba.constData());
            return;
        }

        if (!QMetaObject::connect(obj, sigIndex, this, memberOffset,
                    Qt::DirectConnection, 0)) {
            qWarning("QSignalSpy: QMetaObject::connect returned false. Unable to connect.");
            return;
        }
        sig = ba;
        initArgs(mo->method(sigIndex), obj);
    }

    inline bool isValid() const { return !sig.isEmpty(); }
    inline QByteArray signal() const { return sig; }

    bool wait(int timeout = 5000)
    {
        Q_ASSERT(!m_waiting);
        const int origCount = count();
        m_waiting = true;
        m_loop.enterLoopMSecs(timeout);
        m_waiting = false;
        return count() > origCount;
    }

    int qt_metacall(QMetaObject::Call call, int methodId, void **a) Q_DECL_OVERRIDE
    {
        methodId = QObject::qt_metacall(call, methodId, a);
        if (methodId < 0)
            return methodId;

        if (call == QMetaObject::InvokeMetaMethod) {
            if (methodId == 0) {
                appendArgs(a);
            }
            --methodId;
        }
        return methodId;
    }

private:
    void initArgs(const QMetaMethod &member)
    {
        initArgs(member, 0);
    }

    void initArgs(const QMetaMethod &member, const QObject *obj)
    {
        const QList<QByteArray> params = member.parameterTypes();
        args.reserve(params.size());
        for (int i = 0; i < params.count(); ++i) {
            int tp = QMetaType::type(params.at(i).constData());
            if (tp == QMetaType::UnknownType && obj) {
                void *argv[] = { &tp, &i };
                QMetaObject::metacall(const_cast<QObject*>(obj),
                                      QMetaObject::RegisterMethodArgumentMetaType,
                                      member.methodIndex(), argv);
                if (tp == -1)
                    tp = QMetaType::UnknownType;
            }
            if (tp == QMetaType::UnknownType) {
                Q_ASSERT(tp != QMetaType::Void); // void parameter => metaobject is corrupt
                qWarning("Don't know how to handle '%s', use qRegisterMetaType to register it.",
                         params.at(i).constData());
            }
            args << tp;
        }
    }

    void appendArgs(void **a)
    {
        QList<QVariant> list;
        list.reserve(args.count());
        for (int i = 0; i < args.count(); ++i) {
            const QMetaType::Type type = static_cast<QMetaType::Type>(args.at(i));
            if (type == QMetaType::QVariant)
                list << *reinterpret_cast<QVariant *>(a[i + 1]);
            else
                list << QVariant(type, a[i + 1]);
        }
        append(list);

        if (m_waiting)
            m_loop.exitLoop();
    }

    // the full, normalized signal name
    QByteArray sig;
    // holds the QMetaType types for the argument list of the signal
    QVector<int> args;

    QTestEventLoop m_loop;
    bool m_waiting;
};

QT_END_NAMESPACE

#endif
