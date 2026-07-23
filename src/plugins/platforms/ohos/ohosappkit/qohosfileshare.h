// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSFILESHARE_H
#define QOHOSFILESHARE_H

#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtOhosAppKit/qohosoperationstatus.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace FileShare {

Q_NAMESPACE

enum class OperationMode {
    Read = 1 << 0,
    Write = 1 << 1,
};
Q_DECLARE_FLAGS(OperationModes, OperationMode)
Q_FLAG_NS(OperationModes)

Q_DECLARE_OPERATORS_FOR_FLAGS(OperationModes)

enum class PathPolicyError {
    Unknown,
    PersistenceForbidden,
    InvalidMode,
    InvalidPath,
    PermissionNotPersisted,
};

struct PathPolicy
{
    QString path;
    OperationModes operationModes;
};

struct PathPolicyErrorInfo
{
    QString path;
    PathPolicyError error = PathPolicyError::Unknown;
    QString errorMessage;
};

struct PathPolicyCheckResult
{
    PathPolicy policy;
    bool result = false;
};

class Q_OHOSAPPKIT_EXPORT ActionResult
{
public:
    virtual std::shared_ptr<OperationStatus> operationStatus() const = 0;
    virtual QList<PathPolicyErrorInfo> errorInfoList() const = 0;

    virtual ~ActionResult();

protected:
    ActionResult();

private:
    Q_DISABLE_COPY(ActionResult)
};

class Q_OHOSAPPKIT_EXPORT CheckResult
{
public:
    virtual std::shared_ptr<OperationStatus> operationStatus() const = 0;
    virtual QList<PathPolicyCheckResult> checkResultList() const = 0;

    virtual ~CheckResult();

protected:
    CheckResult();

private:
    Q_DISABLE_COPY(CheckResult)
};

Q_OHOSAPPKIT_EXPORT std::shared_ptr<ActionResult> persistPermission(const QList<PathPolicy> &policies);
Q_OHOSAPPKIT_EXPORT std::shared_ptr<ActionResult> revokePermission(const QList<PathPolicy> &policies);
Q_OHOSAPPKIT_EXPORT std::shared_ptr<ActionResult> activatePermission(const QList<PathPolicy> &policies);
Q_OHOSAPPKIT_EXPORT std::shared_ptr<ActionResult> deactivatePermission(const QList<PathPolicy> &policies);

Q_OHOSAPPKIT_EXPORT std::shared_ptr<CheckResult> checkPersistent(const QList<PathPolicy> &policies);

}

}

QT_END_NAMESPACE

#endif
