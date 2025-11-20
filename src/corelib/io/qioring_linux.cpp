// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qioring_p.h"

QT_REQUIRE_CONFIG(liburing);

#include <QtCore/qobject.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/private/qfiledevice_p.h>

#include <liburing.h>
#include <sys/mman.h>
#include <sys/eventfd.h>
#include <sys/stat.h>

QT_BEGIN_NAMESPACE

// We pretend that iovec and QSpans are the same, assert that size and alignment match:
static_assert(sizeof(iovec)
              == sizeof(decltype(std::declval<QIORingRequest<QIORing::Operation::VectoredRead>>()
                                         .destinations)));
static_assert(alignof(iovec)
              == alignof(decltype(std::declval<QIORingRequest<QIORing::Operation::VectoredRead>>()
                                          .destinations)));

static io_uring_op toUringOp(QIORing::Operation op);

QIORing::~QIORing()
{
    if (eventDescriptor != -1)
        close(eventDescriptor);
    if (io_uringFd != -1)
        close(io_uringFd);
}

bool QIORing::initializeIORing()
{
    if (io_uringFd != -1)
        return true;

    io_uring_params params{};
    params.flags = IORING_SETUP_CQSIZE;
    params.cq_entries = cqEntries;
    const int fd = io_uring_setup(sqEntries, &params);
    if (fd < 0) {
        qErrnoWarning(-fd, "Failed to setup io_uring");
        return false;
    }
    io_uringFd = fd;
    size_t submissionQueueSize = params.sq_off.array + (params.sq_entries * sizeof(quint32));
    size_t completionQueueSize = params.cq_off.cqes + (params.cq_entries * sizeof(io_uring_cqe));
    if (params.features & IORING_FEAT_SINGLE_MMAP)
        submissionQueueSize = std::max(submissionQueueSize, completionQueueSize);
    submissionQueue = mmap(nullptr, submissionQueueSize, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_POPULATE, io_uringFd, IORING_OFF_SQ_RING);
    if (submissionQueue == MAP_FAILED) {
        qErrnoWarning(errno, "Failed to mmap io_uring submission queue");
        close(io_uringFd);
        io_uringFd = -1;
        return false;
    }
    const size_t submissionQueueEntriesSize = params.sq_entries * sizeof(io_uring_sqe);
    submissionQueueEntries = static_cast<io_uring_sqe *>(
            mmap(nullptr, submissionQueueEntriesSize, PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_POPULATE, io_uringFd, IORING_OFF_SQES));
    if (submissionQueueEntries == MAP_FAILED) {
        qErrnoWarning(errno, "Failed to mmap io_uring submission queue entries");
        munmap(submissionQueue, submissionQueueSize);
        close(io_uringFd);
        io_uringFd = -1;
        return false;
    }
    void *completionQueue = nullptr;
    if (params.features & IORING_FEAT_SINGLE_MMAP) {
        completionQueue = submissionQueue;
    } else {
        completionQueue = mmap(nullptr, completionQueueSize, PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_POPULATE, io_uringFd, IORING_OFF_CQ_RING);
        if (completionQueue == MAP_FAILED) {
            qErrnoWarning(errno, "Failed to mmap io_uring completion queue");
            munmap(submissionQueue, submissionQueueSize);
            munmap(submissionQueueEntries, submissionQueueEntriesSize);
            close(io_uringFd);
            io_uringFd = -1;
            return false;
        }
    }
    sqEntries = params.sq_entries;
    cqEntries = params.cq_entries;

    char *sq = static_cast<char *>(submissionQueue);
    sqHead = reinterpret_cast<quint32 *>(sq + params.sq_off.head);
    sqTail = reinterpret_cast<quint32 *>(sq + params.sq_off.tail);
    sqIndexMask = reinterpret_cast<quint32 *>(sq + params.sq_off.ring_mask);
    sqIndexArray = reinterpret_cast<quint32 *>(sq + params.sq_off.array);

    char *cq = static_cast<char *>(completionQueue);
    cqHead = reinterpret_cast<quint32 *>(cq + params.cq_off.head);
    cqTail = reinterpret_cast<quint32 *>(cq + params.cq_off.tail);
    cqIndexMask = reinterpret_cast<quint32 *>(cq + params.cq_off.ring_mask);
    completionQueueEntries = reinterpret_cast<io_uring_cqe *>(cq + params.cq_off.cqes);

    eventDescriptor = eventfd(0, 0);
    io_uring_register(io_uringFd, IORING_REGISTER_EVENTFD, &eventDescriptor, 1);

    notifier.emplace(eventDescriptor, QSocketNotifier::Read);
    QObject::connect(std::addressof(*notifier), &QSocketNotifier::activated,
                     std::addressof(*notifier), [this]() { completionReady(); });
    return true;
}

