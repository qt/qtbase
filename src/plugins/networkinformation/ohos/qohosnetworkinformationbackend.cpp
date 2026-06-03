// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtCore/qglobal.h>
#include <QtNetwork/private/qnetworkinformation_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcNetInfoOhos)
Q_LOGGING_CATEGORY(lcNetInfoOhos, "qt.network.info.ohos");

namespace {

QString backendName()
{
    return QString::fromUtf16(
        QNetworkInformationBackend::PluginNames[QNetworkInformationBackend::PluginNamesOhosIndex]);
}

class QOhosNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QOhosNetworkInformationBackend();

    QString name() const override;
    QNetworkInformation::Features featuresSupported() const override;

    static QNetworkInformation::Features featuresSupportedStatic();

private:
    Q_DISABLE_COPY_MOVE(QOhosNetworkInformationBackend)
};

class QOhosNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QOhosNetworkInformationBackendFactory() = default;
    ~QOhosNetworkInformationBackendFactory() override = default;
    QString name() const override;
    QNetworkInformation::Features featuresSupported() const override;
    QNetworkInformationBackend *
    create(QNetworkInformation::Features requiredFeatures) const override;

private:
    Q_DISABLE_COPY_MOVE(QOhosNetworkInformationBackendFactory)
};

QString QOhosNetworkInformationBackend::name() const
{
    return backendName();
}

QNetworkInformation::Features QOhosNetworkInformationBackend::featuresSupported() const
{
    return featuresSupportedStatic();
}

QNetworkInformation::Features QOhosNetworkInformationBackend::featuresSupportedStatic()
{
    return {};
}

QOhosNetworkInformationBackend::QOhosNetworkInformationBackend() = default;

QString QOhosNetworkInformationBackendFactory::name() const
{
    return backendName();
}

QNetworkInformation::Features QOhosNetworkInformationBackendFactory::featuresSupported() const
{
    return QOhosNetworkInformationBackend::featuresSupportedStatic();
}

QNetworkInformationBackend *
QOhosNetworkInformationBackendFactory::create(QNetworkInformation::Features requiredFeatures) const
{
    if ((requiredFeatures & featuresSupported()) != requiredFeatures)
        return nullptr;
    return new QOhosNetworkInformationBackend();
}

}

QT_END_NAMESPACE

#include "qohosnetworkinformationbackend.moc"
