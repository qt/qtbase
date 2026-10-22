// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVENTSPY_H
#define QEVENTSPY_H

#include <QtTest/qttestglobal.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qlist.h>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

class QObject;
class QEventSpyPrivate;

class QEventSpy
{
public:
    using Filter = std::function<bool(QObject *receiver, const QEvent *event)>;

    Q_TESTLIB_EXPORT explicit QEventSpy(QObject *object);
    Q_TESTLIB_EXPORT QEventSpy(QObject *object, QEvent::Type type);
    Q_TESTLIB_EXPORT QEventSpy(QObject *object, const QList<QEvent::Type> &types);
    Q_TESTLIB_EXPORT QEventSpy(QObject *object, Filter filter);
    Q_TESTLIB_EXPORT ~QEventSpy();

    Q_TESTLIB_EXPORT bool isValid() const noexcept;
    Q_TESTLIB_EXPORT QObject *object() const;

    Q_TESTLIB_EXPORT qsizetype count() const noexcept;
    Q_TESTLIB_EXPORT qsizetype count(QEvent::Type type) const noexcept;
    qsizetype size() const noexcept { return count(); }
    bool isEmpty() const noexcept { return count() == 0; }
    bool contains(QEvent::Type type) const noexcept { return count(type) > 0; }

    Q_TESTLIB_EXPORT QList<QEvent::Type> eventList() const;
    Q_TESTLIB_EXPORT const QEvent *at(qsizetype i) const;
    template <typename EventType>
    const EventType *at(qsizetype i) const
    { return static_cast<const EventType *>(at(i)); }
    Q_TESTLIB_EXPORT QObject *receiver(qsizetype i) const;

    Q_TESTLIB_EXPORT void clear();

private:
    Q_DISABLE_COPY_MOVE(QEventSpy)

    std::unique_ptr<QEventSpyPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QEVENTSPY_H