QFileDevice::FileError QIORing::mapFileError(NativeResultType error,
                                             QFileDevice::FileError defaultValue)
{
    Q_ASSERT(error < 0);
    if (-error == ECANCELED)
        return QFileDevice::AbortError;
    return defaultValue;
}

void QIORing::completionReady()
{
    // Drain the eventfd:
    [[maybe_unused]]
    quint64 ignored = 0;
    std::ignore = read(eventDescriptor, &ignored, sizeof(ignored));

    quint32 head = __atomic_load_n(cqHead, __ATOMIC_RELAXED);
    const quint32 tail = __atomic_load_n(cqTail, __ATOMIC_ACQUIRE);
    if (tail == head)
        return;

    qCDebug(lcQIORing,
            "Status of completion queue, total entries: %u, tail: %u, head: %u, to process: %u",
            cqEntries, tail, head, (tail - head));
    while (head != tail) {
        /* Get the entry */
        const io_uring_cqe *cqe = &completionQueueEntries[head & *cqIndexMask];
        ++head;
        GenericRequestType *request = reinterpret_cast<GenericRequestType *>(cqe->user_data);
        qCDebug(lcQIORing) << "Got completed entry. Operation:" << request->operation()
                           << "- user_data pointer:" << request;
        switch (request->operation()) {
        case Operation::Open: {
            QIORingRequest<Operation::Open>
                    openRequest = request->template takeRequestData<Operation::Open>();
            if (cqe->res < 0) {
                // qErrnoWarning(-cqe->res, "Failed to open");
                if (-cqe->res == ECANCELED)
                    openRequest.result.template emplace<QFileDevice::FileError>(
                            QFileDevice::AbortError);
                else
                    openRequest.result.template emplace<QFileDevice::FileError>(
                            QFileDevice::OpenError);
            } else {
                auto &result = openRequest.result
                                       .template emplace<QIORingResult<Operation::Open>>();
                result.fd = cqe->res;
            }
            invokeCallback(openRequest);
            break;
        }
        case Operation::Close: {
            QIORingRequest<Operation::Close>
                    closeRequest = request->template takeRequestData<Operation::Close>();
            if (cqe->res < 0) {
                closeRequest.result.emplace<QFileDevice::FileError>(QFileDevice::OpenError);
            } else {
                closeRequest.result.emplace<QIORingResult<Operation::Close>>();
            }
            invokeCallback(closeRequest);
            break;
        }
        case Operation::Read: {
            const ReadWriteStatus status = handleReadCompletion<Operation::Read>(
                    cqe->res, size_t(cqe->res), request);
            if (status == ReadWriteStatus::MoreToDo)
                continue;
            auto readRequest = request->takeRequestData<Operation::Read>();
            invokeCallback(readRequest);
            break;
        }
        case Operation::Write: {
            const ReadWriteStatus status = handleWriteCompletion<Operation::Write>(
                    cqe->res, size_t(cqe->res), request);
            if (status == ReadWriteStatus::MoreToDo)
                continue;
            auto writeRequest = request->takeRequestData<Operation::Write>();
            invokeCallback(writeRequest);
            break;
        }
        case Operation::VectoredRead: {
            const ReadWriteStatus status = handleReadCompletion<Operation::VectoredRead>(
                    cqe->res, size_t(cqe->res), request);
            if (status == ReadWriteStatus::MoreToDo)
                continue;
            auto readvRequest = request->takeRequestData<Operation::VectoredRead>();
            invokeCallback(readvRequest);
            break;
        }
        case Operation::VectoredWrite: {
            const ReadWriteStatus status = handleWriteCompletion<Operation::VectoredWrite>(
                    cqe->res, size_t(cqe->res), request);
            if (status == ReadWriteStatus::MoreToDo)
                continue;
            auto writevRequest = request->takeRequestData<Operation::VectoredWrite>();
            invokeCallback(writevRequest);
            break;
        }
        case Operation::Flush: {
            QIORingRequest<Operation::Flush>
                    flushRequest = request->template takeRequestData<Operation::Flush>();
            if (cqe->res < 0) {
                flushRequest.result.emplace<QFileDevice::FileError>(QFileDevice::WriteError);
            } else {
                // No members to fill out, so just initialize to indicate success
                flushRequest.result.emplace<QIORingResult<Operation::Flush>>();
            }
            flushInProgress = false;
            invokeCallback(flushRequest);
            break;
        }
        case Operation::Cancel: {
            QIORingRequest<Operation::Cancel>
                    cancelRequest = request->template takeRequestData<Operation::Cancel>();
            invokeCallback(cancelRequest);
            break;
        }
        case Operation::Stat: {
            QIORingRequest<Operation::Stat>
                    statRequest = request->template takeRequestData<Operation::Stat>();
            if (cqe->res < 0) {
                statRequest.result.emplace<QFileDevice::FileError>(QFileDevice::OpenError);
            } else {
                struct statx *st = request->getExtra<struct statx>();
                Q_ASSERT(st);
                auto &res = statRequest.result.emplace<QIORingResult<Operation::Stat>>();
                res.size = st->stx_size;
            }
            invokeCallback(statRequest);
            break;
        }
        case Operation::NumOperations:
            Q_UNREACHABLE_RETURN();
            break;
        }
        --inFlightRequests;
        auto it = addrItMap.take(request);
        pendingRequests.erase(it);
    }
    __atomic_store_n(cqHead, head, __ATOMIC_RELEASE);
    qCDebug(lcQIORing,
            "Done processing available completions, updated pointers, tail: %u, head: %u", tail,
            head);
    prepareRequests();
    if (!stagePending && unstagedRequests > 0)
        submitRequests();
}

