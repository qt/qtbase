/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qstdweb_p.h"

#include <emscripten/bind.h>
#include <cstdint>
#include <iostream>

QT_BEGIN_NAMESPACE

namespace qstdweb {

typedef double uint53_t; // see Number.MAX_SAFE_INTEGER

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

Blob::Blob(const emscripten::val &blob)
    :m_blob(blob)
{

}

uint32_t Blob::size() const
{
    return m_blob["size"].as<uint32_t>();
}

// Copies content from the given buffer into a Blob object
Blob Blob::copyFrom(const char *buffer, uint32_t size)
{
    Uint8Array contentCopy = Uint8Array::copyFrom(buffer, size);

    emscripten::val contentArray = emscripten::val::array();
    contentArray.call<void>("push", contentCopy.m_uint8Array);
    emscripten::val type = emscripten::val::object();
    type.set("type","application/octet-stream");
    return Blob(emscripten::val::global("Blob").new_(contentArray, type));
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
void File::stream(uint32_t offset, uint32_t length, char *buffer, const std::function<void ()> &completed) const
{
    // Read file in chunks in order to avoid holding two copies in memory at the same time
    const uint32_t chunkSize = 256 * 1024;
    const uint32_t end = offset + length;
    // assert end < file.size
    auto fileReader = std::make_shared<qstdweb::FileReader>();

    // "this" is valid now, but may not be by the time the chunkCompleted callback
    // below is made. Make a copy of the file handle.
    const File fileHandle = *this;
    auto chunkCompleted = std::make_shared<std::function<void (uint32_t, char *buffer)>>();
    *chunkCompleted = [=](uint32_t chunkBegin, char *chunkBuffer) mutable {

        // Copy current chunk from JS memory to Wasm memory
        qstdweb::ArrayBuffer result = fileReader->result();
        qstdweb::Uint8Array(result).copyTo(chunkBuffer);

        // Read next chunk if not at buffer end
        uint32_t nextChunkBegin = std::min(chunkBegin + result.byteLength(), end);
        uint32_t nextChunkEnd = std::min(nextChunkBegin + chunkSize, end);
        if (nextChunkBegin == end) {
            completed();
            chunkCompleted.reset();
            return;
        }
        char *nextChunkBuffer = chunkBuffer + result.byteLength();
        fileReader->onLoad([=]() { (*chunkCompleted)(nextChunkBegin, nextChunkBuffer); });
        qstdweb::Blob blob = fileHandle.slice(nextChunkBegin, nextChunkEnd);
        fileReader->readAsArrayBuffer(blob);
    };

    // Read first chunk. First iteration is a dummy iteration with no available data.
    (*chunkCompleted)(offset, buffer);
}

// Streams file content into the given buffer asynchronously. The completed
// callback is called on completion.
void File::stream(char *buffer, const std::function<void ()> &completed) const
{
    stream(0, size(), buffer, completed);
}

std::string File::type() const
{
    return m_file["type"].as<std::string>();
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

ArrayBuffer FileReader::result() const
{
    return ArrayBuffer(m_fileReader["result"]);
}

void FileReader::readAsArrayBuffer(const Blob &blob) const
{
    m_fileReader.call<void>("readAsArrayBuffer", blob.m_blob);
}

void FileReader::onLoad(const std::function<void ()> &onLoad)
{
    m_onLoad.reset(new EventCallback(m_fileReader, "load", onLoad));
}

void FileReader::onError(const std::function<void ()> &onError)
{
    m_onError.reset(new EventCallback(m_fileReader, "error", onError));
}

void FileReader::onAbort(const std::function<void ()> &onAbort)
{
    m_onAbort.reset(new EventCallback(m_fileReader, "abort", onAbort));
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
:m_uint8Array(Uint8Array::constructor_().new_(Uint8Array::heap().buffer().m_arrayBuffer, uint32_t(buffer), size))
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

// Copies the Uint8Array conent to a destination on the heap
void Uint8Array::copyTo(char *destination) const
{
    Uint8Array(destination, length()).set(*this);
}

// Copies the Uint8Array conent to a destination on the heap
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
EventCallback::EventCallback(emscripten::val element, const std::string &name, const std::function<void ()> &fn)
:m_fn(fn)
{
    element.set(contextPropertyName(name).c_str(), emscripten::val(intptr_t(this)));
    element.set((std::string("on") + name).c_str(), emscripten::val::module_property("qtStdWebEventCallbackActivate"));
}

void EventCallback::activate(emscripten::val event)
{
    emscripten::val target = event["target"];
    std::string eventName = event["type"].as<std::string>();
    EventCallback *that = reinterpret_cast<EventCallback *>(target[contextPropertyName(eventName).c_str()].as<intptr_t>());
    that->m_fn();
}

std::string EventCallback::contextPropertyName(const std::string &eventName)
{
    return std::string("data-qtEventCallbackContext") + eventName;
}

EMSCRIPTEN_BINDINGS(qtStdwebCalback) {
    emscripten::function("qtStdWebEventCallbackActivate", &EventCallback::activate);
}

} // namespace qstdweb

QT_END_NAMESPACE
