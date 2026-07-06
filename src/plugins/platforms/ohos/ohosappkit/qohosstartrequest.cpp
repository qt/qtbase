// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosstartrequest.h"

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtOhosAppKit/private/qohosstartoptions_p.h>
#include <QtOhosAppKit/private/qohosstartrequest_p.h>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

/*!
    \class QtOhosAppKit::QOhosStartRequest
    \inmodule QtOhosAppKit
    \since 5.12.12

    \brief The QOhosStartRequest class emits signals for start ability.

    QOhosStartRequest adapts the StartOptions completionHandler into Qt signals. The
    requestSucceeded() and requestFailed() signals correspond to onRequestSuccess and
    onRequestFailure callbacks from the platform completion handler.

    Create an instance using createStartRequest().

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
    {Completion Handler}.
*/

namespace {

QOhosElementName convertElementNameFromJsonObject(const QJsonObject &object)
{
    return QOhosElementName{
        .deviceId = object.value(QLatin1String("deviceId")).toString(),
        .bundleName = object.value(QLatin1String("bundleName")).toString(),
        .abilityName = object.value(QLatin1String("abilityName")).toString(),
        .uri = object.value(QLatin1String("uri")).toString(),
        .shortName = object.value(QLatin1String("shortName")).toString(),
        .moduleName = object.value(QLatin1String("moduleName")).toString(),
    };
}

class QOhosStartRequestImpl : public QOhosStartRequest
{
public:
    explicit QOhosStartRequestImpl(QOhosStartOptionsData startOptions);

    QOhosStartOptionsData startOptions() const;

    void setCompletionHandler(QOhosConsumer<bool, QJsonObject, QString> handler);

private:
    QOhosStartOptionsData m_startOptions;
};

QOhosStartRequestImpl::QOhosStartRequestImpl(QOhosStartOptionsData startOptions)
    : m_startOptions(std::move(startOptions))
{
}

QOhosStartOptionsData QOhosStartRequestImpl::startOptions() const
{
    return m_startOptions;
}

void QOhosStartRequestImpl::setCompletionHandler(QOhosConsumer<bool, QJsonObject, QString> handler)
{
    m_startOptions.optCompletionHandler =
        std::make_shared<QOhosConsumer<bool, QJsonObject, QString>>(std::move(handler));
}

}

QOhosStartRequest::QOhosStartRequest() = default;

QOhosStartRequest::~QOhosStartRequest() = default;

/*!
    \fn QSharedPointer<QOhosStartRequest> QtOhosAppKit::createStartRequest(const QOhosStartOptions &options)

    Creates a start request for given \a options and adapts the completion handler into Qt signals.

    Connect to QOhosStartRequest::requestSucceeded() and QOhosStartRequest::requestFailed() to
    receive completion handler results.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
    {Completion Handler}.

    \sa QOhosStartRequest
*/
QSharedPointer<QOhosStartRequest> createStartRequest(const QOhosStartOptions &options)
{
    qRegisterMetaType<QtOhosAppKit::QOhosElementName>();

    auto optQpaStartOptions = tryConvertStartOptionsToQpaFunctionsStruct(options);
    if (!optQpaStartOptions.has_value())
        qCWarning(QtForOhos, "%s: unsupported start options instance, using default options", Q_FUNC_INFO);

    auto request = QSharedPointer<QOhosStartRequestImpl>::create(
        optQpaStartOptions.value_or(QOhosStartOptionsData()));
    request->setCompletionHandler(
        [request = request.toWeakRef()](bool success, const QJsonObject &elementName, const QString &message) {
            auto startRequest = request.toStrongRef();
            if (startRequest.isNull())
                return;

            if (success)
                Q_EMIT startRequest->requestSucceeded(convertElementNameFromJsonObject(elementName), message);
            else
                Q_EMIT startRequest->requestFailed(convertElementNameFromJsonObject(elementName), message);
        });

    return request;
}

std::optional<QOhosStartOptionsData> tryConvertStartRequestToQpaFunctionsStruct(
    const QOhosStartRequest &startRequest)
{
    const auto *startRequestImpl = dynamic_cast<const QOhosStartRequestImpl *>(&startRequest);
    if (startRequestImpl == nullptr) {
        qCWarning(QtForOhos, "%s: unsupported start request instance", Q_FUNC_INFO);
        return std::nullopt;
    }

    return std::make_optional(startRequestImpl->startOptions());
}

}

QT_END_NAMESPACE