bool QIORing::waitForCompletions(QDeadlineTimer deadline)
{
    notifier->setEnabled(false);
    auto reactivateNotifier = qScopeGuard([this]() {
        notifier->setEnabled(true);
    });

    pollfd pfd = qt_make_pollfd(eventDescriptor, POLLIN);
    return qt_safe_poll(&pfd, 1, deadline) > 0;
}

bool QIORing::supportsOperation(Operation op)
{
    switch (op) {
    case QtPrivate::Operation::Open:
    case QtPrivate::Operation::Close:
    case QtPrivate::Operation::Read:
    case QtPrivate::Operation::Write:
    case QtPrivate::Operation::VectoredRead:
    case QtPrivate::Operation::VectoredWrite:
    case QtPrivate::Operation::Flush:
    case QtPrivate::Operation::Cancel:
    case QtPrivate::Operation::Stat:
        return true;
    case QtPrivate::Operation::NumOperations:
        return false;
    }
    return false; // May not always be unreachable!
}

void QIORing::submitRequests()
{
    stagePending = false;
    if (unstagedRequests == 0)
        return;

    auto submitToRing = [this] {
        int ret = io_uring_enter(io_uringFd, unstagedRequests, 0, 0, nullptr);
        if (ret < 0)
            qErrnoWarning("Error occurred notifying kernel about requests...");
        else
            unstagedRequests -= ret;
        qCDebug(lcQIORing) << "io_uring_enter returned" << ret;
        return ret >= 0;
    };
    if (submitToRing()) {
        prepareRequests();
        if (unstagedRequests)
            submitToRing();
    }
}

namespace QtPrivate {
template <typename T>
using DetectFd = decltype(std::declval<const T &>().fd);

template <typename T>
constexpr bool HasFdMember = qxp::is_detected_v<DetectFd, T>;
} // namespace QtPrivate

