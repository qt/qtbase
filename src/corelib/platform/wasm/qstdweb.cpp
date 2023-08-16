// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qstdweb_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qmimedata.h>

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <cstdint>
#include <iostream>

#include <unordered_map>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

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
        emscripten_set_wheel_callback(NULL, 0, 0, NULL);
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

enum class CallbackType {
        Then,
        Catch,
        Finally,
};

void validateCallbacks(const PromiseCallbacks& callbacks) {
    Q_ASSERT(!!callbacks.catchFunc || !!callbacks.finallyFunc || !!callbacks.thenFunc);
}

using ThunkId = int;

#define THUNK_NAME(type, i) callbackThunk##type##i

// A resource pool for exported promise thunk functions. ThunkPool::poolSize sets of
// 3 promise thunks (then, catch, finally) are exported and can be used by promises
// in C++. To allocate a thunk, call allocateThunk. When a thunk is ready for use,
// a callback with allocation RAII object ThunkAllocation will be returned. Deleting
// the object frees the thunk and automatically makes any pending allocateThunk call
// run its callback with a free thunk slot.
class ThunkPool {
public:
    static constexpr size_t poolSize = 4;

    // An allocation for a thunk function set. Following the RAII pattern, destruction of
    // this objects frees a corresponding thunk pool entry.
    // To actually make the thunks react to a js promise's callbacks, call bindToPromise.
    class ThunkAllocation {
    public:
        ThunkAllocation(int thunkId, ThunkPool* pool) : m_thunkId(thunkId), m_pool(pool) {}
        ~ThunkAllocation() {
            m_pool->free(m_thunkId);
        }

        // The id of the underlaying thunk set
        int id() const { return m_thunkId; }

        // Binds the corresponding thunk set to the js promise 'target'.
        void bindToPromise(emscripten::val target, const PromiseCallbacks& callbacks) {
            using namespace emscripten;

            if (Q_LIKELY(callbacks.thenFunc)) {
                target = target.call<val>(
                    "then",
                    emscripten::val::module_property(thunkName(CallbackType::Then, id()).data()));
            }
            if (callbacks.catchFunc) {
                target = target.call<val>(
                    "catch",
                    emscripten::val::module_property(thunkName(CallbackType::Catch, id()).data()));
            }
            if (callbacks.finallyFunc) {
                target = target.call<val>(
                    "finally",
                    emscripten::val::module_property(thunkName(CallbackType::Finally, id()).data()));
            }
        }

    private:
        int m_thunkId;
        ThunkPool* m_pool;
    };

    ThunkPool() {
        std::iota(m_free.begin(), m_free.end(), 0);
    }

    void setThunkCallback(std::function<void(int, CallbackType, emscripten::val)> callback) {
        m_callback = std::move(callback);
    }

    void allocateThunk(std::function<void(std::unique_ptr<ThunkAllocation>)> onAllocated) {
        if (m_free.empty()) {
            m_pendingAllocations.push_back(std::move(onAllocated));
            return;
        }

        const int thunkId = m_free.back();
        m_free.pop_back();
        onAllocated(std::make_unique<ThunkAllocation>(thunkId, this));
    }

    static QByteArray thunkName(CallbackType type, size_t i) {
        return QStringLiteral("promiseCallback%1%2").arg([type]() -> QString {
            switch (type) {
                case CallbackType::Then:
                    return QStringLiteral("Then");
                case CallbackType::Catch:
                    return QStringLiteral("Catch");
                case CallbackType::Finally:
                    return QStringLiteral("Finally");
            }
        }()).arg(i).toLatin1();
    }

    static ThunkPool* get();

#define THUNK(i) \
    static void THUNK_NAME(Then, i)(emscripten::val result) \
    { \
        get()->onThunkCalled(i, CallbackType::Then, std::move(result)); \
    } \
    static void THUNK_NAME(Catch, i)(emscripten::val result) \
    { \
        get()->onThunkCalled(i, CallbackType::Catch, std::move(result)); \
    } \
    static void THUNK_NAME(Finally, i)() \
    { \
        get()->onThunkCalled(i, CallbackType::Finally, emscripten::val::undefined()); \
    }

    THUNK(0);
    THUNK(1);
    THUNK(2);
    THUNK(3);

#undef THUNK

private:
    void onThunkCalled(int index, CallbackType type, emscripten::val result) {
        m_callback(index, type, std::move(result));
    }

    void free(int thunkId) {
        if (m_pendingAllocations.empty()) {
            // Return the thunk to the free pool
            m_free.push_back(thunkId);
            return;
        }

        // Take the next enqueued allocation and reuse the thunk
        auto allocation = m_pendingAllocations.back();
        m_pendingAllocations.pop_back();
        allocation(std::make_unique<ThunkAllocation>(thunkId, this));
    }

