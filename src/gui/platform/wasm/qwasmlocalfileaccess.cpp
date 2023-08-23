// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmlocalfileaccess_p.h"
#include "qlocalfileapi_p.h"
#include <private/qstdweb_p.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>

#include <QtCore/qregularexpression.h>

QT_BEGIN_NAMESPACE

namespace QWasmLocalFileAccess {
namespace FileDialog {
namespace {
bool hasLocalFilesApi()
{
    return !qstdweb::window()["showOpenFilePicker"].isUndefined();
}

void showOpenViaHTMLPolyfill(const QStringList &accept, FileSelectMode fileSelectMode,
                             qstdweb::PromiseCallbacks onFilesSelected)
{
    // Create file input html element which will display a native file dialog
    // and call back to our onchange handler once the user has selected
    // one or more files.
    emscripten::val document = emscripten::val::global("document");
    emscripten::val input = document.call<emscripten::val>("createElement", std::string("input"));
    input.set("type", "file");
    input.set("style", "display:none");
    input.set("accept", LocalFileApi::makeFileInputAccept(accept));
    Q_UNUSED(accept);
    input.set("multiple", emscripten::val(fileSelectMode == FileSelectMode::MultipleFiles));

    // Note: there is no event in case the user cancels the file dialog.
    static std::unique_ptr<qstdweb::EventCallback> changeEvent;
    auto callback = [=](emscripten::val) { onFilesSelected.thenFunc(input["files"]); };
    changeEvent = std::make_unique<qstdweb::EventCallback>(input, "change", callback);

    // Activate file input
    emscripten::val body = document["body"];
    body.call<void>("appendChild", input);
    input.call<void>("click");
    body.call<void>("removeChild", input);
}

void showOpenViaLocalFileApi(const QStringList &accept, FileSelectMode fileSelectMode,
                             qstdweb::PromiseCallbacks callbacks)
{
    using namespace qstdweb;

    auto options = LocalFileApi::makeOpenFileOptions(accept, fileSelectMode == FileSelectMode::MultipleFiles);

    Promise::make(
        window(), QStringLiteral("showOpenFilePicker"),
        {
            .thenFunc = [=](emscripten::val fileHandles) mutable {
                std::vector<emscripten::val> filePromises;
                filePromises.reserve(fileHandles["length"].as<int>());
                for (int i = 0; i < fileHandles["length"].as<int>(); ++i)
                    filePromises.push_back(fileHandles[i].call<emscripten::val>("getFile"));
                Promise::all(std::move(filePromises), callbacks);
            },
            .catchFunc = callbacks.catchFunc,
            .finallyFunc = callbacks.finallyFunc,
        }, std::move(options));
}

void showSaveViaLocalFileApi(const std::string &fileNameHint, qstdweb::PromiseCallbacks callbacks)
{
    using namespace qstdweb;
    using namespace emscripten;

    auto options = LocalFileApi::makeSaveFileOptions(QStringList(), fileNameHint);

    Promise::make(
        window(), QStringLiteral("showSaveFilePicker"),
        std::move(callbacks), std::move(options));
}
}  // namespace

void showOpen(const QStringList &accept, FileSelectMode fileSelectMode,
              qstdweb::PromiseCallbacks callbacks)
{
    hasLocalFilesApi() ?
        showOpenViaLocalFileApi(accept, fileSelectMode, std::move(callbacks)) :
        showOpenViaHTMLPolyfill(accept, fileSelectMode, std::move(callbacks));
}

bool canShowSave()
{
    return hasLocalFilesApi();
}

void showSave(const std::string &fileNameHint, qstdweb::PromiseCallbacks callbacks)
{
    Q_ASSERT(canShowSave());
    showSaveViaLocalFileApi(fileNameHint, std::move(callbacks));
}
}  // namespace FileDialog

namespace {
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

        const qstdweb::File file = qstdweb::File(fileList[fileIndex]);

        // Ask caller if the file should be accepted
        char *buffer = acceptFile(file.size(), file.name());
        if (buffer == nullptr) {
            (*readFile)(fileIndex + 1);
            return;
        }

        // Read file data into caller-provided buffer
        file.stream(buffer, [readFile = readFile.get(), fileIndex, fileDataReady]() {
            fileDataReady();
            (*readFile)(fileIndex + 1);
        });
    };

    (*readFile)(0);
}

QStringList makeFilterList(const std::string &qtAcceptList)
{
    // copy of qt_make_filter_list() from qfiledialog.cpp
    auto filter = QString::fromStdString(qtAcceptList); 
    if (filter.isEmpty())
        return QStringList();
    QString sep(";;");
    if (!filter.contains(sep) && filter.contains(u'\n'))
        sep = u'\n';

    return filter.split(sep);
}
}

