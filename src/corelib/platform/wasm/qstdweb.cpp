// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qstdweb_p.h"
#include "qwasmsuspendresumecontrol_p.h"
#include "qwasmglobal_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qmimedata.h>

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/html5.h>
#include <emscripten/threading.h>

#include <cstdint>
#include <iostream>

#include <unordered_map>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;
using emscripten::val;

namespace qstdweb {

static void usePotentialyUnusedSymbols()
{
    // Using this adds a reference on JSEvents and specialHTMLTargets which are always exported.
    // This hack is needed as it is currently impossible to specify a dollar sign in
    // target_link_options. The following is impossible:
    // DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$JSEvents
    // TODO(mikolajboc): QTBUG-108444, review this when cmake gets fixed.
    // Volatile is to make this unoptimizable, so that the function is referenced, but is not
    // called at runtime.
    volatile bool doIt = false;
    if (doIt)
        emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 0, NULL);
}

Q_CONSTRUCTOR_FUNCTION(usePotentialyUnusedSymbols)
typedef double uint53_t; // see Number.MAX_SAFE_INTEGER
namespace {
// Reads file in chunks in order to avoid holding two copies in memory at the same time
struct ChunkedFileReader
{
public:
    static void read(File file, char *buffer, uint32_t offset, uint32_t end,
                     std::function<void()> onCompleted)
    {
        (new ChunkedFileReader(end, std::move(onCompleted), std::move(file)))
                ->readNextChunk(offset, buffer);
    }

private:
    ChunkedFileReader(uint32_t end, std::function<void()> onCompleted, File file)
        : end(end), onCompleted(std::move(onCompleted)), file(std::move(file))
    {
    }

    void readNextChunk(uint32_t chunkBegin, char *chunkBuffer)
    {
        // Copy current chunk from JS memory to Wasm memory
        qstdweb::ArrayBuffer result = fileReader.result();
        qstdweb::Uint8Array(result).copyTo(chunkBuffer);

        // Read next chunk if not at buffer end
        const uint32_t nextChunkBegin = std::min(chunkBegin + result.byteLength(), end);
        if (nextChunkBegin == end) {
            onCompleted();
            delete this;
            return;
        }
        char *nextChunkBuffer = chunkBuffer + result.byteLength();
        fileReader.onLoad([this, nextChunkBegin, nextChunkBuffer](emscripten::val) {
            readNextChunk(nextChunkBegin, nextChunkBuffer);
        });
        const uint32_t nextChunkEnd = std::min(nextChunkBegin + chunkSize, end);
        qstdweb::Blob blob = file.slice(nextChunkBegin, nextChunkEnd);
        fileReader.readAsArrayBuffer(blob);
    }

    static constexpr uint32_t chunkSize = 256 * 1024;

    qstdweb::FileReader fileReader;
    uint32_t end;
    std::function<void()> onCompleted;
    File file;
};

#if defined(QT_STATIC)

EM_JS(bool, jsHaveAsyncify, (), { return typeof Asyncify !== "undefined"; });
EM_JS(bool, jsHaveJspi, (),
      { return typeof Asyncify !== "undefined" && !!Asyncify.makeAsyncFunction && (!!WebAssembly.Function || !!WebAssembly.Suspending); });

#else

bool jsHaveAsyncify() { return false; }
bool jsHaveJspi() { return false; }

#endif
} // namespace

ArrayBuffer::ArrayBuffer(uint32_t size)
{
    m_arrayBuffer = emscripten::val::global("ArrayBuffer").new_(size);
}

ArrayBuffer::ArrayBuffer(const emscripten::val &arrayBuffer)
    :m_arrayBuffer(arrayBuffer)
{

}

uint32_t ArrayBuffer::byteLength() const
{
    if (m_arrayBuffer.isUndefined() || m_arrayBuffer.isNull())
        return 0;

    return m_arrayBuffer["byteLength"].as<uint32_t>();
}

ArrayBuffer ArrayBuffer::slice(uint32_t begin, uint32_t end) const
{
    return ArrayBuffer(m_arrayBuffer.call<emscripten::val>("slice", begin, end));
}

emscripten::val ArrayBuffer::val() const
{
    return m_arrayBuffer;
}

Blob::Blob(const emscripten::val &blob)
    :m_blob(blob)
{

}

Blob Blob::fromArrayBuffer(const ArrayBuffer &arrayBuffer)
{
    auto array = emscripten::val::array();
    array.call<void>("push", arrayBuffer.val());
    return Blob(emscripten::val::global("Blob").new_(array));
}

uint32_t Blob::size() const
{
    return m_blob["size"].as<uint32_t>();
}

