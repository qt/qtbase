// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosinternalwindowid_p.h"

QT_BEGIN_NAMESPACE

namespace QtOhos {

InternalWindowId InternalWindowId::generate()
{
    static QAtomicInt winIdGenerator(MainWindowId);
    return InternalWindowId(winIdGenerator.fetchAndAddRelaxed(1));
}

InternalWindowId InternalWindowId::fromNapiValue(QNapi::Number napiWindowId)
{
    return InternalWindowId(napiWindowId.Int32Value());
}

constexpr InternalWindowId::InternalWindowId(int value)
    : m_value(value)
{
}

bool InternalWindowId::operator==(const InternalWindowId &other) const
{
    return m_value == other.m_value;
}

bool InternalWindowId::operator!=(const InternalWindowId &other) const
{
    return m_value != other.m_value;
}

bool InternalWindowId::operator<(const InternalWindowId &other) const
{
    return m_value < other.m_value;
}

bool InternalWindowId::isMainWindowId() const
{
    return m_value == MainWindowId;
}

bool InternalWindowId::isValid() const
{
    return m_value != InvalidWindowId;
}

InternalWindowId InternalWindowId::invalidWindowId()
{
    static constexpr InternalWindowId invalidWindowId(InvalidWindowId);
    return invalidWindowId;
}

QNapi::Number InternalWindowId::toNapiValue(napi_env env) const
{
    return QNapi::Number::New(env, m_value);
}

QString InternalWindowId::toString() const
{
    return QString::fromUtf8("WIID_%1")
        .arg(
            isValid()
            ? isMainWindowId()
                ? QString::fromUtf8("MainWindow")
                : QString::number(m_value)
            : QString::fromUtf8("Invalid"));
}

std::string InternalWindowId::toStdString() const
{
    return toString().toStdString();
}

}

QT_END_NAMESPACE
