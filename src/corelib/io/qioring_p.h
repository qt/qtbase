// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef IOPROCESSOR_P_H
#define IOPROCESSOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qtcoreglobal_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qspan.h>
#include <QtCore/qhash.h>
#include <QtCore/qfiledevice.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qdeadlinetimer.h>

#ifdef Q_OS_LINUX
#  include <QtCore/qsocketnotifier.h>
struct io_uring_sqe;
struct io_uring_cqe;
#elif defined(Q_OS_WIN)
#  include <QtCore/qwineventnotifier.h>
#  include <qt_windows.h>
#  include <ioringapi.h>
#endif

#include <algorithm>
#include <filesystem>
#include <QtCore/qxpfunctional.h>
#include <variant>
#include <optional>
#include <type_traits>

/*
    This file defines an interface for the backend of QRandomAccessFile.
    The backends themselves are implemented in platform-specific files, such as
    ioring_linux.cpp, ioring_win.cpp, etc.
    And has a lower-level interface than the public interface will have, but the
    separation hopefully makes it easier to implement the ioring backends, test
    them, and tweak them without the higher-level interface needing to see
    changes, and to make it possible to tweak the higher-level interface without
    needing to touch the (somewhat similar) ioring backends.

    Most of the interface is just an enum QIORing::Operation + the
    QIORingRequest template class, which is specialized for each operation so it
    carries just the relevant data for that operation. And a small mechanism to
    store the request in a generic manner so they can be used in the
    implementation files at the cost of some overhead.

    There will be absolutely zero binary compatibility guarantees for this
    interface.
*/

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQIORing);

namespace QtPrivate {
Q_NAMESPACE

#define FOREACH_IO_OPERATION(OP) \
    OP(Open)                     \
    OP(Close)                    \
    OP(Read)                     \
    OP(Write)                    \
    OP(VectoredRead)             \
    OP(VectoredWrite)            \
    OP(Flush)                    \
    OP(Stat)                     \
    OP(Cancel)                   \
    /**/
#define DEFINE_ENTRY(OP) OP,

// clang-format off
enum class Operation : quint8 {
    FOREACH_IO_OPERATION(DEFINE_ENTRY)

    NumOperations,
};
// clang-format on
Q_ENUM_NS(Operation);
#undef DEFINE_ENTRY

#ifdef Q_OS_WIN
struct IORingApiTable;
#endif
}; // namespace QtPrivate

template <QtPrivate::Operation Op>
struct QIORingRequest;

class QIORing final
{
    class GenericRequestType;
    struct RequestHandleTag; // Just used as an opaque pointer
public:
    static constexpr quint32 DefaultSubmissionQueueSize = 128;
    static constexpr quint32 DefaultCompletionQueueSize = DefaultSubmissionQueueSize * 2;
    using Operation = QtPrivate::Operation;
    using RequestHandle = RequestHandleTag *;

    Q_CORE_EXPORT
    explicit QIORing(quint32 submissionQueueSize = DefaultSubmissionQueueSize,
                     quint32 completionQueueSize = DefaultCompletionQueueSize);
    Q_CORE_EXPORT
    ~QIORing();
    Q_DISABLE_COPY_MOVE(QIORing)

    Q_CORE_EXPORT
    static QIORing *sharedInstance();
    bool ensureInitialized() { return initializeIORing(); }

    Q_CORE_EXPORT
    static bool supportsOperation(Operation op);
    template <Operation Op>
    QIORing::RequestHandle queueRequest(QIORingRequest<Op> &&request)
    {
        Q_ASSERT(supportsOperation(Op));
        auto &r = pendingRequests.emplace_back(std::move(request));
        addrItMap.emplace(&r, std::prev(pendingRequests.end()));
        if (queueRequestInternal(r) == QueuedRequestStatus::CompletedImmediately)
            return nullptr; // Return an invalid handle, to avoid ABA with following requests
        return reinterpret_cast<RequestHandle>(&r);
    }
    Q_CORE_EXPORT
    void submitRequests();
    Q_CORE_EXPORT
    bool waitForRequest(RequestHandle handle, QDeadlineTimer deadline = QDeadlineTimer::Forever);