bool QIORing::verifyFd(QIORing::GenericRequestType &req)
{
    bool result = true;
    invokeOnOp(req, [&](auto *request) {
        if constexpr (QtPrivate::HasFdMember<decltype(*request)>) {
            result = request->fd > 0;
        }
    });
    return result;
}

void QIORing::prepareRequests()
{
    if (!lastUnqueuedIterator) {
        qCDebug(lcQIORing, "Nothing left to queue");
        return;
    }
    Q_ASSERT(!preparingRequests);
    QScopedValueRollback<bool> prepareGuard(preparingRequests, true);

    quint32 tail = __atomic_load_n(sqTail, __ATOMIC_RELAXED);
    const quint32 head = __atomic_load_n(sqHead, __ATOMIC_ACQUIRE);
    qCDebug(lcQIORing,
            "Status of submission queue, total entries: %u, tail: %u, head: %u, free: %u",
            sqEntries, tail, head, sqEntries - (tail - head));

    auto it = *lastUnqueuedIterator;
    lastUnqueuedIterator.reset();
    const auto end = pendingRequests.end();
    bool anyQueued = false;
    // Loop until we either:
    // 1. Run out of requests to prepare for submission (it == end),
    // 2. Have filled the submission queue (unstagedRequests == sqEntries) or,
    // 3. The number of staged requests + currently processing/potentially finished requests is
    //    enough to fill the completion queue (inFlightRequests == cqEntries).
    while (!flushInProgress && unstagedRequests != sqEntries && inFlightRequests != cqEntries
           && it != end) {
        const quint32 index = tail & *sqIndexMask;
        io_uring_sqe *sqe = &submissionQueueEntries[index];
        *sqe = {};
        RequestPrepResult result = prepareRequest(sqe, *it);

        // QueueFull is unused on Linux:
        Q_ASSERT(result != RequestPrepResult::QueueFull);
        if (result == RequestPrepResult::Defer) {
            qCDebug(lcQIORing) << "Request for" << it->operation()
                   << "had to be deferred, will not queue any more requests at the moment.";
            break;
        }
        if (result == RequestPrepResult::RequestCompleted) {
            addrItMap.remove(std::addressof(*it));
            it = pendingRequests.erase(it); // Completed synchronously, either failure or success.
            continue;
        }
        anyQueued = true;
        it->setQueued(true);

        sqIndexArray[index] = index;
        ++inFlightRequests;
        ++unstagedRequests;
        ++tail;
        ++it;
    }
    if (it != end)
        lastUnqueuedIterator = it;

    if (anyQueued) {
        qCDebug(lcQIORing, "Queued %u operation(s)",
                tail - __atomic_load_n(sqTail, __ATOMIC_RELAXED));
        __atomic_store_n(sqTail, tail, __ATOMIC_RELEASE);
    }
}

static io_uring_op toUringOp(QIORing::Operation op)
{
    switch (op) {
    case QIORing::Operation::Open:
        return IORING_OP_OPENAT;
    case QIORing::Operation::Read:
        return IORING_OP_READ;
    case QIORing::Operation::Close:
        return IORING_OP_CLOSE;
    case QIORing::Operation::Write:
        return IORING_OP_WRITE;
    case QIORing::Operation::VectoredRead:
        return IORING_OP_READV;
    case QIORing::Operation::VectoredWrite:
        return IORING_OP_WRITEV;
    case QIORing::Operation::Flush:
        return IORING_OP_FSYNC;
    case QIORing::Operation::Cancel:
        return IORING_OP_ASYNC_CANCEL;
    case QIORing::Operation::Stat:
        return IORING_OP_STATX;
    case QIORing::Operation::NumOperations:
        break;
    }
    Q_UNREACHABLE_RETURN(IORING_OP_NOP);
}

Q_ALWAYS_INLINE
static void prepareFileIOCommon(io_uring_sqe *sqe, const QIORingRequestOffsetFdBase &request, quint64 offset)
{
    sqe->fd = qint32(request.fd);
    sqe->off = offset;
}

