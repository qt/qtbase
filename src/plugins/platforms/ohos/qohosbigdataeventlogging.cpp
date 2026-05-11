// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosbigdataeventlogging.h>

#include <cstring>
#include <ctime>
#include <functional>
#include <qarkui/qarkuiutils.h>
#include <qohosutils.h>
#include <vector>

namespace ch = std::chrono;

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace {

constexpr auto eventsDomainName = "qtapplication";

constexpr auto eventTimePropertyName = "eventTime";

std::shared_ptr<::ParamListNode> makeParamList()
{
    return std::shared_ptr<::ParamListNode>(
        QArkUi::callArkUiOrFailOnNullResult(Q_OHOS_NAMED_FUNC(::OH_HiAppEvent_CreateParamList)),
        [](auto *paramListNode) {
            QArkUi::callArkUi(Q_OHOS_NAMED_FUNC(::OH_HiAppEvent_DestroyParamList), paramListNode);
        });
}

std::string strftimeToString(const char *timeFormat, const struct tm *tm)
{
    constexpr std::size_t defaultBufferSize = 64;
    constexpr std::size_t maxBufferOverFormatSize = 1024 * 1024;

    const auto maxBufferSize = std::strlen(timeFormat) + maxBufferOverFormatSize;

    std::string buffer(defaultBufferSize, '\0');

    while (true) {
        auto strftimeResult = std::strftime(&buffer[0], buffer.size(), timeFormat, tm);
        if (strftimeResult != 0) {
            buffer.resize(strftimeResult);
            break;
        }

        buffer.resize(buffer.size() * 2);
        if (buffer.size() > maxBufferSize) {
            qOhosReportFatalErrorAndAbort(
                "%s: exceeded buffer size for strftime, format: %zu, buffer: %zu",
                Q_FUNC_INFO, std::strlen(timeFormat), buffer.size());
        }
    }

    return buffer;
}

std::string strftimeToString(const char *timeFormat, ch::system_clock::time_point timePoint)
{
    const auto time = ch::system_clock::to_time_t(timePoint);
    return strftimeToString(timeFormat, std::localtime(&time));
}

std::string formatTimestampString(ch::time_point<ch::system_clock> timepoint)
{
    const int subSecondMilliseconds = ch::duration_cast<ch::milliseconds>(timepoint.time_since_epoch()).count() % 1000;

    return QtOhos::printfToString(
        "%s.%03d%s",
        strftimeToString("%Y-%m-%dT%H:%M:%S", timepoint).c_str(), subSecondMilliseconds,
        strftimeToString("%z", timepoint).c_str());
}

class BigEventLoggingEventImpl : public BigEventLoggingEvent
{
public:
    BigEventLoggingEventImpl(
        const std::string &eventName,
        ::EventType eventType,
        std::shared_ptr<::ParamListNode> eventParams);
    virtual ~BigEventLoggingEventImpl();

    bool trySend() const override;

private:
    std::string m_eventName;
    ::EventType m_eventType;
    std::shared_ptr<::ParamListNode> m_eventParams;
};

class BigEventLoggingEventBuilderImpl final : public BigEventLoggingEventBuilder
{
public:
    BigEventLoggingEventBuilderImpl(
        const std::string &eventName,
        ::EventType eventType,
        std::chrono::time_point<std::chrono::system_clock> eventTime);
    virtual ~BigEventLoggingEventBuilderImpl();

    void addParam(const std::string &paramName, const std::string &paramValue) override;
    void addParam(const std::string &paramName, std::int64_t paramValue) override;

    std::shared_ptr<BigEventLoggingEvent> buildEvent() const override;

private:
    std::string m_eventName;
    ::EventType m_eventType;
    std::vector<std::function<::ParamList(::ParamList)>> m_paramListNodeBuilders;
};

BigEventLoggingEventImpl::BigEventLoggingEventImpl(
    const std::string &eventName, ::EventType eventType,
    std::shared_ptr<::ParamListNode> eventParams)
    : m_eventName(eventName)
    , m_eventType(eventType)
    , m_eventParams(eventParams)
{
}

BigEventLoggingEventImpl::~BigEventLoggingEventImpl() = default;

bool BigEventLoggingEventImpl::trySend() const
{
    int result = QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_HiAppEvent_Write),
        QArkUi::CZString(eventsDomainName),
        QArkUi::CZString(m_eventName.c_str()),
        m_eventType, m_eventParams.get());

    if (result != 0) {
        qOhosPrintfError("%s: OH_HiAppEvent_Write failed: %d", Q_FUNC_INFO, result);
        return false;
    }

    return true;
}

BigEventLoggingEventBuilderImpl::BigEventLoggingEventBuilderImpl(
    const std::string &eventName, ::EventType eventType,
    std::chrono::time_point<std::chrono::system_clock> eventTime)
    : m_eventName(eventName)
    , m_eventType(eventType)
{
    addParam(eventTimePropertyName, formatTimestampString(eventTime));
}

BigEventLoggingEventBuilderImpl::~BigEventLoggingEventBuilderImpl() = default;

void BigEventLoggingEventBuilderImpl::addParam(const std::string &paramName, const std::string &paramValue)
{
    m_paramListNodeBuilders.push_back(
        [paramName, paramValue](auto *paramListNode) {
            return QArkUi::callArkUiOrFailOnNullResult(
                Q_OHOS_NAMED_FUNC(::OH_HiAppEvent_AddStringParam),
                paramListNode, QArkUi::CZString(paramName.c_str()),
                QArkUi::CZString(paramValue.c_str()));
        });
}

void BigEventLoggingEventBuilderImpl::addParam(const std::string &paramName, std::int64_t paramValue)
{
    m_paramListNodeBuilders.push_back(
        [paramName, paramValue](auto *paramListNode) {
            return QArkUi::callArkUiOrFailOnNullResult(
                Q_OHOS_NAMED_FUNC(::OH_HiAppEvent_AddInt64Param),
                paramListNode, QArkUi::CZString(paramName.c_str()), paramValue);
        });
}

std::shared_ptr<BigEventLoggingEvent> BigEventLoggingEventBuilderImpl::buildEvent() const
{
    auto paramListNodeSharedPtr = makeParamList();
    auto *paramList = paramListNodeSharedPtr.get();

    for (const auto &paramListNodeBuilder : m_paramListNodeBuilders) {
        auto *modifiedParamList = paramListNodeBuilder(paramList);
        paramList = modifiedParamList;
    }

    return std::make_shared<BigEventLoggingEventImpl>(m_eventName, m_eventType, paramListNodeSharedPtr);
}

}

BigEventLoggingEvent::BigEventLoggingEvent() = default;

BigEventLoggingEvent::~BigEventLoggingEvent() = default;

BigEventLoggingEventBuilder::BigEventLoggingEventBuilder() = default;

BigEventLoggingEventBuilder::~BigEventLoggingEventBuilder() = default;

std::shared_ptr<BigEventLoggingEventBuilder> makeBigEventLoggingEventBuilder(
    const std::string &eventName, ::EventType eventType,
    std::chrono::time_point<std::chrono::system_clock> eventTime)
{
    return std::make_shared<BigEventLoggingEventBuilderImpl>(eventName, eventType, eventTime);
}

}

QT_END_NAMESPACE
