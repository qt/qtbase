// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosplatformservices.h"
#include <QtCore/qurl.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohospathutils_p.h>
#include <QtCore/qfileinfo.h>
#include <QtGui/qcolor.h>
#include <qohosenums.h>
#include <qohosplugincore.h>

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

class QOhosColorPicker : public QPlatformServiceColorPicker
{
public:
    QOhosColorPicker();

    void pickColor() override;
};

QOhosColorPicker::QOhosColorPicker() = default;

void QOhosColorPicker::pickColor()
{
    auto selfRef = QtOhos::makeQThreadSafeRef(this);
    QtOhos::invokeInJsThread(
        [selfRef](QtOhos::JsState &jsState) {
            jsState.evalToPromiseOrRejectOnThrow("@kit.Penkit.imageFeaturePicker.pickForResult()")
            .onThen(
                [selfRef](const QtOhos::CallbackInfo &cbInfo) {
                    auto pickedColorInfo = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                    auto color = pickedColorInfo.get<QNapi::Object>("color");
                    QColor qColor(
                        color.get<QNapi::Number>("red"),
                        color.get<QNapi::Number>("green"),
                        color.get<QNapi::Number>("blue"),
                        color.get<QNapi::Number>("alpha"));
                    selfRef.visitInQtThreadIfAlive(
                        [qColor](QOhosColorPicker &self) {
                            Q_EMIT self.colorPicked(qColor);
                        });
                })
            .onCatch(
                [](const QtOhos::CallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "@kit.Penkit.imageFeaturePicker.pickForResult() failed");
                });
        });
}
} // namespace

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
                                            {"fileUri", tryMapPathToOhosFileUri(url.path().toStdString()).value_or("")},
                                        })
                                },
                            })
                        : QNapi::makeObject(
                            jsState.env(),
                            {
                                {"action", "ohos.want.action.viewData"},
                                {"uri", tryMapPathToOhosFileUri(url.path().toStdString()).value_or("")},
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

QT_END_NAMESPACE