Blob Blob::copyFrom(const char *buffer, uint32_t size, std::string mimeType)
{
    Uint8Array contentCopy = Uint8Array::copyFrom(buffer, size);

    emscripten::val contentArray = emscripten::val::array();
    contentArray.call<void>("push", contentCopy.val());
    emscripten::val type = emscripten::val::object();
    type.set("type", std::move(mimeType));
    return Blob(emscripten::val::global("Blob").new_(contentArray, type));
}

// Copies content from the given buffer into a Blob object
Blob Blob::copyFrom(const char *buffer, uint32_t size)
{
    return copyFrom(buffer, size, "application/octet-stream");
}

Blob Blob::slice(uint32_t begin, uint32_t end) const
{
    return Blob(m_blob.call<emscripten::val>("slice", begin, end));
}

ArrayBuffer Blob::arrayBuffer_sync() const
{
    QEventLoop loop;
    emscripten::val buffer;
    qstdweb::Promise::make(m_blob, QStringLiteral("arrayBuffer"), {
        .thenFunc = [&loop, &buffer](emscripten::val arrayBuffer) {
            buffer = arrayBuffer;
            loop.quit();
        }
    });
    loop.exec();
    return ArrayBuffer(buffer);
}

emscripten::val Blob::val() const
{
    return m_blob;
}

File::File(const emscripten::val &file)
:m_file(file)
{

}

File::~File() = default;

File::File(const File &other) = default;

File::File(File &&other) = default;

File &File::operator=(const File &other) = default;

File &File::operator=(File &&other) = default;


Blob File::slice(uint64_t begin, uint64_t end) const
{
    return Blob(m_file.call<emscripten::val>("slice", uint53_t(begin), uint53_t(end)));
}

std::string File::name() const
{
    return m_file["name"].as<std::string>();
}

uint64_t File::size() const
{
    return uint64_t(m_file["size"].as<uint53_t>());
}

std::string Blob::type() const
{
    return m_blob["type"].as<std::string>();
}

// Streams partial file content into the given buffer asynchronously. The completed
// callback is called on completion.
void File::stream(uint32_t offset, uint32_t length, char *buffer,
                  std::function<void()> completed) const
{
    ChunkedFileReader::read(*this, buffer, offset, offset + length, std::move(completed));
}

// Streams file content into the given buffer asynchronously. The completed
// callback is called on completion.
void File::stream(char *buffer, std::function<void()> completed) const
{
    stream(0, size(), buffer, std::move(completed));
}

std::string File::type() const
{
    return m_file["type"].as<std::string>();
}

emscripten::val File::val() const
{
    return m_file;
}

FileUrlRegistration::FileUrlRegistration(File file)
{
    m_path = QString::fromStdString(emscripten::val::global("window")["URL"].call<std::string>(
        "createObjectURL", file.file()));
}

FileUrlRegistration::~FileUrlRegistration()
{
    emscripten::val::global("window")["URL"].call<void>("revokeObjectURL",
                                                        emscripten::val(m_path.toStdString()));
}

FileUrlRegistration::FileUrlRegistration(FileUrlRegistration &&other) = default;

FileUrlRegistration &FileUrlRegistration::operator=(FileUrlRegistration &&other) = default;

FileList::FileList(const emscripten::val &fileList)
    :m_fileList(fileList)
{

}

int FileList::length() const
{
    return m_fileList["length"].as<int>();
}

File FileList::item(int index) const
{
    return File(m_fileList[index]);
}

File FileList::operator[](int index) const
{
    return item(index);
}

emscripten::val FileList::val() const
{
    return m_fileList;
}

ArrayBuffer FileReader::result() const
{
    return ArrayBuffer(m_fileReader["result"]);
}

void FileReader::readAsArrayBuffer(const Blob &blob) const
{
    m_fileReader.call<void>("readAsArrayBuffer", blob.m_blob);
}

void FileReader::onLoad(const std::function<void(emscripten::val)> &onLoad)
{
    m_onLoad.reset();
    m_onLoad = std::make_unique<EventCallback>(m_fileReader, "load", onLoad);
}

void FileReader::onError(const std::function<void(emscripten::val)> &onError)
{
    m_onError.reset();
    m_onError = std::make_unique<EventCallback>(m_fileReader, "error", onError);
}

void FileReader::onAbort(const std::function<void(emscripten::val)> &onAbort)
{
    m_onAbort.reset();
    m_onAbort = std::make_unique<EventCallback>(m_fileReader, "abort", onAbort);
}

emscripten::val FileReader::val() const
{
    return m_fileReader;
}

// Constructs a Uint8Array which references the given emscripten::val, which must contain a JS Unit8Array
Uint8Array::Uint8Array(const emscripten::val &uint8Array)
: m_uint8Array(uint8Array)
{

}

