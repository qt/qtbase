// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qioring_p.h"

QT_REQUIRE_CONFIG(windows_ioring);

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qobject.h>
#include <QtCore/qscopedvaluerollback.h>

#include <qt_windows.h>
#include <ioringapi.h>

#include <QtCore/q26numeric.h>

QT_BEGIN_NAMESPACE

// We don't really build for 32-bit windows anymore, but this code is definitely wrong if someone
// does.
static_assert(sizeof(qsizetype) > sizeof(UINT32),
              "This code is written with assuming 64-bit Windows.");

using namespace Qt::StringLiterals;

namespace QtPrivate {
#define FOREACH_WIN_IORING_FUNCTION(Fn) \
    Fn(BuildIoRingReadFile) \
    Fn(BuildIoRingWriteFile) \
    Fn(BuildIoRingFlushFile) \
    Fn(BuildIoRingCancelRequest) \
    Fn(QueryIoRingCapabilities) \
    Fn(CreateIoRing) \
    Fn(GetIoRingInfo) \
    Fn(SubmitIoRing) \
    Fn(CloseIoRing) \
    Fn(PopIoRingCompletion) \
    Fn(SetIoRingCompletionEvent) \
    /**/
struct IORingApiTable
{
#define DefineIORingFunction(Name) \
    using Name##Fn = decltype(&::Name); \
    Name##Fn Name = nullptr;

    FOREACH_WIN_IORING_FUNCTION(DefineIORingFunction)

#undef DefineIORingFunction
};

static const IORingApiTable *getApiTable()
{
    static const IORingApiTable apiTable = []() {
        IORingApiTable apiTable;
        const HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        if (Q_UNLIKELY(!kernel32)) // how would this happen
            return apiTable;

#define ResolveFunction(Name) \
    apiTable.Name = IORingApiTable::Name##Fn(QFunctionPointer(GetProcAddress(kernel32, #Name)));

        FOREACH_WIN_IORING_FUNCTION(ResolveFunction)

#undef ResolveFunction
        return apiTable;
    }();

#define TEST_TABLE_OK(X) \
    apiTable.X && /* chain */
#define BOOL_CHAIN(...) (__VA_ARGS__ true)
    const bool success = BOOL_CHAIN(FOREACH_WIN_IORING_FUNCTION(TEST_TABLE_OK));
#undef BOOL_CHAIN
#undef TEST_TABLE_OK

    return success ? std::addressof(apiTable) : nullptr;
}
} // namespace QtPrivate

static HRESULT buildReadOperation(HIORING ioRingHandle, qintptr fd, QSpan<std::byte> destination,
                                  quint64 offset, quintptr userData)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    const IORING_HANDLE_REF fileRef((HANDLE(fd)));
    const IORING_BUFFER_REF bufferRef(destination.data());
    const auto maxSize = q26::saturate_cast<UINT32>(destination.size());
    Q_ASSERT(maxSize == destination.size());
    const auto *apiTable = QtPrivate::getApiTable();
    Q_ASSERT(apiTable); // If we got this far it needs to be here
    return apiTable->BuildIoRingReadFile(ioRingHandle, fileRef, bufferRef, maxSize, offset,
                                         userData, IOSQE_FLAGS_NONE);
}

static HRESULT buildWriteOperation(HIORING ioRingHandle, qintptr fd, QSpan<const std::byte> source,
                                   quint64 offset, quintptr userData)
{
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    const IORING_HANDLE_REF fileRef((HANDLE(fd)));
    const IORING_BUFFER_REF bufferRef(const_cast<std::byte *>(source.data()));
    const auto maxSize = q26::saturate_cast<UINT32>(source.size());
    Q_ASSERT(maxSize == source.size());
    const auto *apiTable = QtPrivate::getApiTable();
    Q_ASSERT(apiTable); // If we got this far it needs to be here
    // @todo: FILE_WRITE_FLAGS can be set to write-through, could be used for Unbuffered mode.
    return apiTable->BuildIoRingWriteFile(ioRingHandle, fileRef, bufferRef, maxSize, offset,
                                          FILE_WRITE_FLAGS_NONE, userData, IOSQE_FLAGS_NONE);
}

QIORing::~QIORing()
{
    if (initialized) {
        CloseHandle(eventHandle);
        apiTable->CloseIoRing(ioRingHandle);
    }
}

