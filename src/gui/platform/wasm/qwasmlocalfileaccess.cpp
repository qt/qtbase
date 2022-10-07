// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmlocalfileaccess_p.h"
#include <private/qstdweb_p.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>

#include <QtCore/qregularexpression.h>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

namespace QWasmLocalFileAccess {

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
        file.stream(buffer, [=]() {
            fileDataReady();
            (*readFile)(fileIndex + 1);
        });
    };

    (*readFile)(0);
}

std::string acceptListFromQtFormat(const std::string &qtAcceptList)
{
    // copy of qt_make_filter_list() from qfiledialog.cpp
    auto make_filter_list = [](const QString &filter) -> QStringList
    {
        if (filter.isEmpty())
            return QStringList();

        QString sep(";;"_L1);
        if (!filter.contains(sep) && filter.contains(u'\n'))
            sep = u'\n';

        return filter.split(sep);
    };

    const QStringList fileFilter = make_filter_list(QString::fromStdString(qtAcceptList));
    QStringList transformed;
    for (const auto &element : fileFilter) {
        // Accepts either a string in format:
        // GROUP3
        // or in this format:
        // GROUP1 (GROUP2)
        // Group 1 is treated as the description, whereas group 2 or 3 are treated as the filter
        // list.
        static QRegularExpression regex(
                QString(QStringLiteral("(?:([^(]*)\\(([^()]+)\\)[^)]*)|([^()]+)")));
        static QRegularExpression wordCharacterRegex(QString(QStringLiteral("\\w")));
        const auto match = regex.match(element);

        if (!match.hasMatch())
            continue;

        constexpr size_t FilterListFromParensIndex = 2;
        constexpr size_t PlainFilterListIndex = 3;
        QString filterList = match.captured(match.hasCaptured(FilterListFromParensIndex)
                                                    ? FilterListFromParensIndex
                                                    : PlainFilterListIndex);
        for (auto singleExtension : filterList.split(QStringLiteral(" "), Qt::SkipEmptyParts)) {
            // Checks for a filter that matches everything:
            // Any number of asterisks or any number of asterisks with a '.' between them.
            // The web filter does not support wildcards.
            static QRegularExpression qtAcceptAllRegex(QRegularExpression::anchoredPattern(
                    QString(QStringLiteral("[*]+|[*]+\\.[*]+"))));
            if (qtAcceptAllRegex.match(singleExtension).hasMatch())
                continue;

            // Checks for correctness. The web filter only allows filename extensions and does not
            // filter the actual filenames, therefore we check whether the filter provided only
            // filters for the extension.
            static QRegularExpression qtFilenameMatcherRegex(QRegularExpression::anchoredPattern(
                    QString(QStringLiteral("(\\*?)(\\.[^*]+)"))));

            auto extensionMatch = qtFilenameMatcherRegex.match(singleExtension);
            if (extensionMatch.hasMatch())
                transformed.append(extensionMatch.captured(2));
        }
    }
    return transformed.join(QStringLiteral(",")).toStdString();
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
    input.set("accept", acceptListFromQtFormat(accept));
    input.set("multiple", emscripten::val(fileSelectMode == MultipleFiles));

    // Note: there is no event in case the user cancels the file dialog.
    static std::unique_ptr<qstdweb::EventCallback> changeEvent;
    auto callback = [=](emscripten::val) { filesSelected(qstdweb::FileList(input["files"])); };
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
    // Save a file by creating programmatically clicking a download
    // link to an object url to a Blob containing a copy of the file
    // content. The copy is made so that the passed in content buffer
    // can be released as soon as this function returns.
    qstdweb::Blob contentBlob = qstdweb::Blob::copyFrom(content, size);
    emscripten::val document = emscripten::val::global("document");
    emscripten::val window = emscripten::val::global("window");
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

} // namespace QWasmLocalFileAccess

QT_END_NAMESPACE
