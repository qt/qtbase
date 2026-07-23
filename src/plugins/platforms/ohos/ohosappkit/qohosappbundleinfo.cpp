// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosappbundleinfo_p.h"

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

using namespace Private;

namespace {

class QOhosBundleInfoImpl : public BundleInfo
{
public:
    QOhosBundleInfoImpl(int versionCode);

    int versionCode() const override;

private:
    int m_versionCode;
};

QOhosBundleInfoImpl::QOhosBundleInfoImpl(int versionCode)
    : BundleInfo()
    , m_versionCode(versionCode)
{
}

/*!
    \fn int QtOhosAppKit::BundleInfo::versionCode() const

    Returns application's version code.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-bundlemanager-bundleinfo}
    {BundleInfo's versionCode}
*/
int QOhosBundleInfoImpl::versionCode() const
{
    return m_versionCode;
}

}

/*!
    \class QtOhosAppKit::BundleInfo
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The BundleInfo class contains API to provide native application bundle info.

    To learn more about bundle info in HarmonyOS, see
    \l{https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-bundlemanager-bundleinfo}
    {Bundle Info}.
*/

BundleInfo::BundleInfo() = default;
BundleInfo::~BundleInfo() = default;

std::shared_ptr<BundleInfo> Private::createBundleInfo(int versionCode)
{
    return std::make_shared<QOhosBundleInfoImpl>(versionCode);
}

}

QT_END_NAMESPACE