// Constructs a Uint8Array which references an ArrayBuffer
Uint8Array::Uint8Array(const ArrayBuffer &buffer)
: m_uint8Array(Uint8Array::constructor_().new_(buffer.m_arrayBuffer))
{

}

// Constructs a Uint8Array which references a view into an ArrayBuffer
Uint8Array::Uint8Array(const ArrayBuffer &buffer, uint32_t offset, uint32_t length)
: m_uint8Array(Uint8Array::constructor_().new_(buffer.m_arrayBuffer, offset, length))
{

}

// Constructs a Uint8Array which references an area on the heap.
Uint8Array::Uint8Array(const char *buffer, uint32_t size)
:m_uint8Array(emscripten::typed_memory_view(size, buffer))
{

}

// Constructs a Uint8Array which allocates and references a new ArrayBuffer with the given size.
Uint8Array::Uint8Array(uint32_t size)
: m_uint8Array(Uint8Array::constructor_().new_(size))
{

}

ArrayBuffer Uint8Array::buffer() const
{
    return ArrayBuffer(m_uint8Array["buffer"]);
}

uint32_t Uint8Array::length() const
{
    return m_uint8Array["length"].as<uint32_t>();
}

void Uint8Array::set(const Uint8Array &source)
{
    m_uint8Array.call<void>("set", source.m_uint8Array); // copies source content
}

Uint8Array Uint8Array::subarray(uint32_t begin, uint32_t end)
{
    // Note: using uint64_t here errors with "Cannot convert a BigInt value to a number"
    // (see JS BigInt and Number types). Use uint32_t for now.
    return Uint8Array(m_uint8Array.call<emscripten::val>("subarray", begin, end));
}

// Copies the Uint8Array content to a destination on the heap
void Uint8Array::copyTo(char *destination) const
{
    Uint8Array(destination, length()).set(*this);
}

// Copies the Uint8Array content to a destination QByteArray
QByteArray Uint8Array::copyToQByteArray() const
{
    if (length() > std::numeric_limits<qsizetype>::max())
        return QByteArray();

    QByteArray destinationArray;
    destinationArray.resize(length());
    copyTo(destinationArray.data());
    return destinationArray;
}

// Copies the Uint8Array content to a destination on the heap
void Uint8Array::copy(char *destination, const Uint8Array &source)
{
    Uint8Array(destination, source.length()).set(source);
}

// Copies content from a source on the heap to a new Uint8Array object
Uint8Array Uint8Array::copyFrom(const char *buffer, uint32_t size)
{
    Uint8Array contentCopy(size);
    contentCopy.set(Uint8Array(buffer, size));
    return contentCopy;
}

// Copies content from a QByteArray to a new Uint8Array object
Uint8Array Uint8Array::copyFrom(const QByteArray &buffer)
{
    return copyFrom(buffer.constData(), buffer.size());
}

emscripten::val Uint8Array::val() const
{
    return m_uint8Array;
}

emscripten::val Uint8Array::constructor_()
{
    return emscripten::val::global("Uint8Array");
}

EventCallback::EventCallback(emscripten::val element, const std::string &name,
                             const std::function<void(emscripten::val)> &fn)
  :QWasmEventHandler(element, name, fn)
{

}

void Promise::adoptPromise(emscripten::val promise, PromiseCallbacks callbacks)
{
    Q_ASSERT_X(!!callbacks.catchFunc || !!callbacks.finallyFunc || !!callbacks.thenFunc,
        "Promise::adoptPromise", "must provide at least one callback function");

    QWasmSuspendResumeControl *suspendResume = QWasmSuspendResumeControl::get();
    Q_ASSERT(suspendResume);

    // Registers a possibly-empty callback with suspendresumecontrol. Returns
    // the the handler index if there was a valid callback, or nullopt.
    auto registerCallback = [suspendResume](std::function<void(emscripten::val)> cb) -> std::optional<uint32_t>{
        if (!cb)
            return std::nullopt;
        return std::optional<uint32_t>{suspendResume->registerEventHandler(std::move(cb))};
    };

    // Register callbacks with suspendresumecontrol, so that it can
    // resume the wasm instance when the promise resolves. The finally
    // callback is sepecial, since we remove the event handlers there
    // as cleanup, including the event handler for the cleanup function
    // itself.
    std::optional<uint32_t> thenIndex = registerCallback(std::move(callbacks.thenFunc));
    std::optional<uint32_t> catchIndex = registerCallback(std::move(callbacks.catchFunc));
    std::shared_ptr<uint32_t> finallyIndex = std::make_shared<uint32_t>();;
    auto finallyFunc = callbacks.finallyFunc;

    // 'Finally' callback which performs clean-up and calls the user-provided finally.
    auto finally = [suspendResume, thenIndex, catchIndex, finallyIndex, finallyFunc](emscripten::val){

        // Clean up event handlers
        if (thenIndex)
            suspendResume->removeEventHandler(*thenIndex);
        if (catchIndex)
            suspendResume->removeEventHandler(*catchIndex);
        suspendResume->removeEventHandler(*finallyIndex);

        // Call user finally
        if (finallyFunc)
            finallyFunc();
    };

    *finallyIndex = suspendResume->registerEventHandler(std::move(finally));

    // Set handlers on the promise
    if (thenIndex)
        promise =
                promise.call<emscripten::val>("then", suspendResume->jsEventHandlerAt(*thenIndex));

    if (catchIndex)
        promise = promise.call<emscripten::val>("catch",
                                                suspendResume->jsEventHandlerAt(*catchIndex));

    promise = promise.call<emscripten::val>("finally",
                                            suspendResume->jsEventHandlerAt(*finallyIndex));
}