    std::function<void(int, CallbackType, emscripten::val)> m_callback;

    std::vector<int> m_free = std::vector<int>(poolSize);
    std::vector<std::function<void(std::unique_ptr<ThunkAllocation>)>> m_pendingAllocations;
};

Q_GLOBAL_STATIC(ThunkPool, g_thunkPool)

ThunkPool* ThunkPool::get()
{
    return g_thunkPool;
}

#define CALLBACK_BINDING(i) \
    emscripten::function(ThunkPool::thunkName(CallbackType::Then, i).data(), \
                         &ThunkPool::THUNK_NAME(Then, i)); \
    emscripten::function(ThunkPool::thunkName(CallbackType::Catch, i).data(), \
                         &ThunkPool::THUNK_NAME(Catch, i)); \
    emscripten::function(ThunkPool::thunkName(CallbackType::Finally, i).data(), \
                         &ThunkPool::THUNK_NAME(Finally, i));

EMSCRIPTEN_BINDINGS(qtThunkPool) {
    CALLBACK_BINDING(0)
    CALLBACK_BINDING(1)
    CALLBACK_BINDING(2)
    CALLBACK_BINDING(3)
}

#undef CALLBACK_BINDING
#undef THUNK_NAME

class WebPromiseManager
{
public:
    WebPromiseManager();
    ~WebPromiseManager();

    WebPromiseManager(const WebPromiseManager& other) = delete;
    WebPromiseManager(WebPromiseManager&& other) = delete;
    WebPromiseManager& operator=(const WebPromiseManager& other) = delete;
    WebPromiseManager& operator=(WebPromiseManager&& other) = delete;

    void adoptPromise(emscripten::val target, PromiseCallbacks callbacks);

    static WebPromiseManager* get();

private:
    struct RegistryEntry {
        PromiseCallbacks callbacks;
        std::unique_ptr<ThunkPool::ThunkAllocation> allocation;
    };

    static std::optional<CallbackType> parseCallbackType(emscripten::val callbackType);

    void subscribeToJsPromiseCallbacks(int i, const PromiseCallbacks& callbacks, emscripten::val jsContextfulPromise);
    void promiseThunkCallback(int i, CallbackType type, emscripten::val result);

    void registerPromise(std::unique_ptr<ThunkPool::ThunkAllocation> allocation, PromiseCallbacks promise);
    void unregisterPromise(ThunkId context);

    std::array<RegistryEntry, ThunkPool::poolSize> m_promiseRegistry;
};

Q_GLOBAL_STATIC(WebPromiseManager, webPromiseManager)

