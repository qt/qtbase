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
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <system_error>
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

std::system_error makeSystemErrorFromErrno(const std::string &message, int errnoValue)
{
    return std::system_error(errnoValue, std::generic_category(), message);
}

std::string getSetupDataExchangeDirPath()
{
    return appTempDir + "/qohos-child-setup-data"s;
}

std::string getChildSetupDataPath(int childPid)
{
    return getSetupDataExchangeDirPath() + "/"s + std::to_string(childPid);
}

void makeSetupDataExchangeDirIfNeeded()
{
    auto setupDataExchangeDirPath = getSetupDataExchangeDirPath();

    if (::mkdir(setupDataExchangeDirPath.c_str(), 0700) != 0 && errno != EEXIST) {
        throw makeSystemErrorFromErrno(
            "error creating 'child setup data' directory", errno);
    }

    struct ::stat statBuf;
    if (::stat(setupDataExchangeDirPath.c_str(), &statBuf) != 0) {
        throw makeSystemErrorFromErrno(
            "stat() failed for 'child setup data' directory", errno);
    }

    if (!S_ISDIR(statBuf.st_mode))
        throw std::runtime_error("non-directory found at 'child setup data' directory path");
}

void writeChildSetupDataFile(int childPid, const std::string &setupDataStr)
{
    auto childSetupDataPath = getChildSetupDataPath(childPid);
    auto tmpChildSetupDataPath = childSetupDataPath + ".tmp"s;

    makeSetupDataExchangeDirIfNeeded();

    (void) ::unlink(childSetupDataPath.c_str());
    (void) ::unlink(tmpChildSetupDataPath.c_str());

    auto openRes = ::open(tmpChildSetupDataPath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (openRes < 0) {
        throw makeSystemErrorFromErrno(
            "error opening 'child setup data' file '"s + tmpChildSetupDataPath + "' for write"s, errno);
    }
    int fileFd = openRes;

    auto writeRes = ::write(fileFd, setupDataStr.data(), setupDataStr.size());
    int writeErrno = errno;

    (void) ::close(fileFd);

    if (writeRes < 0) {
        (void) ::unlink(tmpChildSetupDataPath.c_str());
        throw makeSystemErrorFromErrno(
            "error writing to 'child setup data' file '"s + tmpChildSetupDataPath + "'"s, writeErrno);
    }
    auto writtenSize = static_cast<std::size_t>(writeRes);
    if (writtenSize != setupDataStr.size()) {
        (void) ::unlink(tmpChildSetupDataPath.c_str());
        throw std::runtime_error(
            "incomplete write to 'child setup data' file: "s
            + std::to_string(writtenSize)  + "/"s + std::to_string(setupDataStr.size()));
    }

    if (::rename(tmpChildSetupDataPath.c_str(), childSetupDataPath.c_str()) != 0) {
        throw makeSystemErrorFromErrno(
            "error renaming 'child setup data' file '"s + childSetupDataPath + "'"s, errno);
    }
}

std::string tryReadChildSetupDataFile(int childPid)
{
    auto childSetupDataPath = getChildSetupDataPath(childPid);

    auto openRes = ::open(childSetupDataPath.c_str(), O_RDONLY);
    if (openRes < 0 && errno == ENOENT)
        return {};
    if (openRes < 0) {
        throw makeSystemErrorFromErrno(
            "error opening 'child setup data' file '"s + childSetupDataPath + "' for read"s, errno);
    }
    int fileFd = openRes;

    char readBuffer[qChildSetupMaxSetupDataSize + 1];
    auto readRes = ::read(fileFd, readBuffer, sizeof(readBuffer));
    int readErrno = errno;

    (void) ::close(fileFd);

    if (readRes < 0) {
        throw makeSystemErrorFromErrno(
            "error reading 'child setup data' file '"s + childSetupDataPath + "'"s, readErrno);
    }
    auto readSize = static_cast<std::size_t>(readRes);
    if (readSize > qChildSetupMaxSetupDataSize) {
        throw std::runtime_error(
            "received 'child setup data' file '"s + childSetupDataPath + "' is too big"s);
    } else if (readSize == 0) {
        throw std::runtime_error(
            "received 'child setup data' file '"s + childSetupDataPath + "' is empty"s);
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

}

QNapi::Object readChildProcessSetupData(Napi::Env env)
{
    namespace ch = std::chrono;

    auto childPid = ::getpid();

    std::string setupDataStr;
    try {
        auto startTime = ch::steady_clock::now();
        auto timeoutEnd = startTime + ch::seconds(qChildSetupDataSendTimeoutSecs);
        do {
            setupDataStr = tryReadChildSetupDataFile(childPid);
            if (!setupDataStr.empty())
                break;
            std::this_thread::sleep_for(qChildSetupDataSendRetryDelay);
        } while (ch::steady_clock::now() < timeoutEnd);

        if (!setupDataStr.empty())
            removeChildSetupDataFileIfExists(childPid);
        else
            throw std::runtime_error("timeout waiting for 'child setup data' file");
    } catch (const std::exception &e) {
        qOhosPrintfError("%s: error reading subprocess setup data: %s", Q_FUNC_INFO, e.what());
    }

    QNapi::Object global = env.Global();
    try {
        return global.eval<QNapi::Object>("JSON.parse(*)", {setupDataStr});
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

            try {
                writeChildSetupDataFile(childPid, setupDataStr);
            } catch (const std::exception &e) {
                qOhosPrintfError("%s: error writing subprocess setup data: %s", Q_FUNC_INFO, e.what());
                return;
            }

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