void downloadDataAsFile(const char *content, size_t size, const std::string &fileNameHint)
{
    // Save a file by creating programmatically clicking a download
    // link to an object url to a Blob containing a copy of the file
    // content. The copy is made so that the passed in content buffer
    // can be released as soon as this function returns.
    qstdweb::Blob contentBlob = qstdweb::Blob::copyFrom(content, size);
    emscripten::val document = emscripten::val::global("document");
    emscripten::val window = qstdweb::window();
    emscripten::val contentUrl = window["URL"].call<emscripten::val>("createObjectURL", contentBlob.val());
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

void openFiles(const std::string &accept, FileSelectMode fileSelectMode,
    const std::function<void (int fileCount)> &fileDialogClosed,
    const std::function<char *(uint64_t size, const std::string& name)> &acceptFile,
    const std::function<void()> &fileDataReady)
{
    FileDialog::showOpen(makeFilterList(accept), fileSelectMode, {
        .thenFunc = [=](emscripten::val result) {
            auto files = qstdweb::FileList(result);
            fileDialogClosed(files.length());
            readFiles(files, acceptFile, fileDataReady);
        },
        .catchFunc = [=](emscripten::val) {
            fileDialogClosed(0);
        }
    });
}

void openFile(const std::string &accept,
    const std::function<void (bool fileSelected)> &fileDialogClosed,
    const std::function<char *(uint64_t size, const std::string& name)> &acceptFile,
    const std::function<void()> &fileDataReady)
{
    auto fileDialogClosedWithInt = [=](int fileCount) { fileDialogClosed(fileCount != 0); };
    openFiles(accept, FileSelectMode::SingleFile, fileDialogClosedWithInt, acceptFile, fileDataReady);
}

void saveDataToFileInChunks(emscripten::val fileHandle, const QByteArray &data)
{
    using namespace emscripten;
    using namespace qstdweb;

    Promise::make(fileHandle, QStringLiteral("createWritable"), {
        .thenFunc = [=](val writable) {
            struct State {
                size_t written;
                std::function<void(val result)> continuation;
            };

            static constexpr size_t desiredChunkSize = 1024u;
#if defined(__EMSCRIPTEN_SHARED_MEMORY__)
            qstdweb::Uint8Array chunkArray(desiredChunkSize);
#endif

            auto state = std::make_shared<State>();
            state->written = 0u;
            state->continuation = [=](val) mutable {
                const size_t remaining = data.size() - state->written;
                if (remaining == 0) {
                    Promise::make(writable, QStringLiteral("close"), { .thenFunc = [=](val) {} });
                    state.reset();
                    return;
                }

                const auto currentChunkSize = std::min(remaining, desiredChunkSize);

#if defined(__EMSCRIPTEN_SHARED_MEMORY__)
                // If shared memory is used, WebAssembly.Memory is instantiated with the 'shared'
                // option on. Passing a typed_memory_view to SharedArrayBuffer to
                // FileSystemWritableFileStream.write is disallowed by security policies, so we
                // need to make a copy of the data to a chunk array buffer.
                Promise::make(
                    writable, QStringLiteral("write"),
                    {
                        .thenFunc = state->continuation,
                    },
                    chunkArray.copyFrom(data.constData() + state->written, currentChunkSize)
                        .val()
                        .call<emscripten::val>("subarray", emscripten::val(0),
                                               emscripten::val(currentChunkSize)));
#else
                Promise::make(writable, QStringLiteral("write"),
                    {
                        .thenFunc = state->continuation,
                    },
                    val(typed_memory_view(currentChunkSize, data.constData() + state->written)));
#endif
                state->written += currentChunkSize;
            };

            state->continuation(val::undefined());
        },
    });
}

void saveFile(const QByteArray &data, const std::string &fileNameHint)
{
    if (!FileDialog::canShowSave()) {
        downloadDataAsFile(data.constData(), data.size(), fileNameHint);
        return;
    }

    FileDialog::showSave(fileNameHint, {
        .thenFunc = [=](emscripten::val result) {
            saveDataToFileInChunks(result, data);
        },
    });
}

void saveFile(const char *content, size_t size, const std::string &fileNameHint)
{
    if (!FileDialog::canShowSave()) {
        downloadDataAsFile(content, size, fileNameHint);
        return;
    }

    FileDialog::showSave(fileNameHint, {
        .thenFunc = [=](emscripten::val result) {
            saveDataToFileInChunks(result, QByteArray(content, size));
        },
    });
}

} // namespace QWasmLocalFileAccess

QT_END_NAMESPACE