WebPromiseManager::WebPromiseManager()
{
    ThunkPool::get()->setThunkCallback(std::bind(
        &WebPromiseManager::promiseThunkCallback, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

std::optional<CallbackType>
WebPromiseManager::parseCallbackType(emscripten::val callbackType)
{
    if (!callbackType.isString())
        return std::nullopt;

    const std::string data = callbackType.as<std::string>();
    if (data == "then")
        return CallbackType::Then;
    if (data == "catch")
        return CallbackType::Catch;
    if (data == "finally")
        return CallbackType::Finally;
    return std::nullopt;
}

WebPromiseManager::~WebPromiseManager() = default;

WebPromiseManager *WebPromiseManager::get()
{
    return webPromiseManager();
}

void WebPromiseManager::promiseThunkCallback(int context, CallbackType type, emscripten::val result)
{
    auto* promiseState = &m_promiseRegistry[context];

    auto* callbacks = &promiseState->callbacks;
    bool expectingOtherCallbacks;
    switch (type) {
        case CallbackType::Then:
            callbacks->thenFunc(result);
            // At this point, if there is no finally function, we are sure that the Catch callback won't be issued.
            expectingOtherCallbacks = !!callbacks->finallyFunc;
            break;
        case CallbackType::Catch:
            callbacks->catchFunc(result);
            expectingOtherCallbacks = !!callbacks->finallyFunc;
            break;
        case CallbackType::Finally:
            callbacks->finallyFunc();
            expectingOtherCallbacks = false;
            break;
    }

    if (!expectingOtherCallbacks)
        unregisterPromise(context);
}

void WebPromiseManager::registerPromise(
    std::unique_ptr<ThunkPool::ThunkAllocation> allocation,
    PromiseCallbacks callbacks)
{
    const ThunkId id = allocation->id();
    m_promiseRegistry[id] =
        RegistryEntry {std::move(callbacks), std::move(allocation)};
}

void WebPromiseManager::unregisterPromise(ThunkId context)
{
    m_promiseRegistry[context] = {};
}

void WebPromiseManager::adoptPromise(emscripten::val target, PromiseCallbacks callbacks) {
    ThunkPool::get()->allocateThunk([=](std::unique_ptr<ThunkPool::ThunkAllocation> allocation) {
        allocation->bindToPromise(std::move(target), callbacks);
        registerPromise(std::move(allocation), std::move(callbacks));
    });
}
#if defined(QT_STATIC)

EM_JS(bool, jsHaveAsyncify, (), { return typeof Asyncify !== "undefined"; });

#else

bool jsHaveAsyncify() { return false; }

#endif

struct DataTransferReader
{
public:
    using DoneCallback = std::function<void(std::unique_ptr<QMimeData>)>;

    static std::shared_ptr<CancellationFlag> read(emscripten::val webDataTransfer,
                                                  std::function<QVariant(QByteArray)> imageReader,
                                                  DoneCallback onCompleted)
    {
        auto cancellationFlag = std::make_shared<CancellationFlag>();
        (new DataTransferReader(std::move(onCompleted), std::move(imageReader), cancellationFlag))
                ->read(webDataTransfer);
        return cancellationFlag;
    }

    ~DataTransferReader() = default;

private:
    DataTransferReader(DoneCallback onCompleted, std::function<QVariant(QByteArray)> imageReader,
                       std::shared_ptr<CancellationFlag> cancellationFlag)
        : mimeData(std::make_unique<QMimeData>()),
          imageReader(std::move(imageReader)),
          onCompleted(std::move(onCompleted)),
          cancellationFlag(cancellationFlag)
    {
    }

    void read(emscripten::val webDataTransfer)
    {
        enum class ItemKind {
            File,
            String,
        };

        const auto items = webDataTransfer["items"];
        for (int i = 0; i < items["length"].as<int>(); ++i) {
            const auto item = items[i];
            const auto itemKind =
                    item["kind"].as<std::string>() == "string" ? ItemKind::String : ItemKind::File;
            const auto itemMimeType = QString::fromStdString(item["type"].as<std::string>());

            switch (itemKind) {
            case ItemKind::File: {
                ++fileCount;

                qstdweb::File file(item.call<emscripten::val>("getAsFile"));

                QByteArray fileContent(file.size(), Qt::Uninitialized);
                file.stream(fileContent.data(), [this, itemMimeType, fileContent]() {
                    if (!fileContent.isEmpty()) {
                        if (itemMimeType.startsWith("image/"_L1)) {
                            mimeData->setImageData(imageReader(fileContent));
                        } else {
                            mimeData->setData(itemMimeType, fileContent.data());
                        }
                    }
                    ++doneCount;
                    onFileRead();
                });
                break;
            }
            case ItemKind::String:
                if (itemMimeType.contains("STRING"_L1, Qt::CaseSensitive)
                    || itemMimeType.contains("TEXT"_L1, Qt::CaseSensitive)) {
                    break;
                }
                QString a;
                const QString data = QString::fromEcmaString(webDataTransfer.call<emscripten::val>(
                        "getData", emscripten::val(itemMimeType.toStdString())));

                if (!data.isEmpty()) {
                    if (itemMimeType == "text/html"_L1)
                        mimeData->setHtml(data);
                    else if (itemMimeType.isEmpty() || itemMimeType == "text/plain"_L1)
                        mimeData->setText(data); // the type can be empty
                    else
                        mimeData->setData(itemMimeType, data.toLocal8Bit());
                }
                break;
            }
        }

        onFileRead();
    }

    void onFileRead()
    {
        Q_ASSERT(doneCount <= fileCount);
        if (doneCount < fileCount)
            return;

        std::unique_ptr<DataTransferReader> deleteThisLater(this);
        if (!cancellationFlag.expired())
            onCompleted(std::move(mimeData));
    }

    int fileCount = 0;
    int doneCount = 0;
    std::unique_ptr<QMimeData> mimeData;
    std::function<QVariant(QByteArray)> imageReader;
    DoneCallback onCompleted;

    std::weak_ptr<CancellationFlag> cancellationFlag;
};

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

emscripten::val ArrayBuffer::val()
{
    return m_arrayBuffer;
}

Blob::Blob(const emscripten::val &blob)
    :m_blob(blob)
{

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

emscripten::val Blob::val()
{
    return m_blob;
}

File::File(const emscripten::val &file)
:m_file(file)
{

}

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

emscripten::val File::val()
{
    return m_file;
}

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

emscripten::val FileReader::val()
{
    return m_fileReader;
}

Uint8Array Uint8Array::heap()
{
    return Uint8Array(heap_());
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
:m_uint8Array(Uint8Array::constructor_().new_(Uint8Array::heap().buffer().m_arrayBuffer, uintptr_t(buffer), size))
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

emscripten::val Uint8Array::val()
{
    return m_uint8Array;
}

emscripten::val Uint8Array::heap_()
{
    return emscripten::val::module_property("HEAPU8");
}

emscripten::val Uint8Array::constructor_()
{
    return emscripten::val::global("Uint8Array");
}

// Registers a callback function for a named event on the given element. The event
// name must be the name as returned by the Event.type property: e.g. "load", "error".
EventCallback::~EventCallback()
{
    // Clean up if this instance's callback is still installed on the element
    if (m_element[contextPropertyName(m_eventName).c_str()].as<intptr_t>() == intptr_t(this)) {
        m_element.set(contextPropertyName(m_eventName).c_str(), emscripten::val::undefined());
        m_element.set((std::string("on") + m_eventName).c_str(), emscripten::val::undefined());
    }
}

EventCallback::EventCallback(emscripten::val element, const std::string &name, const std::function<void(emscripten::val)> &fn)
    :m_element(element)
    ,m_eventName(name)
    ,m_fn(fn)
{
    Q_ASSERT_X(m_element[contextPropertyName(m_eventName)].isUndefined(), Q_FUNC_INFO,
               "Only one event callback of type currently supported with EventCallback");
    m_element.set(contextPropertyName(m_eventName).c_str(), emscripten::val(intptr_t(this)));
    m_element.set((std::string("on") + m_eventName).c_str(), emscripten::val::module_property("qtStdWebEventCallbackActivate"));
}

void EventCallback::activate(emscripten::val event)
{
    emscripten::val target = event["currentTarget"];
    std::string eventName = event["type"].as<std::string>();
    emscripten::val property = target[contextPropertyName(eventName)];
    // This might happen when the event bubbles
    if (property.isUndefined())
        return;
    EventCallback *that = reinterpret_cast<EventCallback *>(property.as<intptr_t>());
    that->m_fn(event);
}

std::string EventCallback::contextPropertyName(const std::string &eventName)
{
    return std::string("data-qtEventCallbackContext") + eventName;
}

EMSCRIPTEN_BINDINGS(qtStdwebCalback) {
    emscripten::function("qtStdWebEventCallbackActivate", &EventCallback::activate);
}

namespace Promise {
    void adoptPromise(emscripten::val promiseObject, PromiseCallbacks callbacks) {
        validateCallbacks(callbacks);

        WebPromiseManager::get()->adoptPromise(
            std::move(promiseObject), std::move(callbacks));
    }

    void all(std::vector<emscripten::val> promises, PromiseCallbacks callbacks) {
        struct State {
            std::map<int, emscripten::val> results;
            int remainingThenCallbacks;
            int remainingFinallyCallbacks;
        };

        validateCallbacks(callbacks);

        auto state = std::make_shared<State>();
        state->remainingThenCallbacks = state->remainingFinallyCallbacks = promises.size();

        for (size_t i = 0; i < promises.size(); ++i) {
            PromiseCallbacks individualPromiseCallback;
            if (callbacks.thenFunc) {
                individualPromiseCallback.thenFunc = [i, state, callbacks](emscripten::val partialResult) mutable {
                    state->results.emplace(i, std::move(partialResult));
                    if (!--(state->remainingThenCallbacks)) {
                        std::vector<emscripten::val> transformed;
                        for (auto& data : state->results) {
                            transformed.push_back(std::move(data.second));
                        }
                        callbacks.thenFunc(emscripten::val::array(std::move(transformed)));
                    }
                };
            }
            if (callbacks.catchFunc) {
                individualPromiseCallback.catchFunc = [state, callbacks](emscripten::val error) mutable {
                    callbacks.catchFunc(error);
                };
            }
            individualPromiseCallback.finallyFunc = [state, callbacks]() mutable {
                if (!--(state->remainingFinallyCallbacks)) {
                    if (callbacks.finallyFunc)
                        callbacks.finallyFunc();
                    // Explicitly reset here for verbosity, this would have been done automatically with the
                    // destruction of the adopted promise in WebPromiseManager.
                    state.reset();
                }
            };

            adoptPromise(std::move(promises.at(i)), std::move(individualPromiseCallback));
        }
    }
}

bool haveAsyncify()
{
    static bool HaveAsyncify = jsHaveAsyncify();
    return HaveAsyncify;
}

std::shared_ptr<CancellationFlag>
readDataTransfer(emscripten::val webDataTransfer, std::function<QVariant(QByteArray)> imageReader,
                 std::function<void(std::unique_ptr<QMimeData>)> onDone)
{
    return DataTransferReader::read(webDataTransfer, std::move(imageReader), std::move(onDone));
}

} // namespace qstdweb

QT_END_NAMESPACE