Q_ALWAYS_INLINE
static void prepareFileReadWrite(io_uring_sqe *sqe, const QIORingRequestOffsetFdBase &request,
                                 const void *address, quint64 offset, qsizetype size)
{
    prepareFileIOCommon(sqe, request, offset);
    sqe->len = quint32(size);
    sqe->addr = quint64(address);
}

// @todo: stolen from qfsfileengine_unix.cpp
static inline int openModeToOpenFlags(QIODevice::OpenMode mode)
{
    int oflags = QT_OPEN_RDONLY;
#ifdef QT_LARGEFILE_SUPPORT
    oflags |= QT_OPEN_LARGEFILE;
#endif

    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite)
        oflags = QT_OPEN_RDWR;
    else if (mode & QIODevice::WriteOnly)
        oflags = QT_OPEN_WRONLY;

    if ((mode & QIODevice::WriteOnly)
        && !(mode & QIODevice::ExistingOnly)) // QFSFileEnginePrivate::openModeCanCreate(mode))
        oflags |= QT_OPEN_CREAT;

    if (mode & QIODevice::Truncate)
        oflags |= QT_OPEN_TRUNC;

    if (mode & QIODevice::Append)
        oflags |= QT_OPEN_APPEND;

    if (mode & QIODevice::NewOnly)
        oflags |= QT_OPEN_EXCL;

    return oflags;
}

