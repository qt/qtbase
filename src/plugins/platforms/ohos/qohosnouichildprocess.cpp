// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosnouichildprocess.h"
#include <QtCore/qjsondocument.h>
#include <QtCore/private/qohoslogger_p.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <chrono>
#include <fcntl.h>
#include <memory>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

using namespace std::string_literals;

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace {

// Note: we hard-code this, as the child process doesn't have access to app context
const auto appTempDir = "/data/storage/el2/base/temp";

constexpr unsigned qChildSetupDataSendTimeoutSecs = 5;

constexpr auto qChildSetupDataSendRetryDelay = std::chrono::milliseconds(50);

constexpr std::size_t qChildSetupMaxSetupDataSize = 64 * 1024;

std::string getSetupDataExchangeDirPath()
{
    return appTempDir + "/qohos-child-setup-data"s;
}

std::string getChildSetupDataPath(int childPid)
{
    return getSetupDataExchangeDirPath() + "/"s + std::to_string(childPid);
}

bool tryMakeSetupDataExchangeDirIfNeeded()
{
    auto setupDataExchangeDirPath = getSetupDataExchangeDirPath();

    if (::mkdir(setupDataExchangeDirPath.c_str(), 0700) != 0 && errno != EEXIST) {
        qOhosPrintfError(
            "%s: error creating 'child setup data' directory: %s",
            Q_FUNC_INFO, std::strerror(errno));
        return false;
    }

    struct ::stat statBuf;
    if (::stat(setupDataExchangeDirPath.c_str(), &statBuf) != 0) {
        qOhosPrintfError(
            "%s: stat() failed for 'child setup data' directory: %s",
            Q_FUNC_INFO, std::strerror(errno));
        return false;
    }

    if (!S_ISDIR(statBuf.st_mode)) {
        qOhosPrintfError(
            "%s: non-directory found at 'child setup data' directory path", Q_FUNC_INFO);
        return false;
    }

    return true;
}

bool tryWriteChildSetupDataFile(int childPid, const std::string &setupDataStr)
{
    auto childSetupDataPath = getChildSetupDataPath(childPid);
    auto tmpChildSetupDataPath = childSetupDataPath + ".tmp"s;

    if (!tryMakeSetupDataExchangeDirIfNeeded())
        return false;

    (void) ::unlink(childSetupDataPath.c_str());
    (void) ::unlink(tmpChildSetupDataPath.c_str());

    auto openRes = ::open(tmpChildSetupDataPath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (openRes < 0) {
        qOhosPrintfError(
            "%s: error opening 'child setup data' file '%s' for write: %s",
            Q_FUNC_INFO, tmpChildSetupDataPath.c_str(), std::strerror(errno));
        return false;
    }
    int fileFd = openRes;

    auto writeRes = ::write(fileFd, setupDataStr.data(), setupDataStr.size());
    int writeErrno = errno;

    (void) ::close(fileFd);

    if (writeRes < 0) {
        (void) ::unlink(tmpChildSetupDataPath.c_str());
        qOhosPrintfError(
            "%s: error writing to 'child setup data' file '%s': %s",
            Q_FUNC_INFO, tmpChildSetupDataPath.c_str(), std::strerror(writeErrno));
        return false;
    }
    auto writtenSize = static_cast<std::size_t>(writeRes);
    if (writtenSize != setupDataStr.size()) {
        (void) ::unlink(tmpChildSetupDataPath.c_str());
        qOhosPrintfError(
            "%s: incomplete write to 'child setup data' file: %zu/%zu",
            Q_FUNC_INFO, writtenSize, setupDataStr.size());
        return false;
    }

    if (::rename(tmpChildSetupDataPath.c_str(), childSetupDataPath.c_str()) != 0) {
        qOhosPrintfError(
            "%s: error renaming 'child setup data' file '%s': %s",
            Q_FUNC_INFO, childSetupDataPath.c_str(), std::strerror(errno));
        return false;
    }

    return true;
}

// Returns std::nullopt on error, an empty string when the file is not available (yet).
std::optional<std::string> tryReadChildSetupDataFile(int childPid)
{
    auto childSetupDataPath = getChildSetupDataPath(childPid);

    auto openRes = ::open(childSetupDataPath.c_str(), O_RDONLY);
    if (openRes < 0 && errno == ENOENT)
        return std::string();
    if (openRes < 0) {
        qOhosPrintfError(
            "%s: error opening 'child setup data' file '%s' for read: %s",
            Q_FUNC_INFO, childSetupDataPath.c_str(), std::strerror(errno));
        return {};
    }
    int fileFd = openRes;

    char readBuffer[qChildSetupMaxSetupDataSize + 1];
    auto readRes = ::read(fileFd, readBuffer, sizeof(readBuffer));
    int readErrno = errno;

    (void) ::close(fileFd);

    if (readRes < 0) {
        qOhosPrintfError(
            "%s: error reading 'child setup data' file '%s': %s",
            Q_FUNC_INFO, childSetupDataPath.c_str(), std::strerror(readErrno));
        return {};
    }
    auto readSize = static_cast<std::size_t>(readRes);
    if (readSize > qChildSetupMaxSetupDataSize) {
        qOhosPrintfError(
            "%s: received 'child setup data' file '%s' is too big",
            Q_FUNC_INFO, childSetupDataPath.c_str());
        return {};
    }
    if (readSize == 0) {
        qOhosPrintfError(
            "%s: received 'child setup data' file '%s' is empty",
            Q_FUNC_INFO, childSetupDataPath.c_str());
        return {};
    }

    return std::string(readBuffer, readSize);
}

bool checkIfChildSetupDataFileExists(int childPid)
{
    return ::access(getChildSetupDataPath(childPid).c_str(), F_OK) == 0;
}

void removeChildSetupDataFileIfExists(int childPid)
{
    (void) ::unlink(getChildSetupDataPath(childPid).c_str());
}

// Returns the setup data, or std::nullopt if it couldn't be read (the reason is logged).
std::optional<std::string> tryWaitForChildSetupData(int childPid)
{
    namespace ch = std::chrono;

    auto timeoutEnd = ch::steady_clock::now() + ch::seconds(qChildSetupDataSendTimeoutSecs);
    do {
        auto optSetupDataStr = tryReadChildSetupDataFile(childPid);
        if (!optSetupDataStr)
            return {};
        if (!optSetupDataStr->empty()) {
            removeChildSetupDataFileIfExists(childPid);
            return optSetupDataStr;
        }
        std::this_thread::sleep_for(qChildSetupDataSendRetryDelay);
    } while (ch::steady_clock::now() < timeoutEnd);

    qOhosPrintfError("%s: timeout waiting for 'child setup data' file", Q_FUNC_INFO);
    return {};
}

}

QNapi::Object readChildProcessSetupData(Napi::Env env)
{
    auto optSetupDataStr = tryWaitForChildSetupData(::getpid());
    if (!optSetupDataStr)
        return QNapi::makeObject(env);

    QNapi::Object global = env.Global();
    try {
        return global.eval<QNapi::Object>("JSON.parse(*)", {optSetupDataStr.value()});
    } catch (const Napi::Error &e) {
        qOhosPrintfError("%s: subprocess setup data has illegal format: %s", Q_FUNC_INFO, e.what());
        return QNapi::makeObject(env);
    }
}

void sendChildProcessSetupData(int childPid, QJsonObject setupData)
{
    namespace ch = std::chrono;

    auto senderThread = std::thread(
        [childPid, setupData]() {
            auto setupDataStr = QJsonDocument(setupData).toJson(QJsonDocument::Compact).toStdString();

            if (!tryWriteChildSetupDataFile(childPid, setupDataStr))
                return;

            auto startTime = ch::steady_clock::now();
            auto timeoutEnd = startTime + ch::seconds(qChildSetupDataSendTimeoutSecs);
            do {
                std::this_thread::sleep_for(qChildSetupDataSendRetryDelay);
            } while (checkIfChildSetupDataFileExists(childPid) && ch::steady_clock::now() < timeoutEnd);

            if (checkIfChildSetupDataFileExists(childPid)) {
                removeChildSetupDataFileIfExists(childPid);
                qOhosPrintfError("%s: failed to send setup data to child %d", Q_FUNC_INFO, childPid);
            }
        });
    senderThread.detach();
}

}

QT_END_NAMESPACE
