// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformservices.h"
#include <QtCore/QUrl>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qscopeguard.h>
#include <QtGui/QColor>
#include <cstdlib>
#include <filemanagement/file_uri/oh_file_uri.h>
#include <qohosenums.h>
#include <qohosplugincore.h>
#include <qohosqpafunctions_p.h>

QT_BEGIN_NAMESPACE

using QOhosWantConstantFlags = QtOhos::enums::ohos::app::ability::wantConstant::Flags;

namespace {

void callStartAbility(QNapi::Object baseQAbility, QNapi::Object want, QOhosConsumer<bool> resultConsumer)
{
    qOhosPrintfDebug("Calling startAbility() with Want '%s'", QNapi::toJsonString(want).c_str());

    baseQAbility.evalToPromiseOrRejectOnThrow("context.startAbility(*)", {want})
    .withContext(std::move(resultConsumer))
    .onThenWithContext(
        [](const QtOhos::CallbackInfo &, auto &resultConsumer) {
            qOhosPrintfDebug("Got success from startAbility()");
            resultConsumer(true);
        })
    .onCatchWithContext(
        [](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
            QtOhos::logJsCallbackError(cbInfo, "Got error from startAbility()");
            resultConsumer(false);
        });
}

std::string callOhFileUriConversionFunc(
    FileManagement_ErrCode (*convFunc)(const char *, unsigned int, char **),
    const char *convFuncName, const std::string &input)
{
    std::string outputString;

    char *outputPtr = nullptr;
    auto outputPtrGuard = qScopeGuard(std::bind(::free, outputPtr));
    auto convFuncRetVal = convFunc(input.c_str(), input.size(), &outputPtr);

    if (convFuncRetVal == FileManagement_ErrCode::ERR_OK && outputPtr != nullptr) {
        outputString = outputPtr;
    } else {
        qWarning(
            "OH FileUri conversion function '%s' failed for input '%s', retval: %d",
            convFuncName, input.c_str(), static_cast<int>(convFuncRetVal));
    }

    return outputString;
}

} // namespace

class QOhosColorPicker : public QPlatformServiceColorPicker
{
public:
    QOhosColorPicker() {}
    ~QOhosColorPicker() {
        m_ohosColorPickingHandler.reset();
    }

    void pickColor() override
    {
        m_ohosColorPickingHandler =
                QtOhos::getQOhosQpaFunctions().startPickingColorFromScreenWithConsumer(
                        [this](QOhosOptional<quint32> rgbaColor) {
                            if (rgbaColor.hasValue())
                                emit this->colorPicked(QColor::fromRgba(rgbaColor.value()));
                        });
    }
private:
    std::shared_ptr<void> m_ohosColorPickingHandler;
};

QOhosPlatformServices::QOhosPlatformServices() = default;

QPlatformServiceColorPicker *QOhosPlatformServices::colorPicker(QWindow *parent)
{
    Q_UNUSED(parent);
    return new QOhosColorPicker;
}

bool QOhosPlatformServices::hasCapability(Capability capability) const
{
    switch (capability) {
    case ColorPicking:
        return true;
    default:
        return false;
    }
}

bool QOhosPlatformServices::openUrl(const QUrl &url)
{
    return QtOhos::evalInJsThreadWithPromise<bool>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<bool> evalPromise) {
            auto mainUiAbility = jsState.defaultQAbilityPeer()->qAbility();
            if (mainUiAbility.IsEmpty()) {
                evalPromise(false);
                return;
            }

            auto want =
                !url.isLocalFile()
                    ? QNapi::makeObject(
                        jsState.env(),
                        {
                            {"action", "ohos.want.action.viewData"},
                            {"entities", QNapi::makeArray(jsState.env(), {"entity.system.browsable"})},
                            {"uri", url.toString().toStdString()},
                        })
                    : QFileInfo(url.path()).isDir()
                        ? QNapi::makeObject(
                            jsState.env(),
                            {
                                {"abilityName", "MainAbility"},
                                {"bundleName", "com.huawei.hmos.filemanager"},
                                {
                                    "parameters",
                                    QNapi::makeObject(
                                        jsState.env(),
                                        {
                                            {"fileUri", mapPathToOhosUriInJsThread(url.path().toStdString())},
                                        })
                                },
                            })
                        : QNapi::makeObject(
                            jsState.env(),
                            {
                                {"action", "ohos.want.action.viewData"},
                                {"uri", mapPathToOhosUriInJsThread(url.path().toStdString())},
                                {
                                    "flags",
                                    jsState.mapOhosEnumToJs(QOhosWantConstantFlags::FLAG_AUTH_READ_URI_PERMISSION).Int32Value()
                                    | jsState.mapOhosEnumToJs(QOhosWantConstantFlags::FLAG_AUTH_WRITE_URI_PERMISSION).Int32Value()
                                },
                            });
            callStartAbility(mainUiAbility, want, std::move(evalPromise));
        },
        Q_FUNC_INFO);
}

bool QOhosPlatformServices::openDocument(const QUrl &url)
{
    return openUrl(url);
}

QByteArray QOhosPlatformServices::desktopEnvironment() const
{
    return QByteArray("Ohos");
}

std::string QOhosPlatformServices::mapPathToOhosUriInJsThread(const std::string &path)
{
    return callOhFileUriConversionFunc(OH_FileUri_GetUriFromPath, "OH_FileUri_GetUriFromPath", path);
}

std::string QOhosPlatformServices::mapOhosFileUriToPathInJsThread(const std::string &ohosFileUri)
{
    return callOhFileUriConversionFunc(OH_FileUri_GetPathFromUri, "OH_FileUri_GetPathFromUri", ohosFileUri);
}

QT_END_NAMESPACE