bool QIORing::initializeIORing()
{
    if (initialized)
        return true;

    if (apiTable = QtPrivate::getApiTable(); !apiTable) {
        qCWarning(lcQIORing, "Failed to retrieve API table");
        return false;
    }

    IORING_CAPABILITIES capabilities;
    apiTable->QueryIoRingCapabilities(&capabilities);
    if (capabilities.MaxVersion < IORING_VERSION_3) // 3 adds write, flush and drain
        return false;
    if ((capabilities.FeatureFlags & IORING_FEATURE_SET_COMPLETION_EVENT) == 0)
        return false; // We currently require the SET_COMPLETION_EVENT feature

    qCDebug(lcQIORing) << "Creating QIORing, requesting space for" << sqEntries
                       << "submission queue entries, and" << cqEntries
                       << "completion queue entries";

    IORING_CREATE_FLAGS flags;
    memset(&flags, 0, sizeof(flags));
    HRESULT hr = apiTable->CreateIoRing(IORING_VERSION_3, flags, sqEntries, cqEntries,
                                        &ioRingHandle);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "failed to initialize QIORing");
        return false;
    }
    auto earlyExitCleanup = qScopeGuard([this]() {
        if (eventHandle != INVALID_HANDLE_VALUE)
            CloseHandle(eventHandle);
        apiTable->CloseIoRing(ioRingHandle);
    });
    eventHandle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (eventHandle == INVALID_HANDLE_VALUE) {
        qErrnoWarning("Failed to create event handle");
        return false;
    }
    notifier.emplace(eventHandle);
    hr = apiTable->SetIoRingCompletionEvent(ioRingHandle, eventHandle);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to assign the event handle to QIORing");
        return false;
    }
    IORING_INFO info;
    if (SUCCEEDED(apiTable->GetIoRingInfo(ioRingHandle, &info))) {
        sqEntries = info.SubmissionQueueSize;
        cqEntries = info.CompletionQueueSize;
        qCDebug(lcQIORing) << "QIORing configured with capacity for" << sqEntries
                           << "submissions, and" << cqEntries << "completions.";
    }
    QObject::connect(std::addressof(*notifier), &QWinEventNotifier::activated,
                     std::addressof(*notifier), [this]() { completionReady(); });
    initialized = true;
    earlyExitCleanup.dismiss();
    return true;
}