void Promise::all(std::vector<emscripten::val> promises, PromiseCallbacks callbacks)
{
    auto arr = emscripten::val::array(promises);
    auto all = val::global("Promise").call<emscripten::val>("all", arr);
    return adoptPromise(all, callbacks);
}

//  Asyncify and thread blocking: Normally, it's not possible to block the main
//  thread, except if asyncify is enabled. Secondary threads can always block.
//
//  haveAsyncify(): returns true if the main thread can block on QEventLoop::exec(),
//      if either asyncify 1 or 2 (JSPI) is available.
//
//  haveJspi(): returns true if asyncify 2 (JSPI) is available.
//
//  canBlockCallingThread(): returns true if the calling thread can block on
//      QEventLoop::exec(), using either asyncify or as a seconarday thread.
bool haveJspi()
{
    static bool HaveJspi = jsHaveJspi();
    return HaveJspi;
}

bool haveAsyncify()
{
    static bool HaveAsyncify = jsHaveAsyncify() || haveJspi();
    return HaveAsyncify;
}

bool canBlockCallingThread()
{
    return haveAsyncify() || !emscripten_is_main_runtime_thread();
}

BlobIODevice::BlobIODevice(Blob blob)
    : m_blob(blob)
{

}

bool BlobIODevice::open(QIODevice::OpenMode mode)
{
    if (mode.testFlag(QIODevice::WriteOnly))
        return false;
    return QIODevice::open(mode);
}

bool BlobIODevice::isSequential() const
{
    return false;
}

qint64 BlobIODevice::size() const
{
    return m_blob.size();
}

bool BlobIODevice::seek(qint64 pos)
{
    if (pos >= size())
        return false;
    return QIODevice::seek(pos);
}

qint64 BlobIODevice::readData(char *data, qint64 maxSize)
{
    uint64_t begin = QIODevice::pos();
    uint64_t end = std::min<uint64_t>(begin + maxSize, size());
    uint64_t size = end - begin;
    if (size > 0) {
        qstdweb::ArrayBuffer buffer = m_blob.slice(begin, end).arrayBuffer_sync();
        qstdweb::Uint8Array(buffer).copyTo(data);
    }
    return size;
}

qint64 BlobIODevice::writeData(const char *, qint64)
{
    Q_UNREACHABLE();
}

Uint8ArrayIODevice::Uint8ArrayIODevice(Uint8Array array)
    : m_array(array)
{

}

bool Uint8ArrayIODevice::open(QIODevice::OpenMode mode)
{
    return QIODevice::open(mode);
}

bool Uint8ArrayIODevice::isSequential() const
{
    return false;
}

qint64 Uint8ArrayIODevice::size() const
{
    return m_array.length();
}

bool Uint8ArrayIODevice::seek(qint64 pos)
{
    if (pos >= size())
        return false;
    return QIODevice::seek(pos);
}

qint64 Uint8ArrayIODevice::readData(char *data, qint64 maxSize)
{
    uint64_t begin = QIODevice::pos();
    uint64_t end = std::min<uint64_t>(begin + maxSize, size());
    uint64_t size = end - begin;
    if (size > 0)
        m_array.subarray(begin, end).copyTo(data);
    return size;
}

qint64 Uint8ArrayIODevice::writeData(const char *data, qint64 maxSize)
{
    uint64_t begin = QIODevice::pos();
    uint64_t end = std::min<uint64_t>(begin + maxSize, size());
    uint64_t size = end - begin;
    if (size > 0)
        m_array.subarray(begin, end).set(Uint8Array(data, size));
    return size;
}

} // namespace qstdweb

QT_END_NAMESPACE