    quint32 submissionQueueSize() const noexcept { return sqEntries; }
    quint32 completionQueueSize() const noexcept { return cqEntries; }

private:
    std::list<GenericRequestType> pendingRequests;
    using PendingRequestsIterator = decltype(pendingRequests.begin());
    QHash<void *, PendingRequestsIterator> addrItMap;
    std::optional<PendingRequestsIterator> lastUnqueuedIterator;
    quint32 sqEntries = 0;
    quint32 cqEntries = 0;
    quint32 inFlightRequests = 0;
    quint32 unstagedRequests = 0;
    bool stagePending = false;
    bool preparingRequests = false;
    qsizetype ongoingSplitOperations = 0;

    Q_CORE_EXPORT
    bool initializeIORing();

    enum class QueuedRequestStatus : bool {
        Pending = false,
        CompletedImmediately = true,
    };
    Q_CORE_EXPORT
    QueuedRequestStatus queueRequestInternal(GenericRequestType &request);
    void prepareRequests();
    void completionReady();
    bool waitForCompletions(QDeadlineTimer deadline);

    template <typename Fun>
    static auto invokeOnOp(GenericRequestType &req, Fun fn);

    template <Operation Op>
    static void setFileErrorResult(QIORingRequest<Op> &req, QFileDevice::FileError error)
    {
        req.result.template emplace<QFileDevice::FileError>(error);
    }
    static void setFileErrorResult(GenericRequestType &req, QFileDevice::FileError error);
    static void finishRequestWithError(GenericRequestType &req, QFileDevice::FileError error);
    static bool verifyFd(GenericRequestType &req);

    enum RequestPrepResult : quint8 {
        Ok,
        QueueFull,
        Defer,
        RequestCompleted,
    };
    enum class ReadWriteStatus : bool {
        MoreToDo,
        Finished,
    };
#ifdef Q_OS_LINUX
    std::optional<QSocketNotifier> notifier;
    // io_uring 'sq', 'sqe', 'cq', and 'cqe' pointers:
    void *submissionQueue = nullptr;
    io_uring_sqe *submissionQueueEntries = nullptr;
    const io_uring_cqe *completionQueueEntries = nullptr;

    // Some pointers for working with the ring-buffer.
    // The pointers to const are controlled by the kernel.
    const quint32 *sqHead = nullptr;
    quint32 *sqTail = nullptr;
    const quint32 *sqIndexMask = nullptr;
    quint32 *sqIndexArray = nullptr;
    quint32 *cqHead = nullptr;
    const quint32 *cqTail = nullptr;
    const quint32 *cqIndexMask = nullptr;
    // Because we want the flush to act as a barrier operation we need to track
    // if there is one currently in progress. With kernel 6.16+ this seems to be
    // fixed, but since we support older kernels we implement this deferring
    // ourselves.
    bool flushInProgress = false;

    int io_uringFd = -1;
    int eventDescriptor = -1;
    [[nodiscard]]
    RequestPrepResult prepareRequest(io_uring_sqe *sqe, GenericRequestType &request);
    template <Operation Op>
    ReadWriteStatus handleReadCompletion(const io_uring_cqe *cqe, GenericRequestType *request);
    template <Operation Op>
    ReadWriteStatus handleWriteCompletion(const io_uring_cqe *cqe, GenericRequestType *request);
#elif defined(Q_OS_WIN)
    // We use UINT32 because that's the type used for size parameters in their API.
    static constexpr qsizetype MaxReadWriteLen = std::numeric_limits<UINT32>::max();
    std::optional<QWinEventNotifier> notifier;
    HIORING ioRingHandle = nullptr;
    HANDLE eventHandle = INVALID_HANDLE_VALUE;
    const QtPrivate::IORingApiTable *apiTable;

    bool initialized = false;
    bool queueWasFull = false;

    [[nodiscard]]
    RequestPrepResult prepareRequest(GenericRequestType &request);
    QIORing::ReadWriteStatus handleReadCompletion(
            HRESULT result, quintptr information, QSpan<std::byte> *destinations, void *voidExtra,
            qxp::function_ref<qint64(std::variant<QFileDevice::FileError, qint64>)> setResult);
    template <Operation Op>
    ReadWriteStatus handleReadCompletion(const IORING_CQE *cqe, GenericRequestType *request);
    ReadWriteStatus handleWriteCompletion(
            HRESULT result, quintptr information, const QSpan<const std::byte> *sources,
            void *voidExtra,
            qxp::function_ref<qint64(std::variant<QFileDevice::FileError, qint64>)> setResult);
    template <Operation Op>
    ReadWriteStatus handleWriteCompletion(const IORING_CQE *cqe, GenericRequestType *request);
#endif
};

