// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSTARTREQUEST_H
#define QOHOSSTARTREQUEST_H

#include <QtCore/qobject.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtOhosAppKit/qohosbundlemanager.h>
#include <QtOhosAppKit/qohosstartoptions.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

class Q_OHOSAPPKIT_EXPORT QOhosStartRequest : public QObject
{
    Q_OBJECT

public:
    ~QOhosStartRequest() override;

Q_SIGNALS:
    void requestSucceeded(QOhosElementName elementName, QString message);
    void requestFailed(QOhosElementName elementName, QString message);

protected:
    QOhosStartRequest();

private:
    Q_DISABLE_COPY(QOhosStartRequest)
};

Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosStartRequest> createStartRequest(const QOhosStartOptions &options);

}

QT_END_NAMESPACE

#endif // QOHOSSTARTREQUEST_H
