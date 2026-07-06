// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSFILESHARE_H
#define QOHOSFILESHARE_H

#include <QtCore/qlist.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtOhosAppKit/qohosoperationstatus.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace FileShare {

Q_NAMESPACE

enum class OperationMode {
    Read = 1 << 0,
    Write = 1 << 1,
};
Q_FLAG_NS(OperationMode)
Q_DECLARE_FLAGS(OperationModes, OperationMode)

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
    virtual QSharedPointer<QOhosOperationStatus> operationStatus() const = 0;
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
    virtual QSharedPointer<QOhosOperationStatus> operationStatus() const = 0;
    virtual QList<PathPolicyCheckResult> checkResultList() const = 0;

    virtual ~CheckResult();

protected:
    CheckResult();

private:
    Q_DISABLE_COPY(CheckResult)
};

Q_OHOSAPPKIT_EXPORT QSharedPointer<ActionResult> persistPermission(const QList<PathPolicy> &policies);
Q_OHOSAPPKIT_EXPORT QSharedPointer<ActionResult> revokePermission(const QList<PathPolicy> &policies);
Q_OHOSAPPKIT_EXPORT QSharedPointer<ActionResult> activatePermission(const QList<PathPolicy> &policies);
Q_OHOSAPPKIT_EXPORT QSharedPointer<ActionResult> deactivatePermission(const QList<PathPolicy> &policies);

Q_OHOSAPPKIT_EXPORT QSharedPointer<CheckResult> checkPersistent(const QList<PathPolicy> &policies);

}

}

QT_END_NAMESPACE

#endif
