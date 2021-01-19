/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

#include <qglobal.h>
#include <emscripten/val.h>
#include <cstdint>
#include <functional>

QT_BEGIN_NAMESPACE

namespace qstdweb {

    // DOM API in C++, implemented using emscripten val.h and bind.h.
    // This is private API and can be extened and changed as needed.

    class ArrayBuffer;
    class Blob;
    class File;
    class FileList;
    class FileReader;
    class Uint8Array;
    class EventCallback;

    class ArrayBuffer {
    public:
        explicit ArrayBuffer(const emscripten::val &arrayBuffer);
        uint32_t byteLength() const;

    private:
        friend class Uint8Array;
        emscripten::val m_arrayBuffer = emscripten::val::undefined();
    };

    class Blob {
    public:
        explicit Blob(const emscripten::val &blob);
        uint32_t size() const;

    private:
        friend class FileReader;
        emscripten::val m_blob = emscripten::val::undefined();
    };

    class File {
    public:
        File() = default;
        explicit File(const emscripten::val &file);

        Blob slice(uint64_t begin, uint64_t end) const;
        std::string name() const;
        uint64_t size() const;

    private:
        emscripten::val m_file = emscripten::val::undefined();
    };

    class FileList {
    public:
        FileList() = default;
        explicit FileList(const emscripten::val &fileList);

        int length() const;
        File item(int index) const;
        File operator[](int index) const;

    private:
        emscripten::val m_fileList = emscripten::val::undefined();
    };

    class FileReader {
    public:
        ArrayBuffer result() const;
        void readAsArrayBuffer(const Blob &blob) const;

        void onLoad(const std::function<void ()> &onLoad);
        void onError(const std::function<void ()> &onError);
        void onAbort(const std::function<void ()> &onAbort);

    private:
        emscripten::val m_fileReader = emscripten::val::global("FileReader").new_();
        std::unique_ptr<EventCallback> m_onLoad;
        std::unique_ptr<EventCallback> m_onError;
        std::unique_ptr<EventCallback> m_onAbort;
    };

    class Uint8Array {
    public:
        static Uint8Array heap();
        explicit Uint8Array(const emscripten::val &uint8Array);
        explicit Uint8Array(const ArrayBuffer &buffer);
        Uint8Array(const ArrayBuffer &buffer, uint32_t offset, uint32_t length);
        Uint8Array(char *buffer, uint32_t size);

        ArrayBuffer buffer() const;
        uint32_t length() const;
        void set(const Uint8Array &source);

        void copyTo(char *destination) const;
        static void copy(char *destination, const Uint8Array &source);
    private:
        static emscripten::val heap_();
        static emscripten::val constructor_();
        emscripten::val m_uint8Array = emscripten::val::undefined();
    };

    class EventCallback
    {
    public:
        EventCallback(emscripten::val element, const std::string &name, const std::function<void ()> &fn);
        static void activate(emscripten::val event);
    private:
        static std::string contextPropertyName(const std::string &eventName);
        std::function<void ()> m_fn;
    };
}

QT_END_NAMESPACE

#endif
