/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qwasmlocalfileaccess_p.h"
#include <private/qstdweb_p.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

namespace QWasmLocalFileAccess {

void streamFile(const qstdweb::File &file, uint32_t offset, uint32_t length, char *buffer, const std::function<void ()> &completed)
{
    // Read file in chunks in order to avoid holding two copies in memory at the same time
    const uint32_t chunkSize = 256 * 1024;
    const uint32_t end = offset + length;
    // assert end < file.size
    auto fileReader = std::make_shared<qstdweb::FileReader>();

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
        qstdweb::Blob blob = file.slice(nextChunkBegin, nextChunkEnd);
        fileReader->readAsArrayBuffer(blob);
    };

    // Read first chunk. First iteration is a dummy iteration with no available data.
    (*chunkCompleted)(offset, buffer);
}

void streamFile(const qstdweb::File &file, char *buffer, const std::function<void ()> &completed)
{
    streamFile(file, 0, file.size(), buffer, completed);
}

void readFiles(const qstdweb::FileList &fileList,
    const std::function<char *(uint64_t size, const std::string name)> &acceptFile,
    const std::function<void ()> &fileDataReady)
{
    auto readFile = std::make_shared<std::function<void(int)>>();

    *readFile = [=](int fileIndex) mutable {
        // Stop when all files have been processed
        if (fileIndex >= fileList.length()) {
            readFile.reset();
            return;
        }

        const qstdweb::File file = fileList[fileIndex];

        // Ask caller if the file should be accepted
        char *buffer = acceptFile(file.size(), file.name());
        if (buffer == nullptr) {
            (*readFile)(fileIndex + 1);
            return;
        }

        // Read file data into caller-provided buffer
        streamFile(file, buffer, [=]() {
            fileDataReady();
            (*readFile)(fileIndex + 1);
        });
    };

    (*readFile)(0);
}

typedef std::function<void (const qstdweb::FileList &fileList)> OpenFileDialogCallback;
void openFileDialog(const std::string &accept, FileSelectMode fileSelectMode,
                    const OpenFileDialogCallback &filesSelected)
{
    // Create file input html element which will display a native file dialog
    // and call back to our onchange handler once the user has selected
    // one or more files.
    emscripten::val document = emscripten::val::global("document");
    emscripten::val input = document.call<emscripten::val>("createElement", std::string("input"));
    input.set("type", "file");
    input.set("style", "display:none");
    input.set("accept", emscripten::val(accept));
    input.set("multiple", emscripten::val(fileSelectMode == MultipleFiles));

    // Note: there is no event in case the user cancels the file dialog.
    static std::unique_ptr<qstdweb::EventCallback> changeEvent;
    auto callback = [=]() { filesSelected(qstdweb::FileList(input["files"])); };
    changeEvent.reset(new qstdweb::EventCallback(input, "change", callback));

    // Activate file input
    emscripten::val body = document["body"];
    body.call<void>("appendChild", input);
    input.call<void>("click");
    body.call<void>("removeChild", input);
}

void openFiles(const std::string &accept, FileSelectMode fileSelectMode,
    const std::function<void (int fileCount)> &fileDialogClosed,
    const std::function<char *(uint64_t size, const std::string name)> &acceptFile,
    const std::function<void()> &fileDataReady)
{
    openFileDialog(accept, fileSelectMode, [=](const qstdweb::FileList &files) {
        fileDialogClosed(files.length());
        readFiles(files, acceptFile, fileDataReady);
    });
}

void openFile(const std::string &accept,
    const std::function<void (bool fileSelected)> &fileDialogClosed,
    const std::function<char *(uint64_t size, const std::string name)> &acceptFile,
    const std::function<void()> &fileDataReady)
{
    auto fileDialogClosedWithInt = [=](int fileCount) { fileDialogClosed(fileCount != 0); };
    openFiles(accept, FileSelectMode::SingleFile, fileDialogClosedWithInt, acceptFile, fileDataReady);
}

void saveFile(const char *content, size_t size, const std::string &fileNameHint)
{
    // Save a file by creating programatically clicking a download
    // link to an object url to a Blob containing the file content.
    // File content is copied once, so that the passed in content
    // buffer can be released as soon as this function returns - we
    // don't know for how long the browser will retain the TypedArray
    // view used to create the Blob.

    emscripten::val document = emscripten::val::global("document");
    emscripten::val window = emscripten::val::global("window");

    emscripten::val fileContentView = emscripten::val(emscripten::typed_memory_view(size, content));
    emscripten::val fileContentCopy = emscripten::val::global("ArrayBuffer").new_(size);
    emscripten::val fileContentCopyView = emscripten::val::global("Uint8Array").new_(fileContentCopy);
    fileContentCopyView.call<void>("set", fileContentView);

    emscripten::val contentArray = emscripten::val::array();
    contentArray.call<void>("push", fileContentCopyView);
    emscripten::val type = emscripten::val::object();
    type.set("type","application/octet-stream");
    emscripten::val contentBlob = emscripten::val::global("Blob").new_(contentArray, type);

    emscripten::val contentUrl = window["URL"].call<emscripten::val>("createObjectURL", contentBlob);
    emscripten::val contentLink = document.call<emscripten::val>("createElement", std::string("a"));
    contentLink.set("href", contentUrl);
    contentLink.set("download", fileNameHint);
    contentLink.set("style", "display:none");

    emscripten::val body = document["body"];
    body.call<void>("appendChild", contentLink);
    contentLink.call<void>("click");
    body.call<void>("removeChild", contentLink);

    window["URL"].call<emscripten::val>("revokeObjectURL", contentUrl);
}

} // namespace QWasmLocalFileAccess

QT_END_NAMESPACE
