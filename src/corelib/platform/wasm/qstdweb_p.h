// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTDWEB_P_H
#define QSTDWEB_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qglobal_p.h>
#include <QtCore/qglobal.h>
#include "QtCore/qhash.h"
#include "QtCore/qiodevice.h"

#include <emscripten/val.h>

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

#if QT_CONFIG(thread)
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#endif  // #if QT_CONFIG(thread)

QT_BEGIN_NAMESPACE

class QMimeData;

namespace qstdweb {
    extern const char makeContextfulPromiseFunctionName[];

    // DOM API in C++, implemented using emscripten val.h and bind.h.
    // This is private API and can be extended and changed as needed.
    // The API mirrors that of the native API, with some extensions
    // to ease usage from C++ code.

    class ArrayBuffer;
    class Blob;
    class File;
    class FileList;
    class FileReader;
    class Uint8Array;
    class EventCallback;

    class Q_CORE_EXPORT ArrayBuffer {
    public:
        explicit ArrayBuffer(uint32_t size);
        explicit ArrayBuffer(const emscripten::val &arrayBuffer);
        uint32_t byteLength() const;
        ArrayBuffer slice(uint32_t begin, uint32_t end) const;
        emscripten::val val() const;

    private:
        friend class Uint8Array;
        emscripten::val m_arrayBuffer = emscripten::val::undefined();
    };

    class Q_CORE_EXPORT Blob {
    public:
        explicit Blob(const emscripten::val &blob);
        static Blob fromArrayBuffer(const ArrayBuffer &arrayBuffer);
        uint32_t size() const;
        static Blob copyFrom(const char *buffer, uint32_t size, std::string mimeType);
        static Blob copyFrom(const char *buffer, uint32_t size);
        Blob slice(uint32_t begin, uint32_t end) const;
        ArrayBuffer arrayBuffer_sync() const;
        emscripten::val val() const;
        std::string type() const;

    private:
        friend class FileReader;
        emscripten::val m_blob = emscripten::val::undefined();
    };

    class Q_CORE_EXPORT File {
    public:
        File() = default;
        explicit File(const emscripten::val &file);
        ~File();

        File(const File &other);
        File(File &&other);
        File &operator=(const File &other);
        File &operator=(File &&other);

        Blob slice(uint64_t begin, uint64_t end) const;
        std::string name() const;
        uint64_t size() const;
        std::string type() const;
        void stream(uint32_t offset, uint32_t length, char *buffer,
                    std::function<void()> completed) const;
        void stream(char *buffer, std::function<void()> completed) const;
        emscripten::val val() const;
        void fileUrlRegistration() const;
        const QString &fileUrlPath() const { return m_urlPath; }
        emscripten::val file() const { return m_file; }

    private:
        emscripten::val m_file = emscripten::val::undefined();
        QString m_urlPath;
    };

    class Q_CORE_EXPORT FileUrlRegistration
    {
    public:
        explicit FileUrlRegistration(File file);
        ~FileUrlRegistration();

        FileUrlRegistration(const FileUrlRegistration &other) = delete;
        FileUrlRegistration(FileUrlRegistration &&other);
        FileUrlRegistration &operator=(const FileUrlRegistration &other) = delete;
        FileUrlRegistration &operator=(FileUrlRegistration &&other);

        const QString &path() const { return m_path; }

    private:
        QString m_path;
    };

    using FileUrlRegistrations = std::vector<std::unique_ptr<FileUrlRegistration>>;

    class Q_CORE_EXPORT FileList {
    public:
        FileList() = default;
        explicit FileList(const emscripten::val &fileList);

        int length() const;
        File item(int index) const;
        File operator[](int index) const;
        emscripten::val val() const;

    private:
        emscripten::val m_fileList = emscripten::val::undefined();
    };

    class Q_CORE_EXPORT FileReader {
    public:
        ArrayBuffer result() const;
        void readAsArrayBuffer(const Blob &blob) const;

        void onLoad(const std::function<void(emscripten::val)> &onLoad);
        void onError(const std::function<void(emscripten::val)> &onError);
        void onAbort(const std::function<void(emscripten::val)> &onAbort);
        emscripten::val val() const;

    private:
        emscripten::val m_fileReader = emscripten::val::global("FileReader").new_();
        std::unique_ptr<EventCallback> m_onLoad;
        std::unique_ptr<EventCallback> m_onError;
        std::unique_ptr<EventCallback> m_onAbort;
    };

    class Q_CORE_EXPORT Uint8Array {
    public:
        static Uint8Array heap();
        explicit Uint8Array(const emscripten::val &uint8Array);
        explicit Uint8Array(const ArrayBuffer &buffer);
        explicit Uint8Array(uint32_t size);
        Uint8Array(const ArrayBuffer &buffer, uint32_t offset, uint32_t length);
        Uint8Array(const char *buffer, uint32_t size);