struct QIORingRequestEmptyBase
{
};

template <QtPrivate::Operation Op>
struct QIORingResult;
template <QtPrivate::Operation Op>
struct QIORingRequest;

// @todo: q23::expected once emplace() returns a reference
template <QtPrivate::Operation Op>
using ExpectedResultType = std::variant<std::monostate, QIORingResult<Op>, QFileDevice::FileError>;

struct QIORingRequestOffsetFdBase : QIORingRequestEmptyBase
{
    qintptr fd;
    quint64 offset;
};

template <QtPrivate::Operation Op, typename Base = QIORingRequestOffsetFdBase>
struct QIORingRequestBase : Base
{
    ExpectedResultType<Op> result; // To be filled in by the backend
    QtPrivate::SlotObjUniquePtr callback;
    template <typename Func>
    Q_ALWAYS_INLINE void setCallback(Func &&func)
    {
        using Prototype = void (*)(const QIORingRequest<Op> &);
        callback.reset(QtPrivate::makeCallableObject<Prototype>(std::forward<Func>(func)));
    }
};

template <>
struct QIORingResult<QtPrivate::Operation::Open>
{
    // On Windows this is a HANDLE
    qintptr fd;
};
template <>
struct QIORingRequest<QtPrivate::Operation::Open> final
    : QIORingRequestBase<QtPrivate::Operation::Open, QIORingRequestEmptyBase>
{
    std::filesystem::path path;
    QFileDevice::OpenMode flags;
};
template <>
struct QIORingResult<QtPrivate::Operation::Close>
{
};
template <>
struct QIORingRequest<QtPrivate::Operation::Close> final
    : QIORingRequestBase<QtPrivate::Operation::Close, QIORingRequestEmptyBase>
{
    // On Windows this is a HANDLE
    qintptr fd;
};

template <>
struct QIORingResult<QtPrivate::Operation::Write>
{
    qint64 bytesWritten;
};
template <>
struct QIORingRequest<QtPrivate::Operation::Write> final
    : QIORingRequestBase<QtPrivate::Operation::Write>
{
    QSpan<const std::byte> source;
};
template <>
struct QIORingResult<QtPrivate::Operation::VectoredWrite> final
    : QIORingResult<QtPrivate::Operation::Write>
{
};
template <>
struct QIORingRequest<QtPrivate::Operation::VectoredWrite> final
    : QIORingRequestBase<QtPrivate::Operation::VectoredWrite>
{
    QSpan<const QSpan<const std::byte>> sources;
};

template <>
struct QIORingResult<QtPrivate::Operation::Read>
{
    qint64 bytesRead;
};
template <>
struct QIORingRequest<QtPrivate::Operation::Read> final
    : QIORingRequestBase<QtPrivate::Operation::Read>
{
    QSpan<std::byte> destination;
};

template <>
struct QIORingResult<QtPrivate::Operation::VectoredRead> final
    : QIORingResult<QtPrivate::Operation::Read>
{
};
template <>
struct QIORingRequest<QtPrivate::Operation::VectoredRead> final
    : QIORingRequestBase<QtPrivate::Operation::VectoredRead>
{
    QSpan<QSpan<std::byte>> destinations;
};

template <>
struct QIORingResult<QtPrivate::Operation::Flush> final
{
    // No value in the result, just a success or failure
};
template <>
struct QIORingRequest<QtPrivate::Operation::Flush> final : QIORingRequestBase<QtPrivate::Operation::Flush, QIORingRequestEmptyBase>
{
    // On Windows this is a HANDLE
    qintptr fd;
};

template <>
struct QIORingResult<QtPrivate::Operation::Stat> final
{
    quint64 size;
};
template <>
struct QIORingRequest<QtPrivate::Operation::Stat> final
    : QIORingRequestBase<QtPrivate::Operation::Stat, QIORingRequestEmptyBase>
{
    // On Windows this is a HANDLE
    qintptr fd;
};