auto QIORing::prepareRequest(io_uring_sqe *sqe, GenericRequestType &request) -> RequestPrepResult
{
    sqe->user_data = qint64(&request);
    sqe->opcode = toUringOp(request.operation());

    if (!verifyFd(request)) {
        finishRequestWithError(request, QFileDevice::OpenError);
        return RequestPrepResult::RequestCompleted;
    }

    switch (request.operation()) {
    case Operation::Open: {
        const QIORingRequest<Operation::Open>
                *openRequest = request.template requestData<Operation::Open>();
        sqe->fd = AT_FDCWD; // Could also support proper openat semantics
        sqe->addr = reinterpret_cast<quint64>(openRequest->path.native().c_str());
        sqe->open_flags = openModeToOpenFlags(openRequest->flags);
        auto &mode = sqe->len;
        mode = 0666; // With an explicit API we can use QtPrivate::toMode_t() for this
        break;
    }
    case Operation::Close: {
        if (ongoingSplitOperations)
            return Defer;
        const QIORingRequest<Operation::Close>
                *closeRequest = request.template requestData<Operation::Close>();
        sqe->fd = closeRequest->fd;
        // Force all earlier entries in the sq to finish before this is processed:
        sqe->flags |= IOSQE_IO_DRAIN;
        break;
    }
    case Operation::Read: {
        const QIORingRequest<Operation::Read>
                *readRequest = request.template requestData<Operation::Read>();
        auto span = readRequest->destination;
        auto offset = readRequest->offset;
        if (span.size() >= MaxReadWriteLen) {
            qCDebug(lcQIORing) << "Requested Read of size" << span.size() << "has to be split";
            auto *extra = request.getOrInitializeExtra<QtPrivate::ReadWriteExtra>();
            if (extra->spanOffset == 0) // First time setup
                ++ongoingSplitOperations;
            qsizetype remaining = span.size() - extra->spanOffset;
            span.slice(extra->spanOffset, std::min(remaining, MaxReadWriteLen));
            offset += extra->totalProcessed;
        }
        prepareFileReadWrite(sqe, *readRequest, span.data(), offset, span.size());
        break;
    }
    case Operation::Write: {
        const QIORingRequest<Operation::Write>
                *writeRequest = request.template requestData<Operation::Write>();
        auto span = writeRequest->source;
        auto offset = writeRequest->offset;
        if (span.size() >= MaxReadWriteLen) {
            qCDebug(lcQIORing) << "Requested Write of size" << span.size() << "has to be split";
            auto *extra = request.getOrInitializeExtra<QtPrivate::ReadWriteExtra>();
            if (extra->spanOffset == 0) // First time setup
                ++ongoingSplitOperations;
            qsizetype remaining = span.size() - extra->spanOffset;
            span.slice(extra->spanOffset, std::min(remaining, MaxReadWriteLen));
            offset += extra->totalProcessed;
        }
        prepareFileReadWrite(sqe, *writeRequest, span.data(), offset, span.size());
        break;
    }
    case Operation::VectoredRead: {
        // @todo Apply the split read/write concept that will apply above to this too
        const QIORingRequest<Operation::VectoredRead>
                *readvRequest = request.template requestData<Operation::VectoredRead>();
        prepareFileReadWrite(sqe, *readvRequest, readvRequest->destinations.data(), readvRequest->offset,
                             readvRequest->destinations.size());
        break;
    }
    case Operation::VectoredWrite: {
        // @todo Apply the split read/write concept that will apply above to this too
        const QIORingRequest<Operation::VectoredWrite>
                *writevRequest = request.template requestData<Operation::VectoredWrite>();
        prepareFileReadWrite(sqe, *writevRequest, writevRequest->sources.data(), writevRequest->offset,
                             writevRequest->sources.size());
        break;
    }
    case Operation::Flush: {
        if (ongoingSplitOperations)
            return Defer;
        const QIORingRequest<Operation::Flush>
                *flushRequest = request.template requestData<Operation::Flush>();
        sqe->fd = qint32(flushRequest->fd);
        // Force all earlier entries in the sq to finish before this is processed:
        sqe->flags |= IOSQE_IO_DRAIN;
        flushInProgress = true;
        break;
    }
    case Operation::Cancel: {
        const QIORingRequest<Operation::Cancel>
                *cancelRequest = request.template requestData<Operation::Cancel>();
        auto *otherOperation = reinterpret_cast<GenericRequestType *>(cancelRequest->handle);
        auto it = std::as_const(addrItMap).find(otherOperation);
        if (it == addrItMap.cend()) { // : The request to cancel doesn't exist
            invokeCallback(*cancelRequest);
            return RequestPrepResult::RequestCompleted;
        }
        if (!otherOperation->wasQueued()) {
            // The request hasn't been queued yet, so we can just drop it from
            // the pending requests and call the callback.
            Q_ASSERT(!lastUnqueuedIterator);
            finishRequestWithError(*otherOperation, QFileDevice::AbortError);
            pendingRequests.erase(*it); // otherOperation is deleted
            addrItMap.erase(it);
            invokeCallback(*cancelRequest);
            return RequestPrepResult::RequestCompleted;
        }
        sqe->addr = quint64(otherOperation);
        break;
    }
    case Operation::Stat: {
        const QIORingRequest<Operation::Stat>
                *statRequest = request.template requestData<Operation::Stat>();
        // We need to store the statx struct somewhere:
        struct statx *st = request.getOrInitializeExtra<struct statx>();

        sqe->fd = statRequest->fd;
        // We want to use the fd as the target of query instead of as the fd of the relative dir,
        // so we set addr to an empty string, and specify the AT_EMPTY_PATH flag.
        static const char emptystr[] = "";
        sqe->addr = qint64(emptystr);
        sqe->statx_flags = AT_EMPTY_PATH;
        sqe->len = STATX_ALL; // @todo configure somehow
        sqe->off = quint64(st);
        break;
    }
    case Operation::NumOperations:
        Q_UNREACHABLE_RETURN(RequestPrepResult::RequestCompleted);
        break;
    }
    return RequestPrepResult::Ok;
}

void QIORing::GenericRequestType::cleanupExtra(Operation op, void *extra)
{
    switch (op) {
    case Operation::Open:
    case Operation::Close:
    case Operation::VectoredRead:
    case Operation::VectoredWrite:
    case Operation::Cancel:
    case Operation::Flush:
    case Operation::NumOperations:
        break;
    case Operation::Read:
    case Operation::Write:
        delete static_cast<QtPrivate::ReadWriteExtra *>(extra);
        return;
    case Operation::Stat:
        delete static_cast<struct statx *>(extra);
        return;
    }
}

QT_END_NAMESPACE
