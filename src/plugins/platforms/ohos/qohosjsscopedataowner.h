// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSSCOPEDATAOWNER_H
#define QOHOSJSSCOPEDATAOWNER_H

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <memory>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

// Holds a Qt-thread object's JS-thread state, created on the JS thread on
// first access and released there.
template<typename JsScopeData>
class JsScopeDataOwner
{
public:
    JsScopeDataOwner();

    JsScopeDataOwner(const JsScopeDataOwner &) = delete;
    JsScopeDataOwner(JsScopeDataOwner &&) = delete;
    JsScopeDataOwner &operator=(const JsScopeDataOwner &) = delete;
    JsScopeDataOwner &operator=(JsScopeDataOwner &&) = delete;

    // JS thread only; create the data on first access
    JsScopeData *operator->() const;
    JsScopeData &operator*() const;

private:
    struct State
    {
        std::shared_ptr<JsScopeData> data;
    };

    static void checkJsThreadOrAbort();
    void createDataIfMissing() const;

    std::shared_ptr<State> m_state;
};

template<typename JsScopeData>
JsScopeDataOwner<JsScopeData>::JsScopeDataOwner()
    : m_state(std::make_shared<State>())
{
}

template<typename JsScopeData>
JsScopeData *JsScopeDataOwner<JsScopeData>::operator->() const
{
    checkJsThreadOrAbort();
    createDataIfMissing();
    return m_state->data.get();
}

template<typename JsScopeData>
JsScopeData &JsScopeDataOwner<JsScopeData>::operator*() const
{
    checkJsThreadOrAbort();
    createDataIfMissing();
    return *m_state->data;
}

template<typename JsScopeData>
void JsScopeDataOwner<JsScopeData>::checkJsThreadOrAbort()
{
    if (!isJsThread())
        qOhosReportFatalErrorAndAbort("%s: JS scope data accessed outside the JS thread", Q_FUNC_INFO);
}

template<typename JsScopeData>
void JsScopeDataOwner<JsScopeData>::createDataIfMissing() const
{
    if (!m_state->data)
        m_state->data = makeProxyWithJsThreadDeleter(std::make_shared<JsScopeData>());
}

}

QT_END_NAMESPACE

#endif
