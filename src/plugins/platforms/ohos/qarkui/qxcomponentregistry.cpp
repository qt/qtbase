// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/qxcomponentregistry.h>

#include <QtCore/private/qohoslogger_p.h>
#include <qohosutils.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {

QOhosOptional<QXComponentNode>
QXComponentRegistry::tryTakeNodeByXComponentId(const QXComponentId &id)
{
    QOhosOptional<QXComponentNode> result;
    auto it = m_xComponents.find(id);
    if (it != m_xComponents.end()) {
        result = it->second;
        m_xComponents.erase(it);
    }
    return result;
}

QXComponentRegistry &QXComponentRegistry::instance()
{
    static QXComponentRegistry registry;
    return registry;
}

bool QXComponentRegistry::Init(napi_env env, napi_value exports)
{
    auto exportsObj = QNapi::checkedCast<QNapi::Object>(QNapi::Value{env, exports});
    auto xComponentWrappedValue = QNapi::getOptionalPropOrEmpty<QNapi::Object>(
        exportsObj,
        OH_NATIVE_XCOMPONENT_OBJ);
    if (xComponentWrappedValue.IsEmpty())
        return false;

    ::OH_NativeXComponent *xComponent = nullptr;
    auto status = ::napi_unwrap(
        env,
        xComponentWrappedValue,
        reinterpret_cast<void **>(&xComponent));
    if (status != ::napi_ok) {
        qOhosPrintfError("Failed to unwrap xcomponent with napi_status: %d", status);
        return false;
    }

    auto optXComponentId = QXComponentId::tryCreateFromXComponent(xComponent);
    if (!optXComponentId.hasValue()) {
        qOhosPrintfError("Failed to retrieve id from xcomponent. Ignoring the component");
        return false;
    }

    auto xComponentId = optXComponentId.value();

    auto optXComponentIdType = xComponentId.recognizedType();
    if (!optXComponentIdType.hasValue()) {
        qOhosPrintfError(
            "Ignoring xComponent due to unrecognized id value: %s",
            xComponentId.stringId().c_str());
        return false;
    }

    bool canRegister = false;
    switch (optXComponentIdType.value()) {
    case QXComponentId::RecognizedType::RenderXComponent:
        canRegister = false;
        break;
    case QXComponentId::RecognizedType::NativeNodeFloatWindow:
    case QXComponentId::RecognizedType::NativeNodeSubWindow:
    case QXComponentId::RecognizedType::NativeNodeMainWindow:
        canRegister = true;
        break;
    }

    if (!canRegister) {
        qOhosPrintfDebug(
            "Ignoring xComponent because its id type is not supported in registry. id: %s type: %d",
            xComponentId.stringId().c_str(),
            optXComponentIdType.value());
        return false;
    }

    auto &registry = instance();

    bool xComponentRegistered;
    std::tie(std::ignore, xComponentRegistered) =
        registry.m_xComponents.emplace(xComponentId, QXComponentNode(xComponent));

    if (!xComponentRegistered) {
        qOhosReportFatalErrorAndAbort(
            "Error: Duplicate xComponent detected. Duplicate ID: %s",
            xComponentId.stringId().c_str());
    }

    return true;
}

}

QT_END_NAMESPACE
