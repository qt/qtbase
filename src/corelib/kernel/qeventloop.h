// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVENTLOOP_H
#define QEVENTLOOP_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QEventLoopLocker;
class QEventLoopPrivate;

class Q_CORE_EXPORT QEventLoop : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventLoop)
    friend class QEventLoopLocker;

public:
    explicit QEventLoop(QObject *parent = nullptr);
    ~QEventLoop();

    enum ProcessEventsFlag {
        AllEvents = 0x00,
        ExcludeUserInputEvents = 0x01,
        ExcludeSocketNotifiers = 0x02,
        WaitForMoreEvents = 0x04,
        X11ExcludeTimers = 0x08,
        EventLoopExec = 0x20,
        DialogExec = 0x40,
        ApplicationExec = 0x80,
    };
    Q_DECLARE_FLAGS(ProcessEventsFlags, ProcessEventsFlag)
    Q_FLAG(ProcessEventsFlags)

    bool processEvents(ProcessEventsFlags flags = AllEvents);
    void processEvents(ProcessEventsFlags flags, int maximumTime);

    int exec(ProcessEventsFlags flags = AllEvents);
    bool isRunning() const;

    void wakeUp();

    bool event(QEvent *event) override;

public Q_SLOTS:
    void exit(int returnCode = 0);
    void quit();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QEventLoop::ProcessEventsFlags)

class QEventLoopLockerPrivate;

class QEventLoopLocker
{
public:
    Q_NODISCARD_CTOR Q_CORE_EXPORT QEventLoopLocker() noexcept;
    Q_NODISCARD_CTOR Q_CORE_EXPORT explicit QEventLoopLocker(QEventLoop *loop) noexcept;
    Q_NODISCARD_CTOR Q_CORE_EXPORT explicit QEventLoopLocker(QThread *thread) noexcept;
    Q_CORE_EXPORT ~QEventLoopLocker();

private:
    Q_DISABLE_COPY(QEventLoopLocker)
    friend class QEventLoopLockerPrivate;

    //
    // Private implementation details.
    // Do not call from public inline API!
    //
    enum class Type : quintptr {
        EventLoop,
        Thread,
        Application,
    };
    explicit QEventLoopLocker(void *ptr, Type t) noexcept;
    quintptr p;
    static constexpr quintptr TypeMask = 0x3;
    Type type() const { return Type(p & TypeMask); }
    void *pointer() const { return reinterpret_cast<void *>(p & ~TypeMask); }
    template <typename Func>
    void visit(Func func) const;
};

QT_END_NAMESPACE

#endif // QEVENTLOOP_H
