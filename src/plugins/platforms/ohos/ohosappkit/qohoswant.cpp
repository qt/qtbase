// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoswant.h"

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

/*!
    \enum QtOhosAppKit::QOhosWantFlag
    \since 5.12.12

    QOhosWantFlag specifies how the Want will be handled.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-ability-wantconstant-V5#flags}
    {Flags}.

    \value AuthReadUriPermission Grants the permission to read the URI.
    \value AuthWriteUriPermission Grants the permission to write data to the URI.
    \value InstallOnDemand Ability will be installed if it has not been installed.
*/

/*!
    \class QtOhosAppKit::QOhosWant
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief QOhosWant wraps Ohos \l {https://developer.huawei.com/consumer/en/doc/harmonyos-guides-V5/want-V5}
    {Want} class.

    It keeps information that can be transmitted between applications. One of the usage scenarios of
    QOhosWant is a parameter of starting an ability or starting application process.

    \sa startAbility()
    \sa startAppProcess()
*/

/*!
    \variable QtOhosAppKit::QOhosWant::deviceId
    \brief Represents the device Id on which Ability is to be performed.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Device Id}.
*/

/*!
    \variable QtOhosAppKit::QOhosWant::bundleName
    \brief Represents an application bundle name.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Bundle Name}.
*/

/*!
    \variable QtOhosAppKit::QOhosWant::moduleName
    \brief Represents the module name.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Module Name}.
*/

/*!
    \variable QtOhosAppKit::QOhosWant::abilityName
    \brief Represents the name of Ability to be started.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Ability Name}.
*/

/*!
    \variable QtOhosAppKit::QOhosWant::uri
    \brief Represents the type of pending data.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Uri}.
*/

/*!
    \variable QtOhosAppKit::QOhosWant::type
    \brief Represents the MIME type of the file to open for example 'text/xml', 'image/ * ', etc.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Type}.
*/

/*!
    \variable QtOhosAppKit::QOhosWant::action
    \brief Represents the general operations to be performed, for example: viewing, sharing,
    application details.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Action}.
*/

/*!
    \variable QtOhosAppKit::QOhosWant::entities
    \brief Represents the additional category information of the target Ability for example:
    browser, video player.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Entities}.
*/

/*!
    \variable QtOhosAppKit::QOhosWant::flags
    \brief Represents the way to deal with Want.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Flags}.

    \sa QOhosWantFlag
*/

/*!
    \variable QtOhosAppKit::QOhosWant::parameters
    \brief Represents the list of parameters in the Want object.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-want-V5}
    {Parameters}.
*/

/*!
    \variable QtOhosAppKit::QOhosWant::fds
    \brief Represents the file descriptor map in the Want object.
    When serialized for startAbility(), each entry is passed to the platform
    using the fixed HarmonyOS file descriptor parameter format:
    \code
    parameters[key] = { "type": "FD", "value": fd }
    \endcode
    Use application-specific keys, for example a reverse-DNS prefix, and avoid
    using the same key in both fds and parameters.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-want}
    {Fds}.
*/

/*!
    \class QtOhosAppKit::QOhosWantInfo
    \inmodule QtOhosAppKit
    \since 5.12.12

    \brief The QOhosWantInfo class is to represent Ohos want type.
*/

/*!
    \class QtOhosAppKit::QOhosWantInfo::ContactInfo
    \brief The ContactInfo struct represents contact information extracted from a Want.

    This struct is used to hold the contact type and contact ID when the application
    is launched to handle a contact-related action.
*/

/*!
    \fn QtOhosAppKit::QOhosWant QtOhosAppKit::QOhosWantInfo::want() const = 0;

    Return associated with this instance QOhosWant object.
*/

/*!
    \fn virtual QSharedPointer<QList<QSharedPointer<QtOhosAppKit::ShareKit::QOhosSharedRecord>>> QtOhosAppKit::QOhosWantInfo::tryGetSharedRecordsFromShareKit() const = 0

    Tries to get shared records from assosiated want. The shared data is expected to be stored in the want parameters.
    Shared records are delivered on an application start or while the application is already running. Returns \c nullptr if no such data found.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section661613319216}
    {systemShare.getSharedData}.

    \sa QtOhosAppKit::QOhosAbilityContext::newWantInfoReceived(QSharedPointer<QtOhosAppKit::QOhosWantInfo> wantInfo)
    \sa QSharedPointer<QtOhosAppKit::QOhosWantInfo> QtOhosAppKit::QOhosAppContext::getAppLaunchWantInfo()
*/

/*!
    \fn virtual QSharedPointer<QtOhosAppKit::QOhosWantInfo::ContactInfo> QtOhosAppKit::QOhosWantInfo::tryGetContactInfo() const = 0

    Tries to get contact information from the associated Want. Returns \c nullptr if no contact
    information is found.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section19799132782117}
    {systemShare.getContactInfo}.

    \sa QtOhosAppKit::QOhosAbilityContext::newWantInfoReceived(QSharedPointer<QtOhosAppKit::QOhosWantInfo> wantInfo)
    \sa QSharedPointer<QtOhosAppKit::QOhosWantInfo> QtOhosAppKit::QOhosAppContext::getAppLaunchWantInfo()
*/

QOhosWantInfo::QOhosWantInfo() = default;

QOhosWantInfo::~QOhosWantInfo() = default;

}

QT_END_NAMESPACE
