// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSBIGDATAEVENTLOGGING_H
#define QOHOSBIGDATAEVENTLOGGING_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <chrono>
#include <cstdint>
#include <hiappevent/hiappevent.h>
#include <memory>
#include <string>

QT_BEGIN_NAMESPACE

namespace QtOhos {

class BigEventLoggingEvent
{
public:
    virtual ~BigEventLoggingEvent();

    virtual bool trySend() const = 0;

protected:
    BigEventLoggingEvent();
};

class BigEventLoggingEventBuilder
{
public:
    virtual ~BigEventLoggingEventBuilder();

    virtual void addParam(const std::string &paramName, const std::string &paramValue) = 0;
    virtual void addParam(const std::string &paramName, std::int64_t paramValue) = 0;

    virtual std::shared_ptr<BigEventLoggingEvent> buildEvent() const = 0;

protected:
    BigEventLoggingEventBuilder();
};

std::shared_ptr<BigEventLoggingEventBuilder> makeBigEventLoggingEventBuilder(
    const std::string &eventName, ::EventType eventType,
    std::chrono::time_point<std::chrono::system_clock> eventTime);

}

QT_END_NAMESPACE

#endif