QIORing::ReadWriteStatus QIORing::handleReadCompletion(
            HRESULT result, quintptr information, QSpan<std::byte> *destinations, void *voidExtra,
            qxp::function_ref<qint64(std::variant<QFileDevice::FileError, qint64>)> setResultFn)
{
    if (FAILED(result)) {
        if (result == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
            return ReadWriteStatus::Finished;

        if (result == E_ABORT)
            setResultFn(QFileDevice::AbortError);
        else
            setResultFn(QFileDevice::ReadError);
    } else if (auto *extra = static_cast<QtPrivate::ReadWriteExtra *>(voidExtra)) {
        const qsizetype bytesRead = q26::saturate_cast<decltype(MaxReadWriteLen)>(information);
        qCDebug(lcQIORing) << "Partial read of" << bytesRead << "bytes completed";
        extra->totalProcessed = setResultFn(bytesRead);
        extra->spanOffset += bytesRead;
        qCDebug(lcQIORing) << "Read operation progress: span" << extra->spanIndex << "offset"
                           << extra->spanOffset << "of" << destinations[extra->spanIndex].size()
                           << "bytes. Total read:" << extra->totalProcessed << "bytes";
        // The while loop is in case there is an empty span, we skip over it:
        while (extra->spanOffset == destinations[extra->spanIndex].size()) {
            // Move to next span
            if (++extra->spanIndex == extra->numSpans)
                return ReadWriteStatus::Finished;
            extra->spanOffset = 0;
        }
        return ReadWriteStatus::MoreToDo;
    } else {
        setResultFn(q26::saturate_cast<decltype(MaxReadWriteLen)>(information));
    }
    return ReadWriteStatus::Finished;
}

template <QIORing::Operation Op>
Q_ALWAYS_INLINE QIORing::ReadWriteStatus QIORing::handleReadCompletion(const IORING_CQE *cqe,
                                                                       GenericRequestType *request)
{
    static_assert(Op == Operation::Read || Op == Operation::VectoredRead);
    QIORingRequest<Op> *readRequest = request->requestData<Op>();
    Q_ASSERT(readRequest);
    auto *destinations = [&readRequest]() {
        if constexpr (Op == Operation::Read)
            return &readRequest->destination;
        else
            return &readRequest->destinations[0];
    }();
    auto setResult = [readRequest](const std::variant<QFileDevice::FileError, qint64> &result) {
        if (result.index() == 0) { // error
            QIORing::setFileErrorResult(*readRequest, *std::get_if<QFileDevice::FileError>(&result));
            return 0ll;
        }
        // else: success
        auto &readResult = [&readRequest]() -> QIORingResult<Op> & {
            if (auto *result = std::get_if<QIORingResult<Op>>(&readRequest->result))
                return *result;
            return readRequest->result.template emplace<QIORingResult<Op>>();
        }();
        qint64 bytesRead = *std::get_if<qint64>(&result);
        readResult.bytesRead += bytesRead;
        return readResult.bytesRead;
    };
    QIORing::ReadWriteStatus rwstatus = handleReadCompletion(
            cqe->ResultCode, cqe->Information, destinations, request->getExtra<void>(), setResult);
    switch (rwstatus) {
    case ReadWriteStatus::Finished:
        if (request->getExtra<void>())
            --ongoingSplitOperations;
        break;
    case ReadWriteStatus::MoreToDo: {
        // Move the request such that it is next in the list to be processed:
        auto &it = addrItMap[request];
        const auto where = lastUnqueuedIterator.value_or(pendingRequests.end());
        pendingRequests.splice(where, pendingRequests, it);
        it = std::prev(where);
        lastUnqueuedIterator = it;
        break;
    }
    }
    return rwstatus;
}

QIORing::ReadWriteStatus QIORing::handleWriteCompletion(
        HRESULT result, quintptr information, const QSpan<const std::byte> *sources, void *voidExtra,
        qxp::function_ref<qint64(std::variant<QFileDevice::FileError, qint64>)> setResultFn)
{
    if (FAILED(result)) {
        if (result == E_ABORT)
            setResultFn(QFileDevice::AbortError);
        else
            setResultFn(QFileDevice::WriteError);
    } else if (auto *extra = static_cast<QtPrivate::ReadWriteExtra *>(voidExtra)) {
        const qsizetype bytesWritten = q26::saturate_cast<decltype(MaxReadWriteLen)>(information);
        qCDebug(lcQIORing) << "Partial write of" << bytesWritten << "bytes completed";
        extra->totalProcessed = setResultFn(bytesWritten);
        extra->spanOffset += bytesWritten;
        qCDebug(lcQIORing) << "Write operation progress: span" << extra->spanIndex << "offset"
                           << extra->spanOffset << "of" << sources[extra->spanIndex].size()
                           << "bytes. Total written:" << extra->totalProcessed << "bytes";
        // The while loop is in case there is an empty span, we skip over it:
        while (extra->spanOffset == sources[extra->spanIndex].size()) {
            // Move to next span
            if (++extra->spanIndex == extra->numSpans)
                return ReadWriteStatus::Finished;
            extra->spanOffset = 0;
        }
        return ReadWriteStatus::MoreToDo;
    } else {
        setResultFn(q26::saturate_cast<decltype(MaxReadWriteLen)>(information));
    }
    return ReadWriteStatus::Finished;
}

template <QIORing::Operation Op>
Q_ALWAYS_INLINE QIORing::ReadWriteStatus QIORing::handleWriteCompletion(const IORING_CQE *cqe,
                                                                        GenericRequestType *request)
{
    static_assert(Op == Operation::Write || Op == Operation::VectoredWrite);
    QIORingRequest<Op> *writeRequest = request->requestData<Op>();
    auto *sources = [&writeRequest]() {
        if constexpr (Op == Operation::Write)
            return &writeRequest->source;
        else
            return &writeRequest->sources[0];
    }();
    auto setResult = [writeRequest](const std::variant<QFileDevice::FileError, qint64> &result) {
        if (result.index() == 0) { // error
            QIORing::setFileErrorResult(*writeRequest, *std::get_if<QFileDevice::FileError>(&result));
            return 0ll;
        }
        // else: success
        auto &writeResult = [&writeRequest]() -> QIORingResult<Op> & {
            if (auto *result = std::get_if<QIORingResult<Op>>(&writeRequest->result))
                return *result;
            return writeRequest->result.template emplace<QIORingResult<Op>>();
        }();
        qint64 bytesWritten = *std::get_if<qint64>(&result);
        writeResult.bytesWritten += bytesWritten;
        return writeResult.bytesWritten;
    };
    QIORing::ReadWriteStatus rwstatus = handleWriteCompletion(
            cqe->ResultCode, cqe->Information, sources, request->getExtra<void>(), setResult);
    switch (rwstatus) {
    case ReadWriteStatus::Finished:
        if (request->getExtra<void>())
            --ongoingSplitOperations;
        break;
    case ReadWriteStatus::MoreToDo: {
        // Move the request such that it is next in the list to be processed:
        auto &it = addrItMap[request];
        const auto where = lastUnqueuedIterator.value_or(pendingRequests.end());
        pendingRequests.splice(where, pendingRequests, it);
        it = std::prev(where);
        lastUnqueuedIterator = it;
        break;
    }
    }
    return rwstatus;
}

void QIORing::completionReady()
{
    ResetEvent(eventHandle);
    IORING_CQE entry;
    while (apiTable->PopIoRingCompletion(ioRingHandle, &entry) == S_OK) {
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        auto *request = reinterpret_cast<GenericRequestType *>(entry.UserData);
        if (!addrItMap.contains(request)) {
            qCDebug(lcQIORing) << "Got completed entry, but cannot find it in the map. Likely "
                                  "deleted, ignoring. UserData pointer:"
                               << request;
            continue;
        }
        qCDebug(lcQIORing) << "Got completed entry. Operation:" << request->operation()
                           << "- UserData pointer:" << request
                           << "- Result:" << qt_error_string(entry.ResultCode) << '('
                           << QByteArray("0x"_ba + QByteArray::number(entry.ResultCode, 16)).data()
                           << ')';
        switch (request->operation()) {
        case Operation::Open: // Synchronously finishes
            Q_UNREACHABLE_RETURN();
        case Operation::Close: {
            auto closeRequest = request->takeRequestData<Operation::Close>();
            // We ignore the result of the flush, we are closing the handle anyway.
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            if (CloseHandle(HANDLE(closeRequest.fd)))
                closeRequest.result.emplace<QIORingResult<Operation::Close>>();
            else
                closeRequest.result.emplace<QFileDevice::FileError>(QFileDevice::OpenError);
            invokeCallback(closeRequest);
            break;
        }
        case Operation::Read: {
            const ReadWriteStatus status = handleReadCompletion<Operation::Read>(&entry, request);
            if (status == ReadWriteStatus::MoreToDo)
                continue;
            auto readRequest = request->takeRequestData<Operation::Read>();
            invokeCallback(readRequest);
            break;
        }
        case Operation::Write: {
            const ReadWriteStatus status = handleWriteCompletion<Operation::Write>(&entry, request);
            if (status == ReadWriteStatus::MoreToDo)
                continue;
            auto writeRequest = request->takeRequestData<Operation::Write>();
            invokeCallback(writeRequest);
            break;
        }
        case Operation::VectoredRead: {
            const ReadWriteStatus status = handleReadCompletion<Operation::VectoredRead>(&entry,
                                                                                         request);
            if (status == ReadWriteStatus::MoreToDo)
                continue;
            auto vectoredReadRequest = request->takeRequestData<Operation::VectoredRead>();
            invokeCallback(vectoredReadRequest);
            break;
        }
        case Operation::VectoredWrite: {
            const ReadWriteStatus status = handleWriteCompletion<Operation::VectoredWrite>(&entry,
                                                                                           request);
            if (status == ReadWriteStatus::MoreToDo)
                continue;
            auto vectoredWriteRequest = request->takeRequestData<Operation::VectoredWrite>();
            invokeCallback(vectoredWriteRequest);
            break;
        }
        case Operation::Flush: {
            auto flushRequest = request->takeRequestData<Operation::Flush>();
            if (FAILED(entry.ResultCode)) {
                qErrnoWarning(entry.ResultCode, "Flush operation failed");
                // @todo any FlushError?
                flushRequest.result.emplace<QFileDevice::FileError>(
                        QFileDevice::FileError::WriteError);
            } else {
                flushRequest.result.emplace<QIORingResult<Operation::Flush>>();
            }
            invokeCallback(flushRequest);
            break;
        }
        case QtPrivate::Operation::Cancel: {
            auto cancelRequest = request->takeRequestData<Operation::Cancel>();
            invokeCallback(cancelRequest);
            break;
        }
        case QtPrivate::Operation::Stat:
            Q_UNREACHABLE_RETURN(); // Completes synchronously
            break;
        case Operation::NumOperations:
            Q_UNREACHABLE_RETURN();
            break;
        }
        auto it = addrItMap.take(request);
        pendingRequests.erase(it);
        --inFlightRequests;
        queueWasFull = false;
    }
    prepareRequests();
    if (unstagedRequests > 0)
        submitRequests();
}

bool QIORing::waitForCompletions(QDeadlineTimer deadline)
{
    notifier->setEnabled(false);
    auto reactivateNotifier = qScopeGuard([this]() {
        notifier->setEnabled(true);
    });

    while (!deadline.hasExpired()) {
        DWORD timeout = 0;
        if (deadline.isForever()) {
            timeout = INFINITE;
        } else {
            timeout = q26::saturate_cast<DWORD>(deadline.remainingTime());
            if (timeout == INFINITE)
                --timeout;
        }
        if (WaitForSingleObject(eventHandle, timeout) == WAIT_OBJECT_0)
            return true;
    }
    return false;
}

static HANDLE openFile(const QIORingRequest<QIORing::Operation::Open> &openRequest)
{
    DWORD access = 0;
    if (openRequest.flags.testFlag(QIODevice::ReadOnly))
        access |= GENERIC_READ;
    if (openRequest.flags.testFlag(QIODevice::WriteOnly))
        access |= GENERIC_WRITE;

    DWORD disposition = 0;
    if (openRequest.flags.testFlag(QIODevice::Append)) {
        qCWarning(lcQIORing, "Opening file with Append not supported for random access file");
        return INVALID_HANDLE_VALUE;
    }
    if (openRequest.flags.testFlag(QIODevice::NewOnly)) {
        disposition = CREATE_NEW;
    } else {
        // If Write is specified we _may_ create a file.
        // See qfsfileengine_p.h openModeCanCreate.
        disposition = openRequest.flags.testFlag(QIODeviceBase::WriteOnly)
                        && !openRequest.flags.testFlags(QIODeviceBase::ExistingOnly)
                ? OPEN_ALWAYS
                : OPEN_EXISTING;
    }
    const DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    const DWORD flagsAndAttribs = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
    HANDLE h = CreateFile(openRequest.path.native().c_str(), access, shareMode, nullptr,
                          disposition, flagsAndAttribs, nullptr);
    if (h != INVALID_HANDLE_VALUE && openRequest.flags.testFlag(QIODeviceBase::Truncate)) {
        FILE_END_OF_FILE_INFO info;
        memset(&info, 0, sizeof(info));
        SetFileInformationByHandle(h, FileEndOfFileInfo, &info, sizeof(info));
    }
    return h;
}

bool QIORing::supportsOperation(Operation op)
{
    switch (op) {
    case QtPrivate::Operation::Open:
    case QtPrivate::Operation::Close:
    case QtPrivate::Operation::Read:
    case QtPrivate::Operation::Write:
    case QtPrivate::Operation::Flush:
    case QtPrivate::Operation::Cancel:
    case QtPrivate::Operation::Stat:
    case QtPrivate::Operation::VectoredRead:
    case QtPrivate::Operation::VectoredWrite:
        return true;
    case QtPrivate::Operation::NumOperations:
        return false;
    }
    return false; // Not unreachable, we could allow more for io_uring
}

void QIORing::submitRequests()
{
    stagePending = false;
    if (unstagedRequests == 0)
        return;

    // We perform a miniscule wait - to see if anything already in the queue is already completed -
    // if we have been told the queue is full. Then we can try queuing more things right away
    const bool shouldTryWait = std::exchange(queueWasFull, false);
    const auto submitToRing = [this, &shouldTryWait] {
        quint32 submittedEntries = 0;
        HRESULT hr = apiTable->SubmitIoRing(ioRingHandle, shouldTryWait ? 1 : 0, 1,
                                            &submittedEntries);
        qCDebug(lcQIORing) << "Submitted" << submittedEntries << "requests";
        unstagedRequests -= submittedEntries;
        if (FAILED(hr)) {
            // Too noisy, not a real problem
            // qErrnoWarning(hr, "Failed to submit QIORing request: %u", submittedEntries);
            return false;
        }
        return submittedEntries > 0;
    };
    if (submitToRing() && shouldTryWait) {
        // We try to prepare some more request and submit more if able
        prepareRequests();
        if (unstagedRequests > 0)
            submitToRing();
    }
}

void QIORing::prepareRequests()
{
    if (!lastUnqueuedIterator)
        return;
    Q_ASSERT(!preparingRequests);
    QScopedValueRollback<bool> prepareGuard(preparingRequests, true);

    auto it = *lastUnqueuedIterator;
    lastUnqueuedIterator.reset();
    const auto end = pendingRequests.end();
    while (!queueWasFull && it != end) {
        auto &request = *it;
        switch (prepareRequest(request)) {
        case RequestPrepResult::Ok:
            ++unstagedRequests;
            ++inFlightRequests;
            break;
        case RequestPrepResult::QueueFull:
            qCDebug(lcQIORing) << "Queue was reported as full, in flight requests:"
                               << inFlightRequests << "submission queue size:" << sqEntries
                               << "completion queue size:" << cqEntries;
            queueWasFull = true;
            lastUnqueuedIterator = it;
            return;
        case RequestPrepResult::Defer:
            qCDebug(lcQIORing) << "Request for" << request.operation()
                               << "had to be deferred, will not queue any more requests at the "
                                  "moment.";
            lastUnqueuedIterator = it;
            return; //
        case RequestPrepResult::RequestCompleted:
            // Used for requests that immediately finish. So we erase it:
            qCDebug(lcQIORing) << "Request for" << request.operation()
                               << "completed synchronously.";
            addrItMap.remove(&request);
            it = pendingRequests.erase(it);
            continue; // Don't increment iterator again
        }
        ++it;
    }
    if (it != pendingRequests.end())
        lastUnqueuedIterator = it;
}

namespace QtPrivate {
template <typename T>
using DetectHasFd = decltype(std::declval<const T &>().fd);

template <typename T>
constexpr bool OperationHasFd_v = qxp::is_detected_v<DetectHasFd, T>;
} // namespace QtPrivate

auto QIORing::prepareRequest(GenericRequestType &request) -> RequestPrepResult
{
    qCDebug(lcQIORing) << "Preparing a request with operation" << request.operation();
    HRESULT hr = -1;

    if (!verifyFd(request)) {
        finishRequestWithError(request, QFileDevice::OpenError);
        return RequestPrepResult::RequestCompleted;
    }

    switch (request.operation()) {
    case Operation::Open: {
        QIORingRequest<Operation::Open> openRequest = request.takeRequestData<Operation::Open>();
        HANDLE fileDescriptor = openFile(openRequest);
        if (fileDescriptor == INVALID_HANDLE_VALUE) {
            openRequest.result.emplace<QFileDevice::FileError>(QFileDevice::FileError::OpenError);
        } else {
            auto &result = openRequest.result.emplace<QIORingResult<Operation::Open>>();
            result.fd = qintptr(fileDescriptor);
        }
        invokeCallback(openRequest);
        return RequestPrepResult::RequestCompleted;
    }
    case Operation::Close: {
        if (ongoingSplitOperations > 0)
            return RequestPrepResult::Defer;

        // We need to wait until all previous OPS are done before we close the request.
        // There is no no-op request in the Windows QIORing, so we issue a flush.
        auto *closeRequest = request.requestData<Operation::Close>();
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        const IORING_HANDLE_REF fileRef(HANDLE(closeRequest->fd));
        hr = apiTable->BuildIoRingFlushFile(ioRingHandle, fileRef, FILE_FLUSH_MIN_METADATA,
                                            quintptr(std::addressof(request)),
                                            IOSQE_FLAGS_DRAIN_PRECEDING_OPS);
        break;
    }
    case Operation::Read: {
        auto *readRequest = request.requestData<Operation::Read>();
        auto span = readRequest->destination;
        auto offset = readRequest->offset;
        if (span.size() > MaxReadWriteLen) {
            qCDebug(lcQIORing) << "Requested Read of size" << span.size() << "has to be split";
            auto *extra = request.getOrInitializeExtra<QtPrivate::ReadWriteExtra>();
            if (extra->spanOffset == 0)
                ++ongoingSplitOperations;
            const qsizetype remaining = span.size() - extra->spanOffset;
            span.slice(extra->spanOffset, std::min(remaining, MaxReadWriteLen));
            offset += extra->totalProcessed;
        }
        hr = buildReadOperation(ioRingHandle, readRequest->fd, span, offset,
                                quintptr(std::addressof(request)));
        break;
    }
    case Operation::VectoredRead: {
        auto *vectoredReadRequest = request.requestData<Operation::VectoredRead>();
        auto span = vectoredReadRequest->destinations.front();
        auto offset = vectoredReadRequest->offset;
        if (Q_LIKELY(vectoredReadRequest->destinations.size() > 1
                     || span.size() > MaxReadWriteLen)) {
            auto *extra = request.getOrInitializeExtra<QtPrivate::ReadWriteExtra>();
            if (extra->spanOffset == 0 && extra->spanIndex == 0)
                ++ongoingSplitOperations;
            extra->numSpans = vectoredReadRequest->destinations.size();

            span = vectoredReadRequest->destinations[extra->spanIndex];

            const qsizetype remaining = span.size() - extra->spanOffset;
            span.slice(extra->spanOffset, std::min(remaining, MaxReadWriteLen));
            offset += extra->totalProcessed;
        }
        hr = buildReadOperation(ioRingHandle, vectoredReadRequest->fd, span,
                                offset, quintptr(std::addressof(request)));
        break;
    }
    case Operation::Write: {
        auto *writeRequest = request.requestData<Operation::Write>();
        auto span = writeRequest->source;
        auto offset = writeRequest->offset;
        if (span.size() > MaxReadWriteLen) {
            qCDebug(lcQIORing) << "Requested Write of size" << span.size() << "has to be split";
            auto *extra = request.getOrInitializeExtra<QtPrivate::ReadWriteExtra>();
            if (extra->spanOffset == 0)
                ++ongoingSplitOperations;
            const qsizetype remaining = span.size() - extra->spanOffset;
            span.slice(extra->spanOffset, std::min(remaining, MaxReadWriteLen));
            offset += extra->totalProcessed;
        }
        hr = buildWriteOperation(ioRingHandle, writeRequest->fd, span, offset,
                                 quintptr(std::addressof(request)));
        break;
    }
    case Operation::VectoredWrite: {
        auto *vectoredWriteRequest = request.requestData<Operation::VectoredWrite>();
        auto span = vectoredWriteRequest->sources.front();
        auto offset = vectoredWriteRequest->offset;
        if (Q_LIKELY(vectoredWriteRequest->sources.size() > 1
                     || span.size() > MaxReadWriteLen)) {
            auto *extra = request.getOrInitializeExtra<QtPrivate::ReadWriteExtra>();
            if (extra->spanOffset == 0 && extra->spanIndex == 0)
                ++ongoingSplitOperations;
            extra->numSpans = vectoredWriteRequest->sources.size();

            span = vectoredWriteRequest->sources[extra->spanIndex];

            const qsizetype remaining = span.size() - extra->spanOffset;
            span.slice(extra->spanOffset, std::min(remaining, MaxReadWriteLen));
            offset += extra->totalProcessed;
        }
        hr = buildWriteOperation(ioRingHandle, vectoredWriteRequest->fd, span,
                                 offset, quintptr(std::addressof(request)));
        break;
    }
    case Operation::Flush: {
        if (ongoingSplitOperations > 0)
            return RequestPrepResult::Defer;
        auto *flushRequest = request.requestData<Operation::Flush>();
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        const IORING_HANDLE_REF fileRef(HANDLE(flushRequest->fd));
        hr = apiTable->BuildIoRingFlushFile(ioRingHandle, fileRef, FILE_FLUSH_DEFAULT,
                                            quintptr(std::addressof(request)),
                                            IOSQE_FLAGS_DRAIN_PRECEDING_OPS);
        break;
    }
    case QtPrivate::Operation::Stat: {
        auto statRequest = request.takeRequestData<Operation::Stat>();
        FILE_STANDARD_INFO info;
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        if (!GetFileInformationByHandleEx(HANDLE(statRequest.fd), FileStandardInfo, &info,
                                          sizeof(info))) {
            DWORD winErr = GetLastError();
            QFileDevice::FileError error = QFileDevice::UnspecifiedError;
            if (winErr == ERROR_FILE_NOT_FOUND || winErr == ERROR_INVALID_HANDLE)
                error = QFileDevice::OpenError;
            else if (winErr == ERROR_ACCESS_DENIED)
                error = QFileDevice::PermissionsError;
            statRequest.result.emplace<QFileDevice::FileError>(error);
        } else {
            auto &result = statRequest.result.emplace<QIORingResult<Operation::Stat>>();
            result.size = info.EndOfFile.QuadPart;
        }
        invokeCallback(statRequest);
        return RequestPrepResult::RequestCompleted;
    }
    case Operation::Cancel: {
        auto *cancelRequest = request.requestData<Operation::Cancel>();
        auto *otherOperation = reinterpret_cast<GenericRequestType *>(cancelRequest->handle);
        if (!otherOperation || !addrItMap.contains(otherOperation)) {
            qCDebug(lcQIORing, "Invalid cancel for non-existant operation");
            invokeCallback(*cancelRequest);
            return RequestPrepResult::RequestCompleted;
        }
        qCDebug(lcQIORing) << "Cancelling operation of type" << otherOperation->operation()
                           << "which was"
                           << (otherOperation->wasQueued() ? "queued" : "not queued");
        Q_ASSERT(&request != otherOperation);
        if (!otherOperation->wasQueued()) {
            // The request hasn't been queued yet, so we can just drop it from
            // the pending requests and call the callback.
            auto it = addrItMap.take(otherOperation);
            finishRequestWithError(*otherOperation, QFileDevice::AbortError);
            pendingRequests.erase(it); // otherOperation is deleted
            invokeCallback(*cancelRequest);
            return RequestPrepResult::RequestCompleted;
        }
        qintptr fd = -1;
        invokeOnOp(*otherOperation, [&fd](auto *request) {
            if constexpr (QtPrivate::OperationHasFd_v<decltype(*request)>)
                fd = request->fd;
        });
        if (fd == -1) {
            qCDebug(lcQIORing, "Invalid cancel for non-existant fd");
            invokeCallback(*cancelRequest);
            return RequestPrepResult::RequestCompleted;
        }
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        const IORING_HANDLE_REF fileRef((HANDLE(fd)));
        hr = apiTable->BuildIoRingCancelRequest(ioRingHandle, fileRef, quintptr(otherOperation),
                                                quintptr(std::addressof(request)));
        break;
    }
    case Operation::NumOperations:
        Q_UNREACHABLE_RETURN(RequestPrepResult::RequestCompleted);
        break;
    }
    if (hr == IORING_E_SUBMISSION_QUEUE_FULL)
        return RequestPrepResult::QueueFull;
    if (FAILED(hr)) {
        finishRequestWithError(request, QFileDevice::UnspecifiedError);
        return RequestPrepResult::RequestCompleted;
    }
    request.setQueued(true);
    return RequestPrepResult::Ok;
}

bool QIORing::verifyFd(GenericRequestType &req)
{
    bool result = true;
    invokeOnOp(req, [&](auto *request) {
        if constexpr (QtPrivate::OperationHasFd_v<decltype(*request)>) {
            result = quintptr(request->fd) > 0 && quintptr(request->fd) != quintptr(INVALID_HANDLE_VALUE);
        }
    });
    return result;
}

void QIORing::GenericRequestType::cleanupExtra(Operation op, void *extra)
{
    switch (op) {
    case QtPrivate::Operation::Read:
    case QtPrivate::Operation::VectoredRead:
    case QtPrivate::Operation::Write:
    case QtPrivate::Operation::VectoredWrite:
        delete static_cast<QtPrivate::ReadWriteExtra *>(extra);
        break;
    case QtPrivate::Operation::Open:
    case QtPrivate::Operation::Close:
    case QtPrivate::Operation::Flush:
    case QtPrivate::Operation::Stat:
    case QtPrivate::Operation::Cancel:
    case QtPrivate::Operation::NumOperations:
        break;
    }
}

QT_END_NAMESPACE