        ArrayBuffer buffer() const;
        uint32_t length() const;
        void set(const Uint8Array &source);
        Uint8Array subarray(uint32_t begin, uint32_t end);

        void copyTo(char *destination) const;
        QByteArray copyToQByteArray() const;

        static void copy(char *destination, const Uint8Array &source);
        static Uint8Array copyFrom(const char *buffer, uint32_t size);
        static Uint8Array copyFrom(const QByteArray &buffer);
        emscripten::val val() const;

    private:
        static emscripten::val heap_();
        static emscripten::val constructor_();
        emscripten::val m_uint8Array = emscripten::val::undefined();
    };

    class Q_CORE_EXPORT EventCallback
    {
    public:
        EventCallback() = default;
        ~EventCallback();
        EventCallback(EventCallback const&) = delete;
        EventCallback& operator=(EventCallback const&) = delete;
        EventCallback(emscripten::val element, const std::string &name,
                      const std::function<void(emscripten::val)> &fn);
        static void activate(emscripten::val event);

    private:
        static std::string contextPropertyName(const std::string &eventName);
        emscripten::val m_element = emscripten::val::undefined();
        std::string m_eventName;
        std::function<void(emscripten::val)> m_fn;
    };

    struct PromiseCallbacks
    {
        std::function<void(emscripten::val)> thenFunc;
        std::function<void(emscripten::val)> catchFunc;
        std::function<void()> finallyFunc;
    };

    namespace Promise {
        void Q_CORE_EXPORT adoptPromise(emscripten::val promise, PromiseCallbacks callbacks);

        template<typename... Args>
        void make(emscripten::val target,
                  QString methodName,
                  PromiseCallbacks callbacks,
                  Args... args)
        {
            emscripten::val promiseObject = target.call<emscripten::val>(
                methodName.toStdString().c_str(), std::forward<Args>(args)...);
            if (promiseObject.isUndefined() || promiseObject["constructor"]["name"].as<std::string>() != "Promise") {
                 qFatal("This function did not return a promise");
            }

            adoptPromise(std::move(promiseObject), std::move(callbacks));
        }

        void Q_CORE_EXPORT all(std::vector<emscripten::val> promises, PromiseCallbacks callbacks);
    };

    template<class F>
    decltype(auto) bindForever(F wrappedCallback)
    {
        return wrappedCallback;
    }

    class Q_CORE_EXPORT BlobIODevice: public QIODevice
    {
    public:
        BlobIODevice(Blob blob);
        bool open(QIODeviceBase::OpenMode mode) override;
        bool isSequential() const override;
        qint64 size() const override;
        bool seek(qint64 pos) override;

    protected:
        qint64 readData(char *data, qint64 maxSize) override;
        qint64 writeData(const char *, qint64) override;

    private:
        Blob m_blob;
    };

    class Uint8ArrayIODevice: public QIODevice
    {
    public:
        Uint8ArrayIODevice(Uint8Array array);
        bool open(QIODevice::OpenMode mode) override;
        bool isSequential() const override;
        qint64 size() const override;
        bool seek(qint64 pos) override;

    protected:
        qint64 readData(char *data, qint64 maxSize) override;
        qint64 writeData(const char *data, qint64 size) override;

    private:
        Uint8Array m_array;
    };

    inline emscripten::val window()
    {
        static emscripten::val savedWindow = emscripten::val::global("window");
        return savedWindow;
    }

    bool haveAsyncify();
    bool haveJspi();
    bool canBlockCallingThread();

    struct CancellationFlag
    {
    };

#if QT_CONFIG(thread)
    template<class T>
    T proxyCall(std::function<T()> task, emscripten::ProxyingQueue *queue)
    {
        T result;
        queue->proxySync(emscripten_main_runtime_thread_id(),
                         [task, result = &result]() { *result = task(); });
        return result;
    }

    template<>
    inline void proxyCall<void>(std::function<void()> task, emscripten::ProxyingQueue *queue)
    {
        queue->proxySync(emscripten_main_runtime_thread_id(), task);
    }

    template<class T>
    T runTaskOnMainThread(std::function<T()> task, emscripten::ProxyingQueue *queue)
    {
        return emscripten_is_main_runtime_thread() ? task() : proxyCall<T>(std::move(task), queue);
    }

    template<class T>
    T runTaskOnMainThread(std::function<T()> task)
    {
        emscripten::ProxyingQueue singleUseQueue;
        return runTaskOnMainThread<T>(task, &singleUseQueue);
    }

#else
    template<class T>
    T runTaskOnMainThread(std::function<T()> task)
    {
        return task();
    }
#endif // QT_CONFIG(thread)

}

QT_END_NAMESPACE

#endif