// This is not inheriting the QIORingRequestBase because it doesn't have a result,
// whether it was successful or not is indicated by whether the operation
// it was cancelling was successful or not.
template <>
struct QIORingRequest<QtPrivate::Operation::Cancel> final : QIORingRequestEmptyBase
{
    QIORing::RequestHandle handle;
    QtPrivate::SlotObjUniquePtr callback;
    template <typename Func>
    Q_ALWAYS_INLINE void setCallback(Func &&func)
    {
        using Op = QtPrivate::Operation;
        using Prototype = void (*)(const QIORingRequest<Op::Cancel> &);
        callback.reset(QtPrivate::makeCallableObject<Prototype>(std::forward<Func>(func)));
    }
};

template <QIORing::Operation Op>
Q_ALWAYS_INLINE void invokeCallback(const QIORingRequest<Op> &request)
{
    if (!request.callback)
        return;
    void *args[2] = { nullptr, const_cast<QIORingRequest<Op> *>(&request) };
    request.callback->call(nullptr, args);
}

class QIORing::GenericRequestType
{
    friend class QIORing;

#define POPULATE_VARIANT(Op) \
    QIORingRequest<Operation::Op>, \
    /**/

    std::variant<
        FOREACH_IO_OPERATION(POPULATE_VARIANT)
        std::monostate
    > taggedUnion;

#undef POPULATE_VARIANT

    void *extraData = nullptr;
    bool queued = false;

    template <Operation Op>
    Q_ALWAYS_INLINE void initializeStorage(QIORingRequest<Op> &&t) noexcept
    {
        static_assert(Op < Operation::NumOperations);
        taggedUnion.emplace<QIORingRequest<Op>>(std::move(t));
    }

    Q_CORE_EXPORT
    static void cleanupExtra(Operation op, void *extra);
    template <typename T>
    T *getOrInitializeExtra()
    {
        if (!extraData)
            extraData = new T();
        return static_cast<T *>(extraData);
    }
    template <typename T>
    T *getExtra() const
    {
        return static_cast<T *>(extraData);
    }
    void reset() noexcept
    {
        Operation op = operation();
        taggedUnion.emplace<std::monostate>();
        if (extraData)
            cleanupExtra(op, std::exchange(extraData, nullptr));
    }

public:
    template <Operation Op>
    explicit GenericRequestType(QIORingRequest<Op> &&t) noexcept
    {
        initializeStorage(std::move(t));
    }
    ~GenericRequestType() noexcept
    {
        reset();
    }
    Q_DISABLE_COPY_MOVE(GenericRequestType)
    // We have to provide equality operators. Since copying is disabled, we just check for equality
    // based on the address in memory. Two requests could be constructed to be equal, but we don't
    // actually care because the order in which they are added to the queue may also matter.
    friend bool operator==(const GenericRequestType &l, const GenericRequestType &r) noexcept
    {
        return std::addressof(l) == std::addressof(r);
    }
    friend bool operator!=(const GenericRequestType &l, const GenericRequestType &r) noexcept
    {
        return !(l == r);
    }

    Operation operation() const { return Operation(taggedUnion.index()); }
    template <Operation Op>
    QIORingRequest<Op> *requestData()
    {
        if (operation() == Op)
            return std::get_if<QIORingRequest<Op>>(&taggedUnion);
        Q_ASSERT("Wrong operation requested, see operation()");
        return nullptr;
    }
    template <Operation Op>
    QIORingRequest<Op> takeRequestData()
    {
        if (operation() == Op)
            return std::move(*std::get_if<QIORingRequest<Op>>(&taggedUnion));
        Q_ASSERT("Wrong operation requested, see operation()");
        return {};
    }
    bool wasQueued() const { return queued; }
    void setQueued(bool status) { queued = status; }
};

template <typename Fun>
auto QIORing::invokeOnOp(GenericRequestType &req, Fun fn)
{
#define INVOKE_ON_OP(Op)                           \
case QIORing::Operation::Op:                       \
    fn(req.template requestData<Operation::Op>()); \
    return;                                        \
    /**/

    switch (req.operation()) {
        FOREACH_IO_OPERATION(INVOKE_ON_OP)
    case QIORing::Operation::NumOperations:
        break;
    }

    Q_UNREACHABLE();
#undef INVOKE_ON_OP
}

namespace QtPrivate {
// The 'extra' struct for Read/Write operations that must be split up
struct ReadWriteExtra
{
    qint64 totalProcessed = 0;
    qsizetype spanIndex = 0;
    qsizetype spanOffset = 0;
    qsizetype numSpans = 1;
};
} // namespace QtPrivate

QT_END_NAMESPACE

#endif // IOPROCESSOR_P_H
